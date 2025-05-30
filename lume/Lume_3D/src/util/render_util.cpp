/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <algorithm>

#include <3d/intf_graphics_context.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <core/intf_engine.h>
#include <core/json/json.h>
#include <core/namespace.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_plugin.h>
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

constexpr const string_view RENDER_NODE_DEFAULT_CAMERA_CONTROLLER_STR = "RenderNodeDefaultCameraController";
constexpr const string_view RENDER_NODE_DEFAULT_MATERIAL_RENDER_SLOT_STR = "RenderNodeDefaultMaterialRenderSlot";
constexpr const string_view RENDER_NODE_CAMERA_WEATHER_STR = "RenderNodeCameraWeather";

constexpr bool ENABLE_WEATHER_INJECT { true };

RenderNodeGraphDesc LoadRenderNodeGraph(IRenderNodeGraphLoader& rngLoader, const string_view rng)
{
    IRenderNodeGraphLoader::LoadResult lr = rngLoader.Load(rng);
    if (!lr.success) {
        CORE_LOG_E("error loading render node graph: %s - error: %s", rng.data(), lr.error.data());
    }
    return lr.desc;
}

inline RenderNodeDesc GetDefaultCameraControllerNode()
{
    RenderNodeDesc rnd;
    rnd.typeName = RENDER_NODE_DEFAULT_CAMERA_CONTROLLER_STR;
    rnd.nodeName = "CORE3D_RN_CAM_CTRL";
    rnd.nodeJson = "{\"typeName\" : \"RenderNodeDefaultCameraController\",\"nodeName\" : \"CORE3D_RN_CAM_CTRL\"}";
    return rnd;
}

inline RenderNodeDesc GetDefaultCameraCloudsNode()
{
    RenderNodeDesc rnd;
    rnd.typeName = RENDER_NODE_CAMERA_WEATHER_STR;
    rnd.nodeName = "CORE3D_RN_CAM_WEATHER";
    rnd.nodeJson = "{\"typeName\" : \"RenderNodeCameraWeather\",\"nodeName\" : \"CORE3D_RN_CAM_WEATHER\"}";
    return rnd;
}

inline json::standalone_value GetPodPostProcess(const string_view name)
{
    auto renderDataStore = json::standalone_value { json::standalone_value::object {} };
    renderDataStore["dataStoreName"] = "RenderDataStorePod";
    renderDataStore["typeName"] = "RenderDataStorePod"; // This is render data store TYPE_NAME
    renderDataStore["configurationName"] = string(name);
    return renderDataStore;
}

inline json::standalone_value GetPostProcess(const string_view name)
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

vector<const RENDER_NS::RenderNodeTypeInfo*> GetRenderNodesWithDependencies()
{
    // Gather RenderNodeTypeInfo which have after of before dependencies.
    vector<const RENDER_NS::RenderNodeTypeInfo*> renderNodesWithDependencies;
    auto typeInfos = CORE_NS::GetPluginRegister().GetTypeInfos(RENDER_NS::RenderNodeTypeInfo::UID);
    for (auto* info : typeInfos) {
        if (info && (info->typeUid == RenderNodeTypeInfo::UID)) {
            auto* renderNodeTypeInfo = static_cast<const RenderNodeTypeInfo*>(info);
            if ((renderNodeTypeInfo->afterNode != Uid {}) || (renderNodeTypeInfo->beforeNode != Uid {})) {
                renderNodesWithDependencies.push_back(renderNodeTypeInfo);
            }
        }
    }
    return renderNodesWithDependencies;
}

void InjectRenderNodes(RenderNodeGraphDesc& desc)
{
    const auto typeInfos = CORE_NS::GetPluginRegister().GetTypeInfos(RENDER_NS::RenderNodeTypeInfo::UID);
    const auto renderNodeTypeInfos =
        array_view(reinterpret_cast<const RenderNodeTypeInfo* const*>(typeInfos.data()), typeInfos.size());
    const auto renderNodesWithDependencies = GetRenderNodesWithDependencies();
    for (const auto* depending : renderNodesWithDependencies) {
        // Try adding only if node isn't already there.
        if (std::any_of(desc.nodes.cbegin(), desc.nodes.cend(),
                [typeName = depending->typeName](const RenderNodeDesc& desc) { return desc.typeName == typeName; })) {
            continue;
        }

        ptrdiff_t index = -1;

        // Find the insertion index based on afterNode
        if (depending->afterNode != Uid {}) {
            // Find the typeinfo of the dependency UID
            auto info = std::find_if(renderNodeTypeInfos.cbegin(), renderNodeTypeInfos.cend(),
                [uid = depending->afterNode](const RenderNodeTypeInfo* info) { return info->uid == uid; });
            if (info != renderNodeTypeInfos.cend()) {
                // Find the last instance matching dependency typeName
                auto graphIt = std::find_if(desc.nodes.crbegin(), desc.nodes.crend(),
                    [typeName = (*info)->typeName](const RenderNodeDesc& desc) { return desc.typeName == typeName; });
                if (graphIt != desc.nodes.crend()) {
                    index = graphIt.base() - desc.nodes.cbegin();
                }
            }
        }

        // Find the insertion index based on beforeNode
        if (depending->beforeNode != Uid {}) {
            // Find the typeinfo of the dependency UID
            auto info = std::find_if(renderNodeTypeInfos.cbegin(), renderNodeTypeInfos.cend(),
                [uid = depending->beforeNode](const RenderNodeTypeInfo* info) { return info->uid == uid; });
            if (info != renderNodeTypeInfos.cend()) {
                // Find the last instance matching dependency typeName
                auto graphIt = std::find_if(desc.nodes.crbegin(), desc.nodes.crend(),
                    [typeName = (*info)->typeName](const RenderNodeDesc& desc) { return desc.typeName == typeName; });
                if (graphIt != desc.nodes.crend()) {
                    index = graphIt.base() - desc.nodes.cbegin();
                }
            }
        }

        // If no valid index found, continue to the next node
        if (index < 0 || size_t(index) > desc.nodes.size()) {
            continue;
        }

        // Create the new render node descriptor
        json::standalone_value jsonVal(json::standalone_value::object {});
        jsonVal["typeName"] = depending->typeName;
        auto nodeName = RenderDataConstants::RenderDataFixedString("CORE3D_RN_SCENE_") + depending->typeName;
        jsonVal["nodeName"] = nodeName.data();
        desc.nodes.insert(
            desc.nodes.cbegin() + index, RenderNodeDesc { depending->typeName, nodeName, {}, to_string(jsonVal) });
    }
}

void FillCameraDescsData(const RenderCamera& renderCamera, const string& customCameraName, RenderNodeGraphDesc& desc)
{
    // check for render compatibility for render node graph RenderNodeDefaultCameraController
    if (!renderCamera.customRenderNodeGraphFile.empty()) {
        bool forceInject = true;
        for (const auto& rnRef : desc.nodes) {
            if (rnRef.typeName == RENDER_NODE_DEFAULT_CAMERA_CONTROLLER_STR) {
                forceInject = false;
                break;
            }
        }
        if (forceInject) {
            for (size_t nodeIdx = 0; nodeIdx < desc.nodes.size(); ++nodeIdx) {
                if (desc.nodes[nodeIdx].typeName == RENDER_NODE_DEFAULT_MATERIAL_RENDER_SLOT_STR) {
                    desc.nodes.insert(desc.nodes.begin() + int64_t(nodeIdx), GetDefaultCameraControllerNode());
#if (CORE3D_DEV_ENABLED == 1)
                    CORE_LOG_W("Injecting camera RenderNodeDefaultCameraController render node for compatibility");
#endif
                    break;
                }
            }
        }
    }

    // inject camera weather effect render node (for e.g. clouds) after camera controller
    if constexpr (ENABLE_WEATHER_INJECT) {
        if (renderCamera.environment.flags & RenderCamera::Environment::ENVIRONMENT_FLAG_CAMERA_WEATHER_BIT) {
            for (size_t nodeIdx = 0; nodeIdx < desc.nodes.size(); ++nodeIdx) {
                if (desc.nodes[nodeIdx].typeName == RENDER_NODE_DEFAULT_CAMERA_CONTROLLER_STR) {
                    desc.nodes.insert(desc.nodes.begin() + int64_t(nodeIdx + 1), GetDefaultCameraCloudsNode());
#if (CORE3D_DEV_ENABLED == 1)
                    CORE_LOG_I("Injecting camera RenderNodeCameraWeather render node");
#endif
                    break;
                }
            }
        }
    }
    InjectRenderNodes(desc);

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
    InjectRenderNodes(desc);

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
    if (!renderCamera.customRenderNodeGraphFile.empty()) {
        // custom render node graph file given which is patched
        IRenderNodeGraphLoader& rngl = context_.GetRenderNodeGraphManager().GetRenderNodeGraphLoader();
        return LoadRenderNodeGraph(rngl, renderCamera.customRenderNodeGraphFile);
    }

    if (renderCamera.flags & RenderCamera::CAMERA_FLAG_REFLECTION_BIT) {
        // check for msaa
        return (renderCamera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) ? rngdReflCamMsaa_ : rngdReflCam_;
    }
    if (renderCamera.flags & RenderCamera::CAMERA_FLAG_OPAQUE_BIT) {
        return rngdCamPrePass_;
    }
    if (renderCamera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
        return rngdDeferred_;
    }
    if (renderCamera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) {
        // NOTE: check for optimal GL(ES) render node graph if backbuffer is multisampled
        const bool depthOutput =
            ((renderCamera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_OUTPUT_DEPTH_BIT) != 0U);
        if (renderCamera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
            return depthOutput ? rngdCamHdrMsaaDepth_ : rngdCamHdrMsaa_;
        }

        if ((backendType_ == DeviceBackendType::VULKAN) ||
            (renderCamera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_CUSTOM_TARGETS_BIT)) {
            return depthOutput ? rngdCamLwrpMsaaDepth_ : rngdCamLwrpMsaa_;
        }
        const auto& gpuResourceMgr = context_.GetDevice().GetGpuResourceManager();
        // NOTE: special handling on msaa swapchain with GLES
        // creates issues with multi-ECS cases and might need to be dropped
        // Prefer assigning swapchain image to camera in this case
        const RenderHandleReference imageHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_BACKBUFFER");
        const GpuImageDesc imageDesc = gpuResourceMgr.GetImageDescriptor(imageHandle);
        if ((renderCamera.flags & RenderCamera::CAMERA_FLAG_MAIN_BIT) && (imageDesc.sampleCountFlags > 1) &&
            (!depthOutput)) {
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_I("3d_util_depth_gles_mixing" + to_string(renderCamera.id),
                "CORE3D_VALIDATION: GL(ES) Camera MSAA flags checked from CORE_DEFAULT_BACKBUFFER. "
                "Prefer assigning swapchain handle to camera.");
#endif
            return rngdCamLwrpMsaaGles_;
        }
        return depthOutput ? rngdCamLwrpMsaaDepth_ : rngdCamLwrpMsaa_;
    }
    if (renderCamera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
        return rngdCamHdr_;
    }
    return rngdCamLwrp_;
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

    InjectRenderNodes(desc);

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

    InjectRenderNodes(desc);

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
