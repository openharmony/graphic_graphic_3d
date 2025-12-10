/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using RENDER_NS::BloomConfiguration;
using RENDER_NS::BlurConfiguration;
using RENDER_NS::ColorConversionConfiguration;
using RENDER_NS::ColorFringeConfiguration;
using RENDER_NS::DitherConfiguration;
using RENDER_NS::DofConfiguration;
using RENDER_NS::FxaaConfiguration;
using RENDER_NS::LensFlareConfiguration;
using RENDER_NS::MotionBlurConfiguration;
using RENDER_NS::TaaConfiguration;
using RENDER_NS::TonemapConfiguration;
using RENDER_NS::UpscaleConfiguration;
using RENDER_NS::VignetteConfiguration;

using CORE3D_NS::PostProcessComponent;

ENUM_TYPE_METADATA(PostProcessComponent::FlagBits, ENUM_VALUE(TONEMAP_BIT, "Tonemap"), ENUM_VALUE(BLOOM_BIT, "Bloom"),
    ENUM_VALUE(VIGNETTE_BIT, "Vignette"), ENUM_VALUE(COLOR_FRINGE_BIT, "Color Fringe"),
    ENUM_VALUE(UPSCALE_BIT, "Upscale"), ENUM_VALUE(DITHER_BIT, "Dither"), ENUM_VALUE(BLUR_BIT, "Blur"),
    ENUM_VALUE(COLOR_CONVERSION_BIT, "Color Conversion"), ENUM_VALUE(FXAA_BIT, "Fast Approximate Anti-Aliasing"),
    ENUM_VALUE(TAA_BIT, "Temporal Anti-Aliasing"), ENUM_VALUE(DOF_BIT, "Depth of Field"),
    ENUM_VALUE(MOTION_BLUR_BIT, "Motion Blur"), ENUM_VALUE(LENS_FLARE_BIT, "Lens Flare"))

/** Extend propertysystem with the enums */

// Declare their metadata
ENUM_TYPE_METADATA(RENDER_NS::TonemapConfiguration::TonemapType, ENUM_VALUE(TONEMAP_ACES, "Aces"),
    ENUM_VALUE(TONEMAP_ACES_2020, "Aces 2020"), ENUM_VALUE(TONEMAP_FILMIC, "Filmic"),
    ENUM_VALUE(TONEMAP_PBR_NEUTRAL, "Neutral"))

ENUM_TYPE_METADATA(RENDER_NS::BloomConfiguration::BloomQualityType, ENUM_VALUE(QUALITY_TYPE_LOW, "Low Quality"),
    ENUM_VALUE(QUALITY_TYPE_NORMAL, "Normal Quality"), ENUM_VALUE(QUALITY_TYPE_HIGH, "High Quality"))

ENUM_TYPE_METADATA(RENDER_NS::BloomConfiguration::BloomType, ENUM_VALUE(TYPE_NORMAL, "Normal Type"),
    ENUM_VALUE(TYPE_HORIZONTAL, "Horizontal Type"), ENUM_VALUE(TYPE_VERTICAL, "Vertical Type"),
    ENUM_VALUE(TYPE_BILATERAL, "Bilateral Type"))

ENUM_TYPE_METADATA(RENDER_NS::DitherConfiguration::DitherType, ENUM_VALUE(INTERLEAVED_NOISE, "Interleaved Noise"),
    ENUM_VALUE(TRIANGLE_NOISE, "Triangle Noise"), ENUM_VALUE(TRIANGLE_NOISE_RGB, "Triangle Noise RGB"))

ENUM_TYPE_METADATA(RENDER_NS::BlurConfiguration::BlurQualityType, ENUM_VALUE(QUALITY_TYPE_LOW, "Low Quality"),
    ENUM_VALUE(QUALITY_TYPE_NORMAL, "Normal Quality"), ENUM_VALUE(QUALITY_TYPE_HIGH, "High Quality"))

ENUM_TYPE_METADATA(RENDER_NS::BlurConfiguration::BlurType, ENUM_VALUE(TYPE_NORMAL, "Normal Type"),
    ENUM_VALUE(TYPE_HORIZONTAL, "Horizontal Type"), ENUM_VALUE(TYPE_VERTICAL, "Vertical Type"))

ENUM_TYPE_METADATA(RENDER_NS::ColorConversionConfiguration::ConversionFunctionType,
    ENUM_VALUE(CONVERSION_LINEAR, "Linear"), ENUM_VALUE(CONVERSION_LINEAR_TO_SRGB, "Linear to sRGB"),
    ENUM_VALUE(CONVERSION_MULTIPLY_WITH_ALPHA, "Multiply With Alpha"), )

ENUM_TYPE_METADATA(RENDER_NS::FxaaConfiguration::Sharpness, ENUM_VALUE(SOFT, "Soft"), ENUM_VALUE(MEDIUM, "Medium"),
    ENUM_VALUE(SHARP, "Sharp"))

ENUM_TYPE_METADATA(RENDER_NS::FxaaConfiguration::Quality, ENUM_VALUE(LOW, "Low"), ENUM_VALUE(MEDIUM, "Medium"),
    ENUM_VALUE(HIGH, "High"))

ENUM_TYPE_METADATA(RENDER_NS::TaaConfiguration::Sharpness, ENUM_VALUE(SOFT, "Soft"), ENUM_VALUE(MEDIUM, "Medium"),
    ENUM_VALUE(SHARP, "Sharp"))

ENUM_TYPE_METADATA(RENDER_NS::TaaConfiguration::Quality, ENUM_VALUE(LOW, "Low"), ENUM_VALUE(MEDIUM, "Medium"),
    ENUM_VALUE(HIGH, "High"))

DATA_TYPE_METADATA(RENDER_NS::TonemapConfiguration, MEMBER_PROPERTY(tonemapType, "Tonemap Type", 0),
    MEMBER_PROPERTY(exposure, "Exposure", 0))

DATA_TYPE_METADATA(RENDER_NS::BloomConfiguration, MEMBER_PROPERTY(bloomType, "Type", 0),
    MEMBER_PROPERTY(bloomQualityType, "Quality", 0), MEMBER_PROPERTY(thresholdHard, "Hard Threshold", 0),
    MEMBER_PROPERTY(thresholdSoft, "Soft Threshold", 0), MEMBER_PROPERTY(amountCoefficient, "Amount Coefficient", 0),
    MEMBER_PROPERTY(dirtMaskCoefficient, "Dist Mask Coefficient", 0), MEMBER_PROPERTY(scatter, "Scatter", 0),
    MEMBER_PROPERTY(scaleFactor, "Scale Factor", 0), MEMBER_PROPERTY(dirtMaskImage, "Dirt Mask Image", 0),
    MEMBER_PROPERTY(useCompute, "Use Compute", CORE_NS::PropertyFlags::IS_HIDDEN))

DATA_TYPE_METADATA(RENDER_NS::VignetteConfiguration, MEMBER_PROPERTY(coefficient, "Coefficient", 0),
    MEMBER_PROPERTY(power, "Power", 0))

DATA_TYPE_METADATA(RENDER_NS::ColorFringeConfiguration, MEMBER_PROPERTY(distanceCoefficient, "Distance Coefficient", 0))

DATA_TYPE_METADATA(RENDER_NS::DitherConfiguration, MEMBER_PROPERTY(ditherType, "Type", 0),
    MEMBER_PROPERTY(amountCoefficient, "Amount", 0))

DATA_TYPE_METADATA(RENDER_NS::BlurConfiguration, MEMBER_PROPERTY(blurType, "Type", 0),
    MEMBER_PROPERTY(blurQualityType, "Quality", 0), MEMBER_PROPERTY(filterSize, "Filter Size", 0),
    MEMBER_PROPERTY(maxMipLevel, "Max Mip Levels", 0))

DATA_TYPE_METADATA(RENDER_NS::ColorConversionConfiguration,
    BITFIELD_MEMBER_PROPERTY(conversionFunctionType, "Conversion Function", PropertyFlags::IS_BITFIELD,
        RENDER_NS::ColorConversionConfiguration::ConversionFunctionType))

DATA_TYPE_METADATA(
    RENDER_NS::FxaaConfiguration, MEMBER_PROPERTY(sharpness, "Sharpness", 0), MEMBER_PROPERTY(quality, "Quality", 0))

DATA_TYPE_METADATA(
    RENDER_NS::TaaConfiguration, MEMBER_PROPERTY(sharpness, "Sharpness", 0), MEMBER_PROPERTY(quality, "Quality", 0))

DATA_TYPE_METADATA(RENDER_NS::DofConfiguration, MEMBER_PROPERTY(focusPoint, "Focust Point", 0),
    MEMBER_PROPERTY(focusRange, "Focus Range", 0), MEMBER_PROPERTY(nearTransitionRange, "Near Transition Range", 0),
    MEMBER_PROPERTY(farTransitionRange, "Far Transition Range", 0), MEMBER_PROPERTY(nearBlur, "Near Blur", 0),
    MEMBER_PROPERTY(farBlur, "Far Blur", 0), MEMBER_PROPERTY(nearPlane, "Near Plane", 0),
    MEMBER_PROPERTY(farPlane, "Far Plane", 0))

ENUM_TYPE_METADATA(RENDER_NS::MotionBlurConfiguration::Sharpness, ENUM_VALUE(SOFT, "Soft"),
    ENUM_VALUE(MEDIUM, "Medium"), ENUM_VALUE(SHARP, "Sharp"))

ENUM_TYPE_METADATA(RENDER_NS::MotionBlurConfiguration::Quality, ENUM_VALUE(LOW, "Low"), ENUM_VALUE(MEDIUM, "Medium"),
    ENUM_VALUE(HIGH, "High"))

DATA_TYPE_METADATA(RENDER_NS::MotionBlurConfiguration, MEMBER_PROPERTY(alpha, "Alpha", 0),
    MEMBER_PROPERTY(velocityCoefficient, "Velocity Coefficient", 0), MEMBER_PROPERTY(sharpness, "Sharpness", 0),
    MEMBER_PROPERTY(quality, "Quality", 0))

ENUM_TYPE_METADATA(RENDER_NS::LensFlareConfiguration::Quality, ENUM_VALUE(LOW, "Low"), ENUM_VALUE(MEDIUM, "Medium"),
    ENUM_VALUE(HIGH, "High"))
DATA_TYPE_METADATA(RENDER_NS::LensFlareConfiguration, MEMBER_PROPERTY(quality, "Quality", 0),
    MEMBER_PROPERTY(intensity, "Intensity", 0), MEMBER_PROPERTY(flarePosition, "Flare Position", 0))

DATA_TYPE_METADATA(UpscaleConfiguration, MEMBER_PROPERTY(ratio, "Upscaling ratio", 0),
    MEMBER_PROPERTY(smoothScale, "Gradient smooth scale", 0),
    MEMBER_PROPERTY(structureSensitivity, "Structure sensitivity", 0),
    MEMBER_PROPERTY(edgeSharpness, "Edge sharpness", 0))
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
    BEGIN_PROPERTY(PostProcessComponent, componentMetaData_)
#include <3d/ecs/components/post_process_component.h>
    END_PROPERTY();

public:
    explicit PostProcessComponentManager(IEcs& ecs)
        : BaseManager<PostProcessComponent, IPostProcessComponentManager>(
              ecs, CORE_NS::GetName<PostProcessComponent>(), 0U)
    {}

    ~PostProcessComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetaData_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetaData_)) {
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
    static_cast<PostProcessComponentManager*>(instance)->~PostProcessComponentManager();
    ::operator delete(instance);
}

CORE3D_END_NAMESPACE()
