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

#include "engine.h"

#include <algorithm>
#include <chrono>
#include <cstring>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/ecs/intf_ecs.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_filesystem_api.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/threading/intf_thread_pool.h>

#include "image/image_loader_manager.h"
#include "image/loaders/image_loader_ktx.h"
#include "image/loaders/image_loader_stb_image.h"
#include "os/platform.h"

#if (CORE_PERF_ENABLED == 1)
#include "perf/performance_data_manager.h"
#endif

CORE_BEGIN_NAMESPACE()
namespace {
#if (CORE_EMBEDDED_ASSETS_ENABLED == 1)
// Core Rofs Data.
extern "C" const uint64_t SIZEOFDATAFORCORE;
extern "C" const void* const BINARYDATAFORCORE[];
#endif

using BASE_NS::array_view;
using BASE_NS::make_unique;
using BASE_NS::pair;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::Uid;

// This is defined in the CMake generated version.cpp
void LogEngineBuildInfo()
{
#define CORE_TO_STRING_INTERNAL(x) #x
#define CORE_TO_STRING(x) CORE_TO_STRING_INTERNAL(x)

#ifdef NDEBUG
    CORE_LOG_I("Core engine version: %s", GetVersion().data());
#else
    CORE_LOG_I("Version: %s (DEBUG)", GetVersion().data());
#endif

    CORE_LOG_I("CORE_VALIDATION_ENABLED=" CORE_TO_STRING(CORE_VALIDATION_ENABLED));
    CORE_LOG_I("CORE_DEV_ENABLED=" CORE_TO_STRING(CORE_DEV_ENABLED));
}
} // namespace

Engine::Engine(EngineCreateInfo const& createInfo)
    : platform_(Platform::Create(createInfo.platformCreateInfo)), applicationContext_(createInfo.applicationContext)
{
    LogEngineBuildInfo();
    auto factory = CORE_NS::GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    fileManager_ = factory->CreateFilemanager();
    fileManager_->RegisterFilesystem("file", factory->CreateStdFileSystem());
    fileManager_->RegisterFilesystem("memory", factory->CreateMemFileSystem());
#if (CORE_EMBEDDED_ASSETS_ENABLED == 1)
    fileManager_->RegisterFilesystem(
        "corerofs", factory->CreateROFilesystem(BINARYDATAFORCORE, static_cast<size_t>(SIZEOFDATAFORCORE)));
#endif

    RegisterDefaultPaths();
}

Engine::~Engine()
{
    GetPluginRegister().RemoveListener(*this);

#if (CORE_PERF_ENABLED == 1)
    if (auto perfFactory = CORE_NS::GetInstance<IPerformanceDataManagerFactory>(UID_PERFORMANCE_FACTORY); perfFactory) {
        for (const auto& perfMan : perfFactory->GetAllCategories()) {
            CORE_LOG_I("%s PerformanceData for this run:", perfMan->GetCategory().data());
            static_cast<const PerformanceDataManager&>(*perfMan).DumpToLog();
        }
    }
#endif

    UnloadPlugins();

    fileManager_.reset();
}

CORE_NS::IEcs* IEcsInstance(IClassFactory&, const IThreadPool::Ptr&);

IEcs::Ptr Engine::CreateEcs()
{
    // construct a secondary ecs instance.
    if (auto threadFactory = CORE_NS::GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY); threadFactory) {
        auto threadPool = threadFactory->CreateThreadPool(threadFactory->GetNumberOfCores());
        return IEcs::Ptr { IEcsInstance(*this, threadPool) };
    }

    return IEcs::Ptr {};
}

IEcs::Ptr Engine::CreateEcs(IThreadPool& threadPool)
{
    return IEcs::Ptr { IEcsInstance(*this, IThreadPool::Ptr { &threadPool }) };
}

void Engine::Init()
{
    CORE_LOG_D("Engine init.");

    imageManager_ = make_unique<ImageLoaderManager>(*fileManager_);

    // Pre-register some basic image formats.

    LoadPlugins();

    GetPluginRegister().AddListener(*this);
}

void Engine::RegisterDefaultPaths()
{
    platform_->RegisterDefaultPaths(*fileManager_);
#if (CORE_EMBEDDED_ASSETS_ENABLED == 1)
    // Create engine:// protocol that points to embedded engine asset files.
    CORE_LOG_D("Registered core asset path: 'corerofs://core/'");
    fileManager_->RegisterPath("engine", "corerofs://core/", false);
#endif

    // Create shaders:// protocol that points to shader files.
    fileManager_->RegisterPath("shaders", "engine://shaders/", false);
    // Create shaderstates:// protocol that points to (graphics) shader state files.
    fileManager_->RegisterPath("shaderstates", "engine://shaderstates/", false);
    // Create vertexinputdeclarations:// protocol that points to vid files.
    fileManager_->RegisterPath("vertexinputdeclarations", "engine://vertexinputdeclarations/", false);
    // Create pipelinelayouts:// protocol that points to vid files.
    fileManager_->RegisterPath("pipelinelayouts", "engine://pipelinelayouts/", false);
    // Create renderdataconfigurations:// protocol that points to render byteData configurations.
    fileManager_->RegisterPath("renderdataconfigurations", "engine://renderdataconfigurations/", false);
}

void Engine::UnloadPlugins()
{
    for (auto& plugin : plugins_) {
        if (plugin.second->destroyPlugin) {
            plugin.second->destroyPlugin(plugin.first);
        }
    }
}

void Engine::LoadPlugins()
{
    for (auto info : CORE_NS::GetPluginRegister().GetTypeInfos(IEnginePlugin::UID)) {
        if (auto enginePlugin = static_cast<const IEnginePlugin*>(info); enginePlugin && enginePlugin->createPlugin) {
            auto token = enginePlugin->createPlugin(*this);
            plugins_.push_back({ token, enginePlugin });
        }
    }
}

bool Engine::TickFrame()
{
    return TickFrame({ nullptr, size_t(0) });
}

bool Engine::TickFrame(const array_view<IEcs*>& ecsInputs)
{
    using namespace std::chrono;
    const auto currentTime =
        static_cast<uint64_t>(duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());

    if (firstTime_ == ~0u) {
        previousFrameTime_ = firstTime_ = currentTime;
    }
    deltaTime_ = currentTime - previousFrameTime_;
    constexpr auto limitHz = duration_cast<microseconds>(duration<float, std::ratio<1, 15u>>(1)).count();
    if (deltaTime_ > limitHz) {
        deltaTime_ = limitHz; // clamp the time step to no longer than 15hz.
    }
    previousFrameTime_ = currentTime;

    const uint64_t totalTime = currentTime - firstTime_;

    bool needRender = false;
    for (auto& ecs : ecsInputs) {
        if (TickFrame(*ecs, totalTime, deltaTime_)) {
            needRender = true;
        }
    }

    return needRender;
}

bool Engine::TickFrame(IEcs& ecs, uint64_t totalTime, uint64_t deltaTime)
{
    // run garbage collection before updating the systems to ensure only valid entities/ components are available.
    ecs.ProcessEvents();

    const bool needRender = ecs.Update(totalTime, deltaTime);

    // do gc also after the systems have been updated to ensure any deletes done by systems are effective
    // and client doesn't see stale entities.
    ecs.ProcessEvents();

    return needRender;
}

IImageLoaderManager& Engine::GetImageLoaderManager()
{
    CORE_ASSERT_MSG(imageManager_, "Engine not initialized");
    return *imageManager_;
}

IFileManager& Engine::GetFileManager()
{
    CORE_ASSERT_MSG(fileManager_, "Engine not initialized");
    return *fileManager_;
}

EngineTime Engine::GetEngineTime() const
{
    return { previousFrameTime_ - firstTime_, deltaTime_ };
}

const IInterface* Engine::GetInterface(const Uid& uid) const
{
    if ((uid == IEngine::UID) || (uid == IClassFactory::UID) || (uid == IInterface::UID)) {
        return static_cast<const IEngine*>(this);
    }
    if (uid == IClassRegister::UID) {
        return static_cast<const IClassRegister*>(this);
    }

    return nullptr;
}

IInterface* Engine::GetInterface(const Uid& uid)
{
    if ((uid == IEngine::UID) || (uid == IClassFactory::UID) || (uid == IInterface::UID)) {
        return static_cast<IEngine*>(this);
    }
    if (uid == IClassRegister::UID) {
        return static_cast<IClassRegister*>(this);
    }

    return nullptr;
}

void Engine::RegisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    // keep interfaceTypeInfos_ sorted according to UIDs
    const auto pos = std::upper_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
        [](Uid value, const InterfaceTypeInfo* element) { return value < element->uid; });
    interfaceTypeInfos_.insert(pos, &interfaceInfo);
}

void Engine::UnregisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    if (!interfaceTypeInfos_.empty()) {
        const auto pos = std::lower_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
            [](const InterfaceTypeInfo* element, Uid value) { return element->uid < value; });
        if ((pos != interfaceTypeInfos_.cend()) && (*pos)->uid == interfaceInfo.uid) {
            interfaceTypeInfos_.erase(pos);
        }
    }
}

array_view<const InterfaceTypeInfo* const> Engine::GetInterfaceMetadata() const
{
    return interfaceTypeInfos_;
}

const InterfaceTypeInfo& Engine::GetInterfaceMetadata(const Uid& uid) const
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

IInterface* Engine::GetInstance(const Uid& uid) const
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.getInterface) {
        return data.getInterface(const_cast<Engine&>(*this), data.token);
    }
    return nullptr;
}

IInterface::Ptr Engine::CreateInstance(const Uid& uid)
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.createInterface) {
        return IInterface::Ptr { data.createInterface(*this, data.token) };
    }
    return {};
}

void Engine::OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos)
{
    if (type == EventType::ADDED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == IEnginePlugin::UID && static_cast<const IEnginePlugin*>(info)->createPlugin) {
                auto enginePlugin = static_cast<const IEnginePlugin*>(info);
                if (std::none_of(plugins_.begin(), plugins_.end(),
                        [enginePlugin](const pair<PluginToken, const IEnginePlugin*>& pluginData) {
                            return pluginData.second == enginePlugin;
                        })) {
                    auto token = enginePlugin->createPlugin(*this);
                    plugins_.push_back({ token, enginePlugin });
                }
            }
        }
    } else if (type == EventType::REMOVED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == IEnginePlugin::UID) {
                auto enginePlugin = static_cast<const IEnginePlugin*>(info);
                if (auto pos = std::find_if(plugins_.cbegin(), plugins_.cend(),
                        [enginePlugin](const pair<PluginToken, const IEnginePlugin*>& pluginData) {
                            return pluginData.second == enginePlugin;
                        });
                    pos != plugins_.cend()) {
                    if (enginePlugin->destroyPlugin) {
                        enginePlugin->destroyPlugin(pos->first);
                    }
                    plugins_.erase(pos);
                }
            }
        }
    }
}

string_view Engine::GetVersion()
{
    return CORE_NS::GetVersion();
}

bool Engine::IsDebugBuild()
{
    return CORE_NS::IsDebugBuild();
}

IEngine::Ptr CreateEngine(EngineCreateInfo const& createInfo)
{
    auto engine = new Engine(createInfo);
    return IEngine::Ptr { engine };
}

const IPlatform& Engine::GetPlatform() const
{
    return *platform_;
}

void Engine::Ref()
{
    refCount_++;
}

void Engine::Unref()
{
    if (--refCount_ == 0) {
        delete this;
    }
}
CORE_END_NAMESPACE()
