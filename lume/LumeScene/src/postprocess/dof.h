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

#ifndef SCENE_SRC_POSTPROCESS_DOF_H
#define SCENE_SRC_POSTPROCESS_DOF_H

#include <scene/interface/postprocess/intf_dof.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(DepthOfField, "4165f797-a08b-4c81-af0b-14642eb1ed9c", META_NS::ObjectCategoryBits::NO_CATEGORY)

class DepthOfField : public Internal::PostProcessEffect<IDepthOfField, CORE3D_NS::PostProcessComponent::DOF_BIT> {
    META_OBJECT(DepthOfField, ClassId::DepthOfField, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, FocusPoint, "focusPoint")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, FocusRange, "focusRange")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, NearTransitionRange, "nearTransitionRange")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, FarTransitionRange, "farTransitionRange")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, NearBlur, "nearBlur")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, FarBlur, "farBlur")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, NearPlane, "nearPlane")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IDepthOfField, float, FarPlane, "farPlane")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(float, FocusPoint)
    META_IMPLEMENT_PROPERTY(float, FocusRange)
    META_IMPLEMENT_PROPERTY(float, NearTransitionRange)
    META_IMPLEMENT_PROPERTY(float, FarTransitionRange)
    META_IMPLEMENT_PROPERTY(float, NearBlur)
    META_IMPLEMENT_PROPERTY(float, FarBlur)
    META_IMPLEMENT_PROPERTY(float, NearPlane)
    META_IMPLEMENT_PROPERTY(float, FarPlane)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_DOF_H
