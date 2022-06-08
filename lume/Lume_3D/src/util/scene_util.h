/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_UTIL_SCENE_UTIL_H
#define CORE_UTIL_SCENE_UTIL_H

#include <3d/ecs/components/light_component.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/string_view.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
class IGraphicsContext;

class SceneUtil : public ISceneUtil {
public:
    explicit SceneUtil(IGraphicsContext& graphicsContext);
    ~SceneUtil() override = default;

    CORE_NS::Entity CreateCamera(CORE_NS::IEcs& ecs, const BASE_NS::Math::Vec3& position,
        const BASE_NS::Math::Quat& rotation, float zNear, float zFar, float fovDegrees) const override;
    void UpdateCameraViewport(
        CORE_NS::IEcs& ecs, CORE_NS::Entity entity, const BASE_NS::Math::UVec2& renderResolution) const override;
    void UpdateCameraViewport(CORE_NS::IEcs& ecs, CORE_NS::Entity entity, const BASE_NS::Math::UVec2& renderResolution,
        bool autoAspect, float fovY, float orthoScale) const override;

    CORE_NS::Entity CreateLight(CORE_NS::IEcs& ecs, const LightComponent& lightComponent,
        const BASE_NS::Math::Vec3& position, const BASE_NS::Math::Quat& rotation) const override;

    /** Reflection plane object */
    struct ReflectionPlane {
        // Reflection plane entity.
        CORE_NS::Entity entity;
        // Reflection plane mesh.
        CORE_NS::Entity mesh;
        // Reflection plane material.
        CORE_NS::Entity material;
        // Reflection plane color target.
        CORE_NS::EntityReference colorTarget;
        // Reflection plane depth target.
        CORE_NS::EntityReference depthTarget;
    };

    void CreateReflectionPlaneComponent(CORE_NS::IEcs& ecs, const CORE_NS::Entity& nodeEntity) override;

    IAnimationPlayback* RetargetSkinAnimation(CORE_NS::IEcs& ecs, CORE_NS::Entity targetEntity,
        CORE_NS::Entity sourceEntity, CORE_NS::Entity animationEntity) const override;

    void GetDefaultMaterialShaderData(CORE_NS::IEcs& ecs, const ISceneUtil::MaterialShaderInfo& info,
        MaterialComponent::Shader& materialShader, MaterialComponent::Shader& depthShader) const override;
    void GetDefaultMaterialShaderData(CORE_NS::IEcs& ecs, const ISceneUtil::MaterialShaderInfo& info,
        const BASE_NS::string_view renderSlot, MaterialComponent::Shader& shader) const override;

private:
    IGraphicsContext& graphicsContext_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_SCENE_UTIL_H
