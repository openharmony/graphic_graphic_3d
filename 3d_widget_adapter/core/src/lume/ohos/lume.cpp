/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
