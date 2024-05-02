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

#ifndef CORE_UTIL_SCENE_UTIL_H
#define CORE_UTIL_SCENE_UTIL_H

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>

CORE3D_BEGIN_NAMESPACE()
class IGraphicsContext;

namespace CameraMatrixUtil {
BASE_NS::Math::Mat4X4 CalculateProjectionMatrix(const CameraComponent& cameraComponent, bool& isCameraNegative);
}

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
    void CameraLookAt(CORE_NS::IEcs& ecs, CORE_NS::Entity entity, const BASE_NS::Math::Vec3& eye,
        const BASE_NS::Math::Vec3& target, const BASE_NS::Math::Vec3& up) override;

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

    void ShareSkin(CORE_NS::IEcs& ecs, CORE_NS::Entity targetEntity, CORE_NS::Entity sourceEntity) const override;

    void RegisterSceneLoader(const ISceneLoader::Ptr& loader) override;
    void UnregisterSceneLoader(const ISceneLoader::Ptr& loader) override;
    ISceneLoader::Ptr GetSceneLoader(BASE_NS::string_view uri) const override;

private:
    IGraphicsContext& graphicsContext_;
    BASE_NS::vector<ISceneLoader::Ptr> sceneLoaders_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_SCENE_UTIL_H
