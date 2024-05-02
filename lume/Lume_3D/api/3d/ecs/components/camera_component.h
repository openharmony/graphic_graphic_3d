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

#if !defined(API_3D_ECS_COMPONENTS_CAMERA_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_CAMERA_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/ecs/components/layer_defines.h>
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/math/matrix.h>
#include <base/util/formats.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>
#include <render/device/gpu_resource_desc.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(ICameraComponentManager, CameraComponent)
#if !defined(IMPLEMENT_MANAGER)
    enum class Projection : uint8_t {
        /** Orthographic camera */
        ORTHOGRAPHIC = 0,
        /** Perspective camera */
        PERSPECTIVE = 1,
        /** Frustum camera */
        FRUSTUM = 2,
        /** Custom matrix provided for camera */
        CUSTOM = 3,
    };

    enum class Culling : uint8_t {
        /** None */
        NONE = 0,
        /** Basic view frustum cull for objects */
        VIEW_FRUSTUM = 1,
    };

    enum SceneFlagBits : uint32_t {
        /** Camera is rendered when it's active. */
        ACTIVE_RENDER_BIT = (1 << 0),
        /** Main camera. If multiple main cameras, the first is chosen as ECS main camera. Main camera is treated always
           as active. */
        MAIN_CAMERA_BIT = (1 << 1),
    };

    enum PipelineFlagBits : uint32_t {
        /** Target clear flags depth. Override camera render node graph loadOp with clear.
         * Without clear the default render node graph based loadOp is used. (Default pipelines use depth clear)
         */
        CLEAR_DEPTH_BIT = (1 << 0),
        /** Target clear flags color. Override camera render node graph loadOp with clear.
         * Without clear the default render node graph based loadOp is used. (Default pipelines do not use color clear)
         */
        CLEAR_COLOR_BIT = (1 << 1),
        /** Enable MSAA for rendering. Only affects non deferred default pipelines. */
        MSAA_BIT = (1 << 2),
        /** Automatically use pre-pass if there are default material needs (e.g. for transmission). Automatic RNG
           generation needs to be enabled for the ECS scene. */
        ALLOW_COLOR_PRE_PASS_BIT = (1 << 3),
        /** Force pre-pass every frame. Use for e.g. custom shaders without default material needs. Automatic RNG
           generation needs to be enabled for the ECS scene. */
        FORCE_COLOR_PRE_PASS_BIT = (1 << 4),
        /** Store history (store history for next frame, needed for e.g. temporal filtering) */
        HISTORY_BIT = (1 << 5),
        /** Jitter camera. With Halton sampling */
        JITTER_BIT = (1 << 6),
        /** Output samplable velocity / normal */
        VELOCITY_OUTPUT_BIT = (1 << 7),
        /** Output samplable depth */
        DEPTH_OUTPUT_BIT = (1 << 8),
        /** Is a multi-view camera and is not be rendered separately at all
         * The camera is added to other camera as multiViewCameras
         */
        MULTI_VIEW_ONLY_BIT = (1 << 9),
        /** Generate environment cubemap dynamically for the camera
         */
        DYNAMIC_CUBEMAP_BIT = (1 << 10),
        /** Disallow reflection plane for camera
         */
        DISALLOW_REFLECTION_BIT = (1 << 11),
    };

    /** Target customization */
    struct TargetUsage {
        /** Target format */
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
        /** Usage flags hints for optimizing resource creation */
        RENDER_NS::ImageUsageFlags usageFlags { 0 };
    };

    /** With default render node graphs one can select the pipeline */
    enum class RenderingPipeline : uint8_t {
        /** Light weight forward pipeline. Renders directly to back buffer */
        LIGHT_FORWARD = 0,
        /** Default forward pipeline */
        FORWARD = 1,
        /** Deferred pipeline */
        DEFERRED = 2,
        /** Custom rendering pipeline */
        CUSTOM = 3,
    };
#endif
    /** Projection type of the camera.
     */
    DEFINE_PROPERTY(Projection, projection, "Projection", 0, VALUE(Projection::PERSPECTIVE))

    /** Culling mode for the camera.
     */
    DEFINE_PROPERTY(Culling, culling, "Culling", 0, VALUE(Culling::VIEW_FRUSTUM))

    /** Default camera rendering pipeline.
     */
    DEFINE_PROPERTY(RenderingPipeline, renderingPipeline, "Rendering Pipeline", 0, VALUE(RenderingPipeline::FORWARD))

    /** Scene flags.
     */
    DEFINE_BITFIELD_PROPERTY(
        uint32_t, sceneFlags, "Scene Flags", PropertyFlags::IS_BITFIELD, VALUE(0), CameraComponent::SceneFlagBits)

    /** Render pipeline flags.
     */
    DEFINE_BITFIELD_PROPERTY(uint32_t, pipelineFlags, "Pipeline Flags", PropertyFlags::IS_BITFIELD,
        VALUE(PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT), CameraComponent::PipelineFlagBits)

    /** Aspect ratio of the camera (perspective only).
     *  If aspect is 0 or less the aspect ratio of the canvas should be used.
     */
    DEFINE_PROPERTY(float, aspect, "Aspect Ratio", 0, VALUE(-1.f))

    /** Field of view of the camera (perspective only).
     */
    DEFINE_PROPERTY(float, yFov, "Vertical Fov", 0, VALUE(60.f * BASE_NS::Math::DEG2RAD))

    /** Viewport scale for orthographic.
     */
    DEFINE_PROPERTY(float, xMag, "X Scale for Orthographic", 0, VALUE(1.f))

    /** Viewport scale for orthographic.
     */
    DEFINE_PROPERTY(float, yMag, "Y Scale for Orthographic", 0, VALUE(1.f))

    /** Viewport horizontal offset for frustum.
     */
    DEFINE_PROPERTY(float, xOffset, "X Offset for Frustum", 0, VALUE(0.f))

    /** Viewport height for orthographic.
     * Viewport vertical offset for frustum.
     */
    DEFINE_PROPERTY(float, yOffset, "Y Offset for Frustum", 0, VALUE(0.f))

    /** Near distance.
     */
    DEFINE_PROPERTY(float, zNear, "Z Near", 0, VALUE(0.3f))

    /** Far distance.
     */
    DEFINE_PROPERTY(float, zFar, "Z Far", 0, VALUE(1000.f))

    /** Viewport position and size in normalized render resolution: [0] = x, [1] = y, [2] = relative width [3] =
     * relative height. (NOTE: relative width/height does not remove the offset)
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, viewport, "Viewport", 0, ARRAY_VALUE(0.0f, 0.0f, 1.0f, 1.0f))

    /** Scissor offset and size in normalized render resolution: [0] = x, [1] = y, [2] = relative width [3] =
     * relative height. (NOTE: relative width/height does not remove the offset)
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, scissor, "Scissor", 0, ARRAY_VALUE(0.0f, 0.0f, 1.0f, 1.0f))

    /** Render resolution of the camera: [0] = width in pixels, [1] = height in pixels.
     */
    DEFINE_PROPERTY(BASE_NS::Math::UVec2, renderResolution, "Render Resolution", 0, ARRAY_VALUE(0, 0))

    /** Projection matrix used when type is CORE_CAMERA_TYPE_CUSTOM. For other camera types projection matrix is
     * calculated from the other properties.
     *
     * The Vulkan coordinate system is used. Compared to the OpenGL coordinate system the NDC Y-axis is flipped and the
     * depth range is from 0 to 1 (instead of -1 to 1).
     *
     * One possibility to convert an OpenGL projection matrix to the coordinate system used by the engine is to multiply
     * it with matrix:
     *
     * | 1.0   0.0   0.0   0.0 |
     * | 0.0  -1.0   0.0   0.0 |
     * | 0.0   0.0   0.5   0.5 |
     * | 0.0   0.0   0.0   1.0 |

     */
    DEFINE_PROPERTY(BASE_NS::Math::Mat4X4, customProjectionMatrix, "Custom Projection Matrix", 0,
        ARRAY_VALUE(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f))

    /** Clear color value. Clears the color buffer(s) if clearFlags set.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, clearColorValue, "Clear Color Value", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f, 0.0f))

    /** Clear depth value. Clears the depth buffer(s) if clearFlags set.
     */
    DEFINE_PROPERTY(float, clearDepthValue, "Clear Depth Value", 0, VALUE(1.0f))

    /** Entity for environment component. Controls indirect and environment lighting.
     */

    /** Entity containing an EnvironmentComponent that is used by this camera when rendering. Controls indirect and
     * environment lighting options. If not defined the scene default environment options will be used.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, environment, "Environment", 0, )

    /** Entity containing a FogComponent that is used by this camera when rendering. If not defined the
     * camera will use default RenderConfigurationComponent configuration.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, fog, "Fog", 0, )

    /** Entity containing a PostProcessComponent that is used by this camera when rendering. If not defined the
     * camera will use default CORE3D_POST_PROCESS_CAM configuration.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, postProcess, "Post process", 0, )

    /** Defines a layer mask which affects camera's rendering. Default is all layer mask, when the camera renders
     * objects from all layers. */
    DEFINE_BITFIELD_PROPERTY(uint64_t, layerMask, "Layer mask", PropertyFlags::IS_BITFIELD,
        VALUE(LayerConstants::ALL_LAYER_MASK), LayerFlagBits)

    /** Entity containing a CameraComponent for pre-pass camera. Most of the values are copied for
     * pre-pass render camera. PipelineFlagBits::XXX_COLOR_PRE_PASS_BIT (s) must be set.
     * Pre-pass can be done automatically as well, but for resolution/layer etc. config ser can control it better.
     * The active bit needs to be disabled from the pre-pass camera, otherwise the camera is processed normally.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, prePassCamera, "Pre-pass camera", 0, )

    /** NOTE: add array of four to targets */

    /** Custom depth target. Must be a valid handle if using RenderTarget::CUSTOM.
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, customDepthTarget, "Custom Depth Target", 0, )

    /** Custom color target. Must be a valid handle if using RenderTarget::CUSTOM.
     */
    DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::EntityReference>, customColorTargets, "Custom Color Targets", 0, )

    /** Depth target creation customization
     */
    DEFINE_PROPERTY(TargetUsage, depthTargetCustomization, "Depth Target Creation Customization", 0, )

    /** Color target creation customization
     */
    DEFINE_PROPERTY(BASE_NS::vector<TargetUsage>, colorTargetCustomization, "Color Target Creation Customization", 0, )

    /** Explicit custom camera render node graph. (Prefer using customRenderNodeGraphFile for correct patching)
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, customRenderNodeGraph, "Explicit Custom Camera Render Node Graph", 0, )

    /** Custom camera render node graph file. (Can be patched with e.g. post process ids etc.)
     * Chosen only if no explicit customSceneRenderNodeGraph
     */
    DEFINE_PROPERTY(BASE_NS::string, customRenderNodeGraphFile, "Custom Scene Render Node Graph File", 0, )

    /** Multi-view camera entities for the base camera
     */
    DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::Entity>, multiViewCameras, "Multi-view camera entities", 0, )

    /** Entity containing multiple EnvironmentComponents that are pushed to camera buffers.
     * Controls indirect and environment lighting. If not defined the scene default environment options will be used */
    DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::Entity>, environments, "Environments", 0, )

END_COMPONENT(ICameraComponentManager, CameraComponent, "184c996b-67aa-4456-9f03-72e2d968931b")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
