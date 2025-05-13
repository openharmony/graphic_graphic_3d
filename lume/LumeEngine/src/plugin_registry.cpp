/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <platform/common/core/os/extensions_create_info.h>

#include <base/util/errors.h>
#include <base/util/uid_util.h>
#include <core/ecs/intf_component_manager.h>
#include <core/implementation_uids.h>
#include <core/log.h>

#include "image/loaders/image_loader_ktx.h"
#include "image/loaders/image_loader_stb_image.h"
#include "io/file_manager.h"
#include "io/std_filesystem.h"
#include "os/intf_library.h"
#if (CORE_PERF_ENABLED == 1)
#include "perf/performance_data_manager.h"
#endif
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
extern const CORE_NS::IPlugin* const g_staticPluginList;
extern const CORE_NS::IPlugin* const g_staticPluginListData;
extern const CORE_NS::IPlugin* const g_staticPluginListEnd;
__attribute__((used)) const CORE_NS::IPlugin* const g_staticPluginListDataRef = g_staticPluginListData;
}
#endif

#else

#if _MSC_VER
static const CORE_NS::IPlugin* const* g_staticPluginList = nullptr;
static size_t g_staticPluginListCount = 0;
#else
__attribute__((visibility("hidden"))) static const CORE_NS::IPlugin* const* g_staticPluginList = nullptr;
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

constexpr bool operator==(const CORE_NS::IPlugin* lhs, const BASE_NS::Uid& rhs) noexcept
{
    return lhs->version.uid == rhs;
}

namespace {
struct LibPlugin {
    ILibrary::Ptr lib;
    const IPlugin* plugin;
};

constexpr bool operator==(const CORE_NS::LibPlugin& libPlugin, const BASE_NS::Uid& rhs) noexcept
{
    return libPlugin.plugin->version.uid == rhs;
}

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
    IDirectory::Ptr pluginFiles = fileManager.OpenDirectory(pluginRoot);
    if (!pluginFiles) {
        return;
    }
    for (const auto& file : pluginFiles->GetEntries()) {
        const string_view pluginFile = file.name;
        if (!pluginFile.ends_with(libraryFileExtension)) {
            continue;
        }
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

bool AddDependencies(vector<Uid>& toBeLoaded, array_view<const LibPlugin> availablePlugins, const Uid& uidToLoad)
{
    bool found = true;
    if (auto pos = FindIf(availablePlugins,
            [&uidToLoad](const LibPlugin& libPlugin) { return libPlugin.plugin->version.uid == uidToLoad; });
        pos != availablePlugins.end()) {
        found = AllOf(pos->plugin->pluginDependencies,
            [&](const Uid& dependency) { return AddDependencies(toBeLoaded, availablePlugins, dependency); });
        if (found) {
            toBeLoaded.push_back(uidToLoad);
        } else {
            CORE_LOG_E("Missing dependencies for: %s", to_string(uidToLoad).data());
        }
    } else {
        CORE_LOG_E("Plugin not found: %s", to_string(uidToLoad).data());
        found = false;
    }
    return found;
}

vector<Uid> GatherRequiredPlugins(const array_view<const Uid> pluginUids, const array_view<const LibPlugin> plugins)
{
    vector<Uid> loadAll(pluginUids.cbegin(), pluginUids.cend());
    if (pluginUids.empty()) {
        loadAll.reserve(plugins.size());
        for (auto& plugin : plugins) {
            loadAll.push_back(plugin.plugin->version.uid);
        }
    }

    if (loadAll.empty()) {
        return {};
    }

    // Gather dependencies of each plugin. toLoad will contain all the dependencies in depth first order.
    vector<Uid> toLoad;
    toLoad.reserve(plugins.size());
    if (!AllOf(loadAll, [&](const Uid& uid) { return AddDependencies(toLoad, plugins, uid); })) {
        toLoad.clear();
    }
    return toLoad;
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

constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo KTX_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "306357a4-d49c-4670-9746-5ccbba567dc9" },
    CreateImageLoaderKtx,
    KTX_IMAGE_TYPES,
};

constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo STB_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "a5049cb8-10bb-4047-b7f5-e9939d5bb3a5" },
    CreateImageLoaderStbImage,
    STB_IMAGE_TYPES,
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

    return interfaces;
}

void PluginRegistry::UnregisterGlobalInterfaces()
{
    UnregisterTypeInfo(STB_LOADER);
    UnregisterTypeInfo(KTX_LOADER);

    for (const auto& info : ownInterfaceInfos_) {
        UnregisterInterfaceType(info);
    }
}

WARNING_SCOPE_START(W_THIS_USED_BASE_INITIALIZER_LIST)
PluginRegistry::PluginRegistry()
#if CORE_PERF_ENABLED
    : perfManFactory_(*this)
#endif // CORE_PERF_ENABLED
{
    ownInterfaceInfos_ = RegisterGlobalInterfaces(*this);
}
WARNING_SCOPE_END()

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
    vector<LibPlugin> availablePlugins;
    GatherStaticPlugins(availablePlugins);
    GatherDynamicPlugins(availablePlugins, fileManager_);

    vector<Uid> toLoad = GatherRequiredPlugins(pluginUids, availablePlugins);
    if (toLoad.empty()) {
        return false;
    }

    // Remove duplicates while counting how many times plugins were requested due to dependencies.
    vector<int32_t> counts;
    counts.reserve(toLoad.size());

    if (toLoad.size() > 1U) {
        auto begin = toLoad.begin();
        auto end = toLoad.end();
        while ((begin + 1) != end) {
            auto newEnd = std::remove_if(begin + 1, end, [current = *begin](const Uid& uid) { return uid == current; });
            auto count = static_cast<int32_t>(std::distance(newEnd, end)) + 1;
            counts.push_back(count);
            end = newEnd;
            ++begin;
        }
        toLoad.erase(end, toLoad.cend());
    }
    counts.push_back(1);

    loading_ = true;
    auto itCounts = counts.cbegin();
    for (const Uid& uid : toLoad) {
        const auto currentCount = *itCounts++;
        // If the plugin was already loaded just increase the reference count.
        if (auto pos = std::find(plugins_.begin(), plugins_.end(), uid); pos != plugins_.end()) {
            const auto index = static_cast<size_t>(std::distance(plugins_.begin(), pos));
            pluginDatas_[index].refcnt += currentCount;
            continue;
        }

        auto pos = std::find(availablePlugins.begin(), availablePlugins.end(), uid);
        if (pos == availablePlugins.end()) {
            // This shouldn't be possible as toLoad should contain UIDs which are found in plugins.
            continue;
        }
        if (!pos->plugin) {
            // This shouldn't be possible as GatherStatic/DynamicPlugins should add only valid plugin pointers, lib can
            // be null.
            continue;
        }
        RegisterPlugin(BASE_NS::move(pos->lib), *(pos->plugin), currentCount);
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
#if defined(CORE_PERF_ENABLED) && (CORE_PERF_ENABLED)
    if (perfLoggerId_) {
        if (pluginUids.empty() || std::any_of(pluginUids.cbegin(), pluginUids.cend(),
                                      [&perfUid = perfTracePlugin_](const Uid& uid) { return uid == perfUid; })) {
            logger_.RemoveOutput(perfLoggerId_);
            perfLoggerId_ = 0U;
        }
    }
#endif
    // to allow plugins to call UnloadPlugins from unregisterInterfaces, gather unloadable entries into separate
    // containers to avoid modifying member containers during iteration.
    decltype(pluginDatas_) removedPluginDatas;
    decltype(plugins_) removedPlugins;
    if (pluginUids.empty()) {
        while (!pluginDatas_.empty() && !plugins_.empty()) {
            removedPlugins.push_back(BASE_NS::move(plugins_.back()));
            removedPluginDatas.push_back(BASE_NS::move(pluginDatas_.back()));
            plugins_.pop_back();
            pluginDatas_.pop_back();
        }
    } else {
        DecreaseRefCounts(pluginUids);

        auto pdIt = pluginDatas_.rbegin();
        for (auto pos = plugins_.rbegin(), last = plugins_.rend(); pos != last;) {
            if (pdIt->refcnt <= 0) {
                removedPlugins.push_back(BASE_NS::move(*pos));
                removedPluginDatas.push_back(BASE_NS::move(*pdIt));
                plugins_.erase(pos.base() - 1);
                pluginDatas_.erase(pdIt.base() - 1);
            }
            ++pos;
            ++pdIt;
        }
    }
    // now it's safe to call unregisterInterfaces without messing up the iteration of pluginDatas_ and plugins_.
    auto dataIt = removedPluginDatas.begin();
    for (auto plugin : removedPlugins) {
        UnregisterPlugin(*plugin, dataIt->token);
        ++dataIt;
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

#if defined(CORE_PERF_ENABLED) && (CORE_PERF_ENABLED)
    if (type.typeUid == PerformanceTraceTypeInfo::UID) {
        perfManFactory_.RemovePerformanceTrace(static_cast<const PerformanceTraceTypeInfo&>(type).uid);
    }
#endif
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

// Internal members
void PluginRegistry::HandlePerfTracePlugin(const PlatformCreateInfo& platformCreateInfo)
{
    const PlatformCreateExtensionInfo* traceSettings = nullptr;
    for (const PlatformCreateExtensionInfo* extension = Platform::Extensions(platformCreateInfo); extension;
         extension = extension->next) {
        switch (extension->type) {
            case PLATFORM_EXTENSION_TRACE_USER:
                CORE_ASSERT(traceSettings == nullptr);
                traceSettings = extension;
                break;
            case PLATFORM_EXTENSION_TRACE_EXTENSION:
                CORE_ASSERT(traceSettings == nullptr);
                traceSettings = extension;
                break;
        }
    }

    // attempt to initialize performance tracing before any other subsystems, performance tracing should come from
    // an dynamic plugin so ideally the dynamic module is loaded automatically during loading of LumeEngine but
    // before any other plugins are loaded so that other plugins.
#if defined(CORE_PERF_ENABLED) && (CORE_PERF_ENABLED)
    if (traceSettings) {
        CORE_LOG_V("Tracing is enabled and application requested it");
        if (traceSettings->type == PLATFORM_EXTENSION_TRACE_USER) {
            // Provide a user applied performance tracer from the application
            perfManFactory_.SetPerformanceTrace(
                {}, IPerformanceTrace::Ptr { static_cast<const PlatformTraceInfoUsr*>(traceSettings)->tracer });
        } else if (traceSettings->type == PLATFORM_EXTENSION_TRACE_EXTENSION) {
            // This load plugin call isn't really that ideal, basically dlopen/LoadLibrary is called all plugins
            // found. The order is undefined, so apotentional risks is that the modules could attempt to use
            // tracing (i.e. memory allocation tracking) prior to tracing plugin being effectively loaded.
            //
            // others problem with loading plugins like this is that it might simply be undesireable to map
            // /all/ plugins into memory space for example when two versions of same plugins exists,
            // (hot-)reloading of modules
            const PlatformTraceInfoExt* traceExtension = static_cast<const PlatformTraceInfoExt*>(traceSettings);
            if (!LoadPlugins({ &traceExtension->plugin, 1U })) {
                CORE_LOG_V("Failed to load %s", to_string(traceExtension->plugin).data());
                return;
            }
            perfTracePlugin_ = traceExtension->plugin;
            auto typeInfos = GetTypeInfos(PerformanceTraceTypeInfo::UID);
            auto itt = std::find_if(typeInfos.cbegin(), typeInfos.cend(), [traceExtension](const ITypeInfo* const a) {
                return static_cast<const PerformanceTraceTypeInfo* const>(a)->uid == traceExtension->type;
            });
            if (itt != typeInfos.end()) {
                auto trace = static_cast<const PerformanceTraceTypeInfo* const>(*itt);
                perfManFactory_.SetPerformanceTrace(trace->uid, trace->createLoader(trace->token));
            } else {
                CORE_ASSERT(false && "cannot find trace plugin");
            }
        }

        // Capture console message and forward them to performance tracer
        perfLoggerId_ = logger_.AddOutput(perfManFactory_.GetLogger());
    } else {
        CORE_LOG_V("Tracing is enabled and application didn't requested it");
    }
#else
    if (traceSettings) {
        CORE_LOG_V("Tracing is disabled but application still requested it");
    }
#endif
}

// Private members
void PluginRegistry::RegisterPlugin(ILibrary::Ptr lib, const IPlugin& plugin, const int32_t refCount)
{
    CORE_LOG_D("\tRegister Plugin: %s %s", plugin.name, to_string(plugin.version.uid).data());
    if (plugin.version.GetVersionString) {
        CORE_LOG_D("\tVersion Info: %s", plugin.version.GetVersionString());
    }
    // when a plugin is loaded/ registered due a requested plugin depends on it ref count starts from zero and it's
    // expected that some following plugin will place a reference. once that plugin releases its reference the
    // dependency is known to be a candidate for unloading.
    PluginData pd { move(lib), {}, refCount };
    if (plugin.registerInterfaces) {
        pd.token = plugin.registerInterfaces(*static_cast<IPluginRegister*>(this));
    }

    pluginDatas_.push_back(move(pd));
    plugins_.push_back(&plugin);
}

void PluginRegistry::UnregisterPlugin(const IPlugin& plugin, PluginToken token)
{
    CORE_LOG_D("\tUnregister Plugin: %s %s", plugin.name, to_string(plugin.version.uid).data());

    if (plugin.unregisterInterfaces) {
        plugin.unregisterInterfaces(token);
    }
}

void PluginRegistry::DecreaseRefCounts(const array_view<const Uid> pluginUids)
{
    for (const auto& uid : pluginUids) {
        if (auto pos = std::find_if(
                plugins_.begin(), plugins_.end(), [uid](const IPlugin* pl) { return pl && pl->version.uid == uid; });
            pos != plugins_.end()) {
            const auto index = static_cast<size_t>(std::distance(plugins_.begin(), pos));
            --pluginDatas_[index].refcnt;
            DecreaseRefCounts((*pos)->pluginDependencies);
        }
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

        auto platform = Platform::Create(platformCreateInfo);
        platform->RegisterPluginLocations(registry);

        registry.HandlePerfTracePlugin(platformCreateInfo);
    }
}
CORE_END_NAMESPACE()
