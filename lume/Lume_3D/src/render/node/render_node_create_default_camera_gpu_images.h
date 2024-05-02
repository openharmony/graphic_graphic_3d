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

#ifndef CORE__RENDER__NODE__RENDER_NODE_CREATE_DEFAULT_CAMERA_GPU_IMAGES_H
#define CORE__RENDER__NODE__RENDER_NODE_CREATE_DEFAULT_CAMERA_GPU_IMAGES_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/device/gpu_resource_desc.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class RenderNodeCreateDefaultCameraGpuImages final : public RENDER_NS::IRenderNode {
public:
    RenderNodeCreateDefaultCameraGpuImages() = default;
    ~RenderNodeCreateDefaultCameraGpuImages() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override {};
    ExecuteFlags GetExecuteFlags() const override
    {
        // no work in execute
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "c694a5bd-c6f6-4167-9b61-cf481632c171" };
    static constexpr char const* TYPE_NAME = "RenderNodeCreateDefaultCameraGpuImages";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    struct JsonInputs {
        BASE_NS::vector<RENDER_NS::RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> gpuImageDescs;

        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
    };
    JsonInputs jsonInputs_;

    SceneRenderDataStores stores_;

    BASE_NS::vector<RENDER_NS::RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> descs_;
    BASE_NS::vector<RENDER_NS::RenderHandleReference> resourceHandles_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_CREATE_DEFAULT_CAMERA_GPU_IMAGES_H
