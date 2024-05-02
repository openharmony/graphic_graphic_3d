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

#include <3d/ecs/components/post_process_component.h>
#include <render/datastore/render_data_store_render_pods.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using RENDER_NS::TonemapConfiguration;
using RENDER_NS::BloomConfiguration;
using RENDER_NS::DitherConfiguration;
using RENDER_NS::BlurConfiguration;
using RENDER_NS::ColorConversionConfiguration;
using RENDER_NS::FxaaConfiguration;
using RENDER_NS::VignetteConfiguration;
using RENDER_NS::ColorFringeConfiguration;
using RENDER_NS::TaaConfiguration;
using RENDER_NS::MotionBlurConfiguration;
using RENDER_NS::DofConfiguration;

using CORE3D_NS::PostProcessComponent;

DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandle);
DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandleReference);

BEGIN_ENUM(PostProcessFlagBitsMetaData, PostProcessComponent::FlagBits)
DECL_ENUM(PostProcessComponent::FlagBits, TONEMAP_BIT, "Tonemap")
DECL_ENUM(PostProcessComponent::FlagBits, BLOOM_BIT, "Bloom")
DECL_ENUM(PostProcessComponent::FlagBits, VIGNETTE_BIT, "Vignette")
DECL_ENUM(PostProcessComponent::FlagBits, COLOR_FRINGE_BIT, "Color Fringe")
DECL_ENUM(PostProcessComponent::FlagBits, DITHER_BIT, "Dither")
DECL_ENUM(PostProcessComponent::FlagBits, BLUR_BIT, "Blur")
DECL_ENUM(PostProcessComponent::FlagBits, COLOR_CONVERSION_BIT, "Color Conversion")
DECL_ENUM(PostProcessComponent::FlagBits, FXAA_BIT, "Fast Approximate Anti-Aliasing")
DECL_ENUM(PostProcessComponent::FlagBits, TAA_BIT, "Temporat Anti-Aliasing")
DECL_ENUM(PostProcessComponent::FlagBits, DOF_BIT, "Depth of Field")
DECL_ENUM(PostProcessComponent::FlagBits, MOTION_BLUR_BIT, "Motion Blur")
END_ENUM(PostProcessFlagBitsMetaData, PostProcessComponent::FlagBits)

/** Extend propertysystem with the enums */
DECLARE_PROPERTY_TYPE(TonemapConfiguration::TonemapType);
DECLARE_PROPERTY_TYPE(BloomConfiguration::BloomQualityType);
DECLARE_PROPERTY_TYPE(BloomConfiguration::BloomType);
DECLARE_PROPERTY_TYPE(DitherConfiguration::DitherType);
DECLARE_PROPERTY_TYPE(BlurConfiguration::BlurQualityType);
DECLARE_PROPERTY_TYPE(BlurConfiguration::BlurType);
DECLARE_PROPERTY_TYPE(ColorConversionConfiguration::ConversionFunctionType);
DECLARE_PROPERTY_TYPE(FxaaConfiguration::Sharpness);
DECLARE_PROPERTY_TYPE(FxaaConfiguration::Quality);
DECLARE_PROPERTY_TYPE(TaaConfiguration::Sharpness);
DECLARE_PROPERTY_TYPE(TaaConfiguration::Quality);
DECLARE_PROPERTY_TYPE(MotionBlurConfiguration::Sharpness);
DECLARE_PROPERTY_TYPE(MotionBlurConfiguration::Quality);

/** Extend propertysystem with the types */
DECLARE_PROPERTY_TYPE(TonemapConfiguration);
DECLARE_PROPERTY_TYPE(BloomConfiguration);
DECLARE_PROPERTY_TYPE(VignetteConfiguration);
DECLARE_PROPERTY_TYPE(ColorFringeConfiguration);
DECLARE_PROPERTY_TYPE(DitherConfiguration);
DECLARE_PROPERTY_TYPE(BlurConfiguration);
DECLARE_PROPERTY_TYPE(ColorConversionConfiguration);
DECLARE_PROPERTY_TYPE(FxaaConfiguration);
DECLARE_PROPERTY_TYPE(TaaConfiguration);
DECLARE_PROPERTY_TYPE(DofConfiguration);
DECLARE_PROPERTY_TYPE(MotionBlurConfiguration);

// Declare their metadata
BEGIN_ENUM(TonemapConfigurationTonemapTypeMetaData, TonemapConfiguration::TonemapType)
DECL_ENUM(TonemapConfiguration::TonemapType, TONEMAP_ACES, "aces")
DECL_ENUM(TonemapConfiguration::TonemapType, TONEMAP_ACES_2020, "aces_2020")
DECL_ENUM(TonemapConfiguration::TonemapType, TONEMAP_FILMIC, "filmic")
END_ENUM(TonemapConfigurationTonemapTypeMetaData, TonemapConfiguration::TonemapType)

BEGIN_ENUM(BloomConfigurationQualityTypeMetaData, BloomConfiguration::BloomQualityType)
DECL_ENUM(BloomConfiguration::BloomQualityType, QUALITY_TYPE_LOW, "low")
DECL_ENUM(BloomConfiguration::BloomQualityType, QUALITY_TYPE_NORMAL, "normal")
DECL_ENUM(BloomConfiguration::BloomQualityType, QUALITY_TYPE_HIGH, "high")
END_ENUM(BloomConfigurationQualityTypeMetaData, BloomConfiguration::BloomQualityType)

BEGIN_ENUM(BloomConfigurationBloomTypeMetaData, BloomConfiguration::BloomType)
DECL_ENUM(BloomConfiguration::BloomType, TYPE_NORMAL, "normal")
DECL_ENUM(BloomConfiguration::BloomType, TYPE_HORIZONTAL, "horizontal")
DECL_ENUM(BloomConfiguration::BloomType, TYPE_VERTICAL, "vertical")
DECL_ENUM(BloomConfiguration::BloomType, TYPE_BILATERAL, "bilateral")
END_ENUM(BloomConfigurationBloomTypeMetaData, BloomConfiguration::BloomType)

BEGIN_ENUM(DitherConfigurationDitherTypeMetaData, DitherConfiguration::DitherType)
DECL_ENUM(DitherConfiguration::DitherType, INTERLEAVED_NOISE, "interleaved_noise")
DECL_ENUM(DitherConfiguration::DitherType, TRIANGLE_NOISE, "triangle_noise")
DECL_ENUM(DitherConfiguration::DitherType, TRIANGLE_NOISE_RGB, "triangle_noise_rgb")
END_ENUM(DitherConfigurationDitherTypeMetaData, DitherConfiguration::DitherType)

BEGIN_ENUM(BlurConfigurationBlurQualityTypeMetaData, BlurConfiguration::BlurQualityType)
DECL_ENUM(BlurConfiguration::BlurQualityType, QUALITY_TYPE_LOW, "low")
DECL_ENUM(BlurConfiguration::BlurQualityType, QUALITY_TYPE_NORMAL, "normal")
DECL_ENUM(BlurConfiguration::BlurQualityType, QUALITY_TYPE_HIGH, "high")
END_ENUM(BlurConfigurationBlurQualityTypeMetaData, BlurConfiguration::BlurQualityType)

BEGIN_ENUM(BlurConfigurationBlurTypeMetaData, BlurConfiguration::BlurType)
DECL_ENUM(BlurConfiguration::BlurType, TYPE_NORMAL, "low")
DECL_ENUM(BlurConfiguration::BlurType, TYPE_HORIZONTAL, "normal")
DECL_ENUM(BlurConfiguration::BlurType, TYPE_VERTICAL, "high")
END_ENUM(BlurConfigurationBlurTypeMetaData, BlurConfiguration::BlurType)

BEGIN_ENUM(
    ColorConversionConfigurationConversionFunctionTypeMetaData, ColorConversionConfiguration::ConversionFunctionType)
DECL_ENUM(ColorConversionConfiguration::ConversionFunctionType, CONVERSION_LINEAR, "linear")
DECL_ENUM(ColorConversionConfiguration::ConversionFunctionType, CONVERSION_LINEAR_TO_SRGB, "linear_to_srgb")
END_ENUM(
    ColorConversionConfigurationConversionFunctionTypeMetaData, ColorConversionConfiguration::ConversionFunctionType)

BEGIN_ENUM(FxaaConfigurationSharpnessMetaData, FxaaConfiguration::Sharpness)
DECL_ENUM(FxaaConfiguration::Sharpness, SOFT, "soft")
DECL_ENUM(FxaaConfiguration::Sharpness, MEDIUM, "medium")
DECL_ENUM(FxaaConfiguration::Sharpness, SHARP, "sharp")
END_ENUM(FxaaConfigurationSharpnessMetaData, FxaaConfiguration::Sharpness)

BEGIN_ENUM(FxaaConfigurationQualityMetaData, FxaaConfiguration::Quality)
DECL_ENUM(FxaaConfiguration::Quality, LOW, "low")
DECL_ENUM(FxaaConfiguration::Quality, MEDIUM, "medium")
DECL_ENUM(FxaaConfiguration::Quality, HIGH, "high")
END_ENUM(FxaaConfigurationQualityMetaData, FxaaConfiguration::Quality)

BEGIN_ENUM(TaaConfigurationSharpnessMetaData, TaaConfiguration::Sharpness)
DECL_ENUM(TaaConfiguration::Sharpness, SOFT, "soft")
DECL_ENUM(TaaConfiguration::Sharpness, MEDIUM, "medium")
DECL_ENUM(TaaConfiguration::Sharpness, SHARP, "sharp")
END_ENUM(TaaConfigurationSharpnessMetaData, TaaConfiguration::Sharpness)

BEGIN_ENUM(TaaConfigurationQualityMetaData, TaaConfiguration::Quality)
DECL_ENUM(TaaConfiguration::Quality, LOW, "low")
DECL_ENUM(TaaConfiguration::Quality, MEDIUM, "medium")
DECL_ENUM(TaaConfiguration::Quality, HIGH, "high")
END_ENUM(TaaConfigurationQualityMetaData, TaaConfiguration::Quality)

BEGIN_METADATA(TonemapConfigurationMetaData, TonemapConfiguration)
DECL_PROPERTY2(TonemapConfiguration, tonemapType, "", 0)
DECL_PROPERTY2(TonemapConfiguration, exposure, "", 0)
END_METADATA(TonemapConfigurationMetaData, TonemapConfiguration)

BEGIN_METADATA(BloomConfigurationMetaData, BloomConfiguration)
DECL_PROPERTY2(BloomConfiguration, bloomType, "", 0)
DECL_PROPERTY2(BloomConfiguration, bloomQualityType, "", 0)
DECL_PROPERTY2(BloomConfiguration, thresholdHard, "", 0)
DECL_PROPERTY2(BloomConfiguration, thresholdSoft, "", 0)
DECL_PROPERTY2(BloomConfiguration, amountCoefficient, "", 0)
DECL_PROPERTY2(BloomConfiguration, dirtMaskCoefficient, "", 0)
DECL_PROPERTY2(BloomConfiguration, dirtMaskImage, "", 0)
DECL_PROPERTY2(BloomConfiguration, useCompute, "", 0)
END_METADATA(BloomConfigurationMetaData, BloomConfiguration)

BEGIN_METADATA(VignetteConfigurationMetaData, VignetteConfiguration)
DECL_PROPERTY2(VignetteConfiguration, coefficient, "", 0)
DECL_PROPERTY2(VignetteConfiguration, power, "", 0)
END_METADATA(VignetteConfigurationMetaData, VignetteConfiguration)

BEGIN_METADATA(ColorFringeConfigurationMetaData, ColorFringeConfiguration)
DECL_PROPERTY2(ColorFringeConfiguration, coefficient, "", 0)
DECL_PROPERTY2(ColorFringeConfiguration, distanceCoefficient, "", 0)
END_METADATA(ColorFringeConfigurationMetaData, ColorFringeConfiguration)

BEGIN_METADATA(DitherConfigurationMetaData, DitherConfiguration)
DECL_PROPERTY2(DitherConfiguration, ditherType, "", 0)
DECL_PROPERTY2(DitherConfiguration, amountCoefficient, "", 0)
END_METADATA(DitherConfigurationMetaData, DitherConfiguration)

BEGIN_METADATA(BlurConfigurationMetaData, BlurConfiguration)
DECL_PROPERTY2(BlurConfiguration, blurType, "", 0)
DECL_PROPERTY2(BlurConfiguration, blurQualityType, "", 0)
DECL_PROPERTY2(BlurConfiguration, filterSize, "", 0)
DECL_PROPERTY2(BlurConfiguration, maxMipLevel, "", 0)
END_METADATA(BlurConfigurationMetaData, BlurConfiguration)

BEGIN_METADATA(ColorConversionConfigurationMetaData, ColorConversionConfiguration)
DECL_PROPERTY2(ColorConversionConfiguration, conversionFunctionType, "", 0)
END_METADATA(ColorConversionConfigurationMetaData, ColorConversionConfiguration)

BEGIN_METADATA(FxaaConfigurationMetaData, FxaaConfiguration)
DECL_PROPERTY2(FxaaConfiguration, sharpness, "", 0)
DECL_PROPERTY2(FxaaConfiguration, quality, "", 0)
END_METADATA(FxaaConfigurationMetaData, FxaaConfiguration)

BEGIN_METADATA(TaaConfigurationMetaData, TaaConfiguration)
DECL_PROPERTY2(TaaConfiguration, sharpness, "", 0)
DECL_PROPERTY2(TaaConfiguration, quality, "", 0)
END_METADATA(TaaConfigurationMetaData, TaaConfiguration)

BEGIN_METADATA(DofConfigurationMetaData, DofConfiguration)
DECL_PROPERTY2(DofConfiguration, focusPoint, "", 0)
DECL_PROPERTY2(DofConfiguration, focusRange, "", 0)
DECL_PROPERTY2(DofConfiguration, nearTransitionRange, "", 0)
DECL_PROPERTY2(DofConfiguration, farTransitionRange, "", 0)
DECL_PROPERTY2(DofConfiguration, nearBlur, "", 0)
DECL_PROPERTY2(DofConfiguration, farBlur, "", 0)
DECL_PROPERTY2(DofConfiguration, nearPlane, "", 0)
DECL_PROPERTY2(DofConfiguration, farPlane, "", 0)
END_METADATA(DofConfigurationMetaData, DofConfiguration)

BEGIN_ENUM(MotionBlurConfigurationSharpnessMetaData, MotionBlurConfiguration::Sharpness)
DECL_ENUM(MotionBlurConfiguration::Sharpness, SOFT, "soft")
DECL_ENUM(MotionBlurConfiguration::Sharpness, MEDIUM, "medium")
DECL_ENUM(MotionBlurConfiguration::Sharpness, SHARP, "sharp")
END_ENUM(MotionBlurConfigurationSharpnessMetaData, MotionBlurConfiguration::Sharpness)

BEGIN_ENUM(MotionBlurConfigurationQualityMetaData, MotionBlurConfiguration::Quality)
DECL_ENUM(MotionBlurConfiguration::Quality, LOW, "low")
DECL_ENUM(MotionBlurConfiguration::Quality, MEDIUM, "medium")
DECL_ENUM(MotionBlurConfiguration::Quality, HIGH, "high")
END_ENUM(MotionBlurConfigurationQualityMetaData, MotionBlurConfiguration::Quality)

BEGIN_METADATA(MotionBlurConfigurationMetaData, MotionBlurConfiguration)
DECL_PROPERTY2(MotionBlurConfiguration, alpha, "", 0)
DECL_PROPERTY2(MotionBlurConfiguration, velocityCoefficient, "", 0)
DECL_PROPERTY2(MotionBlurConfiguration, sharpness, "", 0)
DECL_PROPERTY2(MotionBlurConfiguration, quality, "", 0)
END_METADATA(MotionBlurConfigurationMetaData, MotionBlurConfiguration)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;

class PostProcessComponentManager final : public BaseManager<PostProcessComponent, IPostProcessComponentManager> {
    BEGIN_PROPERTY(PostProcessComponent, ComponentMetadata)
#include <3d/ecs/components/post_process_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit PostProcessComponentManager(IEcs& ecs)
        : BaseManager<PostProcessComponent, IPostProcessComponentManager>(ecs, CORE_NS::GetName<PostProcessComponent>())
    {}

    ~PostProcessComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* IPostProcessComponentManagerInstance(IEcs& ecs)
{
    return new PostProcessComponentManager(ecs);
}

void IPostProcessComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<PostProcessComponentManager*>(instance);
}

CORE3D_END_NAMESPACE()
