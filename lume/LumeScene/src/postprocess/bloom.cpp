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

#include "bloom.h"

#include <3d/ecs/components/post_process_component.h>
#include <render/device/intf_gpu_resource_manager.h>

#include "entity_converting_value.h"

META_TYPE(RENDER_NS::RenderHandle)

SCENE_BEGIN_NAMESPACE()
namespace {

struct RenderHandleImageConverter {
    using SourceType = IImage::Ptr;
    using TargetType = RENDER_NS::RenderHandle;

    RenderHandleImageConverter(IInternalScene::Ptr scene) : scene_(scene) {}

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<IImage>(any);
        if (auto scene = scene_.lock()) {
            if (RENDER_NS::RenderHandleUtil::IsValid(v)) {
                scene
                    ->AddTask([&] {
                        if (!p) {
                            p = interface_pointer_cast<IImage>(scene->CreateObject(ClassId::Image));
                        }
                        if (auto i = interface_cast<IRenderResource>(p)) {
                            i->SetRenderHandle(scene->GetRenderContext().GetDevice().GetGpuResourceManager().Get(v));
                        }
                    })
                    .Wait();
            } else {
                p = nullptr;
            }
        }
        return p;
    }

    TargetType ConvertToTarget(const SourceType& v) const
    {
        TargetType handle;
        if (auto scene = scene_.lock()) {
            scene
                ->AddTask([&] {
                    if (auto i = interface_cast<IRenderResource>(v)) {
                        handle = i->GetRenderHandle().GetHandle();
                    }
                })
                .Wait();
        }
        return handle;
    }

    IInternalScene::WeakPtr scene_;
};
} // namespace

bool Bloom::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    BASE_NS::string cpath = "PostProcessComponent.bloomConfiguration." + path;
    if (p->GetName() == "DirtMaskImage") {
        auto ep = object_->CreateProperty(cpath).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(
                   META_NS::IValue::Ptr(new ConvertingValue<RenderHandleImageConverter>(ep, { object_->GetScene() })));
    }
    if (p->GetName() == "Enabled") {
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return flags_ && i &&
               i->PushValue(META_NS::IValue::Ptr(
                   new ConvertingValue<PPEffectEnabledConverter<CORE3D_NS::PostProcessComponent::BLOOM_BIT>>(
                       flags_, { flags_ })));
    }
    return AttachEngineProperty(p, cpath);
}

SCENE_END_NAMESPACE()