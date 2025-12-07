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

#ifndef SCENE_SRC_COMPONENT_CAMERA_EFFECT_COMPONENT_H
#define SCENE_SRC_COMPONENT_CAMERA_EFFECT_COMPONENT_H

#include <scene/ext/component.h>
#include <scene/ext/intf_internal_camera.h>
#include <scene/interface/intf_camera.h>

#include <3d/ecs/components/post_process_effect_component.h>

#include <meta/api/container/find_cache.h>
#include <meta/api/event_handler.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(
    CameraEffectComponent, "ea10012f-86fa-4f1e-9c46-4e0f514b89a3", META_NS::ObjectCategoryBits::NO_CATEGORY)

class CameraEffectComponent : public META_NS::IntroduceInterfaces<Component, ICameraEffect> {
    META_OBJECT(CameraEffectComponent, ClassId::CameraEffectComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_ARRAY_PROPERTY_DATA(ICameraEffect, IEffect::Ptr, Effects, "")
    META_END_STATIC_DATA()

    META_IMPLEMENT_ARRAY_PROPERTY(IEffect::Ptr, Effects)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    bool SetEcsObject(const IEcsObject::Ptr& obj) override;

public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    BASE_NS::string GetName() const override;

    bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override;
    bool Detaching(const IAttach::Ptr& target) override;

private:
    CORE3D_NS::IPostProcessEffectComponentManager* CreateEffectComponent();
    void DestroyEffectComponent();

    META_NS::ArrayProperty<const IEffect::Ptr> GetEffectsOrNull() const;
    BASE_NS::vector<RENDER_NS::IRenderPostProcess::Ptr> GetRenderEffects() const;
    void UpdateEffects();
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_COMPONENT_CAMERA_EFFECT_COMPONENT_H
