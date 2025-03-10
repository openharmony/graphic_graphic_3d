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

#include "util/scene_util.h"

#include <algorithm>
#include <cinttypes>
#include <unordered_set>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/reflection_probe_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/util/algorithm.h>
#include <base/util/uid_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>

#include "uri_lookup.h"
#include "util/component_util_functions.h"

namespace {
constexpr uint32_t REFLECTION_PROBE_DEFAULT_SIZE { 256U };
}

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace CameraMatrixUtil {
Math::Mat4X4 CalculateProjectionMatrix(const CameraComponent& cameraComponent, bool& isCameraNegative)
{
    switch (cameraComponent.projection) {
        case CameraComponent::Projection::ORTHOGRAPHIC: {
            auto orthoProj = Math::OrthoRhZo(cameraComponent.xMag * -0.5f, cameraComponent.xMag * 0.5f,
                cameraComponent.yMag * -0.5f, cameraComponent.yMag * 0.5f, cameraComponent.zNear, cameraComponent.zFar);
            orthoProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
            return orthoProj;
        }
        case CameraComponent::Projection::PERSPECTIVE: {
            auto aspect = 1.f;
            if (cameraComponent.aspect > 0.f) {
                aspect = cameraComponent.aspect;
            } else if (cameraComponent.renderResolution.y > 0U) {
                aspect = static_cast<float>(cameraComponent.renderResolution.x) /
                         static_cast<float>(cameraComponent.renderResolution.y);
            }
            auto persProj =
                Math::PerspectiveRhZo(cameraComponent.yFov, aspect, cameraComponent.zNear, cameraComponent.zFar);
            persProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
            return persProj;
        }
        case CameraComponent::Projection::FRUSTUM: {
            auto aspect = 1.f;
            if (cameraComponent.aspect > 0.f) {
                aspect = cameraComponent.aspect;
            } else if (cameraComponent.renderResolution.y > 0U) {
                aspect = static_cast<float>(cameraComponent.renderResolution.x) /
                         static_cast<float>(cameraComponent.renderResolution.y);
            }
            // with offset 0.5, the camera should offset half the screen
            const float scale = tan(cameraComponent.yFov * 0.5f) * cameraComponent.zNear;
            const float xOffset = cameraComponent.xOffset * scale * aspect * 2.0f;
            const float yOffset = cameraComponent.yOffset * scale * 2.0f;
            float left = -aspect * scale;
            float right = aspect * scale;
            float bottom = -scale;
            float top = scale;
            auto persProj = Math::PerspectiveRhZo(left + xOffset, right + xOffset, bottom + yOffset, top + yOffset,
                cameraComponent.zNear, cameraComponent.zFar);
            persProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
            return persProj;
        }
        case CameraComponent::Projection::CUSTOM: {
            isCameraNegative = Math::Determinant(cameraComponent.customProjectionMatrix) < 0.0f;
            return cameraComponent.customProjectionMatrix;
        }
        default:
            return Math::IDENTITY_4X4;
    }
}
} // namespace CameraMatrixUtil

SceneUtil::SceneUtil(IGraphicsContext& graphicsContext) : graphicsContext_(graphicsContext) {}

Entity SceneUtil::CreateCamera(
    IEcs& ecs, const Math::Vec3& position, const Math::Quat& rotation, float zNear, float zFar, float fovDegrees) const
{
    auto lmm = GetManager<ILocalMatrixComponentManager>(ecs);
    auto wmm = GetManager<IWorldMatrixComponentManager>(ecs);
    auto ncm = GetManager<INodeComponentManager>(ecs);
    auto tcm = GetManager<ITransformComponentManager>(ecs);
    auto ccm = GetManager<ICameraComponentManager>(ecs);
    if (!lmm || !wmm || !ncm || !tcm || !ccm) {
        return {};
    }

    Entity camera = ecs.GetEntityManager().Create();

    lmm->Create(camera);
    wmm->Create(camera);
    ncm->Create(camera);

    TransformComponent tc;
    tc.position = position;
    tc.rotation = rotation;
    tcm->Set(camera, tc);

    CameraComponent cc;
    cc.sceneFlags |= CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
    cc.projection = CameraComponent::Projection::PERSPECTIVE;
    cc.yFov = Math::DEG2RAD * fovDegrees;
    cc.zNear = zNear;
    cc.zFar = zFar;
    ccm->Set(camera, cc);

    return camera;
}

void SceneUtil::UpdateCameraViewport(IEcs& ecs, Entity entity, const Math::UVec2& renderResolution) const
{
    auto ccm = GetManager<ICameraComponentManager>(ecs);
    if (ccm && CORE_NS::EntityUtil::IsValid(entity)) {
        ScopedHandle<CameraComponent> cameraHandle = ccm->Write(entity);
        if (!cameraHandle) {
            ccm->Create(entity);
            cameraHandle = ccm->Write(entity);
            if (!cameraHandle) {
                return;
            }
        }
        CameraComponent& cameraComponent = *cameraHandle;
        cameraComponent.aspect = (renderResolution.y > 0U)
                                     ? (static_cast<float>(renderResolution.x) / static_cast<float>(renderResolution.y))
                                     : 1.0f;
        cameraComponent.renderResolution[0] = renderResolution.x;
        cameraComponent.renderResolution[1] = renderResolution.y;
    }
}

void SceneUtil::UpdateCameraViewport(
    IEcs& ecs, Entity entity, const Math::UVec2& renderResolution, bool autoAspect, float fovY, float orthoScale) const
{
    auto ccm = GetManager<ICameraComponentManager>(ecs);
    if (ccm && CORE_NS::EntityUtil::IsValid(entity)) {
        ScopedHandle<CameraComponent> cameraHandle = ccm->Write(entity);
        if (!cameraHandle) {
            ccm->Create(entity);
            cameraHandle = ccm->Write(entity);
            if (!cameraHandle) {
                return;
            }
        }
        CameraComponent& cameraComponent = *cameraHandle;
        if (autoAspect) {
            const float aspectRatio =
                (renderResolution.y > 0)
                    ? (static_cast<float>(renderResolution.x) / static_cast<float>(renderResolution.y))
                    : 1.0f;

            // Using the fov value as xfov on portrait screens to keep the object
            // better in the frame.
            const float yFov = (aspectRatio > 1.0f) ? fovY : (2.0f * Math::atan(Math::tan(fovY * 0.5f) / aspectRatio));

            // Update the camera parameters.
            cameraComponent.aspect = aspectRatio;
            cameraComponent.yFov = yFov;

            // The camera can also be in ortho mode. Using a separate zoom value.
            cameraComponent.yMag = orthoScale;
            cameraComponent.xMag = cameraComponent.yMag * aspectRatio;
        }

        cameraComponent.renderResolution[0] = renderResolution.x;
        cameraComponent.renderResolution[1] = renderResolution.y;
    }
}

void SceneUtil::CameraLookAt(
    IEcs& ecs, Entity entity, const Math::Vec3& eye, const Math::Vec3& target, const Math::Vec3& up)
{
    auto ncm = GetManager<INodeComponentManager>(ecs);
    auto tcm = GetManager<ITransformComponentManager>(ecs);
    if (!ncm || !tcm) {
        return;
    }

    auto parentWorld = Math::Mat4X4(1.f);
    auto getParent = [ncm](Entity entity) {
        if (auto nodeHandle = ncm->Read(entity)) {
            return nodeHandle->parent;
        }
        return Entity {};
    };

    // walk up the hierachy to get an up-to-date world matrix.
    for (Entity node = getParent(entity); EntityUtil::IsValid(node); node = getParent(node)) {
        if (auto parentTransformHandle = tcm->Read(node)) {
            parentWorld = Math::Trs(parentTransformHandle->position, parentTransformHandle->rotation,
                          parentTransformHandle->scale) * parentWorld;
        }
    }

    // entity's local transform should invert any parent transformations and apply the transform of rotating towards
    // target and moving to eye. this transform is the inverse of the look-at matrix.
    auto worldMatrix = Math::Inverse(parentWorld) * Math::Inverse(Math::LookAtRh(eye, target, up));

    Math::Vec3 scale;
    Math::Quat orientation;
    Math::Vec3 translation;
    Math::Vec3 skew;
    Math::Vec4 perspective;
    if (Math::Decompose(worldMatrix, scale, orientation, translation, skew, perspective)) {
        if (auto transformHandle = tcm->Write(entity)) {
            transformHandle->position = translation;
            transformHandle->rotation = orientation;
            transformHandle->scale = scale;
        }
    }
}

Entity SceneUtil::CreateLight(
    IEcs& ecs, const LightComponent& lightComponent, const Math::Vec3& position, const Math::Quat& rotation) const
{
    auto lmm = GetManager<ILocalMatrixComponentManager>(ecs);
    auto wmm = GetManager<IWorldMatrixComponentManager>(ecs);
    auto ncm = GetManager<INodeComponentManager>(ecs);
    auto nameM = GetManager<INameComponentManager>(ecs);
    auto tcm = GetManager<ITransformComponentManager>(ecs);
    auto lcm = GetManager<ILightComponentManager>(ecs);
    if (!lmm || !wmm || !ncm || !nameM || !tcm || !lcm) {
        return {};
    }

    Entity light = ecs.GetEntityManager().Create();

    lmm->Create(light);
    wmm->Create(light);
    ncm->Create(light);
    nameM->Create(light);
    constexpr string_view lightName("Light");
    nameM->Write(light)->name = lightName;

    TransformComponent tc;
    tc.position = position;
    tc.rotation = rotation;
    tcm->Set(light, tc);

    LightComponent lc = lightComponent;
    lc.shadowEnabled = (lc.type != LightComponent::Type::POINT) && lc.shadowEnabled;
    lc.range = ComponentUtilFunctions::CalculateSafeLightRange(lc.range, lc.intensity);
    lcm->Set(light, lc);

    return light;
}

// reflection plane helpers
namespace {
// default size, updated in render system based on main scene camera
constexpr Math::UVec2 DEFAULT_PLANE_TARGET_SIZE { 2u, 2u };

EntityReference CreateReflectionPlaneGpuImage(IGpuResourceManager& gpuResourceMgr,
    IRenderHandleComponentManager& handleManager, INameComponentManager& nameManager, const string_view name,
    const Format format, const ImageUsageFlags usageFlags, const MemoryPropertyFlags memoryPropertyFlags)
{
    const auto entity = handleManager.GetEcs().GetEntityManager().CreateReferenceCounted();
    handleManager.Create(entity);

    GpuImageDesc desc;
    desc.width = DEFAULT_PLANE_TARGET_SIZE.x;
    desc.height = DEFAULT_PLANE_TARGET_SIZE.y;
    desc.depth = 1U;
    desc.mipCount = DefaultMaterialCameraConstants::REFLECTION_PLANE_MIP_COUNT;
    desc.format = format;
    desc.memoryPropertyFlags = memoryPropertyFlags;
    desc.usageFlags = usageFlags;
    desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    handleManager.Write(entity)->reference = gpuResourceMgr.Create(name, desc);

    nameManager.Create(entity);
    nameManager.Write(entity)->name = name;
    return entity;
}

SceneUtil::ReflectionPlane CreateReflectionPlaneObjectFromEntity(
    IEcs& ecs, IGraphicsContext& graphicsContext, const Entity& nodeEntity)
{
    SceneUtil::ReflectionPlane plane;
    auto* renderMeshCM = GetManager<IRenderMeshComponentManager>(ecs);
    auto* meshCM = GetManager<IMeshComponentManager>(ecs);
    auto* matCM = GetManager<IMaterialComponentManager>(ecs);
    auto* gpuHandleCM = GetManager<IRenderHandleComponentManager>(ecs);

    auto* nameCM = GetManager<INameComponentManager>(ecs);
    if (!(renderMeshCM && meshCM && matCM && gpuHandleCM && nameCM)) {
        return plane;
    }

    auto& device = graphicsContext.GetRenderContext().GetDevice();
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    plane.entity = nodeEntity;
    plane.colorTarget = CreateReflectionPlaneGpuImage(gpuResourceMgr, *gpuHandleCM, *nameCM,
        DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + to_hex(nodeEntity.id),
        Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32,
        ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // NOTE: uses transient attachement usage flag and cannot be read.
    plane.depthTarget = CreateReflectionPlaneGpuImage(gpuResourceMgr, *gpuHandleCM, *nameCM,
        DefaultMaterialCameraConstants::CAMERA_DEPTH_PREFIX_NAME + to_hex(nodeEntity.id), Format::BASE_FORMAT_D16_UNORM,
        ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);

    const auto meshHandle = [&]() {
        const auto rmcHandle = renderMeshCM->Read(nodeEntity);
        if (!rmcHandle) {
            return ScopedHandle<const MeshComponent> {};
        }
        return meshCM->Read(rmcHandle->mesh);
    }();
    if (!meshHandle && meshHandle->submeshes.empty()) {
        return plane;
    }
    if (auto matHandle = matCM->Write(meshHandle->submeshes[0].material); matHandle) {
        MaterialComponent& matComponent = *matHandle;
        auto* uriCM = GetManager<IUriComponentManager>(ecs);
        auto* renderHandleCM = GetManager<IRenderHandleComponentManager>(ecs);
        constexpr const string_view uri = "3dshaders://shader/core3d_dm_fw_reflection_plane.shader";
        auto shaderEntity = LookupResourceByUri(uri, *uriCM, *renderHandleCM);
        if (!EntityUtil::IsValid(shaderEntity)) {
            shaderEntity = ecs.GetEntityManager().Create();
            renderHandleCM->Create(shaderEntity);
            renderHandleCM->Write(shaderEntity)->reference = device.GetShaderManager().GetShaderHandle(uri);
            uriCM->Create(shaderEntity);
            uriCM->Write(shaderEntity)->uri = uri;
        }
        matComponent.materialShader.shader = ecs.GetEntityManager().GetReferenceCounted(shaderEntity);

        matComponent.textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].image = plane.colorTarget;
        matComponent.extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::DISCARD_BIT;
    }

    return plane;
}
} // namespace

void SceneUtil::CreateReflectionPlaneComponent(IEcs& ecs, const Entity& nodeEntity)
{
    SceneUtil::ReflectionPlane plane = CreateReflectionPlaneObjectFromEntity(ecs, graphicsContext_, nodeEntity);
    if (EntityUtil::IsValid(plane.entity)) {
        auto prcm = GetManager<IPlanarReflectionComponentManager>(ecs);
        PlanarReflectionComponent prc;
        prc.colorRenderTarget = plane.colorTarget;
        prc.depthRenderTarget = plane.depthTarget;
        prc.renderTargetResolution[0] = DEFAULT_PLANE_TARGET_SIZE.x;
        prc.renderTargetResolution[1] = DEFAULT_PLANE_TARGET_SIZE.y;
        prcm->Set(plane.entity, prc);
    }
}

namespace {
float CalculateScalingFactor(IEcs& ecs, Entity targetEntity, Entity sourceEntity)
{
    float scale = 1.f;
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
    auto meshManager = GetManager<IMeshComponentManager>(ecs);
    auto transformManager = GetManager<ITransformComponentManager>(ecs);
    if (!renderMeshManager || !meshManager || !transformManager) {
        return scale;
    }

    Entity dstMeshEntity;
    Entity srcMeshEntity;
    if (auto renderMeshComponent = renderMeshManager->Read(targetEntity); renderMeshComponent) {
        dstMeshEntity = renderMeshComponent->mesh;
    }
    if (auto renderMeshComponent = renderMeshManager->Read(sourceEntity); renderMeshComponent) {
        srcMeshEntity = renderMeshComponent->mesh;
    }
    if (!EntityUtil::IsValid(dstMeshEntity) || !EntityUtil::IsValid(srcMeshEntity)) {
        return scale;
    }
    auto getMeshHeight = [](IMeshComponentManager& meshManager, Entity entity) {
        if (auto meshComponent = meshManager.Read(entity); meshComponent) {
            return (meshComponent->aabbMax.y - meshComponent->aabbMin.y);
        }
        return 0.f;
    };

    const float dstHeight = getMeshHeight(*meshManager, dstMeshEntity);
    const float srcHeight = getMeshHeight(*meshManager, srcMeshEntity);
    if ((dstHeight == 0.f) || (srcHeight == 0.f)) {
        return scale;
    }

    auto dstMeshScale = transformManager->Get(targetEntity).scale;
    auto srcMeshScale = transformManager->Get(sourceEntity).scale;
    const auto dstSize = dstHeight * dstMeshScale.y;
    const auto srcSize = srcHeight * srcMeshScale.y;
    if (srcSize == 0.f) {
        return scale;
    }
    scale = dstSize / srcSize;
    return scale;
}

vector<Entity> CreateJointMapping(
    IEcs& ecs, array_view<const Entity> dstJointEntities, array_view<const Entity> srcJointEntities)
{
    vector<Entity> srcToDstJointMapping;
    auto* nameManager = GetManager<INameComponentManager>(ecs);
    if (!nameManager) {
        return srcToDstJointMapping;
    }

    auto getName = [nameManager](const Entity& jointEntity) -> string_view {
        if (auto nameComponent = nameManager->Read(jointEntity); nameComponent) {
            return nameComponent->name;
        }
        return {};
    };

    vector<string_view> dstJointNames;
    dstJointNames.reserve(dstJointEntities.size());
    std::transform(dstJointEntities.begin(), dstJointEntities.end(), std::back_inserter(dstJointNames), getName);

    vector<string_view> srcJointNames;
    srcJointNames.reserve(srcJointEntities.size());
    std::transform(srcJointEntities.begin(), srcJointEntities.end(), std::back_inserter(srcJointNames), getName);

    srcToDstJointMapping.reserve(srcJointEntities.size());

    const auto dstJointNamesBegin = dstJointNames.cbegin();
    const auto dstJointNamesEnd = dstJointNames.cend();
    for (const auto& srcJointName : srcJointNames) {
        const auto pos = std::find(dstJointNamesBegin, dstJointNamesEnd, srcJointName);
        srcToDstJointMapping.push_back(
            (pos != dstJointNamesEnd) ? dstJointEntities[static_cast<size_t>(std::distance(dstJointNamesBegin, pos))]
                                      : Entity {});
        if (pos == dstJointNamesEnd) {
            CORE_LOG_W("Target skin missing joint %s", srcJointName.data());
        }
    }
    return srcToDstJointMapping;
}

vector<Entity> UpdateTracks(IEcs& ecs, array_view<EntityReference> targetTracks,
    array_view<const EntityReference> sourceTracks, array_view<const Entity> srcJointEntities,
    array_view<const Entity> srcToDstJointMapping, float scale)
{
    vector<Entity> trackTargets;
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(ecs);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(ecs);
    if (!animationTrackManager || !animationOutputManager) {
        return trackTargets;
    }
    trackTargets.reserve(sourceTracks.size());
    auto& entityManager = ecs.GetEntityManager();
    // update tracks to point to target skin's joints.
    std::transform(sourceTracks.begin(), sourceTracks.end(), targetTracks.begin(),
        [&entityManager, animationTrackManager, animationOutputManager, &srcJointEntities, &srcToDstJointMapping,
            &trackTargets, scale](const EntityReference& srcTrackEntity) {
            const auto srcTrackId = animationTrackManager->GetComponentId(srcTrackEntity);
            const auto srcTargetEntity = (srcTrackId != IComponentManager::INVALID_COMPONENT_ID)
                                             ? animationTrackManager->Read(srcTrackId)->target
                                             : Entity {};
            if (EntityUtil::IsValid(srcTargetEntity)) {
                // check that the src track target is one of the src joints
                if (const auto pos = std::find(srcJointEntities.begin(), srcJointEntities.end(), srcTargetEntity);
                    pos != srcJointEntities.end()) {
                    auto dstTrackEntity = entityManager.CreateReferenceCounted();
                    animationTrackManager->Create(dstTrackEntity);
                    auto dstTrack = animationTrackManager->Write(dstTrackEntity);
                    auto srcTrack = animationTrackManager->Read(srcTrackId);
                    *dstTrack = *srcTrack;
                    const auto jointIndex = static_cast<size_t>(pos - srcJointEntities.begin());
                    trackTargets.push_back(srcToDstJointMapping[jointIndex]);
                    dstTrack->target = {};

                    // joint position track needs to be offset
                    if ((dstTrack->component == ITransformComponentManager::UID) &&
                        (dstTrack->property == "position")) {
                        if (const auto srcOutputId = animationOutputManager->GetComponentId(srcTrack->data);
                            srcOutputId != IComponentManager::INVALID_COMPONENT_ID) {
                            // create new animation output with original position corrected by the scale.
                            dstTrack->data = entityManager.CreateReferenceCounted();
                            animationOutputManager->Create(dstTrack->data);
                            const auto dstOutput = animationOutputManager->Write(dstTrack->data);
                            const auto srcOutput = animationOutputManager->Read(srcOutputId);
                            dstOutput->type = srcOutput->type;
                            auto& dst = dstOutput->data;
                            const auto& src = srcOutput->data;
                            dst.resize(src.size());
                            const auto count = dst.size() / sizeof(Math::Vec3);
                            const auto srcPositions =
                                array_view(reinterpret_cast<const Math::Vec3*>(src.data()), count);
                            auto dstPositions = array_view(reinterpret_cast<Math::Vec3*>(dst.data()), count);
                            std::transform(srcPositions.begin(), srcPositions.end(), dstPositions.begin(),
                                [scale](const Math::Vec3 position) { return position * scale; });
                        }
                    }
                    return dstTrackEntity;
                }
            }
            CORE_LOG_W("no target for track %" PRIx64, static_cast<Entity>(srcTrackEntity).id);
            return srcTrackEntity;
        });
    return trackTargets;
}
} // namespace

IAnimationPlayback* SceneUtil::RetargetSkinAnimation(
    IEcs& ecs, Entity targetEntity, Entity sourceEntity, Entity animationEntity) const
{
    auto jointsManager = GetManager<ISkinJointsComponentManager>(ecs);
    auto animationManager = GetManager<IAnimationComponentManager>(ecs);
    auto transformManager = GetManager<ITransformComponentManager>(ecs);
    auto animationSystem = GetSystem<IAnimationSystem>(ecs);
    if (!jointsManager || !animationManager || !transformManager || !animationSystem) {
        return nullptr;
    }
    auto dstJointsComponent = jointsManager->Read(targetEntity);
    auto srcJointsComponent = jointsManager->Read(sourceEntity);
    if (!dstJointsComponent || !srcJointsComponent) {
        return nullptr;
    }
    if (!animationManager->HasComponent(animationEntity)) {
        return nullptr;
    }

    auto dstJointEntities = array_view(dstJointsComponent->jointEntities, dstJointsComponent->count);
    auto srcJointEntities = array_view(srcJointsComponent->jointEntities, srcJointsComponent->count);

    const vector<Entity> srcToDstJointMapping = CreateJointMapping(ecs, dstJointEntities, srcJointEntities);

    // calculate a scaling factor based on mesh heights
    const auto scale = CalculateScalingFactor(ecs, targetEntity, sourceEntity);

    // compensate difference in root joint positions.
    // synchronize initial pose by copying src joint rotations to dst joints.
    {
        auto srcIt = srcJointEntities.begin();
        for (const auto& dstE : srcToDstJointMapping) {
            auto srcTransform = transformManager->Read(*srcIt);
            auto dstTransform = transformManager->Write(dstE);
            if (srcTransform && dstTransform) {
                dstTransform->position = srcTransform->position * scale;
                dstTransform->rotation = srcTransform->rotation;
            }
            ++srcIt;
        }
    }

    auto& entityManager = ecs.GetEntityManager();
    const auto dstAnimationEntity = entityManager.Create();
    if (auto* nameManager = GetManager<INameComponentManager>(ecs)) {
        nameManager->Create(dstAnimationEntity);
        auto srcName = nameManager->Read(animationEntity);
        nameManager->Write(dstAnimationEntity)->name =
            (srcName ? srcName->name : string_view("<empty>")) + " retargeted";
    }

    vector<Entity> trackTargets;
    {
        animationManager->Create(dstAnimationEntity);
        auto dstAnimationComponent = animationManager->Write(dstAnimationEntity);
        auto srcAnimationComponent = animationManager->Read(animationEntity);
        if (!srcAnimationComponent) {
            return nullptr;
        }
        // copy the src animation.
        *dstAnimationComponent = *srcAnimationComponent;
        trackTargets = UpdateTracks(ecs, dstAnimationComponent->tracks, srcAnimationComponent->tracks, srcJointEntities,
            srcToDstJointMapping, scale);
    }

    return animationSystem->CreatePlayback(dstAnimationEntity, trackTargets);
}

void SceneUtil::GetDefaultMaterialShaderData(IEcs& ecs, const ISceneUtil::MaterialShaderInfo& info,
    MaterialComponent::Shader& materialShader, MaterialComponent::Shader& depthShader) const
{
    auto* renderHandleMgr = GetManager<IRenderHandleComponentManager>(ecs);
    if (renderHandleMgr) {
        IEntityManager& entityMgr = ecs.GetEntityManager();
        const IShaderManager& shaderMgr = graphicsContext_.GetRenderContext().GetDevice().GetShaderManager();
        {
            const uint32_t renderSlotId =
                (info.alphaBlend)
                    ? shaderMgr.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT)
                    : shaderMgr.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);

            const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(renderSlotId);
            materialShader.shader = GetOrCreateEntityReference(entityMgr, *renderHandleMgr, rsd.shader);
            RENDER_NS::GraphicsState gs = shaderMgr.GetGraphicsState(rsd.graphicsState);
            gs.rasterizationState.cullModeFlags = info.cullModeFlags;
            gs.rasterizationState.frontFace = info.frontFace;
            const uint64_t gsHash = shaderMgr.HashGraphicsState(gs);
            const RenderHandleReference gsHandle = shaderMgr.GetGraphicsStateHandleByHash(gsHash);
            if (gsHandle) {
                materialShader.graphicsState = GetOrCreateEntityReference(entityMgr, *renderHandleMgr, gsHandle);
            }
        }
        if (!info.alphaBlend) {
            const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH);
            const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(renderSlotId);
            depthShader.shader = GetOrCreateEntityReference(entityMgr, *renderHandleMgr, rsd.shader);
            RENDER_NS::GraphicsState gs = shaderMgr.GetGraphicsState(rsd.graphicsState);
            gs.rasterizationState.cullModeFlags = info.cullModeFlags;
            gs.rasterizationState.frontFace = info.frontFace;
            const uint64_t gsHash = shaderMgr.HashGraphicsState(gs);
            const RenderHandleReference gsHandle = shaderMgr.GetGraphicsStateHandleByHash(gsHash);
            if (gsHandle) {
                depthShader.graphicsState = GetOrCreateEntityReference(entityMgr, *renderHandleMgr, gsHandle);
            }
        }
    }
}

void SceneUtil::GetDefaultMaterialShaderData(IEcs& ecs, const ISceneUtil::MaterialShaderInfo& info,
    const string_view renderSlot, MaterialComponent::Shader& shader) const
{
    auto* renderHandleMgr = GetManager<IRenderHandleComponentManager>(ecs);
    if (renderHandleMgr) {
        IEntityManager& entityMgr = ecs.GetEntityManager();
        const IShaderManager& shaderMgr = graphicsContext_.GetRenderContext().GetDevice().GetShaderManager();
        const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(renderSlot);
        if (renderSlotId != ~0u) {
            const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(renderSlotId);
            if (rsd.shader) {
                shader.shader = GetOrCreateEntityReference(entityMgr, *renderHandleMgr, rsd.shader);
            } else {
                CORE_LOG_D("SceneUtil: render slot base shader not found (%s)", renderSlot.data());
            }
            if (rsd.graphicsState) {
                RENDER_NS::GraphicsState gs = shaderMgr.GetGraphicsState(rsd.graphicsState);
                gs.rasterizationState.cullModeFlags = info.cullModeFlags;
                gs.rasterizationState.frontFace = info.frontFace;
                const uint64_t gsHash = shaderMgr.HashGraphicsState(gs);
                const RenderHandleReference gsHandle = shaderMgr.GetGraphicsStateHandleByHash(gsHash);
                shader.graphicsState = GetOrCreateEntityReference(entityMgr, *renderHandleMgr, gsHandle);
            } else {
                CORE_LOG_D("SceneUtil: render slot base graphics state not found (%s)", renderSlot.data());
            }
        } else {
            CORE_LOG_W("SceneUtil: render slot id not found (%s)", renderSlot.data());
        }
    }
}

void SceneUtil::ShareSkin(IEcs& ecs, Entity targetEntity, Entity sourceEntity) const
{
    auto jointsManager = GetManager<ISkinJointsComponentManager>(ecs);
    if (!jointsManager) {
        CORE_LOG_E("Missing ISkinJointsComponentManager.");
        return;
    }
    vector<Entity> dstToSrcJointMapping;
    {
        auto dstJointsComponent = jointsManager->Read(targetEntity);
        auto srcJointsComponent = jointsManager->Read(sourceEntity);
        if (!dstJointsComponent) {
            CORE_LOG_E("target doesn't have SkinJointsComponent.");
            return;
        }
        if (!srcJointsComponent) {
            CORE_LOG_E("source doesn't have SkinJointsComponent.");
            return;
        }

        auto dstJointEntities = array_view(dstJointsComponent->jointEntities, dstJointsComponent->count);
        auto srcJointEntities = array_view(srcJointsComponent->jointEntities, srcJointsComponent->count);

        dstToSrcJointMapping = CreateJointMapping(ecs, srcJointEntities, dstJointEntities);
        if (dstJointsComponent->count != dstToSrcJointMapping.size()) {
            CORE_LOG_E("couldn't match all joints.");
        }
    }
    auto dstJointsComponent = jointsManager->Write(targetEntity);
    const auto jointCount = Math::min(dstToSrcJointMapping.size(), countof(dstJointsComponent->jointEntities));
    std::copy(dstToSrcJointMapping.data(), dstToSrcJointMapping.data() + jointCount, dstJointsComponent->jointEntities);
}

void SceneUtil::RegisterSceneLoader(const ISceneLoader::Ptr& loader)
{
    if (auto pos = std::find_if(sceneLoaders_.cbegin(), sceneLoaders_.cend(),
            [newLoader = loader.get()](const ISceneLoader::Ptr& registered) { return registered.get() == newLoader; });
        pos == sceneLoaders_.cend()) {
        sceneLoaders_.push_back(loader);
    }
}

void SceneUtil::UnregisterSceneLoader(const ISceneLoader::Ptr& loader)
{
    if (auto pos = std::find_if(sceneLoaders_.cbegin(), sceneLoaders_.cend(),
            [toBeRemoved = loader.get()](
                const ISceneLoader::Ptr& registered) { return registered.get() == toBeRemoved; });
        pos != sceneLoaders_.cend()) {
        sceneLoaders_.erase(pos);
    }
}

ISceneLoader::Ptr SceneUtil::GetSceneLoader(BASE_NS::string_view uri) const
{
    for (auto& sceneLoader : sceneLoaders_) {
        for (const auto& extension : sceneLoader->GetSupportedExtensions()) {
            if (uri.ends_with(extension)) {
                return sceneLoader;
            }
        }
    }
    return {};
}

namespace {
EntityReference CreateTargetGpuImageEnv(IGpuResourceManager& gpuResourceMgr, IEcs& ecs, const Math::UVec2& size)
{
    auto* renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    if (!renderHandleManager) {
        return {};
    }
    GpuImageDesc envDesc { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_CUBE, BASE_FORMAT_B10G11R11_UFLOAT_PACK32,
        CORE_IMAGE_TILING_OPTIMAL, CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
        CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, size.x, size.y, 1U, 1U, 6U, CORE_SAMPLE_COUNT_1_BIT,
        ComponentMapping {} };
    auto handle = gpuResourceMgr.Create(envDesc);
    return GetOrCreateEntityReference(ecs.GetEntityManager(), *renderHandleManager, handle);
}

EntityReference CreateTargetGpuImageDepth(IGpuResourceManager& gpuResourceMgr, IEcs& ecs, const Math::UVec2& size)
{
    auto* renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    if (!renderHandleManager) {
        return {};
    }
    GpuImageDesc depthDesc { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_CUBE, BASE_FORMAT_D16_UNORM,
        CORE_IMAGE_TILING_OPTIMAL,
        CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
        CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, size.x, size.y, 1U, 1U, 6U,
        CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };
    auto handle = gpuResourceMgr.Create(depthDesc);
    return GetOrCreateEntityReference(ecs.GetEntityManager(), *renderHandleManager, handle);
}

CORE_NS::EntityReference CreateReflectionProbeCamera(
    CORE_NS::IEcs& ecs, IGraphicsContext& graphicsContext, const BASE_NS::Math::Vec3& position)
{
    auto ccm = GetManager<ICameraComponentManager>(ecs);
    auto ppcm = GetManager<IPostProcessComponentManager>(ecs);
    auto tcm = GetManager<ITransformComponentManager>(ecs);
    auto wmm = GetManager<IWorldMatrixComponentManager>(ecs);
    auto lmm = GetManager<ILocalMatrixComponentManager>(ecs);
    auto ncm = GetManager<INodeComponentManager>(ecs);
    if (!ccm || !ppcm || !tcm || !wmm || !lmm || !ncm) {
        return {};
    }
    EntityReference camera = ecs.GetEntityManager().CreateReferenceCounted();

    CameraComponent cc;
    auto& gpuResourceMgr = graphicsContext.GetRenderContext().GetDevice().GetGpuResourceManager();

    auto envEntityRef =
        CreateTargetGpuImageEnv(gpuResourceMgr, ecs, { REFLECTION_PROBE_DEFAULT_SIZE, REFLECTION_PROBE_DEFAULT_SIZE });
    auto depthEntityRef = CreateTargetGpuImageDepth(
        gpuResourceMgr, ecs, { REFLECTION_PROBE_DEFAULT_SIZE, REFLECTION_PROBE_DEFAULT_SIZE });
    cc.customColorTargets.push_back(envEntityRef);
    cc.customDepthTarget = depthEntityRef;
    cc.pipelineFlags = CameraComponent::PipelineFlagBits::CUBEMAP_BIT |
                       CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT |
                       CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT;
    cc.renderingPipeline = CameraComponent::RenderingPipeline::LIGHT_FORWARD;
    cc.sceneFlags |= CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
    cc.projection = CameraComponent::Projection::PERSPECTIVE;
    cc.postProcess = camera;
    ccm->Set(camera, cc);

    PostProcessComponent ppc;
    ppc.enableFlags = 0;
    ppcm->Set(camera, ppc);

    TransformComponent tc;
    tc.position = position;
    tc.rotation = Math::Quat { 0.f, 0.f, 0.f, 1.f };
    tcm->Set(camera, tc);

    lmm->Create(camera);
    wmm->Create(camera);
    ncm->Create(camera);

    return camera;
}
} // namespace

CORE_NS::EntityReference SceneUtil::CreateReflectionProbe(CORE_NS::IEcs& ecs, const BASE_NS::Math::Vec3& position) const
{
    auto pcm = GetManager<IReflectionProbeComponentManager>(ecs);
    if (!pcm) {
        return {};
    }

    auto camera = CreateReflectionProbeCamera(ecs, graphicsContext_, position);
    if (!camera) {
        return {};
    }

    // cube cam should always have 90 degrees of fov
    const float viewportAngle { 90.0f };
    // orthographic multiplier, should not matter with perspective view but for clarity, setting with no scale.
    const float distortionMultiplier { 0.5f };
    UpdateCameraViewport(ecs, camera, { REFLECTION_PROBE_DEFAULT_SIZE, REFLECTION_PROBE_DEFAULT_SIZE }, true,
        Math::DEG2RAD * viewportAngle, viewportAngle * distortionMultiplier);

    EntityReference probe = ecs.GetEntityManager().CreateReferenceCounted();

    ReflectionProbeComponent pc;
    pc.probeCamera = camera;
    pcm->Set(probe, pc);

    return probe;
}

namespace {
constexpr uint64_t TYPES[] = { CORE_NS::PropertyType::BOOL_T, CORE_NS::PropertyType::CHAR_T,
    CORE_NS::PropertyType::INT8_T, CORE_NS::PropertyType::INT16_T, CORE_NS::PropertyType::INT32_T,
    CORE_NS::PropertyType::INT64_T, CORE_NS::PropertyType::UINT8_T, CORE_NS::PropertyType::UINT16_T,
    CORE_NS::PropertyType::UINT32_T, CORE_NS::PropertyType::UINT64_T,
#ifdef __APPLE__
    CORE_NS::PropertyType::SIZE_T,
#endif
    CORE_NS::PropertyType::FLOAT_T, CORE_NS::PropertyType::DOUBLE_T, CORE_NS::PropertyType::BOOL_ARRAY_T,
    CORE_NS::PropertyType::CHAR_ARRAY_T, CORE_NS::PropertyType::INT8_ARRAY_T, CORE_NS::PropertyType::INT16_ARRAY_T,
    CORE_NS::PropertyType::INT32_ARRAY_T, CORE_NS::PropertyType::INT64_ARRAY_T, CORE_NS::PropertyType::UINT8_ARRAY_T,
    CORE_NS::PropertyType::UINT16_ARRAY_T, CORE_NS::PropertyType::UINT32_ARRAY_T, CORE_NS::PropertyType::UINT64_ARRAY_T,
#ifdef __APPLE__
    CORE_NS::PropertyType::SIZE_ARRAY_T,
#endif
    CORE_NS::PropertyType::FLOAT_ARRAY_T, CORE_NS::PropertyType::DOUBLE_ARRAY_T, CORE_NS::PropertyType::IVEC2_T,
    CORE_NS::PropertyType::IVEC3_T, CORE_NS::PropertyType::IVEC4_T, CORE_NS::PropertyType::VEC2_T,
    CORE_NS::PropertyType::VEC3_T, CORE_NS::PropertyType::VEC4_T, CORE_NS::PropertyType::UVEC2_T,
    CORE_NS::PropertyType::UVEC3_T, CORE_NS::PropertyType::UVEC4_T, CORE_NS::PropertyType::QUAT_T,
    CORE_NS::PropertyType::MAT3X3_T, CORE_NS::PropertyType::MAT4X4_T, CORE_NS::PropertyType::UID_T,
    CORE_NS::PropertyType::STRING_T, CORE_NS::PropertyType::IVEC2_ARRAY_T, CORE_NS::PropertyType::IVEC3_ARRAY_T,
    CORE_NS::PropertyType::IVEC4_ARRAY_T, CORE_NS::PropertyType::VEC2_ARRAY_T, CORE_NS::PropertyType::VEC3_ARRAY_T,
    CORE_NS::PropertyType::VEC4_ARRAY_T, CORE_NS::PropertyType::UVEC2_ARRAY_T, CORE_NS::PropertyType::UVEC3_ARRAY_T,
    CORE_NS::PropertyType::UVEC4_ARRAY_T, CORE_NS::PropertyType::QUAT_ARRAY_T, CORE_NS::PropertyType::MAT3X3_ARRAY_T,
    CORE_NS::PropertyType::MAT4X4_ARRAY_T, CORE_NS::PropertyType::UID_ARRAY_T, CORE_NS::PropertyType::FLOAT_VECTOR_T,
    CORE_NS::PropertyType::MAT4X4_VECTOR_T };

template<typename EntityHandler, typename EntityReferenceHandler>
void FindEntities(const CORE_NS::Property& property, uintptr_t offset, EntityHandler&& entityFunc,
    EntityReferenceHandler&& entityRefFunc)
{
    if (property.type == CORE_NS::PropertyType::ENTITY_T) {
        auto ptr = reinterpret_cast<CORE_NS::Entity*>(offset);
        if (ptr && EntityUtil::IsValid(*ptr)) {
            entityFunc(ptr);
        }
    } else if (property.type == CORE_NS::PropertyType::ENTITY_REFERENCE_T) {
        auto ptr = reinterpret_cast<CORE_NS::EntityReference*>(offset);
        if (ptr && EntityUtil::IsValid(*ptr)) {
            entityRefFunc(ptr);
        }
    } else if (std::any_of(std::begin(TYPES), std::end(TYPES),
        [&current = property.type](const uint64_t type) {
            return type == current;})) {
        // One of the basic types so no further processing needed.
    } else if (property.metaData.containerMethods) {
        auto& containerProperty = property.metaData.containerMethods->property;
        if (property.type.isArray) {
            // Array of properties.
            for (size_t i = 0; i < property.count; i++) {
                uintptr_t ptr = offset + i * containerProperty.size;
                FindEntities(containerProperty, ptr, BASE_NS::forward<EntityHandler>(entityFunc),
                    BASE_NS::forward<EntityReferenceHandler>(entityRefFunc));
            }
        } else {
            // This is a "non trivial container"
            // (So it needs to read the data and not just the metadata to figure out the data structure).
            const auto count = property.metaData.containerMethods->size(offset);
            for (size_t i = 0; i < count; i++) {
                uintptr_t ptr = property.metaData.containerMethods->get(offset, i);
                FindEntities(containerProperty, ptr, BASE_NS::forward<EntityHandler>(entityFunc),
                    BASE_NS::forward<EntityReferenceHandler>(entityRefFunc));
            }
        }
    } else if (!property.metaData.memberProperties.empty()) {
        // Custom type (struct). Process sub properties recursively.
        for (size_t i = 0; i < property.count; i++) {
            const auto ptr = offset;
            offset += property.size / property.count;
            for (const auto& child : property.metaData.memberProperties) {
                FindEntities(child, ptr + child.offset, BASE_NS::forward<EntityHandler>(entityFunc),
                    BASE_NS::forward<EntityReferenceHandler>(entityRefFunc));
            }
        }
    }
}

vector<Entity> GatherEntities(const IEcs& source, const Entity sourceEntity)
{
    vector<Entity> entities;
    if (!CORE_NS::EntityUtil::IsValid(sourceEntity)) {
        return entities;
    }
    auto nodeSystem = GetSystem<INodeSystem>(source);
    if (!nodeSystem) {
        CORE_LOG_W("Missing INodeSystem");
        return entities;
    }
    vector<IComponentManager*> managers;

    vector<Entity> stack;
    stack.push_back(sourceEntity);
    while (!stack.empty()) {
        const Entity entity = stack.back();
        stack.pop_back();

        if (auto pos = LowerBound(entities.cbegin(), entities.cend(), entity);
            (pos != entities.cend()) && (*pos == entity)) {
            continue;
        } else {
            entities.insert(pos, entity);
        }
        source.GetComponents(entity, managers);
        for (const auto& srcManager : managers) {
            if (srcManager->GetUid() == INodeComponentManager::UID) {
                // for the hierarchy we need either ask the children via NodeSystem/SceneNode or construct the hierarchy
                // based on parent Entity.
                if (auto sceneNode = nodeSystem->GetNode(entity)) {
                    auto children = sceneNode->GetChildren();
                    stack.reserve(stack.size() + children.size());
                    for (const auto child : children) {
                        stack.push_back(child->GetEntity());
                    }
                }
            } else if (auto* data = srcManager->GetData(entity)) {
                // for other component types it's fine to go through all the properties.
                if (const auto base = reinterpret_cast<uintptr_t>(data->RLock())) {
                    for (const auto& property : data->Owner()->MetaData()) {
                        FindEntities(
                            property, base + property.offset,
                            [&stack](const Entity* entity) { stack.push_back(*entity); },
                            [&stack](const EntityReference* entity) { stack.push_back(*entity); });
                    }
                }
                data->RUnlock();
            }
        }
    }

    return entities;
}

vector<Entity> GatherEntities(const IEcs& source)
{
    vector<Entity> entities;
    auto& entityManager = source.GetEntityManager();
    auto it = entityManager.Begin();
    if (!it || it->Compare(entityManager.End())) {
        return entities;
    }
    do {
        entities.push_back(it->Get());
    } while (it->Next());
    return entities;
}

void UpdateEntities(
    IComponentManager* manager, Entity entity, const BASE_NS::unordered_map<CORE_NS::Entity, CORE_NS::Entity>& oldToNew)
{
    auto* data = manager->GetData(entity);
    if (!data) {
        return;
    }
    const auto base = reinterpret_cast<uintptr_t>(data->RLock());
    if (!base) {
        data->RUnlock();
        return;
    }
    // as multiple properties can point to the same value, use set to track which have been updates.
    std::unordered_set<uintptr_t> updatedProperties;
    auto& em = manager->GetEcs().GetEntityManager();
    for (const auto& property : data->Owner()->MetaData()) {
        FindEntities(
            property, base + property.offset,
            [oldToNew, data, &updatedProperties](Entity* entity) {
                if (updatedProperties.count(reinterpret_cast<uintptr_t>(entity))) {
                    return;
                }
                updatedProperties.insert(reinterpret_cast<uintptr_t>(entity));
                if (const auto it = oldToNew.find(*entity); it != oldToNew.end()) {
                    data->WLock();
                    *entity = it->second;
                    data->WUnlock();
                } else {
                    CORE_LOG_D("couldn't find %s", to_hex(entity->id).data());
                }
            },
            [oldToNew, data, &updatedProperties, &em](EntityReference* entity) {
                if (updatedProperties.count(reinterpret_cast<uintptr_t>(entity))) {
                    return;
                }
                updatedProperties.insert(reinterpret_cast<uintptr_t>(entity));
                if (const auto it = oldToNew.find(*entity); it != oldToNew.end()) {
                    data->WLock();
                    *entity = em.GetReferenceCounted(it->second);
                    data->WUnlock();
                } else {
                    CORE_LOG_D("couldn't find %s", to_hex(static_cast<const Entity>(*entity).id).data());
                }
            });
    }
    data->RUnlock();
}

struct CloneResults {
    vector<Entity> newEntities;
    unordered_map<Entity, Entity> oldToNew;
};

CloneResults CloneEntities(IEcs& destination, const IEcs& source, vector<Entity> srcEntities)
{
    unordered_map<Entity, Entity> srcToDst;
    vector<Entity> newEntities;
    newEntities.reserve(srcEntities.size());
    vector<IComponentManager*> managers;
    while (!srcEntities.empty()) {
        const Entity srcEntity = srcEntities.back();
        srcEntities.pop_back();

        const Entity dstEntity = destination.GetEntityManager().Create();
        newEntities.push_back(dstEntity);
        srcToDst[srcEntity] = dstEntity;

        source.GetComponents(srcEntity, managers);
        for (const auto& srcManager : managers) {
            const auto* srcData = srcManager->GetData(srcEntity);
            if (!srcData) {
                continue;
            }
            auto* dstManager = destination.GetComponentManager(srcManager->GetUid());
            if (!dstManager) {
                CORE_LOG_W("ComponentManager %s missing from destination.", to_string(srcManager->GetUid()).data());
                continue;
            }
            dstManager->Create(dstEntity);
            dstManager->SetData(dstEntity, *srcData);
        }
    }

    for (const auto& newEntity : newEntities) {
        destination.GetComponents(newEntity, managers);
        for (const auto& dstManager : managers) {
            UpdateEntities(dstManager, newEntity, srcToDst);
        }
    }
    return CloneResults { BASE_NS::move(newEntities), BASE_NS::move(srcToDst) };
}
} // namespace

Entity SceneUtil::Clone(
    IEcs& destination, const Entity parentEntity, const IEcs& source, const Entity sourceEntity) const
{
    if (&destination == &source) {
        return {};
    }
    if (CORE_NS::EntityUtil::IsValid(parentEntity) && !destination.GetEntityManager().IsAlive(parentEntity)) {
        return {};
    }
    if (!CORE_NS::EntityUtil::IsValid(sourceEntity) || !source.GetEntityManager().IsAlive(sourceEntity)) {
        return {};
    }
    auto result = CloneEntities(destination, source, GatherEntities(source, sourceEntity));
    Entity destinationEntity = result.oldToNew[sourceEntity];
    auto* nodeManager = GetManager<INodeComponentManager>(destination);
    if (nodeManager) {
        auto handle = nodeManager->Write(destinationEntity);
        if (handle) {
            handle->parent = parentEntity;
        }
    } else {
        CORE_LOG_W("Failed to reparent: missing INodeComponentManager");
    }
    return destinationEntity;
}

vector<Entity> SceneUtil::Clone(IEcs& destination, const IEcs& source) const
{
    if (&destination == &source) {
        return {};
    }

    auto result = CloneEntities(destination, source, GatherEntities(source));
    return result.newEntities;
}
CORE3D_END_NAMESPACE()
