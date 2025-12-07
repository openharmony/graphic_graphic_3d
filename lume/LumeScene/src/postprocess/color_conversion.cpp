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

#include "color_conversion.h"

#include <3d/ecs/components/post_process_component.h>

SCENE_BEGIN_NAMESPACE()

namespace {

struct ColorConversionFunctionConverter {
    ColorConversionFunctionConverter(META_NS::ConstProperty<uint32_t> fn) : fn_(fn) {}
    using SourceType = ColorConversionFunctionType;
    using TargetType = uint32_t;

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        return (v & RENDER_NS::ColorConversionConfiguration::ConversionFunctionType::CONVERSION_LINEAR_TO_SRGB)
                   ? ColorConversionFunctionType::LINEAR_TO_SRGB
                   : ColorConversionFunctionType::LINEAR;
    }

    TargetType ConvertToTarget(const SourceType& v) const
    {
        bool srgb = v & ColorConversionFunctionType::LINEAR_TO_SRGB;
        auto tv = META_NS::GetValue(fn_);
        return srgb ? tv | RENDER_NS::ColorConversionConfiguration::CONVERSION_LINEAR_TO_SRGB
                    : tv & ~RENDER_NS::ColorConversionConfiguration::CONVERSION_LINEAR_TO_SRGB;
    }

private:
    META_NS::ConstProperty<uint32_t> fn_;
};

struct MultiplyWithAlphaConverter {
    MultiplyWithAlphaConverter(META_NS::ConstProperty<uint32_t> fn) : fn_(fn) {}
    using SourceType = bool;
    using TargetType = uint32_t;

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        return v & RENDER_NS::ColorConversionConfiguration::CONVERSION_MULTIPLY_WITH_ALPHA;
    }

    TargetType ConvertToTarget(const SourceType& v) const
    {
        auto tv = META_NS::GetValue(fn_);
        return v ? tv | RENDER_NS::ColorConversionConfiguration::CONVERSION_MULTIPLY_WITH_ALPHA
                 : tv & ~RENDER_NS::ColorConversionConfiguration::CONVERSION_MULTIPLY_WITH_ALPHA;
    }

private:
    META_NS::ConstProperty<uint32_t> fn_;
};

} // namespace

META_NS::Property<uint32_t> ColorConversion::GetFunctionProperty()
{
    if (!function_) {
        if (auto ecso = GetEcsObject()) {
            function_ = ecso->CreateProperty(GetPropertyPath("conversionFunctionType")).GetResult();
        }
    }
    return function_;
}

bool ColorConversion::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (!p) {
        return false;
    }
    const auto name = p->GetName();
    if (name == "Function") {
        auto fn = GetFunctionProperty();
        return fn && PushPropertyValue(
                         p, META_NS::IValue::Ptr { new ConvertingValue<ColorConversionFunctionConverter>(fn, { fn }) });
    }
    if (name == "MultiplyWithAlpha") {
        auto fn = GetFunctionProperty();
        return fn && PushPropertyValue(
                         p, META_NS::IValue::Ptr { new ConvertingValue<MultiplyWithAlphaConverter>(fn, { fn }) });
    }
    return PostProcessEffect::InitDynamicProperty(p, path);
}

BASE_NS::string_view ColorConversion::GetComponentPath() const
{
    static constexpr BASE_NS::string_view p("PostProcessComponent.colorConversionConfiguration.");
    return p;
}

SCENE_END_NAMESPACE()
