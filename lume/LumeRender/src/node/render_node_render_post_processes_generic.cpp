/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "render_node_render_post_processes_generic.h"

#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
void RenderNodeRenderPostProcessesGeneric::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    ppInterfaceUtil_.Init(renderNodeContextMgr);
}

void RenderNodeRenderPostProcessesGeneric::PreExecuteFrame()
{
    ppInterfaceUtil_.PreExecuteFrame();
}

IRenderNode::ExecuteFlags RenderNodeRenderPostProcessesGeneric::GetExecuteFlags() const
{
    return ppInterfaceUtil_.GetExecuteFlags();
}

void RenderNodeRenderPostProcessesGeneric::ExecuteFrame(IRenderCommandList& cmdList)
{
    ppInterfaceUtil_.ExecuteFrame(cmdList);
}

// for plugin / factory interface
IRenderNode* RenderNodeRenderPostProcessesGeneric::Create()
{
    return new RenderNodeRenderPostProcessesGeneric();
}

void RenderNodeRenderPostProcessesGeneric::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeRenderPostProcessesGeneric*>(instance);
}
RENDER_END_NAMESPACE()
