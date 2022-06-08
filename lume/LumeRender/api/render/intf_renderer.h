/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
     */
    virtual void RenderFrame(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) = 0;

    /** Render deferred can be called multiple times a frame.
     * Creates a list of deferred render node graphs which will be rendered in order when calling
     * Can be called from multiple threads.
     * @param renderNodeGraphs Multiple render node graph handles.
     */
    virtual void RenderDeferred(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) = 0;

    /** Render deferred render node graphs. Should be called once a frame.
     * Renders deferred render node graphs in input order.
     */
    virtual void RenderDeferredFrame() = 0;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDERER_H
