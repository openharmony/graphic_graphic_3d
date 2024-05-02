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

#include "plugin_registry.h"

#include <algorithm>

#include <base/util/uid_util.h>
#include <core/ecs/intf_component_manager.h>
#include <core/log.h>

#include "image/loaders/image_loader_ktx.h"
#include "image/loaders/image_loader_stb_image.h"
#include "image/loaders/image_loader_libpng.h"
#include "image/loaders/image_loader_libjpeg.h"
#include "io/file_manager.h"
#include "io/std_filesystem.h"
#include "os/intf_library.h"
#include "static_plugin_decl.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::move;
using BASE_NS::pair;
using BASE_NS::reverse_iterator;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::to_string;
using BASE_NS::Uid;
using BASE_NS::vector;

IInterface* CreateFileMonitor(CORE_NS::IClassFactory& registry, CORE_NS::PluginToken token);
IInterface* GetFileApiFactory(CORE_NS::IClassRegister& registry, CORE_NS::PluginToken token);

namespace StaticPluginRegistry {
// static_plugin_registry magic begin
#if defined(CORE_USE_COMPILER_GENERATED_STATIC_LIST) && (CORE_USE_COMPILER_GENERATED_STATIC_LIST == 1)

#if _MSC_VER
#pragma section("spd$b", long, read)
#pragma section("spd$d", long, read)
#pragma section("spd$e", long, read)
__declspec(allocate("spd$b")) static constexpr const IPlugin* g_staticPluginList = nullptr;
__declspec(allocate("spd$e")) static constexpr const IPlugin* g_staticPluginListEnd = nullptr;
#else
// clang-format off
__asm__(
    ".pushsection " SECTION(spl.1)
    " .local g_staticPluginList\n"
    " g_staticPluginList:\n"
    ".dc.a 0x0\n"
    ".section " SECTION(spl.2)
    " .local g_staticPluginListData\n"
    "g_staticPluginListData:\n"
    " .dc.a 0x0\n"
    " .section " SECTION(spl.3)
    " .local g_staticPluginListEnd\n"
    " g_staticPluginListEnd:\n"
    ".dc.a 0x0\n"
    " .popsection\n");
// clang-format on
extern "C" {
extern CORE_NS::IPlugin const* const g_staticPluginList;
extern CORE_NS::IPlugin const* const g_staticPluginListData;
extern CORE_NS::IPlugin const* const g_staticPluginListEnd;
__attribute__((used)) CORE_NS::IPlugin const* const g_staticPluginListDataRef = g_staticPluginListData;
}
#endif

#else

#if _MSC_VER
static CORE_NS::IPlugin const* const* g_staticPluginList = nullptr;
static size_t g_staticPluginListCount = 0;
#else
__attribute__((visibility("hidden"))) static CORE_NS::IPlugin const* const* g_staticPluginList = nullptr;
__attribute__((visibility("hidden"))) static size_t g_staticPluginListCount = 0;
#endif

void RegisterStaticPlugin(const CORE_NS::IPlugin& plugin)
{
    static BASE_NS::vector<const CORE_NS::IPlugin*> gGlobalPlugins;
    gGlobalPlugins.push_back(&plugin);
    g_staticPluginList = gGlobalPlugins.data();
    g_staticPluginListCount = gGlobalPlugins.size();
}
#endif
} // namespace StaticPluginRegistry

namespace {
struct LibPlugin {
    ILibrary::Ptr lib;
    const IPlugin* plugin;
};

template<typename Container, typename Predicate>
inline typename Container::const_iterator FindIf(const Container& container, Predicate&& predicate)
{
    return std::find_if(container.cbegin(), container.cend(), BASE_NS::forward<Predicate>(predicate));
}

template<typename Container, typename Predicate>
inline bool NoneOf(const Container& container, Predicate&& predicate)
{
    return std::none_of(container.cbegin(), container.cend(), BASE_NS::forward<Predicate>(predicate));
}

template<typename Container, typename Predicate>
inline bool AllOf(const Container& container, Predicate&& predicate)
{
    return std::all_of(container.cbegin(), container.cend(), BASE_NS::forward<Predicate>(predicate));
}

void GatherStaticPlugins(vector<LibPlugin>& plugins)
{
    CORE_LOG_V("Static plugins:");
#if defined(CORE_USE_COMPILER_GENERATED_STATIC_LIST) && (CORE_USE_COMPILER_GENERATED_STATIC_LIST == 1)
    const array_view<const IPlugin* const> staticPluginRegistry(
        &StaticPluginRegistry::g_staticPluginList, &StaticPluginRegistry::g_staticPluginListEnd);
#else
    const array_view<const IPlugin* const> staticPluginRegistry(
        StaticPluginRegistry::g_staticPluginList, StaticPluginRegistry::g_staticPluginListCount);
#endif

    for (const auto plugin : staticPluginRegistry) {
        if (plugin && (plugin->typeUid == IPlugin::UID)) {
            CORE_LOG_V("\t%s", plugin->name);
            plugins.push_back({ nullptr, plugin });
        }
    }
}

void GatherDynamicPlugins(vector<LibPlugin>& plugins, IFileManager& fileManager)
{
    CORE_LOG_V("Dynamic plugins:");
    const auto libraryFileExtension = ILibrary::GetFileExtension();
    constexpr string_view pluginRoot { "plugins://" };
    if (IDirectory::Ptr pluginFiles = fileManager.OpenDirectory(pluginRoot); pluginFiles) {
        for (const auto& file : pluginFiles->GetEntries()) {
            const string_view pluginFile = file.name;
            if (pluginFile.ends_with(libraryFileExtension)) {
                const string absoluteFile = fileManager.GetEntry(pluginRoot + file.name).name;
                if (ILibrary::Ptr lib = ILibrary::Load(absoluteFile); lib) {
                    const IPlugin* plugin = lib->GetPlugin();
                    if (plugin && (plugin->typeUid == IPlugin::UID)) {
                        CORE_LOG_V("\t%s", plugin->name);
                        plugins.push_back({ move(lib), plugin });
                    }
                }
            }
        }
    }
}

void Notify(const array_view<IPluginRegister::ITypeInfoListener*> listeners,
    IPluginRegister::ITypeInfoListener::EventType type, array_view<const ITypeInfo* const> typeInfos)
{
    for (IPluginRegister::ITypeInfoListener* listener : listeners) {
        if (listener) {
            listener->OnTypeInfoEvent(type, typeInfos);
        }
    }
}
static constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo KTX_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "306357a4-d49c-4670-9746-5ccbba567dc9" },
    CreateImageLoaderKtx,
    KTX_IMAGE_TYPES,
};
static constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo STB_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "a5049cb8-10bb-4047-b7f5-e9939d5bb3a5" },
    CreateImageLoaderStbImage,
    STB_IMAGE_TYPES,
};
static constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo PNG_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "dacbcb8d-60d6-4337-8295-7af99b517c1d" },
    CreateImageLoaderLibPNGImage,
    PNG_IMAGE_TYPES,
};
static constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo JPEG_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "c5fb2284-561f-4078-8a00-74b82f161964" },
    CreateImageLoaderLibJPEGImage,
    JPEG_IMAGE_TYPES,
};
} // namespace

vector<InterfaceTypeInfo> PluginRegistry::RegisterGlobalInterfaces(PluginRegistry& registry)
{
    vector<InterfaceTypeInfo> interfaces = {
        InterfaceTypeInfo { &registry, UID_LOGGER, GetName<ILogger>().data(), nullptr,
            [](IClassRegister& /* registry */, PluginToken token) -> IInterface* {
                return &static_cast<PluginRegistry*>(token)->logger_;
            } },
        InterfaceTypeInfo { &registry, UID_FRUSTUM_UTIL, GetName<IFrustumUtil>().data(), nullptr,
            [](IClassRegister& /* registry */, PluginToken token) -> IInterface* {
                return &static_cast<PluginRegistry*>(token)->frustumUtil_;
            } },
        InterfaceTypeInfo { &registry, UID_ENGINE_FACTORY, GetName<IEngineFactory>().data(), nullptr,
            [](IClassRegister& /* registry */, PluginToken token) -> IInterface* {
                return static_cast<PluginRegistry*>(token)->engineFactory_.GetInterface(IEngineFactory::UID);
            } },
        InterfaceTypeInfo { &registry, UID_SYSTEM_GRAPH_LOADER, GetName<ISystemGraphLoaderFactory>().data(), nullptr,
            [](IClassRegister& /* registry */, PluginToken token) -> IInterface* {
                return &static_cast<PluginRegistry*>(token)->systemGraphLoadeFactory;
            } },
        InterfaceTypeInfo { &registry, UID_GLOBAL_FACTORY, "Global registry factory", nullptr,
            [](IClassRegister& registry, PluginToken /* token */) -> IInterface* {
                return registry.GetInterface<IClassFactory>();
            } },
        InterfaceTypeInfo {
            &registry, UID_FILESYSTEM_API_FACTORY, "Filesystem API factory", nullptr, GetFileApiFactory },
        InterfaceTypeInfo { &registry, UID_FILE_MONITOR, "Filemonitor", CreateFileMonitor, nullptr },
        InterfaceTypeInfo { &registry, UID_FILE_MANAGER, "FileManager",
            [](IClassFactory& /* factory */, PluginToken /* token */) -> IInterface* {
                return new CORE_NS::FileManager();
            },
            nullptr },
        InterfaceTypeInfo { &registry, UID_TASK_QUEUE_FACTORY, "Task queue factory", nullptr,
            [](IClassRegister& /* registry */, PluginToken token) -> IInterface* {
                return &static_cast<PluginRegistry*>(token)->taskQueueFactory_;
            } },
#if (CORE_PERF_ENABLED == 1)
        InterfaceTypeInfo { &registry, UID_PERFORMANCE_FACTORY, GetName<IPerformanceDataManagerFactory>().data(),
            nullptr,
            [](IClassRegister& /* registry */, PluginToken token) -> IInterface* {
                return &static_cast<PluginRegistry*>(token)->perfManFactory_;
            } }
#endif
    };

    for (const auto& info : interfaces) {
        registry.RegisterInterfaceType(info);
    }
    registry.RegisterTypeInfo(KTX_LOADER);
    registry.RegisterTypeInfo(STB_LOADER);
    registry.RegisterTypeInfo(PNG_LOADER);
    registry.RegisterTypeInfo(JPEG_LOADER);
    return interfaces;
}

void PluginRegistry::UnregisterGlobalInterfaces()
{
    UnregisterTypeInfo(STB_LOADER);
    UnregisterTypeInfo(KTX_LOADER);
    UnregisterTypeInfo(PNG_LOADER);
    UnregisterTypeInfo(JPEG_LOADER);
    for (const auto& info : ownInterfaceInfos_) {
        UnregisterInterfaceType(info);
    }
}

PluginRegistry::PluginRegistry()
{
    ownInterfaceInfos_ = RegisterGlobalInterfaces(*this);
}

PluginRegistry::~PluginRegistry()
{
    UnloadPlugins({});
    UnregisterGlobalInterfaces();
}

// IPluginRegister
array_view<const IPlugin* const> PluginRegistry::GetPlugins() const
{
    return plugins_;
}

bool PluginRegistry::LoadPlugins(const array_view<const Uid> pluginUids)
{
    // Gather all the available static and dynamic libraries.
    vector<LibPlugin> plugins;
    GatherStaticPlugins(plugins);
    GatherDynamicPlugins(plugins, fileManager_);

    // If a list of UIDs was given remove all except the requires plugins.
    if (!pluginUids.empty()) {
        // Gather dependencies of each plugin.
        vector<Uid> toLoad;
        toLoad.reserve(plugins.size());

        auto addDependencies = [](auto&& addDependencies, vector<Uid>& toBeLoaded,
                                   const vector<LibPlugin>& availablePlugins,
                                   BASE_NS::vector<const IPlugin*>& loadedPlugins, const Uid& uidToLoad) -> bool {
            bool found = true;
            // Only consider plugins which are not already loaded, and not yet in the loading list.
            if (NoneOf(
                    loadedPlugins, [&uidToLoad](const IPlugin* loaded) { return loaded->version.uid == uidToLoad; }) &&
                NoneOf(toBeLoaded, [&uidToLoad](const Uid& willLoad) { return willLoad == uidToLoad; })) {
                if (auto pos = FindIf(availablePlugins,
                        [&uidToLoad](
                            const LibPlugin& libPlugin) { return libPlugin.plugin->version.uid == uidToLoad; });
                    pos != availablePlugins.end()) {
                    found = AllOf(pos->plugin->pluginDependencies, [&](const Uid& dependency) {
                        return addDependencies(
                            addDependencies, toBeLoaded, availablePlugins, loadedPlugins, dependency);
                    });
                    if (found) {
                        toBeLoaded.push_back(uidToLoad);
                    } else {
                        CORE_LOG_E("Missing dependencies for: %s", to_string(uidToLoad).data());
                    }
                } else {
                    CORE_LOG_E("Plugin not found: %s", to_string(uidToLoad).data());
                    found = false;
                }
            }
            return found;
        };
        const bool found = AllOf(pluginUids,
            [&](const Uid& uid) { return addDependencies(addDependencies, toLoad, plugins, plugins_, uid); });

        // Order the available plugins to match the to-be-loaded list and remove extras.
        auto begin = plugins.begin();
        auto end = plugins.end();
        for (const Uid& uid : toLoad) {
            if (auto pos = std::find_if(
                    begin, end, [uid](const LibPlugin& libPlugin) { return libPlugin.plugin->version.uid == uid; });
                pos != end) {
                std::rotate(begin, pos, end);
                ++begin;
            }
        }
        plugins.erase(begin, end);

        if (!found) {
            CORE_LOG_E("Unable to load plugins:");
            for (const auto& uid : pluginUids) {
                if (NoneOf(plugins, [uid](const auto& available) { return available.plugin->version.uid == uid; })) {
                    CORE_LOG_E("\t%s", to_string(uid).data());
                }
            }
            return false;
        }
    }

    // Now we should have only the desired plugins and can load all of them.
    CORE_LOG_D("Load plugins:");
    loading_ = true;
    for (auto& plugin : plugins) {
        RegisterPlugin(move(plugin.lib), *plugin.plugin,
            NoneOf(pluginUids,
                [&loading = (plugin.plugin->version.uid)](const Uid& userRequest) { return userRequest == loading; }));
    }
    loading_ = false;

    if (!newTypeInfos_.empty()) {
        Notify(typeInfoListeners_, ITypeInfoListener::EventType::ADDED, newTypeInfos_);
        newTypeInfos_.clear();
    }

    return true;
}

void PluginRegistry::UnloadPlugins(const array_view<const Uid> pluginUids)
{
    CORE_LOG_D("Unload plugins:");
    if (pluginUids.empty()) {
        while (!pluginDatas_.empty() && !plugins_.empty()) {
            UnregisterPlugin(*plugins_.back(), pluginDatas_.back().token);
            plugins_.pop_back();
            pluginDatas_.pop_back();
        }
        plugins_.clear();
        pluginDatas_.clear();
    } else {
        [](array_view<const IPlugin*> plugins, array_view<PluginData> pluginDatas,
            const array_view<const Uid>& pluginUids) {
            auto recurse = [](const array_view<const IPlugin*>& plugins, array_view<PluginData>& pluginDatas,
                               const array_view<const Uid>& pluginUids, auto& recurseRef) -> void {
                for (const auto& uid : pluginUids) {
                    if (auto pos = std::find_if(plugins.begin(), plugins.end(),
                            [uid](const IPlugin* pl) { return pl && pl->version.uid == uid; });
                        pos != plugins.end()) {
                        const auto index = static_cast<size_t>(std::distance(plugins.begin(), pos));
                        if (--pluginDatas[index].refcnt <= 0) {
                            recurseRef(plugins, pluginDatas, (*pos)->pluginDependencies, recurseRef);
                        }
                    }
                }
            };
            recurse(plugins, pluginDatas, pluginUids, recurse);
        }(plugins_, pluginDatas_, pluginUids);

        auto pdIt = pluginDatas_.crbegin();
        for (auto pos = plugins_.crbegin(), last = plugins_.crend(); pos != last;) {
            if (pdIt->refcnt <= 0) {
                UnregisterPlugin(*(*pos), pdIt->token);
                plugins_.erase(pos.base() - 1);
                pluginDatas_.erase(pdIt.base() - 1);
            }
            ++pos;
            ++pdIt;
        }
    }
}

IClassRegister& PluginRegistry::GetClassRegister() const
{
    return *const_cast<PluginRegistry*>(this);
}

void PluginRegistry::RegisterTypeInfo(const ITypeInfo& type)
{
    if (const auto pos = typeInfos_.find(type.typeUid); pos != typeInfos_.cend()) {
        pos->second.push_back(&type);
    } else {
        typeInfos_.insert({ type.typeUid, {} }).first->second.push_back(&type);
    }

    // During plugin loading gather all the infos and send events once.
    if (loading_) {
        newTypeInfos_.push_back(&type);
    } else {
        const ITypeInfo* const infos[] = { &type };
        Notify(typeInfoListeners_, ITypeInfoListener::EventType::ADDED, infos);
    }
}

void PluginRegistry::UnregisterTypeInfo(const ITypeInfo& type)
{
    if (const auto typeInfos = typeInfos_.find(type.typeUid); typeInfos != typeInfos_.cend()) {
        auto& infos = typeInfos->second;
        if (const auto info = std::find(infos.cbegin(), infos.cend(), &type); info != infos.cend()) {
            infos.erase(info);
        }
    }

    const ITypeInfo* const infos[] = { &type };
    Notify(typeInfoListeners_, ITypeInfoListener::EventType::REMOVED, infos);
}

array_view<const ITypeInfo* const> PluginRegistry::GetTypeInfos(const Uid& typeUid) const
{
    if (const auto typeInfos = typeInfos_.find(typeUid); typeInfos != typeInfos_.cend()) {
        return typeInfos->second;
    }
    return {};
}

void PluginRegistry::AddListener(ITypeInfoListener& listener)
{
    if (std::none_of(typeInfoListeners_.begin(), typeInfoListeners_.end(),
            [adding = &listener](const auto& current) { return current == adding; })) {
        typeInfoListeners_.push_back(&listener);
    }
}

void PluginRegistry::RemoveListener(const ITypeInfoListener& listener)
{
    if (auto pos = std::find(typeInfoListeners_.begin(), typeInfoListeners_.end(), &listener);
        pos != typeInfoListeners_.end()) {
        *pos = nullptr;
    }
}

// IClassRegister
void PluginRegistry::RegisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    // keep interfaceTypeInfos_ sorted according to UIDs
    const auto pos = std::upper_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
        [](Uid value, const InterfaceTypeInfo* element) { return value < element->uid; });
    interfaceTypeInfos_.insert(pos, &interfaceInfo);
}

void PluginRegistry::UnregisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    if (!interfaceTypeInfos_.empty()) {
        const auto pos = std::lower_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
            [](const InterfaceTypeInfo* element, Uid value) { return element->uid < value; });
        if ((pos != interfaceTypeInfos_.cend()) && (*pos)->uid == interfaceInfo.uid) {
            interfaceTypeInfos_.erase(pos);
        }
    }
}

array_view<const InterfaceTypeInfo* const> PluginRegistry::GetInterfaceMetadata() const
{
    return { interfaceTypeInfos_.data(), interfaceTypeInfos_.size() };
}

const InterfaceTypeInfo& PluginRegistry::GetInterfaceMetadata(const Uid& uid) const
{
    static constexpr InterfaceTypeInfo invalidType {};

    if (!interfaceTypeInfos_.empty()) {
        const auto pos = std::lower_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), uid,
            [](const InterfaceTypeInfo* element, Uid value) { return element->uid < value; });
        if ((pos != interfaceTypeInfos_.cend()) && (*pos)->uid == uid) {
            return *(*pos);
        }
    }
    return invalidType;
}

IInterface* PluginRegistry::GetInstance(const Uid& uid) const
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.getInterface) {
        return data.getInterface(const_cast<PluginRegistry&>(*this), data.token);
    }
    return nullptr;
}

// IClassFactory
IInterface::Ptr PluginRegistry::CreateInstance(const Uid& uid)
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.createInterface) {
        return IInterface::Ptr { data.createInterface(*this, data.token) };
    }
    return {};
}

// IInterface
const IInterface* PluginRegistry::GetInterface(const Uid& uid) const
{
    return const_cast<PluginRegistry*>(this)->GetInterface(uid);
}

IInterface* PluginRegistry::GetInterface(const Uid& uid)
{
    if ((uid == IInterface::UID) || (uid == IClassRegister::UID)) {
        return static_cast<IClassRegister*>(this);
    }
    if (uid == IClassFactory::UID) {
        return static_cast<IClassFactory*>(this);
    }
    return nullptr;
}

void PluginRegistry::Ref() {}

void PluginRegistry::Unref() {}

// Public members
void PluginRegistry::RegisterPluginPath(const string_view path)
{
    if (!fileProtocolRegistered_) {
        fileProtocolRegistered_ = true;
        fileManager_.RegisterFilesystem("file", IFilesystem::Ptr { new StdFilesystem("/") });
    }
    fileManager_.RegisterPath("plugins", path, false);
}

IFileManager& PluginRegistry::GetFileManager()
{
    return fileManager_;
}

// Private members
void PluginRegistry::RegisterPlugin(ILibrary::Ptr lib, const IPlugin& plugin, bool asDependency)
{
    CORE_LOG_D("\tRegister Plugin: %s %s", plugin.name, to_string(plugin.version.uid).data());
    if (plugin.version.GetVersionString) {
        CORE_LOG_D("\tVersion Info: %s", plugin.version.GetVersionString());
    }
    if (std::any_of(plugins_.begin(), plugins_.end(),
            [&plugin](const IPlugin* pl) { return strcmp(plugin.name, pl->name) == 0; })) {
        CORE_LOG_W("\tSkipping duplicate plugin: %s!", plugin.name);
        return;
    }
    // when a plugin is loaded/ registered due a requested plugin depends on it ref count starts from zero and it's
    // expected that some following plugin will place a reference. once that plugin releases its reference the
    // dependency is known to be a candidate for unloading.
    PluginData pd { move(lib), {}, asDependency ? 0 : 1 };
    if (plugin.registerInterfaces) {
        pd.token = plugin.registerInterfaces(*static_cast<IPluginRegister*>(this));
    }

    pluginDatas_.push_back(move(pd));
    plugins_.push_back(&plugin);
    for (const auto& dependency : plugin.pluginDependencies) {
        if (auto pos = std::find_if(plugins_.begin(), plugins_.end(),
                [dependency](const IPlugin* plugin) { return plugin->version.uid == dependency; });
            pos != plugins_.end()) {
            const auto index = static_cast<size_t>(std::distance(plugins_.begin(), pos));
            ++pluginDatas_[index].refcnt;
        }
    }
}

void PluginRegistry::UnregisterPlugin(const IPlugin& plugin, PluginToken token)
{
    CORE_LOG_D("\tUnregister Plugin: %s %s", plugin.name, to_string(plugin.version.uid).data());

    if (plugin.unregisterInterfaces) {
        plugin.unregisterInterfaces(token);
    }
}

// Library exports
CORE_PUBLIC IPluginRegister& GetPluginRegister()
{
    static PluginRegistry registry;
    return registry;
}

CORE_PUBLIC void CreatePluginRegistry(const PlatformCreateInfo& platformCreateInfo)
{
    static bool once = false;
    if (!once) {
        once = true;
        auto& registry = static_cast<PluginRegistry&>(GetPluginRegister());
        // Create plugins:// protocol that points to plugin files.
        // register path to system plugins
        // Root path is the location where system plugins , non-rofs assets etc could be held.
        auto platform = Platform::Create(platformCreateInfo);
        platform->RegisterPluginLocations(registry);
    }
}
CORE_END_NAMESPACE()
