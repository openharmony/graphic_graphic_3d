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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_BACK_BUFFER_GPU_BUFFER_H
#define RENDER_RENDER__NODE__RENDER_NODE_BACK_BUFFER_GPU_BUFFER_H

#include <base/util/uid.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IGpuResourceManager;
class IRenderCommandList;
class IRenderNodeContextManager;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderNodeBackBufferGpuBuffer final : public IRenderNode {
public:
    RenderNodeBackBufferGpuBuffer() = default;
    ~RenderNodeBackBufferGpuBuffer() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override {};
    void ExecuteFrame(IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "ee4ae160-8b7f-4043-8c86-2510f48b4e3b" };
    static constexpr char const* TYPE_NAME = "CORE_RN_BACKBUFFER_GPUBUFFER";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_BACK_BUFFER_GPU_BUFFER_H
