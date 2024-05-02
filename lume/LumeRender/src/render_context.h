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

#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "loader/render_data_configuration_loader.h"
#include "nodecontext/render_node_post_process_util.h"

CORE_BEGIN_NAMESPACE()
class IEngine;
class IFileManager;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class Device;
class Renderer;
class RenderDataStoreManager;
class RenderNodeManager;
class RenderNodeGraphManager;
class RenderNodeGraphLoader;
class RenderUtil;

class IDevice;
class IRenderer;
class IRenderDataStoreManager;
class IRenderNodeGraphManager;
class IRenderUtil;

struct RenderPluginState;
struct IRenderPlugin;

class RenderContext final : public IRenderContext,
                            public CORE_NS::IClassRegister,
                            CORE_NS::IPluginRegister::ITypeInfoListener {
public:
    RenderContext(RenderPluginState& pluginState, CORE_NS::IEngine& engine);
    ~RenderContext() override;

    /** Init, create device for render use.
     *  @param createInfo Device creation info.
     */
    RenderResultCode Init(const RenderCreateInfo& createInfo) override;

    /** Get active device. */
    IDevice& GetDevice() const override;
    /** Get rendererer */
    IRenderer& GetRenderer() const override;
    /** Get render node graph manager */
    IRenderNodeGraphManager& GetRenderNodeGraphManager() const override;
    /** Get render data store manager */
    IRenderDataStoreManager& GetRenderDataStoreManager() const override;

    /** Get render utilities */
    IRenderUtil& GetRenderUtil() const override;

    CORE_NS::IEngine& GetEngine() const override;

    BASE_NS::string_view GetVersion() override;

    RenderCreateInfo GetCreateInfo() const;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

    // IClassFactory
    IInterface::Ptr CreateInstance(const BASE_NS::Uid& uid) override;

    // IClassRegister
    void RegisterInterfaceType(const CORE_NS::InterfaceTypeInfo& interfaceInfo) override;
    void UnregisterInterfaceType(const CORE_NS::InterfaceTypeInfo& interfaceInfo) override;
    BASE_NS::array_view<const CORE_NS::InterfaceTypeInfo* const> GetInterfaceMetadata() const override;
    const CORE_NS::InterfaceTypeInfo& GetInterfaceMetadata(const BASE_NS::Uid& uid) const override;
    IInterface* GetInstance(const BASE_NS::Uid& uid) const override;

private:
    // IPluginRegister::ITypeInfoListener
    void OnTypeInfoEvent(EventType type, BASE_NS::array_view<const CORE_NS::ITypeInfo* const> typeInfos) override;

    void RegisterDefaultPaths();
    BASE_NS::unique_ptr<Device> CreateDevice(const DeviceCreateInfo& createInfo);

    RenderPluginState& pluginState_;
    CORE_NS::IEngine& engine_;
    CORE_NS::IFileManager* fileManager_ { nullptr };
    BASE_NS::unique_ptr<Device> device_;
    BASE_NS::unique_ptr<RenderDataStoreManager> renderDataStoreMgr_;
    BASE_NS::unique_ptr<RenderNodeGraphManager> renderNodeGraphMgr_;
    BASE_NS::unique_ptr<Renderer> renderer_;
    BASE_NS::unique_ptr<RenderUtil> renderUtil_;
    RenderDataConfigurationLoaderImpl renderDataConfigurationLoader_;

    CORE_NS::InterfaceTypeInfo interfaceInfos_[2U] {
        CORE_NS::InterfaceTypeInfo {
            this,
            UID_RENDER_DATA_CONFIGURATION_LOADER,
            CORE_NS::GetName<IRenderDataConfigurationLoader>().data(),
            {}, // nullptr for CreateInstance
            [](CORE_NS::IClassRegister& registry, CORE_NS::PluginToken token) -> CORE_NS::IInterface* {
                if (token) {
                    return &(static_cast<RenderContext*>(token)->renderDataConfigurationLoader_);
                }
                return nullptr;
            },
        },
        CORE_NS::InterfaceTypeInfo {
            this, UID_RENDER_NODE_POST_PROCESS_UTIL, CORE_NS::GetName<IRenderNodePostProcessUtil>().data(),
            [](CORE_NS::IClassFactory&, CORE_NS::PluginToken token) -> CORE_NS::IInterface* {
                if (token) {
                    return new RenderNodePostProcessUtilImpl();
                }
                return nullptr;
            },
            nullptr, // nullptr for GetInstance
        },
    };

    uint32_t refCount_ { 0 };

    BASE_NS::vector<BASE_NS::pair<CORE_NS::PluginToken, const IRenderPlugin*>> plugins_;
    BASE_NS::vector<const CORE_NS::InterfaceTypeInfo*> interfaceTypeInfos_;

    BASE_NS::vector<RenderHandleReference> defaultGpuResources_;

    RenderCreateInfo createInfo_ {};
};

struct RenderPluginState {
    CORE_NS::IEngine& engine_;
    IRenderContext::Ptr context_;
    CORE_NS::InterfaceTypeInfo interfaceInfo_ {
        this,
        UID_RENDER_CONTEXT,
        CORE_NS::GetName<IRenderContext>().data(),
        [](CORE_NS::IClassFactory& factory, CORE_NS::PluginToken token) -> CORE_NS::IInterface* {
            if (token) {
                auto state = static_cast<RenderPluginState*>(token);
                return static_cast<RenderPluginState*>(token)->CreateInstance(state->engine_);
            }
            return nullptr;
        },
        [](CORE_NS::IClassRegister& registry, CORE_NS::PluginToken token) -> CORE_NS::IInterface* {
            if (token) {
                return static_cast<RenderPluginState*>(token)->GetInstance();
            }
            return nullptr;
        },
    };

    IRenderContext* CreateInstance(CORE_NS::IEngine& engine);
    IRenderContext* GetInstance();
    void Destroy();
};
RENDER_END_NAMESPACE()

#endif // RENDER_CONTEXT_H
