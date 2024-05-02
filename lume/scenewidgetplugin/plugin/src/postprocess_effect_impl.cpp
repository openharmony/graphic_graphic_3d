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

#include <scene_plugin/api/postprocess_uid.h>
#include <scene_plugin/interface/intf_bitmap.h>

#include <render/datastore/render_data_store_render_pods.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_postprocess_private.h"
#include "node_impl.h"

#include "task_utils.h"

using SCENE_NS::MakeTask;
using SCENE_NS::IBitmap;

namespace {
static constexpr BASE_NS::string_view PP_COMPONENT_NAME = "PostProcessComponent";

static constexpr size_t PP_COMPONENT_NAME_LEN = PP_COMPONENT_NAME.size() + 1;
static constexpr BASE_NS::string_view PP_TONEMAP_TYPE =
    "PostProcessComponent.tonemapConfiguration.tonemapType"; // enum 32 TONEMAP_ACES
static constexpr BASE_NS::string_view PP_TONEMAP_EXPOSURE =
    "PostProcessComponent.tonemapConfiguration.exposure"; // float 0.7f
static constexpr BASE_NS::string_view PP_BLOOM_TYPE =
    "PostProcessComponent.bloomConfiguration.bloomType"; // enum 32 TYPE_NORMAL
static constexpr BASE_NS::string_view PP_BLOOM_QUALITY =
    "PostProcessComponent.bloomConfiguration.bloomQualityType"; // enum 32 QUALITY_TYPE_NORMAL
static constexpr BASE_NS::string_view PP_BLOOM_THH =
    "PostProcessComponent.bloomConfiguration.thresholdHard"; // float 1.f
static constexpr BASE_NS::string_view PP_BLOOM_THS =
    "PostProcessComponent.bloomConfiguration.thresholdSoft"; // float 2.f
static constexpr BASE_NS::string_view PP_BLOOM_AC =
    "PostProcessComponent.bloomConfiguration.amountCoefficient"; // float 0.25f
static constexpr BASE_NS::string_view PP_BLOOM_DMC =
    "PostProcessComponent.bloomConfiguration.dirtMaskCoefficient"; // float 0.f
static constexpr BASE_NS::string_view PP_BLOOM_DMI =
    "PostProcessComponent.bloomConfiguration.dirtMaskImage"; // RenderHandle <- need to be added
                                                             // "Render::RenderHandle"
static constexpr BASE_NS::string_view PP_BLOOM_UC = "PostProcessComponent.bloomConfiguration.useCompute";  // bool false
static constexpr BASE_NS::string_view PP_VIG_C = "PostProcessComponent.vignetteConfiguration.coefficient"; // float 0.5f
static constexpr BASE_NS::string_view PP_VIG_P = "PostProcessComponent.vignetteConfiguration.power";       // float 0.4f
static constexpr BASE_NS::string_view PP_CF_C =
    "PostProcessComponent.colorFringeConfiguration.coefficient"; // float 1.0f
static constexpr BASE_NS::string_view PP_CF_DC =
    "PostProcessComponent.colorFringeConfiguration.distanceCoefficient"; // float 2.0f
static constexpr BASE_NS::string_view PP_DC_TYPE =
    "PostProcessComponent.ditherConfiguration.ditherType"; // enum 32 INTERLEAVED_NOICE
static constexpr BASE_NS::string_view PP_DC_AC =
    "PostProcessComponent.ditherConfiguration.amountCoefficient"; // float 0.1f
static constexpr BASE_NS::string_view PP_BLUR_TYPE =
    "PostProcessComponent.blurConfiguration.blurType"; // enum 32  NORMAL
static constexpr BASE_NS::string_view PP_BLUR_QT =
    "PostProcessComponent.blurConfiguration.blurQualityType"; // enum 32 QUALITY_TYPE_NORMAL
static constexpr BASE_NS::string_view PP_BLUR_FS = "PostProcessComponent.blurConfiguration.filterSize";   // float  1.f
static constexpr BASE_NS::string_view PP_BLUR_MML = "PostProcessComponent.blurConfiguration.maxMipLevel"; // uint32 0u
static constexpr BASE_NS::string_view PP_CC_TYPE =
    "PostProcessComponent.colorConversionConfiguration.conversionFunctionType"; // enum 32 LINEAR
static constexpr BASE_NS::string_view PP_FXAA_SHARPNESS =
    "PostProcessComponent.fxaaConfiguration.sharpness"; // enum 32 SHARP
static constexpr BASE_NS::string_view PP_FXAA_QUALITY =
    "PostProcessComponent.fxaaConfiguration.quality"; // enum 32 MEDIUM
static constexpr BASE_NS::string_view PP_TAA_SHARPNESS =
    "PostProcessComponent.taaConfiguration.sharpness"; // enum 32 SHARP
static constexpr BASE_NS::string_view PP_TAA_QUALITY =
    "PostProcessComponent.taaConfiguration.quality"; // enum 32 MEDIUM
static constexpr BASE_NS::string_view PP_DOF_FOCUSPOINT =
    "PostProcessComponent.dofConfiguration.focusPoint"; // float 3.f
static constexpr BASE_NS::string_view PP_DOF_FOCUSRANGE =
    "PostProcessComponent.dofConfiguration.focusRange"; // float 1.f
static constexpr BASE_NS::string_view PP_DOF_NEAR_TR =
    "PostProcessComponent.dofConfiguration.nearTransitionRange"; // float 1.f
static constexpr BASE_NS::string_view PP_DOF_FAR_TR =
    "PostProcessComponent.dofConfiguration.farTransitionRange";                                            // float 1.f
static constexpr BASE_NS::string_view PP_DOF_NEAR_BLUR = "PostProcessComponent.dofConfiguration.nearBlur"; // float 2.f
static constexpr BASE_NS::string_view PP_DOF_FAR_BLUR = "PostProcessComponent.dofConfiguration.farBlur";   // float 2.f
static constexpr BASE_NS::string_view PP_DOF_NEAR_PLANE =
    "PostProcessComponent.dofConfiguration.nearPlane"; // float 0.1f
static constexpr BASE_NS::string_view PP_DOF_FAR_PLANE =
    "PostProcessComponent.dofConfiguration.farPlane";                                                     // float 1000.
static constexpr BASE_NS::string_view PP_MB_ALPHA = "PostProcessComponent.motionBlurConfiguration.alpha"; // float 1.f
static constexpr BASE_NS::string_view PP_MB_VELOCITY =
    "PostProcessComponent.motionBlurConfiguration.velocityCoefficient"; // float 1.f
static constexpr BASE_NS::string_view PP_MB_QUALITY =
    "PostProcessComponent.motionBlurConfiguration.quality"; // enum 32 MEDIUM
static constexpr BASE_NS::string_view PP_MB_SHARPNESS =
    "PostProcessComponent.motionBlurConfiguration.sharpness"; // enum 32 SHARP


class BloomImpl : public META_NS::ObjectFwd<BloomImpl, SCENE_NS::ClassId::Bloom, META_NS::ClassId::Object,
                      SCENE_NS::IBloom, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IBloom, SCENE_NS::IBloom::BloomType, Type, SCENE_NS::IBloom::BloomType::NORMAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBloom, SCENE_NS::QualityType, Quality, SCENE_NS::QualityType::NORMAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBloom, float, ThresholdHard, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBloom, float, ThresholdSoft, 2.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBloom, float, AmountCoefficient, 0.25f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBloom, float, DirtMaskCoefficient, 0.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBloom, IBitmap::Ptr, DirtMaskImage, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBloom, bool, UseCompute, false)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetSceneHolder(sh);
        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            ConvertBindChanges<SCENE_NS::IBloom::BloomType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Type), meta, PP_BLOOM_TYPE);
            ConvertBindChanges<SCENE_NS::QualityType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Quality), meta, PP_BLOOM_QUALITY);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(ThresholdHard), meta, PP_BLOOM_THH);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(ThresholdSoft), meta, PP_BLOOM_THS);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(AmountCoefficient), meta, PP_BLOOM_AC);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(DirtMaskCoefficient), meta, PP_BLOOM_DMC);

            if (auto renderHandle = meta->GetPropertyByName(PP_BLOOM_DMI)) {
                // Bind image handle with other means
                DirtMaskImage()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([sh, this]() {
                    sh->QueueEngineTask(
                        MakeTask(
                            [target = ecsObject_->GetEntity(), image = META_NS::GetValue(DirtMaskImage())](auto sh) {
                                CORE_NS::Entity imageEntity = sh->BindUIBitmap(image, true);
                                sh->SetRenderHandle(target, imageEntity);
                                return false;
                            },
                            sh),
                        false);
                }),
                    reinterpret_cast<uint64_t>(this));

                renderHandle->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([sh, this]() {
                    // best effort
                    if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
                        if (auto renderHandle = meta->GetPropertyByName<uint64_t>(PP_BLOOM_DMI)) {
                            auto handle = META_NS::GetValue(renderHandle);
                            BASE_NS::string uri;
                            RENDER_NS::RenderHandle rh { handle };
                            if (sh && sh->GetRenderHandleUri(rh, uri)) {
                                if (auto image = META_NS::GetValue(DirtMaskImage())) {
                                    if (auto uribitmap = interface_cast<IBitmap>(image)) {
                                        META_NS::SetValue(uribitmap->Uri(), uri);
                                    }
                                } else {
                                    auto uribitmap = META_NS::GetObjectRegistry().Create<IBitmap>(
                                        SCENE_NS::ClassId::Bitmap);
                                    META_NS::SetValue(uribitmap->Uri(), uri);
                                }
                            }
                        }
                    }
                }),
                    reinterpret_cast<uint64_t>(this));
            }

            BindChanges<bool>(propHandler_, META_ACCESS_PROPERTY(UseCompute), meta, PP_BLOOM_UC);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class BlurImpl : public META_NS::ObjectFwd<BlurImpl, SCENE_NS::ClassId::Blur, META_NS::ClassId::Object, SCENE_NS::IBlur,
                     IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IBlur, SCENE_NS::IBlur::BlurType, Type, SCENE_NS::IBlur::BlurType::NORMAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBlur, SCENE_NS::QualityType, Quality, SCENE_NS::QualityType::NORMAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBlur, float, FilterSize, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBlur, uint32_t, MaxMipmapLevel, 0u)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            ConvertBindChanges<SCENE_NS::IBlur::BlurType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Type), meta, PP_BLUR_TYPE);
            ConvertBindChanges<SCENE_NS::QualityType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Quality), meta, PP_BLUR_QT);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FilterSize), meta, PP_BLUR_FS);
            BindChanges<uint32_t>(propHandler_, META_ACCESS_PROPERTY(MaxMipmapLevel), meta, PP_BLUR_MML);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class ColorConversionImpl
    : public META_NS::ObjectFwd<ColorConversionImpl, SCENE_NS::ClassId::ColorConversion, META_NS::ClassId::Object,
          SCENE_NS::IColorConversion, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IColorConversion, SCENE_NS::IColorConversion::ConversionFunctionType,
        Function, SCENE_NS::IColorConversion::ConversionFunctionType::LINEAR)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            ConvertBindChanges<SCENE_NS::IColorConversion::ConversionFunctionType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Function), meta, PP_CC_TYPE);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class ColorFringeImpl
    : public META_NS::ObjectFwd<ColorFringeImpl, SCENE_NS::ClassId::ColorFringe, META_NS::ClassId::Object,
          SCENE_NS::IColorFringe, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IColorFringe, float, Coefficient, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IColorFringe, float, DistanceCoefficient, 2.f)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Coefficient), meta, PP_CF_C);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(DistanceCoefficient), meta, PP_CF_DC);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class DepthOfFieldImpl
    : public META_NS::ObjectFwd<DepthOfFieldImpl, SCENE_NS::ClassId::DepthOfField, META_NS::ClassId::Object,
          SCENE_NS::IDepthOfField, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, FocusPoint, 3.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, FocusRange, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, NearTransitionRange, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, FarTransitionRange, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, NearBlur, 2.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, FarBlur, 2.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, NearPlane, 0.1f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDepthOfField, float, FarPlane, 1000.f)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FocusPoint), meta, PP_DOF_FOCUSPOINT);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FocusRange), meta, PP_DOF_FOCUSRANGE);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(NearTransitionRange), meta, PP_DOF_NEAR_TR);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FarTransitionRange), meta, PP_DOF_FAR_TR);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(NearBlur), meta, PP_DOF_NEAR_BLUR);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FarBlur), meta, PP_DOF_FAR_BLUR);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(NearPlane), meta, PP_DOF_NEAR_PLANE);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FarPlane), meta, PP_DOF_FAR_PLANE);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class DitherImpl : public META_NS::ObjectFwd<DitherImpl, SCENE_NS::ClassId::Dither, META_NS::ClassId::Object,
                       SCENE_NS::IDither, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IDither, SCENE_NS::IDither::DitherType, Type, SCENE_NS::IDither::DitherType::INTERLEAVED_NOISE)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IDither, float, Coefficient, 0.1f)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            ConvertBindChanges<SCENE_NS::IDither::DitherType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Type), meta, PP_DC_TYPE);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Coefficient), meta, PP_DC_AC);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class FxaaImpl : public META_NS::ObjectFwd<FxaaImpl, SCENE_NS::ClassId::Fxaa, META_NS::ClassId::Object, SCENE_NS::IFxaa,
                     IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IFxaa, SCENE_NS::SharpnessType, Sharpness, SCENE_NS::SharpnessType::SHARP)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IFxaa, SCENE_NS::QualityType, Quality, SCENE_NS::QualityType::NORMAL)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            ConvertBindChanges<SCENE_NS::SharpnessType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Sharpness), meta, PP_FXAA_SHARPNESS);
            ConvertBindChanges<SCENE_NS::QualityType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Quality), meta, PP_FXAA_QUALITY);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class MotionBlurImpl
    : public META_NS::ObjectFwd<MotionBlurImpl, SCENE_NS::ClassId::MotionBlur, META_NS::ClassId::Object,
          SCENE_NS::IMotionBlur, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMotionBlur, float, Alpha, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMotionBlur, float, Velocity, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IMotionBlur, SCENE_NS::QualityType, Quality, SCENE_NS::QualityType::NORMAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IMotionBlur, SCENE_NS::SharpnessType, Sharpness, SCENE_NS::SharpnessType::MEDIUM)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Alpha), meta, PP_MB_ALPHA);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Velocity), meta, PP_MB_VELOCITY);
            ConvertBindChanges<SCENE_NS::QualityType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Quality), meta, PP_MB_QUALITY);
            ConvertBindChanges<SCENE_NS::SharpnessType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Sharpness), meta, PP_MB_SHARPNESS);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class TaaImpl : public META_NS::ObjectFwd<TaaImpl, SCENE_NS::ClassId::Taa, META_NS::ClassId::Object, SCENE_NS::ITaa,
                    IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ITaa, SCENE_NS::SharpnessType, Sharpness, SCENE_NS::SharpnessType::SHARP)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ITaa, SCENE_NS::QualityType, Quality, SCENE_NS::QualityType::NORMAL)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            ConvertBindChanges<SCENE_NS::SharpnessType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Sharpness), meta, PP_TAA_SHARPNESS);
            ConvertBindChanges<SCENE_NS::QualityType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Quality), meta, PP_TAA_QUALITY);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class TonemapImpl : public META_NS::ObjectFwd<TonemapImpl, SCENE_NS::ClassId::Tonemap, META_NS::ClassId::Object,
                        SCENE_NS::ITonemap, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ITonemap, ITonemap::TonemapType, Type, ITonemap::TonemapType::ACES)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ITonemap, float, Exposure, 0.7f)

    ~TonemapImpl()
    {
        SCENE_PLUGIN_VERBOSE_LOG("%s", __func__);
    }

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetSceneHolder(sh);
        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            ConvertBindChanges<ITonemap::TonemapType, uint32_t>(
                propHandler_, META_ACCESS_PROPERTY(Type), meta, PP_TONEMAP_TYPE);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Exposure), meta, PP_TONEMAP_EXPOSURE);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

class VignetteImpl : public META_NS::ObjectFwd<VignetteImpl, SCENE_NS::ClassId::Vignette, META_NS::ClassId::Object,
                         SCENE_NS::IVignette, IPostProcessEffectPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcessEffect, bool, Enabled)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IVignette, float, Coefficient, 0.5f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IVignette, float, Power, 0.4f)

    void Bind(SCENE_NS::IEcsObject::Ptr ecsObject, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        ecsObject_ = ecsObject;
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Coefficient), meta, PP_VIG_C);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Power), meta, PP_VIG_P);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    PropertyHandlerArrayHolder propHandler_ {};
};

} // namespace

SCENE_BEGIN_NAMESPACE()

void RegisterPostprocessEffectImpl()
{
    /* META_NS::RegisterPropertyType<SCENE_NS::IColorConversion::ConversionFunctionType>();
    RegisterUIntSerialization<SCENE_NS::IColorConversion::ConversionFunctionType>();

    META_NS::RegisterPropertyType<SCENE_NS::ITonemap::TonemapType>();
    RegisterUIntSerialization<SCENE_NS::ITonemap::TonemapType>();

    META_NS::RegisterPropertyType<SCENE_NS::QualityType>();
    RegisterUIntSerialization<SCENE_NS::QualityType>();

    META_NS::RegisterPropertyType<SCENE_NS::SharpnessType>();
    RegisterUIntSerialization<SCENE_NS::SharpnessType>();

    META_NS::RegisterPropertyType<SCENE_NS::IBloom::BloomType>();
    RegisterUIntSerialization<SCENE_NS::IBloom::BloomType>();

    META_NS::RegisterPropertyType<SCENE_NS::IBlur::BlurType>();
    RegisterUIntSerialization<SCENE_NS::IBlur::BlurType>();

    META_NS::RegisterPropertyType<SCENE_NS::IDither::DitherType>();
    RegisterUIntSerialization<SCENE_NS::IDither::DitherType>();
    */
    auto& registry = META_NS::GetObjectRegistry();
    registry.RegisterObjectType<BloomImpl>();
    registry.RegisterObjectType<BlurImpl>();
    registry.RegisterObjectType<ColorConversionImpl>();
    registry.RegisterObjectType<ColorFringeImpl>();
    registry.RegisterObjectType<DepthOfFieldImpl>();
    registry.RegisterObjectType<DitherImpl>();
    registry.RegisterObjectType<FxaaImpl>();
    registry.RegisterObjectType<MotionBlurImpl>();
    registry.RegisterObjectType<TaaImpl>();
    registry.RegisterObjectType<TonemapImpl>();
    registry.RegisterObjectType<VignetteImpl>();
}

void UnregisterPostprocessEffectImpl()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.UnregisterObjectType<BloomImpl>();
    registry.UnregisterObjectType<BlurImpl>();
    registry.UnregisterObjectType<ColorConversionImpl>();
    registry.UnregisterObjectType<ColorFringeImpl>();
    registry.UnregisterObjectType<DepthOfFieldImpl>();
    registry.UnregisterObjectType<DitherImpl>();
    registry.UnregisterObjectType<FxaaImpl>();
    registry.UnregisterObjectType<MotionBlurImpl>();
    registry.UnregisterObjectType<TaaImpl>();
    registry.UnregisterObjectType<TonemapImpl>();
    registry.UnregisterObjectType<VignetteImpl>();
}

SCENE_END_NAMESPACE()
