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

#ifndef CORE_PLUGIN_REGISTRY_H
#define CORE_PLUGIN_REGISTRY_H

#include <base/containers/array_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include "engine_factory.h"
#include "io/file_manager.h"
#include "loader/system_graph_loader.h"
#include "log/logger.h"
#include "os/intf_library.h"
#include "os/platform.h"
#if (CORE_PERF_ENABLED == 1)
#include "perf/performance_data_manager.h"
#endif
#include "threading/task_queue_factory.h"
#include "util/frustum_util.h"

CORE_BEGIN_NAMESPACE()
/**
    Registry for interfaces that are engine independent.
*/
class PluginRegistry final : public IPluginRegister, public IClassRegister, public IClassFactory {
public:
    PluginRegistry();
    ~PluginRegistry();

    // IPluginRegister
    BASE_NS::array_view<const IPlugin* const> GetPlugins() const override;
    bool LoadPlugins(const BASE_NS::array_view<const BASE_NS::Uid> pluginUids) override;
    void UnloadPlugins(const BASE_NS::array_view<const BASE_NS::Uid> pluginUids) override;
    IClassRegister& GetClassRegister() const override;
    void RegisterTypeInfo(const ITypeInfo& type) override;
    void UnregisterTypeInfo(const ITypeInfo& type) override;
    BASE_NS::array_view<const ITypeInfo* const> GetTypeInfos(const BASE_NS::Uid& typeUid) const override;
    void AddListener(ITypeInfoListener& listener) override;
    void RemoveListener(const ITypeInfoListener& listener) override;

    // IClassRegister
    void RegisterInterfaceType(const InterfaceTypeInfo& interfaceInfo) override;
    void UnregisterInterfaceType(const InterfaceTypeInfo& interfaceInfo) override;
    BASE_NS::array_view<const InterfaceTypeInfo* const> GetInterfaceMetadata() const override;
    const InterfaceTypeInfo& GetInterfaceMetadata(const BASE_NS::Uid& uid) const override;
    IInterface* GetInstance(const BASE_NS::Uid& uid) const override;

    // IClassFactory
    IInterface::Ptr CreateInstance(const BASE_NS::Uid& uid) override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

    void RegisterPluginPath(const BASE_NS::string_view path) override;
    IFileManager& GetFileManager() override;

protected:
    static BASE_NS::vector<InterfaceTypeInfo> RegisterGlobalInterfaces(PluginRegistry& registry);
    void UnregisterGlobalInterfaces();

    struct PluginData {
        ILibrary::Ptr library;
        PluginToken token;
        int32_t refcnt;
    };

    void RegisterPlugin(ILibrary::Ptr lib, const IPlugin& plugin, bool asDependency);
    static void UnregisterPlugin(const IPlugin& plugin, PluginToken token);

    BASE_NS::vector<PluginData> pluginDatas_;
    BASE_NS::vector<const IPlugin*> plugins_;

    BASE_NS::unordered_map<BASE_NS::Uid, BASE_NS::vector<const ITypeInfo*>> typeInfos_;
    BASE_NS::vector<const ITypeInfo*> newTypeInfos_;
    BASE_NS::vector<const ITypeInfo*> oldTypeInfos_;

    BASE_NS::vector<const InterfaceTypeInfo*> interfaceTypeInfos_;

    BASE_NS::vector<ITypeInfoListener*> typeInfoListeners_;

    Logger logger_ { true };
    EngineFactory engineFactory_;
    SystemGraphLoaderFactory systemGraphLoadeFactory;
    FrustumUtil frustumUtil_;
    TaskQueueFactory taskQueueFactory_;
#if (CORE_PERF_ENABLED == 1)
    PerformanceDataManagerFactory perfManFactory_;
#endif
    BASE_NS::vector<InterfaceTypeInfo> ownInterfaceInfos_;
    FileManager fileManager_;
    bool fileProtocolRegistered_ { false };
    bool loading_ { false };
};
CORE_END_NAMESPACE()

#endif // CORE_PLUGIN_REGISTRY_H
