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

#include "graphics_context.h"

#include <algorithm>

#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/util/uid.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_plugin.h>
#include <render/intf_render_context.h>
#include <render/loader/intf_render_data_configuration_loader.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#if (RENDER_HAS_VULKAN_BACKEND)
#include <render/vulkan/intf_device_vk.h>
#endif

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_plugin.h>
#include <3d/loaders/intf_scene_loader.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>

#include "gltf/gltf2.h"
#include "render/render_node_scene_util.h"
#include "util/mesh_builder.h"
#include "util/mesh_util.h"
#include "util/picking.h"
#include "util/render_util.h"
#include "util/scene_util.h"
#include "util/uri_lookup.h"

extern "C" void InitRegistry(CORE_NS::IPluginRegister& pluginRegistry);

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
struct RegisterPathStrings {
    string_view protocol;
    string_view uri;
};
static constexpr RegisterPathStrings RENDER_DATA_PATHS[] = {
    { "3dshaders", "rofs3D://shaders/" },
    { "3dshaderstates", "rofs3D://shaderstates/" },
    { "3dvertexinputdeclarations", "rofs3D://vertexinputdeclarations/" },
    { "3dpipelinelayouts", "rofs3D://pipelinelayouts/" },
    { "3drenderdataconfigurations", "rofs3D://renderdataconfigurations/" },
    { "3drendernodegraphs", "rofs3D://rendernodegraphs/" },
};

static constexpr IShaderManager::ShaderFilePathDesc SHADER_FILE_PATHS {
    "3dshaders://",
    "3dshaderstates://",
    "3dpipelinelayouts://",
    "3dvertexinputdeclarations://",
};

static constexpr string_view POST_PROCESS_PATH { "3drenderdataconfigurations://postprocess/" };
static constexpr string_view POST_PROCESS_DATA_STORE_NAME { "RenderDataStorePod" };
static constexpr string_view POST_PROCESS_NAME { "PostProcess" };

void CreateDefaultImages(IDevice& device, vector<RenderHandleReference>& defaultGpuResources)
{
    IGpuResourceManager& gpuResourceMgr = device.GetGpuResourceManager();

    // default material gpu images
    GpuImageDesc desc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
        Format::BASE_FORMAT_R8G8B8A8_SRGB, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
        ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, 2, 2, 1, 1, 1,
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };
    constexpr uint32_t sizeOfUint32 = sizeof(uint32_t);

    desc.format = Format::BASE_FORMAT_R8G8B8A8_UNORM;
    {
        constexpr const uint32_t normalData[4u] = { 0xFFFF7f7f, 0xFFFF7f7f, 0xFFFF7f7f, 0xFFFF7f7f };
        const auto normalDataView =
            array_view(reinterpret_cast<const uint8_t*>(normalData), sizeOfUint32 * countof(normalData));
        defaultGpuResources.push_back(gpuResourceMgr.Create(
            DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_NORMAL, desc, normalDataView));
    }

    desc.format = Format::BASE_FORMAT_R8_UNORM;
    {
        constexpr const uint8_t byteData[4u] = { 0xff, 0xff, 0xff, 0xff };
        const auto byteDataView =
            array_view(reinterpret_cast<const uint8_t*>(byteData), sizeof(uint8_t) * countof(byteData));
        defaultGpuResources.push_back(
            gpuResourceMgr.Create(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_AO, desc, byteDataView));
    }

    // env cubemaps
    desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE;
    desc.format = Format::BASE_FORMAT_R8G8B8A8_SRGB;
    desc.createFlags = ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    desc.layerCount = 6u;
    {
        // Env color is currently in rgbd format (alpha channel is used as a divider for the rgb values).
        const vector<uint32_t> cubeData(4u * 6u, 0xFFFFffff);
        const array_view<const uint8_t> cubeDataView(
            reinterpret_cast<const uint8_t*>(cubeData.data()), cubeData.size() * sizeOfUint32);
        defaultGpuResources.push_back(gpuResourceMgr.Create(
            DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP, desc, cubeDataView));

        // Skybox is currently in rgbd format (alpha channel is used as a divider for the rgb values).
        defaultGpuResources.push_back(gpuResourceMgr.Create(
            DefaultMaterialGpuResourceConstants::CORE_DEFAULT_SKYBOX_CUBEMAP, desc, cubeDataView));
    }
}

void CreateDefaultSamplers(IDevice& device, vector<RenderHandleReference>& defaultGpuResources)
{
    IGpuResourceManager& gpuResourceMgr = device.GetGpuResourceManager();
    {
        GpuSamplerDesc sampler {
            Filter::CORE_FILTER_LINEAR,                                  // magFilter
            Filter::CORE_FILTER_LINEAR,                                  // minFilter
            Filter::CORE_FILTER_LINEAR,                                  // mipMapMode
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
        };
        constexpr float CUBE_MAP_LOD_COEFF { 8.0f };
        sampler.minLod = 0.0f;
        sampler.maxLod = CUBE_MAP_LOD_COEFF;
        defaultGpuResources.push_back(
            gpuResourceMgr.Create(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER, sampler));
    }
    {
        GpuSamplerDesc sampler {
            Filter::CORE_FILTER_LINEAR,                                  // magFilter
            Filter::CORE_FILTER_LINEAR,                                  // minFilter
            Filter::CORE_FILTER_LINEAR,                                  // mipMapMode
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
        };
        defaultGpuResources.push_back(
            gpuResourceMgr.Create(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_VSM_SHADOW_SAMPLER, sampler));

        sampler.compareOp = CompareOp::CORE_COMPARE_OP_GREATER;
        sampler.enableCompareOp = true;
        defaultGpuResources.push_back(
            gpuResourceMgr.Create(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_PCF_SHADOW_SAMPLER, sampler));
    }
}

void CreateDefaultResources(IDevice& device, vector<RenderHandleReference>& defaultGpuResources)
{
    CreateDefaultImages(device, defaultGpuResources);
    CreateDefaultSamplers(device, defaultGpuResources);
}
} // namespace

// Core Rofs Data.
extern "C" {
extern const void* const BINARY_DATA_FOR_3D[];
extern const uint64_t SIZE_OF_DATA_FOR_3D;
}

array_view<const RenderDataStoreTypeInfo> GetRenderDataStores3D();
array_view<const RenderNodeTypeInfo> GetRenderNodes3D();
array_view<const ComponentManagerTypeInfo> GetComponentManagers3D();
array_view<const SystemTypeInfo> GetSystems3D();

struct Agp3DPluginState {
    IRenderContext& renderContext;
    unique_ptr<GraphicsContext> context;

    vector<RenderHandleReference> defaultGpuResources;
    vector<string> defaultPostProcesses;

    Picking picker_;
    RenderNodeSceneUtilImpl renderNodeSceneUtil;
    InterfaceTypeInfo interfaces[4] = {
        InterfaceTypeInfo {
            this,
            UID_MESH_BUILDER,
            CORE_NS::GetName<IMeshBuilder>().data(),
            [](IClassFactory&, PluginToken token) -> IInterface* {
                if (token) {
                    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
                    return new MeshBuilder(state->renderContext);
                }
                return nullptr;
            },
            nullptr,
        },
        InterfaceTypeInfo {
            this,
            UID_PICKING,
            CORE_NS::GetName<IPicking>().data(),
            nullptr,
            [](IClassRegister& registry, PluginToken token) -> IInterface* {
                if (token) {
                    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
                    return &state->picker_;
                }
                return nullptr;
            },
        },
        InterfaceTypeInfo {
            this,
            UID_RENDER_NODE_SCENE_UTIL,
            CORE_NS::GetName<IRenderNodeSceneUtil>().data(),
            nullptr,
            [](IClassRegister& registry, PluginToken token) -> IInterface* {
                if (token) {
                    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
                    return &state->renderNodeSceneUtil;
                }
                return nullptr;
            },
        },
        InterfaceTypeInfo {
            this,
            UID_GRAPHICS_CONTEXT,
            CORE_NS::GetName<IGraphicsContext>().data(),
            [](IClassFactory& registry, PluginToken token) -> IInterface* {
                if (token) {
                    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
                    if (!state->context) {
                        state->context = make_unique<GraphicsContext>(*state, state->renderContext);
                    }
                    return state->context->GetInterface(IInterface::UID);
                }
                return nullptr;
            },
            [](IClassRegister& registry, PluginToken token) -> IInterface* {
                if (token) {
                    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
                    return state->context->GetInterface(IInterface::UID);
                }
                return nullptr;
            },
        },
    };

    void Destroy()
    {
        context.reset();
    }
};

GraphicsContext::GraphicsContext(struct Agp3DPluginState& factory, IRenderContext& context)
    : factory_(factory), context_(context)
{}

GraphicsContext ::~GraphicsContext()
{
    GetPluginRegister().RemoveListener(*this);

    for (auto& info : plugins_) {
        if (info.second && info.second->destroyPlugin) {
            info.second->destroyPlugin(info.first);
        }
    }
    if (sceneUtil_ && gltf2_) {
        sceneUtil_->UnregisterSceneLoader(IInterface::Ptr { gltf2_->GetInterface(ISceneLoader::UID) });
    }
}

void GraphicsContext::Init()
{
    if (initialized_) {
        return;
    }
    auto& engine = context_.GetEngine();

    meshUtil_ = make_unique<MeshUtil>(engine);
    gltf2_ = make_unique<Gltf2>(*this);
    sceneUtil_ = make_unique<SceneUtil>(*this);
    renderUtil_ = make_unique<RenderUtil>(*this);
    sceneUtil_->RegisterSceneLoader(IInterface::Ptr { gltf2_->GetInterface(ISceneLoader::UID) });
    initialized_ = true;

    GetPluginRegister().AddListener(*this);

    for (auto info : CORE_NS::GetPluginRegister().GetTypeInfos(I3DPlugin::UID)) {
        if (auto plugin = static_cast<const I3DPlugin*>(info); plugin && plugin->createPlugin) {
            auto token = plugin->createPlugin(*this);
            plugins_.push_back({ token, plugin });
        }
    }
}

IRenderContext& GraphicsContext::GetRenderContext() const
{
    return context_;
}

array_view<const RenderHandleReference> GraphicsContext::GetRenderNodeGraphs(const IEcs& ecs) const
{
    // NOTE: gets the render node graphs from built-in RenderSystem
    if (IRenderSystem* rs = GetSystem<IRenderSystem>(ecs); rs) {
        return rs->GetRenderNodeGraphs();
    } else {
        return {};
    }
}

ISceneUtil& GraphicsContext::GetSceneUtil() const
{
    return *sceneUtil_;
}

IMeshUtil& GraphicsContext::GetMeshUtil() const
{
    return *meshUtil_;
}

IGltf2& GraphicsContext::GetGltf() const
{
    return *gltf2_;
}

IRenderUtil& GraphicsContext::GetRenderUtil() const
{
    return *renderUtil_;
}

const IInterface* GraphicsContext::GetInterface(const Uid& uid) const
{
    if (uid == IGraphicsContext::UID) {
        return static_cast<const IGraphicsContext*>(this);
    } else if (uid == IInterface::UID) {
        return static_cast<const IInterface*>(static_cast<const IGraphicsContext*>(this));
    } else if (uid == IClassRegister::UID) {
        return static_cast<const IClassRegister*>(this);
    } else if (uid == IClassFactory::UID) {
        return static_cast<const IClassFactory*>(this);
    }
    return nullptr;
}

IInterface* GraphicsContext::GetInterface(const Uid& uid)
{
    if (uid == IGraphicsContext::UID) {
        return static_cast<IGraphicsContext*>(this);
    } else if (uid == IInterface::UID) {
        return static_cast<IInterface*>(static_cast<IGraphicsContext*>(this));
    } else if (uid == IClassRegister::UID) {
        return static_cast<IClassRegister*>(this);
    } else if (uid == IClassFactory::UID) {
        return static_cast<IClassFactory*>(this);
    }
    return nullptr;
}

void GraphicsContext::Ref()
{
    refcnt_++;
}

void GraphicsContext::Unref()
{
    if (--refcnt_ == 0) {
        factory_.Destroy();
    }
}

void GraphicsContext::RegisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    // keep interfaceTypeInfos_ sorted according to UIDs
    const auto pos = std::upper_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
        [](Uid value, const InterfaceTypeInfo* element) { return value < element->uid; });
    interfaceTypeInfos_.insert(pos, &interfaceInfo);
}

void GraphicsContext::UnregisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    if (!interfaceTypeInfos_.empty()) {
        const auto pos = std::lower_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
            [](const InterfaceTypeInfo* element, Uid value) { return element->uid < value; });
        if ((pos != interfaceTypeInfos_.cend()) && (*pos)->uid == interfaceInfo.uid) {
            interfaceTypeInfos_.erase(pos);
        }
    }
}

array_view<const InterfaceTypeInfo* const> GraphicsContext::GetInterfaceMetadata() const
{
    return interfaceTypeInfos_;
}

const InterfaceTypeInfo& GraphicsContext::GetInterfaceMetadata(const Uid& uid) const
{
    static InterfaceTypeInfo invalidType {};

    if (!interfaceTypeInfos_.empty()) {
        const auto pos = std::lower_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), uid,
            [](const InterfaceTypeInfo* element, Uid value) { return element->uid < value; });
        if ((pos != interfaceTypeInfos_.cend()) && (*pos)->uid == uid) {
            return *(*pos);
        }
    }
    return invalidType;
}

IInterface* GraphicsContext::GetInstance(const BASE_NS::Uid& uid) const
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.getInterface) {
        return data.getInterface(const_cast<GraphicsContext&>(*this), data.token);
    }
    return nullptr;
}

IInterface::Ptr GraphicsContext::CreateInstance(const BASE_NS::Uid& uid)
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.createInterface) {
        return IInterface::Ptr { data.createInterface(*this, data.token) };
    }
    return IInterface::Ptr {};
}

void GraphicsContext::OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos)
{
    if (type == EventType::ADDED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == I3DPlugin::UID && static_cast<const I3DPlugin*>(info)->createPlugin) {
                auto plugin = static_cast<const I3DPlugin*>(info);
                if (std::none_of(plugins_.begin(), plugins_.end(),
                        [plugin](const pair<PluginToken, const I3DPlugin*>& pluginData) {
                            return pluginData.second == plugin;
                        })) {
                    auto token = plugin->createPlugin(*this);
                    plugins_.push_back({ token, plugin });
                }
            }
        }
    } else if (type == EventType::REMOVED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == I3DPlugin::UID) {
                auto plugin = static_cast<const I3DPlugin*>(info);
                if (auto pos = std::find_if(plugins_.begin(), plugins_.end(),
                        [plugin](const pair<PluginToken, const I3DPlugin*>& pluginData) {
                            return pluginData.second == plugin;
                        });
                    pos != plugins_.end()) {
                    if (plugin->destroyPlugin) {
                        plugin->destroyPlugin(pos->first);
                    }
                    plugins_.erase(pos);
                }
            }
        }
    }
}

void RegisterTypes(IPluginRegister& pluginRegistry);
void UnregisterTypes(IPluginRegister& pluginRegistry);

namespace {
PluginToken CreatePlugin3D(IRenderContext& context)
{
    Agp3DPluginState* token = new Agp3DPluginState { context, {}, {}, {}, {}, {} };
    auto& registry = *context.GetInterface<IClassRegister>();
    for (const auto& info : token->interfaces) {
        registry.RegisterInterfaceType(info);
    }

    IFileManager& fileManager = context.GetEngine().GetFileManager();
#if (CORE3D_EMBEDDED_ASSETS_ENABLED == 1)
    // Create engine:// protocol that points to embedded asset files.
    fileManager.RegisterFilesystem("rofs3D", fileManager.CreateROFilesystem(BINARY_DATA_FOR_3D, SIZE_OF_DATA_FOR_3D));
#endif
    for (uint32_t idx = 0; idx < countof(RENDER_DATA_PATHS); ++idx) {
        fileManager.RegisterPath(RENDER_DATA_PATHS[idx].protocol, RENDER_DATA_PATHS[idx].uri, false);
    }
    context.GetDevice().GetShaderManager().LoadShaderFiles(SHADER_FILE_PATHS);

    CreateDefaultResources(context.GetDevice(), token->defaultGpuResources);
    {
        auto* renderDataConfigurationLoader = CORE_NS::GetInstance<IRenderDataConfigurationLoader>(
            *context.GetInterface<IClassRegister>(), UID_RENDER_DATA_CONFIGURATION_LOADER);
        auto* dataStore = context.GetRenderDataStoreManager().GetRenderDataStore(POST_PROCESS_DATA_STORE_NAME);
        if (renderDataConfigurationLoader && dataStore) {
            auto& fileMgr = context.GetEngine().GetFileManager();
            constexpr string_view ppPath = POST_PROCESS_PATH;
            if (auto dir = fileMgr.OpenDirectory(ppPath); dir) {
                for (const auto& entry : dir->GetEntries()) {
                    if (entry.type == IDirectory::Entry::Type::FILE) {
                        const auto loadedPP =
                            renderDataConfigurationLoader->LoadPostProcess(fileMgr, ppPath + entry.name);
                        if (loadedPP.loadResult.success) {
                            token->defaultPostProcesses.push_back(loadedPP.name);
                            auto pod = static_cast<IRenderDataStorePod*>(dataStore);
                            pod->CreatePod(
                                POST_PROCESS_NAME, loadedPP.name, arrayviewU8(loadedPP.postProcessConfiguration));
                        }
                    }
                }
            }
        }
    }

    return token;
}

void DestroyPlugin3D(PluginToken token)
{
    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
    IFileManager& fileManager = state->renderContext.GetEngine().GetFileManager();

    state->defaultGpuResources.clear();
    {
        auto* renderDataConfigurationLoader = CORE_NS::GetInstance<IRenderDataConfigurationLoader>(
            *state->renderContext.GetInterface<IClassRegister>(), UID_RENDER_DATA_CONFIGURATION_LOADER);
        auto* dataStore =
            state->renderContext.GetRenderDataStoreManager().GetRenderDataStore(POST_PROCESS_DATA_STORE_NAME);
        if (renderDataConfigurationLoader && dataStore) {
            auto pod = static_cast<IRenderDataStorePod*>(dataStore);
            for (const auto& ref : state->defaultPostProcesses) {
                pod->DestroyPod(POST_PROCESS_NAME, ref);
            }
        }
        state->defaultPostProcesses.clear();
    }
    state->renderContext.GetDevice().GetShaderManager().UnloadShaderFiles(SHADER_FILE_PATHS);
#if (CORE3D_EMBEDDED_ASSETS_ENABLED == 1)
    fileManager.UnregisterFilesystem("rofs3D");
#endif
    for (uint32_t idx = 0; idx < countof(RENDER_DATA_PATHS); ++idx) {
        fileManager.UnregisterPath(RENDER_DATA_PATHS[idx].protocol, RENDER_DATA_PATHS[idx].uri);
    }

    auto& registry = *state->renderContext.GetInterface<IClassRegister>();
    for (const auto& info : state->interfaces) {
        registry.UnregisterInterfaceType(info);
    }

    delete state;
}

PluginToken CreateEcsPlugin3D(IEcs& ecs)
{
    return {};
}

void DestroyEcsPlugin3D(PluginToken token) {}

constexpr IRenderPlugin RENDER_PLUGIN(CreatePlugin3D, DestroyPlugin3D);
constexpr IEcsPlugin ECS_PLUGIN(CreateEcsPlugin3D, DestroyEcsPlugin3D);
} // namespace

PluginToken RegisterInterfaces3D(IPluginRegister& pluginRegistry)
{
    InitRegistry(pluginRegistry);
    pluginRegistry.RegisterTypeInfo(RENDER_PLUGIN);
    pluginRegistry.RegisterTypeInfo(ECS_PLUGIN);

    RegisterTypes(GetPluginRegister());

    return &pluginRegistry;
}

void UnregisterInterfaces3D(PluginToken token)
{
    IPluginRegister* pluginRegistry = static_cast<IPluginRegister*>(token);

    UnregisterTypes(GetPluginRegister());

    pluginRegistry->UnregisterTypeInfo(ECS_PLUGIN);
    pluginRegistry->UnregisterTypeInfo(RENDER_PLUGIN);
}
CORE3D_END_NAMESPACE()
