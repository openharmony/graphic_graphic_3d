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

#include "core_render_node_factory.h"

#include <render/intf_plugin.h>
#include <render/namespace.h>

#include "node/render_node_back_buffer.h"
#include "node/render_node_bloom.h"
#include "node/render_node_combined_post_process.h"
#include "node/render_node_compute_generic.h"
#include "node/render_node_create_gpu_buffers.h"
#include "node/render_node_create_gpu_images.h"
#include "node/render_node_default_acceleration_structure_staging.h"
#include "node/render_node_end_frame_staging.h"
#include "node/render_node_fullscreen_generic.h"
#include "node/render_node_mip_chain_post_process.h"
#include "node/render_node_shader_passes_generic.h"
#include "node/render_node_single_post_process.h"
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
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeBloom>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeCombinedPostProcess>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeComputeGeneric>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeCreateGpuBuffers>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeCreateGpuImages>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeEndFrameStaging>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeFullscreenGeneric>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeShaderPassesGeneric>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeMipChainPostProcess>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeSinglePostProcess>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeStaging>());
    renderNodeManager.AddRenderNodeFactory(FillRenderNodeType<RenderNodeDefaultAccelerationStructureStaging>());
}
RENDER_END_NAMESPACE()
