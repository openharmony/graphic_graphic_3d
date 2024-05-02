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

#ifndef SCENEPLUGINAPI_POSTPROCESS_H
#define SCENEPLUGINAPI_POSTPROCESS_H

#include <scene_plugin/api/postprocess_uid.h>
#include <scene_plugin/interface/intf_camera.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

class Bloom final : public META_NS::Internal::ObjectInterfaceAPI<Bloom, ClassId::Bloom> {
    META_API(Bloom)
    META_API_OBJECT_CONVERTIBLE(IBloom)
    META_API_CACHE_INTERFACE(IBloom, Bloom)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, Type, IBloom::BloomType)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, Quality, QualityType)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, ThresholdHard, float)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, ThresholdSoft, float)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, AmountCoefficient, float)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, DirtMaskCoefficient, float)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, DirtMaskImage, IBitmap::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(Bloom, UseCompute, bool)
};

class Blur final : public META_NS::Internal::ObjectInterfaceAPI<Blur, ClassId::Blur> {
    META_API(Blur)
    META_API_OBJECT_CONVERTIBLE(IBlur)
    META_API_CACHE_INTERFACE(IBlur, Blur)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Blur, Type, IBlur::BlurType)
    META_API_INTERFACE_PROPERTY_CACHED(Blur, Quality, QualityType)
    META_API_INTERFACE_PROPERTY_CACHED(Blur, FilterSize, float)
    META_API_INTERFACE_PROPERTY_CACHED(Blur, MaxMipmapLevel, uint32_t)
};

class ColorConversion final : public META_NS::Internal::ObjectInterfaceAPI<ColorConversion, ClassId::ColorConversion> {
    META_API(ColorConversion)
    META_API_OBJECT_CONVERTIBLE(IColorConversion)
    META_API_CACHE_INTERFACE(IColorConversion, ColorConversion)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(ColorConversion, Function, IColorConversion::ConversionFunctionType)
};

class ColorFringe final : public META_NS::Internal::ObjectInterfaceAPI<ColorFringe, ClassId::ColorFringe> {
    META_API(ColorFringe)
    META_API_OBJECT_CONVERTIBLE(IColorFringe)
    META_API_CACHE_INTERFACE(IColorFringe, ColorFringe)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(ColorFringe, Coefficient, float)
    META_API_INTERFACE_PROPERTY_CACHED(ColorFringe, DistanceCoefficient, float)
};

class DepthOfField final : public META_NS::Internal::ObjectInterfaceAPI<DepthOfField, ClassId::DepthOfField> {
    META_API(DepthOfField)
    META_API_OBJECT_CONVERTIBLE(IDepthOfField)
    META_API_CACHE_INTERFACE(IDepthOfField, DepthOfField)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, FocusPoint, float)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, FocusRange, float)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, NearTransitionRange, float)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, FarTransitionRange, float)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, NearBlur, float)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, FarBlur, float)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, NearPlane, float)
    META_API_INTERFACE_PROPERTY_CACHED(DepthOfField, FarPlane, float)
};

class Dither final : public META_NS::Internal::ObjectInterfaceAPI<Dither, ClassId::Dither> {
    META_API(Dither)
    META_API_OBJECT_CONVERTIBLE(IDither)
    META_API_CACHE_INTERFACE(IDither, Dither)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Dither, Type, IDither::DitherType)
    META_API_INTERFACE_PROPERTY_CACHED(Dither, Coefficient, float)
};

class Fxaa final : public META_NS::Internal::ObjectInterfaceAPI<Fxaa, ClassId::Fxaa> {
    META_API(Fxaa)
    META_API_OBJECT_CONVERTIBLE(IFxaa)
    META_API_CACHE_INTERFACE(IFxaa, Fxaa)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Fxaa, Sharpness, SharpnessType)
    META_API_INTERFACE_PROPERTY_CACHED(Fxaa, Quality, QualityType)
};

class MotionBlur final : public META_NS::Internal::ObjectInterfaceAPI<MotionBlur, ClassId::MotionBlur> {
    META_API(MotionBlur)
    META_API_OBJECT_CONVERTIBLE(IMotionBlur)
    META_API_CACHE_INTERFACE(IMotionBlur, MotionBlur)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(MotionBlur, Alpha, float)
    META_API_INTERFACE_PROPERTY_CACHED(MotionBlur, Velocity, float)
    META_API_INTERFACE_PROPERTY_CACHED(MotionBlur, Quality, QualityType)
    META_API_INTERFACE_PROPERTY_CACHED(MotionBlur, Sharpness, SharpnessType)
};

class Taa final : public META_NS::Internal::ObjectInterfaceAPI<Taa, ClassId::Taa> {
    META_API(Taa)
    META_API_OBJECT_CONVERTIBLE(ITaa)
    META_API_CACHE_INTERFACE(ITaa, Taa)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Taa, Sharpness, SharpnessType)
    META_API_INTERFACE_PROPERTY_CACHED(Taa, Quality, QualityType)
};

class Tonemap final : public META_NS::Internal::ObjectInterfaceAPI<Tonemap, ClassId::Tonemap> {
    META_API(Tonemap)
    META_API_OBJECT_CONVERTIBLE(ITonemap)
    META_API_CACHE_INTERFACE(ITonemap, Tonemap)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Tonemap, Type, ITonemap::TonemapType)
    META_API_INTERFACE_PROPERTY_CACHED(Tonemap, Exposure, float)
};

class Vignette final : public META_NS::Internal::ObjectInterfaceAPI<Vignette, ClassId::Vignette> {
    META_API(Vignette)
    META_API_OBJECT_CONVERTIBLE(IVignette)
    META_API_CACHE_INTERFACE(IVignette, Vignette)
    META_API_CACHE_INTERFACE(IPostProcessEffect, PostprocessEffect)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostprocessEffect, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Vignette, Coefficient, float)
    META_API_INTERFACE_PROPERTY_CACHED(Vignette, Power, float)
};

#define MAP_PROPERTY_TO_USER(interface_type, user_name)                               \
public:                                                                               \
    SCENE_NS::user_name user_name()                                                   \
    {                                                                                 \
        SCENE_NS::user_name ret;                                                      \
        ret.Initialize(interface_pointer_cast<META_NS::IObject>(                      \
            META_NS::GetValue(this->Get##interface_type##Interface()->user_name()))); \
        return ret;                                                                   \
    }

class PostProcess final : public META_NS::Internal::ObjectInterfaceAPI<PostProcess, ClassId::PostProcess> {
    META_API(PostProcess)
    META_API_OBJECT_CONVERTIBLE(IPostProcess)
    META_API_CACHE_INTERFACE(IPostProcess, PostProcess)

public:
    META_API_INTERFACE_PROPERTY_CACHED(PostProcess, Name, BASE_NS::string)
    MAP_PROPERTY_TO_USER(PostProcess, Bloom)
    MAP_PROPERTY_TO_USER(PostProcess, Blur)
    MAP_PROPERTY_TO_USER(PostProcess, ColorConversion)
    MAP_PROPERTY_TO_USER(PostProcess, ColorFringe)
    MAP_PROPERTY_TO_USER(PostProcess, DepthOfField)
    MAP_PROPERTY_TO_USER(PostProcess, Dither)
    MAP_PROPERTY_TO_USER(PostProcess, Fxaa)
    MAP_PROPERTY_TO_USER(PostProcess, MotionBlur)
    MAP_PROPERTY_TO_USER(PostProcess, Taa)
    MAP_PROPERTY_TO_USER(PostProcess, Tonemap)
    MAP_PROPERTY_TO_USER(PostProcess, Vignette)
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_POSTPROCESS_H
