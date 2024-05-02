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
#ifndef SCENEPLUGIN_INTF_CAMERA_H
#define SCENEPLUGIN_INTF_CAMERA_H

#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/intf_postprocess.h>

#include <base/containers/vector.h>

#include <meta/api/animation/animation.h>
#include <meta/base/types.h>
#include <meta/interface/intf_container.h>

#include <scene_plugin/interface/compatibility.h>

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::Camera
REGISTER_INTERFACE(ICamera, "cf283e39-40c6-4838-993b-af8f47899150")
class ICamera : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICamera, InterfaceId::ICamera)
public:
    /**
     * @brief Field of view of the camera (perspective only).
     * @return
     */
    META_PROPERTY(float, FoV)
    /**
     * @brief Aspect ratio of the camera (perspective only).
     *  If aspect is 0 or less the aspect ratio of the canvas should be used.
     * @return
     */
    META_PROPERTY(float, AspectRatio)
    /**
     * @brief Near distance.
     * @return
     */
    META_PROPERTY(float, NearPlane)

    /**
     * @brief Far distance.
     * @return
     */
    META_PROPERTY(float, FarPlane)

    /**
     * @brief Viewport horizontal scale (orthographic projection).
     * @return
     */
    META_PROPERTY(float, XMagnification)

    /**
     * @brief Viewport vertical scale (orthographic projection).
     * @return
     */
    META_PROPERTY(float, YMagnification)

    /**
     * @brief Viewport horizontal offset (frustum projection).
     * @return
     */
    META_PROPERTY(float, XOffset)

    /**
     * @brief Viewport vertical offset (frustum projection).
     * @return
     */
    META_PROPERTY(float, YOffset)

    /**
     * @brief The SceneCameraProjection enum
     */
    enum SceneCameraProjection {
        /* Orthographic camera */
        SCENE_CAM_PROJECTION_ORTHOGRAPHIC = 0,
        /* Perspective camera */
        SCENE_CAM_PROJECTION_PERSPECTIVE = 1,
        /* Perspective camera */
        SCENE_CAM_PROJECTION_FRUSTUM = 2
    };

    /**
     * @brief Projection type of the camera. SCENE_CAM_PROJECTION_PERSPECTIVE is used by default.
     * @return
     */
    META_PROPERTY(uint8_t, Projection)

    /**
     * @brief The SceneCameraCulling enum
     */
    enum SceneCameraCulling {
        SCENE_CAM_CULLING_NONE = 0,
        /* Basic view frustum cull for objects */
        SCENE_CAM_CULLING_VIEW_FRUSTUM = 1
    };

    /**
     * @brief Culling mode for the camera.
     * @return
     */
    META_PROPERTY(uint8_t, Culling)

    /**
     * @brief The SceneCameraPipeline enum
     */
    enum SceneCameraPipeline {
        /* Light weight forward pipeline. Renders directly to back buffer */
        SCENE_CAM_PIPELINE_LIGHT_FORWARD = 0,
        /* Default forward pipeline */
        SCENE_CAM_PIPELINE_FORWARD = 1,
        /* Deferred pipeline */
        SCENE_CAM_PIPELINE_DEFERRED = 2,
        /* Custom rendering pipeline */
        SCENE_CAM_PIPELINE_CUSTOM = 3,
    };

    /**
     * @brief  Default camera rendering pipeline. Defaults to SCENE_CAM_PIPELINE_FORWARD
     * @return
     */
    META_PROPERTY(uint8_t, RenderingPipeline)

    /**
     * @brief Scene flags.
     * @return
     */
    META_PROPERTY(uint32_t, SceneFlags)

    /**
     * @brief Render pipeline flags.
     * @return
     */
    META_PROPERTY(uint32_t, PipelineFlags)

    /**
     * @brief Viewport position and size in normalized render resolution: [0] = x, [1] = y, [2] = relative width [3] =
     * relative height. (NOTE: relative width/height does not remove the offset)
     * @return
     */
    META_PROPERTY(BASE_NS::Math::Vec4, Viewport)

    /**
     * @brief Scissor offset and size in normalized render resolution: [0] = x, [1] = y, [2] = relative width [3] =
     * relative height. (NOTE: relative width/height does not remove the offset)
     * @return
     */
    META_PROPERTY(BASE_NS::Math::Vec4, Scissor)

    /**
     * @brief Render resolution of the camera: [0] = width in pixels, [1] = height in pixels.
     * @return
     */
    META_PROPERTY(BASE_NS::Math::UVec2, RenderTargetSize)

    /**
     * @brief Clear color value. Clears the color buffer(s) if clearFlags set.
     * @return
     */
    META_PROPERTY(SCENE_NS::Color, ClearColor)

    /**
     * @brief Clear depth value. Clears the depth buffer(s) if clearFlags set.
     * @return
     */
    META_PROPERTY(float, ClearDepth)

    /**
     * @brief Custom camera render node graph file. (Can be patched with e.g. post process ids etc.)
     * Chosen only if no explicit customSceneRenderNodeGraph.
     * @return
     */
    META_PROPERTY(BASE_NS::string, RenderNodeGraphFile)

    /**
     * @brief Camera post process configuration.
     * @return
     */
    META_PROPERTY(IPostProcess::Ptr, PostProcess)

    /**
     * @brief Set the given camera as multi view camera to this instance. Camera will be rendered only as a part of
     * this camera rendering. Will hold a strong reference to the given camera. Same instance can be added and removed
     * multiple times.
     * @param camera Camera to render on as a part of this camera.
     */
    virtual void AddMultiviewCamera(ICamera::Ptr camera) = 0;

    /**
     * @brief Remove the given camera as multi view camera from this instance. Camera flag for multiview rendering is
     * restored. Same instance can be added and removed multiple times.
     */
    virtual void RemoveMultiviewCamera(const ICamera::Ptr& camera) = 0;

    /** Projects normalized screen coordinates to world coordinates. Origin is at the top left corner. Z-value of the
     * screen coordinate is interpreted so that 0.0 maps to the near plane and 1.0 to the far plane.
     * @param screenCoordinate Normalized screen coordinates.
     * @return Projected world coordinates.
     */
    virtual IPickingResult::Ptr ScreenToWorld(BASE_NS::Math::Vec3 screenCoordinate) const = 0;

    /** Projects world coordinates to normalized screen coordinates.
     * @param worldCoordinate World coordinates to be projected.
     * @return Projected screen coordinates.
     */
    virtual IPickingResult::Ptr WorldToScreen(BASE_NS::Math::Vec3 worldCoordinate) const = 0;

    /**
     * Get all nodes hit by ray using a camera and 2D screen coordinates as input.
     * @param screenPos screen coordinates for hit test. Where (0, 0) is the upper left corner of the screen and (1, 1)
     * the lower left corner.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual IRayCastResult::Ptr RayCastFromCamera(const BASE_NS::Math::Vec2& screenPos) const = 0;

    /**
     * Get nodes hit by ray using this camera and 2D screen coordinates as input. Only nodes having the given layer
     * mask are in the result.
     * @param screenPos screen coordinates for hit test. Where (0, 0) is the upper left corner of the screen and (1, 1)
     * the lower left corner.
     * @param layerMask Layer mask for limiting the returned result.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual IRayCastResult::Ptr RayCastFromCamera(const BASE_NS::Math::Vec2& screenPos, uint64_t layerMask) const = 0;

    /** Resolve origin and direction of the ray
     * @param screenPos Screen coordinates to be projected.
     * @return On success, the first entry on the array tells the ray origin, and second one tells the normalized
     * direction
     */
    virtual IPickingResult::Ptr RayFromCamera(const BASE_NS::Math::Vec2& screenPos) const = 0;

    virtual void SetDefaultRenderTargetSize(uint64_t width, uint64_t height) = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ICamera::WeakPtr);
META_TYPE(SCENE_NS::ICamera::Ptr);

#endif // SCENEPLUGIN_INTF_CAMERA_H
