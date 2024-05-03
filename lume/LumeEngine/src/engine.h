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

#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/ecs/intf_ecs.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

BASE_BEGIN_NAMESPACE()
struct Uid;
template<class T1, class T2>
struct pair;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IImageLoaderManager;
class IPlatform;
class IThreadPool;

class Engine final : public IEngine, virtual public IClassRegister, IPluginRegister::ITypeInfoListener {
public:
    explicit Engine(EngineCreateInfo const& createInfo);
    ~Engine() override;

    void Init() override;
    bool TickFrame() override;
    bool TickFrame(const BASE_NS::array_view<IEcs*>& ecsInputs) override;
    IFileManager& GetFileManager() override;

    IImageLoaderManager& GetImageLoaderManager() override;

    const IPlatform& GetPlatform() const override;

    EngineTime GetEngineTime() const override;

    IEcs::Ptr CreateEcs() override;
    IEcs::Ptr CreateEcs(IThreadPool& threadPool) override;

    BASE_NS::string_view GetVersion() override;
    bool IsDebugBuild() override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

    // IClassFactory
    IInterface::Ptr CreateInstance(const BASE_NS::Uid& uid) override;

    // IClassRegister
    void RegisterInterfaceType(const InterfaceTypeInfo& interfaceInfo) override;
    void UnregisterInterfaceType(const InterfaceTypeInfo& interfaceInfo) override;
    BASE_NS::array_view<const InterfaceTypeInfo* const> GetInterfaceMetadata() const override;
    const InterfaceTypeInfo& GetInterfaceMetadata(const BASE_NS::Uid& uid) const override;
    IInterface* GetInstance(const BASE_NS::Uid& uid) const override;

    // IPluginRegister::ITypeInfoListener
    void OnTypeInfoEvent(EventType type, BASE_NS::array_view<const ITypeInfo* const> typeInfos) override;

private:
    void RegisterDefaultPaths();
    void LoadPlugins();
    void UnloadPlugins();
    static bool TickFrame(IEcs& ecs, uint64_t totalTime, uint64_t deltaTime);

    uint64_t firstTime_ { ~0u };
    uint64_t previousFrameTime_ { ~0u };
    uint64_t deltaTime_ { 1 };

    BASE_NS::unique_ptr<IPlatform> platform_;
    ContextInfo applicationContext_;

    IFileManager::Ptr fileManager_;

    BASE_NS::unique_ptr<class ImageLoaderManager> imageManager_;
    uint32_t refCount_ { 0 };

    BASE_NS::vector<BASE_NS::pair<PluginToken, const IEnginePlugin*>> plugins_;
    BASE_NS::vector<const InterfaceTypeInfo*> interfaceTypeInfos_;
};
CORE_END_NAMESPACE()

#endif // CORE_ENGINE_H
