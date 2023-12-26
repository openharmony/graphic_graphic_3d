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
#include "lume.h"

#include <base/containers/array_view.h>

#include <core/ecs/intf_system_graph_loader.h>
#include <core/implementation_uids.h>
#include <core/namespace.h>
#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>
#include <core/property/intf_property_handle.h>
#include <core/plugin/intf_plugin_register.h>

namespace OHOS::Render3D {
Lume::~Lume()
{
}

CORE_NS::PlatformCreateInfo Lume::ToEnginePlatformData(const PlatformData& data) const
{
    return {
        data.coreRootPath_.c_str(),
        data.appRootPath_.c_str(),
        data.appPluginPath_.c_str(),
        data.hapInfo_.hapPath_.c_str(),
        data.hapInfo_.bundleName_.c_str(),
        data.hapInfo_.moduleName_.c_str()
    };
}
void Lume::RegisterAssertPath()
{}
} // namespace OHOS::Render3D
