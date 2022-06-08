/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "core_render_node_factory.h"

#include <render/intf_plugin.h>
#include <render/namespace.h>

#include "node/render_node_back_buffer.h"
#include "node/render_node_back_buffer_gpu_buffer.h"
#include "node/render_node_bloom.h"
#include "node/render_node_combined_post_process.h"
#include "node/render_node_compute_generic.h"
#include "node/render_node_create_gpu_images.h"
#include "node/render_node_default_acceleration_structure_staging.h"
#include "node/render_node_fullscreen_generic.h"
#include "node/render_node_staging.h"
#include "nodecontext/render_node_manager.h"

using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
template<class RenderNodeType>
RenderNodeTypeInfo FillRenderNodeType()
{
    return { { RenderNodeTypeInfo::UID }, RenderNodeType::UID, RenderNodeType::TYPE_NAME, RenderNodeType::Create,
        RenderNodeType::Destroy, RenderNodeType::BACKEND_FLAGS, RenderNodeType::CLASS_TYPE };
}

void RegisterCoreRenderNodes(RenderNodeManager& renderNodeManager)
{
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeBackBuffer>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeBackBufferGpuBuffer>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeBloom>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeCombinedPostProcess>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeComputeGeneric>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeCreateGpuImages>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeFullscreenGeneric>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeStaging>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeDefaultAccelerationStructureStaging>());
}
RENDER_END_NAMESPACE()
