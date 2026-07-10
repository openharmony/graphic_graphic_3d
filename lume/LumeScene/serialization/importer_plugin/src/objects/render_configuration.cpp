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

#include "render_configuration.h"

#include <scene/interface/intf_render_configuration.h>

#include <meta/api/util.h>

#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {
// clang-format off
constexpr NamedValue<SCENE_NS::SceneShadowType> SHADOW_TYPE_TABLE[] = {
    { "pcf",          SCENE_NS::SceneShadowType::PCF },
    { "vsm",          SCENE_NS::SceneShadowType::VSM },
    { "variablePcf",  SCENE_NS::SceneShadowType::VARIABLE_PCF },
};
constexpr NamedValue<SCENE_NS::SceneShadowQuality> SHADOW_QUALITY_TABLE[] = {
    { "low",     SCENE_NS::SceneShadowQuality::LOW },
    { "normal",  SCENE_NS::SceneShadowQuality::NORMAL },
    { "high",    SCENE_NS::SceneShadowQuality::HIGH },
    { "ultra",   SCENE_NS::SceneShadowQuality::ULTRA },
};
constexpr NamedValue<SCENE_NS::SceneShadowSmoothness> SHADOW_SMOOTHNESS_TABLE[] = {
    { "hard",    SCENE_NS::SceneShadowSmoothness::HARD },
    { "normal",  SCENE_NS::SceneShadowSmoothness::NORMAL },
    { "soft",    SCENE_NS::SceneShadowSmoothness::SOFT },
};
// clang-format on

template<typename Enum, size_t N>
IDiagnostics::Ptr ApplyEnum(ImportContext& ctx, BASE_NS::string_view jsonName, const NamedValue<Enum> (&table)[N],
    const META_NS::Property<Enum>& target)
{
    ErrorHandler h(ctx);
    auto value = GetOptString(ctx, jsonName);
    if (!h.HandleOptValue(value)) {
        return h;
    }
    if (value.error) {
        return value.error;
    }
    auto result = LookupValue(table, value.GetValue());
    if (!result) {
        return ctx.CreateDiagnostics(BASE_NS::string("Invalid ") + jsonName + ": " + value.GetValue());
    }
    META_NS::SetValue(target, *result);
    return h;
}

template<typename T, typename V>
IDiagnostics::Ptr ApplyValue(ErrorHandler& h, OptValue<V> value, const META_NS::Property<T>& target)
{
    if (!h.HandleOptValue(value)) {
        return nullptr;
    }
    if (value.error) {
        return value.error;
    }
    META_NS::SetValue(target, static_cast<T>(value.GetValue()));
    return nullptr;
}

IDiagnostics::Ptr ImportShadowSettings(ImportContext& ctx, SCENE_NS::IRenderConfiguration& rc, ErrorHandler& h)
{
    if (auto err = ApplyEnum(ctx, "shadowType", SHADOW_TYPE_TABLE, rc.ShadowType()); h.Handle(err)) {
        return err;
    }
    if (auto err = ApplyEnum(ctx, "shadowQuality", SHADOW_QUALITY_TABLE, rc.ShadowQuality()); h.Handle(err)) {
        return err;
    }
    if (auto err = ApplyEnum(ctx, "shadowSmoothness", SHADOW_SMOOTHNESS_TABLE, rc.ShadowSmoothness()); h.Handle(err)) {
        return err;
    }
    if (auto err = ApplyValue(h, GetOptUVec2(ctx, "shadowResolution"), rc.ShadowResolution())) {
        return err;
    }
    if (auto err = ApplyValue(h, GetOptFloat(ctx, "variablePcfRadius"), rc.VariablePcfRadius())) {
        return err;
    }
    if (auto err = ApplyValue(h, GetOptUInt(ctx, "variablePcfSampleCount"), rc.VariablePcfSampleCount())) {
        return err;
    }
    return nullptr;
}

IDiagnostics::Ptr ImportRenderGraphUris(ImportContext& ctx, SCENE_NS::IRenderConfiguration& rc, ErrorHandler& h)
{
    if (auto err = ApplyValue(h, GetOptString(ctx, "renderNodeGraphUri"), rc.RenderNodeGraphUri())) {
        return err;
    }
    if (auto err = ApplyValue(h, GetOptString(ctx, "postRenderNodeGraphUri"), rc.PostRenderNodeGraphUri())) {
        return err;
    }
    return nullptr;
}
}  // namespace

IDiagnostics::Ptr ImportRenderConfigurationInto(ImportContext& context, const SCENE_NS::IScene::Ptr& scene)
{
    auto trace = context.Trace("Importing renderConfiguration");
    if (!scene) {
        return context.CreateDiagnostics("Scene is null");
    }
    auto rc = META_NS::GetValue(scene->RenderConfiguration());
    if (!rc) {
        return context.CreateDiagnostics("Failed to obtain RenderConfiguration");
    }

    ErrorHandler h(context);
    if (auto err = ImportEnvironmentProperty(context, rc->Environment().GetProperty(), "environment"); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportRenderGraphUris(context, *rc, h)) {
        return err;
    }
    if (auto err = ImportShadowSettings(context, *rc, h)) {
        return err;
    }
    return h;
}

SCENE_IMP_END_NAMESPACE()
