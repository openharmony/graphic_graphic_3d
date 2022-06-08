/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_IRENDER_BACKEND_NODE_H
#define API_RENDER_IRENDER_BACKEND_NODE_H

#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>

RENDER_BEGIN_NAMESPACE()
class ILowLevelDevice;
struct RenderBackendRecordingState;

/** @ingroup group_render_irenderbackendnode */
/** Base class for render backend nodes
 *
 * Inherit to create new render backend node that can be used in render node graph.
 *
 * Render backend node methods are never called by the API user.
 * Renderer backend calls the methods in correct pipeline positions automatically.
 */
class IRenderBackendNode : public IRenderNode {
public:
    IRenderBackendNode(const IRenderBackendNode&) = delete;
    IRenderBackendNode& operator=(const IRenderBackendNode&) = delete;

    virtual void ExecuteBackendFrame(
        const ILowLevelDevice& device, const RenderBackendRecordingState& recordingState) = 0;

protected:
    IRenderBackendNode() = default;
    virtual ~IRenderBackendNode() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_BACKEND_NODE_H
