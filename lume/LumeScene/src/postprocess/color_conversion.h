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

#ifndef SCENE_SRC_POSTPROCESS_COLOR_CONVERSION_H
#define SCENE_SRC_POSTPROCESS_COLOR_CONVERSION_H

#include <scene/interface/postprocess/intf_color_conversion.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(ColorConversion, "f532feeb-0e71-454b-a1ac-99a05933c506", META_NS::ObjectCategoryBits::NO_CATEGORY)

class ColorConversion
    : public Internal::PostProcessEffect<IColorConversion, CORE3D_NS::PostProcessComponent::COLOR_CONVERSION_BIT> {
    META_OBJECT(ColorConversion, ClassId::ColorConversion, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorConversion, ColorConversionFunctionType, Function, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IColorConversion, bool, MultiplyWithAlpha, "")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(ColorConversionFunctionType, Function)
    META_IMPLEMENT_PROPERTY(bool, MultiplyWithAlpha)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

private:
    META_NS::Property<uint32_t> GetFunctionProperty();
    META_NS::Property<uint32_t> function_;

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_COLOR_CONVERSION_H
