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

#ifndef API_3D_UTIL_RENDER_UTIL_H
#define API_3D_UTIL_RENDER_UTIL_H

#include <3d/namespace.h>
#include <3d/render/render_data_defines_3d.h>
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

    /* Camera render node graph descs */
    struct CameraRenderNodeGraphDescs {
        /* Camera render node graph desc */
        RENDER_NS::RenderNodeGraphDesc camera;
        /* Camera based post process render node graph desc */
        RENDER_NS::RenderNodeGraphDesc postProcess;
        /* Multi-view camera count */
        uint32_t multiViewCameraCount { 0U };
        /* Multi-view camera post processes */
        RENDER_NS::RenderNodeGraphDesc
            multiViewCameraPostProcesses[RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT];
    };
    /** Create camera render node graph descs. (Create desc for camera and post process if any)
     * Camera ids are combined to render node names with render scene name.
     * If multiple ECS are used the render scene data store name needs to be unique.
     * @param renderScene Render camera.
     * @param renderCamera Render camera.
     * @param flags Reserved for later use.
     * @return RenderNodeGraphDescs for a given render camera.
     */
    virtual CameraRenderNodeGraphDescs GetRenderNodeGraphDescs(
        const RenderScene& renderScene, const RenderCamera& renderCamera, const uint32_t flags) const = 0;

    /** Create camera render node graph descs. (Create desc for camera and post process if any)
     * Camera ids are combined to render node names with render scene name.
     * If multiple ECS are used the render scene data store name needs to be unique.
     * Processes multi-view cameras
     * @param renderScene Render camera.
     * @param renderCamera Render camera.
     * @param flags Reserved for later use.
     * @param multiviewCameras when multi-view enabled in main camera.
     * @return RenderNodeGraphDescs for a given render camera.
     */
    virtual CameraRenderNodeGraphDescs GetRenderNodeGraphDescs(const RenderScene& renderScene,
        const RenderCamera& renderCamera, const uint32_t flags,
        const BASE_NS::array_view<const RenderCamera> multiviewCameras) const = 0;

    /** Create scene render node graph desc. Render scene naming will be used for uniqueness.
     * @param renderScene Render scene.
     * @param flags Reserved for later use.
     * @return RenderNodeGraphDesc for a given render scene.
     */
    virtual RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const uint32_t flags) const = 0;

    /** Create scene render node graph desc. Render scene naming will be used for uniqueness.
     * @param renderScene Render scene.
     * @param rngFile Custom rng file which will be patched with scene ids.
     * @param flags Reserved for later use.
     * @return RenderNodeGraphDesc for a given render scene.
     */
    virtual RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const BASE_NS::string& rngFile, const uint32_t flags) const = 0;

protected:
    IRenderUtil() = default;
    virtual ~IRenderUtil() = default;
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_UTIL_RENDER_UTIL_H
