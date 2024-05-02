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
    /** Execute flagbits from HasExecuteFrameWork().
     * There's room to add more flags for e.g. type of work (or to not parallelize by itself)
     */
    enum ExecuteFlagBits : uint32_t {
        /** Default behaviour */
        EXECUTE_FLAG_BITS_DEFAULT = 0,
        /** Do not execute render frame */
        EXECUTE_FLAG_BITS_DO_NOT_EXECUTE = (1 << 0),
    };
    using ExecuteFlags = uint32_t;

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

    /** Parallel, called every frame after every PreExecuteFrame.
     * Is not run if HasExecuteFrameWork() returns false.
     * Do NOT create gpu resources here.
     * @param cmdList Render command list for rendering/compute calls.
     */
    virtual void ExecuteFrame(IRenderCommandList& cmdList) = 0;

    /** Called every frame before ExecuteFrame.
     * @param ExecuteFlags Execute flags information for ExecuteFrame run.
     */
    virtual ExecuteFlags GetExecuteFlags() const = 0;

protected:
    IRenderNode() = default;
    virtual ~IRenderNode() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_H
