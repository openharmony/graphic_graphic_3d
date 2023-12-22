/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "graphics_manager.h"
#include "i_engine.h"
#include "platform_data.h"
#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
PlatformData GraphicsManager::GetPlatformData() const
{
    #define TO_STRING(name) #name
    #define PLATFORM_PATH_NAME(name) TO_STRING(name)
    PlatformData data {
        PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH),
    };
    #undef TO_STRING
    #undef PLATFORM_PATH_NAME
    WIDGET_LOGD("platformdata %s, %s, %s,", data.coreRootPath_.c_str(),
        data.appRootPath_.c_str(), data.appPluginPath_.c_str());
    return data;
}

PlatformData GraphicsManager::GetPlatformData(const HapInfo& hapInfo) const
{
    #define TO_STRING(name) #name
    #define PLATFORM_PATH_NAME(name) TO_STRING(name)
    PlatformData data {
        PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH),
        hapInfo,
    };
    #undef TO_STRING
    #undef PLATFORM_PATH_NAME
    WIDGET_LOGD("platformdata %{public}s, %{public}s, %{public}s,", data.coreRootPath_.c_str(),
        data.appRootPath_.c_str(), data.appPluginPath_.c_str());

    WIDGET_LOGD("HapInfo %{public}s, %{public}s, %{public}s,", data.hapInfo_.hapPath_.c_str(),
        data.hapInfo_.bundleName_.c_str(), data.hapInfo_.moduleName_.c_str());
    return data;
}

GraphicsManager& GraphicsManager::GetInstance()
{
    static GraphicsManager graphicsManager;
    return graphicsManager;
}

GraphicsManager::~GraphicsManager()
{}
} // namespace OHOS::Render3D
