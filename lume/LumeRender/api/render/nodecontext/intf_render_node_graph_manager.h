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

#ifndef API_RENDER_IRENDER_NODE_GRAPH_MANAGER_H
#define API_RENDER_IRENDER_NODE_GRAPH_MANAGER_H

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderNodeGraphLoader;

/** \addtogroup group_render_irendernodegraphmanager
 *  @{
 */
/** Render node desctriptor */
struct RenderNodeDesc {
    /** Type name */
    RenderDataConstants::RenderDataFixedString typeName;
    /** Name of render node instance */
    RenderDataConstants::RenderDataFixedString nodeName;
    /** Description of setup data */
    RenderNodeGraphInputs description;
    /** Render node specific json string */
    BASE_NS::string nodeJson;
};

/** Render node desctriptor */
struct RenderNodeGraphOutputResource {
    /** Simple output name */
    RenderDataConstants::RenderDataFixedString name;
    /** Name of render node instance where the output is from */
    RenderDataConstants::RenderDataFixedString nodeName;
};

/** Render node graph descriptor */
struct RenderNodeGraphDesc {
    /** Render node graph name */
    RenderDataConstants::RenderDataFixedString renderNodeGraphName;
    /** Render node graph (default) data store name (can be used in various for access point) */
    RenderDataConstants::RenderDataFixedString renderNodeGraphDataStoreName;
    /** Optional uri, which can be fetched later to get path to the loaded file */
    RenderDataConstants::RenderDataFixedString renderNodeGraphUri;
    /** Render nodes */
    BASE_NS::vector<RenderNodeDesc> nodes;

    /** Render node graph output resources */
    BASE_NS::vector<RenderNodeGraphOutputResource> outputResources;
};

struct RenderNodeDescInfo {
    /** Type name */
    RenderDataConstants::RenderDataFixedString typeName;
    /** Name of render node instance */
    RenderDataConstants::RenderDataFixedString nodeName;
    /** Specific flags for node to change state (e.g. enable / disable) */
    uint32_t flags { 0u };
};

/** Render node graph descriptor */
struct RenderNodeGraphDescInfo {
    /** Render node graph name */
    RenderDataConstants::RenderDataFixedString renderNodeGraphName;
    /** Render node graph (default) data store name (can be used in various for access point) */
    RenderDataConstants::RenderDataFixedString renderNodeGraphDataStoreName;
    /** Render node graph uri (if set) */
    RenderDataConstants::RenderDataFixedString renderNodeGraphUri;
    /** Render nodes */
    BASE_NS::vector<RenderNodeDescInfo> nodes;
};

/** Render node graph input and output resources. Depending on the rendering state these might not be valid */
struct RenderNodeGraphResourceInfo {
    /** Render node input resources */
    BASE_NS::vector<RenderHandleReference> inputResources;
    /** Render node output resources */
    BASE_NS::vector<RenderHandleReference> outputResources;
};

/** Interface class to hold all render node graphs and their data.
 *
 * Internally synchronized.
 */
class IRenderNodeGraphManager {
public:
    /**
     * With dynamic render node graphs render nodes can be inserted or erased.
     */
    enum class RenderNodeGraphUsageType : uint8_t {
        RENDER_NODE_GRAPH_STATIC = 0,
        RENDER_NODE_GRAPH_DYNAMIC = 1,
    };

    IRenderNodeGraphManager(const IRenderNodeGraphManager&) = delete;
    IRenderNodeGraphManager& operator=(const IRenderNodeGraphManager&) = delete;

    /** Get render handle reference for raw handle.
     *  @param handle Raw render handle
     *  @return Returns A render handle reference for the handle.
     */
    virtual RenderHandleReference Get(const RenderHandle& handle) const = 0;

    /** Create render node graph.
     * @param usage Usage style of graphs. (Static, Dynamic)
     * @param desc Render node graph description.
     * @param renderNodeGraphName Override render node graph name.
     * @param renderNodeGraphName Override render node graph data store name (default access point data store).
     * @return Render data handle resource.
     */
    virtual RenderHandleReference Create(const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc,
        const BASE_NS::string_view renderNodeGraphName, const BASE_NS::string_view renderNodeGraphDataStoreName) = 0;

    /** Create render node graph.
     * @param usage Usage style of graphs. (Static, Dynamic)
     * @param desc Render node graph description.
     * @param renderNodeGraphName Override render node graph name.
     * @return Render data handle resource.
     */
    virtual RenderHandleReference Create(const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc,
        const BASE_NS::string_view renderNodeGraphName) = 0;

    /** Create render node graph.
     * @param usage Usage style of graphs. (Static, Dynamic)
     * @param desc Render node graph description.
     * @return Render data handle.
     */
    virtual RenderHandleReference Create(const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc) = 0;

    /** Insert new render node to dynamic render node graph to the last render node in graph.
     * @param handle Handle of a render node graph.
     * @param renderNodeDesc Description of render node.
     */
    virtual void PushBackRenderNode(const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc) = 0;

    /** Remove render node from dynamic render node graph.
     * Erase does not destroy GPU resource that have been created in the erased render node.
     * The resources are destroyed when the render node graph is destroyed.
     * @param handle Handle of a render node graph.
     * @param renderNodeName A valid render node name in this particular render node graph.
     */
    virtual void EraseRenderNode(const RenderHandleReference& handle, const BASE_NS::string_view renderNodeName) = 0;

    /** Insert new render node to dynamic render node graph to a position.
     * @param handle Handle of a render node graph.
     * @param renderNodeDesc Description of render node.
     * @pos renderNodeName Instance name of render node to insert before.
     */
    virtual void InsertBeforeRenderNode(const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc,
        const BASE_NS::string_view renderNodeName) = 0;

    /** Insert new render node to dynamic render node graph to a position.
     * @param handle Handle of a render node graph.
     * @param renderNodeDesc Description of render node.
     * @pos renderNodeName Instance name of render node to insert after.
     */
    virtual void InsertAfterRenderNode(const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc,
        const BASE_NS::string_view renderNodeName) = 0;

    /** Update status etc. with flags.
     * @param handle Handle of a render node graph.
     * @param graphDescInfo Render node graph description info with updatable flags.
     */
    virtual void UpdateRenderNodeGraph(
        const RenderHandleReference& handle, const RenderNodeGraphDescInfo& graphDescInfo) = 0;

    /** Get render node graph description info with render node types, names, and current flags
     * @param handle Handle of a render node graph.
     * @return RenderNodeGraphDescInfo New render node graph description info struct.
     */
    virtual RenderNodeGraphDescInfo GetInfo(const RenderHandleReference& handle) const = 0;

    /** Set render node graph inputs and outputs.
     * This needs to be called before RenderFrame in the frame where these are supposed to be used.
     * Overrides everything -> Need to re-set existing if called.
     * One can reset handle references by calling this with empty inputs and outputs.
     * @param handle Handle of a render node graph.
     * @param inputs Render node graph input handles.
     * @param outputs Render node graph output handles.
     */
    virtual void SetRenderNodeGraphResources(const RenderHandleReference& handle,
        const BASE_NS::array_view<const RenderHandleReference> inputs,
        const BASE_NS::array_view<const RenderHandleReference> outputs) = 0;

    /** Get render node graph resource info. Returns the handles set through SetRnederNodeGraphResources.
     * @param handle Handle of a render node graph.
     * @return RenderNodeGraphResourceInfo Render node graph resources.
     */
    virtual RenderNodeGraphResourceInfo GetRenderNodeGraphResources(
        const RenderHandleReference& handleResource) const = 0;

    /** Create render node graph directly from uri. Stores the uri for later info.
     * @param usage Usage style of graphs. (Static, Dynamic)
     * @param uri Path to render node graph json file.
     * @return Render data handle resource.
     */
    virtual RenderHandleReference LoadAndCreate(
        const RenderNodeGraphUsageType usage, const BASE_NS::string_view uri) = 0;

    /** Access to render node graph loader.
     */
    virtual IRenderNodeGraphLoader& GetRenderNodeGraphLoader() = 0;

protected:
    IRenderNodeGraphManager() = default;
    virtual ~IRenderNodeGraphManager() = default;
};

/** Interface class for creating a RenderNodeGraphDesc from a JSON file.
 */
class IRenderNodeGraphLoader {
public:
    /** Describes result of the loading operation. */
    struct LoadResult {
        LoadResult() = default;
        explicit LoadResult(const BASE_NS::string& error) : success(false), error(error) {}

        /** Indicates, whether the parsing operation is successful. */
        bool success { true };

        /** In case of parsing error, contains the description of the error. */
        BASE_NS::string error;

        /** A valid render node graph descriptor, when success is true. */
        RenderNodeGraphDesc desc;
    };

    /** Load a render node graph description from a file.
     * @param fileManager File manager to be used for file access
     * @param uri URI to a file containing the render node graph description in JSON format.
     * @return If loading fails LoadResult::success is false and LoadResult::error contains an error message.
     */
    virtual LoadResult Load(const BASE_NS::string_view uri) = 0;

    /** Load a render node graph description from a JSON string.
     * @param json String containing the render node graph description in JSON format.
     * @return If loading fails LoadResult::success is false and LoadResult::error contains an error message.
     */
    virtual LoadResult LoadString(const BASE_NS::string_view json) = 0;

protected:
    IRenderNodeGraphLoader() = default;
    virtual ~IRenderNodeGraphLoader() = default;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_GRAPH_MANAGER_H
