/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "util/render_util.h"

#include <3d/intf_graphics_context.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <core/intf_engine.h>
#include <core/json/json.h>
#include <core/namespace.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/render_data_structures.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr const string_view SCENE_STR = "3drendernodegraphs://core3d_rng_scene.rng";
constexpr const string_view CAM_SCENE_LWRP_STR = "3drendernodegraphs://core3d_rng_cam_scene_lwrp.rng";
constexpr const string_view CAM_SCENE_LWRP_MSAA_STR = "3drendernodegraphs://core3d_rng_cam_scene_lwrp_msaa.rng";
constexpr const string_view CAM_SCENE_LWRP_MSAA_GLES_STR =
    "3drendernodegraphs://core3d_rng_cam_scene_lwrp_msaa_gles.rng";
constexpr const string_view CAM_SCENE_HDRP_STR = "3drendernodegraphs://core3d_rng_cam_scene_hdrp.rng";
constexpr const string_view CAM_SCENE_HDRP_MSAA_STR = "3drendernodegraphs://core3d_rng_cam_scene_hdrp_msaa.rng";
constexpr const string_view CAM_SCENE_REFLECTION_STR = "3drendernodegraphs://core3d_rng_reflection_cam_scene.rng";
constexpr const string_view CAM_SCENE_PRE_PASS_STR = "3drendernodegraphs://core3d_rng_cam_scene_pre_pass.rng";
constexpr const string_view CAM_SCENE_DEFERRED_STR = "3drendernodegraphs://core3d_rng_cam_scene_deferred.rng";

RenderNodeGraphDesc LoadRenderNodeGraph(IRenderNodeGraphLoader& rngLoader, const string_view rng)
{
    IRenderNodeGraphLoader::LoadResult lr = rngLoader.Load(rng);
    if (!lr.success) {
        CORE_LOG_E("error loading render node graph: %s - error: %s", rng.data(), lr.error.data());
    }
    return lr.desc;
}
} // namespace

RenderUtil::RenderUtil(IGraphicsContext& graphicsContext)
    : context_(graphicsContext.GetRenderContext()), backendType_(context_.GetDevice().GetBackendType())
{
    InitRenderNodeGraphs();
}

void RenderUtil::InitRenderNodeGraphs()
{
    IRenderNodeGraphManager& rngm = context_.GetRenderNodeGraphManager();
    IRenderNodeGraphLoader& rngl = rngm.GetRenderNodeGraphLoader();

    rngdScene_ = LoadRenderNodeGraph(rngl, SCENE_STR);
    rngdCamLwrp_ = LoadRenderNodeGraph(rngl, CAM_SCENE_LWRP_STR);
    rngdCamLwrpMsaa_ = LoadRenderNodeGraph(rngl, CAM_SCENE_LWRP_MSAA_STR);
    rngdCamLwrpMsaaGles_ = LoadRenderNodeGraph(rngl, CAM_SCENE_LWRP_MSAA_GLES_STR);
    rngdCamHdr_ = LoadRenderNodeGraph(rngl, CAM_SCENE_HDRP_STR);
    rngdCamHdrMsaa_ = LoadRenderNodeGraph(rngl, CAM_SCENE_HDRP_MSAA_STR);
    rngdReflCam_ = LoadRenderNodeGraph(rngl, CAM_SCENE_REFLECTION_STR);
    rngdCamPrePass_ = LoadRenderNodeGraph(rngl, CAM_SCENE_PRE_PASS_STR);

    rngdDeferred_ = LoadRenderNodeGraph(rngl, CAM_SCENE_DEFERRED_STR);
}

RenderNodeGraphDesc RenderUtil::SelectBaseDesc(const RenderCamera& renderCamera) const
{
    RenderNodeGraphDesc desc;
    if (!renderCamera.customRenderNodeGraphFile.empty()) {
        // custom render node graph file given which is patched
        IRenderNodeGraphLoader& rngl = context_.GetRenderNodeGraphManager().GetRenderNodeGraphLoader();
        desc = LoadRenderNodeGraph(rngl, renderCamera.customRenderNodeGraphFile);
    } else {
        if (renderCamera.flags & RenderCamera::CAMERA_FLAG_REFLECTION_BIT) {
            desc = rngdReflCam_;
        } else if (renderCamera.flags & RenderCamera::CAMERA_FLAG_OPAQUE_BIT) {
            desc = rngdCamPrePass_;
        } else {
            if (renderCamera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
                desc = rngdDeferred_;
            } else {
                if (renderCamera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) {
                    // NOTE: check for optimal GL(ES) render node graph if backbuffer is multisampled
                    if (renderCamera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
                        desc = rngdCamHdrMsaa_;
                    } else if (backendType_ == DeviceBackendType::VULKAN) {
                        desc = rngdCamLwrpMsaa_;
                    } else {
                        const auto& gpuResourceMgr = context_.GetDevice().GetGpuResourceManager();
                        const RenderHandleReference imageHandle =
                            gpuResourceMgr.GetImageHandle("CORE_DEFAULT_BACKBUFFER");
                        const GpuImageDesc imageDesc = gpuResourceMgr.GetImageDescriptor(imageHandle);
                        if (imageDesc.sampleCountFlags > 1) {
                            desc = rngdCamLwrpMsaaGles_;
                        } else {
                            desc = rngdCamLwrpMsaa_;
                        }
                    }
                } else if (renderCamera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
                    desc = rngdCamHdr_;
                } else {
                    desc = rngdCamLwrp_;
                }
            }
        }
    }
    return desc;
}

RenderNodeGraphDesc RenderUtil::GetRenderNodeGraphDesc(
    const RenderScene& renderScene, const RenderCamera& renderCamera, const uint32_t flags) const
{
    RenderNodeGraphDesc desc = SelectBaseDesc(renderCamera);
    // process
    desc.renderNodeGraphName = renderScene.name + to_hex(renderCamera.id);
    for (auto& rnRef : desc.nodes) {
        json::standalone_value jsonVal = CORE_NS::json::parse(rnRef.nodeJson.data());
        jsonVal["customCameraId"] = renderCamera.id; // cam id
        jsonVal["customCameraName"] = string(desc.renderNodeGraphName);
        if (renderCamera.flags & RenderCamera::CAMERA_FLAG_REFLECTION_BIT) {
            jsonVal["nodeFlags"] = 7u; // NOTE: hard coded
        }
        if (auto dataStore = jsonVal.find("renderDataStore"); dataStore) {
            if (auto config = dataStore->find("configurationName"); config) {
                if (!config->string_.empty()) {
                    auto renderDataStore = json::standalone_value { json::standalone_value::object {} };
                    renderDataStore["dataStoreName"] = "RenderDataStorePod";
                    renderDataStore["typeName"] = "PostProcess";
                    renderDataStore["configurationName"] = string(renderCamera.postProcessName);
                    jsonVal["renderDataStore"] = move(renderDataStore);
                }
            }
        }
        rnRef.nodeJson = to_string(jsonVal);
    }

    return desc;
}

RenderNodeGraphDesc RenderUtil::GetRenderNodeGraphDesc(const RenderScene& renderScene, const uint32_t flags) const
{
    RenderNodeGraphDesc desc;
    if (!renderScene.customRenderNodeGraphFile.empty()) {
        // custom render node graph file given which is patched
        IRenderNodeGraphLoader& rngl = context_.GetRenderNodeGraphManager().GetRenderNodeGraphLoader();
        desc = LoadRenderNodeGraph(rngl, renderScene.customRenderNodeGraphFile);
    } else {
        desc = rngdScene_;
    }
    const auto sceneIdStr = to_hex(renderScene.id);
    const auto combinedStr = renderScene.name + sceneIdStr;
    desc.renderNodeGraphName = combinedStr;
    for (auto& rnRef : desc.nodes) {
        json::standalone_value jsonVal = CORE_NS::json::parse(rnRef.nodeJson.data());
        jsonVal[string_view("customId")] = renderScene.id; // cam id
        jsonVal[string_view("customName")] = string(combinedStr);
        rnRef.nodeJson = to_string(jsonVal);
    }
    return desc;
}
CORE3D_END_NAMESPACE()
