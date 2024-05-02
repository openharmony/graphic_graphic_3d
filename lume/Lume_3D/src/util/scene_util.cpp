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

#include <cinttypes>

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
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_util.h>
#include <base/containers/fixed_string.h>
#include <base/math/matrix_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/property/intf_property_handle.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>

#include "uri_lookup.h"
#include "util/component_util_functions.h"
#include "util/string_util.h"

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
    IEntityManager& em = ecs.GetEntityManager();
    const Entity camera = em.Create();

    auto lmm = GetManager<ILocalMatrixComponentManager>(ecs);
    lmm->Create(camera);

    auto wmm = GetManager<IWorldMatrixComponentManager>(ecs);
    wmm->Create(camera);

    auto ncm = GetManager<INodeComponentManager>(ecs);
    ncm->Create(camera);

    auto tcm = GetManager<ITransformComponentManager>(ecs);
    TransformComponent tc;
    tc.position = position;
    tc.rotation = rotation;
    tcm->Set(camera, tc);

    auto ccm = GetManager<ICameraComponentManager>(ecs);
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
    auto parentWorld = Math::Mat4X4(1.f);

    auto getParent = [ncm = GetManager<INodeComponentManager>(ecs)](Entity entity) {
        if (auto nodeHandle = ncm->Read(entity)) {
            return nodeHandle->parent;
        }
        return Entity {};
    };

    // walk up the hierachy to get an up-to-date world matrix.
    auto tcm = GetManager<ITransformComponentManager>(ecs);
    for (Entity node = getParent(entity); EntityUtil::IsValid(node); node = getParent(node)) {
        if (auto parentTransformHandle = tcm->Read(node)) {
            parentWorld = Math::Trs(parentTransformHandle->position, parentTransformHandle->rotation,
                              parentTransformHandle->scale) *
                          parentWorld;
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
    IEntityManager& em = ecs.GetEntityManager();
    const Entity light = em.Create();

    auto lmm = GetManager<ILocalMatrixComponentManager>(ecs);
    lmm->Create(light);

    auto wmm = GetManager<IWorldMatrixComponentManager>(ecs);
    wmm->Create(light);

    auto ncm = GetManager<INodeComponentManager>(ecs);
    ncm->Create(light);

    auto nameM = GetManager<INameComponentManager>(ecs);
    nameM->Create(light);
    constexpr string_view lightName("Light");
    nameM->Write(light)->name = lightName;

    auto tcm = GetManager<ITransformComponentManager>(ecs);
    TransformComponent tc;
    tc.position = position;
    tc.rotation = rotation;
    tcm->Set(light, tc);

    auto lcm = GetManager<ILightComponentManager>(ecs);
    LightComponent lc = lightComponent;
    lc.shadowEnabled = (lc.type == LightComponent::Type::POINT) ? false : lc.shadowEnabled;
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
    IRenderMeshComponentManager* renderMeshCM = GetManager<IRenderMeshComponentManager>(ecs);
    IMeshComponentManager* meshCM = GetManager<IMeshComponentManager>(ecs);
    IMaterialComponentManager* matCM = GetManager<IMaterialComponentManager>(ecs);
    IRenderHandleComponentManager* gpuHandleCM = GetManager<IRenderHandleComponentManager>(ecs);

    INameComponentManager* nameCM = GetManager<INameComponentManager>(ecs);
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

    if (const auto rmcHandle = renderMeshCM->Read(nodeEntity); rmcHandle) {
        const RenderMeshComponent& rmc = *rmcHandle;
        if (const auto meshHandle = meshCM->Read(rmc.mesh); meshHandle) {
            const MeshComponent& meshComponent = *meshHandle;
            if (!meshComponent.submeshes.empty()) {
                if (auto matHandle = matCM->Write(meshComponent.submeshes[0].material); matHandle) {
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

                    matComponent.textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].image =
                        plane.colorTarget;
                    matComponent.extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::DISCARD_BIT;
                }
            }
        }
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

    auto getName = [nameManager = GetManager<INameComponentManager>(ecs)](const Entity& jointEntity) -> string_view {
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
    trackTargets.reserve(sourceTracks.size());
    auto& entityManager = ecs.GetEntityManager();
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(ecs);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(ecs);
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

IAnimationPlayback* SceneUtil::RetargetSkinAnimation(
    IEcs& ecs, Entity targetEntity, Entity sourceEntity, Entity animationEntity) const
{
    auto jointsManager = GetManager<ISkinJointsComponentManager>(ecs);
    auto dstJointsComponent = jointsManager->Read(targetEntity);
    auto srcJointsComponent = jointsManager->Read(sourceEntity);
    if (!dstJointsComponent || !srcJointsComponent) {
        return nullptr;
    }
    if (!GetManager<IAnimationComponentManager>(ecs)->HasComponent(animationEntity)) {
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
        auto transformManager = GetManager<ITransformComponentManager>(ecs);
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
    {
        INameComponentManager* nameManager = GetManager<INameComponentManager>(ecs);
        nameManager->Create(dstAnimationEntity);
        auto srcName = nameManager->Read(animationEntity);
        nameManager->Write(dstAnimationEntity)->name =
            (srcName ? srcName->name : string_view("<empty>")) + " retargeted";
    }

    vector<Entity> trackTargets;
    {
        auto animationManager = GetManager<IAnimationComponentManager>(ecs);
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
    auto animationSystem = GetSystem<IAnimationSystem>(ecs);
    return animationSystem->CreatePlayback(dstAnimationEntity, trackTargets);
}

void SceneUtil::GetDefaultMaterialShaderData(IEcs& ecs, const ISceneUtil::MaterialShaderInfo& info,
    MaterialComponent::Shader& materialShader, MaterialComponent::Shader& depthShader) const
{
    IRenderHandleComponentManager* renderHandleMgr = GetManager<IRenderHandleComponentManager>(ecs);
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
    IRenderHandleComponentManager* renderHandleMgr = GetManager<IRenderHandleComponentManager>(ecs);
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
    vector<Entity> dstToSrcJointMapping;

    auto jointsManager = GetManager<ISkinJointsComponentManager>(ecs);
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
    std::copy(dstToSrcJointMapping.data(), dstToSrcJointMapping.data() + dstToSrcJointMapping.size(),
        dstJointsComponent->jointEntities);
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
CORE3D_END_NAMESPACE()
