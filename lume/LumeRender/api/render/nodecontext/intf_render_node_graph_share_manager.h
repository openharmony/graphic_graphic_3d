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

#ifndef API_RENDER_IRENDER_NODE_GRAPH_SHARE_MANAGER_H
#define API_RENDER_IRENDER_NODE_GRAPH_SHARE_MANAGER_H

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_IRenderNodeGraphShareManager */
/**
 * Share input / output and other resources with current render node graph.
 * Resources might change during the run and new RenderNodes might come in between.
 * Render node graph can access the previous render node graph outputs.
 *
 * All the registering needs to be happen in RenderNode::PreExecuteFrame().
 * Not internally synchronized for data modifications.
 */
class IRenderNodeGraphShareManager : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("eef71d88-ddc0-41b8-b593-ac23c44ac086");

    /** Use simple resource names like: color, depth, velocity, alpha... */
    static constexpr uint32_t MAX_NAME_LENGTH { 32u };
    using RenderNodeGraphShareName = BASE_NS::fixed_string<MAX_NAME_LENGTH>;
    /** Named resources struct */
    struct NamedResource {
        /** Name of the resource (simple unique within these few resources) */
        RenderNodeGraphShareName name;
        /** Resource handle */
        RenderHandle handle;
    };

    /** Get render node graph input resource handles.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphInputs() const = 0;

    /** Get named render node graph input resource handles. Empty names if not set.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual BASE_NS::array_view<const NamedResource> GetNamedRenderNodeGraphInputs() const = 0;

    /** Get render node graph output resource handles.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphOutputs() const = 0;

    /** Get named render node graph output resource handles. Empty names if not set.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual BASE_NS::array_view<const NamedResource> GetNamedRenderNodeGraphOutputs() const = 0;

    /** Get render node graph input resource handle with index.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual RenderHandle GetRenderNodeGraphInput(const uint32_t index) const = 0;

    /** Get render node graph output resource handle with index.
     * Set globally through render node graph manager or in render node graph description.
     */
    virtual RenderHandle GetRenderNodeGraphOutput(const uint32_t index) const = 0;

    /** Get outputs from previous render node graph.
     * @return Array view of resource handles.
     */
    virtual BASE_NS::array_view<const RenderHandle> GetPrevRenderNodeGraphOutputs() const = 0;

    /** Get named outputs from previous render node graph.
     * @return Array view of named resources.
     */
    virtual BASE_NS::array_view<const NamedResource> GetNamedPrevRenderNodeGraphOutputs() const = 0;

    /** Get resource render handle with index from previous render node graph.
     * @param index Index of the registered resource.
     * @return Resource handle, invalid if not found.
     */
    virtual RenderHandle GetPrevRenderNodeGraphOutput(const uint32_t index) const = 0;

    /** Get resource render handle with name from previous render node graph.
     * @param resourceName Simple name of the registered resource.
     * @return Resource handle, invalid if not found.
     */
    virtual RenderHandle GetNamedPrevRenderNodeGraphOutput(const BASE_NS::string_view resourceName) const = 0;

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

    /** Register global render node output one-by-one. One should only register specific outputs globally.
     * Should be called every frame in PreExecuteFrame() (and initially in InitNode()).
     * @param name Named output handle to be registered.
     * @param handle Output handle to be registered.
     */
    virtual void RegisterGlobalRenderNodeOutput(const BASE_NS::string_view name, const RenderHandle& handle) = 0;

    /** Get globally registered render node outputs. The render node name is the unique / full render node name.
     * Should be called every frame in PreExecuteFrame() (and initially in InitNode()).
     * @param nodeName Global (and unique) render node name.
     * @return Globally registered render node outputs.
     */
    virtual BASE_NS::array_view<const NamedResource> GetRegisteredGlobalRenderNodeOutputs(
        const BASE_NS::string_view nodeName) const = 0;

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
