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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_CREATE_GPU_IMAGES_H
#define RENDER_RENDER__NODE__RENDER_NODE_CREATE_GPU_IMAGES_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
struct RenderNodeGraphInputs;

class RenderNodeCreateGpuImages final : public IRenderNode {
public:
    RenderNodeCreateGpuImages() = default;
    ~RenderNodeCreateGpuImages() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override {};
    ExecuteFlags GetExecuteFlags() const override
    {
        // no work in execute
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "9942031e-c80c-4d38-ae08-65555592b4df" };
    static constexpr char const* TYPE_NAME = "RenderNodeCreateGpuImages";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct DependencyList {
        bool format { false };
        bool size { false };
        bool mipCount { false };
        bool layerCount { false };
        bool sampleCount { false };
        float sizeScale { 1.0f };
    };

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();
    Size2D GetClampedShadingRateTexelSize(const Size2D& shadingRateTexelSize);

    struct JsonInputs {
        BASE_NS::vector<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> gpuImageDescs;
    };
    JsonInputs jsonInputs_;

    struct Names {
        BASE_NS::string globalName;
        BASE_NS::string shareName;
    };
    BASE_NS::vector<Names> names_;
    BASE_NS::vector<GpuImageDesc> descs_;
    BASE_NS::vector<RenderHandleReference> resourceHandles_;

    BASE_NS::vector<RenderHandle> dependencyHandles_;
    BASE_NS::vector<DependencyList> dependencyList_;
    BASE_NS::vector<Size2D> shadingRateTexelSizes_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_CREATE_GPU_IMAGES_H
