/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

void CreateDefaultResources(IDevice& device, vector<RenderHandleReference>& defaultGpuResources)
{
    // default material gpu images
    {
        GpuImageDesc desc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R8G8B8A8_SRGB, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, 2, 2, 1, 1, 1,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

        IGpuResourceManager& gpuResourceMgr = device.GetGpuResourceManager();
        constexpr uint32_t sizeOfUint32 = sizeof(uint32_t);
        constexpr const uint32_t rgbData[4u] = { 0xFFFFffff, 0xFFFFffff, 0xFFFFffff, 0xFFFFffff };
        const auto rgbDataView = array_view(reinterpret_cast<const uint8_t*>(rgbData), sizeOfUint32 * countof(rgbData));
        defaultGpuResources.emplace_back(
            gpuResourceMgr.Create(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_BASE_COLOR,
                reinterpret_cast<GpuImageDesc&>(desc), rgbDataView));

        desc.format = Format::BASE_FORMAT_R8G8B8A8_UNORM;
        {
            constexpr const uint32_t normalData[4u] = { 0xFFFF7f7f, 0xFFFF7f7f, 0xFFFF7f7f, 0xFFFF7f7f };
            const auto normalDataView =
                array_view(reinterpret_cast<const uint8_t*>(normalData), sizeOfUint32 * countof(normalData));
            defaultGpuResources.emplace_back(gpuResourceMgr.Create(
                DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_NORMAL, desc, normalDataView));
        }
        defaultGpuResources.emplace_back(gpuResourceMgr.Create(
            DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_MATERIAL, desc, rgbDataView));

        desc.format = Format::BASE_FORMAT_R8_UNORM;
        {
            constexpr const uint8_t byteData[4u] = { 0xff, 0xff, 0xff, 0xff };
            const auto byteDataView =
                array_view(reinterpret_cast<const uint8_t*>(byteData), sizeof(uint8_t) * countof(byteData));
            defaultGpuResources.emplace_back(gpuResourceMgr.Create(
                DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_AO, desc, byteDataView));
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
            defaultGpuResources.emplace_back(gpuResourceMgr.Create(
                DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP, desc, cubeDataView));

            // Skybox is currently in rgbd format (alpha channel is used as a divider for the rgb values).
            defaultGpuResources.emplace_back(gpuResourceMgr.Create(
                DefaultMaterialGpuResourceConstants::CORE_DEFAULT_SKYBOX_CUBEMAP, desc, cubeDataView));
        }
    }
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

    Picking picker;
    RenderNodeSceneUtilImpl renderNodeSceneUtil;
    InterfaceTypeInfo interfaces[4] = {
        InterfaceTypeInfo {
            this,
            UID_MESH_BUILDER,
            CORE_NS::GetName<IMeshBuilder>().data(),
            [](IClassFactory&, PluginToken token) -> IInterface* { return new MeshBuilder; },
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
                    return &state->picker;
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
                    return state->context.get();
                }
                return nullptr;
            },
            [](IClassRegister& registry, PluginToken token) -> IInterface* {
                if (token) {
                    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
                    return state->context.get();
                }
                return nullptr;
            },
        },
    };

    void Destroy(IRenderContext& engine)
    {
        context.reset();
    }
};

GraphicsContext::GraphicsContext(struct Agp3DPluginState& factory, IRenderContext& context)
    : factory_(factory), context_(context)
{}

GraphicsContext ::~GraphicsContext() {}

void GraphicsContext::Init()
{
    if (initialized_) {
        return;
    }
    auto& engine = context_.GetEngine();

    meshUtil_ = make_unique<MeshUtil>(engine);
    gltf2_ = make_unique<Gltf2>(*this);
    CreateDefaultResources(context_.GetDevice(), defaultGpuResources_);

    auto* renderDataConfigurationLoader = GetInstance<IRenderDataConfigurationLoader>(
        *context_.GetInterface<IClassRegister>(), UID_RENDER_DATA_CONFIGURATION_LOADER);
    auto* dataStore = context_.GetRenderDataStoreManager().GetRenderDataStore("RenderDataStorePod");
    if (renderDataConfigurationLoader && dataStore) {
        auto& fileMgr = engine.GetFileManager();
        constexpr string_view ppPath = "3drenderdataconfigurations://postprocess/";
        if (auto dir = fileMgr.OpenDirectory(ppPath); dir) {
            for (const auto& entry : dir->GetEntries()) {
                if (entry.type == IDirectory::Entry::Type::FILE) {
                    const auto loadedPP =
                        renderDataConfigurationLoader->LoadPostProcess(engine.GetFileManager(), ppPath + entry.name);
                    if (loadedPP.loadResult.success) {
                        auto pod = static_cast<IRenderDataStorePod*>(dataStore);
                        pod->CreatePod("PostProcess", loadedPP.name, arrayviewU8(loadedPP.postProcessConfiguration));
                    }
                }
            }
        }
    }
    sceneUtil_ = make_unique<SceneUtil>(*this);
    renderUtil_ = make_unique<RenderUtil>(*this);

    initialized_ = true;
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
        return this;
    }
    return nullptr;
}

IInterface* GraphicsContext::GetInterface(const Uid& uid)
{
    if (uid == IGraphicsContext::UID) {
        return this;
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
        factory_.Destroy(context_);
    }
}

void RegisterTypes(IPluginRegister& pluginRegistry);
void UnregisterTypes(IPluginRegister& pluginRegistry);

namespace {
PluginToken CreatePlugin3D(IRenderContext& context)
{
    Agp3DPluginState* token = new Agp3DPluginState { context, {}, {}, {} };
    auto& registry = *context.GetInterface<IClassRegister>();
    for (const auto& info : token->interfaces) {
        registry.RegisterInterfaceType(info);
    }

    RegisterTypes(GetPluginRegister());

    IFileManager& fileManager = context.GetEngine().GetFileManager();
#if (CORE3D_EMBEDDED_ASSETS_ENABLED == 1)
    // Create engine:// protocol that points to embedded asset files.
    fileManager.RegisterFilesystem("rofs3D", fileManager.CreateROFilesystem(BINARY_DATA_FOR_3D, SIZE_OF_DATA_FOR_3D));
#endif
#if (CORE3D_EMBEDDED_ASSETS_ENABLED == 0) || (CORE3D_DEV_ENABLED == 1)
    const string assets = context.GetEngine().GetRootPath() + "../Lume3D/assets/3d/";
    fileManager.RegisterPath("engine", assets, true);
#endif
    for (uint32_t idx = 0; idx < countof(RENDER_DATA_PATHS); ++idx) {
        fileManager.RegisterPath(RENDER_DATA_PATHS[idx].protocol, RENDER_DATA_PATHS[idx].uri, false);
    }
    context.GetDevice().GetShaderManager().LoadShaderFiles(SHADER_FILE_PATHS);

    return token;
}

void DestroyPlugin3D(PluginToken token)
{
    Agp3DPluginState* state = static_cast<Agp3DPluginState*>(token);
    IFileManager& fileManager = state->renderContext.GetEngine().GetFileManager();

    state->renderContext.GetDevice().GetShaderManager().UnloadShaderFiles(SHADER_FILE_PATHS);
#if (CORE3D_EMBEDDED_ASSETS_ENABLED == 1)
    fileManager.UnregisterFilesystem("rofs3D");
#endif
#if (CORE3D_EMBEDDED_ASSETS_ENABLED == 0) || (CORE3D_DEV_ENABLED == 1)
    const string assets = state->renderContext.GetEngine().GetRootPath() + "../Lume3D/assets/3d/";
    fileManager.UnregisterPath("engine", assets);
#endif
    for (uint32_t idx = 0; idx < countof(RENDER_DATA_PATHS); ++idx) {
        fileManager.UnregisterPath(RENDER_DATA_PATHS[idx].protocol, RENDER_DATA_PATHS[idx].uri);
    }

    UnregisterTypes(GetPluginRegister());

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

    return &pluginRegistry;
}

void UnregisterInterfaces3D(PluginToken token)
{
    IPluginRegister* pluginRegistry = static_cast<IPluginRegister*>(token);
    pluginRegistry->UnregisterTypeInfo(ECS_PLUGIN);
    pluginRegistry->UnregisterTypeInfo(RENDER_PLUGIN);
}
CORE3D_END_NAMESPACE()
