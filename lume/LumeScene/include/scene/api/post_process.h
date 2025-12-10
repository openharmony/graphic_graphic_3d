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
#ifndef SCENE_API_POST_PROCESS_H
#define SCENE_API_POST_PROCESS_H

#include <scene/api/resource.h>
#include <scene/interface/intf_postprocess.h>

#include <meta/api/interface_object.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief THe PostProcessEffect class is the base class for all post process effects.
 */
class PostProcessEffect : public META_NS::InterfaceObject<IPostProcessEffect> {
public:
    META_INTERFACE_OBJECT(PostProcessEffect, META_NS::InterfaceObject<IPostProcessEffect>, IPostProcessEffect)
    /// @see IPostProcessEffect::Enabled
    META_INTERFACE_OBJECT_PROPERTY(bool, Enabled)
};

/**
 * @brief The Bloom class wraps a post process effect which implements ITonemap.
 */
class Tonemap : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(Tonemap, PostProcessEffect, ITonemap)
    /// @see ITonemap::Type
    META_INTERFACE_OBJECT_PROPERTY(TonemapType, Type)
    /// @see ITonemap::Exposure
    META_INTERFACE_OBJECT_PROPERTY(float, Exposure)
};

/**
 * @brief The Bloom class wraps a post process effect which implements IBloom.
 */
class Bloom : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(Bloom, PostProcessEffect, IBloom)
    /// @see IBloom::Type
    META_INTERFACE_OBJECT_PROPERTY(BloomType, Type)
    /// @see IBloom::Quality
    META_INTERFACE_OBJECT_PROPERTY(EffectQualityType, Quality)
    /// @see IBloom::ThresholdHard
    META_INTERFACE_OBJECT_PROPERTY(float, ThresholdHard)
    /// @see IBloom::ThresholdSoft
    META_INTERFACE_OBJECT_PROPERTY(float, ThresholdSoft)
    /// @see IBloom::AmountCoefficient
    META_INTERFACE_OBJECT_PROPERTY(float, AmountCoefficient)
    /// @see IBloom::DirtMaskCoefficient
    META_INTERFACE_OBJECT_PROPERTY(float, DirtMaskCoefficient)
    /// @see IBloom::DirtMaskImage
    META_INTERFACE_OBJECT_PROPERTY(Image, DirtMaskImage)
    /// @see IBloom::UseCompute
    META_INTERFACE_OBJECT_PROPERTY(bool, UseCompute)
    /// @see IBloom::Scatter
    META_INTERFACE_OBJECT_PROPERTY(float, Scatter)
    /// @see IBloom::ScaleFactor
    META_INTERFACE_OBJECT_PROPERTY(float, ScaleFactor)
};

/**
 * @brief The Blur class wraps a post process effect which implements IBlur.
 */
class Blur : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(Blur, PostProcessEffect, IBlur)
    /// @see IBlur::Type
    META_INTERFACE_OBJECT_PROPERTY(BlurType, Type)
    /// @see IBlur::Quality
    META_INTERFACE_OBJECT_PROPERTY(EffectQualityType, Quality)
    /// @see IBlur::FilterSize
    META_INTERFACE_OBJECT_PROPERTY(float, FilterSize)
    /// @see IBlur::MaxMipmapLevel
    META_INTERFACE_OBJECT_PROPERTY(uint32_t, MaxMipmapLevel)
};

/**
 * @brief The MotionBlur class wraps a post process effect which implements IMotionBlur.
 */
class MotionBlur : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(MotionBlur, PostProcessEffect, IMotionBlur)
    /// @see IMotionBlur::Quality
    META_INTERFACE_OBJECT_PROPERTY(EffectQualityType, Quality)
    /// @see IMotionBlur::Sharpness
    META_INTERFACE_OBJECT_PROPERTY(EffectSharpnessType, Sharpness)
    /// @see IMotionBlur::Alpha
    META_INTERFACE_OBJECT_PROPERTY(float, Alpha)
    /// @see IMotionBlur::VelocityCoefficient
    META_INTERFACE_OBJECT_PROPERTY(float, VelocityCoefficient)
};

/**
 * @brief The ColorConversion class wraps a post process effect which implements IColorConversion.
 */
class ColorConversion : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(ColorConversion, PostProcessEffect, IColorConversion)
    /// @see IColorConversion::Function
    META_INTERFACE_OBJECT_PROPERTY(ColorConversionFunctionType, Function)
    /// @see IColorConversion::MultiplyWithAlpha
    META_INTERFACE_OBJECT_PROPERTY(bool, MultiplyWithAlpha)
};

/**
 * @brief The ColorFringe class wraps a post process effect which implements IColorFringe.
 */
class ColorFringe : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(ColorFringe, PostProcessEffect, IColorFringe)
    /// @see IColorFringe::DistanceCoefficient
    META_INTERFACE_OBJECT_PROPERTY(float, DistanceCoefficient)
};

/**
 * @brief The DepthOfField class wraps a post process effect which implements IDepthOfField.
 */
class DepthOfField : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(DepthOfField, PostProcessEffect, IDepthOfField)
    /// @see IDepthOfField::FocusPoint
    META_INTERFACE_OBJECT_PROPERTY(float, FocusPoint)
    /// @see IDepthOfField::FocusRange
    META_INTERFACE_OBJECT_PROPERTY(float, FocusRange)
    /// @see IDepthOfField::NearTransitionRange
    META_INTERFACE_OBJECT_PROPERTY(float, NearTransitionRange)
    /// @see IDepthOfField::FarTransitionRange
    META_INTERFACE_OBJECT_PROPERTY(float, FarTransitionRange)
    /// @see IDepthOfField::NearBlur
    META_INTERFACE_OBJECT_PROPERTY(float, NearBlur)
    /// @see IDepthOfField::FarBlur
    META_INTERFACE_OBJECT_PROPERTY(float, FarBlur)
    /// @see IDepthOfField::NearPlane
    META_INTERFACE_OBJECT_PROPERTY(float, NearPlane)
    /// @see IDepthOfField::FarPlane
    META_INTERFACE_OBJECT_PROPERTY(float, FarPlane)
};

/**
 * @brief The Fxaa class wraps a post process effect which implements IFxaa.
 */
class Fxaa : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(Fxaa, PostProcessEffect, IFxaa)
    /// @see IFxaa::Quality
    META_INTERFACE_OBJECT_PROPERTY(EffectQualityType, Quality)
    /// @see IFxaa::Sharpness
    META_INTERFACE_OBJECT_PROPERTY(EffectSharpnessType, Sharpness)
};

/**
 * @brief The Taa class wraps a post process effect which implements ITaa.
 */
class Taa : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(Taa, PostProcessEffect, ITaa)
    /// @see ITaa::Quality
    META_INTERFACE_OBJECT_PROPERTY(EffectQualityType, Quality)
    /// @see ITaa::Sharpness
    META_INTERFACE_OBJECT_PROPERTY(EffectSharpnessType, Sharpness)
};

/**
 * @brief The Vignette class wraps a post process effect which implements IVignette.
 */
class Vignette : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(Vignette, PostProcessEffect, IVignette)
    /// @see IVignette::Coefficient
    META_INTERFACE_OBJECT_PROPERTY(float, Coefficient)
    /// @see IVignette::Power
    META_INTERFACE_OBJECT_PROPERTY(float, Power)
};

/**
 * @brief The lens flare class wraps a post process effect which implements ILensFlare.
 */
class LensFlare : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(LensFlare, PostProcessEffect, ILensFlare)
    /// @see ILensFlare::Quality
    META_INTERFACE_OBJECT_PROPERTY(EffectQualityType, Quality)
    /// @see ILensFlare::Intensity
    META_INTERFACE_OBJECT_PROPERTY(float, Intensity)
    /// @see ILensFlare::FlarePosition
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec3, FlarePosition)
};

/**
 * @brief The Upscale class wraps a post process effect which implements Upscale.
 */
class Upscale : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(Upscale, PostProcessEffect, IUpscale)
    /// @see IUpscale::SmoothScale
    META_INTERFACE_OBJECT_PROPERTY(float, SmoothScale)
    /// @see IUpscale::StructureSensitivity
    META_INTERFACE_OBJECT_PROPERTY(float, StructureSensitivity)
    /// @see IUpscale::EdgeSharpness
    META_INTERFACE_OBJECT_PROPERTY(float, EdgeSharpness)
    /// @see IUpscale::Ratio
    META_INTERFACE_OBJECT_PROPERTY(float, Ratio)
};

/**
 * @brief The PostProcess class wraps a post process setup object which implements IPostProcess.
 */
class PostProcess : public META_NS::InterfaceObject<IPostProcess> {
public:
    META_INTERFACE_OBJECT(PostProcess, META_NS::InterfaceObject<IPostProcess>, IPostProcess)
    META_INTERFACE_OBJECT_INSTANTIATE(PostProcess, ClassId::PostProcess)
    /// @see IPostProcess::Tonemap
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Tonemap, Tonemap)
    /// @see IPostProcess::Bloom
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Bloom, Bloom)
    /// @see IPostProcess::Blur
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Blur, Blur)
    /// @see IPostProcess::MotionBlur
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::MotionBlur, MotionBlur)
    /// @see IPostProcess::ColorConversion
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::ColorConversion, ColorConversion)
    /// @see IPostProcess::ColorFringe
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::ColorFringe, ColorFringe)
    /// @see IPostProcess::DepthOfField
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::DepthOfField, DepthOfField)
    /// @see IPostProcess::Fxaa
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Fxaa, Fxaa)
    /// @see IPostProcess::Taa
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Taa, Taa)
    /// @see IPostProcess::Vignette
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Vignette, Vignette)
    /// @see IPostProcess::LensFlare
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::LensFlare, LensFlare)
    /// @see IPostProcess::LensFlare
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Upscale, Upscale)
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_POST_PROCESS_H
