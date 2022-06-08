/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_RENDER__NODE__RENDER_NODE_STAGING_H
#define RENDER_RENDER__NODE__RENDER_NODE_STAGING_H

#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

#include "device/gpu_resource_manager.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
class IRenderNodeGpuResourceManager;
struct RenderNodeGraphInputs;
struct StagingDirectDataCopyConsumeStruct;

class RenderNodeStaging final : public IRenderNode {
public:
    RenderNodeStaging() = default;
    ~RenderNodeStaging() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "22a56820-befb-4e12-83ee-3cad3f685b0a" };
    static constexpr char const* TYPE_NAME = "CORE_RN_STAGING";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void CopyHostToStaging(
        const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingConsumeStruct& stagingData);
    void CopyStagingToImages(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
        const StagingConsumeStruct& stagingData, const StagingConsumeStruct& renderDataStoreStagingData);
    void CopyStagingToBuffers(IRenderCommandList& cmdList, const StagingConsumeStruct& stagingData,
        const StagingConsumeStruct& renderDataStoreStagingData);
    void CopyImagesToBuffers(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
        const StagingConsumeStruct& stagingData, const StagingConsumeStruct& renderDataStoreStagingData);
    void CopyImagesToImages(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
        const StagingConsumeStruct& stagingData, const StagingConsumeStruct& renderDataStoreStagingData);

    BASE_NS::string dataStoreNameStaging_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_STAGING_H
