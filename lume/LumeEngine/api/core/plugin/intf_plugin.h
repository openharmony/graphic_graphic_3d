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

#ifndef API_CORE_PLUGIN_IPLUGIN_H
#define API_CORE_PLUGIN_IPLUGIN_H

#include <base/containers/array_view.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEngine;
class IComponentManager;
class ISystem;
class IEcs;
class IInterface;
class IClassFactory;
class IClassRegister;
class IPluginRegister;

/** \addtogroup group_plugin_iplugin
 *  @{
 */
/** Plugin version */
struct Version {
    /** UID for identifying different versions of the plugin. */
    const BASE_NS::Uid uid;
    /** Function returning a free form version string. */
    const char* (*GetVersionString)() { nullptr };
};

/** Type information. Base for different registrable information structures. */
struct ITypeInfo {
    BASE_NS::Uid typeUid;
};

/** Information needed from the plugin for managing ComponentManagers. */
struct ComponentManagerTypeInfo : public ITypeInfo {
    /** TypeInfo UID for component manager. */
    static constexpr BASE_NS::Uid UID { "f812e951-c860-4208-99e0-66b45841bb58" };

    using CreateComponentManagerFn = IComponentManager* (*)(IEcs&);
    using DestroyComponentManagerFn = void (*)(IComponentManager* instance);

    /** Unique ID of the component manager. */
    const BASE_NS::Uid uid;
    /** Name used during component manager creation to identify the type of the component manager. */
    char const* const typeName { "" };
    /** Pointer to function which is used to create component manager instances. */
    const CreateComponentManagerFn createManager;
    /** Pointer to function which is used to destroy component manager instances. */
    const DestroyComponentManagerFn destroyManager;
};

/** Information needed from the plugin for managing Systems. */
struct SystemTypeInfo : public ITypeInfo {
    /** TypeInfo UID for system. */
    static constexpr BASE_NS::Uid UID { "31321549-70db-495c-81e9-6fd1cf30af5e" };

    using CreateSystemFn = ISystem* (*)(IEcs&);
    using DestroySystemFn = void (*)(ISystem* instance);

    /** Unique ID of the system. */
    const BASE_NS::Uid uid;
    /** Name used during system creation to identify the type of the system. */
    char const* const typeName { "" };
    /** Pointer to function which is used to create system instances. */
    const CreateSystemFn createSystem;
    /** Pointer to function which is used to destroy system instances. */
    const DestroySystemFn destroySystem;
    /** List of component managers the system might modify during ISystem::Update. Combined with
     * readOnlyComponentDependencies they form a list of component managers to be created before creating this system.
     * These can be also used to determine which systems can be executed in parallel. Default constructed UID can be
     * used as wild card to indicate dependencies not known at compile time. */
    const BASE_NS::array_view<const BASE_NS::Uid> componentDependencies;
    /** List of component managers the system would only read during ISystem::Update. Combined with
     * componentDependencies they form a list of component managers to be created before creating this system.
     * These can be also used to determine which systems can be executed in parallel. Default constructed UID can be
     * used as wild card to indicate dependencies not known at compile time. */
    const BASE_NS::array_view<const BASE_NS::Uid> readOnlyComponentDependencies;
};

// Plugin token is a plugin defined token to identify instance.
// Think of this as an instance to the factory that provides the get/create interface method.
using PluginToken = void*;

/** Information needed from the plugin for managing Interfaces. */
struct InterfaceTypeInfo {
    using CreateInstanceFn = IInterface* (*)(IClassFactory&, PluginToken);
    using GetInstanceFn = IInterface* (*)(IClassRegister&, PluginToken);

    /* Token created by the plugin. (plugin specific instance data for the interface factory) */
    const PluginToken token;
    /** Unique ID of the interface implementation. */
    const BASE_NS::Uid uid;
    /** Name used during system creation to identify the type of the interface. */
    char const* const typeName { "" };
    /** Pointer to function which is used to create interface instances bound to IClassFactory instance.
     * It is acceptable to return same pointer, but with a added reference count.
     */
    const CreateInstanceFn createInterface;
    /** Pointer to function which is used to get singleton interface instances bound to IClassRegister instance.
     * It is acceptable to return same pointer, but with a added reference count.
     */
    const GetInstanceFn getInterface;
};

/** Exported information from a plugin (.so/.dll). */
struct IPlugin : public ITypeInfo {
    /** TypeInfo UID for a plugin. */
    static constexpr BASE_NS::Uid UID { "5fc9b017-5b13-4612-81c2-5a6d6fc3d897" };

    /*
    Plugin lifecycle.
    1. registerInterfaces, once during IPluginRegister::LoadPlugins
       (First call to plugin is here, initialize "global" resources/data/state here.)
    2. IEnginePlugin::createPlugin, IEcsPlugin::createPlugin etc., multiple times.
       (during e.g. engine and ESC initialization)
    3. IEnginePlugin::destroyPlugin, IEcsPlugin::destroyPlugin etc., multiple times.
       (during e.g. engine and ESC destruction)
    4. unregisterInterfaces, once during IPluginRegister::UnloadPlugins
       (Last call to plugin is here, uninitialize "global" resources/data/state here.)
    */

    using RegisterInterfacesFn = PluginToken (*)(IPluginRegister&);
    using UnregisterInterfacesFn = void (*)(PluginToken);

    /** Name of this plugin. */
    const char* const name { "" };
    /** Version information of the plugin. */
    const Version version;

    /** Called when attaching plugin to plugin registry.
     * Is expected to register its own named interfaces (IInterface) which are globally available and also register to
     * different plugin categories e.g. IEnginePlugin and IEcsPlugin when there's a dependency to a specific instance.
     * Think of this as "CreateInterfaceFactory"
     */
    const RegisterInterfacesFn registerInterfaces { nullptr };

    /** Called when detaching plugin from plugin registry.
     * Is expected to unregister its own named interfaces (IInterface) from the global registry and also unregister
     * different plugin categories e.g. IEnginePlugin and IEcsPlugin when there's a dependency to a specific instance.
     */
    const UnregisterInterfacesFn unregisterInterfaces { nullptr };

    /** List of plugins this plugin requires. */
    const BASE_NS::array_view<const BASE_NS::Uid> pluginDependencies;
};

/** A plugin which adds new interfaces to the engine. */
struct IEnginePlugin : public ITypeInfo {
    /** TypeInfo UID for engine plugin. */
    static constexpr BASE_NS::Uid UID { "a81c121b-160c-467e-8bd6-63902da85c6b" };

    /*
    Plugin lifecycle.
    1. createPlugin  (*as many times as engines instantiated)       (initialize IEngine specific state here.)
    2. destroyPlugin  (*as many times as engines instantiated)      (deinitialize IEngine specific state here.)
    */

    using CreatePluginFn = PluginToken (*)(IEngine&);
    using DestroyPluginFn = void (*)(PluginToken);

    constexpr IEnginePlugin(CreatePluginFn create, DestroyPluginFn destroy)
        : ITypeInfo { UID }, createPlugin { create }, destroyPlugin { destroy }
    {}

    /** Initialize function for engine plugin.
     * Called when plugin is initially loaded by engine. Used to register paths etc.
     * Is expected to register its own named interfaces (IInterface) which are tied to the engine instance.
     * Called when attaching to engine.
     */
    const CreatePluginFn createPlugin { nullptr };

    /** Deinitialize function for engine plugin.
     * Called when plugin is about to be unloaded by engine.
     * Called when detaching from engine.
     */
    const DestroyPluginFn destroyPlugin { nullptr };
};

/** A plugin which adds new component managers and systems to the ECS. */
struct IEcsPlugin : public ITypeInfo {
    /** TypeInfo UID for ECS plugin. */
    static constexpr BASE_NS::Uid UID { "b4843032-e144-4757-a28c-03c119c3a10c" };

    using CreatePluginFn = PluginToken (*)(IEcs&);
    using DestroyPluginFn = void (*)(PluginToken);

    constexpr IEcsPlugin(CreatePluginFn create, DestroyPluginFn destroy)
        : ITypeInfo { UID }, createPlugin { create }, destroyPlugin { destroy }
    {}

    /** Initialize function for ECS plugin.
     * Called when attaching to ECS.
     */
    const CreatePluginFn createPlugin { nullptr };

    /** Deinitialize function for ECS plugin.
     * Called when detaching from ECS.
     */
    const DestroyPluginFn destroyPlugin { nullptr };
};

/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_PLUGIN_IPLUGIN_H
