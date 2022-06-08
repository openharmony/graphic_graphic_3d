/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include "os/intf_library.h"
#include "os/platform.h"

CORE_BEGIN_NAMESPACE()
class IEcs;

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

    BASE_NS::string_view GetRootPath() override;

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
    bool TickFrame(IEcs& ecs, uint64_t totalTime, uint64_t deltaTime);

    uint64_t firstTime_ { ~0u };
    uint64_t previousFrameTime_ { ~0u };
    uint64_t deltaTime_ { 1 };

    BASE_NS::unique_ptr<IPlatform> platform_;
    BASE_NS::string rooturi_;
    ContextInfo applicationContext_;

    IFileManager::Ptr fileManager_;

    BASE_NS::unique_ptr<class FileMonitor> fileMonitor_;

    BASE_NS::unique_ptr<class ImageLoaderManager> imageManager_;
    uint32_t refCount_ { 0 };

    BASE_NS::vector<BASE_NS::pair<PluginToken, const IEnginePlugin*>> plugins_;
    BASE_NS::vector<const InterfaceTypeInfo*> interfaceTypeInfos_;
};
CORE_END_NAMESPACE()

#endif // CORE_ENGINE_H
