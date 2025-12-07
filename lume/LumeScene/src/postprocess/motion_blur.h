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

#ifndef SCENE_SRC_POSTPROCESS_MOTION_BLUR_H
#define SCENE_SRC_POSTPROCESS_MOTION_BLUR_H

#include <scene/interface/postprocess/intf_motion_blur.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(MotionBlur, "c2a00f4e-690c-4a76-9a32-92a3d9b76de2", META_NS::ObjectCategoryBits::NO_CATEGORY)

class MotionBlur : public Internal::PostProcessEffect<IMotionBlur, CORE3D_NS::PostProcessComponent::MOTION_BLUR_BIT> {
    META_OBJECT(MotionBlur, ClassId::MotionBlur, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMotionBlur, EffectQualityType, Quality, "quality")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMotionBlur, EffectSharpnessType, Sharpness, "sharpness")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMotionBlur, float, Alpha, "alpha")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMotionBlur, float, VelocityCoefficient, "velocityCoefficient")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(EffectQualityType, Quality)
    META_IMPLEMENT_PROPERTY(EffectSharpnessType, Sharpness)
    META_IMPLEMENT_PROPERTY(float, Alpha)
    META_IMPLEMENT_PROPERTY(float, VelocityCoefficient)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_MOTION_BLUR_H
