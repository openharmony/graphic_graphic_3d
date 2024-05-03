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
