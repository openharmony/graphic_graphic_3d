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

#include "bloom.h"
#include "color_fringe.h"
#include "tonemap.h"
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

META_NS::IObject::Ptr PostProcess::CreateEffect(const META_NS::IProperty::Ptr& p, const META_NS::ClassInfo& id)
{
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
    if (auto obj = interface_pointer_cast<T>(CreateEffect(p, id))) {
        if (META_NS::Property<typename T::Ptr> prop { p }) {
            prop->SetValue(obj);
            return true;
        }
    }
    return false;
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
    if (name == "ColorFringe") {
        return InitEffect<IColorFringe>(p, ClassId::ColorFringe);
    }
    if (name == "Vignette") {
        return InitEffect<IVignette>(p, ClassId::Vignette);
    }

    return false;
}
SCENE_END_NAMESPACE()
