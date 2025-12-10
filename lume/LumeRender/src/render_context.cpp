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

#include "render_context.h"

#include <base/containers/vector.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_class_register.h>

#if (RENDER_PERF_ENABLED == 1)
#include <core/perf/intf_performance_data_manager.h>
#endif

#include "datastore/render_data_store_default_acceleration_structure_staging.h"
#include "datastore/render_data_store_default_gpu_resource_data_copy.h"
#include "datastore/render_data_store_default_staging.h"
#include "datastore/render_data_store_manager.h"
#include "datastore/render_data_store_pod.h"
#include "datastore/render_data_store_post_process.h"
#include "datastore/render_data_store_render_post_processes.h"
#include "datastore/render_data_store_shader_passes.h"
#include "default_engine_constants.h"
#include "device/device.h"
#include "device/shader_manager.h"
#include "loader/render_data_loader.h"
#include "node/core_render_node_factory.h"
#include "nodecontext/render_node_graph_manager.h"
#include "nodecontext/render_node_graph_node_store.h"
#include "nodecontext/render_node_manager.h"
#include "nodecontext/render_node_post_process_util.h"
#include "perf/cpu_perf_scope.h"
#include "renderer.h"
#include "util/render_util.h"

#if RENDER_HAS_VULKAN_BACKEND
#include "vulkan/device_vk.h"
#endif

#if (RENDER_HAS_GL_BACKEND) || (RENDER_HAS_GLES_BACKEND)
#include "gles/device_gles.h"
#include "gles/swapchain_gles.h"
#endif

#include <algorithm>

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
struct RegisterPathStrings {
    string_view protocol;
    string_view uri;
};
constexpr RegisterPathStrings RENDER_DATA_PATHS[] = {
    { "rendershaders", "rofsRndr://shaders/" },
    { "rendershaderstates", "rofsRndr://shaderstates/" },
    { "rendervertexinputdeclarations", "rofsRndr://vertexinputdeclarations/" },
    { "renderpipelinelayouts", "rofsRndr://pipelinelayouts/" },
    { "renderrenderdataconfigurations", "rofsRndr://renderdataconfigurations/" },
    { "renderrendernodegraphs", "rofsRndr://rendernodegraphs/" },
};

#if (RENDER_EMBEDDED_ASSETS_ENABLED == 1)
// Core Rofs Data.
extern "C" const uint64_t SIZEOFDATAFORRENDER;
extern "C" const void* const BINARYDATAFORRENDER[];
#endif

void LogRenderBuildInfo()
{
#define RENDER_TO_STRING_INTERNAL(x) #x
#define RENDER_TO_STRING(x) RENDER_TO_STRING_INTERNAL(x)

    PLUGIN_LOG_I("RENDER_VALIDATION_ENABLED=" RENDER_TO_STRING(RENDER_VALIDATION_ENABLED));
    PLUGIN_LOG_I("RENDER_DEV_ENABLED=" RENDER_TO_STRING(RENDER_DEV_ENABLED));
    PLUGIN_LOG_I("RENDER_PERF_ENABLED=" RENDER_TO_STRING(RENDER_PERF_ENABLED));
}

template<class RenderDataStoreType>
RenderDataStoreTypeInfo FillRenderDataStoreTypeInfo()
{
    return {
        { RenderDataStoreTypeInfo::UID },
        RenderDataStoreType::UID,
        RenderDataStoreType::TYPE_NAME,
        RenderDataStoreType::Create,
    };
}

void RegisterCoreRenderDataStores(RenderDataStoreManager& renderDataStoreManager)
{
    renderDataStoreManager.AddRenderDataStoreFactory(FillRenderDataStoreTypeInfo<RenderDataStorePod>());
    renderDataStoreManager.AddRenderDataStoreFactory(
        FillRenderDataStoreTypeInfo<RenderDataStoreDefaultAccelerationStructureStaging>());
    renderDataStoreManager.AddRenderDataStoreFactory(FillRenderDataStoreTypeInfo<RenderDataStoreDefaultStaging>());
    renderDataStoreManager.AddRenderDataStoreFactory(
        FillRenderDataStoreTypeInfo<RenderDataStoreDefaultGpuResourceDataCopy>());
    renderDataStoreManager.AddRenderDataStoreFactory(FillRenderDataStoreTypeInfo<RenderDataStoreShaderPasses>());
    renderDataStoreManager.AddRenderDataStoreFactory(FillRenderDataStoreTypeInfo<RenderDataStorePostProcess>());
    renderDataStoreManager.AddRenderDataStoreFactory(FillRenderDataStoreTypeInfo<RenderDataStoreRenderPostProcesses>());
}

template<typename DataStoreType>
refcnt_ptr<IRenderDataStore> CreateDataStore(IRenderDataStoreManager& renderDataStoreManager, const string_view name)
{
    refcnt_ptr<IRenderDataStore> renderDataStore = renderDataStoreManager.Create(DataStoreType::UID, name.data());
    if (!renderDataStore) {
        PLUGIN_LOG_E("Render data store creation failed (%s)", name.data());
    }
    return renderDataStore;
}

vector<refcnt_ptr<IRenderDataStore>> CreateDefaultRenderDataStores(
    IRenderDataStoreManager& renderDataStoreManager, RenderDataLoader& renderDataLoader)
{
    vector<refcnt_ptr<IRenderDataStore>> dataStores;
    // add pod store
    {
        auto& renderDataStorePod = dataStores.emplace_back(
            CreateDataStore<RenderDataStorePod>(renderDataStoreManager, RenderDataStorePod::TYPE_NAME));
        if (renderDataStorePod) {
            auto* renderDataStorePodTyped = static_cast<IRenderDataStorePod*>(renderDataStorePod.get());

            NodeGraphBackBufferConfiguration backBufferConfig {};
            const auto len =
                DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER.copy(backBufferConfig.backBufferName,
                    NodeGraphBackBufferConfiguration::CORE_MAX_BACK_BUFFER_NAME_LENGTH - 1);
            backBufferConfig.backBufferName[len] = '\0';
            renderDataStorePodTyped->CreatePod(
                "NodeGraphConfiguration", "NodeGraphBackBufferConfiguration", arrayviewU8(backBufferConfig));

            // load and store configurations
            renderDataLoader.Load("render", *renderDataStorePodTyped);
        }
    }

    dataStores.push_back(CreateDataStore<RenderDataStoreDefaultAccelerationStructureStaging>(
        renderDataStoreManager, RenderDataStoreDefaultAccelerationStructureStaging::TYPE_NAME));
    dataStores.push_back(CreateDataStore<RenderDataStoreDefaultStaging>(
        renderDataStoreManager, RenderDataStoreDefaultStaging::TYPE_NAME));
    dataStores.push_back(CreateDataStore<RenderDataStoreDefaultGpuResourceDataCopy>(
        renderDataStoreManager, RenderDataStoreDefaultGpuResourceDataCopy::TYPE_NAME));
    dataStores.push_back(
        CreateDataStore<RenderDataStoreShaderPasses>(renderDataStoreManager, RenderDataStoreShaderPasses::TYPE_NAME));
    dataStores.push_back(
        CreateDataStore<RenderDataStorePostProcess>(renderDataStoreManager, RenderDataStorePostProcess::TYPE_NAME));
    return dataStores;
}

void CreateDefaultBuffers(IGpuResourceManager& gpuResourceMgr, RenderContext::DefaultGpuResources& defResources)
{
    defResources.resources.push_back(gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_BUFFER,
        GpuBufferDesc { CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                            CORE_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | CORE_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
                            CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT | CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                            CORE_BUFFER_USAGE_INDEX_BUFFER_BIT | CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                            CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), 0u, 1024u }));
    defResources.defGpuBuffer = defResources.resources.back().GetHandle();
}

void CreateDefaultTextures(IGpuResourceManager& gpuResourceMgr, RenderContext::DefaultGpuResources& defResources)
{
    // NOTE: render context color space settings do not affect these
    // Pure black and white are used

    GpuImageDesc desc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
        Format::BASE_FORMAT_R8G8B8A8_UNORM, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
        ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0, // ImageCreateFlags
        0, // EngineImageCreationFlags
        2, 2, 1, 1, 1, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

    constexpr uint32_t sizeOfUint32 = sizeof(uint32_t);
    constexpr const uint32_t rgbData[4u] = { 0x0, 0x0, 0x0, 0x0 };
    const auto rgbDataView = array_view(reinterpret_cast<const uint8_t*>(rgbData), sizeOfUint32 * countof(rgbData));
    defResources.resources.push_back(
        gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE, desc, rgbDataView));
    defResources.defGpuImage = defResources.resources.back().GetHandle();

    constexpr const uint32_t rgbDataWhite[4u] = { 0xFFFFffff, 0xFFFFffff, 0xFFFFffff, 0xFFFFffff };
    const auto rgbDataViewWhite =
        array_view(reinterpret_cast<const uint8_t*>(rgbDataWhite), sizeOfUint32 * countof(rgbDataWhite));
    defResources.resources.push_back(
        gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE_WHITE, desc, rgbDataViewWhite));
}

void CreateDefaultTargets(IGpuResourceManager& gpuResourceMgr, RenderContext::DefaultGpuResources& defResources)
{
    {
        // hard-coded default backbuffer
        // all presentations and swapchain semaphore syncs are done automatically for this client handle
        GpuImageDesc desc {
            ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R8G8B8A8_UNORM,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            0, // ImageCreateFlags
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS |
                EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, // EngineImageCreationFlags
            2,
            2,
            1,
            1,
            1,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };
        auto& gpuResourceMgrImpl = static_cast<GpuResourceManager&>(gpuResourceMgr);
        // create as a swapchain image to get correct handle flags for fast check-up for additional processing
        defResources.resources.push_back(gpuResourceMgrImpl.CreateSwapchainImage(
            {}, DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER, desc));
    }
    {
        GpuImageDesc desc {
            ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_D16_UNORM,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
            0,                                                                        // ImageCreateFlags
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, // EngineImageCreationFlags
            2,
            2,
            1,
            1,
            1,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };
        defResources.resources.push_back(
            gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER_DEPTH, desc));
    }
}

void CreateDefaultSamplers(IGpuResourceManager& gpuResourceMgr, RenderContext::DefaultGpuResources& defResources)
{
    defResources.resources.push_back(gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_SAMPLER,
        GpuSamplerDesc {
            Filter::CORE_FILTER_NEAREST,                                 // magFilter
            Filter::CORE_FILTER_NEAREST,                                 // minFilter
            Filter::CORE_FILTER_NEAREST,                                 // mipMapMode
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
        }));
    defResources.defGpuSampler = defResources.resources.back().GetHandle();

    defResources.resources.push_back(
        gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_NEAREST_REPEAT,
            GpuSamplerDesc {
                Filter::CORE_FILTER_NEAREST,                          // magFilter
                Filter::CORE_FILTER_NEAREST,                          // minFilter
                Filter::CORE_FILTER_NEAREST,                          // mipMapMode
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeU
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeV
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeW
            }));
    defResources.resources.push_back(
        gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_NEAREST_CLAMP,
            GpuSamplerDesc {
                Filter::CORE_FILTER_NEAREST,                                 // magFilter
                Filter::CORE_FILTER_NEAREST,                                 // minFilter
                Filter::CORE_FILTER_NEAREST,                                 // mipMapMode
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
            }));

    defResources.resources.push_back(
        gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_REPEAT,
            GpuSamplerDesc {
                Filter::CORE_FILTER_LINEAR,                           // magFilter
                Filter::CORE_FILTER_LINEAR,                           // minFilter
                Filter::CORE_FILTER_LINEAR,                           // mipMapMode
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeU
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeV
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeW
            }));
    defResources.resources.push_back(
        gpuResourceMgr.Create(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP,
            GpuSamplerDesc {
                Filter::CORE_FILTER_LINEAR,                                  // magFilter
                Filter::CORE_FILTER_LINEAR,                                  // minFilter
                Filter::CORE_FILTER_LINEAR,                                  // mipMapMode
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
                SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
            }));

    GpuSamplerDesc linearMipmapRepeat {
        Filter::CORE_FILTER_LINEAR,                           // magFilter
        Filter::CORE_FILTER_LINEAR,                           // minFilter
        Filter::CORE_FILTER_LINEAR,                           // mipMapMode
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeU
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeV
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeW
    };
    linearMipmapRepeat.minLod = 0.0f;
    linearMipmapRepeat.maxLod = 32.0f;
    defResources.resources.push_back(gpuResourceMgr.Create(
        DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT, linearMipmapRepeat));
    GpuSamplerDesc linearMipmapClamp {
        Filter::CORE_FILTER_LINEAR,                                  // magFilter
        Filter::CORE_FILTER_LINEAR,                                  // minFilter
        Filter::CORE_FILTER_LINEAR,                                  // mipMapMode
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
    };
    linearMipmapClamp.minLod = 0.0f;
    linearMipmapClamp.maxLod = 32.0f;
    defResources.resources.push_back(gpuResourceMgr.Create(
        DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP, linearMipmapClamp));
}

string_view GetPipelineCacheUri(DeviceBackendType backendType)
{
    switch (backendType) {
        case DeviceBackendType::VULKAN:
            return "cache://deviceVkCache.bin";
        case DeviceBackendType::OPENGLES:
            return "cache://deviceGLESCache.bin";
        case DeviceBackendType::OPENGL:
            return "cache://deviceGLCache.bin";
        default:
            break;
    }
    return "";
}
} // namespace

IRenderContext* RenderPluginState::CreateInstance(IEngine& engine)
{
    if (!context_) {
        context_ = IRenderContext::Ptr { new RenderContext(*this, engine) };
    }
    return context_.get();
}

IRenderContext* RenderPluginState::GetInstance() const
{
    return context_.get();
}

void RenderPluginState::Destroy()
{
    context_.reset();
}

RenderContext::RenderContext(RenderPluginState& pluginState, IEngine& engine)
    : pluginState_(pluginState), engine_(engine), fileManager_(&engine.GetFileManager())
{
    for (const auto& ref : interfaceInfos_) {
        RegisterInterfaceType(ref);
    }
    LogRenderBuildInfo();
    RegisterDefaultPaths();
}

RenderContext::~RenderContext()
{
    GetPluginRegister().RemoveListener(*this);
    if (device_) {
        device_->Activate();
        device_->WaitForIdle();
    }

    for (auto& info : plugins_) {
        if (info.second && info.second->destroyPlugin) {
            info.second->destroyPlugin(info.first);
        }
    }

    WritePipelineCacheInternal(false);

    defaultGpuResources_ = {};
    renderer_.reset();
    renderNodeGraphMgr_.reset();
    renderDataStoreMgr_.reset();
    renderUtil_.reset(); // GLES semaphore/fence destruction device needs to be active
    if (device_) {
        device_->Deactivate();
    }
}

RenderResultCode RenderContext::Init(const RenderCreateInfo& createInfo)
{
    PLUGIN_LOG_D("Render init.");
    RENDER_CPU_PERF_SCOPE("RenderContext::Init", "");

    createInfo_ = createInfo;
    device_ = CreateDevice(createInfo_);
    if ((!device_) || (!device_->GetDeviceStatus())) {
        PLUGIN_LOG_E("Device not created successfully, invalid render interface.");
        device_ = {};
        return RenderResultCode::RENDER_ERROR;
    } else {
        device_->Activate();

        // Initialize the pipeline/ program cache.
        if (device_->GetDeviceConfiguration().configurationFlags & CORE_DEVICE_CONFIGURATION_PIPELINE_CACHE_BIT) {
            vector<uint8_t> pipelineCache;
            if (auto file = fileManager_->OpenFile(GetPipelineCacheUri(device_->GetBackendType())); file) {
                pipelineCache.resize(static_cast<size_t>(file->GetLength()));
                file->Read(pipelineCache.data(), pipelineCache.size());
            }
            device_->InitializePipelineCache(pipelineCache);
        }

        // set engine file manager with registered paths
        auto& shaderMgr = (ShaderManager&)device_->GetShaderManager();
        shaderMgr.SetFileManager(engine_.GetFileManager());

        {
            IShaderManager::ShaderFilePathDesc desc;
            desc.shaderPath = "rendershaders://";
            desc.pipelineLayoutPath = "renderpipelinelayouts://";
            desc.vertexInputDeclarationPath = "rendervertexinputdeclarations://";
            // NOTE: does not have states and vids
            shaderMgr.LoadShaderFiles(desc);
        }
        // make sure shaders found above have been created
        shaderMgr.HandlePendingAllocations();

        renderDataStoreMgr_ = make_unique<RenderDataStoreManager>(*this);
        RegisterCoreRenderDataStores(*renderDataStoreMgr_);

        // Add render data stores from plugins
        for (auto info : CORE_NS::GetPluginRegister().GetTypeInfos(RenderDataStoreTypeInfo::UID)) {
            renderDataStoreMgr_->AddRenderDataStoreFactory(*static_cast<const RenderDataStoreTypeInfo*>(info));
        }

        auto loader = RenderDataLoader(*fileManager_);
        defaultRenderDataStores_ = CreateDefaultRenderDataStores(*renderDataStoreMgr_, loader);

        renderNodeGraphMgr_ = make_unique<RenderNodeGraphManager>(*device_, *fileManager_);

        auto& renderNodeMgr = renderNodeGraphMgr_->GetRenderNodeManager();
        RegisterCoreRenderNodes(renderNodeMgr);
        // Add render nodes from plugins
        for (auto info : CORE_NS::GetPluginRegister().GetTypeInfos(RenderNodeTypeInfo::UID)) {
            renderNodeMgr.AddRenderNodeFactory(*static_cast<const RenderNodeTypeInfo*>(info));
        }

        renderUtil_ = make_unique<RenderUtil>(*this);

        renderer_ = make_unique<Renderer>(*this);

        IGpuResourceManager& gpuResourceMgr = device_->GetGpuResourceManager();
        CreateDefaultBuffers(gpuResourceMgr, defaultGpuResources_);
        CreateDefaultTextures(gpuResourceMgr, defaultGpuResources_);
        CreateDefaultTargets(gpuResourceMgr, defaultGpuResources_);
        CreateDefaultSamplers(gpuResourceMgr, defaultGpuResources_);
        ((GpuResourceManager&)gpuResourceMgr)
            .SetDefaultResources({ defaultGpuResources_.defGpuBuffer, defaultGpuResources_.defGpuImage,
                defaultGpuResources_.defGpuSampler });

        device_->Deactivate();

        GetPluginRegister().AddListener(*this);

        for (auto info : CORE_NS::GetPluginRegister().GetTypeInfos(IRenderPlugin::UID)) {
            if (auto renderPlugin = static_cast<const IRenderPlugin*>(info);
                renderPlugin && renderPlugin->createPlugin) {
                auto token = renderPlugin->createPlugin(*this);
                plugins_.push_back({ token, renderPlugin });
            }
        }

        return RenderResultCode::RENDER_SUCCESS;
    }
}

IDevice& RenderContext::GetDevice() const
{
    if (!device_) {
        PLUGIN_LOG_E("Render Init not called or result was not success");
        std::abort();
    }
    return *device_;
}

IRenderer& RenderContext::GetRenderer() const
{
    if (!renderer_) {
        PLUGIN_LOG_E("Render Init not called or result was not success");
        std::abort();
    }
    return *renderer_;
}

IRenderDataStoreManager& RenderContext::GetRenderDataStoreManager() const
{
    if (!renderDataStoreMgr_) {
        PLUGIN_LOG_E("Render Init not called or result was not success");
        std::abort();
    }
    return *renderDataStoreMgr_;
}

IRenderNodeGraphManager& RenderContext::GetRenderNodeGraphManager() const
{
    if (!renderNodeGraphMgr_) {
        PLUGIN_LOG_E("Render Init not called or result was not success");
        std::abort();
    }
    return *renderNodeGraphMgr_;
}

IRenderUtil& RenderContext::GetRenderUtil() const
{
    if (!renderUtil_) {
        PLUGIN_LOG_E("Render Init not called or result was not success");
        std::abort();
    }
    return *renderUtil_;
}

bool RenderContext::ValidMembers() const
{
    return (device_ && renderer_ && renderDataStoreMgr_ && renderNodeGraphMgr_ && renderUtil_);
}

void RenderContext::RegisterDefaultPaths()
{
    // Already handeled during plugin registration. If own filemanager instance is used then these are needed.
#if (RENDER_EMBEDDED_ASSETS_ENABLED == 1)
    // Create render:// protocol that points to embedded asset files.
    PLUGIN_LOG_D("Registered core asset path: 'rofsRndr://render/'");
    fileManager_->RegisterPath("render", "rofsRndr://render/", false);
#endif
    for (const auto& idx : RENDER_DATA_PATHS) {
        fileManager_->RegisterPath(idx.protocol, idx.uri, false);
    }
}

unique_ptr<Device> RenderContext::CreateDevice(const RenderCreateInfo& createInfo)
{
    switch (createInfo.deviceCreateInfo.backendType) {
        case DeviceBackendType::OPENGL:
#if (RENDER_HAS_GL_BACKEND)
            return CreateDeviceGL(*this);
#else
            return nullptr;
#endif
        case DeviceBackendType::OPENGLES:
#if (RENDER_HAS_GLES_BACKEND)
            return CreateDeviceGLES(*this);
#else
            return nullptr;
#endif
        case DeviceBackendType::VULKAN:
#if (RENDER_HAS_VULKAN_BACKEND)
            return CreateDeviceVk(*this);
#else
            return nullptr;
#endif
        default:
            break;
    }
    return nullptr;
}

IEngine& RenderContext::GetEngine() const
{
    return engine_;
}

string_view RenderContext::GetVersion()
{
    return {};
}

void RenderContext::WritePipelineCache() const
{
    // NOTE: device needs to be active for render resources (e.g. with GLES)
    WritePipelineCacheInternal(true);
}

void RenderContext::WritePipelineCacheInternal(bool activate) const
{
    // NOTE: device needs to be active for render resources (e.g. with GLES)
    if (device_ &&
        (device_->GetDeviceConfiguration().configurationFlags & CORE_DEVICE_CONFIGURATION_PIPELINE_CACHE_BIT) &&
        (fileManager_->GetEntry("cache://").type == CORE_NS::IDirectory::Entry::Type::DIRECTORY)) {
        if (activate) {
            device_->Activate();
        }
        vector<uint8_t> pipelineCache = device_->GetPipelineCache();
        if (auto file = fileManager_->CreateFile(GetPipelineCacheUri(device_->GetBackendType())); file) {
            file->Write(pipelineCache.data(), pipelineCache.size());
        }
        if (activate) {
            device_->Deactivate();
        }
    }
}

RenderCreateInfo RenderContext::GetCreateInfo() const
{
    return createInfo_;
}

const IInterface* RenderContext::GetInterface(const Uid& uid) const
{
    if ((uid == IRenderContext::UID) || (uid == IClassFactory::UID) || (uid == IInterface::UID)) {
        return static_cast<const IRenderContext*>(this);
    }
    if (uid == IClassRegister::UID) {
        return static_cast<const IClassRegister*>(this);
    }
    return nullptr;
}

IInterface* RenderContext::GetInterface(const Uid& uid)
{
    if ((uid == IRenderContext::UID) || (uid == IClassFactory::UID) || (uid == IInterface::UID)) {
        return static_cast<IRenderContext*>(this);
    }
    if (uid == IClassRegister::UID) {
        return static_cast<IClassRegister*>(this);
    }
    return nullptr;
}

void RenderContext::Ref()
{
    refCount_++;
}

void RenderContext::Unref()
{
    if (--refCount_ == 1) {
        pluginState_.Destroy();
        delete this;
    }
}

IInterface::Ptr RenderContext::CreateInstance(const Uid& uid)
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.createInterface) {
        return IInterface::Ptr { data.createInterface(*this, data.token) };
    }
    return {};
}

void RenderContext::RegisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    // keep interfaceTypeInfos_ sorted according to UIDs
    const auto pos = std::upper_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
        [](Uid value, const InterfaceTypeInfo* element) { return value < element->uid; });
    interfaceTypeInfos_.insert(pos, &interfaceInfo);
}

void RenderContext::UnregisterInterfaceType(const InterfaceTypeInfo& interfaceInfo)
{
    if (!interfaceTypeInfos_.empty()) {
        const auto pos = std::lower_bound(interfaceTypeInfos_.cbegin(), interfaceTypeInfos_.cend(), interfaceInfo.uid,
            [](const InterfaceTypeInfo* element, Uid value) { return element->uid < value; });
        if ((pos != interfaceTypeInfos_.cend()) && (*pos)->uid == interfaceInfo.uid) {
            interfaceTypeInfos_.erase(pos);
        }
    }
}

array_view<const InterfaceTypeInfo* const> RenderContext::GetInterfaceMetadata() const
{
    return interfaceTypeInfos_;
}

const InterfaceTypeInfo& RenderContext::GetInterfaceMetadata(const Uid& uid) const
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

IInterface* RenderContext::GetInstance(const Uid& uid) const
{
    const auto& data = GetInterfaceMetadata(uid);
    if (data.getInterface) {
        return data.getInterface(const_cast<RenderContext&>(*this), data.token);
    }
    return nullptr;
}

void RenderContext::OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos)
{
    auto& renderNodeMgr = renderNodeGraphMgr_->GetRenderNodeManager();
    if (type == EventType::ADDED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == IRenderPlugin::UID && static_cast<const IRenderPlugin*>(info)->createPlugin) {
                auto renderPlugin = static_cast<const IRenderPlugin*>(info);
                if (std::none_of(plugins_.begin(), plugins_.end(),
                        [renderPlugin](const pair<PluginToken, const IRenderPlugin*>& pluginData) {
                            return pluginData.second == renderPlugin;
                        })) {
                    auto token = renderPlugin->createPlugin(*this);
                    plugins_.push_back({ token, renderPlugin });
                }
            } else if (info && info->typeUid == RenderDataStoreTypeInfo::UID) {
                renderDataStoreMgr_->AddRenderDataStoreFactory(*static_cast<const RenderDataStoreTypeInfo*>(info));
            } else if (info && info->typeUid == RenderNodeTypeInfo::UID) {
                renderNodeMgr.AddRenderNodeFactory(*static_cast<const RenderNodeTypeInfo*>(info));
            }
        }
    } else if (type == EventType::REMOVED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == IRenderPlugin::UID) {
                auto renderPlugin = static_cast<const IRenderPlugin*>(info);
                if (auto pos = std::find_if(plugins_.begin(), plugins_.end(),
                        [renderPlugin](const pair<PluginToken, const IRenderPlugin*>& pluginData) {
                            return pluginData.second == renderPlugin;
                        });
                    pos != plugins_.end()) {
                    if (renderPlugin->destroyPlugin) {
                        renderPlugin->destroyPlugin(pos->first);
                    }
                    plugins_.erase(pos);
                }
            } else if (info && info->typeUid == RenderDataStoreTypeInfo::UID) {
                renderDataStoreMgr_->RemoveRenderDataStoreFactory(*static_cast<const RenderDataStoreTypeInfo*>(info));
            } else if (info && info->typeUid == RenderNodeTypeInfo::UID) {
                renderNodeGraphMgr_->Destroy(static_cast<const RenderNodeTypeInfo*>(info)->typeName);
                renderNodeMgr.RemoveRenderNodeFactory(*static_cast<const RenderNodeTypeInfo*>(info));
            }
        }
    }
}

RENDER_END_NAMESPACE()
