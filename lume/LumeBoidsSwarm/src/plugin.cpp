/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <boids_swarm/ecs/components/boids_swarm_component.h>
#include <boids_swarm/ecs/components/boids_swarm_gravity_component.h>
#include <boids_swarm/ecs/components/boids_swarm_repulsion_component.h>
#include <boids_swarm/ecs/components/boids_swarm_state_component.h>
#include <boids_swarm/ecs/systems/intf_boids_swarm_system.h>
#include <boids_swarm/implementation_uids.h>
#include <boids_swarm/namespace.h>

#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/implementation_uids.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>

#include "ecs/systems/boids_swarm_system.h"

CORE_BEGIN_NAMESPACE()
IPluginRegister* gPluginRegistry{nullptr};
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()

BOIDSSWARM_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

IComponentManager* IBoidsSwarmComponentManagerInstance(IEcs&);
IComponentManager* IBoidsSwarmGravityComponentManagerInstance(IEcs&);
IComponentManager* IBoidsSwarmRepulsionComponentManagerInstance(IEcs&);
IComponentManager* IBoidsSwarmStateComponentManagerInstance(IEcs&);

ISystem* IBoidsSwarmSystemInstance(IEcs&);

void IBoidsSwarmComponentManagerDestroy(IComponentManager*);
void IBoidsSwarmGravityComponentManagerDestroy(IComponentManager*);
void IBoidsSwarmRepulsionComponentManagerDestroy(IComponentManager*);
void IBoidsSwarmStateComponentManagerDestroy(IComponentManager*);

void IBoidsSwarmSystemDestroy(ISystem*);

const char* GetVersionInfo();

constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = {CORE3D_NS::UID_3D_PLUGIN};

constexpr ComponentManagerTypeInfo BoidsSwarmComponentTypeInfo = {
    {ComponentManagerTypeInfo::UID},
    IBoidsSwarmComponentManager::UID,
    CORE_NS::GetName<IBoidsSwarmComponentManager>().data(),
    IBoidsSwarmComponentManagerInstance,
    IBoidsSwarmComponentManagerDestroy,
};

constexpr ComponentManagerTypeInfo BoidsSwarmGravityComponentTypeInfo = {
    {ComponentManagerTypeInfo::UID},
    IBoidsSwarmGravityComponentManager::UID,
    CORE_NS::GetName<IBoidsSwarmGravityComponentManager>().data(),
    IBoidsSwarmGravityComponentManagerInstance,
    IBoidsSwarmGravityComponentManagerDestroy,
};

constexpr ComponentManagerTypeInfo BoidsSwarmRepulsionComponentTypeInfo = {
    {ComponentManagerTypeInfo::UID},
    IBoidsSwarmRepulsionComponentManager::UID,
    CORE_NS::GetName<IBoidsSwarmRepulsionComponentManager>().data(),
    IBoidsSwarmRepulsionComponentManagerInstance,
    IBoidsSwarmRepulsionComponentManagerDestroy,
};

constexpr ComponentManagerTypeInfo BoidsSwarmStateComponentTypeInfo = {
    {ComponentManagerTypeInfo::UID},
    IBoidsSwarmStateComponentManager::UID,
    CORE_NS::GetName<IBoidsSwarmStateComponentManager>().data(),
    IBoidsSwarmStateComponentManagerInstance,
    IBoidsSwarmStateComponentManagerDestroy,
};

constexpr Uid BoidsSwarmSystemDepsRw[] = {
    BoidsSwarmStateComponentTypeInfo.uid,
    CORE3D_NS::ITransformComponentManager::UID,
};

constexpr Uid BoidsSwarmSystemDepsR[] = {
    BoidsSwarmComponentTypeInfo.uid,
    BoidsSwarmGravityComponentTypeInfo.uid,
    BoidsSwarmRepulsionComponentTypeInfo.uid,
};

constexpr ComponentManagerTypeInfo gComponentManagers[] = {
    BoidsSwarmComponentTypeInfo,
    BoidsSwarmGravityComponentTypeInfo,
    BoidsSwarmRepulsionComponentTypeInfo,
    BoidsSwarmStateComponentTypeInfo,
};

constexpr SystemTypeInfo gSystems[] = {
    {
        {SystemTypeInfo::UID},
        IBoidsSwarmSystem::UID,
        CORE_NS::GetName<IBoidsSwarmSystem>().data(),
        IBoidsSwarmSystemInstance,
        IBoidsSwarmSystemDestroy,
        BoidsSwarmSystemDepsRw,
        BoidsSwarmSystemDepsR,
        {},
        CORE3D_NS::INodeSystem::UID,
    },
};

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    gPluginRegistry = &pluginRegistry;

    for (const auto& info : gComponentManagers) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : gSystems) {
        pluginRegistry.RegisterTypeInfo(info);
    }

    return &pluginRegistry;
}

void UnregisterInterfaces(PluginToken token)
{
    if (IPluginRegister* pluginRegistry = static_cast<IPluginRegister*>(token); pluginRegistry) {
        for (const auto& info : gSystems) {
            pluginRegistry->UnregisterTypeInfo(info);
        }
        for (const auto& info : gComponentManagers) {
            pluginRegistry->UnregisterTypeInfo(info);
        }
    }
}

BOIDSSWARM_END_NAMESPACE()

extern "C" {
PLUGIN_DATA(PluginBoidsSwarm){
    {CORE_NS::IPlugin::UID},
    "AGP Boids Swarm",
    {BOIDSSWARM_NS::UID_BOIDS_SWARM_PLUGIN, BOIDSSWARM_NS::GetVersionInfo},
    BOIDSSWARM_NS::RegisterInterfaces,
    BOIDSSWARM_NS::UnregisterInterfaces,
    {BOIDSSWARM_NS::PLUGIN_DEPENDENCIES},
};
DEFINE_STATIC_PLUGIN(PluginBoidsSwarm);
}
