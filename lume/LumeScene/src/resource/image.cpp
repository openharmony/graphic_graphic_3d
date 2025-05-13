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

#include "image.h"

#include <mutex>

#include <render/device/intf_gpu_resource_manager.h>

SCENE_BEGIN_NAMESPACE()

bool Image::SetRenderHandle(RENDER_NS::RenderHandleReference handle)
{
    if (handle.GetHandleType() != RENDER_NS::RenderHandleType::GPU_IMAGE) {
        CORE_LOG_E("Image requires GPU_IMAGE handle");
        return false;
    }
    IRenderContext::Ptr context;
    {
        std::unique_lock lock { mutex_ };
        if (handle_.GetHandle() == handle.GetHandle()) {
            return true;
        }
        handle_ = handle;
        context = context_.lock();
        if (!context) {
            return false;
        }
    }

    RENDER_NS::GpuImageDesc desc;
    context
        ->AddTask([&] {
            auto& resources = context->GetRenderer()->GetDevice().GetGpuResourceManager();
            desc = resources.GetImageDescriptor(handle);
        })
        .Wait();

    // todo: notify in right queue
    Size()->SetValue({ desc.width, desc.height });
    if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
        META_NS::Invoke<META_NS::IOnChanged>(ev);
    }
    return true;
}

SCENE_END_NAMESPACE()
