/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef CORE_LIBRARY_LOADER_H
#define CORE_LIBRARY_LOADER_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>

bool LoadCoreLibrary(BASE_NS::string_view libraryFile = {});
void UnloadCoreLibrary();
BASE_NS::string GetCoreBinaryPath();

//
// Implementation. The using project should define this before including this header once.
// NOTE: Should be done only once and it's better to do this in its own separate compilation unit (i.e. own .cpp file).
//

// Also allow nicer IDE editing of this file by not disabling the implementation part when parsing for intellisense etc.
#if defined(CORE_LIBRARY_LOADER_IMPL) || defined(Q_CREATOR_RUN) || defined(__INTELLISENSE__)

#include <core/plugin/intf_plugin_register.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#endif

#include <base/containers/string.h>

#include <core/os/intf_platform.h>
#include <core/os/platform_create_info.h>

//
// Global data.
//

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
namespace {
#if defined(_WIN32)
HMODULE g_agpEngineHandle = nullptr;
#elif defined(__APPLE__)
auto g_EngineLibrary = BASE_NS::unique_ptr<void, decltype(&dlclose)>(nullptr, dlclose);
#endif
} // namespace
CORE_NS::IPluginRegister& (*CORE_NS::GetPluginRegister)(){ nullptr };
void (*CORE_NS::CreatePluginRegistry)(const struct CORE_NS::PlatformCreateInfo& platformCreateInfo){ nullptr };
#endif // CORE_DYNAMIC

//
// Function implementations.
//

// Does dynamic engine core library loading if necessary.
bool LoadCoreLibrary(BASE_NS::string_view libraryFile)
{
#if defined(_WIN32)
#if defined(_DEBUG)
    // Report memory leaks.
    //_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

    //_CrtSetBreakAlloc(190); //229,268
#endif
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
    if (libraryFile.empty()) {
        g_agpEngineHandle = LoadLibrary("AGPEngineDLL.dll");
    } else {
        g_agpEngineHandle = LoadLibrary(BASE_NS::string(libraryFile).c_str());
    }
    if (!g_agpEngineHandle) {
        return false;
    }
    CORE_NS::GetPluginRegister =
        decltype(CORE_NS::GetPluginRegister)(GetProcAddress(g_agpEngineHandle, MAKEINTRESOURCE(2)));
    CORE_NS::CreatePluginRegistry =
        decltype(CORE_NS::CreatePluginRegistry)(GetProcAddress(g_agpEngineHandle, MAKEINTRESOURCE(1)));
#endif // CORE_DYNAMIC

#elif defined(__APPLE__)
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
    if (!CORE_NS::CreatePluginRegistry) {
        if (libraryFile.empty()) {
            g_EngineLibrary.reset(dlopen("libAGPEngineDLL.dylib", RTLD_LAZY));
        } else {
            g_EngineLibrary.reset(dlopen(BASE_NS::string(libraryFile).c_str(), RTLD_LAZY));
        }

        if (!g_EngineLibrary) {
            printf("failed to load engine %s", dlerror());
            return false;
        }
        CORE_NS::CreatePluginRegistry = reinterpret_cast<decltype(CORE_NS::CreatePluginRegistry)>(
            dlsym(g_EngineLibrary.get(), "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE"));
        CORE_NS::GetPluginRegister = reinterpret_cast<decltype(CORE_NS::GetPluginRegister)>(
            dlsym(g_EngineLibrary.get(), "_ZN4Core17GetPluginRegisterEv"));        
    }
#endif // CORE_DYNAMIC
#endif // __APPLE__

    return true;
}

void UnloadCoreLibrary()
{
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
#if defined(_WIN32)
    if (g_agpEngineHandle) {
        FreeLibrary(g_agpEngineHandle);
    }
    g_agpEngineHandle = nullptr;
#elif defined(__APPLE__)
    dlclose(g_EngineLibrary.get());
    g_EngineLibrary.reset();
#endif // __APPLE__

    CORE_NS::GetPluginRegister = nullptr;
    CORE_NS::CreatePluginRegistry = nullptr;
#endif // CORE_DYNAMIC
}

BASE_NS::string GetCoreBinaryPath()
{
#if defined(_WIN32)
    char binaryFileName[MAX_PATH + 1];
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
    // Get the path to the engine.dll. to use as base for plugins.
    GetModuleFileNameA(g_agpEngineHandle, binaryFileName, sizeof(binaryFileName));
#else
    // Get the path to the executable. to use as base for plugins.
    GetModuleFileNameA(GetModuleHandleA(NULL), binaryFileName, sizeof(binaryFileName));
#endif // CORE_DYNAMIC
    // Remove library/executable filename.
    BASE_NS::string path = binaryFileName;
    auto pos = path.find_last_of("\\");
    path.resize(pos + 1);
#elif defined(__APPLE__)
    BASE_NS::string path;
    if (CORE_NS::GetPluginRegister) {
        Dl_info info {};
        if (dladdr((void*)CORE_NS::GetPluginRegister, &info)) {
            // Remove library/executable filename.
            path = BASE_NS::string { info.dli_fname };
            const auto pos = path.find_last_of("/");
            if (pos != BASE_NS::string::npos) {
                path.resize(pos + 1);
            }
        }
    }
#endif

    return path;
}

#endif // COREDYNAMIC_IMPL

#endif // CORE_LIBRARY_LOADER_H
