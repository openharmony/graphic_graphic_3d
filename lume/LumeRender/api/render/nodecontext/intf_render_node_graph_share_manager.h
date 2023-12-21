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

#ifndef API_RENDER_IRENDER_NODE_GRAPH_SHARE_MANAGER_H
#define API_RENDER_IRENDER_NODE_GRAPH_SHARE_MANAGER_H

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_interface.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_IRenderNodeGraphShareManager */
/**
 * Share input / output and other resources with current render node graph.
 * Resources might change during the run and new RenderNodes might come in between.
 *
 * Methods with PreExecuteFrame -prefix are only valid to be called in RenderNode::PreExecuteFrame().
 * Methods with ExecuteFrame -prefix are only valid to be called in RenderNode::ExecuteFrame().
 */
class IRenderNodeGraphShareManager : public IRenderNodeInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("1c573502-0e25-481b-bf20-b1d1f09af37f");

    /** Get render node graph input resource handles.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphInputs() const = 0;

    /** Get render node graph output resource handles.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphOutputs() const = 0;

    /** Get render node graph input resource handle with index.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual RenderHandle GetRenderNodeGraphInput(const uint32_t index) const = 0;

    /** Get render node graph output resource handle with index.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual RenderHandle GetRenderNodeGraphOutput(const uint32_t index) const = 0;

    /** Register render node graph output resource handles. (I.e. the output of this render node graph)
     * Often these should be set from the outside, but a controller render node can handle this as well.
     * Should be called every frame in PreExecuteFrame() (and initially in InitNode()).
     * @param outputHandles Output handles to be registered.
     */
    virtual void RegisterRenderNodeGraphOutputs(const BASE_NS::array_view<const RenderHandle> outputs) = 0;

    /** Register render node output resource handles. (I.e. the output of this render node)
     * Should be called every frame in PreExecuteFrame() (and initially in InitNode()).
     * @param outputs Output handles to be registered.
     */
    virtual void RegisterRenderNodeOutputs(const BASE_NS::array_view<const RenderHandle> outputs) = 0;

    /** Register render node output one-by-one.
     * Should be called every frame in PreExecuteFrame() (and initially in InitNode()).
     * @param name Named output handle to be registered.
     * @param handle Output handle to be registered.
     */
    virtual void RegisterRenderNodeOutput(const BASE_NS::string_view name, const RenderHandle& handle) = 0;

    /** Get registered output from a named render node with index.
     */
    virtual RenderHandle GetRegisteredRenderNodeOutput(
        const BASE_NS::string_view renderNodeName, const uint32_t index) const = 0;

    /** Get registered output from a named render node with name.
     * @param renderNodeName Render node name of the output.
     * @param resourceName Name of the output resource.
     */
    virtual RenderHandle GetRegisteredRenderNodeOutput(
        const BASE_NS::string_view renderNodeName, const BASE_NS::string_view resourceName) const = 0;

    /** Get registered output from previous render node with index.
     * @param index Index of the output.
     */
    virtual RenderHandle GetRegisteredPrevRenderNodeOutput(const uint32_t index) const = 0;

protected:
    IRenderNodeGraphShareManager() = default;
    virtual ~IRenderNodeGraphShareManager() = default;

    IRenderNodeGraphShareManager(const IRenderNodeGraphShareManager&) = delete;
    IRenderNodeGraphShareManager& operator=(const IRenderNodeGraphShareManager&) = delete;
    IRenderNodeGraphShareManager(IRenderNodeGraphShareManager&&) = delete;
    IRenderNodeGraphShareManager& operator=(IRenderNodeGraphShareManager&&) = delete;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_GRAPH_SHARE_H
