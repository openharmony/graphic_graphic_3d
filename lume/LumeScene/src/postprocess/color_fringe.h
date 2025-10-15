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

#ifndef SCENE_SRC_POSTPROCESS_COLOR_FRINGE_H
#define SCENE_SRC_POSTPROCESS_COLOR_FRINGE_H

#include <scene/interface/postprocess/intf_color_fringe.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(ColorFringe, "3e403293-8e0c-43bf-ae30-cb45d6ad2d65", META_NS::ObjectCategoryBits::NO_CATEGORY)

class ColorFringe
    : public Internal::PostProcessEffect<IColorFringe, CORE3D_NS::PostProcessComponent::COLOR_FRINGE_BIT> {
    META_OBJECT(ColorFringe, ClassId::ColorFringe, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorFringe, float, Coefficient, "coefficient")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorFringe, float, DistanceCoefficient, "distanceCoefficient")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(float, Coefficient)
    META_IMPLEMENT_PROPERTY(float, DistanceCoefficient)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_COLOR_FRINGE_H