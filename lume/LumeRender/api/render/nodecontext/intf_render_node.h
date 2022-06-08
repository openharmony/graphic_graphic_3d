/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_IRENDER_NODE_H
#define API_RENDER_IRENDER_NODE_H

#include <base/containers/string_view.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
struct RenderNodeGraphInputs;

/** @ingroup group_render_irendernode */
/** Base class for render nodes
 *
 * Inherit to create new render node that can be used in render node graph.
 *
 * Render node methods are never called by the API user.
 * Renderer calls the methods in correct pipeline positions automatically.
 */
class IRenderNode {
public:
    /** Render node class type. */
    enum ClassType : uint32_t {
        /** Basic render node (IRenderNode). */
        CLASS_TYPE_NODE = 0,
        /** Render node with low-level backend access (IRenderBackendNode). */
        CLASS_TYPE_BACKEND_NODE = 1,
    };
    /** Render node backend support flags.
     * With typical render nodes, support is default (all backends).
     * With backend render nodes, explicit selection should be done.
     */
    enum BackendFlagBits : uint32_t {
        /** Explicit default zero. Supports all backends */
        BACKEND_FLAG_BITS_DEFAULT = 0,
        /** Explicit support for Vulkan */
        BACKEND_FLAG_BITS_EXPLICIT_VULKAN = (1 << 0),
        /** Explicit support for GLES */
        BACKEND_FLAG_BITS_EXPLICIT_GLES = (1 << 1),
        /** Explicit support for GL */
        BACKEND_FLAG_BITS_EXPLICIT_GL = (1 << 2),
    };
    using BackendFlags = uint32_t;

    IRenderNode(const IRenderNode&) = delete;
    IRenderNode& operator=(const IRenderNode&) = delete;

    /** Sequential, called once after render graph has been initialized.
     * Anything can be done here, including gpu resource creation.
     * Note: InitNode can get called multiple times during runtime e.g. after hot-reloading assets. The node must
     * invalidate any changed state/ handles and assume it starts from scratch.
     * @param renderNodeContextMgr Provides access to needed managers.
     */
    virtual void InitNode(IRenderNodeContextManager& renderNodeContextMgr) = 0;

    /** Sequential, called before ExecuteFrame every frame.
     * Create/destroy gpu resources here if needed. Prefer not doing any other work.
     * IRenderNodeGpuResourceManager keeps track of created resources
     * -> do not explicitly destroy if not needed.
     */
    virtual void PreExecuteFrame() = 0;

    /** Parallel, called every frame after every ExecuteCreateGpuResources().
     * Do NOT create gpu resources here.
     * @param cmdList Render command list for rendering/compute calls.
     */
    virtual void ExecuteFrame(IRenderCommandList& cmdList) = 0;

protected:
    IRenderNode() = default;
    virtual ~IRenderNode() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_H
