/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "BoidsSwarmImpl.h"

#include <boids_swarm/ecs/components/boids_swarm_component.h>
#include <boids_swarm/ecs/components/boids_swarm_gravity_component.h>
#include <boids_swarm/ecs/components/boids_swarm_repulsion_component.h>
#include <boids_swarm/ecs/systems/intf_boids_swarm_system.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <meta/api/make_callback.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/property/intf_property.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_scene.h>

#include "SceneImpl.h"
#include "NodeImpl.h"
#include "SceneResourceImpl.h"
#include "Vec3Impl.h"
#include "QuaternionImpl.h"
#include "Utils.h"
#include "taihe/include/ANIUtils.h"

using f64 = double;

namespace OHOS::Render3D::KITETS {

namespace {

SCENE_NS::IComponent::Ptr FindComponentOnNode(
    const SCENE_NS::INode::Ptr& node, BASE_NS::string_view componentName)
{
    if (!node) {
        return {};
    }
    auto attach = interface_cast<META_NS::IAttach>(node);
    if (!attach) {
        return {};
    }
    auto cont = attach->GetAttachmentContainer(false);
    if (!cont) {
        return {};
    }
    auto comp = cont->FindByName<SCENE_NS::IComponent>(componentName);
    if (!comp) {
        return {};
    }
    return comp;
}

auto GetBoidsSystem(const SCENE_NS::IScene::Ptr& scene) -> BOIDSSWARM_NS::IBoidsSwarmSystem*
{
    if (!scene) {
        return nullptr;
    }
    auto internalScene = scene->GetInternalScene();
    if (!internalScene) {
        return nullptr;
    }
    auto ecs = internalScene->GetEcsContext().GetNativeEcs();
    if (!ecs) {
        return nullptr;
    }
    return CORE_NS::GetSystem<BOIDSSWARM_NS::IBoidsSwarmSystem>(*ecs);
}

SCENE_NS::INode::Ptr GetINodeFromTaiheNode(::SceneNodes::weak::Node node)
{
    if (node.is_error()) {
        return {};
    }
    auto nodeOptional = static_cast<::SceneResources::SceneResource>(node)->getImpl();
    if (!nodeOptional.has_value()) {
        return {};
    }
    auto* nodeImpl = reinterpret_cast<NodeImpl*>(nodeOptional.value());
    if (!nodeImpl) {
        return {};
    }
    auto nodeETS = nodeImpl->GetInternalNode();
    if (!nodeETS) {
        return {};
    }
    return nodeETS->GetInternalNode();
}

SCENE_NS::IScene::Ptr GetISceneFromTaiheScene(::SceneTH::weak::Scene scene)
{
    if (scene.is_error()) {
        return {};
    }
    auto sceneOptional = scene->getImpl();
    if (!sceneOptional.has_value()) {
        return {};
    }
    auto* sceneImpl = reinterpret_cast<SceneImpl*>(sceneOptional.value());
    if (!sceneImpl) {
        return {};
    }
    auto sceneETS = sceneImpl->getInternalScene();
    if (!sceneETS) {
        return {};
    }
    return sceneETS->GetNativeScene();
}

void SetFloatProperty(
    const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName, BASE_NS::string_view propName, float val)
{
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return;
    }
    auto fullName = BASE_NS::string(compName) + "." + BASE_NS::string(propName);
    auto p = meta->GetProperty(fullName);
    if (!p) {
        return;
    }
    if (META_NS::IsCompatibleWith<float>(p)) {
        META_NS::SetValue(META_NS::Property<float>(p), val);
    }
}

void SetVec3Property(const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName,
    BASE_NS::string_view propName, const BASE_NS::Math::Vec3& val)
{
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return;
    }
    auto fullName = BASE_NS::string(compName) + "." + BASE_NS::string(propName);
    auto p = meta->GetProperty(fullName);
    if (!p) {
        return;
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(p)) {
        META_NS::SetValue(META_NS::Property<BASE_NS::Math::Vec3>(p), val);
    }
}

void SetQuatProperty(const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName,
    BASE_NS::string_view propName, const BASE_NS::Math::Quat& val)
{
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return;
    }
    auto fullName = BASE_NS::string(compName) + "." + BASE_NS::string(propName);
    auto p = meta->GetProperty(fullName);
    if (!p) {
        return;
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Quat>(p)) {
        META_NS::SetValue(META_NS::Property<BASE_NS::Math::Quat>(p), val);
    }
}

float ReadFloatProperty(
    const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName, BASE_NS::string_view propName)
{
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return 0.0f;
    }
    auto fullName = BASE_NS::string(compName) + "." + BASE_NS::string(propName);
    auto p = meta->GetProperty(fullName);
    if (!p) {
        return 0.0f;
    }
    if (META_NS::IsCompatibleWith<float>(p)) {
        return META_NS::GetValue(META_NS::Property<float>(p));
    }
    return 0.0f;
}

BASE_NS::Math::Vec3 ReadVec3Property(
    const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName, BASE_NS::string_view propName)
{
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return BASE_NS::Math::ZERO_VEC3;
    }
    auto fullName = BASE_NS::string(compName) + "." + BASE_NS::string(propName);
    auto p = meta->GetProperty(fullName);
    if (!p) {
        return BASE_NS::Math::ZERO_VEC3;
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(p)) {
        return META_NS::GetValue(META_NS::Property<BASE_NS::Math::Vec3>(p));
    }
    return BASE_NS::Math::ZERO_VEC3;
}

BASE_NS::Math::Quat ReadQuatProperty(
    const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName, BASE_NS::string_view propName)
{
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return BASE_NS::Math::Quat {};
    }
    auto fullName = BASE_NS::string(compName) + "." + BASE_NS::string(propName);
    auto p = meta->GetProperty(fullName);
    if (!p) {
        return BASE_NS::Math::Quat {};
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Quat>(p)) {
        return META_NS::GetValue(META_NS::Property<BASE_NS::Math::Quat>(p));
    }
    return BASE_NS::Math::Quat {};
}

::SceneTypes::Vec3 MakeTaiheVec3(const BASE_NS::Math::Vec3& v)
{
    return taihe::make_holder<Vec3Impl, ::SceneTypes::Vec3>(v);
}

::SceneTypes::Quaternion MakeTaiheQuat(const BASE_NS::Math::Quat& q)
{
    return taihe::make_holder<QuaternionImpl, ::SceneTypes::Quaternion>(q);
}

void ApplyBoidsSwarmParams(
    const SCENE_NS::IComponent::Ptr& comp, const ::SceneBoidsSwarm::BoidsSimParameters& params)
{
    constexpr BASE_NS::string_view compName = "BoidsSwarmComponent";
    if (params.initialVelocity) {
        auto v = params.initialVelocity.value();
        SetVec3Property(comp, compName, "initialVelocity",
            BASE_NS::Math::Vec3 { static_cast<float>(v->getX()), static_cast<float>(v->getY()),
                static_cast<float>(v->getZ()) });
    }
    if (params.initialPosition) {
        auto v = params.initialPosition.value();
        SetVec3Property(comp, compName, "initialPosition",
            BASE_NS::Math::Vec3 { static_cast<float>(v->getX()), static_cast<float>(v->getY()),
                static_cast<float>(v->getZ()) });
    }
    if (params.initialRotation) {
        auto v = params.initialRotation.value();
        SetQuatProperty(comp, compName, "initialRotation",
            BASE_NS::Math::Quat { static_cast<float>(v->getX()), static_cast<float>(v->getY()),
                static_cast<float>(v->getZ()), static_cast<float>(v->getW()) });
    }
    if (params.boundaryMinPos) {
        auto v = params.boundaryMinPos.value();
        SetVec3Property(comp, compName, "boundaryMinPos",
            BASE_NS::Math::Vec3 { static_cast<float>(v->getX()), static_cast<float>(v->getY()),
                static_cast<float>(v->getZ()) });
    }
    if (params.boundaryMaxPos) {
        auto v = params.boundaryMaxPos.value();
        SetVec3Property(comp, compName, "boundaryMaxPos",
            BASE_NS::Math::Vec3 { static_cast<float>(v->getX()), static_cast<float>(v->getY()),
                static_cast<float>(v->getZ()) });
    }
    if (params.maxTurnRate) {
        auto v = params.maxTurnRate.value();
        SetVec3Property(comp, compName, "maxTurnRate",
            BASE_NS::Math::Vec3 { static_cast<float>(v->getX()), static_cast<float>(v->getY()),
                static_cast<float>(v->getZ()) });
    }
    if (params.maxVelocityMag) {
        SetFloatProperty(comp, compName, "maxVelocityMag", static_cast<float>(params.maxVelocityMag.value()));
    }
    if (params.maxAccelerationMag) {
        SetFloatProperty(
            comp, compName, "maxAccelerationMag", static_cast<float>(params.maxAccelerationMag.value()));
    }
    if (params.separationWeight) {
        SetFloatProperty(comp, compName, "separationWeight", static_cast<float>(params.separationWeight.value()));
    }
    if (params.separationDistance) {
        SetFloatProperty(
            comp, compName, "separationDistance", static_cast<float>(params.separationDistance.value()));
    }
    if (params.alignmentWeight) {
        SetFloatProperty(comp, compName, "alignmentWeight", static_cast<float>(params.alignmentWeight.value()));
    }
    if (params.alignmentDistance) {
        SetFloatProperty(
            comp, compName, "alignmentDistance", static_cast<float>(params.alignmentDistance.value()));
    }
    if (params.cohesionWeight) {
        SetFloatProperty(comp, compName, "cohesionWeight", static_cast<float>(params.cohesionWeight.value()));
    }
    if (params.cohesionDistance) {
        SetFloatProperty(comp, compName, "cohesionDistance", static_cast<float>(params.cohesionDistance.value()));
    }
    if (params.boundaryWeight) {
        SetFloatProperty(comp, compName, "boundaryWeight", static_cast<float>(params.boundaryWeight.value()));
    }
    if (params.gravityWeight) {
        SetFloatProperty(comp, compName, "gravityWeight", static_cast<float>(params.gravityWeight.value()));
    }
    if (params.repulsionWeight) {
        SetFloatProperty(comp, compName, "repulsionWeight", static_cast<float>(params.repulsionWeight.value()));
    }
    if (params.boundaryDistance) {
        SetFloatProperty(
            comp, compName, "boundaryDistance", static_cast<float>(params.boundaryDistance.value()));
    }
}

void ApplyBoidsSwarmGravityParams(
    const SCENE_NS::IComponent::Ptr& comp, const ::SceneBoidsSwarm::BoidsSimGravityParameters& params)
{
    constexpr BASE_NS::string_view compName = "BoidsSwarmGravityComponent";
    if (params.radius) {
        SetFloatProperty(comp, compName, "radius", static_cast<float>(params.radius.value()));
    }
    if (params.accelerationMag) {
        SetFloatProperty(comp, compName, "accelerationMag", static_cast<float>(params.accelerationMag.value()));
    }
}

void ApplyBoidsSwarmRepulsionParams(
    const SCENE_NS::IComponent::Ptr& comp, const ::SceneBoidsSwarm::BoidsSimRepulsionParameters& params)
{
    constexpr BASE_NS::string_view compName = "BoidsSwarmRepulsionComponent";
    if (params.radius) {
        SetFloatProperty(comp, compName, "radius", static_cast<float>(params.radius.value()));
    }
    if (params.accelerationMag) {
        SetFloatProperty(comp, compName, "accelerationMag", static_cast<float>(params.accelerationMag.value()));
    }
}

} // namespace

BoidsSimWorldImpl::BoidsSimWorldImpl(SCENE_NS::IScene::Ptr scene) : scene_(BASE_NS::move(scene)) {}

BoidsSimWorldImpl::~BoidsSimWorldImpl()
{
    scene_.reset();
}

void BoidsSimWorldImpl::play()
{
    if (!scene_) {
        WIDGET_LOGE("BoidsSimWorldImpl::play: scene unavailable");
        return;
    }
    ExecSyncTask([scene = scene_]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->Play();
        return {};
    });
}

void BoidsSimWorldImpl::pause()
{
    if (!scene_) {
        WIDGET_LOGE("BoidsSimWorldImpl::pause: scene unavailable");
        return;
    }
    ExecSyncTask([scene = scene_]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->Pause();
        return {};
    });
}

void BoidsSimWorldImpl::stop()
{
    if (!scene_) {
        WIDGET_LOGE("BoidsSimWorldImpl::stop: scene unavailable");
        return;
    }
    ExecSyncTask([scene = scene_]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->Stop();
        return {};
    });
}

bool BoidsSimWorldImpl::getIsPlaying()
{
    if (!scene_) {
        WIDGET_LOGE("BoidsSimWorldImpl::getIsPlaying: scene unavailable");
        return false;
    }
    bool isPlaying = false;
    ExecSyncTask([scene = scene_, &isPlaying]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        isPlaying = system->IsPlaying();
        return {};
    });
    return isPlaying;
}

void BoidsSimWorldImpl::addBoidsSimComponent(
    ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimParameters params)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::addBoidsSimComponent: invalid node");
        return;
    }

    auto existing = FindComponentOnNode(iNode, "BoidsSwarmComponent");
    if (existing) {
        WIDGET_LOGE("Component 'BoidsSwarmComponent' already exists on node");
        return;
    }

    auto comp = scene_->CreateComponent(iNode, "BoidsSwarmComponent").GetResult();
    if (!comp) {
        WIDGET_LOGE("Failed to create component 'BoidsSwarmComponent'");
        return;
    }

    comp->PopulateAllProperties();
    ApplyBoidsSwarmParams(comp, params);
}

void BoidsSimWorldImpl::setBoidsSimComponent(
    ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimParameters params)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::setBoidsSimComponent: invalid node");
        return;
    }

    auto comp = FindComponentOnNode(iNode, "BoidsSwarmComponent");
    if (!comp) {
        WIDGET_LOGE("Component 'BoidsSwarmComponent' not found on node");
        return;
    }

    ApplyBoidsSwarmParams(comp, params);
}

::SceneBoidsSwarm::BoidsSimParametersOrNull BoidsSimWorldImpl::getBoidsSimComponent(
    ::SceneNodes::weak::Node node)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::getBoidsSimComponent: invalid node");
        return ::SceneBoidsSwarm::BoidsSimParametersOrNull::make_nValue();
    }

    auto comp = FindComponentOnNode(iNode, "BoidsSwarmComponent");
    if (!comp) {
        WIDGET_LOGE("BoidsSimWorldImpl::getBoidsSimComponent: component 'BoidsSwarmComponent' not found on node");
        return ::SceneBoidsSwarm::BoidsSimParametersOrNull::make_nValue();
    }

    ::SceneBoidsSwarm::BoidsSimParameters result;

    constexpr BASE_NS::string_view compName = "BoidsSwarmComponent";

    result.initialVelocity = ::taihe::optional<::SceneTypes::Vec3>(
        std::in_place, MakeTaiheVec3(ReadVec3Property(comp, compName, "initialVelocity")));
    result.initialPosition = ::taihe::optional<::SceneTypes::Vec3>(
        std::in_place, MakeTaiheVec3(ReadVec3Property(comp, compName, "initialPosition")));
    result.initialRotation = ::taihe::optional<::SceneTypes::Quaternion>(
        std::in_place, MakeTaiheQuat(ReadQuatProperty(comp, compName, "initialRotation")));
    result.boundaryMinPos = ::taihe::optional<::SceneTypes::Vec3>(
        std::in_place, MakeTaiheVec3(ReadVec3Property(comp, compName, "boundaryMinPos")));
    result.boundaryMaxPos = ::taihe::optional<::SceneTypes::Vec3>(
        std::in_place, MakeTaiheVec3(ReadVec3Property(comp, compName, "boundaryMaxPos")));
    result.maxTurnRate = ::taihe::optional<::SceneTypes::Vec3>(
        std::in_place, MakeTaiheVec3(ReadVec3Property(comp, compName, "maxTurnRate")));

    result.maxVelocityMag = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "maxVelocityMag"));
    result.maxAccelerationMag = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "maxAccelerationMag"));
    result.separationWeight = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "separationWeight"));
    result.separationDistance = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "separationDistance"));
    result.alignmentWeight = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "alignmentWeight"));
    result.alignmentDistance = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "alignmentDistance"));
    result.cohesionWeight = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "cohesionWeight"));
    result.cohesionDistance = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "cohesionDistance"));
    result.boundaryWeight = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "boundaryWeight"));
    result.gravityWeight = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "gravityWeight"));
    result.repulsionWeight = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "repulsionWeight"));
    result.boundaryDistance = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "boundaryDistance"));

    return ::SceneBoidsSwarm::BoidsSimParametersOrNull::make_rc(result);
}

void BoidsSimWorldImpl::removeBoidsSimComponent(::SceneNodes::weak::Node node)
{
    RemoveComponent(GetINodeFromTaiheNode(node), "BoidsSwarmComponent",
        BOIDSSWARM_NS::IBoidsSwarmComponentManager::UID);
}

void BoidsSimWorldImpl::addBoidsSimGravityComponent(
    ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimGravityParameters params)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::addBoidsSimGravityComponent: invalid node");
        return;
    }

    auto existing = FindComponentOnNode(iNode, "BoidsSwarmGravityComponent");
    if (existing) {
        WIDGET_LOGE("Component 'BoidsSwarmGravityComponent' already exists on node");
        return;
    }

    auto comp = scene_->CreateComponent(iNode, "BoidsSwarmGravityComponent").GetResult();
    if (!comp) {
        WIDGET_LOGE("Failed to create component 'BoidsSwarmGravityComponent'");
        return;
    }

    comp->PopulateAllProperties();
    ApplyBoidsSwarmGravityParams(comp, params);
}

void BoidsSimWorldImpl::setBoidsSimGravityComponent(
    ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimGravityParameters params)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::setBoidsSimGravityComponent: invalid node");
        return;
    }

    auto comp = FindComponentOnNode(iNode, "BoidsSwarmGravityComponent");
    if (!comp) {
        WIDGET_LOGE("Component 'BoidsSwarmGravityComponent' not found on node");
        return;
    }

    ApplyBoidsSwarmGravityParams(comp, params);
}

::SceneBoidsSwarm::BoidsSimGravityParametersOrNull BoidsSimWorldImpl::getBoidsSimGravityComponent(
    ::SceneNodes::weak::Node node)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::getBoidsSimGravityComponent: invalid node");
        return ::SceneBoidsSwarm::BoidsSimGravityParametersOrNull::make_nValue();
    }

    auto comp = FindComponentOnNode(iNode, "BoidsSwarmGravityComponent");
    if (!comp) {
        WIDGET_LOGE(
            "BoidsSimWorldImpl::getBoidsSimGravityComponent: component 'BoidsSwarmGravityComponent' not found on node");
        return ::SceneBoidsSwarm::BoidsSimGravityParametersOrNull::make_nValue();
    }

    ::SceneBoidsSwarm::BoidsSimGravityParameters result;

    constexpr BASE_NS::string_view compName = "BoidsSwarmGravityComponent";

    result.radius = ::taihe::optional<f64>(std::in_place, ReadFloatProperty(comp, compName, "radius"));
    result.accelerationMag = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "accelerationMag"));

    return ::SceneBoidsSwarm::BoidsSimGravityParametersOrNull::make_rc(result);
}

void BoidsSimWorldImpl::removeBoidsSimGravityComponent(::SceneNodes::weak::Node node)
{
    RemoveComponent(GetINodeFromTaiheNode(node), "BoidsSwarmGravityComponent",
        BOIDSSWARM_NS::IBoidsSwarmGravityComponentManager::UID);
}

void BoidsSimWorldImpl::addBoidsSimRepulsionComponent(
    ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimRepulsionParameters params)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::addBoidsSimRepulsionComponent: invalid node");
        return;
    }

    auto existing = FindComponentOnNode(iNode, "BoidsSwarmRepulsionComponent");
    if (existing) {
        WIDGET_LOGE("Component 'BoidsSwarmRepulsionComponent' already exists on node");
        return;
    }

    auto comp = scene_->CreateComponent(iNode, "BoidsSwarmRepulsionComponent").GetResult();
    if (!comp) {
        WIDGET_LOGE("Failed to create component 'BoidsSwarmRepulsionComponent'");
        return;
    }

    comp->PopulateAllProperties();
    ApplyBoidsSwarmRepulsionParams(comp, params);
}

void BoidsSimWorldImpl::setBoidsSimRepulsionComponent(
    ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimRepulsionParameters params)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::setBoidsSimRepulsionComponent: invalid node");
        return;
    }

    auto comp = FindComponentOnNode(iNode, "BoidsSwarmRepulsionComponent");
    if (!comp) {
        WIDGET_LOGE("Component 'BoidsSwarmRepulsionComponent' not found on node");
        return;
    }

    ApplyBoidsSwarmRepulsionParams(comp, params);
}

::SceneBoidsSwarm::BoidsSimRepulsionParametersOrNull
BoidsSimWorldImpl::getBoidsSimRepulsionComponent(::SceneNodes::weak::Node node)
{
    auto iNode = GetINodeFromTaiheNode(node);
    if (!iNode) {
        WIDGET_LOGE("BoidsSimWorldImpl::getBoidsSimRepulsionComponent: invalid node");
        return ::SceneBoidsSwarm::BoidsSimRepulsionParametersOrNull::make_nValue();
    }

    auto comp = FindComponentOnNode(iNode, "BoidsSwarmRepulsionComponent");
    if (!comp) {
        WIDGET_LOGE("BoidsSimWorldImpl::getBoidsSimRepulsionComponent: "
            "component 'BoidsSwarmRepulsionComponent' not found on node");
        return ::SceneBoidsSwarm::BoidsSimRepulsionParametersOrNull::make_nValue();
    }

    ::SceneBoidsSwarm::BoidsSimRepulsionParameters result;

    constexpr BASE_NS::string_view compName = "BoidsSwarmRepulsionComponent";

    result.radius = ::taihe::optional<f64>(std::in_place, ReadFloatProperty(comp, compName, "radius"));
    result.accelerationMag = ::taihe::optional<f64>(
        std::in_place, ReadFloatProperty(comp, compName, "accelerationMag"));

    return ::SceneBoidsSwarm::BoidsSimRepulsionParametersOrNull::make_rc(result);
}

void BoidsSimWorldImpl::removeBoidsSimRepulsionComponent(::SceneNodes::weak::Node node)
{
    RemoveComponent(GetINodeFromTaiheNode(node), "BoidsSwarmRepulsionComponent",
        BOIDSSWARM_NS::IBoidsSwarmRepulsionComponentManager::UID);
}

void BoidsSimWorldImpl::RemoveComponent(
    const SCENE_NS::INode::Ptr& node, BASE_NS::string_view compName, const BASE_NS::Uid& uid)
{
    if (!node) {
        return;
    }
    auto comp = FindComponentOnNode(node, compName);
    if (!comp) {
        WIDGET_LOGE("Component '%.*s' not found on node", static_cast<int>(compName.size()), compName.data());
        return;
    }

    auto ecsAccess = interface_cast<SCENE_NS::IEcsObjectAccess>(comp);
    if (!ecsAccess) {
        WIDGET_LOGE("Component '%.*s' does not have IEcsObjectAccess",
            static_cast<int>(compName.size()), compName.data());
        return;
    }

    auto ecsObj = ecsAccess->GetEcsObject();
    if (!ecsObj) {
        WIDGET_LOGE("ECS object not available for component '%.*s'",
            static_cast<int>(compName.size()), compName.data());
        return;
    }

    ExecSyncTask([ecsObj, uid, compName]() -> META_NS::IAny::Ptr {
        auto entity = ecsObj->GetEntity();
        if (!CORE_NS::EntityUtil::IsValid(entity)) {
            WIDGET_LOGE("Invalid entity for component '%.*s'", static_cast<int>(compName.size()), compName.data());
            return META_NS::IAny::Ptr {};
        }

        auto internalScene = ecsObj->GetScene();
        if (!internalScene) {
            WIDGET_LOGE("Internal scene is not available");
            return META_NS::IAny::Ptr {};
        }

        auto* ecs = internalScene->GetEcsContext().GetNativeEcs().get();
        if (!ecs) {
            WIDGET_LOGE("Native ECS is not available");
            return META_NS::IAny::Ptr {};
        }

        auto* mgr = ecs->GetComponentManager(uid);
        if (!mgr || !mgr->HasComponent(entity)) {
            WIDGET_LOGE("ECS manager or component not found for '%.*s'",
                static_cast<int>(compName.size()), compName.data());
            return META_NS::IAny::Ptr {};
        }

        mgr->Destroy(entity);
        return META_NS::IAny::Ptr {};
    });

    auto attach = interface_cast<META_NS::IAttach>(node);
    if (attach) {
        attach->Detach(interface_pointer_cast<META_NS::IObject>(comp));
    }
}

::SceneBoidsSwarm::BoidsSimWorldOrNull getDefaultBoidsSimWorld(::SceneTH::weak::Scene scene)
{
    if (scene.is_error()) {
        WIDGET_LOGE("getDefaultBoidsSimWorld: null scene");
        return ::SceneBoidsSwarm::BoidsSimWorldOrNull::make_nValue();
    }
    auto nativeScene = GetISceneFromTaiheScene(scene);
    if (!nativeScene) {
        WIDGET_LOGE("getDefaultBoidsSimWorld: failed to get native scene");
        return ::SceneBoidsSwarm::BoidsSimWorldOrNull::make_nValue();
    }
    bool systemAvailable = false;
    ExecSyncTask([scene = nativeScene, &systemAvailable]() -> META_NS::IAny::Ptr {
        auto* system = GetBoidsSystem(scene);
        systemAvailable = system != nullptr;
        return {};
    });

    if (!systemAvailable) {
        WIDGET_LOGE("BoidsSwarmSystem not available (plugin not loaded)");
        return ::SceneBoidsSwarm::BoidsSimWorldOrNull::make_nValue();
    }

    auto rc = taihe::make_holder<BoidsSimWorldImpl, ::SceneBoidsSwarm::BoidsSimWorld>(
        BASE_NS::move(nativeScene));
    return ::SceneBoidsSwarm::BoidsSimWorldOrNull::make_rc(rc);
}

} // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// NOLINTBEGIN
TH_EXPORT_CPP_API_getDefaultBoidsSimWorld(getDefaultBoidsSimWorld);
// NOLINTEND
