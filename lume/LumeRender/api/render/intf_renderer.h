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

#ifndef API_RENDER_IRENDERER_H
#define API_RENDER_IRENDERER_H

#include <base/containers/array_view.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderDataStoreManager;
/** \addtogroup group_render_irenderer
 *  @{
 */
/** Rendering interface.
 *
 * Engine creates a single renderer.
 * 1. RenderFrame() should be called once per frame with a proper render node graphs.
 * or
 * 2 RenderDeferredFrame should be called once per frame.
 *
 * Render Node Graph:
 * Render node graph contains a definition of the rendering pipeline (render nodes that contain render passes).
 * A render node graph needs to be created before rendering can be done.
 * Render node graph can be created (and should be) created with json.
 * Json file is loaded with IRenderNodeGraphLoader and stored to RenderNodeGraphManager.
 * Render Data Store:
 * Render data store contains multiple managers that can store data that is accessable in render nodes.
 *
 * NOTE: render node graphs need to be unique per frame
 * The same render node graph cannot be send to rendering multiple times in a frame
 */
class IRenderer {
public:
    IRenderer() = default;
    virtual ~IRenderer() = default;

    IRenderer(const IRenderer&) = delete;
    IRenderer& operator=(const IRenderer&) = delete;

    /** Render render node graphs. Should be called once a frame. Preferred method to call RenderFrame.
     * Render node graphs are run in input order.
     * @param renderNodeGraphs Multiple render node graph handles.
     * @return Frame index of the currently rendered frame when the method returns.
     */
    virtual uint64_t RenderFrame(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) = 0;

    /** Render deferred can be called multiple times a frame.
     * Creates a list of deferred render node graphs which will be rendered in order when calling
     * Can be called from multiple threads.
     * @param renderNodeGraphs Multiple render node graph handles.
     * @return Frame index of next frame where these will be processed.
     */
    virtual uint64_t RenderDeferred(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) = 0;

    /** Render deferred render node graphs. Should be called once a frame.
     * Renders deferred render node graphs in input order.
     * @return Frame index of the currently rendered frame when the method returns.
     */
    virtual uint64_t RenderDeferredFrame() = 0;

    /** Render frame backend info.
     */
    struct RenderFrameBackendInfo {};
    /** Execute current frame backend.
     * Only valid if RenderContext created with SEPARATE_RENDER_FRAME_BACKEND.
     * Needs to be called after RenderFrame or RenderDeferredFrame.
     * @return Frame index of the currently rendered backend frame when the method returns.
     */
    virtual uint64_t RenderFrameBackend(const RenderFrameBackendInfo& info) = 0;

    /** Render frame presentation info.
     */
    struct RenderFramePresentInfo {};
    /** Execute current frame presentation.
     * Only valid if RenderContext created with SEPARATE_RENDER_FRAME_PRESENT.
     * Needs to be called after RenderFrame or RenderDeferredFrame.
     * Needs to be called after RenderFrameBackend if RenderContext created with SEPARATE_RENDER_FRAME_BACKEND.
     * @return Frame index of the currently rendered present frame when the method returns.
     */
    virtual uint64_t RenderFramePresent(const RenderFramePresentInfo& info) = 0;

    /** Render frame status.
     * Provides information of frame counts when different parts are processed.
     * 1. If using: SEPARATE_RENDER_FRAME_PRESENT
     * -> frontEndIndex and backEndIndex change before RenderFramePresent()
     * 2. If using SEPARATE_RENDER_FRAME_BACKEND
     * -> frontEndIndex changes before RenderFrameBackend()
     * 3. Default
     * -> all indices change when RenderFrame() is fully processed
     * When the first frame is rendered the index is 1. (i.e. the actual number is count of rendered frames)
     * The number starts from zero if no frames are rendered yet.
     */
    struct RenderStatus {
        /** Processed front-end frame index (count of frames) */
        uint64_t frontEndIndex { 0ULL };
        /** Processed backend-end frame index (count of frames) */
        uint64_t backEndIndex { 0ULL };
        /** Processed present-end frame index (count of frames) */
        uint64_t presentIndex { 0ULL };
    };
    /** Get status of rendering.
     * @return RenderStatus
     */
    virtual RenderStatus GetFrameStatus() const = 0;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDERER_H
