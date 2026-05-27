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

#include "light_node.h"

#include <scene/interface/intf_light.h>

#include "../diagnostics.h"
#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

SCENE_NS::INode::Ptr ImportLightNode::ConstructNode(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, const BASE_NS::string& name)
{
    return scene
        ->CreateNode(interface_pointer_cast<SCENE_NS::INode>(context.GetImportParameters().object),
            name,
            SCENE_NS::ClassId::LightNode)
        .GetResult();
}

static IDiagnostics::Ptr ImportLightType(ImportContext& context, SCENE_NS::ILight& light)
{
    ErrorHandler h(context);
    if (auto value = GetOptString(context, "lightType"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        if (value.GetValue() == "directional") {
            SetValue(context, light.Type(), SCENE_NS::LightType::DIRECTIONAL);
        } else if (value.GetValue() == "point") {
            SetValue(context, light.Type(), SCENE_NS::LightType::POINT);
        } else if (value.GetValue() == "spot") {
            SetValue(context, light.Type(), SCENE_NS::LightType::SPOT);
        } else {
            CORE_LOG_E("Invalid light type: %s", value.GetValue().c_str());
            return context.CreateDiagnostics("Invalid light type: " + value.GetValue());
        }
    }
    return h;
}

static IDiagnostics::Ptr ImportLightShadow(ImportContext& context, SCENE_NS::ILight& light)
{
    auto optObject = context.GetOptObject("shadow");
    if (optObject.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    auto shadowContext = context.CreateContext(BASE_NS::move(optObject));
    if (auto value = GetOptBool(shadowContext, "enabled"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.ShadowEnabled(), value.GetValue());
    } else {
        // no enabled, but we have shadow structure so enable
        SetValue(context, light.ShadowEnabled(), true);
    }
    if (auto value = GetOptFloat(shadowContext, "strength"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.ShadowStrength(), value.GetValue());
    }
    if (auto value = GetOptFloat(shadowContext, "depthBias"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.ShadowDepthBias(), value.GetValue());
    }
    if (auto value = GetOptFloat(shadowContext, "normalBias"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.ShadowNormalBias(), value.GetValue());
    }
    if (auto value = GetOptUInt(shadowContext, "layerMask"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.ShadowLayerMask(), value.GetValue());
    }
    return h;
}

static IDiagnostics::Ptr ImportLightBaseProps(ImportContext& context, SCENE_NS::ILight& light)
{
    ErrorHandler h(context);
    if (auto value = GetOptColor(context, "color"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.Color(), value.GetValue());
    }
    if (auto value = GetOptFloat(context, "intensity"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.Intensity(), value.GetValue());
    }
    if (auto value = GetOptFloat(context, "nearPlane"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.NearPlane(), value.GetValue());
    }
    if (auto value = GetOptFloat(context, "range"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light.Range(), value.GetValue());
    }
    return h;
}

IDiagnostics::Ptr ImportLightNode::ImportLight(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    auto light = interface_cast<SCENE_NS::ILight>(node);
    if (!light) {
        auto name = context.GetOptString("name");
        CORE_LOG_E("Node does not implement ILight (name: '%s')", name.c_str());
        return context.CreateDiagnostics("Node does not implement ILight (name: '" + name + "')");
    }
    ErrorHandler h(context);
    if (auto err = ImportLightType(context, *light); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportLightBaseProps(context, *light); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportLightShadow(context, *light); h.Handle(err)) {
        return err;
    }
    if (auto value = GetOptFloat(context, "spotInnerAngle"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light->SpotInnerAngle(), value.GetValue());
    }
    if (auto value = GetOptFloat(context, "spotOuterAngle"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light->SpotOuterAngle(), value.GetValue());
    }
    if (auto value = GetOptVec4(context, "additionalFactor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light->AdditionalFactor(), value.GetValue());
    }
    if (auto value = GetOptUInt(context, "lightLayerMask"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, light->LightLayerMask(), value.GetValue());
    }
    return h;
}

ImportResult ImportLightNode::Import(ImportContext& context)
{
    auto res = ImportNode(context, "light");
    if (res) {
        ErrorHandler h(context);
        if (auto err = ImportLight(context, interface_pointer_cast<SCENE_NS::INode>(res.object)); h.Handle(err)) {
            return ImportResult{err};
        }
        MergeDiagnostics(res.error, static_cast<IDiagnostics::Ptr>(h));
    }
    return res;
}

SCENE_IMP_END_NAMESPACE()
