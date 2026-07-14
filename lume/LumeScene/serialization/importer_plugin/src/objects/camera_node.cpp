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

#include "camera_node.h"

#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/resource/image_info.h>

#include "../diagnostics.h"
#include "../import_helpers.h"
#include "flag_tables.h"
#include "format_table.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {

// clang-format off
static constexpr NamedValue<uint32_t> SCENE_FLAGS_TABLE[] = {
    { "active",     static_cast<uint32_t>(SCENE_NS::CameraSceneFlag::ACTIVE_RENDER_BIT) },
    { "mainCamera", static_cast<uint32_t>(SCENE_NS::CameraSceneFlag::MAIN_CAMERA_BIT) },
};

static constexpr NamedValue<uint32_t> PIPELINE_FLAGS_TABLE[] = {
    { "clearDepth",          static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_DEPTH_BIT) },
    { "clearColor",          static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT) },
    { "msaa",                static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT) },
    { "allowColorPrePass",   static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::ALLOW_COLOR_PRE_PASS_BIT) },
    { "forceColorPrePass",   static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::FORCE_COLOR_PRE_PASS_BIT) },
    { "history",             static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::HISTORY_BIT) },
    { "jitter",              static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::JITTER_BIT) },
    { "velocityOutput",      static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::VELOCITY_OUTPUT_BIT) },
    { "depthOutput",         static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::DEPTH_OUTPUT_BIT) },
    { "multiViewOnly",       static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MULTI_VIEW_ONLY_BIT) },
    { "disallowReflection",  static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::DISALLOW_REFLECTION_BIT) },
    { "cubemap",             static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CUBEMAP_BIT) },
};

static constexpr NamedValue<SCENE_NS::CameraCulling> CULLING_TABLE[] = {
    { "none",        SCENE_NS::CameraCulling::NONE },
    { "viewFrustum", SCENE_NS::CameraCulling::VIEW_FRUSTUM },
};

static constexpr NamedValue<SCENE_NS::CameraPipeline> RENDERING_PIPELINE_TABLE[] = {
    { "lightForward", SCENE_NS::CameraPipeline::LIGHT_FORWARD },
    { "forward",      SCENE_NS::CameraPipeline::FORWARD },
    { "deferred",     SCENE_NS::CameraPipeline::DEFERRED },
    { "custom",       SCENE_NS::CameraPipeline::CUSTOM },
};
// clang-format on

}  // namespace

SCENE_NS::INode::Ptr ImportCameraNode::ConstructNode(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, const BASE_NS::string& name)
{
    return scene
        ->CreateNode(interface_pointer_cast<SCENE_NS::INode>(context.GetImportParameters().object),
            name,
            SCENE_NS::ClassId::CameraNode)
        .GetResult();
}

// Read a 2-element float array and apply its components to two separate properties.
static IDiagnostics::Ptr ImportVec2Props(ImportContext& context, BASE_NS::string_view name,
    const META_NS::Property<float>& propX, const META_NS::Property<float>& propY)
{
    auto arr = context.GetOptArray(name);
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    if (arr.size() != 2 || !arr[0].is_number() || !arr[1].is_number()) {
        auto err = context.CreateDiagnostics(BASE_NS::string("invalid property value (property: ") + name + ")");
        if (h.Handle(err)) {
            return err;
        }
        return h;
    }
    SetValue(context, propX, arr[0].as_number<float>());
    SetValue(context, propY, arr[1].as_number<float>());
    return h;
}

static IDiagnostics::Ptr ImportCameraProjection(ImportContext& context, SCENE_NS::ICamera& camera)
{
    ErrorHandler h(context);
    auto value = GetOptString(context, "projection");
    if (value.error && h.Handle(value.error)) {
        return value.error;
    }
    if (!value.value) {
        return h;
    }
    if (value.GetValue() == "orthographic") {
        SetValue(context, camera.Projection(), SCENE_NS::CameraProjection::ORTHOGRAPHIC);
        if (auto err = ImportVec2Props(context, "magnification", camera.XMagnification(), camera.YMagnification());
            h.Handle(err)) {
            return err;
        }
    } else if (value.GetValue() == "perspective") {
        SetValue(context, camera.Projection(), SCENE_NS::CameraProjection::PERSPECTIVE);
        if (auto fov = GetOptFloat(context, "fov"); h.HandleOptValue(fov)) {
            if (fov.error) {
                return fov.error;
            }
            SetValue(context, camera.FoV(), fov.GetValue());
        }
        if (auto ar = GetOptFloat(context, "aspectRatio"); h.HandleOptValue(ar)) {
            if (ar.error) {
                return ar.error;
            }
            SetValue(context, camera.AspectRatio(), ar.GetValue());
        }
    } else if (value.GetValue() == "frustum") {
        SetValue(context, camera.Projection(), SCENE_NS::CameraProjection::FRUSTUM);
        if (auto err = ImportVec2Props(context, "offset", camera.XOffset(), camera.YOffset()); h.Handle(err)) {
            return err;
        }
    } else if (value.GetValue() == "custom") {
        SetValue(context, camera.Projection(), SCENE_NS::CameraProjection::CUSTOM);
        if (auto mat = GetOptMat4x4(context, "customProjectionMatrix"); h.HandleOptValue(mat)) {
            if (mat.error) {
                return mat.error;
            }
            SetValue(context, camera.CustomProjectionMatrix(), mat.GetValue());
        }
    } else {
        CORE_LOG_E("Invalid camera projection: %s", value.GetValue().c_str());
        return context.CreateDiagnostics("Invalid camera projection: " + value.GetValue());
    }
    return h;
}

static IDiagnostics::Ptr ImportCameraCulling(ImportContext& context, SCENE_NS::ICamera& camera)
{
    ErrorHandler h(context);
    if (auto value = GetOptString(context, "culling"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        auto result = LookupValue(CULLING_TABLE, value.GetValue());
        if (!result) {
            CORE_LOG_E("Invalid camera culling: %s", value.GetValue().c_str());
            return context.CreateDiagnostics("Invalid camera culling: " + value.GetValue());
        }
        SetValue(context, camera.Culling(), *result);
    }
    return h;
}

static IDiagnostics::Ptr ImportCameraRenderingPipeline(ImportContext& context, SCENE_NS::ICamera& camera)
{
    ErrorHandler h(context);
    if (auto value = GetOptString(context, "renderingPipeline"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        auto result = LookupValue(RENDERING_PIPELINE_TABLE, value.GetValue());
        if (!result) {
            CORE_LOG_E("Invalid camera renderingPipeline: %s", value.GetValue().c_str());
            return context.CreateDiagnostics("Invalid camera renderingPipeline: " + value.GetValue());
        }
        SetValue(context, camera.RenderingPipeline(), *result);
    }
    return h;
}

static IDiagnostics::Ptr ImportCameraSceneFlags(ImportContext& context, SCENE_NS::ICamera& camera)
{
    auto flags = context.GetOptArray("sceneFlags");
    if (flags.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    uint32_t bits = 0;
    if (auto err = ImportFlagsFromArray(context, flags, SCENE_FLAGS_TABLE, "sceneFlags", bits); h.Handle(err)) {
        return err;
    }
    SetValue(context, camera.SceneFlags(), bits);
    return h;
}

static IDiagnostics::Ptr ImportCameraPipelineFlags(ImportContext& context, SCENE_NS::ICamera& camera)
{
    auto flags = context.GetOptArray("pipelineFlags");
    if (flags.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    uint32_t bits = 0;
    if (auto err = ImportFlagsFromArray(context, flags, PIPELINE_FLAGS_TABLE, "pipelineFlags", bits); h.Handle(err)) {
        return err;
    }
    SetValue(context, camera.PipelineFlags(), bits);
    return h;
}

static OptValue<SCENE_NS::ColorFormat> ParseColorTargetEntry(ImportContext& context, const CORE_NS::json::value& v)
{
    if (!v.is_object()) {
        return OptValue<SCENE_NS::ColorFormat>{
            context.CreateDiagnostics("invalid colorTargets entry: expected object")};
    }
    auto elemContext = context.CreateContext(v);
    SCENE_NS::ColorFormat cf{};
    if (auto fmtValue = GetOptString(elemContext, "format"); fmtValue.value) {
        auto fmt = ParseFormat(fmtValue.GetValue());
        if (!fmt) {
            CORE_LOG_E("Invalid colorTargets format: %s", fmtValue.GetValue().c_str());
            return OptValue<SCENE_NS::ColorFormat>{
                context.CreateDiagnostics("Invalid colorTargets format: " + fmtValue.GetValue())};
        }
        cf.format = *fmt;
    } else if (fmtValue.error) {
        return OptValue<SCENE_NS::ColorFormat>{fmtValue.error};
    }
    for (auto&& fv : elemContext.GetOptArray("flags")) {
        if (!fv.is_string()) {
            return OptValue<SCENE_NS::ColorFormat>{context.CreateDiagnostics("invalid colorTargets flags value")};
        }
        auto bit = LookupValue(IMAGE_USAGE_FLAGS_TABLE, fv.string_);
        if (!bit) {
            CORE_LOG_E("Invalid colorTargets flags value: %s", BASE_NS::string(fv.string_).c_str());
            return OptValue<SCENE_NS::ColorFormat>{
                context.CreateDiagnostics(BASE_NS::string("Invalid colorTargets flags value: ") + fv.string_)};
        }
        cf.usageFlags |= static_cast<uint32_t>(*bit);
    }
    return OptValue<SCENE_NS::ColorFormat>{cf};
}

static IDiagnostics::Ptr ImportColorTargets(ImportContext& context, SCENE_NS::ICamera& camera)
{
    auto arr = context.GetOptArray("colorTargets");
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    BASE_NS::vector<SCENE_NS::ColorFormat> targets;
    for (auto&& v : arr) {
        if (auto entry = ParseColorTargetEntry(context, v); h.HandleOptValue(entry)) {
            if (entry.error) {
                return entry.error;
            }
            targets.push_back(entry.GetValue());
        }
    }
    SetValue(context, camera.ColorTargetCustomization(), targets);
    return h;
}

static IDiagnostics::Ptr ImportCameraPostProcess(ImportContext& context, SCENE_NS::ICamera& camera)
{
    return ImportPostProcessProperty(context, camera.PostProcess().GetProperty(), "postProcess");
}

static IDiagnostics::Ptr ImportCameraViewProps(ImportContext& context, SCENE_NS::ICamera& camera)
{
    ErrorHandler h(context);
    if (auto value = GetOptFloat(context, "nearPlane"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.NearPlane(), value.GetValue());
    }
    if (auto value = GetOptFloat(context, "farPlane"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.FarPlane(), value.GetValue());
    }
    if (auto value = GetOptVec4(context, "viewport"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.Viewport(), value.GetValue());
    }
    if (auto value = GetOptVec4(context, "scissor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.Scissor(), value.GetValue());
    }
    if (auto value = GetOptUVec2(context, "renderTargetSize"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.RenderTargetSize(), value.GetValue());
    }
    return h;
}

static IDiagnostics::Ptr ImportCameraRenderProps(ImportContext& context, SCENE_NS::ICamera& camera)
{
    ErrorHandler h(context);
    if (auto value = GetOptVec4(context, "clearColor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.ClearColor(), value.GetValue());
    }
    if (auto value = GetOptFloat(context, "clearDepth"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.ClearDepth(), value.GetValue());
    }
    if (auto value = GetOptUInt(context, "cameraLayerMask"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.CameraLayerMask(), value.GetValue());
    }
    if (auto value = GetOptUInt(context, "msaaSampleCount"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SCENE_NS::CameraSampleCount sampleCount;
        if (value.GetValue() == 2) {
            sampleCount = SCENE_NS::CameraSampleCount::COUNT_2;
        } else if (value.GetValue() == 4) {
            sampleCount = SCENE_NS::CameraSampleCount::COUNT_4;
        } else if (value.GetValue() == 8) {
            sampleCount = SCENE_NS::CameraSampleCount::COUNT_8;
        } else {
            CORE_LOG_E("Invalid msaaSampleCount: %llu", static_cast<unsigned long long>(value.GetValue()));
            return context.CreateDiagnostics("Invalid msaaSampleCount");
        }
        SetValue(context, camera.MSAASampleCount(), sampleCount);
    }
    if (auto value = GetOptFloat(context, "downsamplePercentage"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        SetValue(context, camera.DownsamplePercentage(), value.GetValue());
    }
    return h;
}

IDiagnostics::Ptr ImportCameraNode::ImportCamera(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    auto camera = interface_cast<SCENE_NS::ICamera>(node);
    if (!camera) {
        auto name = context.GetOptString("name");
        CORE_LOG_E("Node does not implement ICamera (name: '%s')", name.c_str());
        return context.CreateDiagnostics("Node does not implement ICamera (name: '" + name + "')");
    }
    ErrorHandler h(context);
    if (auto err = ImportCameraProjection(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportCameraCulling(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportCameraRenderingPipeline(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportCameraSceneFlags(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportCameraPipelineFlags(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportCameraViewProps(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportCameraRenderProps(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportColorTargets(context, *camera); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportCameraPostProcess(context, *camera); h.Handle(err)) {
        return err;
    }
    return h;
}

ImportResult ImportCameraNode::Import(ImportContext& context)
{
    auto res = ImportNode(context, "camera");
    if (res) {
        ErrorHandler h(context);
        if (auto err = ImportCamera(context, interface_pointer_cast<SCENE_NS::INode>(res.object)); h.Handle(err)) {
            return ImportResult{err};
        }
        MergeDiagnostics(res.error, static_cast<IDiagnostics::Ptr>(h));
    }
    return res;
}

SCENE_IMP_END_NAMESPACE()
