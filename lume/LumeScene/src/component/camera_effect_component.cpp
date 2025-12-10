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

#include "camera_effect_component.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/util.h>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <core/property/property_handle_util.h>

#include <meta/interface/intf_event.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

SCENE_BEGIN_NAMESPACE()

static constexpr BASE_NS::string_view EFFECTS_PROP_NAME = "Effects";

bool CameraEffectComponent::Build(const META_NS::IMetadata::Ptr& d)
{
    return Super::Build(d);
}

bool CameraEffectComponent::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    const auto name = p->GetName();
    if (name == EFFECTS_PROP_NAME) {
        // Prevent the property from being visible in the editor
        META_NS::SetObjectFlags(p, META_NS::ObjectFlagBits::INTERNAL, true);
        p->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>(this, &CameraEffectComponent::UpdateEffects));
        return true;
    }
    return Super::InitDynamicProperty(p, path);
}

bool CameraEffectComponent::SetEcsObject(const IEcsObject::Ptr& obj)
{
    if (Super::SetEcsObject(obj)) {
        UpdateEffects();
        return true;
    }
    return false;
}

META_NS::ArrayProperty<const IEffect::Ptr> CameraEffectComponent::GetEffectsOrNull() const
{
    return GetProperty(EFFECTS_PROP_NAME, META_NS::MetadataQuery::EXISTING);
}

BASE_NS::string CameraEffectComponent::GetName() const
{
    return "CameraEffectComponent";
}

bool CameraEffectComponent::Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext)
{
    UpdateEffects();
    return true;
}

bool CameraEffectComponent::Detaching(const IAttach::Ptr& target)
{
    return true;
}

namespace Internal {

CORE_NS::IEcs::Ptr GetEcs(const IEcsObject::Ptr& ecso)
{
    if (ecso) {
        if (auto scene = ecso->GetScene()) {
            return scene->GetEcsContext().GetNativeEcs();
        }
    }
    return {};
}

std::pair<CORE3D_NS::IPostProcessEffectComponentManager*, CORE_NS::Entity> GetPostProcessEffectComponentManager(
    const IEcsObject::Ptr& ecso)
{
    if (ecso) {
        auto entity = ecso->GetEntity();
        if (auto ecs = GetEcs(ecso); ecs && CORE_NS::EntityUtil::IsValid(entity)) {
            return { static_cast<CORE3D_NS::IPostProcessEffectComponentManager*>(
                         ecs->GetComponentManager(CORE3D_NS::IPostProcessEffectComponentManager::UID)),
                entity };
        }
    }
    return {};
}
} // namespace Internal

CORE3D_NS::IPostProcessEffectComponentManager* CameraEffectComponent::CreateEffectComponent()
{
    if (auto pp = Internal::GetPostProcessEffectComponentManager(GetEcsObject()); pp.first) {
        if (!pp.first->HasComponent(pp.second)) {
            pp.first->Create(pp.second);
        }
        return pp.first;
    }
    return {};
}

void CameraEffectComponent::DestroyEffectComponent()
{
    if (auto pp = Internal::GetPostProcessEffectComponentManager(GetEcsObject()); pp.first) {
        if (pp.first->HasComponent(pp.second)) {
            pp.first->Destroy(pp.second);
        }
    }
}

BASE_NS::vector<RENDER_NS::IRenderPostProcess::Ptr> CameraEffectComponent::GetRenderEffects() const
{
    BASE_NS::vector<RENDER_NS::IRenderPostProcess::Ptr> effects;
    if (const auto p = GetEffectsOrNull()) {
        if (const auto size = p->GetSize()) {
            effects.reserve(size);
            for (auto&& effect : p->GetValue()) {
                effects.emplace_back(BASE_NS::move(effect->GetEffect()));
            }
        }
    }
    return effects;
}

void CameraEffectComponent::UpdateEffects()
{
    // This function always overrides the ECS state to match the state of Effects array of this object
    if (auto scene = GetInternalScene()) {
        auto rf = GetRenderEffects();
        bool hasEffects = scene->RunDirectlyOrInTask([this, effects = BASE_NS::move(rf), scene]() {
            // Add/remove component based on whether we have effects or not
            const bool hasEffects = !effects.empty();
            if (hasEffects) {
                // Has effects, add component + set effects
                if (auto* ppm = CreateEffectComponent()) {
                    if (auto ef = ppm->Write(GetEntity())) {
                        ef->effects = effects;
                    }
                }
            } else {
                // No effects, remove effect component
                DestroyEffectComponent();
            }
            return hasEffects; // Return true if there were any effects
        });
        // Update our SERIALIZE flag based on whether there are any effects defined. If the effect list is empty, no
        // need to serialize this component either
        META_NS::SetObjectFlags(GetSelf(), META_NS::ObjectFlagBits::SERIALIZE, hasEffects);
    }
}

SCENE_END_NAMESPACE()
