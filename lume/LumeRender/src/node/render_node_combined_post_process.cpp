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

#include "render_node_combined_post_process.h"

#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
void RenderNodeCombinedPostProcess::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    rnPostProcessUtil_.Init(*renderNodeContextMgr_, { true, {} });
}

void RenderNodeCombinedPostProcess::PreExecuteFrame()
{
    rnPostProcessUtil_.PreExecute({ true, {} });
}

void RenderNodeCombinedPostProcess::ExecuteFrame(IRenderCommandList& cmdList)
{
    rnPostProcessUtil_.Execute(cmdList);
}

// for plugin / factory interface
IRenderNode* RenderNodeCombinedPostProcess::Create()
{
    return new RenderNodeCombinedPostProcess();
}

void RenderNodeCombinedPostProcess::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCombinedPostProcess*>(instance);
}
RENDER_END_NAMESPACE()
