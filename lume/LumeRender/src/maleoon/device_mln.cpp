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

#include "device_mln.h"

#include <base/containers/unique_ptr.h>
#include <core/log.h>
#include <render/namespace.h>

#include <render/maleoon/maleoon_loader_impl.h>
#include "device/gpu_program_util.h"
#include "device/gpu_resource_manager.h"
#include "device/shader_manager.h"
#include "maleoon/descriptor_set_manager_mln.h"
#include "maleoon/gpu_buffer_mln.h"
#include "maleoon/gpu_image_mln.h"
#include "maleoon/gpu_program_mln.h"
#include "maleoon/gpu_sampler_mln.h"
#include "maleoon/gpu_semaphore_mln.h"
#include "maleoon/node_context_descriptor_set_manager_mln.h"
#include "maleoon/node_context_pool_manager_mln.h"
#include "maleoon/pipeline_state_object_mln.h"
#include "maleoon/render_backend_mln.h"
#include "maleoon/render_frame_sync_mln.h"
#include "maleoon/shader_module_mln.h"
#include "maleoon/swapchain_mln.h"
#include "mln_convert.h"
#include "mln_log.h"
#include "util/log.h"

// Maleoon loader: sandbox-bundle (dlopen bundled libhvgr.so/libmaleoon_v300.so + libswapchain.so)
extern "C" bool mlnLoaderInit(const char* corePath, const char* swapchainPath);
extern "C" void mlnLoaderDeinit();
// Phase 2: resolve core functions via MlnGetDeviceProcAddr(device, name)
extern "C" bool mlnLoaderResolveWithDevice(MlnDevice device);
// Phase 3: resolve swapchain functions via MlnGetDeviceProcAddr(device, name)
extern "C" bool mlnLoaderResolveSwapchainWithDevice(MlnDevice device);
// Bootstrap source: "sandbox_bundle" or "unknown"
extern "C" const char* mlnLoaderGetBootstrapSource();

// Diagnostic probe info for hilog output
extern "C" void mlnLoaderGetProbeInfo(int* outCoreMissing, void** outMlnCreateDevice, void** outMlnGetDeviceProcAddr,
    void** outBootstrapEntry, int* outNullDeviceTested, void** outNullDeviceResult, int* outCoreResolved,
    int* outCoreTotal);

// Log callback: routes loader messages through CORE_LOG_E (hilog-visible).
extern "C" void mlnLoaderSetLogFn(void (*fn)(const char*));

// Returns loader source: "sandbox_bundle" or "unknown"
extern "C" const char* mlnLoaderGetCoreSource();

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {
constexpr uint32_t FORMAT_PROPERTY_COUNT =
    DeviceFormatSupportConstants::LINEAR_FORMAT_MAX_COUNT + DeviceFormatSupportConstants::ADDITIONAL_FORMAT_MAX_COUNT;
} // namespace

// ============================================================================
// DeviceMln
// ============================================================================

DeviceMln::DeviceMln(RenderContext& renderContext) : Device(renderContext)
{
    MLN_LOG_INIT("====== [BACKEND] MALEOON path selected ======");
    MLN_LOG_INIT("DeviceMln: initializing Maleoon backend");

    InitMaleoonDevice();

    if (!mlnDevice_) {
        MLN_LOG_ERR("DeviceMln: initialization FAILED (no Maleoon device)");
        SetDeviceStatus(false);
        return;
    }

    platData_.mlnDevice = mlnDevice_;
    platData_.mlnQueue = mlnQueue_;

    QueryFormatProperties();

    SetDeviceStatus(true);

    // Initialize GPU resource manager, shader manager, and global descriptor set manager
    // These are required by RenderContext::Init() when it calls GetShaderManager()
    const GpuResourceManager::CreateInfo grmCreateInfo{
        GpuResourceManager::GPU_RESOURCE_MANAGER_OPTIMIZE_STAGING_MEMORY,
    };
    gpuResourceMgr_ = make_unique<GpuResourceManager>(*this, grmCreateInfo);
    shaderMgr_ = make_unique<ShaderManager>(*this);
    globalDescriptorSetMgr_ = make_unique<DescriptorSetManagerMln>(*this);
    lowLevelDevice_ = make_unique<LowLevelDeviceMln>(*this);

    MLN_LOG_INIT("DeviceMln: initialization complete (gpuResourceMgr + shaderMgr + descriptorSetMgr)");
}

DeviceMln::~DeviceMln()
{
    MLN_LOG_INIT("DeviceMln: destroying");

    if (!mlnDevice_) {
        // Loader or device creation failed -- nothing to tear down except the loader itself
        mlnLoaderDeinit();
        return;
    }

    WaitForIdle();
    SetDeviceStatus(false);

    // Clean up managers in reverse order of creation
    // Must release before taking down gpu resource manager
    globalDescriptorSetMgr_.reset();
    swapchains_.clear();
    gpuResourceMgr_.reset();
    shaderMgr_.reset();

    if (mlnProgramCache_) {
        MlnDestroyProgramCache(mlnDevice_, mlnProgramCache_);
        mlnProgramCache_ = nullptr;
    }

    MlnDeviceWaitIdle(mlnDevice_);
    MlnDestroyDevice(mlnDevice_);
    mlnDevice_ = nullptr;
    mlnQueue_ = nullptr;

    // Release the Maleoon loader (dlclose bundled .so)
    mlnLoaderDeinit();
}

void DeviceMln::InitMaleoonDevice()
{
    // Read log flags early so MLN_LOG_INIT works during loader init
    // (default: g_mlnLog.init=false → loader messages would be silently dropped)
    MlnLogRefreshFlags();

    // Route loader init messages through hilog so we can see which .so is loaded
    mlnLoaderSetLogFn([](const char* msg) { MLN_LOG_INIT("%s", msg); });

    // Initialize Maleoon loader (dlopen core + swapchain)
    const bool loaderOk = mlnLoaderInit(nullptr, nullptr);

    // Log which runtime path was loaded
    const char* coreSource = mlnLoaderGetCoreSource();
    MLN_LOG_INIT("Core runtime source: %s", coreSource ? coreSource : "(null)");

    // BUG-19 / D2 probe: log loader diagnostic info via CORE_LOG_E (hilog-visible).
    // Fires regardless of loaderOk so we see state even on failure.
    {
        int coreMissing = -1;
        void* fnCreateDevice = nullptr;
        void* fnGetDeviceProcAddr = nullptr;
        void* bootstrapEntry = nullptr;
        int nullDeviceTested = 0;
        void* nullDeviceResult = nullptr;
        int coreResolved = 0, coreTotal = 0;
        mlnLoaderGetProbeInfo(&coreMissing, &fnCreateDevice, &fnGetDeviceProcAddr, &bootstrapEntry, &nullDeviceTested,
            &nullDeviceResult, &coreResolved, &coreTotal);
        MLN_LOG_INIT(
            "Probe: loaderOk=%d coreMissing=%d MlnCreateDevice=%p "
            "GetDeviceProcAddr=%p bootstrap=%p",
            static_cast<int>(loaderOk), coreMissing, fnCreateDevice, fnGetDeviceProcAddr, bootstrapEntry);
        MLN_LOG_INIT("Probe D2: nullDeviceTested=%d nullDeviceResult=%p", nullDeviceTested, nullDeviceResult);
        MLN_LOG_INIT("Probe resolve: %d/%d core functions resolved", coreResolved, coreTotal);
    }

    if (!loaderOk) {
        MLN_LOG_ERR("DeviceMln: mlnLoaderInit FAILED!");
        return;
    }

    const char* bootstrapSrc = mlnLoaderGetBootstrapSource();
    MLN_LOG_INIT("DeviceMln: LoaderMode=sandbox_bundle bootstrap=%s", bootstrapSrc ? bootstrapSrc : "(null)");

    // Create Maleoon device with one graphics queue
    MlnQueueDescriptor queueDesc{};
    queueDesc.extensionCount = 0;
    queueDesc.extensions = nullptr;
    queueDesc.flags = 0;
    queueDesc.priority = MLN_QUEUE_PRIORITY_HIGH;

    MlnApplicationDescriptor appDesc{};
    appDesc.applicationName = "LumeEngine";
    appDesc.applicationVersion = 1;
    appDesc.engineName = "Lume";
    appDesc.engineVersion = 1;

    MlnDeviceDescriptor deviceDesc{};
    deviceDesc.extensionCount = 0;
    deviceDesc.extensions = nullptr;
    deviceDesc.flags = 0;
    deviceDesc.appDescriptor = &appDesc;
    deviceDesc.queueDescriptorCount = 1;
    deviceDesc.queueDescriptors = &queueDesc;
    deviceDesc.layerCount = 0;
    deviceDesc.layerNames = nullptr;

    mlnDevice_ = MlnCreateDevice(&deviceDesc);
    if (!mlnDevice_) {
        MLN_LOG_ERR("DeviceMln: MlnCreateDevice failed");
        return;
    }
    MLN_LOG_INIT("DeviceMln: MlnCreateDevice OK (device=%p)", reinterpret_cast<void*>(mlnDevice_));

    // Phase 2: resolve all remaining core functions via MlnGetDeviceProcAddr(device, name)
    MLN_LOG_INIT("Phase 2: BEGIN mlnLoaderResolveWithDevice(device=%p)", reinterpret_cast<void*>(mlnDevice_));
    const bool phase2Ok = mlnLoaderResolveWithDevice(mlnDevice_);
    {
        int coreMissing = -1;
        int coreResolved = 0, coreTotal = 0;
        mlnLoaderGetProbeInfo(&coreMissing, nullptr, nullptr, nullptr, nullptr, nullptr, &coreResolved, &coreTotal);
        MLN_LOG_INIT("Phase 2: %d/%d core resolved, coreMissing=%d, result=%d", coreResolved, coreTotal, coreMissing,
            static_cast<int>(phase2Ok));
    }
    if (!phase2Ok) {
        MLN_LOG_ERR("DeviceMln: Phase 2 FAILED -- critical core functions missing");
        MlnDestroyDevice(mlnDevice_);
        mlnDevice_ = nullptr;
        return;
    }

    // Phase 3: resolve 11 swapchain functions via MlnGetDeviceProcAddr(device, name)
    MLN_LOG_INIT("Phase 3: BEGIN mlnLoaderResolveSwapchainWithDevice(device=%p)", reinterpret_cast<void*>(mlnDevice_));
    const bool phase3Ok = mlnLoaderResolveSwapchainWithDevice(mlnDevice_);
    MLN_LOG_INIT("Phase 3: result=%d", static_cast<int>(phase3Ok));
    if (!phase3Ok) {
        MLN_LOG_ERR("DeviceMln: Phase 3 FAILED -- critical swapchain functions missing");
        MlnDestroyDevice(mlnDevice_);
        mlnDevice_ = nullptr;
        return;
    }

    // Get default queue
    mlnQueue_ = MlnGetDeviceQueue(mlnDevice_, 0);
    if (!mlnQueue_) {
        MLN_LOG_ERR("DeviceMln: MlnGetDeviceQueue failed");
        return;
    }
    MLN_LOG_INIT("DeviceMln: MlnGetDeviceQueue OK (queue 0)");

    // OPT #5: Cache GPU memory properties once (avoid per-resource MlnGetGpuMemoryProperties calls)
    MlnGetGpuMemoryProperties(mlnDevice_, &cachedMemoryProps_);

    // Log GPU properties
    MlnGpuProperties gpuProps{};
    if (MlnGetGpuProperties(mlnDevice_, &gpuProps) == MLN_STATUS_SUCCESS) {
        MLN_LOG_INIT("DeviceMln: GPU driverVersion=%u", gpuProps.driverVersion);
    }

    // Log device configuration (for build verification)
    const auto& devConfig = GetDeviceConfiguration();
    MLN_LOG_INIT("DeviceMln: CONFIG bufferingCount=%u, swapchainImageCount=%u", devConfig.bufferingCount,
        devConfig.swapchainImageCount);
}

void DeviceMln::QueryFormatProperties()
{
    if (!mlnDevice_) {
        return;
    }

    formatProperties_.resize(FORMAT_PROPERTY_COUNT);

    // Query linear format range: 0..LINEAR_FORMAT_MAX_IDX
    for (uint32_t i = 0; i <= DeviceFormatSupportConstants::LINEAR_FORMAT_MAX_IDX; ++i) {
        MlnGpuFormatProperties mlnProps{};
        MlnFormat mlnFmt = static_cast<MlnFormat>(i);
        if (MlnGetGpuFormatProperties(mlnDevice_, mlnFmt, &mlnProps) == MLN_STATUS_SUCCESS) {
            formatProperties_[i] = FromMlnFormatProperties(mlnProps);
            formatProperties_[i].bytesPerPixel = GpuProgramUtil::FormatByteSize(static_cast<BASE_NS::Format>(i));
        }
    }

    // Query additional format range
    for (uint32_t i = DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER;
         i <= DeviceFormatSupportConstants::ADDITIONAL_FORMAT_END_NUMBER; ++i) {
        MlnGpuFormatProperties mlnProps{};
        MlnFormat mlnFmt = static_cast<MlnFormat>(i);
        if (MlnGetGpuFormatProperties(mlnDevice_, mlnFmt, &mlnProps) == MLN_STATUS_SUCCESS) {
            const uint32_t idx = DeviceFormatSupportConstants::ADDITIONAL_FORMAT_BASE_IDX +
                                 (i - DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER);
            if (idx < FORMAT_PROPERTY_COUNT) {
                formatProperties_[idx] = FromMlnFormatProperties(mlnProps);
                formatProperties_[idx].bytesPerPixel = GpuProgramUtil::FormatByteSize(static_cast<BASE_NS::Format>(i));
            }
        }
    }

    // Log all queried format results — scan the entire formatProperties_ table
    uint32_t queryOk = 0;
    uint32_t queryFail = 0;
    for (uint32_t i = 1; i <= DeviceFormatSupportConstants::LINEAR_FORMAT_MAX_IDX; ++i) {
        if (i < formatProperties_.size()) {
            const auto& fp = formatProperties_[i];
            if (fp.optimalTilingFeatures != 0u) {
                ++queryOk;
            } else if (fp.bytesPerPixel > 0u || GpuProgramUtil::FormatByteSize(static_cast<BASE_NS::Format>(i)) > 0u) {
                // Only log formats that have a known byte size (skip unused/reserved indices)
                ++queryFail;
                MLN_LOG_ERR("FORMAT UNSUPPORTED AT INIT: idx=%u bpp=%u driver returned features=0", i,
                    GpuProgramUtil::FormatByteSize(static_cast<BASE_NS::Format>(i)));
            }
        }
    }
    MLN_LOG_INIT("FORMAT QUERY SUMMARY: %u supported, %u unsupported", queryOk, queryFail);
}

DeviceBackendType DeviceMln::GetBackendType() const
{
    return DeviceBackendType::MALEOON;
}

const DevicePlatformData& DeviceMln::GetPlatformData() const
{
    return platData_;
}

FormatProperties DeviceMln::GetFormatProperties(Format format) const
{
    const uint32_t formatIdx = static_cast<uint32_t>(format);
    FormatProperties props{};
    if (formatIdx <= DeviceFormatSupportConstants::LINEAR_FORMAT_MAX_IDX) {
        if (formatIdx < formatProperties_.size()) {
            props = formatProperties_[formatIdx];
        }
    } else if (formatIdx >= DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER &&
               formatIdx <= DeviceFormatSupportConstants::ADDITIONAL_FORMAT_END_NUMBER) {
        const uint32_t idx = DeviceFormatSupportConstants::ADDITIONAL_FORMAT_BASE_IDX +
                             (formatIdx - DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER);
        if (idx < formatProperties_.size()) {
            props = formatProperties_[idx];
        }
    }
    if (formatIdx != 0u && props.optimalTilingFeatures == 0u) {
        MLN_LOG_ERR(
            "FORMAT UNSUPPORTED AT RUNTIME: format=%u requested but features=0 — "
            "driver does not support this format",
            formatIdx);
    }
    return props;
}

AsBuildSizes DeviceMln::GetAccelerationStructureBuildSizes(const AsBuildGeometryInfo& geometry,
    array_view<const AsGeometryTrianglesInfo> triangles, array_view<const AsGeometryAabbsInfo> aabbs,
    array_view<const AsGeometryInstancesInfo> instances) const
{
    // Translate CORE geometry descriptors to Maleoon format and query build sizes.
    const uint32_t geometryCount = static_cast<uint32_t>(triangles.size() + aabbs.size() + instances.size());
    if (geometryCount == 0) {
        return {};
    }

    BASE_NS::vector<MlnAccelerationStructureGeometryDescriptor> geometries(geometryCount);
    BASE_NS::vector<uint32_t> maxPrimitiveCounts(geometryCount);
    uint32_t idx = 0;

    for (const auto& tri : triangles) {
        auto& g = geometries[idx];
        g.extensionCount = 0;
        g.extensions = nullptr;
        g.geometryType = MLN_GEOMETRY_TYPE_TRIANGLES;
        g.flags = static_cast<MlnGeometryFlags>(tri.geometryFlags);
        g.geometry.triangles.extensionCount = 0;
        g.geometry.triangles.extensions = nullptr;
        g.geometry.triangles.vertexFormat = static_cast<MlnFormat>(tri.vertexFormat);
        g.geometry.triangles.vertexStride = tri.vertexStride;
        g.geometry.triangles.maxVertex = tri.maxVertex;
        g.geometry.triangles.indexType = static_cast<MlnIndexType>(tri.indexType);
        g.geometry.triangles.vertexData = 0; // not needed for size query
        g.geometry.triangles.indexData = 0;
        g.geometry.triangles.transformData = 0;
        // primitiveCount = indexCount / 3 for triangles (or maxVertex / 3 if no indices)
        maxPrimitiveCounts[idx] = (tri.indexCount > 0) ? (tri.indexCount / 3) : (tri.maxVertex / 3);
        idx++;
    }
    for (const auto& aabb : aabbs) {
        auto& g = geometries[idx];
        g.extensionCount = 0;
        g.extensions = nullptr;
        g.geometryType = MLN_GEOMETRY_TYPE_AABBS;
        g.flags = static_cast<MlnGeometryFlags>(aabb.geometryFlags);
        g.geometry.aabbs.extensionCount = 0;
        g.geometry.aabbs.extensions = nullptr;
        g.geometry.aabbs.stride = aabb.stride;
        g.geometry.aabbs.data = 0;
        maxPrimitiveCounts[idx] = 1; // single AABB per geometry entry
        idx++;
    }
    for (const auto& inst : instances) {
        auto& g = geometries[idx];
        g.extensionCount = 0;
        g.extensions = nullptr;
        g.geometryType = MLN_GEOMETRY_TYPE_INSTANCES;
        g.flags = static_cast<MlnGeometryFlags>(inst.geometryFlags);
        g.geometry.instances.extensionCount = 0;
        g.geometry.instances.extensions = nullptr;
        g.geometry.instances.arrayOfPointers = inst.arrayOfPointers ? 1u : 0u;
        g.geometry.instances.data = 0;
        maxPrimitiveCounts[idx] = inst.primitiveCount;
        idx++;
    }

    MlnAccelerationStructureBuildGeometryDescriptor buildGeom{};
    buildGeom.extensionCount = 0;
    buildGeom.extensions = nullptr;
    buildGeom.type = (geometry.type == AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
                         ? MLN_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL
                         : MLN_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildGeom.flags = static_cast<MlnBuildAccelerationStructureFlags>(geometry.flags);
    buildGeom.mode = MLN_ACCELERATION_STRUCTURE_BUILD_MODE_BUILD;
    buildGeom.srcAccelerationStructure = MLN_NULL_HANDLE;
    buildGeom.dstAccelerationStructure = MLN_NULL_HANDLE;
    buildGeom.geometryCount = geometryCount;
    buildGeom.geometries = geometries.data();
    buildGeom.geometriesPointers = nullptr;
    buildGeom.scratchData = 0;

    MlnAccelerationStructureBuildSizesDescriptor mlnSizes{};
    mlnSizes.extensionCount = 0;
    mlnSizes.extensions = nullptr;
    MlnStatus st = MlnGetAccelerationStructureBuildSizes(mlnDevice_, &buildGeom, maxPrimitiveCounts.data(), &mlnSizes);
    if (st != MLN_STATUS_SUCCESS) {
        MLN_LOG_ERR("GetAccelerationStructureBuildSizes: MlnGetAccelerationStructureBuildSizes failed (status=%d)",
            static_cast<int>(st));
        return {};
    }

    AsBuildSizes result{};
    result.accelerationStructureSize = static_cast<uint32_t>(mlnSizes.accelerationStructureSize);
    result.updateScratchSize = static_cast<uint32_t>(mlnSizes.updateScratchSize);
    result.buildScratchSize = static_cast<uint32_t>(mlnSizes.buildScratchSize);
    return result;
}

ILowLevelDevice& DeviceMln::GetLowLevelDevice() const
{
    return *lowLevelDevice_;
}

void DeviceMln::WaitForIdle()
{
    if (mlnDevice_) {
        MlnDeviceWaitIdle(mlnDevice_);
    }
}

PlatformGpuMemoryAllocator* DeviceMln::GetPlatformGpuMemoryAllocator()
{
    // TODO Phase 2: Return Maleoon memory allocator
    return nullptr;
}

unique_ptr<Swapchain> DeviceMln::CreateDeviceSwapchain(const SwapchainCreateInfo& swapchainCreateInfo)
{
    MLN_LOG_INIT("DeviceMln::CreateDeviceSwapchain");
    auto swapchain = make_unique<SwapchainMln>(*this, swapchainCreateInfo);
    if (!swapchain->IsValid()) {
        MLN_LOG_ERR("DeviceMln: swapchain creation failed");
        return nullptr;
    }
    return swapchain;
}

void DeviceMln::DestroyDeviceSwapchain()
{
    MLN_LOG_INIT("DeviceMln::DestroyDeviceSwapchain");
    // The Swapchain is owned by the base Device class and destroyed via unique_ptr.
    // No additional cleanup needed here.
}

void DeviceMln::Activate()
{
    // No-op for Maleoon
}

void DeviceMln::Deactivate()
{
    // No-op for Maleoon
}

bool DeviceMln::AllowThreadedProcessing() const
{
    return MLN_ENABLE_MULTITHREADING != 0;
}

GpuQueue DeviceMln::GetValidGpuQueue(const GpuQueue& gpuQueue) const
{
    // Maleoon has a single unified queue
    return {GpuQueue::QueueType::GRAPHICS, 0};
}

uint32_t DeviceMln::GetGpuQueueCount() const
{
    return 1u;
}

void DeviceMln::InitializePipelineCache(array_view<const uint8_t> initialData)
{
    if (!mlnDevice_) {
        return;
    }
    MlnProgramCacheDescriptor cacheDesc{};
    cacheDesc.flags = 0;
    if (!initialData.empty()) {
        cacheDesc.rawDataSize = initialData.size();
        cacheDesc.rawData = initialData.data();
    } else {
        cacheDesc.rawDataSize = 0;
        cacheDesc.rawData = nullptr;
    }
    mlnProgramCache_ = MlnCreateProgramCache(mlnDevice_, &cacheDesc);
    if (mlnProgramCache_) {
        MLN_LOG_INIT("DeviceMln: MlnCreateProgramCache OK");
    }
}

vector<uint8_t> DeviceMln::GetPipelineCache() const
{
    if (!mlnDevice_ || !mlnProgramCache_) {
        return {};
    }
    size_t cacheSize = 0;
    if (MlnGetProgramCacheData(mlnDevice_, mlnProgramCache_, &cacheSize, nullptr) != MLN_STATUS_SUCCESS) {
        return {};
    }
    if (cacheSize == 0) {
        return {};
    }
    vector<uint8_t> data(cacheSize);
    if (MlnGetProgramCacheData(mlnDevice_, mlnProgramCache_, &cacheSize, data.data()) != MLN_STATUS_SUCCESS) {
        return {};
    }
    return data;
}

unique_ptr<GpuBuffer> DeviceMln::CreateGpuBuffer(const GpuBufferDesc& desc)
{
    return make_unique<GpuBufferMln>(*this, desc);
}

unique_ptr<GpuBuffer> DeviceMln::CreateGpuBuffer(const GpuAccelerationStructureDesc& desc)
{
    return make_unique<GpuBufferMln>(*this, desc);
}

unique_ptr<GpuBuffer> DeviceMln::CreateGpuBuffer(const BackendSpecificBufferDesc& desc)
{
    // Backend-specific buffer wrapping not yet supported for Maleoon
    MLN_LOG_ERR("DeviceMln::CreateGpuBuffer(BackendSpecific) not supported");
    return nullptr;
}

unique_ptr<GpuImage> DeviceMln::CreateGpuImage(const GpuImageDesc& desc)
{
    return make_unique<GpuImageMln>(*this, desc);
}

unique_ptr<GpuImage> DeviceMln::CreateGpuImageView(const GpuImageDesc& desc, const GpuImagePlatformData& platformData)
{
    return make_unique<GpuImageMln>(*this, desc, platformData);
}

unique_ptr<GpuImage> DeviceMln::CreateGpuImageView(
    const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData)
{
    // Backend-specific image view wrapping not yet supported for Maleoon
    MLN_LOG_ERR("DeviceMln::CreateGpuImageView(BackendSpecific) not supported");
    return nullptr;
}

vector<unique_ptr<GpuImage>> DeviceMln::CreateGpuImageViews(const Swapchain& swapchain)
{
    const GpuImageDesc& desc = swapchain.GetDesc();
    const auto& swapchainPlat = static_cast<const SwapchainMln&>(swapchain).GetPlatformData();

    vector<unique_ptr<GpuImage>> gpuImages(swapchainPlat.images.size());
    MLN_LOG_INIT("DeviceMln::CreateGpuImageViews: wrapping %zu swapchain images", swapchainPlat.images.size());
    for (size_t idx = 0; idx < gpuImages.size(); ++idx) {
        MLN_LOG_INIT("DeviceMln::CreateGpuImageViews: image[%zu] resource=%p, view=%p", idx,
            reinterpret_cast<void*>(swapchainPlat.images[idx]),
            reinterpret_cast<void*>(
                idx < swapchainPlat.imageViews.size() ? swapchainPlat.imageViews[idx] : MLN_NULL_HANDLE));
        GpuImagePlatformDataMln gpuImagePlat;
        gpuImagePlat.resource = swapchainPlat.images[idx];
        if (idx < swapchainPlat.imageViews.size()) {
            gpuImagePlat.resourceView = swapchainPlat.imageViews[idx];
        }
        gpuImages[idx] = make_unique<GpuImageMln>(*this, desc, gpuImagePlat);
        MLN_LOG_INIT("DeviceMln::CreateGpuImageViews: image[%zu] wrapped OK", idx);
    }
    MLN_LOG_INIT("DeviceMln::CreateGpuImageViews: created %u swapchain image views (%ux%u)",
        static_cast<uint32_t>(gpuImages.size()), desc.width, desc.height);
    return gpuImages;
}

unique_ptr<GpuSampler> DeviceMln::CreateGpuSampler(const GpuSamplerDesc& desc)
{
    return make_unique<GpuSamplerMln>(*this, desc);
}

unique_ptr<RenderFrameSync> DeviceMln::CreateRenderFrameSync()
{
    return make_unique<RenderFrameSyncMln>(*this);
}

unique_ptr<RenderBackend> DeviceMln::CreateRenderBackend(GpuResourceManager& gpuResourceMgr, CORE_NS::ITaskQueue* queue)
{
    return make_unique<RenderBackendMln>(*this, gpuResourceMgr, queue);
}

unique_ptr<ShaderModule> DeviceMln::CreateShaderModule(const ShaderModuleCreateInfo& data)
{
    return make_unique<ShaderModuleMln>(*this, data);
}

unique_ptr<ShaderModule> DeviceMln::CreateComputeShaderModule(const ShaderModuleCreateInfo& data)
{
    return make_unique<ShaderModuleMln>(*this, data);
}

unique_ptr<GpuShaderProgram> DeviceMln::CreateGpuShaderProgram(const GpuShaderProgramCreateData& data)
{
    return make_unique<GpuShaderProgramMln>(data);
}

unique_ptr<GpuComputeProgram> DeviceMln::CreateGpuComputeProgram(const GpuComputeProgramCreateData& data)
{
    return make_unique<GpuComputeProgramMln>(data);
}

unique_ptr<NodeContextDescriptorSetManager> DeviceMln::CreateNodeContextDescriptorSetManager()
{
    return make_unique<NodeContextDescriptorSetManagerMln>(*this);
}

unique_ptr<NodeContextPoolManager> DeviceMln::CreateNodeContextPoolManager(
    GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue)
{
    return make_unique<NodeContextPoolManagerMln>(*this, gpuResourceMgr, gpuQueue);
}

unique_ptr<GraphicsPipelineStateObject> DeviceMln::CreateGraphicsPipelineStateObject(const GpuShaderProgram& gpuProgram,
    const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
    const VertexInputDeclarationView& vertexInputDeclaration,
    const ShaderSpecializationConstantDataView& specializationConstants,
    array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
    const array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, uint32_t subpassIndex,
    const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    return make_unique<GraphicsPipelineStateObjectMln>(*this, gpuProgram, graphicsState, pipelineLayout,
        vertexInputDeclaration, specializationConstants, dynamicStates, renderPassSubpassDescs, subpassIndex,
        renderPassData, pipelineLayoutData);
}

unique_ptr<ComputePipelineStateObject> DeviceMln::CreateComputePipelineStateObject(const GpuComputeProgram& gpuProgram,
    const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
    const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    return make_unique<ComputePipelineStateObjectMln>(
        *this, gpuProgram, pipelineLayout, specializationConstants, pipelineLayoutData);
}

unique_ptr<GpuSemaphore> DeviceMln::CreateGpuSemaphore()
{
    return make_unique<GpuSemaphoreMln>(*this);
}

unique_ptr<GpuSemaphore> DeviceMln::CreateGpuSemaphoreView(uint64_t handle)
{
    return make_unique<GpuSemaphoreMln>(*this, handle);
}

// ============================================================================
// Factory function
// ============================================================================

unique_ptr<Device> CreateDeviceMln(RenderContext& renderContext)
{
    MLN_LOG_INIT("CreateDeviceMln: creating Maleoon backend device");
    return make_unique<DeviceMln>(renderContext);
}

// ============================================================================
// LowLevelDeviceMln
// ============================================================================

LowLevelDeviceMln::LowLevelDeviceMln(DeviceMln& deviceMln)
    : deviceMln_(deviceMln), gpuResourceMgr_(static_cast<GpuResourceManager&>(deviceMln_.GetGpuResourceManager()))
{}

DeviceBackendType LowLevelDeviceMln::GetBackendType() const
{
    return DeviceBackendType::MALEOON;
}

const DevicePlatformDataMln& LowLevelDeviceMln::GetPlatformDataMln() const
{
    return static_cast<const DevicePlatformDataMln&>(deviceMln_.GetPlatformData());
}

GpuBufferPlatformDataMln LowLevelDeviceMln::GetBuffer(RenderHandle handle) const
{
    if (deviceMln_.GetLockResourceBackendAccess()) {
        auto* buffer = gpuResourceMgr_.GetBuffer<GpuBufferMln>(handle);
        if (buffer) {
            return buffer->GetPlatformData();
        }
    } else {
        PLUGIN_LOG_E("low level device methods can only be used within specific methods");
    }
    return {};
}

GpuImagePlatformDataMln LowLevelDeviceMln::GetImage(RenderHandle handle) const
{
    if (deviceMln_.GetLockResourceBackendAccess()) {
        auto* image = gpuResourceMgr_.GetImage<GpuImageMln>(handle);
        if (image) {
            return image->GetPlatformData();
        }
    } else {
        PLUGIN_LOG_E("low level device methods can only be used within specific methods");
    }
    return {};
}

GpuSamplerPlatformDataMln LowLevelDeviceMln::GetSampler(RenderHandle handle) const
{
    if (deviceMln_.GetLockResourceBackendAccess()) {
        auto* sampler = gpuResourceMgr_.GetSampler<GpuSamplerMln>(handle);
        if (sampler) {
            return sampler->GetPlatformData();
        }
    } else {
        PLUGIN_LOG_E("low level device methods can only be used within specific methods");
    }
    return {};
}
RENDER_END_NAMESPACE()
