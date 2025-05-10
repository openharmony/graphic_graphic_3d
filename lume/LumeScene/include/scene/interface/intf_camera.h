#ifndef SCENE_INTERFACE_ICAMERA_H
#define SCENE_INTERFACE_ICAMERA_H

#include <scene/base/types.h>
#include <scene/interface/intf_input_receiver.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_render_target.h>

#include <base/math/matrix.h>
#include <base/util/color.h>
#include <base/util/formats.h>

#include <meta/base/interface_macros.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief The Camera projection type
 */
enum class CameraProjection : uint8_t {
    /* Orthographic camera */
    ORTHOGRAPHIC = 0,
    /* Perspective camera */
    PERSPECTIVE = 1,
    /* Perspective camera */
    FRUSTUM = 2,
    /* Custom matrix provided for camera */
    CUSTOM = 3
};

/**
 * @brief The Camera culling mode
 */
enum class CameraCulling : uint8_t {
    NONE = 0,
    /* Basic view frustum cull for objects */
    VIEW_FRUSTUM = 1
};

/**
 * @brief The Camera pipeline type
 */
enum class CameraPipeline : uint8_t {
    /* Light weight forward pipeline. Renders directly to back buffer */
    LIGHT_FORWARD = 0,
    /* Default forward pipeline */
    FORWARD = 1,
    /* Deferred pipeline */
    DEFERRED = 2,
    /* Custom rendering pipeline */
    CUSTOM = 3,
};

enum class CameraPipelineFlag : uint32_t {
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

    /** Empty bit (1 << 10) */

    /** Disallow reflection plane for camera
     */
    DISALLOW_REFLECTION_BIT = (1 << 11),
    /** Automatic cubemap camera targets
     * Can be used e.g. with dynamic cubemap renderings for camera
     */
    CUBEMAP_BIT = (1 << 12),
};
inline CameraPipelineFlag operator|(CameraPipelineFlag l, CameraPipelineFlag r)
{
    return CameraPipelineFlag(uint32_t(l) | uint32_t(r));
}

enum class CameraSceneFlag : uint32_t {
    /** Camera is rendered when it's active. */
    ACTIVE_RENDER_BIT = (1 << 0),
    /** Main camera. If multiple main cameras, the first is chosen as ECS main camera. Main camera is treated always
       as active. */
    MAIN_CAMERA_BIT = (1 << 1),
};
inline CameraSceneFlag operator|(CameraSceneFlag l, CameraSceneFlag r)
{
    return CameraSceneFlag(uint32_t(l) | uint32_t(r));
}

struct ColorFormat {
    BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
    uint32_t usageFlags {};
};

class ICamera : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICamera, "76af2b00-f3eb-414e-b547-8758adf4d31a")
public:
    /**
     * @brief Field of view of the camera (perspective only).
     */
    META_PROPERTY(float, FoV)
    /**
     * @brief Aspect ratio of the camera (perspective only).
     *  If aspect is 0 or less the aspect ratio of the canvas should be used.
     */
    META_PROPERTY(float, AspectRatio)
    /**
     * @brief Near distance.
     */
    META_PROPERTY(float, NearPlane)

    /**
     * @brief Far distance.
     */
    META_PROPERTY(float, FarPlane)

    /**
     * @brief Viewport horizontal scale (orthographic projection).
     */
    META_PROPERTY(float, XMagnification)

    /**
     * @brief Viewport vertical scale (orthographic projection).
     */
    META_PROPERTY(float, YMagnification)

    /**
     * @brief Viewport horizontal offset (frustum projection).
     */
    META_PROPERTY(float, XOffset)

    /**
     * @brief Viewport vertical offset (frustum projection).
     */
    META_PROPERTY(float, YOffset)

    /**
     * @brief Projection type of the camera. SCENE_CAM_PROJECTION_PERSPECTIVE is used by default.
     */
    META_PROPERTY(CameraProjection, Projection)

    /**
     * @brief Culling mode for the camera.
     */
    META_PROPERTY(CameraCulling, Culling)

    /**
     * @brief  Default camera rendering pipeline. Defaults to LIGHT_FORWARD
     */
    META_PROPERTY(CameraPipeline, RenderingPipeline)

    /**
     * @brief Scene flags.
     */
    META_PROPERTY(uint32_t, SceneFlags)

    /**
     * @brief Render pipeline flags.
     */
    META_PROPERTY(uint32_t, PipelineFlags)

    /**
     * @brief Viewport position and size in normalized render resolution: [0] = x, [1] = y, [2] = relative width [3] =
     * relative height. (NOTE: relative width/height does not remove the offset)
     */
    META_PROPERTY(BASE_NS::Math::Vec4, Viewport)

    /**
     * @brief Scissor offset and size in normalized render resolution: [0] = x, [1] = y, [2] = relative width [3] =
     * relative height. (NOTE: relative width/height does not remove the offset)
     */
    META_PROPERTY(BASE_NS::Math::Vec4, Scissor)

    /**
     * @brief Render resolution of the camera: [0] = width in pixels, [1] = height in pixels.
     */
    META_PROPERTY(BASE_NS::Math::UVec2, RenderTargetSize)

    /**
     * @brief Clear color value. Clears the color buffer(s) if clearFlags set.
     */
    META_PROPERTY(BASE_NS::Math::Vec4, ClearColor)

    /**
     * @brief Clear depth value. Clears the depth buffer(s) if clearFlags set.
     */
    META_PROPERTY(float, ClearDepth)
    /**
     * @brief Defines a layer mask which affects camera's rendering.
     */
    META_PROPERTY(uint64_t, LayerMask)
    /**
     * @brief Color target creation customization.
     */
    META_ARRAY_PROPERTY(ColorFormat, ColorTargetCustomization)

    /**
     * @brief Projection matrix used when projection type is CUSTOM.
     */
    META_PROPERTY(BASE_NS::Math::Mat4X4, CustomProjectionMatrix)

    /**
     * @brief Camera post process configuration.
     * @return
     */
    META_PROPERTY(IPostProcess::Ptr, PostProcess)

    virtual Future<bool> SetActive(bool active = true) = 0;
    virtual bool IsActive() const = 0;
    virtual Future<bool> SetRenderTarget(const IRenderTarget::Ptr&) = 0;

    /**
     * @brief Send an input event to all attachments of the camera which implement IInputReceiver.
     * @param event The event to send.
     */
    virtual void SendInputEvent(PointerEvent& event) = 0;
};

META_REGISTER_CLASS(CameraNode, "3782e343-7e5e-4bee-af14-7f7deaa806f2", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ICamera)
META_TYPE(SCENE_NS::CameraProjection)
META_TYPE(SCENE_NS::CameraCulling)
META_TYPE(SCENE_NS::CameraPipeline)
META_TYPE(SCENE_NS::CameraPipelineFlag)
META_TYPE(SCENE_NS::CameraSceneFlag)
META_TYPE(SCENE_NS::ColorFormat)
#endif
