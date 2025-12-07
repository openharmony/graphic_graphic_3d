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

#include <3d/ecs/components/name_component.h>

#include <meta/api/util.h>

#include "bloom.h"
#include "blur.h"
#include "color_conversion.h"
#include "color_fringe.h"
#include "dof.h"
#include "fxaa.h"
#include "lens_flare.h"
#include "motion_blur.h"
#include "taa.h"
#include "tonemap.h"
#include "upscale.h"
#include "util.h"
#include "vignette.h"

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity PostProcess::CreateEntity(const IInternalScene::Ptr& scene)
{
    if (scene) {
        const auto& ecs = scene->GetEcsContext();
        if (auto pp = ecs.FindComponent<CORE3D_NS::PostProcessComponent>()) {
            if (auto nameManager = ecs.FindComponent<CORE3D_NS::NameComponent>()) {
                if (auto ent = ecs.GetNativeEcs()->GetEntityManager().Create(); CORE_NS::EntityUtil::IsValid(ent)) {
                    pp->Create(ent);
                    nameManager->Create(ent);
                    return ent;
                }
            }
        }
    }
    return {};
}
bool PostProcess::SetEcsObject(const IEcsObject::Ptr& obj)
{
    auto p = META_NS::GetObjectRegistry().Create<IInternalPostProcess>(ClassId::PostProcessComponent);
    if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
        if (acc->SetEcsObject(obj)) {
            pp_ = p;
        }
    }
    return pp_ != nullptr;
}
IEcsObject::Ptr PostProcess::GetEcsObject() const
{
    auto acc = interface_cast<IEcsObjectAccess>(pp_);
    return acc ? acc->GetEcsObject() : nullptr;
}

META_NS::IObject::Ptr PostProcess::CreateEffect(
    const META_NS::IProperty::Ptr& p, const META_NS::ClassInfo& id, BASE_NS::Uid pid)
{
    if (!(pp_ && p->IsCompatible(pid))) {
        return {};
    }
    auto object = META_NS::GetObjectRegistry().Create(id);
    if (auto i = interface_cast<META_NS::IMutableContainable>(object)) {
        i->SetParent(interface_pointer_cast<META_NS::IObject>(p));
    }
    if (auto i = interface_cast<IPPEffectInit>(object)) {
        i->Init(pp_->EnableFlags());
    }
    if (auto access = interface_cast<IEcsObjectAccess>(object)) {
        access->SetEcsObject(GetEcsObject());
    }
    return object;
}

template<typename T>
bool PostProcess::InitEffect(const META_NS::IProperty::Ptr& p, const META_NS::ClassInfo& id)
{
    using PropertyType = typename T::Ptr;
    static constexpr auto compatibleTypeId = META_NS::UidFromType<PropertyType>();
    return META_NS::SetValue<PropertyType>(p, interface_pointer_cast<T>(CreateEffect(p, id, compatibleTypeId)));
}

bool PostProcess::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (!p) {
        return false;
    }
    const auto name = p->GetName();
    if (name == "Tonemap") {
        return InitEffect<ITonemap>(p, ClassId::Tonemap);
    }
    if (name == "Bloom") {
        return InitEffect<IBloom>(p, ClassId::Bloom);
    }
    if (name == "Blur") {
        return InitEffect<IBlur>(p, ClassId::Blur);
    }
    if (name == "MotionBlur") {
        return InitEffect<IMotionBlur>(p, ClassId::MotionBlur);
    }
    if (name == "ColorConversion") {
        return InitEffect<IColorConversion>(p, ClassId::ColorConversion);
    }
    if (name == "ColorFringe") {
        return InitEffect<IColorFringe>(p, ClassId::ColorFringe);
    }
    if (name == "DepthOfField") {
        return InitEffect<IDepthOfField>(p, ClassId::DepthOfField);
    }
    if (name == "Fxaa") {
        return InitEffect<IFxaa>(p, ClassId::Fxaa);
    }
    if (name == "Taa") {
        return InitEffect<ITaa>(p, ClassId::Taa);
    }
    if (name == "Vignette") {
        return InitEffect<IVignette>(p, ClassId::Vignette);
    }
    if (name == "LensFlare") {
        return InitEffect<ILensFlare>(p, ClassId::LensFlare);
    }
    if (name == "Upscale") {
        return InitEffect<IUpscale>(p, ClassId::Upscale);
    }

    return false;
}
SCENE_END_NAMESPACE()
