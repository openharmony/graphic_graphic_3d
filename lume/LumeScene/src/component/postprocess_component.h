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

#ifndef SCENE_SRC_COMPONENT_POSTPROCESS_COMPONENT_H
#define SCENE_SRC_COMPONENT_POSTPROCESS_COMPONENT_H

#include <scene/ext/component.h>

#include <3d/ecs/components/post_process_component.h>

#include <meta/ext/object.h>

META_TYPE(RENDER_NS::TonemapConfiguration)
META_TYPE(RENDER_NS::BloomConfiguration)
META_TYPE(RENDER_NS::VignetteConfiguration)
META_TYPE(RENDER_NS::ColorFringeConfiguration)
META_TYPE(RENDER_NS::DitherConfiguration)
META_TYPE(RENDER_NS::BlurConfiguration)
META_TYPE(RENDER_NS::ColorConversionConfiguration)
META_TYPE(RENDER_NS::FxaaConfiguration)
META_TYPE(RENDER_NS::TaaConfiguration)
META_TYPE(RENDER_NS::DofConfiguration)
META_TYPE(RENDER_NS::MotionBlurConfiguration)

SCENE_BEGIN_NAMESPACE()

class IInternalPostProcess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalPostProcess, "f9057b24-fe35-495f-a535-c5c2ebac02c1")
public:
    META_PROPERTY(uint32_t, EnableFlags)
    META_PROPERTY(RENDER_NS::TonemapConfiguration, Tonemap)
    META_PROPERTY(RENDER_NS::BloomConfiguration, Bloom)
    META_PROPERTY(RENDER_NS::VignetteConfiguration, Vignette)
    META_PROPERTY(RENDER_NS::ColorFringeConfiguration, ColorFringe)
    META_PROPERTY(RENDER_NS::DitherConfiguration, Dither)
    META_PROPERTY(RENDER_NS::BlurConfiguration, Blur)
    META_PROPERTY(RENDER_NS::ColorConversionConfiguration, ColorConversion)
    META_PROPERTY(RENDER_NS::FxaaConfiguration, Fxaa)
    META_PROPERTY(RENDER_NS::TaaConfiguration, Taa)
    META_PROPERTY(RENDER_NS::DofConfiguration, Dof)
    META_PROPERTY(RENDER_NS::MotionBlurConfiguration, MotionBlur)
};

META_REGISTER_CLASS(
    PostProcessComponent, "c1e6b17f-14bf-41f6-a045-50f0838828b6", META_NS::ObjectCategoryBits::NO_CATEGORY)

class PostProcessComponent : public META_NS::IntroduceInterfaces<Component, IInternalPostProcess> {
    META_OBJECT(PostProcessComponent, ClassId::PostProcessComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(IInternalPostProcess, uint32_t, EnableFlags, "PostProcessComponent.enableFlags")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::TonemapConfiguration, Tonemap, "PostProcessComponent.tonemapConfiguration")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::BloomConfiguration, Bloom, "PostProcessComponent.bloomConfiguration")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::VignetteConfiguration, Vignette, "PostProcessComponent.vignetteConfiguration")
    SCENE_STATIC_PROPERTY_DATA(IInternalPostProcess, RENDER_NS::ColorFringeConfiguration, ColorFringe,
        "PostProcessComponent.colorFringeConfiguration")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::DitherConfiguration, Dither, "PostProcessComponent.ditherConfiguration")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::BlurConfiguration, Blur, "PostProcessComponent.blurConfiguration")
    SCENE_STATIC_PROPERTY_DATA(IInternalPostProcess, RENDER_NS::ColorConversionConfiguration, ColorConversion,
        "PostProcessComponent.colorConversionConfiguration")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::FxaaConfiguration, Fxaa, "PostProcessComponent.fxaaConfiguration")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::TaaConfiguration, Taa, "PostProcessComponent.taaConfiguration")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalPostProcess, RENDER_NS::DofConfiguration, Dof, "PostProcessComponent.dofConfiguration")
    SCENE_STATIC_PROPERTY_DATA(IInternalPostProcess, RENDER_NS::MotionBlurConfiguration, MotionBlur,
        "PostProcessComponent.motionBlurConfiguration")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(uint32_t, EnableFlags)
    META_IMPLEMENT_PROPERTY(RENDER_NS::TonemapConfiguration, Tonemap)
    META_IMPLEMENT_PROPERTY(RENDER_NS::BloomConfiguration, Bloom)
    META_IMPLEMENT_PROPERTY(RENDER_NS::VignetteConfiguration, Vignette)
    META_IMPLEMENT_PROPERTY(RENDER_NS::ColorFringeConfiguration, ColorFringe)
    META_IMPLEMENT_PROPERTY(RENDER_NS::DitherConfiguration, Dither)
    META_IMPLEMENT_PROPERTY(RENDER_NS::BlurConfiguration, Blur)
    META_IMPLEMENT_PROPERTY(RENDER_NS::ColorConversionConfiguration, ColorConversion)
    META_IMPLEMENT_PROPERTY(RENDER_NS::FxaaConfiguration, Fxaa)
    META_IMPLEMENT_PROPERTY(RENDER_NS::TaaConfiguration, Taa)
    META_IMPLEMENT_PROPERTY(RENDER_NS::DofConfiguration, Dof)
    META_IMPLEMENT_PROPERTY(RENDER_NS::MotionBlurConfiguration, MotionBlur)
public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif