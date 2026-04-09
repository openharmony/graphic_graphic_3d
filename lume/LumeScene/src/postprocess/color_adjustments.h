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

#ifndef SCENE_SRC_POSTPROCESS_COLOR_ADJUSTMENTS_H
#define SCENE_SRC_POSTPROCESS_COLOR_ADJUSTMENTS_H

#include <scene/interface/postprocess/intf_color_adjustments.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(ColorAdjustments, "7b5d8e3f-9a4b-4c7d-8e3f-9a4b4c7d8e3f", META_NS::ObjectCategoryBits::NO_CATEGORY)

class ColorAdjustments
    : public Internal::PostProcessEffect<IColorAdjustments, CORE3D_NS::PostProcessComponent::COLOR_ADJUSTMENTS_BIT> {
    META_OBJECT(ColorAdjustments, ClassId::ColorAdjustments, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorAdjustments, BASE_NS::Math::Vec4, FilterColor, "filterColor")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorAdjustments, float, HueShift, "hueShift")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorAdjustments, float, Saturation, "saturation")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorAdjustments, float, Brightness, "brightness")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorAdjustments, float, Contrast, "contrast")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, FilterColor)
    META_IMPLEMENT_PROPERTY(float, HueShift)
    META_IMPLEMENT_PROPERTY(float, Saturation)
    META_IMPLEMENT_PROPERTY(float, Brightness)
    META_IMPLEMENT_PROPERTY(float, Contrast)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_COLOR_ADJUSTMENTS_H
