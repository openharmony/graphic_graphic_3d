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

#include "render_resource.h"

#include <scene/ext/util.h>

#include <render/device/intf_gpu_resource_manager.h>

SCENE_BEGIN_NAMESPACE()

bool RenderResource::Build(const META_NS::IMetadata::Ptr& d)
{
    auto res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct RenderResource");
            return false;
        }
        context_ = context;
    }
    return res;
}

RENDER_NS::RenderHandleReference RenderResource::GetRenderHandle() const
{
    std::shared_lock lock { mutex_ };
    return handle_;
}

SCENE_END_NAMESPACE()
