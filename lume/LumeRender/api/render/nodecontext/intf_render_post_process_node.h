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

#ifndef API_RENDER_NODECONTEXT_IRENDER_POST_PROCESS_NODE_H
#define API_RENDER_NODECONTEXT_IRENDER_POST_PROCESS_NODE_H

#include <base/containers/array_view.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <core/property/intf_property_handle.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()

class IRenderNodeContextManager;
class IRenderCommandList;

/** @ingroup group_render_irenderpostprocessnode */
/**
 * Provides interface for post processes which can run custom code in render nodes
 */
class IRenderPostProcessNode : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("99cb059f-186b-4c38-b75b-28b2e805d63d");

    using Ptr = BASE_NS::refcnt_ptr<IRenderPostProcessNode>;

    /** Get property handle for built-in input properties during rendering.
     * These properties should be updated every frame when executed.
     * @return Pointer to property handle if properties present, nullptr otherwise.
     */
    virtual CORE_NS::IPropertyHandle* GetRenderInputProperties() = 0;

    /** Get property handle for built-in output properties during rendering.
     * These properties are updated every frame during PreExecute and should be taken into account after that.
     * @return Pointer to property handle if properties present, nullptr otherwise.
     */
    virtual CORE_NS::IPropertyHandle* GetRenderOutputProperties() = 0;

    /** Render area request.
     * 1. If the area is default 0-0-0-0 -> check the output target size
     * 2. If no output target set from outside use this for intermediate output target
     * 3. If the are is non-default, apply to output target or create the sized output target
     */
    struct RenderAreaRequest {
        /* Render are, used as explained above */
        RENDER_NS::RenderPassDesc::RenderArea area;
    };
    /** Set render area request
     * 1. If output given, non-default values affect the area
     * 2. If no output given, creates an output intermediate target for given size.
     */
    virtual void SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest) = 0;

    /** Get descriptor counts needed for custom post process execution.
     * If shares descriptor sets with the main render node.
     * This is called and evaluated during rendering.
     * This method needs to return correct values after Init().
     * @return Descriptor counts
     */
    virtual DescriptorCounts GetRenderDescriptorCounts() const = 0;

    /** Called every frame before ExecuteFrame.
     * This should return false if the effect is disabled.
     * @param ExecuteFlags Execute flags information for ExecuteFrame run.
     */
    virtual IRenderNode::ExecuteFlags GetExecuteFlags() const = 0;

    /** Sequential, called when the actual render node is initialized or reset.
     * Note: Init can get called multiple times during runtime. The node must invalidate any changed state/ handles and
     * assume it starts from scratch. This is called and evaluated during rendering.
     * @param renderNodeContextMgr Provides access to needed managers.
     */
    virtual void InitNode(IRenderNodeContextManager& renderNodeContextMgr) = 0;

    /** Sequential, called before ExecuteFrame every frame by the actual render node.
     * Create/destroy gpu resources here if needed. Descriptor sets should be created here as well.
     * Prefer not doing any other work.
     * IRenderNodeGpuResourceManager keeps track of created resources, just store the handle references.
     * This is called and evaluated during rendering.
     */
    virtual void PreExecuteFrame() = 0;

    /** Parallel, called every frame by the actual post process handling render node.
     * Should make sure internally if the effect should be applied.
     * Do NOT create gpu resources here.
     * This is called and evaluated during rendering.
     * @param cmdList Render command list for rendering/compute calls.
     */
    virtual void ExecuteFrame(IRenderCommandList& cmdList) = 0;

protected:
    IRenderPostProcessNode() = default;
    virtual ~IRenderPostProcessNode() = default;

    IRenderPostProcessNode(const IRenderPostProcessNode&) = delete;
    IRenderPostProcessNode& operator=(const IRenderPostProcessNode&) = delete;
    IRenderPostProcessNode(IRenderPostProcessNode&&) = delete;
    IRenderPostProcessNode& operator=(IRenderPostProcessNode&&) = delete;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_NODECONTEXT_IRENDER_POST_PROCESS_NODE_H
