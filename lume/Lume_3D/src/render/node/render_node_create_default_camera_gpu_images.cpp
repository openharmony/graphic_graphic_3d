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

#include "render_node_create_default_camera_gpu_images.h"

#include <3d/render/intf_render_data_store_default_camera.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/render_data_structures.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

void RenderNodeCreateDefaultCameraGpuImages::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    if (jsonInputs_.gpuImageDescs.empty()) {
        CORE_LOG_W("RenderNodeCreateDefaultCameraGpuImages: No gpu image descs given to");
    }

    descs_.reserve(jsonInputs_.gpuImageDescs.size());
    resourceHandles_.reserve(jsonInputs_.gpuImageDescs.size());

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    for (const auto& ref : jsonInputs_.gpuImageDescs) {
        descs_.push_back(ref);

        GpuImageDesc desc = ref.desc;
        desc.width = Math::max(1u, static_cast<uint32_t>(static_cast<float>(desc.width) * ref.dependencySizeScale));
        desc.height = Math::max(1u, static_cast<uint32_t>(static_cast<float>(desc.height) * ref.dependencySizeScale));
        resourceHandles_.push_back(gpuResourceMgr.Create(ref.name, desc));
    }
    // broadcast the resources
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        rngShareMgr.RegisterRenderNodeOutput(descs_[idx].shareName, resourceHandles_[idx].GetHandle());
    }
}

void RenderNodeCreateDefaultCameraGpuImages::PreExecuteFrame()
{
    CORE_ASSERT(resourceHandles_.size() == descs_.size());
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    if (dataStoreCamera) {
        const auto& cameras = dataStoreCamera->GetCameras();
        if (!cameras.empty() && !descs_.empty()) {
            for (size_t idx = 0; idx < descs_.size(); ++idx) {
                // If no camera given, we default to zero camera (main camera)
                uint32_t cameraIndex = 0u;
                if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
                    cameraIndex = dataStoreCamera->GetCameraIndex(jsonInputs_.customCameraId);
                } else if (!(jsonInputs_.customCameraName.empty())) {
                    cameraIndex = dataStoreCamera->GetCameraIndex(jsonInputs_.customCameraName);
                }
                if (cameraIndex < (uint32_t)cameras.size()) {
                    const auto& currCamera = cameras[cameraIndex];
                    auto& currDesc = descs_[idx];
                    const bool xChanged = (currCamera.renderResolution.x != currDesc.desc.width) ? true : false;
                    const bool yChanged = (currCamera.renderResolution.y != currDesc.desc.height) ? true : false;

                    if (xChanged || yChanged) {
                        currDesc.desc.width = currCamera.renderResolution.x;
                        currDesc.desc.height = currCamera.renderResolution.y;
                        // additional copy
                        GpuImageDesc desc = currDesc.desc;
                        desc.width = Math::max(
                            1u, static_cast<uint32_t>(static_cast<float>(desc.width) * currDesc.dependencySizeScale));
                        desc.height = Math::max(
                            1u, static_cast<uint32_t>(static_cast<float>(desc.height) * currDesc.dependencySizeScale));
                        // replace the handle
                        resourceHandles_[idx] = gpuResourceMgr.Create(resourceHandles_[idx], desc);
                    }
                }
            }
        }
    }
    // broadcast the resources
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        rngShareMgr.RegisterRenderNodeOutput(descs_[idx].shareName, resourceHandles_[idx].GetHandle());
    }
}

void RenderNodeCreateDefaultCameraGpuImages::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.gpuImageDescs = parserUtil.GetGpuImageDescs(jsonVal, "gpuImageDescs");
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeCreateDefaultCameraGpuImages::Create()
{
    return new RenderNodeCreateDefaultCameraGpuImages();
}

void RenderNodeCreateDefaultCameraGpuImages::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCreateDefaultCameraGpuImages*>(instance);
}
CORE3D_END_NAMESPACE()
