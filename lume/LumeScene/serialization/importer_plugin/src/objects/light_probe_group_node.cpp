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

#include "light_probe_group_node.h"

#include <scene/interface/intf_light_probe_group.h>

#include <3d/light_probe_types/light_probe_constants.h>

#include "../diagnostics.h"
#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

constexpr size_t VEC3_ELEMENTS = 3;
constexpr size_t SH_COEFFICIENT_COUNT = CORE3D_NS::LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT;

SCENE_NS::INode::Ptr ImportLightProbeGroupNode::ConstructNode(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, const BASE_NS::string& name)
{
    return scene
        ->CreateNode(interface_pointer_cast<SCENE_NS::INode>(context.GetImportParameters().object),
            name,
            SCENE_NS::ClassId::LightProbeGroupNode)
        .GetResult();
}

static OptValue<BASE_NS::Math::Vec3> ParseVec3(
    ImportContext& context, const CORE_NS::json::value& v, BASE_NS::string_view what)
{
    if (!v.is_array() || v.array_.size() != VEC3_ELEMENTS || !v.array_[0].is_number() || !v.array_[1].is_number() ||
        !v.array_[2].is_number()) {
        return OptValue<BASE_NS::Math::Vec3>{
            context.CreateDiagnostics(BASE_NS::string("invalid vec3 value (") + what + ")")};
    }
    return OptValue<BASE_NS::Math::Vec3>{BASE_NS::Math::Vec3{
        v.array_[0].as_number<float>(), v.array_[1].as_number<float>(), v.array_[2].as_number<float>()}};
}

static IDiagnostics::Ptr ParseLightProbe(
    ImportContext& context, const CORE_NS::json::value& jprobe, SCENE_NS::LightProbe& out)
{
    if (!jprobe.is_object()) {
        return context.CreateDiagnostics("Invalid light probe entry: expected object");
    }
    auto probeContext = context.CreateContext(jprobe);
    ErrorHandler h(probeContext);

    if (auto pos = GetOptVec3(probeContext, "position"); h.HandleOptValue(pos)) {
        if (pos.error) {
            return pos.error;
        }
        out.position = pos.GetValue();
    }

    auto shArr = probeContext.GetOptArray("shCoefficients");
    if (!shArr.empty()) {
        if (shArr.size() != SH_COEFFICIENT_COUNT) {
            return probeContext.CreateDiagnostics(
                "shCoefficients must contain exactly " + BASE_NS::to_string(SH_COEFFICIENT_COUNT) + " vec3 entries");
        }
        for (size_t i = 0; i < SH_COEFFICIENT_COUNT; ++i) {
            auto v = ParseVec3(probeContext, shArr[i], "shCoefficients");
            if (!v.error) {
                out.shCoefficients[i] = v.GetValue();
                continue;
            }
            if (h.Handle(v.error)) {
                return v.error;
            }
        }
    }

    if (auto bn = GetOptVec3(probeContext, "bentNormal"); h.HandleOptValue(bn)) {
        if (bn.error) {
            return bn.error;
        }
        out.bentNormal = bn.GetValue();
    }
    if (auto ao = GetOptFloat(probeContext, "ao"); h.HandleOptValue(ao)) {
        if (ao.error) {
            return ao.error;
        }
        out.ao = ao.GetValue();
    }
    return h;
}

IDiagnostics::Ptr ImportLightProbeGroupNode::ImportLightProbeGroup(
    ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    auto group = interface_cast<SCENE_NS::ILightProbeGroup>(node);
    if (!group) {
        auto name = context.GetOptString("name");
        CORE_LOG_E("Node does not implement ILightProbeGroup (name: '%s')", name.c_str());
        return context.CreateDiagnostics("Node does not implement ILightProbeGroup (name: '" + name + "')");
    }
    auto arr = context.GetOptArray("lightProbes");
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    SCENE_NS::LightProbeGroup probes;
    probes.resize(arr.size());
    for (size_t i = 0; i < arr.size(); ++i) {
        if (auto err = ParseLightProbe(context, arr[i], probes[i]); h.Handle(err)) {
            return err;
        }
    }
    group->SetLightProbeGroup(probes);
    return h;
}

ImportResult ImportLightProbeGroupNode::Import(ImportContext& context)
{
    auto res = ImportNode(context, "lightProbeGroup");
    if (res) {
        ErrorHandler h(context);
        if (auto err = ImportLightProbeGroup(context, interface_pointer_cast<SCENE_NS::INode>(res.object));
            h.Handle(err)) {
            return ImportResult{err};
        }
        MergeDiagnostics(res.error, static_cast<IDiagnostics::Ptr>(h));
    }
    return res;
}

SCENE_IMP_END_NAMESPACE()
