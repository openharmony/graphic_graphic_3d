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

#include "meta_object_initializer.h"

#include <cstdint>
#include <windows.h>

#include <base/containers/string.h>
#include <base/util/uid.h>
#include <core/intf_engine.h>
#include <core/os/platform_create_info.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include <meta/base/plugin.h>

namespace {
uint32_t gInitCount = 0;
}

#if CORE_DYNAMIC == 1

namespace {
HMODULE agpEngineHandle = nullptr;
}

CORE_NS::IPluginRegister& (*CORE_NS::GetPluginRegister)() { nullptr };
void (*CORE_NS::CreatePluginRegistry)(const struct CORE_NS::PlatformCreateInfo& platformCreateInfo) { nullptr };

#endif

namespace environmentSetup {

void InitializeMetaObject(const char* const dllPath)
{
    InitializeMetaObject(dllPath, CORE_NS::ILogger::LogLevel::LOG_VERBOSE);
}

void InitializeMetaObject(const char* const dllPath, CORE_NS::ILogger::LogLevel logLevel)
{
    if (gInitCount == 0) {
        char MyFileName[MAX_PATH + 1];
#ifdef _DEBUG
        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
#if CORE_DYNAMIC == 1
        agpEngineHandle = LoadLibrary(dllPath);
        CORE_NS::GetPluginRegister =
            decltype(CORE_NS::GetPluginRegister)(GetProcAddress(agpEngineHandle, MAKEINTRESOURCE(2))); // 2: parm
        CORE_NS::CreatePluginRegistry =
            decltype(CORE_NS::CreatePluginRegistry)(GetProcAddress(agpEngineHandle, MAKEINTRESOURCE(1)));

        // Get the path to the engine.dll. to use as base for plugins.
        GetModuleFileNameA(agpEngineHandle, MyFileName, sizeof(MyFileName));
#else
        // Get the path to the executable. to use as base for plugins.
        GetModuleFileNameA(GetModuleHandleA(NULL), MyFileName, sizeof(MyFileName));
#endif
        CORE_NS::GetLogger()->SetLogLevel(logLevel);

        BASE_NS::string path = MyFileName;
        auto pos = path.find_last_of("\\");
        path.resize(pos + 1);

        CORE_NS::CreatePluginRegistry(
            CORE_NS::PlatformCreateInfo { path.c_str(), "", (path + "plugins\\").c_str() }); // load plugins etc..*/

        const BASE_NS::Uid lst[] { META_NS::META_OBJECT_PLUGIN_UID };
        CORE_NS::GetPluginRegister().LoadPlugins(lst); // Load all plugins
    }
    gInitCount++;
}

void ShutdownMetaObject()
{
    if (gInitCount > 0) {
        gInitCount--;
        if (gInitCount == 0) {
            CORE_NS::GetPluginRegister().UnloadPlugins({}); // Unload all plugins
#if CORE_DYNAMIC == 1
            if (agpEngineHandle) {
                FreeLibrary(agpEngineHandle);
            }
#endif
        }
    }
}

} // namespace environmentSetup
