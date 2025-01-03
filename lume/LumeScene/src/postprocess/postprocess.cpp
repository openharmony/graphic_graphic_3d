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

#include "postprocess.h"

#include <scene/ext/intf_render_resource.h>

#include <render/device/intf_gpu_resource_manager.h>

#include <meta/api/make_callback.h>

#include "../converting_value.h"

SCENE_BEGIN_NAMESPACE()
namespace {
struct TonemapConverter {
    TonemapConverter(META_NS::Property<uint32_t> flags) : flags_(flags) {}

    using SourceType = ITonemap::Ptr;
    using TargetType = RENDER_NS::TonemapConfiguration;

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<ITonemap>(any);
        if (!p) {
            p = META_NS::GetObjectRegistry().Create<ITonemap>(ClassId::Tonemap);
        }
        if (p) {
            p->Enabled()->SetValue(flags_->GetValue() & CORE3D_NS::PostProcessComponent::TONEMAP_BIT);
            p->Type()->SetValue(static_cast<TonemapType>(v.tonemapType));
            p->Exposure()->SetValue(v.exposure);
        }
        return p;
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        TargetType res {};
        if (v && v->Enabled()->GetValue()) {
            flags_->SetValue(flags_->GetValue() | CORE3D_NS::PostProcessComponent::TONEMAP_BIT);
        } else {
            flags_->SetValue(flags_->GetValue() & ~CORE3D_NS::PostProcessComponent::TONEMAP_BIT);
        }
        if (v) {
            res.tonemapType = static_cast<RENDER_NS::TonemapConfiguration::TonemapType>(v->Type()->GetValue());
            res.exposure = v->Exposure()->GetValue();
        }
        return res;
    }

private:
    META_NS::Property<uint32_t> flags_;
};

struct BloomConverter {
    BloomConverter(const IInternalScene::Ptr& scene, META_NS::Property<uint32_t> flags) : scene_(scene), flags_(flags)
    {}

    using SourceType = IBloom::Ptr;
    using TargetType = RENDER_NS::BloomConfiguration;
    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<IBloom>(any);
        if (!p) {
            p = META_NS::GetObjectRegistry().Create<IBloom>(ClassId::Bloom);
        }
        if (p) {
            p->Enabled()->SetValue(flags_->GetValue() & CORE3D_NS::PostProcessComponent::BLOOM_BIT);
            p->Type()->SetValue(static_cast<BloomType>(v.bloomType));
            p->Quality()->SetValue(static_cast<EffectQualityType>(v.bloomQualityType));
            p->ThresholdHard()->SetValue(v.thresholdHard);
            p->ThresholdSoft()->SetValue(v.thresholdSoft);
            p->AmountCoefficient()->SetValue(v.amountCoefficient);
            p->DirtMaskCoefficient()->SetValue(v.dirtMaskCoefficient);
            p->Scatter()->SetValue(v.scatter);
            p->ScaleFactor()->SetValue(v.scaleFactor);

            if (RENDER_NS::RenderHandleUtil::IsValid(v.dirtMaskImage)) {
                if (auto scene = scene_.lock()) {
                    scene
                        ->AddTask([&] {
                            auto& resources = scene->GetRenderContext().GetDevice().GetGpuResourceManager();
                            if (auto handle = resources.Get(v.dirtMaskImage)) {
                                if (auto bitm = META_NS::GetObjectRegistry().Create<IBitmap>(ClassId::Bitmap)) {
                                    if (auto i = interface_cast<IRenderResource>(bitm)) {
                                        i->SetRenderHandle(scene, handle);
                                    }
                                    p->DirtMaskImage()->SetValue(bitm);
                                }
                            }
                        })
                        .Wait();
                }
            }
            p->UseCompute()->SetValue(v.useCompute);
        }
        return p;
    }
    TargetType ConvertToTarget(const SourceType& p) const
    {
        TargetType v {};
        if (p && p->Enabled()->GetValue()) {
            flags_->SetValue(flags_->GetValue() | CORE3D_NS::PostProcessComponent::BLOOM_BIT);
        } else {
            flags_->SetValue(flags_->GetValue() & ~CORE3D_NS::PostProcessComponent::BLOOM_BIT);
        }
        if (p) {
            v.bloomType = static_cast<RENDER_NS::BloomConfiguration::BloomType>(p->Type()->GetValue());
            v.bloomQualityType = static_cast<RENDER_NS::BloomConfiguration::BloomQualityType>(p->Quality()->GetValue());
            v.thresholdHard = p->ThresholdHard()->GetValue();
            v.thresholdSoft = p->ThresholdSoft()->GetValue();
            v.amountCoefficient = p->AmountCoefficient()->GetValue();
            v.dirtMaskCoefficient = p->DirtMaskCoefficient()->GetValue();
            if (auto image = interface_cast<IRenderResource>(p->DirtMaskImage()->GetValue())) {
                v.dirtMaskImage = image->GetRenderHandle().GetHandle();
            }
            v.useCompute = p->UseCompute()->GetValue();
            v.scatter = p->Scatter()->GetValue();
            v.scaleFactor = p->ScaleFactor()->GetValue();
        }
        return v;
    }

private:
    IInternalScene::WeakPtr scene_;
    META_NS::Property<uint32_t> flags_;
};
} // namespace

CORE_NS::Entity PostProcess::CreateEntity(const IInternalScene::Ptr& scene)
{
    auto& ecs = scene->GetEcsContext();
    auto pp = ecs.FindComponent<CORE3D_NS::PostProcessComponent>();
    if (!pp) {
        return CORE_NS::Entity {};
    }
    auto ent = ecs.GetNativeEcs()->GetEntityManager().Create();
    pp->Create(ent);
    return ent;
}
bool PostProcess::SetEcsObject(const IEcsObject::Ptr& obj)
{
    auto p = META_NS::GetObjectRegistry().Create<IInternalPostProcess>(ClassId::PostProcessComponent);
    if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
        if (acc->SetEcsObject(obj)) {
            pp_ = p;
            Init(obj);
        }
    }
    return pp_ != nullptr;
}
IEcsObject::Ptr PostProcess::GetEcsObject() const
{
    auto acc = interface_cast<IEcsObjectAccess>(pp_);
    return acc ? acc->GetEcsObject() : nullptr;
}
void PostProcess::Init(const IEcsObject::Ptr& eobj)
{
    Tonemap()->PushValue(
        META_NS::IValue::Ptr(new ConvertingValue<TonemapConverter>(pp_->Tonemap(), { pp_->EnableFlags() })));
    Bloom()->PushValue(META_NS::IValue::Ptr(
        new ConvertingValue<BloomConverter>(pp_->Bloom(), { eobj->GetScene(), pp_->EnableFlags() })));
}

SCENE_END_NAMESPACE()
