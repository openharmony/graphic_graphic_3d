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
constexpr const string_view CAM_SCENE_LWRP_MSAA_DEPTH_STR =
    "3drendernodegraphs://core3d_rng_cam_scene_lwrp_msaa_depth.rng";
constexpr const string_view CAM_SCENE_LWRP_MSAA_GLES_STR =
    "3drendernodegraphs://core3d_rng_cam_scene_lwrp_msaa_gles.rng";
constexpr const string_view CAM_SCENE_HDRP_STR = "3drendernodegraphs://core3d_rng_cam_scene_hdrp.rng";
constexpr const string_view CAM_SCENE_HDRP_MSAA_STR = "3drendernodegraphs://core3d_rng_cam_scene_hdrp_msaa.rng";
constexpr const string_view CAM_SCENE_HDRP_MSAA_DEPTH_STR =
    "3drendernodegraphs://core3d_rng_cam_scene_hdrp_msaa_depth.rng";
constexpr const string_view CAM_SCENE_REFLECTION_STR = "3drendernodegraphs://core3d_rng_reflection_cam_scene.rng";
constexpr const string_view CAM_SCENE_REFLECTION_MSAA_STR =
    "3drendernodegraphs://core3d_rng_reflection_cam_scene_msaa.rng";
constexpr const string_view CAM_SCENE_PRE_PASS_STR = "3drendernodegraphs://core3d_rng_cam_scene_pre_pass.rng";
constexpr const string_view CAM_SCENE_DEFERRED_STR = "3drendernodegraphs://core3d_rng_cam_scene_deferred.rng";
constexpr const string_view CAM_SCENE_POST_PROCESS_STR = "3drendernodegraphs://core3d_rng_cam_scene_post_process.rng";

RenderNodeGraphDesc LoadRenderNodeGraph(IRenderNodeGraphLoader& rngLoader, const string_view rng)
{
    IRenderNodeGraphLoader::LoadResult lr = rngLoader.Load(rng);
    if (!lr.success) {
        CORE_LOG_E("error loading render node graph: %s - error: %s", rng.data(), lr.error.data());
    }
    return lr.desc;
}

json::standalone_value GetPodPostProcess(const string_view name)
{
    auto renderDataStore = json::standalone_value { json::standalone_value::object {} };
    renderDataStore["dataStoreName"] = "RenderDataStorePod";
    renderDataStore["typeName"] = "RenderDataStorePod"; // This is render data store TYPE_NAME
    renderDataStore["configurationName"] = string(name);
    return renderDataStore;
}

json::standalone_value GetPostProcess(const string_view name)
{
    auto renderDataStore = json::standalone_value { json::standalone_value::object {} };
    renderDataStore["dataStoreName"] = "RenderDataStorePostProcess";
    renderDataStore["typeName"] = "RenderDataStorePostProcess"; // This is render data store TYPE_NAME
    renderDataStore["configurationName"] = string(name);
    return renderDataStore;
}

inline bool HasRenderDataStorePostProcess(const json::standalone_value& dataStore)
{
    if (const auto typeName = dataStore.find("typeName"); typeName) {
        if (typeName->is_string() && (typeName->string_ == "RenderDataStorePostProcess")) {
            return true;
        }
    }
    return false;
}

inline string GetCameraName(const RenderCamera& camera)
{
    return camera.name.empty() ? string(to_hex(camera.id)) : string(camera.name);
}

inline string GetSceneName(const RenderScene& scene)
{
    return scene.name.empty() ? string(to_hex(scene.id)) : string(scene.name);
}

void FillCameraDescsData(const RenderCamera& renderCamera, const string& customCameraName, RenderNodeGraphDesc& desc)
{
    for (auto& rnRef : desc.nodes) {
        json::standalone_value jsonVal = CORE_NS::json::parse(rnRef.nodeJson.data());
        jsonVal["customCameraId"] = renderCamera.id; // cam id
        jsonVal["customCameraName"] = customCameraName;
        if (renderCamera.flags & RenderCamera::CAMERA_FLAG_REFLECTION_BIT) {
            jsonVal["nodeFlags"] = 7u; // NOTE: hard coded
        }
        if (auto dataStore = jsonVal.find("renderDataStore"); dataStore) {
            if (auto config = dataStore->find("configurationName"); config) {
                if (!config->string_.empty()) {
                    const bool rpp = HasRenderDataStorePostProcess(*dataStore);
                    auto renderDataStore = rpp ? GetPostProcess(renderCamera.postProcessName)
                                               : GetPodPostProcess(renderCamera.postProcessName);
                    jsonVal["renderDataStore"] = move(renderDataStore);
                }
            }
        }
        rnRef.nodeJson = to_string(jsonVal);
    }
}

void FillCameraPostProcessDescsData(const RenderCamera& renderCamera, const uint64_t baseCameraId,
    const string& customCameraName, const bool customPostProcess, RenderNodeGraphDesc& desc)
{
    for (auto& rnRef : desc.nodes) {
        json::standalone_value jsonVal = CORE_NS::json::parse(rnRef.nodeJson.data());
        // add camera info as well
        jsonVal["customCameraId"] = renderCamera.id; // cam id
        jsonVal["customCameraName"] = customCameraName;
        jsonVal["multiviewBaseCameraId"] = baseCameraId;
        if (auto dataStore = jsonVal.find("renderDataStore"); dataStore) {
            if (auto config = dataStore->find("configurationName"); config) {
                if (config->is_string() && (!config->string_.empty())) {
                    const bool rpp = customPostProcess || HasRenderDataStorePostProcess(*dataStore);
                    auto renderDataStore = rpp ? GetPostProcess(renderCamera.postProcessName)
                                               : GetPodPostProcess(renderCamera.postProcessName);
                    jsonVal["renderDataStore"] = move(renderDataStore);
                }
            }
        }
        rnRef.nodeJson = to_string(jsonVal);
    }
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
    rngdCamLwrpMsaaDepth_ = LoadRenderNodeGraph(rngl, CAM_SCENE_LWRP_MSAA_DEPTH_STR);
    rngdCamLwrpMsaaGles_ = LoadRenderNodeGraph(rngl, CAM_SCENE_LWRP_MSAA_GLES_STR);
    rngdCamHdr_ = LoadRenderNodeGraph(rngl, CAM_SCENE_HDRP_STR);
    rngdCamHdrMsaa_ = LoadRenderNodeGraph(rngl, CAM_SCENE_HDRP_MSAA_STR);
    rngdCamHdrMsaaDepth_ = LoadRenderNodeGraph(rngl, CAM_SCENE_HDRP_MSAA_DEPTH_STR);
    rngdReflCam_ = LoadRenderNodeGraph(rngl, CAM_SCENE_REFLECTION_STR);
    rngdReflCamMsaa_ = LoadRenderNodeGraph(rngl, CAM_SCENE_REFLECTION_MSAA_STR);
    rngdCamPrePass_ = LoadRenderNodeGraph(rngl, CAM_SCENE_PRE_PASS_STR);

    rngdDeferred_ = LoadRenderNodeGraph(rngl, CAM_SCENE_DEFERRED_STR);

    rngdPostProcess_ = LoadRenderNodeGraph(rngl, CAM_SCENE_POST_PROCESS_STR);
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
            // check for msaa
            desc = (renderCamera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) ? rngdReflCamMsaa_ : rngdReflCam_;
        } else if (renderCamera.flags & RenderCamera::CAMERA_FLAG_OPAQUE_BIT) {
            desc = rngdCamPrePass_;
        } else {
            if (renderCamera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
                desc = rngdDeferred_;
            } else {
                if (renderCamera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) {
                    // NOTE: check for optimal GL(ES) render node graph if backbuffer is multisampled
                    const bool depthOutput =
                        ((renderCamera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_OUTPUT_DEPTH_BIT) != 0U);
                    if (renderCamera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
                        desc = depthOutput ? rngdCamHdrMsaaDepth_ : rngdCamHdrMsaa_;
                    } else if ((backendType_ == DeviceBackendType::VULKAN) ||
                               (renderCamera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_CUSTOM_TARGETS_BIT)) {
                        desc = depthOutput ? rngdCamLwrpMsaaDepth_ : rngdCamLwrpMsaa_;
                    } else {
                        const auto& gpuResourceMgr = context_.GetDevice().GetGpuResourceManager();
                        // NOTE: special handling on msaa swapchain with GLES
                        // creates issues with multi-ECS cases and might need to be dropped
                        // Prefer assigning swapchain image to camera in this case
                        const RenderHandleReference imageHandle =
                            gpuResourceMgr.GetImageHandle("CORE_DEFAULT_BACKBUFFER");
                        const GpuImageDesc imageDesc = gpuResourceMgr.GetImageDescriptor(imageHandle);
                        if ((renderCamera.flags & RenderCamera::CAMERA_FLAG_MAIN_BIT) &&
                            (imageDesc.sampleCountFlags > 1) && (!depthOutput)) {
#if (CORE3D_VALIDATION_ENABLED == 1)
                            CORE_LOG_ONCE_I("3d_util_depth_gles_mixing" + to_string(renderCamera.id),
                                "CORE3D_VALIDATION: GL(ES) Camera MSAA flags checked from CORE_DEFAULT_BACKBUFFER. "
                                "Prefer assigning swapchain handle to camera.");
#endif
                            desc = rngdCamLwrpMsaaGles_;
                        } else {
                            desc = depthOutput ? rngdCamLwrpMsaaDepth_ : rngdCamLwrpMsaa_;
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

RenderNodeGraphDesc RenderUtil::GetBasePostProcessDesc(const RenderCamera& renderCamera) const
{
    // light forward and opaque / pre-pass camera does not have separate post processes
    if ((renderCamera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) &&
        ((renderCamera.flags & RenderCamera::CAMERA_FLAG_OPAQUE_BIT) == 0)) {
        return rngdPostProcess_;
    } else {
        return {};
    }
}

RenderNodeGraphDesc RenderUtil::GetRenderNodeGraphDesc(
    const RenderScene& renderScene, const RenderCamera& renderCamera, const uint32_t flags) const
{
    const string customCameraName = GetCameraName(renderCamera);
    RenderNodeGraphDesc desc = SelectBaseDesc(renderCamera);
    // process
    desc.renderNodeGraphName = renderScene.name + to_hex(renderCamera.id);
    FillCameraDescsData(renderCamera, customCameraName, desc);
    return desc;
}

IRenderUtil::CameraRenderNodeGraphDescs RenderUtil::GetRenderNodeGraphDescs(const RenderScene& renderScene,
    const RenderCamera& renderCamera, const uint32_t flags, const array_view<const RenderCamera> multiviewCameras) const
{
    // Gets RenderNodeGraphDescs for camera and patches
    // 1. With built-in RNGs
    //  * Select RNG for camera
    //  * Select RNG for camera post process (if not LIGHT_FORWARD)
    // 2. With built-in RNG for camera and custom for post process
    //  * Select RNG for camera
    //  * Load RNG for camera post process
    // 3. With custom RNG for camera
    //  * Load RNG for camera
    //  * Load RNG for camera post process, only if custom given
    // NOTE: special opaque / pre-pass camera (transmission) has built-in post process in RNG

    const string customCameraName = GetCameraName(renderCamera);
    IRenderUtil::CameraRenderNodeGraphDescs descs;
    // process camera
    {
        auto& desc = descs.camera;
        desc = SelectBaseDesc(renderCamera);
        desc.renderNodeGraphName = renderScene.name + to_hex(renderCamera.id);
        FillCameraDescsData(renderCamera, customCameraName, desc);
    }
    // process post process
    // NOTE: we do not add base post process render node graph to custom camera render node graphs
    {
        // NOTE: currently there are separate paths for new post process configurations and old
        auto& desc = descs.postProcess;
        const bool customPostProcess = !renderCamera.customPostProcessRenderNodeGraphFile.empty();
        if (customPostProcess) {
            IRenderNodeGraphLoader& rngl = context_.GetRenderNodeGraphManager().GetRenderNodeGraphLoader();
            desc = LoadRenderNodeGraph(rngl, renderCamera.customPostProcessRenderNodeGraphFile);
        } else if (renderCamera.customRenderNodeGraphFile.empty()) {
            // only fetched when using built-in camera RNGs
            desc = GetBasePostProcessDesc(renderCamera);
        }
        if (!desc.nodes.empty()) {
            desc.renderNodeGraphName = renderScene.name + to_hex(renderCamera.id);
            FillCameraPostProcessDescsData(
                renderCamera, RenderSceneDataConstants::INVALID_ID, customCameraName, customPostProcess, desc);
        }
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (renderCamera.multiViewCameraCount != static_cast<uint32_t>(multiviewCameras.size())) {
            CORE_LOG_W("CORE3D_VALIDATION: Multi-view camera count mismatch in render node graph creation.");
        }
#endif
        // multi-view camera post processes
        if (renderCamera.multiViewCameraCount == static_cast<uint32_t>(multiviewCameras.size())) {
#if (CORE3D_VALIDATION_ENABLED == 1)
            if (!renderCamera.customPostProcessRenderNodeGraphFile.empty()) {
                CORE_LOG_W("CORE3D_VALIDATION: Multi-view camera custom post process render node graph not supported.");
            }
#endif
            CORE_ASSERT(descs.multiViewCameraCount <= countof(descs.multiViewCameraPostProcesses));
            descs.multiViewCameraCount = renderCamera.multiViewCameraCount;
            for (size_t mvIdx = 0; mvIdx < descs.multiViewCameraCount; ++mvIdx) {
                const auto& mvCameraRef = multiviewCameras[mvIdx];
                auto& ppDesc = descs.multiViewCameraPostProcesses[mvIdx];
                ppDesc = GetBasePostProcessDesc(mvCameraRef);
                if (!ppDesc.nodes.empty()) {
                    const string ppCustomCameraName = GetCameraName(mvCameraRef);
                    ppDesc.renderNodeGraphName = renderScene.name + to_hex(mvCameraRef.id);
                    FillCameraPostProcessDescsData(mvCameraRef, renderCamera.id, ppCustomCameraName, false, ppDesc);
                }
            }
        }
    }

    return descs;
}

IRenderUtil::CameraRenderNodeGraphDescs RenderUtil::GetRenderNodeGraphDescs(
    const RenderScene& renderScene, const RenderCamera& renderCamera, const uint32_t flags) const
{
    return GetRenderNodeGraphDescs(renderScene, renderCamera, flags, {});
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
    const string customSceneName = GetSceneName(renderScene);
    const auto sceneIdStr = to_hex(renderScene.id);
    const auto combinedStr = renderScene.name + sceneIdStr;
    desc.renderNodeGraphName = combinedStr;
    for (auto& rnRef : desc.nodes) {
        json::standalone_value jsonVal = CORE_NS::json::parse(rnRef.nodeJson.data());
        jsonVal[string_view("customId")] = renderScene.id; // cam id
        jsonVal[string_view("customName")] = customSceneName;
        rnRef.nodeJson = to_string(jsonVal);
    }
    return desc;
}

RenderNodeGraphDesc RenderUtil::GetRenderNodeGraphDesc(
    const RenderScene& renderScene, const string& rngFile, const uint32_t flags) const
{
    RenderNodeGraphDesc desc;
    if (!rngFile.empty()) {
        // custom render node graph file given which is patched
        IRenderNodeGraphLoader& rngl = context_.GetRenderNodeGraphManager().GetRenderNodeGraphLoader();
        desc = LoadRenderNodeGraph(rngl, rngFile);
    } else {
        desc = rngdScene_;
    }
    const string customSceneName = GetSceneName(renderScene);
    const auto sceneIdStr = to_hex(renderScene.id);
    const auto combinedStr = renderScene.name + sceneIdStr;
    desc.renderNodeGraphName = combinedStr;
    for (auto& rnRef : desc.nodes) {
        json::standalone_value jsonVal = CORE_NS::json::parse(rnRef.nodeJson.data());
        jsonVal[string_view("customId")] = renderScene.id;
        jsonVal[string_view("customName")] = customSceneName;
        rnRef.nodeJson = to_string(jsonVal);
    }
    return desc;
}
CORE3D_END_NAMESPACE()
