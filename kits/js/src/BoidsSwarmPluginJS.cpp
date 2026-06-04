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

#include "BoidsSwarmPluginJS.h"

#include <boids_swarm/ecs/components/boids_swarm_component.h>
#include <boids_swarm/ecs/components/boids_swarm_gravity_component.h>
#include <boids_swarm/ecs/components/boids_swarm_repulsion_component.h>
#include <boids_swarm/ecs/components/boids_swarm_state_component.h>
#include <boids_swarm/ecs/systems/intf_boids_swarm_system.h>
#include <meta/api/make_callback.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/property/intf_property.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_scene.h>

#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>

#include "BaseObjectJS.h"

using namespace NapiApi;

namespace {

SCENE_NS::IComponent::Ptr FindComponentOnNode(const SCENE_NS::INode::Ptr& node, BASE_NS::string_view componentName)
{
    auto attach = interface_cast<META_NS::IAttach>(node);
    if (!attach) {
        return {};
    }
    auto cont = attach->GetAttachmentContainer(false);
    if (!cont) {
        return {};
    }
    return cont->FindByName<SCENE_NS::IComponent>(componentName);
}

napi_value Vec3ToNapi(BASE_NS::Math::Vec3 v, napi_env env)
{
    NapiApi::Object obj(env);
    obj.Set("x", NapiApi::Value<float>(env, v.x));
    obj.Set("y", NapiApi::Value<float>(env, v.y));
    obj.Set("z", NapiApi::Value<float>(env, v.z));
    return obj.ToNapiValue();
}

napi_value QuatToNapi(BASE_NS::Math::Quat q, napi_env env)
{
    NapiApi::Object obj(env);
    obj.Set("x", NapiApi::Value<float>(env, q.x));
    obj.Set("y", NapiApi::Value<float>(env, q.y));
    obj.Set("z", NapiApi::Value<float>(env, q.z));
    obj.Set("w", NapiApi::Value<float>(env, q.w));
    return obj.ToNapiValue();
}

napi_value PropertyToNapiValue(const META_NS::IProperty::Ptr& prop, napi_env env)
{
    if (!prop) {
        return nullptr;
    }
    if (META_NS::IsCompatibleWith<float>(prop)) {
        auto val = META_NS::GetValue(META_NS::Property<float>(prop));
        return NapiApi::Value<float>(env, val).ToNapiValue();
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(prop)) {
        auto val = META_NS::GetValue(META_NS::Property<BASE_NS::Math::Vec3>(prop));
        return Vec3ToNapi(val, env);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Quat>(prop)) {
        auto val = META_NS::GetValue(META_NS::Property<BASE_NS::Math::Quat>(prop));
        return QuatToNapi(val, env);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(prop)) {
        auto val = META_NS::GetValue(META_NS::Property<BASE_NS::Math::Vec2>(prop));
        NapiApi::Object obj(env);
        obj.Set("x", NapiApi::Value<float>(env, val.x));
        obj.Set("y", NapiApi::Value<float>(env, val.y));
        return obj.ToNapiValue();
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(prop)) {
        auto val = META_NS::GetValue(META_NS::Property<BASE_NS::Math::Vec4>(prop));
        NapiApi::Object obj(env);
        obj.Set("x", NapiApi::Value<float>(env, val.x));
        obj.Set("y", NapiApi::Value<float>(env, val.y));
        obj.Set("z", NapiApi::Value<float>(env, val.z));
        obj.Set("w", NapiApi::Value<float>(env, val.w));
        return obj.ToNapiValue();
    }
    if (META_NS::IsCompatibleWith<bool>(prop)) {
        auto val = META_NS::GetValue(META_NS::Property<bool>(prop));
        return NapiApi::Value<bool>(env, val).ToNapiValue();
    }
    if (META_NS::IsCompatibleWith<BASE_NS::string>(prop)) {
        auto val = META_NS::GetValue(META_NS::Property<BASE_NS::string>(prop));
        return NapiApi::Value<BASE_NS::string>(env, val).ToNapiValue();
    }
    return nullptr;
}

bool SetJsValueToProperty(const META_NS::IProperty::Ptr& prop, napi_env env, napi_value jsValue)
{
    if (!prop || !jsValue) {
        return false;
    }
    if (META_NS::IsCompatibleWith<float>(prop)) {
        NapiApi::Value<float> v(env, jsValue);
        if (v.IsValid()) {
            return META_NS::SetValue(META_NS::Property<float>(prop), v.valueOrDefault());
        }
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(prop)) {
        NapiApi::Object obj(env, jsValue);
        if (obj) {
            auto x = obj.Get<float>("x");
            auto y = obj.Get<float>("y");
            auto z = obj.Get<float>("z");
            if (x.IsValid() && y.IsValid() && z.IsValid()) {
                BASE_NS::Math::Vec3 v{x.valueOrDefault(), y.valueOrDefault(), z.valueOrDefault()};
                return META_NS::SetValue(META_NS::Property<BASE_NS::Math::Vec3>(prop), v);
            }
        }
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Quat>(prop)) {
        NapiApi::Object obj(env, jsValue);
        if (obj) {
            auto x = obj.Get<float>("x");
            auto y = obj.Get<float>("y");
            auto z = obj.Get<float>("z");
            auto w = obj.Get<float>("w");
            if (x.IsValid() && y.IsValid() && z.IsValid() && w.IsValid()) {
                BASE_NS::Math::Quat q{x.valueOrDefault(), y.valueOrDefault(), z.valueOrDefault(), w.valueOrDefault()};
                return META_NS::SetValue(META_NS::Property<BASE_NS::Math::Quat>(prop), q);
            }
        }
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(prop)) {
        NapiApi::Object obj(env, jsValue);
        if (obj) {
            auto x = obj.Get<float>("x");
            auto y = obj.Get<float>("y");
            if (x.IsValid() && y.IsValid()) {
                BASE_NS::Math::Vec2 v{x.valueOrDefault(), y.valueOrDefault()};
                return META_NS::SetValue(META_NS::Property<BASE_NS::Math::Vec2>(prop), v);
            }
        }
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(prop)) {
        NapiApi::Object obj(env, jsValue);
        if (obj) {
            auto x = obj.Get<float>("x");
            auto y = obj.Get<float>("y");
            auto z = obj.Get<float>("z");
            auto w = obj.Get<float>("w");
            if (x.IsValid() && y.IsValid() && z.IsValid() && w.IsValid()) {
                BASE_NS::Math::Vec4 v{x.valueOrDefault(), y.valueOrDefault(), z.valueOrDefault(), w.valueOrDefault()};
                return META_NS::SetValue(META_NS::Property<BASE_NS::Math::Vec4>(prop), v);
            }
        }
    }
    if (META_NS::IsCompatibleWith<bool>(prop)) {
        NapiApi::Value<bool> v(env, jsValue);
        if (v.IsValid()) {
            return META_NS::SetValue(META_NS::Property<bool>(prop), v.valueOrDefault());
        }
    }
    if (META_NS::IsCompatibleWith<BASE_NS::string>(prop)) {
        NapiApi::Value<BASE_NS::string> v(env, jsValue);
        if (v.IsValid()) {
            return META_NS::SetValue(META_NS::Property<BASE_NS::string>(prop), v.valueOrDefault());
        }
    }
    return false;
}

void SetPropertiesFromJsParams(
    const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName, NapiApi::Object paramObj, napi_env env)
{
    if (!comp || !paramObj) {
        return;
    }
    comp->PopulateAllProperties();
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return;
    }

    napi_value jsKeys;
    auto keyStatus = napi_get_property_names(env, paramObj.ToNapiValue(), &jsKeys);
    if (keyStatus != napi_ok) {
        LOG_E("napi_get_property_names failed (status %d) for component '%.*s'",
            keyStatus,
            static_cast<int>(compName.size()),
            compName.data());
        return;
    }
    NapiApi::Array keysArray(env, jsKeys);
    size_t keyCount = keysArray.Count();

    auto namePrefix = compName + ".";

    for (size_t i = 0; i < keyCount; i++) {
        auto key = NapiApi::Value<BASE_NS::string>(env, keysArray.Get_value(i)).valueOrDefault();
        if (key.empty()) {
            continue;
        }

        napi_value jsVal;
        auto propStatus = napi_get_named_property(env, paramObj.ToNapiValue(), key.c_str(), &jsVal);
        if (propStatus != napi_ok) {
            LOG_E("napi_get_named_property failed (status %d) for key '%s' on component '%.*s'",
                propStatus,
                key.c_str(),
                static_cast<int>(compName.size()),
                compName.data());
            continue;
        }

        if (auto p = meta->GetProperty(namePrefix + key)) {
            SetJsValueToProperty(p, env, jsVal);
        }
    }
}

napi_value BuildJsObjectFromComponent(
    const SCENE_NS::IComponent::Ptr& comp, BASE_NS::string_view compName, napi_env env)
{
    if (!comp) {
        return nullptr;
    }
    comp->PopulateAllProperties();
    auto meta = interface_cast<META_NS::IMetadata>(comp);
    if (!meta) {
        return nullptr;
    }

    auto namePrefix = compName + ".";
    auto prefixLen = namePrefix.size();

    NapiApi::Object resultObj(env);
    for (auto&& p : meta->GetProperties()) {
        if (!p) {
            continue;
        }
        auto fullName = p->GetName();
        if (!fullName.starts_with(namePrefix)) {
            continue;
        }
        auto name = fullName.substr(prefixLen);
        if (name.empty()) {
            continue;
        }
        auto jsVal = PropertyToNapiValue(p, env);
        if (jsVal) {
            resultObj.Set(name, jsVal);
        }
    }
    return resultObj.ToNapiValue();
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

}  // namespace

void BoidsSwarmWorldJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> props;

    props.push_back(TROGetProperty<bool, BoidsSwarmWorldJS, &BoidsSwarmWorldJS::IsPlaying>("isPlaying"));
    props.push_back(
        TROGetSetProperty<float, BoidsSwarmWorldJS, &BoidsSwarmWorldJS::GetPlaySpeed, &BoidsSwarmWorldJS::SetPlaySpeed>(
            "playSpeed"));
    props.push_back(TROGetSetProperty<float,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::GetTimeStepSec,
        &BoidsSwarmWorldJS::SetTimeStepSec>("timeStepSec"));
    props.push_back(TROGetSetProperty<NapiApi::Object,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::GetAxisMask,
        &BoidsSwarmWorldJS::SetAxisMask>("axisMask"));
    props.push_back(TROGetSetProperty<float,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::GetVelocitySmoothingFactor,
        &BoidsSwarmWorldJS::SetVelocitySmoothingFactor>("velocitySmoothingFactor"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, BoidsSwarmWorldJS, &BoidsSwarmWorldJS::Play>("play"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, BoidsSwarmWorldJS, &BoidsSwarmWorldJS::Pause>("pause"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, BoidsSwarmWorldJS, &BoidsSwarmWorldJS::Stop>("stop"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::AddBoidsSimComponent>("addBoidsSimComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::SetBoidsSimComponent>("setBoidsSimComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::AddBoidsSimGravityComponent>("addBoidsSimGravityComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::SetBoidsSimGravityComponent>("setBoidsSimGravityComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::AddBoidsSimRepulsionComponent>("addBoidsSimRepulsionComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::SetBoidsSimRepulsionComponent>("setBoidsSimRepulsionComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::GetBoidsSimComponent>("getBoidsSimComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::GetBoidsSimGravityComponent>("getBoidsSimGravityComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::GetBoidsSimRepulsionComponent>("getBoidsSimRepulsionComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::GetBoidsSimStateComponent>("getBoidsSimStateComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::RemoveBoidsSimComponent>("removeBoidsSimComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::RemoveBoidsSimGravityComponent>("removeBoidsSimGravityComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::RemoveBoidsSimRepulsionComponent>("removeBoidsSimRepulsionComponent"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object, BASE_NS::string>,
        BoidsSwarmWorldJS,
        &BoidsSwarmWorldJS::AddTarget>("addTarget"));

    napi_value func;
    auto status = napi_define_class(env,
        "BoidsSimWorld",
        NAPI_AUTO_LENGTH,
        BaseObject::ctor<BoidsSwarmWorldJS>(),
        nullptr,
        props.size(),
        props.data(),
        &func);
    if (status != napi_ok) {
        LOG_E("napi_define_class failed (status %d) for BoidsSimWorld", status);
        return;
    }

    NapiApi::Object{env, exports}.Set("BoidsSimWorld", func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("BoidsSimWorld", func);
    }
}

BoidsSwarmWorldJS::BoidsSwarmWorldJS(napi_env env, napi_callback_info info) : BaseObject(env, info)
{
    NapiApi::FunctionContext<NapiApi::Object> fromJs(env, info);
    AddBridge("BoidsSwarmWorldJS", fromJs.This());
    if (fromJs) {
        scene_ = fromJs.Arg<0>();
    }
}

BoidsSwarmWorldJS::~BoidsSwarmWorldJS()
{
    DestroyBridge(this);
}

void* BoidsSwarmWorldJS::GetInstanceImpl(uint32_t id)
{
    if (id == BoidsSwarmWorldJS::ID) {
        return static_cast<BoidsSwarmWorldJS*>(this);
    }
    return BaseObject::GetInstanceImpl(id);
}

napi_value BoidsSwarmWorldJS::AddBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    return AddComponent(ctx, "BoidsSwarmComponent");
}

napi_value BoidsSwarmWorldJS::SetBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    return SetComponent(ctx, "BoidsSwarmComponent");
}

napi_value BoidsSwarmWorldJS::AddBoidsSimGravityComponent(
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    return AddComponent(ctx, "BoidsSwarmGravityComponent");
}

napi_value BoidsSwarmWorldJS::SetBoidsSimGravityComponent(
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    return SetComponent(ctx, "BoidsSwarmGravityComponent");
}

napi_value BoidsSwarmWorldJS::AddBoidsSimRepulsionComponent(
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    return AddComponent(ctx, "BoidsSwarmRepulsionComponent");
}

napi_value BoidsSwarmWorldJS::SetBoidsSimRepulsionComponent(
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    return SetComponent(ctx, "BoidsSwarmRepulsionComponent");
}

napi_value BoidsSwarmWorldJS::GetBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return GetComponent(ctx, "BoidsSwarmComponent");
}

napi_value BoidsSwarmWorldJS::GetBoidsSimGravityComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return GetComponent(ctx, "BoidsSwarmGravityComponent");
}

napi_value BoidsSwarmWorldJS::GetBoidsSimRepulsionComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return GetComponent(ctx, "BoidsSwarmRepulsionComponent");
}

// Unlike GetBoidsSimComponent/Gravity/Repulsion which delegate to the generic GetComponent()
// that uses FindComponentOnNode() + BuildJsObjectFromComponent() via the Scene META property
// system, BoidsSwarmStateComponent is a read-only system-managed ECS component (stores computed
// simulation state) that has no Scene IComponent wrapper, so we access the raw ECS layer directly.
napi_value BoidsSwarmWorldJS::GetBoidsSimStateComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (ctx.ArgCount() < 1) {
        LOG_E("Invalid arguments for getBoidsSimStateComponent: expected (node)");
        return ctx.GetNull();
    }

    NapiApi::Object nodeObj = ctx.Arg<0>();
    if (!nodeObj) {
        LOG_E("Invalid node object");
        return ctx.GetNull();
    }

    auto node = nodeObj.GetNative<SCENE_NS::INode>();
    if (!node) {
        LOG_E("Node is not a valid INode");
        return ctx.GetNull();
    }

    auto ecsAccess = interface_pointer_cast<SCENE_NS::IEcsObjectAccess>(node);
    if (!ecsAccess) {
        LOG_E("Node does not have IEcsObjectAccess");
        return ctx.GetNull();
    }

    BASE_NS::Math::Vec3 velocities[BOIDSSWARM_NS::BoidsSwarmStateComponent::VELOCITY_COUNT]{};
    float velocityMag = 0.0f;
    bool success = false;

    ExecSyncTask([ecsAccess, &velocities, &velocityMag, &success]() -> META_NS::IAny::Ptr {
        CORE_NS::IEcs* ecs = ecsAccess->GetEcsObject()->GetScene()->GetEcsContext().GetNativeEcs().get();
        if (!ecs) {
            LOG_E("Failed to get ECS");
            return META_NS::IAny::Ptr{};
        }

        CORE_NS::Entity entity = ecsAccess->GetEcsObject()->GetEntity();
        if (entity == CORE_NS::Entity{}) {
            LOG_E("Invalid entity");
            return META_NS::IAny::Ptr{};
        }

        auto* stateMgr = static_cast<BOIDSSWARM_NS::IBoidsSwarmStateComponentManager*>(
            ecs->GetComponentManager(BOIDSSWARM_NS::IBoidsSwarmStateComponentManager::UID));
        if (stateMgr == nullptr || !stateMgr->HasComponent(entity)) {
            LOG_E("BoidsSwarmStateComponent not found on entity");
            return META_NS::IAny::Ptr{};
        }

        auto handle = stateMgr->Read(entity);
        if (!handle) {
            LOG_E("Failed to read BoidsSwarmStateComponent");
            return META_NS::IAny::Ptr{};
        }

        for (size_t i = 0; i < BOIDSSWARM_NS::BoidsSwarmStateComponent::VELOCITY_COUNT; ++i) {
            velocities[i] = handle->velocities[i];
        }
        velocityMag = handle->velocityMag;
        success = true;
        return META_NS::IAny::Ptr{};
    });

    if (!success) {
        return ctx.GetNull();
    }

    auto env = ctx.GetEnv();
    NapiApi::Object resultObj(env);
    NapiApi::Array velocitiesArray(env, BOIDSSWARM_NS::BoidsSwarmStateComponent::VELOCITY_COUNT);
    for (size_t i = 0; i < BOIDSSWARM_NS::BoidsSwarmStateComponent::VELOCITY_COUNT; ++i) {
        velocitiesArray.Set_value(i, Vec3ToNapi(velocities[i], env));
    }
    resultObj.Set("velocities", velocitiesArray);
    resultObj.Set("velocity", velocitiesArray.Get_value(0));
    resultObj.Set("velocityMag", NapiApi::Value<float>(env, velocityMag));

    return resultObj.ToNapiValue();
}

napi_value BoidsSwarmWorldJS::RemoveBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return RemoveComponent(ctx, "BoidsSwarmComponent", BOIDSSWARM_NS::IBoidsSwarmComponentManager::UID);
}

napi_value BoidsSwarmWorldJS::RemoveBoidsSimGravityComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return RemoveComponent(ctx, "BoidsSwarmGravityComponent", BOIDSSWARM_NS::IBoidsSwarmGravityComponentManager::UID);
}

napi_value BoidsSwarmWorldJS::RemoveBoidsSimRepulsionComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return RemoveComponent(
        ctx, "BoidsSwarmRepulsionComponent", BOIDSSWARM_NS::IBoidsSwarmRepulsionComponentManager::UID);
}

napi_value BoidsSwarmWorldJS::AddTarget(
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object, BASE_NS::string>& ctx)
{
    if (ctx.ArgCount() < 3) {
        LOG_E("Invalid arguments for addTarget: expected (node, target, behaviorType)");
        return ctx.GetNull();
    }

    NapiApi::Object nodeObj = ctx.Arg<0>();
    NapiApi::Object targetObj = ctx.Arg<1>();
    auto behaviorTypeOpt = ctx.Arg<2>();

    if (!nodeObj || !targetObj || !behaviorTypeOpt.IsValid()) {
        LOG_E("Invalid arguments");
        return ctx.GetNull();
    }

    BASE_NS::string behaviorType = behaviorTypeOpt.valueOrDefault("");

    auto node = nodeObj.GetNative<SCENE_NS::INode>();
    auto target = targetObj.GetNative<SCENE_NS::INode>();
    if (!node || !target) {
        LOG_E("Invalid node or target");
        return ctx.GetNull();
    }

    auto nodeEcsAccess = interface_pointer_cast<SCENE_NS::IEcsObjectAccess>(node);
    auto targetEcsAccess = interface_pointer_cast<SCENE_NS::IEcsObjectAccess>(target);
    if (!nodeEcsAccess || !targetEcsAccess) {
        LOG_E("Node or target does not have IEcsObjectAccess");
        return ctx.GetNull();
    }

    BASE_NS::vector<CORE_NS::Entity> BOIDSSWARM_NS::BoidsSwarmComponent::*targetField = nullptr;

    if (behaviorType == "separation") {
        targetField = &BOIDSSWARM_NS::BoidsSwarmComponent::separationTargets;
    } else if (behaviorType == "alignment") {
        targetField = &BOIDSSWARM_NS::BoidsSwarmComponent::alignmentTargets;
    } else if (behaviorType == "cohesion") {
        targetField = &BOIDSSWARM_NS::BoidsSwarmComponent::cohesionTargets;
    } else {
        LOG_E("Invalid behaviorType: %s (expected 'separation', 'alignment', or 'cohesion')", behaviorType.c_str());
        return ctx.GetNull();
    }

    ExecSyncTask([nodeEcsAccess, targetEcsAccess, targetField]() -> META_NS::IAny::Ptr {
        CORE_NS::IEcs* ecs = nodeEcsAccess->GetEcsObject()->GetScene()->GetEcsContext().GetNativeEcs().get();
        if (!ecs) {
            LOG_E("Failed to get ECS");
            return META_NS::IAny::Ptr{};
        }

        CORE_NS::Entity entity = nodeEcsAccess->GetEcsObject()->GetEntity();
        CORE_NS::Entity targetEntity = targetEcsAccess->GetEcsObject()->GetEntity();
        if (entity == CORE_NS::Entity{} || targetEntity == CORE_NS::Entity{}) {
            LOG_E("Invalid entity");
            return META_NS::IAny::Ptr{};
        }

        auto* mgr = static_cast<BOIDSSWARM_NS::IBoidsSwarmComponentManager*>(
            ecs->GetComponentManager(BOIDSSWARM_NS::IBoidsSwarmComponentManager::UID));
        if (!mgr || !mgr->HasComponent(entity)) {
            LOG_E("BoidsSwarmComponent not found");
            return META_NS::IAny::Ptr{};
        }

        auto handle = mgr->Write(entity);
        if (!handle) {
            LOG_E("Failed to get write handle");
            return META_NS::IAny::Ptr{};
        }

        ((*handle).*targetField).push_back(targetEntity);

        return META_NS::IAny::Ptr{};
    });

    return ctx.This().ToNapiValue();
}

napi_value BoidsSwarmWorldJS::Play(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return {};
    }
    ExecSyncTask([scene]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->Play();
        return {};
    });
    return {};
}

napi_value BoidsSwarmWorldJS::Pause(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return {};
    }
    ExecSyncTask([scene]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->Pause();
        return {};
    });
    return {};
}

napi_value BoidsSwarmWorldJS::Stop(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return {};
    }
    ExecSyncTask([scene]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->Stop();
        return {};
    });
    return {};
}

napi_value BoidsSwarmWorldJS::IsPlaying(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return ctx.GetBoolean(false);
    }
    bool isPlaying = false;
    ExecSyncTask([scene, &isPlaying]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        isPlaying = system->IsPlaying();
        return {};
    });
    return ctx.GetBoolean(isPlaying);
}

napi_value BoidsSwarmWorldJS::GetPlaySpeed(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return ctx.GetNumber(BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_PLAY_SPEED);
    }
    float playSpeed = BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_PLAY_SPEED;
    ExecSyncTask([scene, &playSpeed]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        playSpeed = system->GetPlaySpeed();
        return {};
    });
    return ctx.GetNumber(playSpeed);
}

void BoidsSwarmWorldJS::SetPlaySpeed(NapiApi::FunctionContext<float>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return;
    }
    float speed = ctx.Arg<0>();
    ExecSyncTask([scene, speed]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->SetPlaySpeed(speed);
        return {};
    });
}

napi_value BoidsSwarmWorldJS::GetTimeStepSec(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return ctx.GetNumber(BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_TIME_STEP_SEC);
    }
    float timeStepSec = BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_TIME_STEP_SEC;
    ExecSyncTask([scene, &timeStepSec]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        timeStepSec = system->GetTimeStepSec();
        return {};
    });
    return ctx.GetNumber(timeStepSec);
}

void BoidsSwarmWorldJS::SetTimeStepSec(NapiApi::FunctionContext<float>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return;
    }
    float timeStepSec = ctx.Arg<0>();
    ExecSyncTask([scene, timeStepSec]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->SetTimeStepSec(timeStepSec);
        return {};
    });
}

napi_value BoidsSwarmWorldJS::GetAxisMask(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        BASE_NS::Math::IVec3 axisMask = BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_AXIS_MASK;
        NapiApi::Object result(ctx.GetEnv());
        result.Set("x", NapiApi::Value<int32_t>(ctx.GetEnv(), axisMask.x));
        result.Set("y", NapiApi::Value<int32_t>(ctx.GetEnv(), axisMask.y));
        result.Set("z", NapiApi::Value<int32_t>(ctx.GetEnv(), axisMask.z));
        return result.ToNapiValue();
    }
    BASE_NS::Math::IVec3 axisMask = BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_AXIS_MASK;
    ExecSyncTask([scene, &axisMask]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        axisMask = system->GetAxisMask();
        return {};
    });
    NapiApi::Object result(ctx.GetEnv());
    result.Set("x", NapiApi::Value<int32_t>(ctx.GetEnv(), axisMask.x));
    result.Set("y", NapiApi::Value<int32_t>(ctx.GetEnv(), axisMask.y));
    result.Set("z", NapiApi::Value<int32_t>(ctx.GetEnv(), axisMask.z));
    return result.ToNapiValue();
}

void BoidsSwarmWorldJS::SetAxisMask(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return;
    }
    NapiApi::Object axisMaskObj = ctx.Arg<0>();
    if (!axisMaskObj) {
        return;
    }
    BASE_NS::Math::IVec3 axisMask = BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_AXIS_MASK;
    auto x = axisMaskObj.Get<int32_t>("x");
    auto y = axisMaskObj.Get<int32_t>("y");
    auto z = axisMaskObj.Get<int32_t>("z");
    if (x.IsValid())
        axisMask.x = x.valueOrDefault();
    if (y.IsValid())
        axisMask.y = y.valueOrDefault();
    if (z.IsValid())
        axisMask.z = z.valueOrDefault();

    ExecSyncTask([scene, axisMask]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->SetAxisMask(axisMask);
        return {};
    });
}

napi_value BoidsSwarmWorldJS::GetVelocitySmoothingFactor(NapiApi::FunctionContext<>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return ctx.GetNumber(BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_VELOCITY_SMOOTHING_FACTOR);
    }
    float factor = BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_VELOCITY_SMOOTHING_FACTOR;
    ExecSyncTask([scene, &factor]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        factor = system->GetVelocitySmoothingFactor();
        return {};
    });
    return ctx.GetNumber(factor);
}

void BoidsSwarmWorldJS::SetVelocitySmoothingFactor(NapiApi::FunctionContext<float>& ctx)
{
    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return;
    }
    float factor = ctx.Arg<0>();
    ExecSyncTask([scene, factor]() -> META_NS::IAny::Ptr {
        auto system = GetBoidsSystem(scene);
        if (!system) {
            LOG_E("BoidsSwarmSystem not available");
            return {};
        }
        system->SetVelocitySmoothingFactor(factor);
        return {};
    });
}

void BoidsSwarmWorldJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        DisposeBridge(this);
        scene_.Reset();
    }
}

void BoidsSwarmPluginJS::Init(napi_env env, napi_value exports)
{
    auto getDefaultBoidsSimWorldFunc = [](napi_env e, napi_callback_info cb) -> napi_value {
        NapiApi::FunctionContext<NapiApi::Object> fc(e, cb);
        return BoidsSwarmPluginJS::GetDefaultBoidsSimWorld(fc);
    };

    napi_property_descriptor props[] = {
        napi_property_descriptor{"getDefaultBoidsSimWorld",
            nullptr,
            getDefaultBoidsSimWorldFunc,
            nullptr,
            nullptr,
            nullptr,
            napi_static,
            nullptr},
    };

    napi_value func;
    auto status = napi_define_class(env,
        "BoidsSimPlugin",
        NAPI_AUTO_LENGTH,
        BaseObject::ctor<BoidsSwarmPluginJS>(),
        nullptr,
        sizeof(props) / sizeof(props[0]),
        props,
        &func);
    if (status != napi_ok) {
        LOG_E("napi_define_class failed (status %d) for BoidsSimPlugin", status);
    }

    NapiApi::Object{env, exports}.Set("BoidsSimPlugin", func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("BoidsSimPlugin", func);
    }
}

BoidsSwarmPluginJS::BoidsSwarmPluginJS(napi_env env, napi_callback_info info) : BaseObject(env, info)
{
    NapiApi::FunctionContext<> fromJs(env, info);
    AddBridge("BoidsSwarmPluginJS", fromJs.This());
}

BoidsSwarmPluginJS::~BoidsSwarmPluginJS()
{
    DestroyBridge(this);
}

void* BoidsSwarmPluginJS::GetInstanceImpl(uint32_t id)
{
    if (id == BoidsSwarmPluginJS::ID) {
        return static_cast<BoidsSwarmPluginJS*>(this);
    }
    return BaseObject::GetInstanceImpl(id);
}

napi_value BoidsSwarmPluginJS::GetDefaultBoidsSimWorld(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (ctx.ArgCount() < 1) {
        LOG_E("Invalid arguments for getDefaultBoidsSimWorld: expected (scene)");
        return ctx.GetNull();
    }

    NapiApi::Object sceneObj = ctx.Arg<0>();
    if (!sceneObj) {
        LOG_E("Invalid scene object");
        return ctx.GetNull();
    }

    auto scene = sceneObj.GetNative<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Object is not a valid IScene");
        return ctx.GetNull();
    }
    bool systemAvailable = false;
    ExecSyncTask([scene, &systemAvailable]() -> META_NS::IAny::Ptr {
        auto* system = GetBoidsSystem(scene);
        systemAvailable = system != nullptr;
        return {};
    });

    if (!systemAvailable) {
        LOG_E("BoidsSwarmSystem not available (plugin not loaded)");
        return ctx.GetNull();
    }

    napi_value dummyArg[] = {sceneObj.ToNapiValue()};
    NapiApi::JsFuncArgs args{1, dummyArg};
    NapiApi::Object result(ctx.GetEnv(), "BoidsSimWorld", args);
    return result.ToNapiValue();
}

void BoidsSwarmPluginJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        DisposeBridge(this);
    }
}

napi_value BoidsSwarmWorldJS::AddComponent(
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx, BASE_NS::string_view compName)
{
    if (ctx.ArgCount() < 2) {
        LOG_E("Invalid arguments for addComponent: expected (node, param)");
        return ctx.GetNull();
    }

    NapiApi::Object nodeObj = ctx.Arg<0>();
    NapiApi::Object paramObj = ctx.Arg<1>();

    if (!nodeObj || !paramObj) {
        LOG_E("Invalid arguments");
        return ctx.GetNull();
    }

    auto node = nodeObj.GetNative<SCENE_NS::INode>();
    if (!node) {
        LOG_E("Node is not a valid INode");
        return ctx.GetNull();
    }

    auto existing = FindComponentOnNode(node, compName);
    if (existing) {
        LOG_E("Component '%.*s' already exists on node", static_cast<int>(compName.size()), compName.data());
        return ctx.GetNull();
    }

    auto scene = scene_.GetObject<SCENE_NS::IScene>();
    if (!scene) {
        LOG_E("Scene is not available");
        return ctx.GetNull();
    }

    auto env = paramObj.GetEnv();
    auto comp = scene->CreateComponent(node, compName).GetResult();
    if (!comp) {
        LOG_E("Failed to create component '%.*s'", static_cast<int>(compName.size()), compName.data());
        return ctx.GetNull();
    }
    SetPropertiesFromJsParams(comp, compName, paramObj, env);

    return ctx.This().ToNapiValue();
}

napi_value BoidsSwarmWorldJS::SetComponent(
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx, BASE_NS::string_view compName)
{
    if (ctx.ArgCount() < 2) {
        LOG_E("Invalid arguments for setComponent: expected (node, param)");
        return ctx.GetNull();
    }

    NapiApi::Object nodeObj = ctx.Arg<0>();
    NapiApi::Object paramObj = ctx.Arg<1>();

    if (!nodeObj || !paramObj) {
        LOG_E("Invalid arguments");
        return ctx.GetNull();
    }

    auto node = nodeObj.GetNative<SCENE_NS::INode>();
    if (!node) {
        LOG_E("Node is not a valid INode");
        return ctx.GetNull();
    }

    auto comp = FindComponentOnNode(node, compName);
    if (!comp) {
        LOG_E("Component '%.*s' not found on node", static_cast<int>(compName.size()), compName.data());
        return ctx.GetNull();
    }

    auto env = paramObj.GetEnv();
    SetPropertiesFromJsParams(comp, compName, paramObj, env);

    return ctx.This().ToNapiValue();
}

napi_value BoidsSwarmWorldJS::GetComponent(
    NapiApi::FunctionContext<NapiApi::Object>& ctx, BASE_NS::string_view compName)
{
    if (ctx.ArgCount() < 1) {
        LOG_E("Invalid arguments for getComponent: expected (node)");
        return ctx.GetNull();
    }

    NapiApi::Object nodeObj = ctx.Arg<0>();
    if (!nodeObj) {
        LOG_E("Invalid node object");
        return ctx.GetNull();
    }

    auto node = nodeObj.GetNative<SCENE_NS::INode>();
    if (!node) {
        LOG_E("Node is not a valid INode");
        return ctx.GetNull();
    }

    auto comp = FindComponentOnNode(node, compName);
    if (!comp) {
        return ctx.GetNull();
    }

    napi_value resultValue = nullptr;
    auto env = ctx.GetEnv();
    resultValue = BuildJsObjectFromComponent(comp, compName, env);

    return resultValue ? resultValue : ctx.GetNull();
}

napi_value BoidsSwarmWorldJS::RemoveComponent(
    NapiApi::FunctionContext<NapiApi::Object>& ctx, BASE_NS::string_view compName, const BASE_NS::Uid& uid)
{
    if (ctx.ArgCount() < 1) {
        LOG_E("Invalid arguments for removeComponent: expected (node)");
        return ctx.GetNull();
    }

    NapiApi::Object nodeObj = ctx.Arg<0>();
    if (!nodeObj) {
        LOG_E("Invalid node object");
        return ctx.GetNull();
    }

    auto node = nodeObj.GetNative<SCENE_NS::INode>();
    if (!node) {
        LOG_E("Node is not a valid INode");
        return ctx.GetNull();
    }

    auto comp = FindComponentOnNode(node, compName);
    if (!comp) {
        LOG_E("Component '%.*s' not found on node", static_cast<int>(compName.size()), compName.data());
        return ctx.GetNull();
    }

    ExecSyncTask([comp, node, compName, uid]() -> META_NS::IAny::Ptr {
        auto ecsAccess = interface_cast<SCENE_NS::IEcsObjectAccess>(comp);
        if (!ecsAccess) {
            LOG_E(
                "Component '%.*s' does not have IEcsObjectAccess", static_cast<int>(compName.size()), compName.data());
            return {};
        }

        auto ecsObj = ecsAccess->GetEcsObject();
        if (!ecsObj) {
            LOG_E("ECS object not available for component '%.*s'", static_cast<int>(compName.size()), compName.data());
            return {};
        }

        auto entity = ecsObj->GetEntity();
        if (!CORE_NS::EntityUtil::IsValid(entity)) {
            LOG_E("Invalid entity for component '%.*s'", static_cast<int>(compName.size()), compName.data());
            return {};
        }

        auto internalScene = ecsObj->GetScene();
        if (!internalScene) {
            LOG_E("Internal scene is not available");
            return {};
        }

        auto* ecs = internalScene->GetEcsContext().GetNativeEcs().get();
        if (!ecs) {
            LOG_E("Native ECS is not available");
            return {};
        }

        auto* mgr = ecs->GetComponentManager(uid);
        if (!mgr || !mgr->HasComponent(entity)) {
            LOG_E("ECS manager or component not found for '%.*s'", static_cast<int>(compName.size()), compName.data());
            return {};
        }
        mgr->Destroy(entity);

        auto attach = interface_cast<META_NS::IAttach>(node);
        if (!attach) {
            LOG_E("Node does not support IAttach");
            return {};
        }
        attach->Detach(interface_pointer_cast<META_NS::IObject>(comp));

        return {};
    });

    return ctx.This().ToNapiValue();
}
