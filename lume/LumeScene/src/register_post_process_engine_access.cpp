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

#include <scene/base/namespace.h>
#include <scene/interface/intf_postprocess.h>

#include <3d/ecs/components/post_process_component.h>

#include <meta/base/namespace.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/engine/internal_access.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_object_registry.h>

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

META_TYPE(RENDER_NS::TonemapConfiguration);
META_TYPE(RENDER_NS::TonemapConfiguration::TonemapType);
META_TYPE(RENDER_NS::BloomConfiguration);
META_TYPE(RENDER_NS::BloomConfiguration::BloomType);
META_TYPE(RENDER_NS::BloomConfiguration::BloomQualityType);
META_TYPE(RENDER_NS::BlurConfiguration);
META_TYPE(RENDER_NS::BlurConfiguration::BlurType);
META_TYPE(RENDER_NS::BlurConfiguration::BlurQualityType);
META_TYPE(RENDER_NS::ColorConversionConfiguration);
META_TYPE(RENDER_NS::ColorConversionConfiguration::ConversionFunctionType);
META_TYPE(RENDER_NS::ColorFringeConfiguration);
META_TYPE(RENDER_NS::DitherConfiguration);
META_TYPE(RENDER_NS::DitherConfiguration::DitherType);
META_TYPE(RENDER_NS::DofConfiguration);
META_TYPE(RENDER_NS::FxaaConfiguration);
META_TYPE(RENDER_NS::FxaaConfiguration::Quality);
META_TYPE(RENDER_NS::FxaaConfiguration::Sharpness);
META_TYPE(RENDER_NS::MotionBlurConfiguration);
META_TYPE(RENDER_NS::MotionBlurConfiguration::Quality);
META_TYPE(RENDER_NS::MotionBlurConfiguration::Sharpness);
META_TYPE(RENDER_NS::TaaConfiguration);
META_TYPE(RENDER_NS::TaaConfiguration::Quality);
META_TYPE(RENDER_NS::TaaConfiguration::Sharpness);
META_TYPE(RENDER_NS::LensFlareConfiguration);
META_TYPE(RENDER_NS::LensFlareConfiguration::Quality);
META_TYPE(RENDER_NS::VignetteConfiguration);
META_TYPE(RENDER_NS::UpscaleConfiguration);

SCENE_BEGIN_NAMESPACE()

namespace Internal {

template<typename Prop>
void RegisterEngineAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);
    META_NS::GetObjectRegistry().GetEngineData().RegisterInternalValueAccess(
        META_NS::MetaType<Prop>::coreType, CreateShared<META_NS::EngineInternalValueAccess<Prop>>());
}

template<typename Prop, typename AccessType>
void RegisterMapEngineAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);

    auto& r = META_NS::GetObjectRegistry();
    r.GetEngineData().RegisterInternalValueAccess(
        META_NS::MetaType<Prop>::coreType, CreateShared<META_NS::EngineInternalValueAccess<Prop, AccessType>>());
}

template<typename Prop>
void UnregisterEngineAccessImpl()
{
    META_NS::GetObjectRegistry().GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<Prop>::coreType);
}

void RegisterPostProcessEngineAccess()
{
    RegisterEngineAccessImpl<BloomConfiguration>();
    RegisterEngineAccessImpl<BlurConfiguration>();
    RegisterEngineAccessImpl<ColorConversionConfiguration>();
    RegisterEngineAccessImpl<ColorFringeConfiguration>();
    RegisterEngineAccessImpl<DitherConfiguration>();
    RegisterEngineAccessImpl<DofConfiguration>();
    RegisterEngineAccessImpl<FxaaConfiguration>();
    RegisterEngineAccessImpl<MotionBlurConfiguration>();
    RegisterEngineAccessImpl<TaaConfiguration>();
    RegisterEngineAccessImpl<TonemapConfiguration>();
    RegisterEngineAccessImpl<VignetteConfiguration>();
    RegisterEngineAccessImpl<LensFlareConfiguration>();
    RegisterEngineAccessImpl<UpscaleConfiguration>();

    RegisterMapEngineAccessImpl<TonemapConfiguration::TonemapType, TonemapType>();
    RegisterMapEngineAccessImpl<BloomConfiguration::BloomType, BloomType>();
    RegisterMapEngineAccessImpl<BloomConfiguration::BloomQualityType, EffectQualityType>();
    RegisterMapEngineAccessImpl<BlurConfiguration::BlurType, BlurType>();
    RegisterMapEngineAccessImpl<BlurConfiguration::BlurQualityType, EffectQualityType>();
    RegisterMapEngineAccessImpl<FxaaConfiguration::Quality, EffectQualityType>();
    RegisterMapEngineAccessImpl<ColorConversionConfiguration::ConversionFunctionType, ColorConversionFunctionType>();
    RegisterMapEngineAccessImpl<FxaaConfiguration::Quality, EffectQualityType>();
    RegisterMapEngineAccessImpl<FxaaConfiguration::Sharpness, EffectSharpnessType>();
    RegisterMapEngineAccessImpl<MotionBlurConfiguration::Quality, EffectQualityType>();
    RegisterMapEngineAccessImpl<MotionBlurConfiguration::Sharpness, EffectSharpnessType>();
    RegisterMapEngineAccessImpl<TaaConfiguration::Quality, EffectQualityType>();
    RegisterMapEngineAccessImpl<TaaConfiguration::Sharpness, EffectSharpnessType>();
    RegisterMapEngineAccessImpl<LensFlareConfiguration::Quality, EffectQualityType>();
}

void UnregisterPostProcessEngineAccess()
{
    UnregisterEngineAccessImpl<TonemapConfiguration::TonemapType>();
    UnregisterEngineAccessImpl<BloomConfiguration::BloomType>();
    UnregisterEngineAccessImpl<BloomConfiguration::BloomQualityType>();
    UnregisterEngineAccessImpl<BlurConfiguration::BlurType>();
    UnregisterEngineAccessImpl<BlurConfiguration::BlurQualityType>();
    UnregisterEngineAccessImpl<FxaaConfiguration::Quality>();
    UnregisterEngineAccessImpl<ColorConversionConfiguration::ConversionFunctionType>();
    UnregisterEngineAccessImpl<FxaaConfiguration::Quality>();
    UnregisterEngineAccessImpl<FxaaConfiguration::Sharpness>();
    UnregisterEngineAccessImpl<MotionBlurConfiguration::Quality>();
    UnregisterEngineAccessImpl<MotionBlurConfiguration::Sharpness>();
    UnregisterEngineAccessImpl<TaaConfiguration::Quality>();
    UnregisterEngineAccessImpl<TaaConfiguration::Sharpness>();
    UnregisterEngineAccessImpl<LensFlareConfiguration::Quality>();

    UnregisterEngineAccessImpl<BloomConfiguration>();
    UnregisterEngineAccessImpl<BlurConfiguration>();
    UnregisterEngineAccessImpl<ColorConversionConfiguration>();
    UnregisterEngineAccessImpl<ColorFringeConfiguration>();
    UnregisterEngineAccessImpl<DitherConfiguration>();
    UnregisterEngineAccessImpl<DofConfiguration>();
    UnregisterEngineAccessImpl<FxaaConfiguration>();
    UnregisterEngineAccessImpl<MotionBlurConfiguration>();
    UnregisterEngineAccessImpl<TaaConfiguration>();
    UnregisterEngineAccessImpl<TonemapConfiguration>();
    UnregisterEngineAccessImpl<VignetteConfiguration>();
    UnregisterEngineAccessImpl<LensFlareConfiguration>();
    UnregisterEngineAccessImpl<UpscaleConfiguration>();
}

} // namespace Internal
SCENE_END_NAMESPACE()
