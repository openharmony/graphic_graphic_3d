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
#include "texture.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_scene.h>

#include <3d/ecs/components/render_handle_component.h>
#include <render/device/intf_gpu_resource_manager.h>

#include "../entity_converting_value.h"
#include "../resource/image.h"
#include "sampler.h"

SCENE_BEGIN_NAMESPACE()

namespace {

struct SamplerConverter {
    SamplerConverter(const IInternalScene::Ptr& scene) : scene_(scene) {}

    using SourceType = ISampler::Ptr;
    using TargetType = CORE_NS::EntityReference;

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<ISampler>(any);
        if (auto scene = scene_.lock()) {
            scene->RunDirectlyOrInTask([&] {
                if (!p) {
                    p = META_NS::GetObjectRegistry().Create<ISampler>(ClassId::Sampler);
                    if (auto i = interface_cast<ISamplerInternal>(p)) {
                        i->SetScene(scene);
                    }
                }
                auto& ctx = scene->GetRenderContext();
                if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                        scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
                    if (auto internal = interface_cast<ISamplerInternal>(p)) {
                        internal->UpdateSamplerFromHandle(ctx, rhman->GetRenderHandleReference(v));
                    }
                }
            });
        }
        return p;
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        TargetType res;
        if (auto scene = scene_.lock(); scene && v) {
            scene->RunDirectlyOrInTask([&] {
                if (auto i = interface_cast<ISamplerInternal>(v)) {
                    if (auto handle = i->GetHandleFromSampler(scene->GetRenderContext())) {
                        res = scene->GetEcsContext().GetRenderHandleEntity(handle);
                    }
                }
            });
        }
        return res;
    }

private:
    IInternalScene::WeakPtr scene_;
};

} // namespace

bool Texture::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (index_ == -1) {
        CORE_LOG_E("Texture index is uninitialised");
        return false;
    }
    BASE_NS::string cpath = "MaterialComponent.textures[" + BASE_NS::string(BASE_NS::to_string(index_)) + "]." + path;

    if (p->GetName() == "Image") {
        auto ep = object_->CreateProperty(cpath).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(
                   META_NS::IValue::Ptr(new RenderResourceValue<IImage>(ep, { GetInternalScene(), ClassId::Image })));
    }
    if (p->GetName() == "Sampler") {
        auto ep = object_->CreateProperty(cpath).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(new ConvertingValue<SamplerConverter>(ep, { GetInternalScene() })));
    }
    return AttachEngineProperty(p, cpath);
}

SCENE_END_NAMESPACE()
