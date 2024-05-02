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

#ifndef API_CORE_PLUGIN_IPLUGIN_REGISTER_H
#define API_CORE_PLUGIN_IPLUGIN_REGISTER_H

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
struct Uid;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IClassRegister;
class IFileManager;
struct IPlugin;
struct ITypeInfo;

/** \addtogroup group_plugin_iplugin
 *  @{
 */
/** Interface plugins use to register new data types. */
class IPluginRegister {
public:
    /** Get list of all plugins
     * @return An array of currently available plugins.
     */
    virtual BASE_NS::array_view<const IPlugin* const> GetPlugins() const = 0;

    /** Load plugins matching the given UIDs. If an empty list is given, all plugins will be loaded.
     * @param pluginUids List of UIDs to load (IPlugin::version::uid).
     * @return true if all the plugins could be loaded, false otherwise.
     */
    virtual bool LoadPlugins(const BASE_NS::array_view<const BASE_NS::Uid> pluginUids) = 0;

    /** Unload plugins matching the given UIDs. If an empty list is given, all plugins will be unloaded.
     * @param pluginUids List of UIDs to unload (IPlugin::version::uid).
     */
    virtual void UnloadPlugins(const BASE_NS::array_view<const BASE_NS::Uid> pluginUids) = 0;

    /** Access to the registered interface classes.
     * @return Class register owned by this plugin register.
     */
    virtual IClassRegister& GetClassRegister() const = 0;

    /** Register type information. Plugins and applications can use this to add new types of ISystems,
     * IComponentManagers etc.
     * @param type Type information to register.
     */
    virtual void RegisterTypeInfo(const ITypeInfo& type) = 0;

    /** Unregister type information.
     * @param type Type information to unregister.
     */
    virtual void UnregisterTypeInfo(const ITypeInfo& type) = 0;

    /** Get all the registered type of a certain category e.g. SystemTypeInfo::UID.
     * @param typeUid Type category UID.
     */
    virtual BASE_NS::array_view<const ITypeInfo* const> GetTypeInfos(const BASE_NS::Uid& typeUid) const = 0;

    /** Interface for observing loading and unloading of type infos. */
    class ITypeInfoListener {
    public:
        enum class EventType {
            /** Type has been added. */
            ADDED,
            /** Type is about to be removed. */
            REMOVED,
        };
        /** Called when ITypeInfos are added or removed.
         * @param type Event type.
         * @param typeInfos Tyep infos to which the event applies.
         */
        virtual void OnTypeInfoEvent(EventType type, BASE_NS::array_view<const ITypeInfo* const> typeInfos) = 0;

    protected:
        ITypeInfoListener() = default;
        virtual ~ITypeInfoListener() = default;
    };

    /** Add a type info event listener.
     * @param listener Listener instance.
     */
    virtual void AddListener(ITypeInfoListener& listener) = 0;

    /** Remove a type info event listener.
     * @param listener Listener instance.
     */
    virtual void RemoveListener(const ITypeInfoListener& listener) = 0;

    /** Register plugin load path.
     * @param path path to be added.
     */
    virtual void RegisterPluginPath(const BASE_NS::string_view path) = 0;

    /** Get file manager instance used by plugin registry.
     * @return file manager instance
     */
    virtual IFileManager& GetFileManager() = 0;

protected:
    IPluginRegister() = default;
    virtual ~IPluginRegister() = default;
};

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
/** Get plugin register */
extern IPluginRegister& (*GetPluginRegister)();

/** Setup the plugin registry */
extern void (*CreatePluginRegistry)(const struct PlatformCreateInfo& platformCreateInfo);
#else
/** Get plugin register */
CORE_PUBLIC IPluginRegister& GetPluginRegister();

/** Setup the plugin registry */
CORE_PUBLIC void CreatePluginRegistry(const struct PlatformCreateInfo& platformCreateInfo);
#endif
/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_PLUGIN_IPLUGIN_REGISTER_H
