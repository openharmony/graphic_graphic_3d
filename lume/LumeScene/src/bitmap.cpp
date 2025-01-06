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

#include "bitmap.h"

#include <scene/ext/intf_ecs_context.h>

#include <render/device/intf_gpu_resource_manager.h>

#include "util.h"

SCENE_BEGIN_NAMESPACE()

bool Bitmap::SetRenderHandle(
    const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference handle, CORE_NS::EntityReference ent)
{
    if (handle.GetHandleType() != RENDER_NS::RenderHandleType::GPU_IMAGE) {
        CORE_LOG_E("Bitmap requires GPU_IMAGE handle");
        return false;
    }
    {
        std::unique_lock lock { mutex_ };
        if (handle_.GetHandle() == handle.GetHandle() && entity_ == ent) {
            return true;
        }
        if (ent) {
            handle_ = handle;
            entity_ = ent;
            return true;
        }
    }

    RENDER_NS::GpuImageDesc desc;
    scene
        ->AddTask([&] {
            if (!ent) {
                ent = HandleFromRenderResource(scene, handle);
            }
            auto& resources = scene->GetRenderContext().GetDevice().GetGpuResourceManager();
            desc = resources.GetImageDescriptor(handle);
        })
        .Wait();

    if (!ent) {
        return false;
    }
    {
        std::unique_lock lock { mutex_ };
        handle_ = handle;
        entity_ = ent;
    }
    META_ACCESS_PROPERTY(Size)->SetValue({ desc.width, desc.height });
    META_NS::Invoke<META_NS::IOnChanged>(EventOnResourceChanged(META_NS::MetadataQuery::EXISTING));
    return true;
}
RENDER_NS::RenderHandleReference Bitmap::GetRenderHandle() const
{
    std::shared_lock lock { mutex_ };
    return handle_;
}
CORE_NS::EntityReference Bitmap::GetEntity() const
{
    std::shared_lock lock { mutex_ };
    return entity_;
}

SCENE_END_NAMESPACE()
