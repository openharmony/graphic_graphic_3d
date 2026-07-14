/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// MaleoonLoader - Runtime loader and trampoline for Maleoon API
//
// Sandbox-bundle mode:
//   Phase 1a: dlopen bundled core .so (libhvgr.so, fallback libmaleoon_v300.so),
//             dlsym MlnCreateDevice + MlnGetDeviceProcAddr as bootstrap entries.
//   Phase 1b: dlopen bundled swapchain .so (libswapchain.so),
//             dlsym MlnGetDeviceProcAddr as swap-specific proc source.
//   Phase 2: resolve all core functions via core MlnGetDeviceProcAddr(device, name).
//   Phase 3: resolve 11 swapchain functions:
//            dlsym(swapLib, name) → core GetDeviceProcAddr(device, name)
//            → swap GetDeviceProcAddr(device, name).
//            Swap proc enabled by default (core only resolves 2/11).
//            Env MLN_SWAP_PROC_ENABLE=0 to disable for diagnostics.
// All .so files are expected in the HAP sandbox (same directory as the AGP render .so).
// Exports all MlnXxx functions as trampolines for AGP to link against.

#ifdef _WIN32
#ifdef interface
#undef interface
#endif
#endif

#include <maleoon/maleoon.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <securec.h>

#include <atomic>

// ============================================================================
// Export macro
// ============================================================================

#ifdef _WIN32
#define MLN_LOADER_EXPORT extern "C" __declspec(dllexport)
#else
#define MLN_LOADER_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// ============================================================================
// PFN typedefs for functions NOT in SDK maleoon_func_ptrs.h
// ============================================================================

// Async program creation (SDK func_ptrs.h lacks PFN_ for these)
typedef MlnProgram(MLNAPI_PTR* PFN_MlnCreateGraphicsProgramAsync)(
    MlnDevice, MlnProgramCache, const MlnGraphicsProgramState*, const MlnPriority);
typedef MlnProgram(MLNAPI_PTR* PFN_MlnCreateComputeProgramAsync)(
    MlnDevice, MlnProgramCache, const MlnComputeProgramState*, const MlnPriority);

// Sync functions missing from SDK func_ptrs.h
typedef MlnStatus(MLNAPI_PTR* PFN_MlnWaitForTimelines)(MlnDevice, const MlnTimelineWaitDescriptor*, u64);
typedef MlnStatus(MLNAPI_PTR* PFN_MlnGetTimelineValue)(MlnDevice, MlnTimeline, u64*);
typedef MlnStatus(MLNAPI_PTR* PFN_MlnExportTimelineFd)(MlnDevice, const MlnTimelineExportFdDescriptor*, int*);
typedef MlnStatus(MLNAPI_PTR* PFN_MlnImportTimelineFd)(MlnDevice, const MlnTimelineImportFdDescriptor*);
typedef MlnStatus(MLNAPI_PTR* PFN_MlnResetTimeline)(MlnDevice, MlnTimeline, u64);
typedef void(MLNAPI_PTR* PFN_MlnDestroyTimeline)(MlnDevice, MlnTimeline);

// Presentation functions (not in SDK func_ptrs.h)
typedef MlnRenderTarget(MLNAPI_PTR* PFN_MlnCreateRenderTarget)(MlnDevice, const MlnRenderTargetDescriptor*);
typedef void(MLNAPI_PTR* PFN_MlnDestroyRenderTarget)(MlnDevice, MlnRenderTarget);
typedef MlnDisplaySurface(MLNAPI_PTR* PFN_MlnCreateDisplaySurface)(MlnDevice, const MlnDisplaySurfaceDescriptor*);
typedef MlnStatus(MLNAPI_PTR* PFN_MlnGetDisplaySurfaceCapabilities)(
    MlnDevice, MlnDisplaySurface, MlnDisplaySurfaceCapabilities*);
// Swapchain / Display (PFN signatures now match maleoon_func_ptrs.h in 0324 headers)
typedef void(MLNAPI_PTR* PFN_MlnDestroyDisplaySurface)(MlnDevice, MlnDisplaySurface);
typedef MlnSwapchain(MLNAPI_PTR* PFN_MlnCreateSwapchain)(MlnDevice, const MlnSwapchainDescriptor*);
typedef MlnStatus(MLNAPI_PTR* PFN_MlnGetSwapchainImages)(MlnDevice, MlnSwapchain, u32*, MlnResource*);
typedef void(MLNAPI_PTR* PFN_MlnDestroySwapchain)(MlnDevice, MlnSwapchain);
typedef MlnStatus(MLNAPI_PTR* PFN_MlnPresentSwapchain)(MlnQueue, const MlnPresentDescriptor*);

// Extension / Helper (optional, may not exist in system loader)
typedef void (*PFN_MlnSetResourceDebugName)(MlnDevice, MlnResource, const char*);

// ============================================================================
// Log callback: routes loader messages through hilog (set by device_mln.cpp)
// fprintf(stderr) is invisible on OHOS; this callback makes init visible.
// ============================================================================
typedef void (*MlnLoaderLogFn)(const char* message);
static MlnLoaderLogFn s_logFn = nullptr;

// Dual-output log: stderr + hilog (if s_logFn set by device_mln.cpp before init)
static thread_local char s_lbuf[512];
#define LLOG(fmt, ...)                                                                        \
    do {                                                                                      \
        if (snprintf_s(s_lbuf, sizeof(s_lbuf), sizeof(s_lbuf) - 1, fmt, ##__VA_ARGS__) < 0) { \
            s_lbuf[0] = '\0';                                                                 \
        }                                                                                     \
        fprintf(stderr, "%s\n", s_lbuf);                                                      \
        if (s_logFn)                                                                          \
            s_logFn(s_lbuf);                                                                  \
    } while (0)

// ============================================================================
// Internal state
// ============================================================================

static void* s_coreLib = nullptr;         // libmaleoon_loader.so (unified) or libhvgr.so/libmaleoon_v300.so (split)
static void* s_swapLib = nullptr;         // libswapchain.so (only used in split mode, nullptr in unified mode)
static bool s_unifiedLoaderMode = false;  // true when using libmaleoon_loader.so (skip Phase 1b)

// Bootstrap entries from core .so
static PFN_MlnGetDeviceProcAddr s_bootstrapGetDeviceProcAddr = nullptr;

// Swapchain-specific GetDeviceProcAddr (from libswapchain.so, if available)
static PFN_MlnGetDeviceProcAddr s_swapGetDeviceProcAddr = nullptr;

// Env-gated swap proc switch (MLN_SWAP_PROC_ENABLE=1), set in Phase 3
static bool s_swapProcEnabled = false;

// Probe diagnostic state (device_mln.cpp retrieves via mlnLoaderGetProbeInfo)
static int s_probeCoreResolvedCount = 0;
static int s_probeCoreTotalCount = 0;
static int s_probeCoreMissing = -1;
static void* s_probeMlnCreateDevice = nullptr;
static void* s_probeMlnGetDeviceProcAddr = nullptr;
static void* s_probeBootstrapEntry = nullptr;
static int s_probeSwapResolvedCount = 0;
static int s_probeSwapTotalCount = 0;

// Reference count: tracks how many DeviceMln instances are using the loader.
// Prevents mlnLoaderDeinit from clearing function pointers while other
// DeviceMln instances still need them (e.g. during scene switches between
// WidgetAdapter and SceneAdapter paths).
static std::atomic<int> s_loaderRefCount{0};

// ============================================================================
// Function pointers - core
// ============================================================================

// Device
static PFN_MlnCreateDevice s_MlnCreateDevice;
static PFN_MlnDestroyDevice s_MlnDestroyDevice;
static PFN_MlnDeviceWaitIdle s_MlnDeviceWaitIdle;
static PFN_MlnGetDeviceProcAddr s_MlnGetDeviceProcAddr;
static PFN_MlnGetGpuFeatures s_MlnGetGpuFeatures;
static PFN_MlnGetGpuProperties s_MlnGetGpuProperties;
static PFN_MlnGetGpuFormatProperties s_MlnGetGpuFormatProperties;
static PFN_MlnGetGpuImageFormatProperties s_MlnGetGpuImageFormatProperties;
static PFN_MlnGetDeviceQueue s_MlnGetDeviceQueue;
static PFN_MlnQueueSubmit s_MlnQueueSubmit;
static PFN_MlnQueueWaitIdle s_MlnQueueWaitIdle;
static PFN_MlnQueueSetPriority s_MlnQueueSetPriority;
static PFN_MlnQueueGetPriority s_MlnQueueGetPriority;

// Memory
static PFN_MlnGetGpuMemoryProperties s_MlnGetGpuMemoryProperties;
static PFN_MlnGetResourceMemoryRequirements s_MlnGetResourceMemoryRequirements;
static PFN_MlnGetBufferResourceMemoryRequirements s_MlnGetBufferResourceMemoryRequirements;
static PFN_MlnGetImageResourceMemoryRequirements s_MlnGetImageResourceMemoryRequirements;
static PFN_MlnAllocateMemory s_MlnAllocateMemory;
static PFN_MlnFreeMemory s_MlnFreeMemory;
static PFN_MlnMapMemory s_MlnMapMemory;
static PFN_MlnUnmapMemory s_MlnUnmapMemory;
static PFN_MlnFlushMappedMemoryRanges s_MlnFlushMappedMemoryRanges;
static PFN_MlnInvalidateMappedMemoryRanges s_MlnInvalidateMappedMemoryRanges;

// Resource
static PFN_MlnCreateBufferResource s_MlnCreateBufferResource;
static PFN_MlnCreateImageResource s_MlnCreateImageResource;
static PFN_MlnBindResourceMemory s_MlnBindResourceMemory;
static PFN_MlnDestroyResource s_MlnDestroyResource;
static PFN_MlnCreateBufferResourceView s_MlnCreateBufferResourceView;
static PFN_MlnCreateImageResourceView s_MlnCreateImageResourceView;
static PFN_MlnDestroyResourceView s_MlnDestroyResourceView;
static PFN_MlnCreateSampler s_MlnCreateSampler;
static PFN_MlnDestroySampler s_MlnDestroySampler;

// Shader / Program
static PFN_MlnCreateProgramCache s_MlnCreateProgramCache;
static PFN_MlnGetProgramCacheData s_MlnGetProgramCacheData;
static PFN_MlnMergeProgramCaches s_MlnMergeProgramCaches;
static PFN_MlnDestroyProgramCache s_MlnDestroyProgramCache;
static PFN_MlnCreateProgramInterface s_MlnCreateProgramInterface;
static PFN_MlnDestroyProgramInterface s_MlnDestroyProgramInterface;
static PFN_MlnCreateGraphicsProgram s_MlnCreateGraphicsProgram;
static PFN_MlnCreateComputeProgram s_MlnCreateComputeProgram;
static PFN_MlnCreateGraphicsProgramAsync s_MlnCreateGraphicsProgramAsync;
static PFN_MlnCreateComputeProgramAsync s_MlnCreateComputeProgramAsync;
static PFN_MlnGetCreatedProgramAsyncResult s_MlnGetCreatedProgramAsyncResult;
static PFN_MlnWaitCreatedProgramAsyncResult s_MlnWaitCreatedProgramAsyncResult;
static PFN_MlnDestroyProgram s_MlnDestroyProgram;

// Binding
static PFN_MlnCreateBindingLayout s_MlnCreateBindingLayout;
static PFN_MlnDestroyBindingLayout s_MlnDestroyBindingLayout;
static PFN_MlnCreateBindingSet s_MlnCreateBindingSet;
static PFN_MlnUpdateBindingSets s_MlnUpdateBindingSets;
static PFN_MlnDestroyBindingSet s_MlnDestroyBindingSet;

// Execution
static PFN_MlnCreateGraphicsObjectGroup s_MlnCreateGraphicsObjectGroup;
static PFN_MlnUpdateGraphicsObjectGroup s_MlnUpdateGraphicsObjectGroup;
static PFN_MlnCreateComputeObjectGroup s_MlnCreateComputeObjectGroup;
static PFN_MlnUpdateComputeObjectGroup s_MlnUpdateComputeObjectGroup;
static PFN_MlnCreateTransferObjectGroup s_MlnCreateTransferObjectGroup;
static PFN_MlnCreateGraphicsDataGraph s_MlnCreateGraphicsDataGraph;
static PFN_MlnCreateComputeDataGraph s_MlnCreateComputeDataGraph;
static PFN_MlnCreateTransferDataGraph s_MlnCreateTransferDataGraph;
static PFN_MlnCreateSchedulingGraph s_MlnCreateSchedulingGraph;
static PFN_MlnDestroySchedulingGraph s_MlnDestroySchedulingGraph;
static PFN_MlnDestroyObjectGroup s_MlnDestroyObjectGroup;
static PFN_MlnDestroyDataGraph s_MlnDestroyDataGraph;

// Sync
static PFN_MlnCreateTimeline s_MlnCreateTimeline;
static PFN_MlnSignalTimelines s_MlnSignalTimelines;
static PFN_MlnWaitForTimelines s_MlnWaitForTimelines;
static PFN_MlnGetTimelineValue s_MlnGetTimelineValue;
static PFN_MlnExportTimelineFd s_MlnExportTimelineFd;
static PFN_MlnImportTimelineFd s_MlnImportTimelineFd;
static PFN_MlnResetTimeline s_MlnResetTimeline;
static PFN_MlnDestroyTimeline s_MlnDestroyTimeline;

// Extension / Helper (optional)
static PFN_MlnSetResourceDebugName s_MlnSetResourceDebugName;

// Ray Tracing (Maleoon 0416 header additions)
//   Inline ray-query path: AS build + device address + capabilities.
//   Ray tracing *pipeline* (rayGen/miss/chit) + SBT are intentionally NOT included
//   at this stage (AGP RT direction decision — rayquery only).
static PFN_MlnGetBufferResourceAddress s_MlnGetBufferResourceAddress;
static PFN_MlnCreateAccelerationStructure s_MlnCreateAccelerationStructure;
static PFN_MlnDestroyAccelerationStructure s_MlnDestroyAccelerationStructure;
static PFN_MlnGetAccelerationStructureDeviceAddress s_MlnGetAccelerationStructureDeviceAddress;
static PFN_MlnGetAccelerationStructureBuildSizes s_MlnGetAccelerationStructureBuildSizes;
static PFN_MlnCreateAccelerationStructureObjectGroup s_MlnCreateAccelerationStructureObjectGroup;
static PFN_MlnCreateAccelerationStructureDataGraph s_MlnCreateAccelerationStructureDataGraph;
static PFN_MlnGetRayTracingCapabilities s_MlnGetRayTracingCapabilities;
static PFN_MlnCheckAccelerationStructureCompatibility s_MlnCheckAccelerationStructureCompatibility;

// ============================================================================
// Function pointers - presentation (swapchain)
// ============================================================================

static PFN_MlnCreateRenderTarget s_MlnCreateRenderTarget;
static PFN_MlnDestroyRenderTarget s_MlnDestroyRenderTarget;
static PFN_MlnCreateDisplaySurface s_MlnCreateDisplaySurface;
static PFN_MlnGetDisplaySurfaceCapabilities s_MlnGetDisplaySurfaceCapabilities;
static PFN_MlnGetDisplaySurfaceFormats s_MlnGetDisplaySurfaceFormats;
static PFN_MlnGetDisplaySurfacePresentModes s_MlnGetDisplaySurfacePresentModes;
static PFN_MlnDestroyDisplaySurface s_MlnDestroyDisplaySurface;
static PFN_MlnCreateSwapchain s_MlnCreateSwapchain;
static PFN_MlnAcquireNextImage s_MlnAcquireNextImage;
static PFN_MlnGetSwapchainImages s_MlnGetSwapchainImages;
static PFN_MlnDestroySwapchain s_MlnDestroySwapchain;
static PFN_MlnPresentSwapchain s_MlnPresentSwapchain;

// ============================================================================
// Platform helpers
// ============================================================================

#ifndef _WIN32
static void* LoadLib(const char* path)
{
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}

static void* GetSym(void* lib, const char* name)
{
    return dlsym(lib, name);
}

static void FreeLib(void* lib)
{
    dlclose(lib);
}

static const char* GetLibError()
{
    return dlerror();
}
#endif

// ============================================================================
// Loader init
//   Strategy: try system libmaleoon_loader.so first (unified dispatcher that
//   internally loads core + swapchain with correct linker namespace and Gralloc
//   HDI init). If unavailable, fall back to split mode: load core + swapchain
//   separately.
// ============================================================================

MLN_LOADER_EXPORT bool MLNAPI_CALL mlnLoaderInit(const char* /*corePath*/, const char* /*swapchainPath*/)
{
    int prev = s_loaderRefCount.fetch_add(1);
    if (prev > 0) {
        LLOG("[MaleoonLoader] mlnLoaderInit: already loaded (refCount=%d), skipping dlopen", prev + 1);
        return true;
    }

#ifdef _WIN32
    LLOG("[MaleoonLoader] ERROR: sandbox-bundle mode not supported on Windows");
    s_loaderRefCount.fetch_sub(1);
    return false;
#else
    // -- Phase 1a: try unified libmaleoon_loader.so first --
    // The system loader internally manages core + swapchain loading with correct
    // linker namespace, Gralloc HDI permissions, and internal state coupling.
    // When using the unified loader, Phase 1b is skipped entirely — all functions
    // (core + swapchain) resolve through the loader's MlnGetDeviceProcAddr.
    static const char* loaderCandidates[] = {
        "libmaleoon.so",  // HAP bundle (preferred)
        "/system/lib64/libmaleoon.so",
        "/vendor/lib64/libmaleoon.so",
        "/vendor/lib64/passthrough/libmaleoon.so",
        "/vendor/lib64/chipsetsdk/libmaleoon.so",
    };
    for (auto* path : loaderCandidates) {
        LLOG("[MaleoonLoader] Phase 1a(unified): trying dlopen('%s') ...", path);
        void* lib = LoadLib(path);
        if (!lib) {
            LLOG("[MaleoonLoader] Phase 1a(unified): dlopen('%s') FAILED: %s", path, GetLibError());
            continue;
        }
        LLOG("[MaleoonLoader] Phase 1a(unified): dlopen('%s') OK, handle=%p", path, lib);

        auto fnGetProc = (PFN_MlnGetDeviceProcAddr)GetSym(lib, "MlnGetDeviceProcAddr");
        auto fnCreate = (PFN_MlnCreateDevice)GetSym(lib, "MlnCreateDevice");
        LLOG("[MaleoonLoader] Phase 1a(unified): MlnCreateDevice=%p  MlnGetDeviceProcAddr=%p",
            reinterpret_cast<void*>(fnCreate),    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<void*>(fnGetProc));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

        if (!fnCreate && fnGetProc) {
            fnCreate = (PFN_MlnCreateDevice)fnGetProc(nullptr, "MlnCreateDevice");
            LLOG("[MaleoonLoader] Phase 1a(unified): GetDeviceProcAddr(nullptr,'MlnCreateDevice')=%p",
                reinterpret_cast<void*>(fnCreate));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }

        if (fnCreate && fnGetProc) {
            s_coreLib = lib;
            s_MlnCreateDevice = fnCreate;
            s_bootstrapGetDeviceProcAddr = fnGetProc;
            s_unifiedLoaderMode = true;
            LLOG("[MaleoonLoader] Phase 1a(unified): OK via '%s' — UNIFIED mode, Phase 1b skipped", path);
            break;
        }

        LLOG("[MaleoonLoader] Phase 1a(unified): '%s' bootstrap incomplete -- skipping", path);
        FreeLib(lib);
    }

    // -- Phase 1a fallback: split mode (core + swapchain separately) --
    if (!s_coreLib) {
        LLOG("[MaleoonLoader] Phase 1a(unified): no loader found, trying SPLIT mode");
        static const char* coreCandidates[] = {
            "/vendor/lib64/passthrough/libmaleoon_v300.so",
            "libhvgr.so",          // bundle fallback (SONAME=libhvgr)
            "libmaleoon_v300.so",  // bundle fallback
        };
        const char* corePath = nullptr;
        for (auto* path : coreCandidates) {
            LLOG("[MaleoonLoader] Phase 1a(split): trying dlopen('%s') ...", path);
            void* lib = LoadLib(path);
            if (!lib) {
                LLOG("[MaleoonLoader] Phase 1a(split): dlopen('%s') FAILED: %s", path, GetLibError());
                continue;
            }
            LLOG("[MaleoonLoader] Phase 1a(split): dlopen('%s') OK, handle=%p", path, lib);

            auto fnGetProc = (PFN_MlnGetDeviceProcAddr)GetSym(lib, "MlnGetDeviceProcAddr");
            auto fnCreate = (PFN_MlnCreateDevice)GetSym(lib, "MlnCreateDevice");
            LLOG("[MaleoonLoader] Phase 1a(split): MlnCreateDevice=%p  MlnGetDeviceProcAddr=%p",
                reinterpret_cast<void*>(fnCreate),    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                reinterpret_cast<void*>(fnGetProc));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

            if (!fnCreate && fnGetProc) {
                fnCreate = (PFN_MlnCreateDevice)fnGetProc(nullptr, "MlnCreateDevice");
                LLOG("[MaleoonLoader] Phase 1a(split): GetDeviceProcAddr(nullptr,'MlnCreateDevice')=%p",
                    reinterpret_cast<void*>(fnCreate));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            }

            if (fnCreate && fnGetProc) {
                s_coreLib = lib;
                s_MlnCreateDevice = fnCreate;
                s_bootstrapGetDeviceProcAddr = fnGetProc;
                corePath = path;
                break;
            }

            LLOG("[MaleoonLoader] Phase 1a(split): '%s' bootstrap incomplete -- skipping", path);
            FreeLib(lib);
        }
        if (!s_coreLib) {
            LLOG("[MaleoonLoader] Phase 1a FAILED: no loader or core .so with valid bootstrap");
            s_loaderRefCount.fetch_sub(1);
            return false;
        }
        LLOG("[MaleoonLoader] Phase 1a(split): OK via '%s'", corePath);
    }

    s_probeMlnCreateDevice =
        reinterpret_cast<void*>(s_MlnCreateDevice);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    s_probeMlnGetDeviceProcAddr =
        reinterpret_cast<void*>(s_bootstrapGetDeviceProcAddr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    s_probeBootstrapEntry =
        reinterpret_cast<void*>(s_bootstrapGetDeviceProcAddr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    // -- Phase 1b: load swapchain .so (SPLIT mode only) --
    // In unified mode, the loader handles swapchain internally — skip Phase 1b.
    if (s_unifiedLoaderMode) {
        LLOG("[MaleoonLoader] Phase 1b: SKIPPED (unified loader mode)");
    } else {
        static const char* swapCandidates[] = {
            "/system/lib64/libswapchain.so",
            "/vendor/lib64/libswapchain.so",
            "/system/lib64/chipset-pub-sdk/libswapchain.so",
            "libswapchain.so",  // fallback: bare name → HAP bundle or default search
        };
        const char* swapSource = nullptr;
        for (auto* swapPath : swapCandidates) {
            LLOG("[MaleoonLoader] Phase 1b: trying dlopen('%s') ...", swapPath);
            void* lib = LoadLib(swapPath);
            if (lib) {
                LLOG("[MaleoonLoader] Phase 1b: dlopen('%s') OK, handle=%p", swapPath, lib);
                s_swapLib = lib;
                swapSource = swapPath;
                break;
            }
            LLOG("[MaleoonLoader] Phase 1b: dlopen('%s') FAILED: %s", swapPath, GetLibError());
        }
        if (s_swapLib) {
            s_swapGetDeviceProcAddr = (PFN_MlnGetDeviceProcAddr)GetSym(s_swapLib, "MlnGetDeviceProcAddr");
            LLOG("[MaleoonLoader] Phase 1b: OK via '%s', swap MlnGetDeviceProcAddr=%p",
                swapSource,
                reinterpret_cast<void*>(
                    s_swapGetDeviceProcAddr));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        } else {
            LLOG("[MaleoonLoader] Phase 1b: ALL swapchain candidates FAILED (swapchain may fail later)");
        }
    }

    return true;
#endif
}

// ============================================================================
// Native mode: true when core .so is loaded (bundled)
// ============================================================================

MLN_LOADER_EXPORT bool MLNAPI_CALL mlnLoaderIsNativeMode()
{
    return s_coreLib != nullptr;
}

// ============================================================================
// Deinit: unload bundled core + swapchain .so
// ============================================================================

MLN_LOADER_EXPORT void MLNAPI_CALL mlnLoaderDeinit()
{
    int prev = s_loaderRefCount.fetch_sub(1);
    if (prev <= 0) {
        // Unbalanced deinit — reset to 0 and return
        s_loaderRefCount.store(0);
        LLOG("[MaleoonLoader] mlnLoaderDeinit: unbalanced call (refCount was %d), ignoring", prev);
        return;
    }
    if (prev > 1) {
        LLOG("[MaleoonLoader] mlnLoaderDeinit: refCount=%d, keeping library loaded", prev - 1);
        return;
    }
    // refCount reached 0 — perform full cleanup
    LLOG("[MaleoonLoader] mlnLoaderDeinit: refCount=0, performing full cleanup");
    if (s_swapLib) {
        FreeLib(s_swapLib);
        s_swapLib = nullptr;
    }
    if (s_coreLib) {
        FreeLib(s_coreLib);
        s_coreLib = nullptr;
    }
    s_bootstrapGetDeviceProcAddr = nullptr;
    s_swapGetDeviceProcAddr = nullptr;
    s_swapProcEnabled = false;
    s_unifiedLoaderMode = false;

    // Clear ALL function pointers to prevent dangling calls after dlclose
    // Device
    s_MlnCreateDevice = nullptr;
    s_MlnDestroyDevice = nullptr;
    s_MlnDeviceWaitIdle = nullptr;
    s_MlnGetDeviceProcAddr = nullptr;
    s_MlnGetGpuFeatures = nullptr;
    s_MlnGetGpuProperties = nullptr;
    s_MlnGetGpuFormatProperties = nullptr;
    s_MlnGetGpuImageFormatProperties = nullptr;
    s_MlnGetDeviceQueue = nullptr;
    s_MlnQueueSubmit = nullptr;
    s_MlnQueueWaitIdle = nullptr;
    s_MlnQueueSetPriority = nullptr;
    s_MlnQueueGetPriority = nullptr;
    // Memory
    s_MlnGetGpuMemoryProperties = nullptr;
    s_MlnGetResourceMemoryRequirements = nullptr;
    s_MlnGetBufferResourceMemoryRequirements = nullptr;
    s_MlnGetImageResourceMemoryRequirements = nullptr;
    s_MlnAllocateMemory = nullptr;
    s_MlnFreeMemory = nullptr;
    s_MlnMapMemory = nullptr;
    s_MlnUnmapMemory = nullptr;
    s_MlnFlushMappedMemoryRanges = nullptr;
    s_MlnInvalidateMappedMemoryRanges = nullptr;
    // Resource
    s_MlnCreateBufferResource = nullptr;
    s_MlnCreateImageResource = nullptr;
    s_MlnBindResourceMemory = nullptr;
    s_MlnDestroyResource = nullptr;
    s_MlnCreateBufferResourceView = nullptr;
    s_MlnCreateImageResourceView = nullptr;
    s_MlnDestroyResourceView = nullptr;
    s_MlnCreateSampler = nullptr;
    s_MlnDestroySampler = nullptr;
    // Shader / Program
    s_MlnCreateProgramCache = nullptr;
    s_MlnGetProgramCacheData = nullptr;
    s_MlnMergeProgramCaches = nullptr;
    s_MlnDestroyProgramCache = nullptr;
    s_MlnCreateProgramInterface = nullptr;
    s_MlnDestroyProgramInterface = nullptr;
    s_MlnCreateGraphicsProgram = nullptr;
    s_MlnCreateComputeProgram = nullptr;
    s_MlnCreateGraphicsProgramAsync = nullptr;
    s_MlnCreateComputeProgramAsync = nullptr;
    s_MlnGetCreatedProgramAsyncResult = nullptr;
    s_MlnWaitCreatedProgramAsyncResult = nullptr;
    s_MlnDestroyProgram = nullptr;
    // Binding
    s_MlnCreateBindingLayout = nullptr;
    s_MlnDestroyBindingLayout = nullptr;
    s_MlnCreateBindingSet = nullptr;
    s_MlnUpdateBindingSets = nullptr;
    s_MlnDestroyBindingSet = nullptr;
    // Execution
    s_MlnCreateGraphicsObjectGroup = nullptr;
    s_MlnUpdateGraphicsObjectGroup = nullptr;
    s_MlnCreateComputeObjectGroup = nullptr;
    s_MlnUpdateComputeObjectGroup = nullptr;
    s_MlnCreateTransferObjectGroup = nullptr;
    s_MlnCreateGraphicsDataGraph = nullptr;
    s_MlnCreateComputeDataGraph = nullptr;
    s_MlnCreateTransferDataGraph = nullptr;
    s_MlnCreateSchedulingGraph = nullptr;
    s_MlnDestroySchedulingGraph = nullptr;
    s_MlnDestroyObjectGroup = nullptr;
    s_MlnDestroyDataGraph = nullptr;
    // Sync
    s_MlnCreateTimeline = nullptr;
    s_MlnSignalTimelines = nullptr;
    s_MlnWaitForTimelines = nullptr;
    s_MlnGetTimelineValue = nullptr;
    s_MlnExportTimelineFd = nullptr;
    s_MlnImportTimelineFd = nullptr;
    s_MlnResetTimeline = nullptr;
    s_MlnDestroyTimeline = nullptr;
    // Extension
    s_MlnSetResourceDebugName = nullptr;
    // Ray Tracing
    s_MlnGetBufferResourceAddress = nullptr;
    s_MlnCreateAccelerationStructure = nullptr;
    s_MlnDestroyAccelerationStructure = nullptr;
    s_MlnGetAccelerationStructureDeviceAddress = nullptr;
    s_MlnGetAccelerationStructureBuildSizes = nullptr;
    s_MlnCreateAccelerationStructureObjectGroup = nullptr;
    s_MlnCreateAccelerationStructureDataGraph = nullptr;
    s_MlnGetRayTracingCapabilities = nullptr;
    s_MlnCheckAccelerationStructureCompatibility = nullptr;
    // Presentation
    s_MlnCreateRenderTarget = nullptr;
    s_MlnDestroyRenderTarget = nullptr;
    s_MlnCreateDisplaySurface = nullptr;
    s_MlnGetDisplaySurfaceCapabilities = nullptr;
    s_MlnGetDisplaySurfaceFormats = nullptr;
    s_MlnGetDisplaySurfacePresentModes = nullptr;
    s_MlnDestroyDisplaySurface = nullptr;
    s_MlnCreateSwapchain = nullptr;
    s_MlnAcquireNextImage = nullptr;
    s_MlnGetSwapchainImages = nullptr;
    s_MlnDestroySwapchain = nullptr;
    s_MlnPresentSwapchain = nullptr;

    // Clear probe state
    s_probeCoreMissing = -1;
    s_probeMlnCreateDevice = nullptr;
    s_probeMlnGetDeviceProcAddr = nullptr;
    s_probeBootstrapEntry = nullptr;
    s_probeCoreResolvedCount = 0;
    s_probeCoreTotalCount = 0;
    s_probeSwapResolvedCount = 0;
    s_probeSwapTotalCount = 0;
}

// ============================================================================
// Log callback
// ============================================================================

MLN_LOADER_EXPORT void MLNAPI_CALL mlnLoaderSetLogFn(void (*fn)(const char*))
{
    s_logFn = fn;
}

// ============================================================================
// Phase 2: resolve all core functions using MlnGetDeviceProcAddr(device, name)
// Called by device_mln.cpp AFTER MlnCreateDevice succeeds.
// ============================================================================

MLN_LOADER_EXPORT bool MLNAPI_CALL mlnLoaderResolveWithDevice(MlnDevice device)
{
    if (!s_bootstrapGetDeviceProcAddr) {
        LLOG("[MaleoonLoader] Phase 2: FATAL -- no bootstrap GetDeviceProcAddr");
        return false;
    }
    if (!device) {
        LLOG("[MaleoonLoader] Phase 2: FATAL -- null device handle");
        return false;
    }

    LLOG("[MaleoonLoader] Phase 2: resolving core with device=%p", static_cast<const void*>(device));

    int coreResolved = 0, coreTotal = 0;
    char p2buf[128];

#define LOAD_CORE_P2(name)                                                                     \
    do {                                                                                       \
        if (s_logFn) {                                                                         \
            if (snprintf_s(p2buf, sizeof(p2buf), sizeof(p2buf) - 1, "P2: %s...", #name) < 0) { \
                p2buf[0] = '\0';                                                               \
            }                                                                                  \
            s_logFn(p2buf);                                                                    \
        }                                                                                      \
        s_##name = (PFN_##name)s_bootstrapGetDeviceProcAddr(device, #name);                    \
        coreTotal++;                                                                           \
        if (s_##name) {                                                                        \
            coreResolved++;                                                                    \
        } else {                                                                               \
            LLOG("[MaleoonLoader] P2 NULL: %s", #name);                                        \
        }                                                                                      \
    } while (0)
    // Device (MlnCreateDevice already resolved in Phase 1)
    LOAD_CORE_P2(MlnDestroyDevice);
    LOAD_CORE_P2(MlnDeviceWaitIdle);
    LOAD_CORE_P2(MlnGetDeviceProcAddr);
    LOAD_CORE_P2(MlnGetGpuFeatures);
    LOAD_CORE_P2(MlnGetGpuProperties);
    LOAD_CORE_P2(MlnGetGpuFormatProperties);
    LOAD_CORE_P2(MlnGetGpuImageFormatProperties);
    LOAD_CORE_P2(MlnGetDeviceQueue);
    LOAD_CORE_P2(MlnQueueSubmit);
    LOAD_CORE_P2(MlnQueueWaitIdle);
    LOAD_CORE_P2(MlnQueueSetPriority);
    LOAD_CORE_P2(MlnQueueGetPriority);

    // Memory
    LOAD_CORE_P2(MlnGetGpuMemoryProperties);
    LOAD_CORE_P2(MlnGetResourceMemoryRequirements);
    LOAD_CORE_P2(MlnGetBufferResourceMemoryRequirements);
    LOAD_CORE_P2(MlnGetImageResourceMemoryRequirements);
    LOAD_CORE_P2(MlnAllocateMemory);
    LOAD_CORE_P2(MlnFreeMemory);
    LOAD_CORE_P2(MlnMapMemory);
    LOAD_CORE_P2(MlnUnmapMemory);
    LOAD_CORE_P2(MlnFlushMappedMemoryRanges);
    LOAD_CORE_P2(MlnInvalidateMappedMemoryRanges);

    // Resource
    LOAD_CORE_P2(MlnCreateBufferResource);
    LOAD_CORE_P2(MlnCreateImageResource);
    LOAD_CORE_P2(MlnBindResourceMemory);
    LOAD_CORE_P2(MlnDestroyResource);
    LOAD_CORE_P2(MlnCreateBufferResourceView);
    LOAD_CORE_P2(MlnCreateImageResourceView);
    LOAD_CORE_P2(MlnDestroyResourceView);
    LOAD_CORE_P2(MlnCreateSampler);
    LOAD_CORE_P2(MlnDestroySampler);

    // Shader / Program
    LOAD_CORE_P2(MlnCreateProgramCache);
    LOAD_CORE_P2(MlnGetProgramCacheData);
    LOAD_CORE_P2(MlnMergeProgramCaches);
    LOAD_CORE_P2(MlnDestroyProgramCache);
    LOAD_CORE_P2(MlnCreateProgramInterface);
    LOAD_CORE_P2(MlnDestroyProgramInterface);
    LOAD_CORE_P2(MlnCreateGraphicsProgram);
    LOAD_CORE_P2(MlnCreateComputeProgram);
    LOAD_CORE_P2(MlnCreateGraphicsProgramAsync);
    LOAD_CORE_P2(MlnCreateComputeProgramAsync);
    LOAD_CORE_P2(MlnGetCreatedProgramAsyncResult);
    LOAD_CORE_P2(MlnWaitCreatedProgramAsyncResult);
    LOAD_CORE_P2(MlnDestroyProgram);

    // Binding
    LOAD_CORE_P2(MlnCreateBindingLayout);
    LOAD_CORE_P2(MlnDestroyBindingLayout);
    LOAD_CORE_P2(MlnCreateBindingSet);
    LOAD_CORE_P2(MlnUpdateBindingSets);
    LOAD_CORE_P2(MlnDestroyBindingSet);

    // Execution
    LOAD_CORE_P2(MlnCreateGraphicsObjectGroup);
    LOAD_CORE_P2(MlnUpdateGraphicsObjectGroup);
    LOAD_CORE_P2(MlnCreateComputeObjectGroup);
    LOAD_CORE_P2(MlnUpdateComputeObjectGroup);
    LOAD_CORE_P2(MlnCreateTransferObjectGroup);
    LOAD_CORE_P2(MlnCreateGraphicsDataGraph);
    LOAD_CORE_P2(MlnCreateComputeDataGraph);
    LOAD_CORE_P2(MlnCreateTransferDataGraph);
    LOAD_CORE_P2(MlnCreateSchedulingGraph);
    LOAD_CORE_P2(MlnDestroySchedulingGraph);
    LOAD_CORE_P2(MlnDestroyObjectGroup);
    LOAD_CORE_P2(MlnDestroyDataGraph);

    // Sync
    LOAD_CORE_P2(MlnCreateTimeline);
    LOAD_CORE_P2(MlnSignalTimelines);
    LOAD_CORE_P2(MlnWaitForTimelines);
    LOAD_CORE_P2(MlnGetTimelineValue);
    LOAD_CORE_P2(MlnExportTimelineFd);
    LOAD_CORE_P2(MlnImportTimelineFd);
    LOAD_CORE_P2(MlnResetTimeline);
    LOAD_CORE_P2(MlnDestroyTimeline);

    // Extension (optional)
    LOAD_CORE_P2(MlnSetResourceDebugName);

    // Ray Tracing (optional on legacy drivers; unresolved => RT support flag stays false)
    LOAD_CORE_P2(MlnGetBufferResourceAddress);
    LOAD_CORE_P2(MlnCreateAccelerationStructure);
    LOAD_CORE_P2(MlnDestroyAccelerationStructure);
    LOAD_CORE_P2(MlnGetAccelerationStructureDeviceAddress);
    LOAD_CORE_P2(MlnGetAccelerationStructureBuildSizes);
    LOAD_CORE_P2(MlnCreateAccelerationStructureObjectGroup);
    LOAD_CORE_P2(MlnCreateAccelerationStructureDataGraph);
    LOAD_CORE_P2(MlnGetRayTracingCapabilities);
    LOAD_CORE_P2(MlnCheckAccelerationStructureCompatibility);

#undef LOAD_CORE_P2

    // ---- dladdr diagnostic: compare function pointer origins with probe results ----
    // Probe showed all rendering-stage APIs resolve to libmaleoon_v300.so and produce
    // TTJ_ logs. AGP logs show no TTJ_ for the same APIs. This diagnostic prints
    // the actual .so each function pointer comes from inside the AGP/HAP process.
#ifndef _WIN32
    {
        auto printDladdr = [](const char* name, void* addr) {
            if (!addr) {
                LLOG("[MaleoonLoader] DLADDR: %-40s = (null)", name);
                return;
            }
            Dl_info info;
            if (dladdr(addr, &info) && info.dli_fname) {
                LLOG("[MaleoonLoader] DLADDR: %-40s = %p [%s + 0x%lx]",
                    name,
                    addr,
                    info.dli_fname,
                    static_cast<unsigned long>(
                        reinterpret_cast<char*>(addr) -
                        reinterpret_cast<char*>(
                            info.dli_fbase)));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            } else {
                LLOG("[MaleoonLoader] DLADDR: %-40s = %p [dladdr failed]", name, addr);
            }
        };

        LLOG("[MaleoonLoader] DLADDR: --- Group A (TTJ_-present in probe) ---");
        printDladdr("MlnGetDeviceQueue",
            reinterpret_cast<void*>(s_MlnGetDeviceQueue));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateTimeline",
            reinterpret_cast<void*>(s_MlnCreateTimeline));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateBindingSet",
            reinterpret_cast<void*>(s_MlnCreateBindingSet));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateImageResource",
            reinterpret_cast<void*>(s_MlnCreateImageResource));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

        LLOG("[MaleoonLoader] DLADDR: --- Group B (TTJ_-missing in AGP) ---");
        printDladdr("MlnCreateBindingLayout",
            reinterpret_cast<void*>(s_MlnCreateBindingLayout));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateProgramInterface",
            reinterpret_cast<void*>(
                s_MlnCreateProgramInterface));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateGraphicsProgram",
            reinterpret_cast<void*>(
                s_MlnCreateGraphicsProgram));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateTransferObjectGroup",
            reinterpret_cast<void*>(
                s_MlnCreateTransferObjectGroup));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateTransferDataGraph",
            reinterpret_cast<void*>(
                s_MlnCreateTransferDataGraph));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateGraphicsObjectGroup",
            reinterpret_cast<void*>(
                s_MlnCreateGraphicsObjectGroup));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateGraphicsDataGraph",
            reinterpret_cast<void*>(
                s_MlnCreateGraphicsDataGraph));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnCreateSchedulingGraph",
            reinterpret_cast<void*>(
                s_MlnCreateSchedulingGraph));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        printDladdr("MlnQueueSubmit",
            reinterpret_cast<void*>(s_MlnQueueSubmit));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        // MlnCreateRenderTarget is Phase 3 (swapchain), will be null here

        LLOG("[MaleoonLoader] DLADDR: --- bootstrap ---");
        printDladdr("s_bootstrapGetDeviceProcAddr",
            reinterpret_cast<void*>(
                s_bootstrapGetDeviceProcAddr));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        LLOG("[MaleoonLoader] DLADDR: s_coreLib=%p, s_unifiedLoaderMode=%d",
            s_coreLib,
            static_cast<int>(s_unifiedLoaderMode));

        // Probe reference offsets (from libmaleoon_v300.so):
        // MlnQueueSubmit=0x790248, MlnCreateGraphicsObjectGroup=0x78c2dc,
        // MlnCreateSchedulingGraph=0x794b28, MlnCreateBindingLayout=0x793b58
        // If AGP shows DIFFERENT offsets or DIFFERENT .so, dispatch is wrong.
        LLOG("[MaleoonLoader] DLADDR: Probe ref offsets (libmaleoon_v300.so): "
             "QueueSubmit=0x790248 GraphicsOG=0x78c2dc SG=0x794b28 BL=0x793b58");
    }
#endif
    // ---- end dladdr diagnostic ----

    // Update probe counters
    s_probeCoreResolvedCount = coreResolved;
    s_probeCoreTotalCount = coreTotal;
    s_probeMlnGetDeviceProcAddr =
        reinterpret_cast<void*>(s_MlnGetDeviceProcAddr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    // Critical check -- every function called unconditionally by device_mln.cpp
    int coreMissing = 0;
    if (!s_MlnDestroyDevice)
        coreMissing++;
    if (!s_MlnDeviceWaitIdle)
        coreMissing++;
    if (!s_MlnGetDeviceProcAddr)
        coreMissing++;
    if (!s_MlnGetGpuProperties)
        coreMissing++;
    if (!s_MlnGetDeviceQueue)
        coreMissing++;
    if (!s_MlnQueueSubmit)
        coreMissing++;
    if (!s_MlnAllocateMemory)
        coreMissing++;
    if (!s_MlnCreateBufferResource)
        coreMissing++;
    if (!s_MlnCreateImageResource)
        coreMissing++;
    if (!s_MlnCreateGraphicsProgram)
        coreMissing++;
    if (!s_MlnCreateGraphicsObjectGroup)
        coreMissing++;
    if (!s_MlnCreateGraphicsDataGraph)
        coreMissing++;
    if (!s_MlnCreateSchedulingGraph)
        coreMissing++;
    if (!s_MlnCreateTimeline)
        coreMissing++;
    s_probeCoreMissing = coreMissing;

    LLOG("[MaleoonLoader] Phase 2: %d/%d core resolved, %d critical missing", coreResolved, coreTotal, coreMissing);

    return (coreMissing == 0);
}

// ============================================================================
// Phase 3: resolve 11 swapchain functions
// Unified mode: all via loader's GetDeviceProcAddr (source "loader_proc").
// Split mode:
//   1. dlsym(s_swapLib, name)                 → source "dlsym_swap"
//   2. s_bootstrapGetDeviceProcAddr(dev, name) → source "core_proc"  (only 2/11)
//   3. s_swapGetDeviceProcAddr(dev, name)      → source "swap_proc"  (remaining 9/11)
// Swap proc enabled by default (core only resolves RenderTarget 2/11).
// Env MLN_SWAP_PROC_ENABLE=0 to disable for diagnostics.
// ============================================================================

MLN_LOADER_EXPORT bool MLNAPI_CALL mlnLoaderResolveSwapchainWithDevice(MlnDevice device)
{
    if (!device) {
        LLOG("[MaleoonLoader] Phase 3: FATAL -- null device handle");
        return false;
    }

    // Swap proc enabled by default (ALL-IN, same as old_MALEOON_LOADER_IMPL).
    // Core's GetDeviceProcAddr only resolves 2/11 swapchain functions;
    // the remaining 9 require swap's own GetDeviceProcAddr.
    // Env MLN_SWAP_PROC_ENABLE=0 can disable for diagnostics.
    s_swapProcEnabled = true;
#ifndef _WIN32
    const char* envVal = getenv("MLN_SWAP_PROC_ENABLE");
    if (envVal && envVal[0] == '0')
        s_swapProcEnabled = false;
#endif

    LLOG("[MaleoonLoader] Phase 3: device=%p  mode=%s  swapLib=%p  swapProc=%p(%s)  coreProc=%p",
        static_cast<const void*>(device),
        s_unifiedLoaderMode ? "UNIFIED" : "SPLIT",
        s_swapLib,
        reinterpret_cast<void*>(s_swapGetDeviceProcAddr),  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        s_swapProcEnabled ? "enabled(default)" : "disabled(env)",
        reinterpret_cast<void*>(s_bootstrapGetDeviceProcAddr));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    int scTotal = 0, nLoader = 0, nDlsym = 0, nCore = 0, nSwap = 0, nNull = 0;
    char p3buf[160];

    // Macro: unified mode → loader_proc only; split mode → dlsym_swap → core_proc → swap_proc
#define LOAD_SC_P3(pfnType, name)                                                \
    do {                                                                         \
        scTotal++;                                                               \
        const char* src = "null";                                                \
        if (s_unifiedLoaderMode) {                                               \
            /* Unified: all functions via loader's GetDeviceProcAddr */          \
            s_##name = (pfnType)s_bootstrapGetDeviceProcAddr(device, #name);     \
            if (s_##name) {                                                      \
                src = "loader_proc";                                             \
                nLoader++;                                                       \
            }                                                                    \
        } else {                                                                 \
            /* Split: dlsym_swap → core_proc → swap_proc */                  \
            if (s_swapLib) {                                                     \
                s_##name = (pfnType)GetSym(s_swapLib, #name);                    \
                if (s_##name) {                                                  \
                    src = "dlsym_swap";                                          \
                    nDlsym++;                                                    \
                }                                                                \
            }                                                                    \
            if (!s_##name && s_bootstrapGetDeviceProcAddr) {                     \
                s_##name = (pfnType)s_bootstrapGetDeviceProcAddr(device, #name); \
                if (s_##name) {                                                  \
                    src = "core_proc";                                           \
                    nCore++;                                                     \
                }                                                                \
            }                                                                    \
            if (!s_##name && s_swapProcEnabled && s_swapGetDeviceProcAddr) {     \
                s_##name = (pfnType)s_swapGetDeviceProcAddr(device, #name);      \
                if (s_##name) {                                                  \
                    src = "swap_proc";                                           \
                    nSwap++;                                                     \
                }                                                                \
            }                                                                    \
        }                                                                        \
        if (!s_##name) {                                                         \
            nNull++;                                                             \
        }                                                                        \
        LLOG("[MaleoonLoader] P3: %-35s = %p  [%s]",                             \
            #name,                                                               \
            reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(s_##name)),      \
            src);                                                                \
    } while (0)

    LOAD_SC_P3(PFN_MlnCreateRenderTarget, MlnCreateRenderTarget);
    LOAD_SC_P3(PFN_MlnDestroyRenderTarget, MlnDestroyRenderTarget);
    LOAD_SC_P3(PFN_MlnCreateDisplaySurface, MlnCreateDisplaySurface);
    LOAD_SC_P3(PFN_MlnGetDisplaySurfaceCapabilities, MlnGetDisplaySurfaceCapabilities);
    LOAD_SC_P3(PFN_MlnGetDisplaySurfaceFormats, MlnGetDisplaySurfaceFormats);
    LOAD_SC_P3(PFN_MlnGetDisplaySurfacePresentModes, MlnGetDisplaySurfacePresentModes);
    LOAD_SC_P3(PFN_MlnDestroyDisplaySurface, MlnDestroyDisplaySurface);
    LOAD_SC_P3(PFN_MlnCreateSwapchain, MlnCreateSwapchain);
    LOAD_SC_P3(PFN_MlnAcquireNextImage, MlnAcquireNextImage);
    LOAD_SC_P3(PFN_MlnGetSwapchainImages, MlnGetSwapchainImages);
    LOAD_SC_P3(PFN_MlnDestroySwapchain, MlnDestroySwapchain);
    LOAD_SC_P3(PFN_MlnPresentSwapchain, MlnPresentSwapchain);

#undef LOAD_SC_P3

    s_probeSwapResolvedCount = nLoader + nDlsym + nCore + nSwap;
    s_probeSwapTotalCount = scTotal;

    // Critical check
    int scMissing = 0;
    if (!s_MlnCreateSwapchain) {
        scMissing++;
        LLOG("[MaleoonLoader] P3 CRITICAL: MlnCreateSwapchain");
    }
    if (!s_MlnAcquireNextImage) {
        scMissing++;
        LLOG("[MaleoonLoader] P3 CRITICAL: MlnAcquireNextImage");
    }
    if (!s_MlnPresentSwapchain) {
        scMissing++;
        LLOG("[MaleoonLoader] P3 CRITICAL: MlnPresentSwapchain");
    }
    if (!s_MlnGetSwapchainImages) {
        scMissing++;
        LLOG("[MaleoonLoader] P3 CRITICAL: MlnGetSwapchainImages");
    }
    if (!s_MlnCreateRenderTarget) {
        scMissing++;
        LLOG("[MaleoonLoader] P3 CRITICAL: MlnCreateRenderTarget");
    }
    if (!s_MlnCreateDisplaySurface) {
        scMissing++;
        LLOG("[MaleoonLoader] P3 CRITICAL: MlnCreateDisplaySurface");
    }

    LLOG("[MaleoonLoader] Phase 3 summary: total=%d, loader=%d, dlsym=%d, core=%d, swap=%d, null=%d, "
         "critical_missing=%d",
        scTotal,
        nLoader,
        nDlsym,
        nCore,
        nSwap,
        nNull,
        scMissing);

#ifndef _WIN32
    // dladdr for Phase 3 rendering-stage function (MlnCreateRenderTarget)
    {
        Dl_info info;
        void* rt =
            reinterpret_cast<void*>(s_MlnCreateRenderTarget);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        if (rt && dladdr(rt, &info) && info.dli_fname) {
            LLOG("[MaleoonLoader] DLADDR-P3: MlnCreateRenderTarget = %p [%s + 0x%lx]",
                rt,
                info.dli_fname,
                static_cast<unsigned long>(
                    reinterpret_cast<char*>(rt) -
                    reinterpret_cast<char*>(info.dli_fbase)));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        } else {
            LLOG("[MaleoonLoader] DLADDR-P3: MlnCreateRenderTarget = %p [%s]", rt, rt ? "dladdr failed" : "null");
        }
    }
#endif

    return (scMissing == 0);
}

// ============================================================================
// Probe info for device_mln.cpp diagnostic logging
// ============================================================================

MLN_LOADER_EXPORT void MLNAPI_CALL mlnLoaderGetProbeInfo(int* outCoreMissing, void** outMlnCreateDevice,
    void** outMlnGetDeviceProcAddr, void** outBootstrapEntry, int* outNullDeviceTested, void** outNullDeviceResult,
    int* outCoreResolved, int* outCoreTotal)
{
    if (outCoreMissing)
        *outCoreMissing = s_probeCoreMissing;
    if (outMlnCreateDevice)
        *outMlnCreateDevice = s_probeMlnCreateDevice;
    if (outMlnGetDeviceProcAddr)
        *outMlnGetDeviceProcAddr = s_probeMlnGetDeviceProcAddr;
    if (outBootstrapEntry)
        *outBootstrapEntry = s_probeBootstrapEntry;
    if (outNullDeviceTested)
        *outNullDeviceTested = 0;  // not applicable for sandbox bundle
    if (outNullDeviceResult)
        *outNullDeviceResult = nullptr;
    if (outCoreResolved)
        *outCoreResolved = s_probeCoreResolvedCount;
    if (outCoreTotal)
        *outCoreTotal = s_probeCoreTotalCount;
}

MLN_LOADER_EXPORT const char* MLNAPI_CALL mlnLoaderGetCoreSource()
{
    if (!s_coreLib)
        return "unknown";
    return s_unifiedLoaderMode ? "unified_loader" : "split_core";
}

MLN_LOADER_EXPORT const char* MLNAPI_CALL mlnLoaderGetBootstrapSource()
{
    if (!s_coreLib)
        return "unknown";
    return s_unifiedLoaderMode ? "unified_loader" : "split_core";
}

MLN_LOADER_EXPORT bool MLNAPI_CALL mlnLoaderIsSwapProcEnabled()
{
    return s_swapProcEnabled;
}

// ============================================================================
// Trampoline functions - Device
// ============================================================================

MLN_LOADER_EXPORT MlnDevice MLNAPI_CALL MlnCreateDevice(const MlnDeviceDescriptor* descriptor)
{
    if (!s_MlnCreateDevice) {
        fprintf(stderr, "[MaleoonLoader] FATAL: MlnCreateDevice NULL!\n");
        return nullptr;
    }

    return s_MlnCreateDevice(descriptor);
}

MLN_LOADER_EXPORT PFN_MlnVoidFunction MLNAPI_CALL MlnGetDeviceProcAddr(MlnDevice device, const char* name)
{
    if (!s_MlnGetDeviceProcAddr)
        return nullptr;
    return s_MlnGetDeviceProcAddr(device, name);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetGpuFeatures(MlnDevice device, MlnGpuFeatures* features)
{
    if (!s_MlnGetGpuFeatures)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetGpuFeatures(device, features);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetGpuProperties(MlnDevice device, MlnGpuProperties* properties)
{
    if (!s_MlnGetGpuProperties)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetGpuProperties(device, properties);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetGpuFormatProperties(
    MlnDevice device, MlnFormat format, MlnGpuFormatProperties* properties)
{
    if (!s_MlnGetGpuFormatProperties)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetGpuFormatProperties(device, format, properties);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetGpuImageFormatProperties(MlnDevice device, MlnFormat format,
    MlnImageType type, MlnImageTiling tiling, MlnImageUsageFlags usage, MlnImageDescriptorFlags flags,
    MlnGpuImageFormatProperties* properties)
{
    if (!s_MlnGetGpuImageFormatProperties)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetGpuImageFormatProperties(device, format, type, tiling, usage, flags, properties);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnDeviceWaitIdle(MlnDevice device)
{
    if (!s_MlnDeviceWaitIdle)
        return MLN_STATUS_UNKNOWN;
    return s_MlnDeviceWaitIdle(device);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyDevice(MlnDevice device)
{
    if (!s_MlnDestroyDevice)
        return;
    s_MlnDestroyDevice(device);
}

MLN_LOADER_EXPORT MlnQueue MLNAPI_CALL MlnGetDeviceQueue(MlnDevice device, u32 index)
{
    if (!s_MlnGetDeviceQueue)
        return nullptr;
    return s_MlnGetDeviceQueue(device, index);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnQueueSubmit(MlnQueue queue, const MlnSubmitDescriptor* submitDescriptor)
{
    if (!s_MlnQueueSubmit)
        return MLN_STATUS_UNKNOWN;
    return s_MlnQueueSubmit(queue, submitDescriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnQueueWaitIdle(MlnQueue queue)
{
    if (!s_MlnQueueWaitIdle)
        return MLN_STATUS_UNKNOWN;
    return s_MlnQueueWaitIdle(queue);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnQueueSetPriority(MlnQueue queue, MlnQueuePriority priority, float adjust)
{
    if (!s_MlnQueueSetPriority)
        return MLN_STATUS_UNKNOWN;
    return s_MlnQueueSetPriority(queue, priority, adjust);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnQueueGetPriority(MlnQueue queue, MlnQueuePriority* priority, float* adjust)
{
    if (!s_MlnQueueGetPriority)
        return MLN_STATUS_UNKNOWN;
    return s_MlnQueueGetPriority(queue, priority, adjust);
}

// ============================================================================
// Trampoline functions - Memory
// ============================================================================

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetGpuMemoryProperties(MlnDevice device, MlnGpuMemoryProperties* pProperties)
{
    if (!s_MlnGetGpuMemoryProperties)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetGpuMemoryProperties(device, pProperties);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnGetResourceMemoryRequirements(MlnDevice device,
    const MlnResourceMemoryRequirementsDescriptor* descriptor, MlnMemoryRequirements* memoryRequirements)
{
    if (s_MlnGetResourceMemoryRequirements) {
        s_MlnGetResourceMemoryRequirements(device, descriptor, memoryRequirements);
    }
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnGetBufferResourceMemoryRequirements(
    MlnDevice device, const MlnBufferDescriptor* descriptor, MlnMemoryRequirements* memoryRequirements)
{
    if (s_MlnGetBufferResourceMemoryRequirements) {
        s_MlnGetBufferResourceMemoryRequirements(device, descriptor, memoryRequirements);
    }
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnGetImageResourceMemoryRequirements(
    MlnDevice device, const MlnImageDescriptor* descriptor, MlnMemoryRequirements* memoryRequirements)
{
    if (s_MlnGetImageResourceMemoryRequirements) {
        s_MlnGetImageResourceMemoryRequirements(device, descriptor, memoryRequirements);
    }
}

MLN_LOADER_EXPORT MlnDeviceMemory MLNAPI_CALL MlnAllocateMemory(
    MlnDevice device, const MlnMemoryAllocateDescriptor* descriptor)
{
    if (!s_MlnAllocateMemory)
        return nullptr;
    return s_MlnAllocateMemory(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnFreeMemory(MlnDevice device, MlnDeviceMemory memory)
{
    if (!s_MlnFreeMemory)
        return;
    s_MlnFreeMemory(device, memory);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnMapMemory(
    MlnDevice device, const MlnMemoryMapDescriptor* descriptor, void** data)
{
    if (!s_MlnMapMemory)
        return MLN_STATUS_UNKNOWN;
    return s_MlnMapMemory(device, descriptor, data);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnUnmapMemory(MlnDevice device, const MlnMemoryUnmapDescriptor* descriptor)
{
    if (!s_MlnUnmapMemory)
        return MLN_STATUS_UNKNOWN;
    return s_MlnUnmapMemory(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnFlushMappedMemoryRanges(
    MlnDevice device, u32 memoryRangeCount, const MlnMappedMemoryRangeDescriptor* pMemoryRanges)
{
    if (!s_MlnFlushMappedMemoryRanges)
        return MLN_STATUS_UNKNOWN;
    return s_MlnFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnInvalidateMappedMemoryRanges(
    MlnDevice device, u32 memoryRangeCount, const MlnMappedMemoryRangeDescriptor* pMemoryRanges)
{
    if (!s_MlnInvalidateMappedMemoryRanges)
        return MLN_STATUS_UNKNOWN;
    return s_MlnInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

// ============================================================================
// Trampoline functions - Resource
// ============================================================================

MLN_LOADER_EXPORT MlnResource MLNAPI_CALL MlnCreateBufferResource(
    MlnDevice device, const MlnBufferDescriptor* descriptor)
{
    if (!s_MlnCreateBufferResource)
        return nullptr;
    return s_MlnCreateBufferResource(device, descriptor);
}

MLN_LOADER_EXPORT MlnResource MLNAPI_CALL MlnCreateImageResource(MlnDevice device, const MlnImageDescriptor* descriptor)
{
    if (!s_MlnCreateImageResource)
        return nullptr;
    return s_MlnCreateImageResource(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnBindResourceMemory(
    MlnDevice device, const MlnBindResourceMemoryDescriptor* descriptor)
{
    return s_MlnBindResourceMemory(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyResource(MlnDevice device, MlnResource resource)
{
    if (!s_MlnDestroyResource)
        return;
    s_MlnDestroyResource(device, resource);
}

MLN_LOADER_EXPORT MlnResourceView MLNAPI_CALL MlnCreateBufferResourceView(
    MlnDevice device, const MlnBufferViewDescriptor* descriptor)
{
    if (!s_MlnCreateBufferResourceView)
        return nullptr;
    return s_MlnCreateBufferResourceView(device, descriptor);
}

MLN_LOADER_EXPORT MlnResourceView MLNAPI_CALL MlnCreateImageResourceView(
    MlnDevice device, const MlnImageViewDescriptor* descriptor)
{
    if (!s_MlnCreateImageResourceView)
        return nullptr;
    return s_MlnCreateImageResourceView(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyResourceView(MlnDevice device, MlnResourceView resourceView)
{
    if (!s_MlnDestroyResourceView)
        return;
    s_MlnDestroyResourceView(device, resourceView);
}

MLN_LOADER_EXPORT MlnSampler MLNAPI_CALL MlnCreateSampler(MlnDevice device, const MlnSamplerDescriptor* descriptor)
{
    if (!s_MlnCreateSampler)
        return nullptr;
    return s_MlnCreateSampler(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroySampler(MlnDevice device, MlnSampler sampler)
{
    if (!s_MlnDestroySampler)
        return;
    s_MlnDestroySampler(device, sampler);
}

// ============================================================================
// Trampoline functions - Shader / Program
// ============================================================================

MLN_LOADER_EXPORT MlnProgramCache MLNAPI_CALL MlnCreateProgramCache(
    MlnDevice device, const MlnProgramCacheDescriptor* descriptor)
{
    if (!s_MlnCreateProgramCache)
        return nullptr;
    return s_MlnCreateProgramCache(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetProgramCacheData(
    MlnDevice device, MlnProgramCache programCache, size_t* cacheSize, void* cache)
{
    if (!s_MlnGetProgramCacheData)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetProgramCacheData(device, programCache, cacheSize, cache);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnMergeProgramCaches(
    MlnDevice device, MlnProgramCache dstCache, u32 srcCacheCount, const MlnProgramCache* srcCaches)
{
    if (!s_MlnMergeProgramCaches)
        return MLN_STATUS_UNKNOWN;
    return s_MlnMergeProgramCaches(device, dstCache, srcCacheCount, srcCaches);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyProgramCache(MlnDevice device, MlnProgramCache programCache)
{
    if (!s_MlnDestroyProgramCache)
        return;
    s_MlnDestroyProgramCache(device, programCache);
}

MLN_LOADER_EXPORT MlnProgramInterface MLNAPI_CALL MlnCreateProgramInterface(
    MlnDevice device, const MlnProgramInterfaceDescriptor* descriptor)
{
    if (!s_MlnCreateProgramInterface)
        return nullptr;
    return s_MlnCreateProgramInterface(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyProgramInterface(MlnDevice device, MlnProgramInterface programInterface)
{
    if (!s_MlnDestroyProgramInterface)
        return;
    s_MlnDestroyProgramInterface(device, programInterface);
}

MLN_LOADER_EXPORT MlnProgram MLNAPI_CALL MlnCreateGraphicsProgram(
    MlnDevice device, MlnProgramCache programCache, const MlnGraphicsProgramState* states)
{
    if (!s_MlnCreateGraphicsProgram)
        return nullptr;
    return s_MlnCreateGraphicsProgram(device, programCache, states);
}

MLN_LOADER_EXPORT MlnProgram MLNAPI_CALL MlnCreateComputeProgram(
    MlnDevice device, MlnProgramCache programCache, const MlnComputeProgramState* state)
{
    if (!s_MlnCreateComputeProgram)
        return nullptr;
    return s_MlnCreateComputeProgram(device, programCache, state);
}

MLN_LOADER_EXPORT MlnProgram MLNAPI_CALL MlnCreateGraphicsProgramAsync(
    MlnDevice device, MlnProgramCache programCache, const MlnGraphicsProgramState* states, const MlnPriority priority)
{
    if (!s_MlnCreateGraphicsProgramAsync)
        return nullptr;
    return s_MlnCreateGraphicsProgramAsync(device, programCache, states, priority);
}

MLN_LOADER_EXPORT MlnProgram MLNAPI_CALL MlnCreateComputeProgramAsync(
    MlnDevice device, MlnProgramCache programCache, const MlnComputeProgramState* state, const MlnPriority priority)
{
    if (!s_MlnCreateComputeProgramAsync)
        return nullptr;
    return s_MlnCreateComputeProgramAsync(device, programCache, state, priority);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetCreatedProgramAsyncResult(MlnDevice device, MlnProgram program)
{
    if (!s_MlnGetCreatedProgramAsyncResult)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetCreatedProgramAsyncResult(device, program);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnWaitCreatedProgramAsyncResult(MlnDevice device, MlnProgram program)
{
    if (!s_MlnWaitCreatedProgramAsyncResult)
        return MLN_STATUS_UNKNOWN;
    return s_MlnWaitCreatedProgramAsyncResult(device, program);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyProgram(MlnDevice device, MlnProgram program)
{
    if (!s_MlnDestroyProgram)
        return;
    s_MlnDestroyProgram(device, program);
}

// ============================================================================
// Trampoline functions - Binding
// ============================================================================

MLN_LOADER_EXPORT MlnBindingLayout MLNAPI_CALL MlnCreateBindingLayout(
    MlnDevice device, const MlnBindingLayoutDescriptor* descriptor)
{
    if (!s_MlnCreateBindingLayout)
        return nullptr;
    return s_MlnCreateBindingLayout(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyBindingLayout(MlnDevice device, MlnBindingLayout bindingLayout)
{
    if (!s_MlnDestroyBindingLayout) {
        return;
    }
    s_MlnDestroyBindingLayout(device, bindingLayout);
}

MLN_LOADER_EXPORT MlnBindingSet MLNAPI_CALL MlnCreateBindingSet(
    MlnDevice device, MlnBindingLayout bindingLayout, u32 varBindingSize)
{
    if (!s_MlnCreateBindingSet)
        return nullptr;
    return s_MlnCreateBindingSet(device, bindingLayout, varBindingSize);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnUpdateBindingSets(MlnDevice device, u32 bindingWriteCount,
    const MlnWriteBindingSet* bindingWrites, u32 bindingCopyCount, const MlnCopyBindingSet* bindingCopies)
{
    if (!s_MlnUpdateBindingSets)
        return;
    s_MlnUpdateBindingSets(device, bindingWriteCount, bindingWrites, bindingCopyCount, bindingCopies);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyBindingSet(MlnDevice device, const MlnBindingSet bindingSet)
{
    if (!s_MlnDestroyBindingSet)
        return;
    s_MlnDestroyBindingSet(device, bindingSet);
}

// ============================================================================
// Trampoline functions - Execution
// ============================================================================

MLN_LOADER_EXPORT MlnObjectGroup MLNAPI_CALL MlnCreateGraphicsObjectGroup(
    MlnDevice device, const MlnGraphicsObjectGroupDescriptor* descriptor)
{
    if (!s_MlnCreateGraphicsObjectGroup)
        return nullptr;
    return s_MlnCreateGraphicsObjectGroup(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnUpdateGraphicsObjectGroup(
    MlnDevice device, MlnObjectGroup objectGroup, const MlnGraphicsObjectGroupUpdateDescriptor* descriptor)
{
    if (!s_MlnUpdateGraphicsObjectGroup)
        return MLN_STATUS_UNKNOWN;
    return s_MlnUpdateGraphicsObjectGroup(device, objectGroup, descriptor);
}

MLN_LOADER_EXPORT MlnObjectGroup MLNAPI_CALL MlnCreateComputeObjectGroup(
    MlnDevice device, const MlnComputeObjectGroupDescriptor* descriptor)
{
    if (!s_MlnCreateComputeObjectGroup)
        return nullptr;
    return s_MlnCreateComputeObjectGroup(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnUpdateComputeObjectGroup(
    MlnDevice device, MlnObjectGroup objectGroup, const MlnComputeObjectGroupUpdateDescriptor* descriptor)
{
    if (!s_MlnUpdateComputeObjectGroup)
        return MLN_STATUS_UNKNOWN;
    return s_MlnUpdateComputeObjectGroup(device, objectGroup, descriptor);
}

MLN_LOADER_EXPORT MlnObjectGroup MLNAPI_CALL MlnCreateTransferObjectGroup(
    MlnDevice device, const MlnTransferObjectGroupDescriptor* descriptor)
{
    if (!s_MlnCreateTransferObjectGroup)
        return nullptr;
    return s_MlnCreateTransferObjectGroup(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyObjectGroup(MlnDevice device, MlnObjectGroup objectGroup)
{
    if (!s_MlnDestroyObjectGroup)
        return;
    s_MlnDestroyObjectGroup(device, objectGroup);
}

MLN_LOADER_EXPORT MlnDataGraph MLNAPI_CALL MlnCreateGraphicsDataGraph(
    MlnDevice device, const MlnGraphicsDataGraphDescriptor* descriptor)
{
    if (!s_MlnCreateGraphicsDataGraph)
        return nullptr;
    return s_MlnCreateGraphicsDataGraph(device, descriptor);
}

MLN_LOADER_EXPORT MlnDataGraph MLNAPI_CALL MlnCreateComputeDataGraph(
    MlnDevice device, const MlnComputeDataGraphDescriptor* descriptor)
{
    if (!s_MlnCreateComputeDataGraph)
        return nullptr;
    return s_MlnCreateComputeDataGraph(device, descriptor);
}

MLN_LOADER_EXPORT MlnDataGraph MLNAPI_CALL MlnCreateTransferDataGraph(
    MlnDevice device, const MlnTransferDataGraphDescriptor* descriptor)
{
    if (!s_MlnCreateTransferDataGraph)
        return nullptr;
    return s_MlnCreateTransferDataGraph(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyDataGraph(MlnDevice device, MlnDataGraph dataGraph)
{
    if (!s_MlnDestroyDataGraph)
        return;
    s_MlnDestroyDataGraph(device, dataGraph);
}

MLN_LOADER_EXPORT MlnSchedulingGraph MLNAPI_CALL MlnCreateSchedulingGraph(
    MlnDevice device, const MlnSchedulingGraphDescriptor* descriptor)
{
    if (!s_MlnCreateSchedulingGraph)
        return nullptr;
    return s_MlnCreateSchedulingGraph(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroySchedulingGraph(MlnDevice device, MlnSchedulingGraph schedulingGraph)
{
    if (!s_MlnDestroySchedulingGraph)
        return;
    s_MlnDestroySchedulingGraph(device, schedulingGraph);
}

// ============================================================================
// Trampoline functions - Sync
// ============================================================================

MLN_LOADER_EXPORT MlnTimeline MLNAPI_CALL MlnCreateTimeline(MlnDevice device, const MlnTimelineDescriptor* descriptor)
{
    if (!s_MlnCreateTimeline)
        return nullptr;
    return s_MlnCreateTimeline(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnSignalTimelines(
    MlnDevice device, const MlnTimelineSignalDescriptor* descriptor)
{
    if (!s_MlnSignalTimelines)
        return MLN_STATUS_UNKNOWN;
    return s_MlnSignalTimelines(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnWaitForTimelines(
    MlnDevice device, const MlnTimelineWaitDescriptor* descriptor, u64 timeout)
{
    if (!s_MlnWaitForTimelines)
        return MLN_STATUS_UNKNOWN;
    return s_MlnWaitForTimelines(device, descriptor, timeout);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetTimelineValue(MlnDevice device, MlnTimeline timeline, u64* value)
{
    if (!s_MlnGetTimelineValue)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetTimelineValue(device, timeline, value);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnExportTimelineFd(
    MlnDevice device, const MlnTimelineExportFdDescriptor* descriptor, int* fd)
{
    if (!s_MlnExportTimelineFd)
        return MLN_STATUS_UNKNOWN;
    return s_MlnExportTimelineFd(device, descriptor, fd);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnImportTimelineFd(
    MlnDevice device, const MlnTimelineImportFdDescriptor* descriptor)
{
    if (!s_MlnImportTimelineFd)
        return MLN_STATUS_UNKNOWN;
    return s_MlnImportTimelineFd(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnResetTimeline(MlnDevice device, MlnTimeline timeline, u64 initialValue)
{
    if (!s_MlnResetTimeline)
        return MLN_STATUS_UNKNOWN;
    return s_MlnResetTimeline(device, timeline, initialValue);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyTimeline(MlnDevice device, MlnTimeline timeline)
{
    if (!s_MlnDestroyTimeline)
        return;
    s_MlnDestroyTimeline(device, timeline);
}

// ============================================================================
// Trampoline functions - Presentation (swapchain)
// ============================================================================

MLN_LOADER_EXPORT MlnRenderTarget MLNAPI_CALL MlnCreateRenderTarget(
    MlnDevice device, const MlnRenderTargetDescriptor* descriptor)
{
    if (!s_MlnCreateRenderTarget) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnCreateRenderTarget\n");
        return nullptr;
    }
    return s_MlnCreateRenderTarget(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyRenderTarget(MlnDevice device, MlnRenderTarget renderTarget)
{
    if (s_MlnDestroyRenderTarget)
        s_MlnDestroyRenderTarget(device, renderTarget);
}

MLN_LOADER_EXPORT MlnDisplaySurface MLNAPI_CALL MlnCreateDisplaySurface(
    MlnDevice device, const MlnDisplaySurfaceDescriptor* descriptor)
{
    if (!s_MlnCreateDisplaySurface) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnCreateDisplaySurface\n");
        return nullptr;
    }
    return s_MlnCreateDisplaySurface(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetDisplaySurfaceCapabilities(
    MlnDevice device, MlnDisplaySurface displaySurface, MlnDisplaySurfaceCapabilities* capabilities)
{
    if (!s_MlnGetDisplaySurfaceCapabilities)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetDisplaySurfaceCapabilities(device, displaySurface, capabilities);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetDisplaySurfaceFormats(
    MlnDevice device, MlnDisplaySurface displaySurface, u32* formatCount, MlnDisplaySurfaceFormat* formats)
{
    if (!s_MlnGetDisplaySurfaceFormats)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetDisplaySurfaceFormats(device, displaySurface, formatCount, formats);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetDisplaySurfacePresentModes(
    MlnDevice device, MlnDisplaySurface displaySurface, u32* presentModeCount, MlnPresentMode* presentModes)
{
    if (!s_MlnGetDisplaySurfacePresentModes)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetDisplaySurfacePresentModes(device, displaySurface, presentModeCount, presentModes);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyDisplaySurface(MlnDevice device, MlnDisplaySurface displaySurface)
{
    if (s_MlnDestroyDisplaySurface)
        s_MlnDestroyDisplaySurface(device, displaySurface);
}

MLN_LOADER_EXPORT MlnSwapchain MLNAPI_CALL MlnCreateSwapchain(
    MlnDevice device, const MlnSwapchainDescriptor* descriptor)
{
    if (!s_MlnCreateSwapchain) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnCreateSwapchain\n");
        return nullptr;
    }
    return s_MlnCreateSwapchain(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnAcquireNextImage(
    MlnDevice device, MlnSwapchain swapchain, u64 timeout, MlnTimeline timeline, u64 timelineValue, u32* imageIndex)
{
    if (!s_MlnAcquireNextImage)
        return MLN_STATUS_UNKNOWN;
    return s_MlnAcquireNextImage(device, swapchain, timeout, timeline, timelineValue, imageIndex);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetSwapchainImages(
    MlnDevice device, MlnSwapchain swapchain, u32* imageCount, MlnResource* imageResources)
{
    if (!s_MlnGetSwapchainImages)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetSwapchainImages(device, swapchain, imageCount, imageResources);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroySwapchain(MlnDevice device, MlnSwapchain swapchain)
{
    if (s_MlnDestroySwapchain)
        s_MlnDestroySwapchain(device, swapchain);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnPresentSwapchain(MlnQueue queue, const MlnPresentDescriptor* descriptor)
{
    if (!s_MlnPresentSwapchain)
        return MLN_STATUS_UNKNOWN;
    return s_MlnPresentSwapchain(queue, descriptor);
}

// ============================================================================
// Trampoline functions - Extension / Helper (optional, null-guarded)
// ============================================================================

MLN_LOADER_EXPORT void MlnSetResourceDebugName(MlnDevice device, MlnResource resource, const char* name)
{
    if (s_MlnSetResourceDebugName) {
        s_MlnSetResourceDebugName(device, resource, name);
    }
}

// ============================================================================
// Trampoline functions - Ray Tracing (optional, null-guarded)
//   Null-guarded pattern mirrors presentation/extension trampolines: on
//   legacy driver builds without RT, AGP just sees zero/null returns and can
//   detect missing support via MlnGetRayTracingCapabilities failing.
// ============================================================================

MLN_LOADER_EXPORT MlnDeviceAddress MLNAPI_CALL MlnGetBufferResourceAddress(MlnDevice device, MlnResource resource)
{
    if (!s_MlnGetBufferResourceAddress) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnGetBufferResourceAddress\n");
        return 0;
    }

    return s_MlnGetBufferResourceAddress(device, resource);
}

MLN_LOADER_EXPORT MlnAccelerationStructure MLNAPI_CALL MlnCreateAccelerationStructure(
    MlnDevice device, const MlnAccelerationStructureDescriptor* descriptor)
{
    if (!s_MlnCreateAccelerationStructure) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnCreateAccelerationStructure\n");
        return nullptr;
    }

    return s_MlnCreateAccelerationStructure(device, descriptor);
}

MLN_LOADER_EXPORT void MLNAPI_CALL MlnDestroyAccelerationStructure(
    MlnDevice device, MlnAccelerationStructure accelerationStructure)
{
    if (s_MlnDestroyAccelerationStructure) {
        s_MlnDestroyAccelerationStructure(device, accelerationStructure);
    }
}

MLN_LOADER_EXPORT MlnDeviceAddress MLNAPI_CALL MlnGetAccelerationStructureDeviceAddress(
    MlnDevice device, MlnAccelerationStructure accelerationStructure)
{
    if (!s_MlnGetAccelerationStructureDeviceAddress) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnGetAccelerationStructureDeviceAddress\n");
        return 0;
    }

    return s_MlnGetAccelerationStructureDeviceAddress(device, accelerationStructure);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetAccelerationStructureBuildSizes(MlnDevice device,
    const MlnAccelerationStructureBuildGeometryDescriptor* buildGeometry, const u32* maxPrimitiveCounts,
    MlnAccelerationStructureBuildSizesDescriptor* buildSizes)
{
    if (!s_MlnGetAccelerationStructureBuildSizes)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetAccelerationStructureBuildSizes(device, buildGeometry, maxPrimitiveCounts, buildSizes);
}

MLN_LOADER_EXPORT MlnObjectGroup MLNAPI_CALL MlnCreateAccelerationStructureObjectGroup(
    MlnDevice device, const MlnAccelerationStructureObjectGroupDescriptor* descriptor)
{
    if (!s_MlnCreateAccelerationStructureObjectGroup) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnCreateAccelerationStructureObjectGroup\n");
        return nullptr;
    }

    return s_MlnCreateAccelerationStructureObjectGroup(device, descriptor);
}

MLN_LOADER_EXPORT MlnDataGraph MLNAPI_CALL MlnCreateAccelerationStructureDataGraph(
    MlnDevice device, const MlnAccelerationStructureDataGraphDescriptor* descriptor)
{
    if (!s_MlnCreateAccelerationStructureDataGraph) {
        fprintf(stderr, "[MaleoonLoader] NULL call: MlnCreateAccelerationStructureDataGraph\n");
        return nullptr;
    }

    return s_MlnCreateAccelerationStructureDataGraph(device, descriptor);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnGetRayTracingCapabilities(
    MlnDevice device, MlnRayTracingCapabilities* capabilities)
{
    if (!s_MlnGetRayTracingCapabilities)
        return MLN_STATUS_UNKNOWN;
    return s_MlnGetRayTracingCapabilities(device, capabilities);
}

MLN_LOADER_EXPORT MlnStatus MLNAPI_CALL MlnCheckAccelerationStructureCompatibility(MlnDevice device,
    const MlnAccelerationStructureVersion* version, MlnAccelerationStructureCompatibility* compatibility)
{
    if (!s_MlnCheckAccelerationStructureCompatibility)
        return MLN_STATUS_UNKNOWN;
    return s_MlnCheckAccelerationStructureCompatibility(device, version, compatibility);
}
