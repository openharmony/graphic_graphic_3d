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

#ifndef API_3D_UTIL_SCENE_UTIL_H
#define API_3D_UTIL_SCENE_UTIL_H

#include <3d/ecs/components/material_component.h>
#include <3d/loaders/intf_scene_loader.h>
#include <3d/namespace.h>
#include <base/containers/string_view.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/entity.h>
#include <core/namespace.h>
#include <render/device/pipeline_state_desc.h>
#include <render/resource_handle.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IAnimationPlayback;
struct LightComponent;

/** @ingroup group_util_isceneutil
 * @{
 */
/** Interface for helper class to create different ECS related objects.
 */
class ISceneUtil {
public:
    /** Create camera.
     * @param ecs Entity component system to contain the camera instance.
     * @param position Position of the camera, defaults to origin.
     * @param rotation Rotation of the camera, defaults to identity.
     * @param zNear Near plane distance.
     * @param zFar Far plane distance.
     * @param fovDegrees FOV in degrees.
     * @return Newly created camera entity.
     */
    virtual CORE_NS::Entity CreateCamera(CORE_NS::IEcs& ecs, const BASE_NS::Math::Vec3& position,
        const BASE_NS::Math::Quat& rotation, float zNear, float zFar, float fovDegrees) const = 0;

    /** Update camera viewport and aspect ratio according to render resolution.
     * @param ecs Entity component system to contain the camera instance.
     * @param entity Camera entity.
     * @param renderResolution Render resolution (width and height).
     */
    virtual void UpdateCameraViewport(
        CORE_NS::IEcs& ecs, CORE_NS::Entity entity, const BASE_NS::Math::UVec2& renderResolution) const = 0;

    /** Update camera viewport according to render resolution, optionally also determines aspect ratio and proper FOV
     * according to screen shape.
     * Calculated values might change due to projection type.
     * @param ecs Entity component system to contain the camera instance.
     * @param entity Camera entity.
     * @param renderResolution Render resolution (width and height).
     * @param autoAspect If true, also calculates the aspect ratio.
     * @param fovY Suggested y-fov in radians, in case the screen shape is portrait the x-fov is calculated and used.
     * @param orthoScale Scale for ortho camera.
     */
    virtual void UpdateCameraViewport(CORE_NS::IEcs& ecs, CORE_NS::Entity entity,
        const BASE_NS::Math::UVec2& renderResolution, bool autoAspect, float fovY, float orthoScale) const = 0;

    /** Update the transformation of the given entity such that it is located at 'eye', rotated towards 'target', and
     * its up vector is 'up'. The up vector will only match 'up' if the direction from 'eye' to 'target' is
     * perpendicular to 'up'.
     * @param ecs Entity component system to contain the camera instance.
     * @param entity Entity to modify.
     * @param eye Final world space position of the entity.
     * @param target World space position where the entity's z-axis will point to.
     * @param up Desired up vector of the entity.
     */
    virtual void CameraLookAt(CORE_NS::IEcs& ecs, CORE_NS::Entity entity, const BASE_NS::Math::Vec3& eye,
        const BASE_NS::Math::Vec3& target, const BASE_NS::Math::Vec3& up) = 0;

    /** Create a light. Creates a new ecs light based on inputs.
     * For shadow lights a ecs camera is created and sane default values are calculated.
     * @param ecs Entity component system to contain the light instance.
     * @param lightComponent A filled up light component struct (will be attached to ECS internally.
     * @param position Position of the light, defaults to origin.
     * @param rotation Rotation of the light, defaults to identity.
     * @return Newly created light entity.
     */
    virtual CORE_NS::Entity CreateLight(CORE_NS::IEcs& ecs, const LightComponent& lightComponent,
        const BASE_NS::Math::Vec3& position, const BASE_NS::Math::Quat& rotation) const = 0;

    /** Create reflection plane component. Component is added to the given entity. For reflections to work the entity is
     * expected to have a RenderMeshComponent. The method updates the material of the mesh for planar reflections.
     * @param ecs Entity component system instance.
     * @param nodeEntity Scene node entity where to attach.
     */
    virtual void CreateReflectionPlaneComponent(CORE_NS::IEcs& ecs, const CORE_NS::Entity& nodeEntity) = 0;

    /** Creates a new animation for targetEntity which animates the target's joints the same way as animationEntity
     * animatest the joints of sourceEntity.
     * @param ecs Entity component system instance.
     * @param targetEntity Entity which will be the target of the new animation.
     * @param sourceEntity Entity whos skin will be used for retargetting the animation to targetEntity.
     * @param animationEntity Entity with a skinning animation.
     * @return Newly created animation playback.
     */
    virtual IAnimationPlayback* RetargetSkinAnimation(CORE_NS::IEcs& ecs, CORE_NS::Entity targetEntity,
        CORE_NS::Entity sourceEntity, CORE_NS::Entity animationEntity) const = 0;

    /** Material shader info for default material shader data fetching.
     */
    struct MaterialShaderInfo {
        /** Alpha blend mode */
        bool alphaBlend { false };
        /** Cull mode */
        RENDER_NS::CullModeFlags cullModeFlags { RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_NONE };
        /** Front face */
        RENDER_NS::FrontFace frontFace { RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE };
    };

    /** Returns default material shader built-in data for material component usage
     * @param ecs Entity component system instance.
     * @param info Material shader info which is used to fetch default material handles and entities.
     * @param materialShader Returns material shader with entity references.
     * @param depthSahder Returns depth shader with entity references.
     */
    virtual void GetDefaultMaterialShaderData(CORE_NS::IEcs& ecs, const MaterialShaderInfo& info,
        MaterialComponent::Shader& materialShader, MaterialComponent::Shader& depthShader) const = 0;

    /** Returns default material shader data based on render slot name.
     * NOTE: Alpha blend does not affect this, i.e. we do not flip the flag in graphics state
     * @param ecs Entity component system instance.
     * @param info Material shader info which is used to fetch default material handles and entities.
     * @param renderSlot Name of the render slot.
     * @param shader Returns material shader with entity references.
     */
    virtual void GetDefaultMaterialShaderData(CORE_NS::IEcs& ecs, const ISceneUtil::MaterialShaderInfo& info,
        const BASE_NS::string_view renderSlot, MaterialComponent::Shader& shader) const = 0;

    /** Share the sourceEntity's skin with the targetEntity.
     * @param ecs Entity component system instance.
     * @param targetEntity Entity whos skin will be modified to use the source joints.
     * @param sourceEntity Entity whos joints will be applied to target.
     */
    virtual void ShareSkin(CORE_NS::IEcs& ecs, CORE_NS::Entity targetEntity, CORE_NS::Entity sourceEntity) const = 0;

    /** Register a new scene loader. Plugins implementing support for new scene file formats should register an instance
     * so that it can be discovered with GetSceneLoader.
     * @param loader Pointer to a new scene loader instance.
     */
    virtual void RegisterSceneLoader(const ISceneLoader::Ptr& loader) = 0;

    /** Unregister a previously registered scene loader. Loaders must be unregistered e.g. when the plugin offering the
     * instance is unloaded.
     * @param loader Pointer to a previously registered scene loader instance.
     */
    virtual void UnregisterSceneLoader(const ISceneLoader::Ptr& loader) = 0;

    /** Get a scene loader which can handle the given file. Selection is done based on file extensions. Actual loading
     * might still fail with the returned loader.
     * @param uri File URI.
     * @return Pointer to a scene loader or null if none of the registered loaders are able to handle the file.
     */
    virtual ISceneLoader::Ptr GetSceneLoader(BASE_NS::string_view uri) const = 0;

protected:
    ISceneUtil() = default;
    virtual ~ISceneUtil() = default;
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_UTIL_SCENE_UTIL_H
