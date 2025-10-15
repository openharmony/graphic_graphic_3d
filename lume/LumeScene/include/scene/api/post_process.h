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
class Tonemap : public META_NS::InterfaceObject<ITonemap> {
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
class Bloom : public META_NS::InterfaceObject<IBloom> {
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
 * @brief The ColorFringe class wraps a post process effect which implements IColorFringe.
 */
class ColorFringe : public PostProcessEffect {
public:
    META_INTERFACE_OBJECT(ColorFringe, PostProcessEffect, IColorFringe)
    /// @see IColorFringe::Coefficient
    META_INTERFACE_OBJECT_PROPERTY(float, Coefficient)
    /// @see IColorFringe::DistanceCoefficient
    META_INTERFACE_OBJECT_PROPERTY(float, DistanceCoefficient)
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
 * @brief The PostProcess class wraps a post process setup object which implements IPostProcess.
 */
class PostProcess : META_NS::InterfaceObject<IPostProcess> {
public:
    META_INTERFACE_OBJECT(PostProcess, META_NS::InterfaceObject<IPostProcess>, IPostProcess)
    META_INTERFACE_OBJECT_INSTANTIATE(PostProcess, ClassId::PostProcess)
    /// @see IPostProcess::Tonemap
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Tonemap, Tonemap)
    /// @see IPostProcess::Bloom
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Bloom, Bloom)
    /// @see IPostProcess::ColorFringe
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::ColorFringe, ColorFringe)
    /// @see IPostProcess::Vignette
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Vignette, Vignette)
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_POST_PROCESS_H
