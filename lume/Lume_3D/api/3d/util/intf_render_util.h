/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_3D_UTIL_RENDER_UTIL_H
#define API_3D_UTIL_RENDER_UTIL_H

#include <3d/namespace.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

CORE3D_BEGIN_NAMESPACE()
struct RenderCamera;
struct RenderScene;
/** @ingroup group_util_irenderutil
 * @{
 */
/** Interface for helper class to to create 3D rendering related objects etc.
 */
class IRenderUtil {
public:
    /** Create camera render node graph desc.
     * Camera ids are combined to render node names with render scene name.
     * If multiple ECS are used the render scene data store name needs to be unique.
     * @param renderScene Render camera.
     * @param renderCamera Render camera.
     * @param flags Reserved for later use.
     * @return RenderNodeGraphDesc for a given render camera.
     */
    virtual RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const RenderCamera& renderCamera, const uint32_t flags) const = 0;

    /** Create scene render node graph desc. Render scene naming will be used for uniqueness.
     * @param renderScene Render scene.
     * @param flags Reserved for later use.
     * @return RenderNodeGraphDesc for a given render scene.
     */
    virtual RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const uint32_t flags) const = 0;

protected:
    IRenderUtil() = default;
    virtual ~IRenderUtil() = default;
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_UTIL_RENDER_UTIL_H
