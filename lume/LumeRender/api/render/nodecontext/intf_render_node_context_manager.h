/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_IRENDER_NODE_CONTEXT_MANAGER_H
#define API_RENDER_IRENDER_NODE_CONTEXT_MANAGER_H

#include <base/util/uid.h>
#include <core/json/json.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_interface.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class INodeContextDescriptorSetManager;
class INodeContextPsoManager;
class IRenderNodeRenderDataStoreManager;
class IRenderNodeUtil;
class IRenderNodeShaderManager;
class IRenderNodeGpuResourceManager;
class IRenderNodeGraphShareManager;
class IRenderNodeParserUtil;

/** @ingroup group_render_irendernodecontextmanager */
/**
 * Provides access to render node managers and resources.
 * In addition provides access to IRenderNodeContextInterfaces.
 */
class IRenderNodeContextManager : public IRenderNodeInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("7c7cf4c3-4acc-4609-afa6-365ce5d0031c");

    /** Get render data store manager. Global render data store manager for accessing various render data stores.
     */
    virtual const IRenderNodeRenderDataStoreManager& GetRenderDataStoreManager() const = 0;

    /** Get render node specific shader manager interface. Uses global shader manager.
     */
    virtual const IRenderNodeShaderManager& GetShaderManager() const = 0;

    /** Get render node GPU manager. Global resources, automatic destruction for render node created resources.
     */
    virtual IRenderNodeGpuResourceManager& GetGpuResourceManager() = 0;

    /** Get render node context descriptor set manager. Per render node descriptor set manager.
     */
    virtual INodeContextDescriptorSetManager& GetDescriptorSetManager() = 0;

    /** Get render node context pso manager. Per render node PSO cache.
     */
    virtual INodeContextPsoManager& GetPsoManager() = 0;

    /** Get render node graph share manager. Share data within render nodes in render node graph.
     */
    virtual IRenderNodeGraphShareManager& GetRenderNodeGraphShareManager() = 0;

    /** Get render node util helper class.
     */
    virtual const IRenderNodeUtil& GetRenderNodeUtil() const = 0;

    /** Get render node graph data
     */
    virtual const RenderNodeGraphData& GetRenderNodeGraphData() const = 0;

    /** Get render node graph parser
     */
    virtual const IRenderNodeParserUtil& GetRenderNodeParserUtil() const = 0;

    /** Get render node full (context) name.
     * Combined with render node graph name and should be unique.
     * (One should add name to render node graph if the same render node graph can be used many times.)
     */
    virtual BASE_NS::string_view GetName() const = 0;

    /** Render node name given in render node graph.
     * Is unique within the render node graph. (Do not use for naming global resources)
     */
    virtual BASE_NS::string_view GetNodeName() const = 0;

    /** Get render node specific json block.
     */
    virtual CORE_NS::json::value GetNodeJson() const = 0;

    /** Built-in render node graph inputs
     * @return RenderNodeGraphInputs
     */
    virtual const RenderNodeGraphInputs& GetRenderNodeGraphInputs() const = 0;

    /** Get render context.
     * @return render context.
     */
    virtual IRenderContext& GetRenderContext() const = 0;

    /** Get render node context specific render node interface by UID.
     */
    virtual IRenderNodeInterface* GetInterface(const BASE_NS::Uid& uid) const = 0;

    /** Get render node context interfaces by UID with interface type cast
     */
    template<typename InterfaceType>
    InterfaceType* GetInterface()
    {
        return static_cast<InterfaceType*>(GetInterface(InterfaceType::UID));
    }

protected:
    IRenderNodeContextManager() = default;
    virtual ~IRenderNodeContextManager() = default;

    IRenderNodeContextManager(const IRenderNodeContextManager&) = delete;
    IRenderNodeContextManager& operator=(const IRenderNodeContextManager&) = delete;
    IRenderNodeContextManager(IRenderNodeContextManager&&) = delete;
    IRenderNodeContextManager& operator=(IRenderNodeContextManager&&) = delete;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_CONTEXT_MANAGER_H
