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

#include <3d/ecs/components/render_handle_component.h>
#include <render/device/intf_gpu_resource_manager.h>

#include "bitmap.h"
#include "entity_converting_value.h"

SCENE_BEGIN_NAMESPACE()
namespace {
// clang-format off
constexpr const char* const SAMPLERNAMES[] = {
    "Unknown",
    "CORE_DEFAULT_SAMPLER_NEAREST_REPEAT",
    "CORE_DEFAULT_SAMPLER_NEAREST_CLAMP",
    "CORE_DEFAULT_SAMPLER_LINEAR_REPEAT",
    "CORE_DEFAULT_SAMPLER_LINEAR_CLAMP",
    "CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT",
    "CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP"
};
static_assert(sizeof(SAMPLERNAMES) / sizeof(*SAMPLERNAMES) == size_t(TextureSampler::END_OF_LIST));
// clang-format on

static TextureSampler NameToTextureSampler(BASE_NS::string_view name)
{
    size_t i = 0;
    for (; i != size_t(TextureSampler::END_OF_LIST) && name != SAMPLERNAMES[i]; ++i) {
    }

    TextureSampler s = TextureSampler::UNKNOWN;
    if (i < size_t(TextureSampler::END_OF_LIST)) {
        s = static_cast<TextureSampler>(i);
    }
    return s;
}

static BASE_NS::string TextureSamplerToName(TextureSampler s)
{
    size_t i = static_cast<size_t>(s);
    if (i < size_t(TextureSampler::END_OF_LIST)) {
        return SAMPLERNAMES[i];
    }
    return "Unknown";
}

struct SamplerConverter {
    using SourceType = TextureSampler;
    using TargetType = CORE_NS::EntityReference;

    SamplerConverter(IInternalScene::Ptr scene) : scene_(scene) {}

    SourceType ConvertToSource(META_NS::IAny&, const TargetType& v) const
    {
        TextureSampler s = TextureSampler::UNKNOWN;
        if (auto scene = scene_.lock()) {
            auto f = scene->AddTask([&] {
                BASE_NS::string name;
                if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                        scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
                    name = scene->GetRenderContext().GetDevice().GetGpuResourceManager().GetName(
                        rhman->GetRenderHandleReference(v));
                }
                return name;
            });
            s = NameToTextureSampler(f.GetResult());
        }
        return s;
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        CORE_NS::EntityReference ent;
        if (auto scene = scene_.lock()) {
            auto name = TextureSamplerToName(v);
            scene
                ->AddTask([&] {
                    if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                            scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
                        auto handle =
                            scene->GetRenderContext().GetDevice().GetGpuResourceManager().GetSamplerHandle(name);
                        ent = CORE3D_NS::GetOrCreateEntityReference(
                            scene->GetEcsContext().GetNativeEcs()->GetEntityManager(), *rhman, handle);
                    }
                })
                .Wait();
        }
        return ent;
    }

    IInternalScene::WeakPtr scene_;
};
} // namespace

bool Texture::Build(const META_NS::IMetadata::Ptr& d)
{
    if (!Super::Build(d)) {
        return false;
    }

    auto ip = d->GetProperty<size_t>("Index");
    if (!ip) {
        return false;
    }
    index_ = META_NS::GetValue(ip);

    return true;
}

bool Texture::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    BASE_NS::string cpath = "MaterialComponent.textures[" + BASE_NS::string(BASE_NS::to_string(index_)) + "]." + path;

    if (p->GetName() == "Image") {
        auto ep = object_->CreateEngineProperty(cpath).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(
                   new RenderResourceValue<IBitmap>(ep, { object_->GetScene(), ClassId::Bitmap })));
    }
    return AttachEngineProperty(p, cpath);
}

SCENE_END_NAMESPACE()
