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
#ifndef SCENEPLUGIN_INTF_POSTPROCESS_H
#define SCENEPLUGIN_INTF_POSTPROCESS_H

#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/intf_bitmap.h>

#include <base/containers/vector.h>

#include <meta/api/animation/animation.h>
#include <meta/base/types.h>
#include <meta/interface/intf_container.h>

SCENE_BEGIN_NAMESPACE()

enum class SharpnessType : uint32_t {
    /** Produces soft effect */
    SOFT = 0,
    /** Produces normal effect */
    MEDIUM = 1,
    /** Produces sharp effect */
    SHARP = 2
};

enum QualityType : uint32_t {
    /** Low quality */
    LOW = 0,
    /** Normal quality */
    NORMAL = 1,
    /** High quality */
    HIGH = 2,
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::SharpnessType);
META_TYPE(SCENE_NS::QualityType);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IPostProcessEffect, "6655ac7d-9c42-48fb-ac54-5bff1f049077")
class IPostProcessEffect : public META_NS::INamed {
    META_INTERFACE(META_NS::INamed, IPostProcessEffect, InterfaceId::IPostProcessEffect)
public:
    /**
     * @brief Sets whether the effect is enabled or disabled.
     * @return True if effect is enabled, otherwise false.
     */
    META_PROPERTY(bool, Enabled);
};

REGISTER_INTERFACE(ITonemap, "6942c567-3cbd-47c7-8380-c516f529f6ef")
class ITonemap : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, ITonemap, InterfaceId::ITonemap)
public:
    enum TonemapType : uint32_t {
        /** Aces */
        ACES = 0,
        /** Aces 2020 */
        ACES_2020 = 1,
        /** Filmic */
        FILMIC = 2,
    };

    /**
     * @brief Camera postprocessing settings, tonemap type
     * @return
     */
    META_PROPERTY(TonemapType, Type)

    /**
     * @brief Camera postprocessing settings, tonemap exposure
     * @return
     */
    META_PROPERTY(float, Exposure)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ITonemap::TonemapType)
META_TYPE(SCENE_NS::ITonemap::WeakPtr);
META_TYPE(SCENE_NS::ITonemap::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IBloom, "77f38aa6-300c-496f-9401-97b483be5d16")
class IBloom : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IBloom, InterfaceId::IBloom)
public:

    /** Bloom type enum */
    enum BloomType : uint32_t {
        /** Normal, smooth to every direction */
        NORMAL = 0,
        /** Blurred/Blooms more in horizontal direction */
        HORIZONTAL = 1,
        /** Blurred/Blooms more in vertical direction */
        VERTICAL = 2,
        /** Bilateral filter, uses depth if available */
        BILATERAL = 3,
    };

    /**
     * @brief Camera postprocessing settings, bloom type
     * @return
     */
    META_PROPERTY(BloomType, Type)
    /**
     * @brief Camera postprocessing settings, bloom quality type
     * @return
     */
    META_PROPERTY(QualityType, Quality)
    /**
     * @brief Camera postprocessing settings, bloom threshold hard
     * @return
     */
    META_PROPERTY(float, ThresholdHard)
    /**
     * @brief Camera postprocessing settings, bloom threshold soft
     * @return
     */
    META_PROPERTY(float, ThresholdSoft)
    /**
     * @brief Camera postprocessing settings, bloom amount coefficient
     * @return
     */
    META_PROPERTY(float, AmountCoefficient)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, DirtMaskCoefficient)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(IBitmap::Ptr, DirtMaskImage)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(bool, UseCompute)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IBloom::BloomType)
META_TYPE(SCENE_NS::IBloom::WeakPtr);
META_TYPE(SCENE_NS::IBloom::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IVignette, "bf3a09bd-cc53-48c7-8e5d-0a6e7bf327fd")
class IVignette : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IVignette, InterfaceId::IVignette)
public:
    /**
     * @brief Camera postprocessing settings, vignette coefficient
     * @return
     */
    META_PROPERTY(float, Coefficient)
    /**
     * @brief Camera postprocessing settings, vignette power
     * @return
     */
    META_PROPERTY(float, Power)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IVignette::WeakPtr);
META_TYPE(SCENE_NS::IVignette::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IColorFringe, "8b95f17a-6c31-4fc5-b63e-f0034c1ecc7e")
class IColorFringe : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IColorFringe, InterfaceId::IColorFringe)
public:
    /**
     * @brief Camera postprocessing settings, ColorFringe Coefficient
     * @return
     */
    META_PROPERTY(float, Coefficient)
    /**
     * @brief Camera postprocessing settings, ColorFringe DistanceCoefficient
     * @return
     */
    META_PROPERTY(float, DistanceCoefficient)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IColorFringe::WeakPtr);
META_TYPE(SCENE_NS::IColorFringe::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IDither, "93a5ff50-ab54-49f8-9f80-623918f7ec0a")
class IDither : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IDither, InterfaceId::IDither)
public:
    /** Dither type enum */
    enum DitherType : uint32_t {
        /** Interleaved noise */
        INTERLEAVED_NOISE = 0,
        /** Interleaved noise */
        TRIANGLE_NOISE = 0,
        /** Interleaved noise */
        TRIANGLE_NOISE_RGB = 0,
    };

    /**
     * @brief Camera postprocessing settings, Dither type
     * @return
     */
    META_PROPERTY(DitherType, Type)
    /**
     * @brief Camera postprocessing settings, Dither coefficient
     * @return
     */
    META_PROPERTY(float, Coefficient)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IDither::DitherType);
META_TYPE(SCENE_NS::IDither::WeakPtr);
META_TYPE(SCENE_NS::IDither::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IBlur, "282c0b68-ce16-4e66-9ec1-02fd935a4c4d")
class IBlur : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IBlur, InterfaceId::IBlur)
public:
    /** Blur type enum */
    enum BlurType : uint32_t {
        /** Normal, smooth to every direction */
        NORMAL = 0,
        /** Blurred more in horizontal direction */
        HORIZONTAL = 1,
        /** Blurred more in vertical direction */
        VERTICAL = 2,
    };

    /**
     * @brief Camera postprocessing settings, Blur type
     * @return
     */
    META_PROPERTY(BlurType, Type)
    /**
     * @brief Camera postprocessing settings, Blur quality type
     * @return
     */
    META_PROPERTY(QualityType, Quality)
    /**
     * @brief Camera postprocessing settings, Blur fliter size
     * @return
     */
    META_PROPERTY(float, FilterSize)
    /**
     * @brief Camera postprocessing settings, Blur maximum mipmap level
     * @return
     */
    META_PROPERTY(uint32_t, MaxMipmapLevel)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IBlur::BlurType);
META_TYPE(SCENE_NS::IBlur::WeakPtr);
META_TYPE(SCENE_NS::IBlur::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IColorConversion, "ef36e89e-2379-4297-9cac-10943f1c605c")
class IColorConversion : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IColorConversion, InterfaceId::IColorConversion)
public:
    /** Tonemap type */
    enum ConversionFunctionType : uint32_t {
        /** Linear -> no conversion in normal situation. */
        LINEAR = 0,
        /** Linear to sRGB conversion */
        LINEAR_TO_SRGB = 1,
    };

    /**
     * @brief Camera postprocessing settings, Blur color conversion function
     * @return
     */
    META_PROPERTY(ConversionFunctionType, Function)
};

SCENE_END_NAMESPACE()
META_TYPE(SCENE_NS::IColorConversion::ConversionFunctionType);
META_TYPE(SCENE_NS::IColorConversion::WeakPtr);
META_TYPE(SCENE_NS::IColorConversion::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IFxaa, "9a270f3f-f3d6-4bb2-b005-9ee33c74a726")
class IFxaa : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IFxaa, InterfaceId::IFxaa)
public:
    /**
     * @brief Camera postprocessing settings, FXAA Quality type
     * @return
     */
    META_PROPERTY(QualityType, Quality)
    /**
     * @brief Camera postprocessing settings, FXAA Sharpness type
     * @return
     */
    META_PROPERTY(SharpnessType, Sharpness)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IFxaa::WeakPtr);
META_TYPE(SCENE_NS::IFxaa::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(ITaa, "5265155f-d679-4478-923c-1af3503a1d2a")
class ITaa : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, ITaa, InterfaceId::ITaa)
public:
    /**
     * @brief Camera postprocessing settings, FXAA Quality type
     * @return
     */
    META_PROPERTY(QualityType, Quality)
    /**
     * @brief Camera postprocessing settings, FXAA Sharpness type
     * @return
     */
    META_PROPERTY(SharpnessType, Sharpness)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ITaa::WeakPtr);
META_TYPE(SCENE_NS::ITaa::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IDepthOfField, "c578779f-7f70-466b-8a61-92d90cb284e5")
class IDepthOfField : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IDepthOfField, InterfaceId::IDepthOfField)
public:
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, FocusPoint)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, FocusRange)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, NearTransitionRange)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, FarTransitionRange)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, NearBlur)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, FarBlur)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, NearPlane)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, FarPlane)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IDepthOfField::WeakPtr);
META_TYPE(SCENE_NS::IDepthOfField::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IMotionBlur, "6fbeb47d-6a33-47d5-945e-9bc8088b321c")
class IMotionBlur : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IMotionBlur, InterfaceId::IMotionBlur)
public:
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, Alpha)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, Velocity)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(QualityType, Quality)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(SharpnessType, Sharpness)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IMotionBlur::WeakPtr);
META_TYPE(SCENE_NS::IMotionBlur::Ptr);

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::PostProcess
REGISTER_INTERFACE(IPostProcess, "e3f3a72e-dca8-40f3-8cae-feba8dc235ba")
class IPostProcess : public META_NS::INamed {
    META_INTERFACE(META_NS::INamed, IPostProcess, InterfaceId::IPostProcess)
public:
    /**
     * @brief Camera postprocessing settings, tonemap
     * @return
     */
    META_PROPERTY(ITonemap::Ptr, Tonemap)
    /**
     * @brief Camera postprocessing settings, bloom
     * @return
     */
    META_PROPERTY(IBloom::Ptr, Bloom)
    /**
     * @brief Camera postprocessing settings, vignette
     * @return
     */
    META_PROPERTY(IVignette::Ptr, Vignette)
    /**
     * @brief Camera postprocessing settings, color fringe
     * @return
     */
    META_PROPERTY(IColorFringe::Ptr, ColorFringe)
    /**
     * @brief Camera postprocessing settings, dither
     * @return
     */
    META_PROPERTY(IDither::Ptr, Dither)
    /**
     * @brief Camera postprocessing settings, blur
     * @return
     */
    META_PROPERTY(IBlur::Ptr, Blur)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(IColorConversion::Ptr, ColorConversion)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(IFxaa::Ptr, Fxaa)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(ITaa::Ptr, Taa)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(IDepthOfField::Ptr, DepthOfField)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(IMotionBlur::Ptr, MotionBlur)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IPostProcess::WeakPtr);
META_TYPE(SCENE_NS::IPostProcess::Ptr);

#endif // SCENEPLUGIN_INTF_POSTPROCESS_H
