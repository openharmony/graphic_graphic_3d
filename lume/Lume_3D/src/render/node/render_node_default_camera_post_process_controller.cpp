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

#include "render_node_default_camera_post_process_controller.h"

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <core/plugin/intf_class_register.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_post_process_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "render/render_node_scene_util.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE3D_BEGIN_NAMESPACE()
namespace {
void FillPostProcessImages(const RenderCamera& cam, const uint32_t layer,
    const array_view<const IRenderNodeGraphShareManager::NamedResource> namedResources,
    IRenderNodePostProcessUtil::PostProcessInfo& info)
{
    RenderHandle outputHandle {};
    const bool mvLayerCam = ((layer != 0U) && (layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS));
    if (mvLayerCam) {
        if (cam.colorTargets[0U]) {
            outputHandle = cam.colorTargets[0U].GetHandle();
        } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_W(to_string(cam.id) + "FillPostProcessImages_",
                "Multi-view target output not found for layer (%u)", layer);
#endif
        }
    }
    // NOTE: history not currently supported for multi-view only camera
    for (const auto& ref : namedResources) {
        if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_OUTPUT) {
            auto& res = info.imageData.output;
            if ((layer != 0U) && (layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)) {
                res.handle = outputHandle;
            } else {
                res.handle = ref.handle;
            }
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR) {
            auto& res = info.imageData.input;
            res.layer = layer;
            res.handle = ref.handle;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH) {
            auto& res = info.imageData.depth;
            res.layer = layer;
            res.handle = ref.handle;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_VELOCITY_NORMAL) {
            auto& res = info.imageData.velocity;
            res.layer = layer;
            res.handle = ref.handle;
        } else if ((ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY) && (!mvLayerCam)) {
            info.imageData.history.handle = ref.handle;
        } else if ((ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY_NEXT) && (!mvLayerCam)) {
            info.imageData.historyNext.handle = ref.handle;
        }
    }
}
} // namespace

void RenderNodeDefaultCameraPostProcessController::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    rnPostProcessUtil_ = CORE_NS::CreateInstance<IRenderNodePostProcessUtil>(
        renderNodeContextMgr_->GetRenderContext(), UID_RENDER_NODE_POST_PROCESS_UTIL);

    validPostProcessInputs_ = false;
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    currentScene_ = {};

    // id checked first for custom camera
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        currentScene_.customCameraId = jsonInputs_.customCameraId;
    } else if (!jsonInputs_.customCameraName.empty()) {
        currentScene_.customCameraName = jsonInputs_.customCameraName;
    }

    {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto* dataStoreScene = static_cast<IRenderDataStoreDefaultScene*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
        const auto* dataStoreCamera = static_cast<IRenderDataStoreDefaultCamera*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
        const bool validRenderDataStore = dataStoreScene && dataStoreCamera;
        if (validRenderDataStore) {
            UpdateCurrentScene(*dataStoreScene, *dataStoreCamera);
        }
    }

    if (rnPostProcessUtil_) {
        IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        // fetch from global render node graph register
        const array_view<const IRenderNodeGraphShareManager::NamedResource> namedResources =
            shrMgr.GetRegisteredGlobalRenderNodeOutputs(currentScene_.cameraControllerRenderNodeName);
        IRenderNodePostProcessUtil::PostProcessInfo info;
        info.parseRenderNodeInputs = false;
        FillPostProcessImages(currentScene_.camera, currentScene_.multiviewLayer, namedResources, info);
        if (info.parseRenderNodeInputs || RenderHandleUtil::IsValid(info.imageData.output.handle)) {
            validPostProcessInputs_ = true;
        }

        rnPostProcessUtil_->Init(*renderNodeContextMgr_, info);
    }

    RegisterOutputs();
}

void RenderNodeDefaultCameraPostProcessController::PreExecuteFrame()
{
    {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto* dataStoreScene = static_cast<IRenderDataStoreDefaultScene*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
        const auto* dataStoreCamera = static_cast<IRenderDataStoreDefaultCamera*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
        const bool validRenderDataStore = dataStoreScene && dataStoreCamera;
        if (validRenderDataStore) {
            UpdateCurrentScene(*dataStoreScene, *dataStoreCamera);
        }
    }

    validPostProcessInputs_ = false;
    if (rnPostProcessUtil_) {
        IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        // fetch from global render node graph register
        const array_view<const IRenderNodeGraphShareManager::NamedResource> namedResources =
            shrMgr.GetRegisteredGlobalRenderNodeOutputs(currentScene_.cameraControllerRenderNodeName);
        IRenderNodePostProcessUtil::PostProcessInfo info;
        info.parseRenderNodeInputs = false;
        FillPostProcessImages(currentScene_.camera, currentScene_.multiviewLayer, namedResources, info);

        if (info.parseRenderNodeInputs || RenderHandleUtil::IsValid(info.imageData.output.handle)) {
            validPostProcessInputs_ = true;
        }

        if (validPostProcessInputs_) {
            rnPostProcessUtil_->PreExecute(info);
        }
    }

    RegisterOutputs();
}

void RenderNodeDefaultCameraPostProcessController::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (rnPostProcessUtil_ && validPostProcessInputs_) {
        rnPostProcessUtil_->Execute(cmdList);
    }
}

void RenderNodeDefaultCameraPostProcessController::RegisterOutputs()
{
    if ((currentScene_.customCameraId == INVALID_CAM_ID) && currentScene_.customCameraName.empty()) {
        return;
    }

    IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
    // fetch from global render node graph register
    const array_view<const IRenderNodeGraphShareManager::NamedResource> namedResources =
        shrMgr.GetRegisteredGlobalRenderNodeOutputs(currentScene_.cameraControllerRenderNodeName);
    for (const auto& ref : namedResources) {
        shrMgr.RegisterRenderNodeOutput(ref.name, ref.handle);
    }
}

void RenderNodeDefaultCameraPostProcessController::UpdateCurrentScene(
    const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultCamera& dataStoreCamera)
{
    const auto scene = dataStoreScene.GetScene();
    uint32_t cameraIdx = scene.cameraIndex;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraName);
    }

    currentScene_.multiviewLayer = PipelineStateConstants::GPU_IMAGE_ALL_LAYERS;
    const auto cameras = dataStoreCamera.GetCameras();
    const uint32_t cameraCount = static_cast<uint32_t>(cameras.size());
    if (cameraIdx < cameraCount) {
        // store current frame camera
        currentScene_.camera = cameras[cameraIdx];
        if (currentScene_.camera.multiViewParentCameraId != INVALID_CAM_ID) {
            // find the base multiview camera
            if (const uint32_t multiviewBaseCamIdx =
                    dataStoreCamera.GetCameraIndex(currentScene_.camera.multiViewParentCameraId);
                multiviewBaseCamIdx < cameraCount) {
                const auto& multiviewBaseCam = cameras[multiviewBaseCamIdx];
                for (uint32_t camIdx = 0; camIdx < multiviewBaseCam.multiViewCameraCount; ++camIdx) {
                    if (multiviewBaseCam.multiViewCameraIds[camIdx] == currentScene_.camera.id) {
                        // set our layer
                        currentScene_.multiviewLayer = camIdx + 1U;
                    }
                }
            }
        }
        // if base multiview camera
        if (currentScene_.camera.multiViewCameraCount > 0U) {
            currentScene_.multiviewLayer = 0U;
        }
    }
    // full name for fetching data
    const uint64_t fetchCamId = (currentScene_.camera.multiViewParentCameraId != INVALID_CAM_ID)
                                    ? currentScene_.camera.multiViewParentCameraId
                                    : currentScene_.camera.id;
    currentScene_.cameraControllerRenderNodeName = string(scene.name) + to_hex(fetchCamId) + "CORE3D_RN_CAM_CTRL";
}

void RenderNodeDefaultCameraPostProcessController::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
}

// for plugin / factory interface
IRenderNode* RenderNodeDefaultCameraPostProcessController::Create()
{
    return new RenderNodeDefaultCameraPostProcessController();
}

void RenderNodeDefaultCameraPostProcessController::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultCameraPostProcessController*>(instance);
}
CORE3D_END_NAMESPACE()
