/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "render_node_default_camera_post_process_interface_controller.h"

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
    IRenderNodePostProcessInterfaceUtil::Info& info)
{
    RenderHandle outputHandle {};
    const bool mvLayerCam = ((layer != 0U) && (layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS));
    if (mvLayerCam) {
        if (cam.colorTargets[0U]) {
            outputHandle = cam.colorTargets[0U].GetHandle();
        } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_W(to_string(cam.id) + "FillPostProcessInterfaceImages_",
                "Multi-view target output not found for layer (%u)", layer);
#endif
        }
    }
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
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_UPSCALED) {
            auto& res = info.imageData.inputUpscaled;
            res.layer = layer;
            res.handle = ref.handle;
        }
    }
}
} // namespace

void RenderNodeDefaultCameraPostProcessInterfaceController::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    rnPostProcessUtil_ = CORE_NS::CreateInstance<IRenderNodePostProcessInterfaceUtil>(
        renderNodeContextMgr_->GetRenderContext(), UID_RENDER_NODE_POST_PROCESS_INTERFACE_UTIL);

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    currentScene_ = {};
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
        IRenderNodePostProcessInterfaceUtil::Info info;
        info.dataStorePrefix = stores_.dataStoreNamePrefix;
        FillPostProcessImages(currentScene_.camData.camera, currentScene_.multiviewLayer, namedResources, info);
        rnPostProcessUtil_->SetInfo(info);
        rnPostProcessUtil_->Init(*renderNodeContextMgr_);
    }

    RegisterOutputs();
}

void RenderNodeDefaultCameraPostProcessInterfaceController::PreExecuteFrame()
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

    if (rnPostProcessUtil_) {
        IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        // fetch from global render node graph register
        const array_view<const IRenderNodeGraphShareManager::NamedResource> namedResources =
            shrMgr.GetRegisteredGlobalRenderNodeOutputs(currentScene_.cameraControllerRenderNodeName);
        IRenderNodePostProcessInterfaceUtil::Info info;
        info.dataStorePrefix = stores_.dataStoreNamePrefix;
        FillPostProcessImages(currentScene_.camData.camera, currentScene_.multiviewLayer, namedResources, info);
        rnPostProcessUtil_->SetInfo(info);

        rnPostProcessUtil_->PreExecute();
    }

    RegisterOutputs();
}

void RenderNodeDefaultCameraPostProcessInterfaceController::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (rnPostProcessUtil_) {
        rnPostProcessUtil_->Execute(cmdList);
    }
}

void RenderNodeDefaultCameraPostProcessInterfaceController::RegisterOutputs()
{
    if ((jsonInputs_.customCameraId == INVALID_CAM_ID) && jsonInputs_.customCameraName.empty()) {
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

void RenderNodeDefaultCameraPostProcessInterfaceController::UpdateCurrentScene(
    const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultCamera& dataStoreCamera)
{
    currentScene_.multiviewLayer = PipelineStateConstants::GPU_IMAGE_ALL_LAYERS;
    currentScene_.camData = RenderNodeSceneUtil::GetSceneCameraData(
        dataStoreScene, dataStoreCamera, jsonInputs_.customCameraId, jsonInputs_.customCameraName);

    currentScene_.scene = dataStoreScene.GetScene();
    const auto& cam = currentScene_.camData.camera;
    const auto& cameras = dataStoreCamera.GetCameras();
    const uint32_t cameraCount = static_cast<uint32_t>(cameras.size());
    if (cam.multiViewParentCameraId != INVALID_CAM_ID) {
        // find the base multiview camera
        if (const uint32_t multiviewBaseCamIdx = dataStoreCamera.GetCameraIndex(cam.multiViewParentCameraId);
            multiviewBaseCamIdx < cameraCount) {
            const auto& multiviewBaseCam = cameras[multiviewBaseCamIdx];
            for (uint32_t camIdx = 0; camIdx < multiviewBaseCam.multiViewCameraCount; ++camIdx) {
                if (multiviewBaseCam.multiViewCameraIds[camIdx] == cam.id) {
                    // set our layer
                    currentScene_.multiviewLayer = camIdx + 1U;
                }
            }
        }
    }
    // if base multiview camera
    if (cam.multiViewCameraCount > 0U) {
        currentScene_.multiviewLayer = 0U;
    }
    // full name for fetching data
    const uint64_t fetchCamId = (cam.multiViewParentCameraId != INVALID_CAM_ID) ? cam.multiViewParentCameraId : cam.id;
    currentScene_.cameraControllerRenderNodeName = currentScene_.scene.name + to_hex(fetchCamId) + "CORE3D_RN_CAM_CTRL";
}

void RenderNodeDefaultCameraPostProcessInterfaceController::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
}

// for plugin / factory interface
IRenderNode* RenderNodeDefaultCameraPostProcessInterfaceController::Create()
{
    return new RenderNodeDefaultCameraPostProcessInterfaceController();
}

void RenderNodeDefaultCameraPostProcessInterfaceController::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultCameraPostProcessInterfaceController*>(instance);
}
CORE3D_END_NAMESPACE()
