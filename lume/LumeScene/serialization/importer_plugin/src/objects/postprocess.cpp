/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "postprocess.h"

#include <scene/interface/intf_postprocess.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/postprocess/intf_blur.h>
#include <scene/interface/postprocess/intf_color_conversion.h>
#include <scene/interface/postprocess/intf_color_fringe.h>
#include <scene/interface/postprocess/intf_dof.h>
#include <scene/interface/postprocess/intf_fxaa.h>
#include <scene/interface/postprocess/intf_lens_flare.h>
#include <scene/interface/postprocess/intf_motion_blur.h>
#include <scene/interface/postprocess/intf_taa.h>
#include <scene/interface/postprocess/intf_tonemap.h>
#include <scene/interface/postprocess/intf_upscale.h>
#include <scene/interface/postprocess/intf_vignette.h>
#include <scene/interface/resource/types.h>

#include <meta/api/metadata_util.h>

#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {
// clang-format off
static constexpr NamedValue<SCENE_NS::EffectQualityType> QUALITY_TABLE[] = {
    { "low",    SCENE_NS::EffectQualityType::LOW },
    { "normal", SCENE_NS::EffectQualityType::NORMAL },
    { "high",   SCENE_NS::EffectQualityType::HIGH },
};

static constexpr NamedValue<SCENE_NS::EffectSharpnessType> SHARPNESS_TABLE[] = {
    { "soft",   SCENE_NS::EffectSharpnessType::SOFT },
    { "medium", SCENE_NS::EffectSharpnessType::MEDIUM },
    { "sharp",  SCENE_NS::EffectSharpnessType::SHARP },
};

static constexpr NamedValue<SCENE_NS::BloomType> BLOOM_TYPE_TABLE[] = {
    { "normal",     SCENE_NS::BloomType::NORMAL },
    { "horizontal", SCENE_NS::BloomType::HORIZONTAL },
    { "vertical",   SCENE_NS::BloomType::VERTICAL },
    { "bilateral",  SCENE_NS::BloomType::BILATERAL },
};

static constexpr NamedValue<SCENE_NS::TonemapType> TONEMAP_TYPE_TABLE[] = {
    { "aces",       SCENE_NS::TonemapType::ACES },
    { "aces2020",   SCENE_NS::TonemapType::ACES_2020 },
    { "filmic",     SCENE_NS::TonemapType::FILMIC },
    { "pbrNeutral", SCENE_NS::TonemapType::TONEMAP_PBR_NEUTRAL },
};

static constexpr NamedValue<SCENE_NS::BlurType> BLUR_TYPE_TABLE[] = {
    { "normal",     SCENE_NS::BlurType::NORMAL },
    { "horizontal", SCENE_NS::BlurType::HORIZONTAL },
    { "vertical",   SCENE_NS::BlurType::VERTICAL },
};

static constexpr NamedValue<SCENE_NS::ColorConversionFunctionType> COLOR_CONVERSION_TABLE[] = {
    { "linear",       SCENE_NS::ColorConversionFunctionType::LINEAR },
    { "linearToSrgb", SCENE_NS::ColorConversionFunctionType::LINEAR_TO_SRGB },
};
// clang-format on
}  // namespace

static META_NS::IObject::Ptr CreateEffectTemplate()
{
    return META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
}

static META_NS::IMetadata* GetMeta(const META_NS::IObject::Ptr& obj)
{
    return interface_cast<META_NS::IMetadata>(obj);
}

static IDiagnostics::Ptr ImportEffectEnabled(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptBool(context, "enabled"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "Enabled", value.GetValue());
    }
    return h;
}

using EffectResult = OptValue<META_NS::IObject::Ptr>;

static EffectResult ImportTonemap(ImportContext& context)
{
    auto obj = context.GetOptObject("tonemap");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create tonemap template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptString(effectContext, "type"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        auto result = LookupValue(TONEMAP_TYPE_TABLE, value.GetValue());
        if (!result) {
            CORE_LOG_E("Invalid tonemap type: %s", value.GetValue().c_str());
            return EffectResult{effectContext.CreateDiagnostics("Invalid tonemap type: " + value.GetValue())};
        }
        AddSetProperty(*meta, "Type", *result);
    }
    if (auto value = GetOptFloat(effectContext, "exposure"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "Exposure", value.GetValue());
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static IDiagnostics::Ptr ImportBloomProps(
    ImportContext& context, const META_NS::IObject::Ptr& effect, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto err = ImportEnumProperty(context, meta, "quality", "Quality", QUALITY_TABLE); h.Handle(err)) {
        return err;
    }
    static const BASE_NS::string_view floatProps[] = {
        "thresholdHard", "thresholdSoft", "amountCoefficient", "dirtMaskCoefficient", "scatter", "scaleFactor"};
    static const BASE_NS::string_view floatPropNames[] = {
        "ThresholdHard", "ThresholdSoft", "AmountCoefficient", "DirtMaskCoefficient", "Scatter", "ScaleFactor"};
    for (size_t i = 0; i < sizeof(floatProps) / sizeof(floatProps[0]); ++i) {
        if (auto value = GetOptFloat(context, floatProps[i]); h.HandleOptValue(value)) {
            if (value.error) {
                return value.error;
            }
            AddSetProperty(meta, floatPropNames[i], value.GetValue());
        }
    }
    if (auto err = ImportImageProperty(context, effect, "dirtMaskImage", "DirtMaskImage"); h.Handle(err)) {
        return err;
    }
    if (auto value = GetOptBool(context, "useCompute"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "UseCompute", value.GetValue());
    }
    return h;
}

static EffectResult ImportBloom(ImportContext& context)
{
    auto obj = context.GetOptObject("bloom");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create bloom template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "type", "Type", BLOOM_TYPE_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportBloomProps(effectContext, effect, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportBlur(ImportContext& context)
{
    auto obj = context.GetOptObject("blur");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create blur template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptString(effectContext, "type"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        auto result = LookupValue(BLUR_TYPE_TABLE, value.GetValue());
        if (!result) {
            CORE_LOG_E("Invalid blur type: %s", value.GetValue().c_str());
            return EffectResult{effectContext.CreateDiagnostics("Invalid blur type: " + value.GetValue())};
        }
        AddSetProperty(*meta, "Type", *result);
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "quality", "Quality", QUALITY_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptFloat(effectContext, "filterSize"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "FilterSize", value.GetValue());
    }
    if (auto value = GetOptUInt(effectContext, "maxMipmapLevel"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "MaxMipmapLevel", static_cast<uint32_t>(value.GetValue()));
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportMotionBlur(ImportContext& context)
{
    auto obj = context.GetOptObject("motionBlur");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create motionBlur template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "quality", "Quality", QUALITY_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "sharpness", "Sharpness", SHARPNESS_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptFloat(effectContext, "alpha"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "Alpha", value.GetValue());
    }
    if (auto value = GetOptFloat(effectContext, "velocityCoefficient"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "VelocityCoefficient", value.GetValue());
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportColorConversion(ImportContext& context)
{
    auto obj = context.GetOptObject("colorConversion");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create colorConversion template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptString(effectContext, "function"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        auto result = LookupValue(COLOR_CONVERSION_TABLE, value.GetValue());
        if (!result) {
            CORE_LOG_E("Invalid color conversion function: %s", value.GetValue().c_str());
            return EffectResult{
                effectContext.CreateDiagnostics("Invalid color conversion function: " + value.GetValue())};
        }
        AddSetProperty(*meta, "Function", *result);
    }
    if (auto value = GetOptBool(effectContext, "multiplyWithAlpha"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "MultiplyWithAlpha", value.GetValue());
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportColorFringe(ImportContext& context)
{
    auto obj = context.GetOptObject("colorFringe");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create colorFringe template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptFloat(effectContext, "distanceCoefficient"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "DistanceCoefficient", value.GetValue());
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportDepthOfField(ImportContext& context)
{
    auto obj = context.GetOptObject("depthOfField");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create depthOfField template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    static const BASE_NS::string_view floatProps[] = {"focusPoint",
        "focusRange",
        "nearTransitionRange",
        "farTransitionRange",
        "nearBlur",
        "farBlur",
        "nearPlane",
        "farPlane"};
    static const BASE_NS::string_view floatPropNames[] = {"FocusPoint",
        "FocusRange",
        "NearTransitionRange",
        "FarTransitionRange",
        "NearBlur",
        "FarBlur",
        "NearPlane",
        "FarPlane"};
    for (size_t i = 0; i < sizeof(floatProps) / sizeof(floatProps[0]); ++i) {
        if (auto value = GetOptFloat(effectContext, floatProps[i]); h.HandleOptValue(value)) {
            if (value.error) {
                return EffectResult{value.error};
            }
            AddSetProperty(*meta, floatPropNames[i], value.GetValue());
        }
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportFxaa(ImportContext& context)
{
    auto obj = context.GetOptObject("fxaa");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create fxaa template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "quality", "Quality", QUALITY_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "sharpness", "Sharpness", SHARPNESS_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportTaa(ImportContext& context)
{
    auto obj = context.GetOptObject("taa");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create taa template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "quality", "Quality", QUALITY_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "sharpness", "Sharpness", SHARPNESS_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportVignette(ImportContext& context)
{
    auto obj = context.GetOptObject("vignette");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create vignette template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptFloat(effectContext, "coefficient"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "Coefficient", value.GetValue());
    }
    if (auto value = GetOptFloat(effectContext, "power"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "Power", value.GetValue());
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportLensFlare(ImportContext& context)
{
    auto obj = context.GetOptObject("lensFlare");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create lensFlare template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto err = ImportEnumProperty(effectContext, *meta, "quality", "Quality", QUALITY_TABLE); h.Handle(err)) {
        return EffectResult{err};
    }
    if (auto value = GetOptFloat(effectContext, "intensity"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "Intensity", value.GetValue());
    }
    if (auto value = GetOptVec3(effectContext, "flarePosition"); h.HandleOptValue(value)) {
        if (value.error) {
            return EffectResult{value.error};
        }
        AddSetProperty(*meta, "FlarePosition", value.GetValue());
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static EffectResult ImportUpscale(ImportContext& context)
{
    auto obj = context.GetOptObject("upscale");
    if (obj.empty()) {
        return {};
    }
    auto effectContext = context.CreateContext(BASE_NS::move(obj));
    auto effect = CreateEffectTemplate();
    auto meta = GetMeta(effect);
    if (!meta) {
        return EffectResult{context.CreateDiagnostics("Failed to create upscale template")};
    }
    ErrorHandler h(effectContext);
    if (auto err = ImportEffectEnabled(effectContext, *meta); h.Handle(err)) {
        return EffectResult{err};
    }
    static const BASE_NS::string_view floatProps[] = {"smoothScale", "structureSensitivity", "edgeSharpness", "ratio"};
    static const BASE_NS::string_view floatPropNames[] = {
        "SmoothScale", "StructureSensitivity", "EdgeSharpness", "Ratio"};
    for (size_t i = 0; i < sizeof(floatProps) / sizeof(floatProps[0]); ++i) {
        if (auto value = GetOptFloat(effectContext, floatProps[i]); h.HandleOptValue(value)) {
            if (value.error) {
                return EffectResult{value.error};
            }
            AddSetProperty(*meta, floatPropNames[i], value.GetValue());
        }
    }
    EffectResult r{effect};
    r.error = h;
    return r;
}

static IDiagnostics::Ptr AddEffect(META_NS::IMetadata& meta, BASE_NS::string_view propName, EffectResult result)
{
    if (result.value) {
        AddSetProperty(meta, propName, *result.value);
    }
    return result.error;
}

static IDiagnostics::Ptr ImportAllEffects(ImportContext& context, META_NS::IMetadata& meta)
{
    using EffectImporter = EffectResult (*)(ImportContext&);
    struct EffectEntry {
        BASE_NS::string_view name;
        EffectImporter importer;
    };
    static const EffectEntry effects[] = {
        {"Tonemap", ImportTonemap},
        {"Bloom", ImportBloom},
        {"Blur", ImportBlur},
        {"MotionBlur", ImportMotionBlur},
        {"ColorConversion", ImportColorConversion},
        {"ColorFringe", ImportColorFringe},
        {"DepthOfField", ImportDepthOfField},
        {"Fxaa", ImportFxaa},
        {"Taa", ImportTaa},
        {"Vignette", ImportVignette},
        {"LensFlare", ImportLensFlare},
        {"Upscale", ImportUpscale},
    };
    ErrorHandler h(context);
    for (auto&& e : effects) {
        if (auto err = AddEffect(meta, e.name, e.importer(context)); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

ImportResult ImportPostProcess::Import(ImportContext& context)
{
    auto trace = context.Trace("Importing postProcess");
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::PostProcessTemplate);
    if (!obj) {
        return ImportResult{context.CreateDiagnostics("Failed to create post process template")};
    }
    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    if (!meta) {
        return ImportResult{context.CreateDiagnostics("Post process template has no metadata")};
    }

    ErrorHandler h(context);
    if (auto err = ImportAllEffects(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }

    ImportResult result{obj};
    result.error = h;
    return result;
}

SCENE_IMP_END_NAMESPACE()
