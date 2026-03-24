/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "scene_adapter/fg_module.h"

#include <core/ecs/intf_ecs.h>
#include <3d/implementation_uids.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/render_configuration_component.h>

#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <scene/ext/intf_internal_scene.h>
#include <parameters.h>

#define LOW_QUALITY 3.0
#define BALANCED_QUALITY 2.0
#define HIGH_QUALITY 1.5
#define ULTRA_QUALITY 1.3

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace FG;

BASE_NS::refcnt_ptr<CORE_NS::IEcs> FGModule::ecs_;

namespace {
GpuImageDesc GetGpuImageDesc(
    shared_ptr<IRenderContext> rc,
    const Base::string_view& name,
    const Format format, const uint32_t width, const uint32_t height)
{
    IGpuResourceManager& gpuResourceMgr = rc->GetDevice().GetGpuResourceManager();
    CORE_NS::IImageContainer::ImageDesc imageDesc;
    imageDesc.imageFlags = 0;
    imageDesc.format = format;
    imageDesc.width = width;
    imageDesc.height = height;
    imageDesc.depth = 1;
    imageDesc.blockPixelDepth = 1;
    imageDesc.blockPixelHeight = 1;
    imageDesc.blockPixelWidth = 1;
    imageDesc.bitsPerBlock = 8; // 8
    imageDesc.mipCount = 1;
    imageDesc.layerCount = 1;
    imageDesc.imageViewType =
        IImageContainer::ImageViewType::VIEW_TYPE_2D;
    imageDesc.blockPixelDepth = 1;
    imageDesc.imageType =
        IImageContainer::ImageType::TYPE_2D;
    GpuImageDesc gpuImageDesc = gpuResourceMgr.CreateGpuImageDesc(imageDesc);
    gpuImageDesc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    gpuImageDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    gpuImageDesc.createFlags = 0;
    gpuImageDesc.engineCreationFlags =
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    gpuImageDesc.mipCount = 1;
    gpuImageDesc.layerCount = 1;
    gpuImageDesc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT;
    return gpuImageDesc;
}

RenderHandleReference CreateTexture(
    shared_ptr<IRenderContext> rc, const Base::string_view& name,
    const Format format, const uint32_t width, const uint32_t height)
{
    IGpuResourceManager& gpuResourceMgr = rc->GetDevice().GetGpuResourceManager();
    auto gpuImageDesc = GetGpuImageDesc(rc, name, format, width, height);
    ImageUsageFlags usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT |
        CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
        CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
    gpuImageDesc.usageFlags = usageFlags;
    auto imageHandle = gpuResourceMgr.Create(name, gpuImageDesc);
    return imageHandle;
}

RenderHandleReference CreateTextureDepth(
    shared_ptr<IRenderContext> rc, const Base::string_view& name,
    const Format format, const uint32_t width, const uint32_t height)
{
    IGpuResourceManager& gpuResourceMgr = rc->GetDevice().GetGpuResourceManager();
    auto gpuImageDesc = GetGpuImageDesc(rc, name, format, width, height);
    ImageUsageFlags usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
    gpuImageDesc.usageFlags = usageFlags;
    RenderHandleReference imageHandle = gpuResourceMgr.Create(name, gpuImageDesc);
    return imageHandle;
}
} // namespace

RenderHandleReference CreateRenderNodeGraphFG(shared_ptr<IRenderContext> renderContext_,
    const string_view rngPath)
{
    IRenderNodeGraphManager& graphManager = renderContext_->GetRenderNodeGraphManager();
    const RenderHandleReference handle =
        graphManager.LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, rngPath);
    return handle;
}

struct CreatedImageData {
    Math::UVec2 size { 0, 0 };
    RenderHandleReference handle;
    CORE_NS::EntityReference entity;
};

FG::FGModule::FGModule() {}

void FGModule::Init(shared_ptr<SCENE_NS::IInternalScene> scene,
    shared_ptr<IRenderContext> renderContext,
    shared_ptr<CORE_NS::IEngine> engine,
    refcnt_ptr<CORE_NS::IEcs> ecs)
{
    ecs_ = ecs;
    rngPredict_ = CreateRenderNodeGraphFG(renderContext,
        "fg_rofs://rendernodegraphs/fg_predict.rng");
    const string_view display_real = fg_.withSR_ ?
        "fg_rofs://rendernodegraphs/fg_display_real_with_sr.rng" :
        "fg_rofs://rendernodegraphs/fg_display_real.rng";
    rngDisplayReal_ = CreateRenderNodeGraphFG(renderContext, display_real);
    const string_view displayPredict = fg_.withSR_ ?
        "fg_rofs://rendernodegraphs/fg_display_predict_with_sr.rng" :
        "fg_rofs://rendernodegraphs/fg_display_predict.rng";
    rngDisplayPredict_ = CreateRenderNodeGraphFG(renderContext, displayPredict);
   
    scene->AppendFGRenderNodeGraph(rngPredict, rngDisplayReal, rngDisplayPredict);
}

void FGModule::Update(BASE_NS::shared_ptr<SCENE_NS::IInternalScene> scene,
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc)
{
    uint64_t frameIndex = 0;
    if (rc) {
        frameIndex = rc->GetDevice().GetFrameCount();
    }

    customRenderNodeGraph_.clear();

    if (rngPredict_ && rngDisplayReal_ && rngDisplayPredict_) {
        if(frameIndex <= 2) { // Frame 1, 2
            // Mixed Frame: (Render, Predict, Display Render) Frame 1, 2
            customRenderNodeGraph_.push_back(rngPredict_);
            customRenderNodeGraph_.push_back(rngDisplayReal_);
            scene->ModifyCustomRenderNodeGraph(
                Scene::IInternalScene::RenderNodeGraphModificationMode::APPEND, customRenderNodeGraph_);
        } else if (frameIndex % 2 == 0) { // Frame 4, 6...
            // Predict Frame: (Display Render, Render) Frame 4, 6...
            customRenderNodeGraph_.push_back(rngPredict_);
            customRenderNodeGraph_.push_back(rngDisplayPredict_);
            scene->ModifyCustomRenderNodeGraph(
                Scene::IInternalScene::RenderNodeGraphModificationMode::REPLACE, customRenderNodeGraph_);
        } else {
            // Real Frame: (Display Render, Render) Frame 3, 5...
            customRenderNodeGraph_.push_back(rngDisplayReal_);
            scene->ModifyCustomRenderNodeGraph(
                Scene::IInternalScene::RenderNodeGraphModificationMode::PREPEND, customRenderNodeGraph_);
        }
    }
}

const FGData FGModule::InitConfig()
{
    if (!ecs_) {
        return fg_;
    }
    auto* fgConfigMgr = static_cast<IFGComponentManager*>(ecs_->GetComponentManager(IFGComponentManager::UID));
    if (!fgConfigMgr) {
        return fg_;
    }
    auto fgHandle = fgConfigMgr->Read(0);
    if (!fgHandle) {
        return fg_;
    }

    const FGComponent& fgComponent = *fgHandle;

    uint32_t quality = fgComponent.quality;
    uint32_t algorithm = fgComponent.algorithm;

    enum FGQualityType {
        QUALITY_TYPE_FIX = 0,
        QUALITY_TYPE_NORMAL = 1,
        QUALITY_TYPE_LOW = 2,
        QUALITY_TYPE_BAD = 3,
    };

    enum FGAlgorithm {
        FG_HYDRA = 0,
        FG_GRIDWARP = 1,
    };

    if (quality == 0) {
        fg_.quality_ = FGDetailConfiguration::FGQualityType::QUALITY_TYPE_FIX;
    } else if (quality == 1) {
        fg_.quality_ = FGDetailConfiguration::FGQualityType::QUALITY_TYPE_NORMAL;
    } else if (quality == 2) { // 2
        fg_.quality_ = FGDetailConfiguration::FGQualityType::QUALITY_TYPE_LOW;
    } else if (quality == 3) { // 3
        fg_.quality_ = FGDetailConfiguration::FGQualityType::QUALITY_TYPE_BAD;
    } else {
        fg_.quality_ = FGDetailConfiguration::FGQualityType::QUALITY_TYPE_FIX;
    }
 
    if (algorithm == 0) {
        fg_.algorithm_ = FGDetailConfiguration::FGAlgorithm::FG_HYDRA;
    } else if (algorithm == 1) {
        fg_.algorithm_ = FGDetailConfiguration::FGAlgorithm::FG_GRIDWARP;
    } else if (algorithm == 2) { // 2
        fg_.algorithm_ = FGDetailConfiguration::FGAlgorithm::FG_IOI;
    } else {
        fg_.algorithm_ = FGDetailConfiguration::FGAlgorithm::FG_HYDRA;
    }

    return fg_;
}

const FGData FGModule::GetConfig()
{
    return fg_;
}

bool FGModule::IsEnable()
{
    return fg_.enable_;
}

bool FGModule::EnableFG()
{
    BASE_NS::array_view<const Core::IPlugin* const> plugins = Core::GetPluginRegister().GetPlugins();
    for (auto plugin: plugins) {
        if (plugin->version.uid == FG::UID_FG_PLUGIN) {
            fg_.enable_ = true;
            return fg_.enable_;
        }
    }
    fg_.enable_ = false;
    return fg_.enable_;
}

void FGModule::SetWindowSize(const int& width, const int& height)
{
    if (width<0 || height<0) return;
    fg_.width_ = width;
    fg_.height_ = height;
}

RenderHandleReference FGModule::CreateGpuResource(
    shared_ptr<IRenderContext> rc, float width, float height)
{
    IGpuResourceManager& gpuResourceMgr = rc->GetDevice().GetGpuResourceManager();
    GpuImageDesc desc;
    desc.width = static_cast<int>(width);
    desc.height = static_cast<int>(height);
    desc.depth = 1;
    desc.format = Format::BASE_FORMAT_R8G8B8A8_SRGB;
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
    desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags =
      EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    auto handle = gpuResourceMgr.Create("fgInput", desc);
    return handle;
}

void FGModule::CreateGpuImages(
    shared_ptr<IRenderContext> rc, float width, float height,
    const FGData& fg,
    RENDER_NS::RenderHandleReference& FGColorOutputHandle_,
    RENDER_NS::RenderHandleReference& FGPredictOutputHandle_,
    RENDER_NS::RenderHandleReference& FGDepthOutputHandle_)
{
    FGColorOutputHandle_ = CreateTexture(rc, "FG_COLOR_OUTPUT",
        BASE_FORMAT_R8G8B8A8_SRGB,
        static_cast<int>(width), static_cast<int>(height));
    FGPredictOutputHandle_ = CreateTexture(rc, "FG_PREDICT_OUTPUT",
        BASE_FORMAT_R8G8B8A8_SRGB,
        static_cast<int>(width), static_cast<int>(height));
    FGDepthOutputHandle_ = CreateTextureDepth(rc, "SCENE_DEPTH_EMPTY",
        fg.algorithm_ == 1? BASE_FORMAT_D32_SFLOAT_S8_UINT : BASE_FORMAT_D32_SFLOAT,
        static_cast<int>(width), static_cast<int>(height));
}

void FGModule::AttachComponent()
{
    if (!ecs_) {
        return;
    }
    auto* fgConfigMgr = static_cast<IFGComponentManager*>(
        (*ecs_).GetComponentManager(IFGComponentManager::UID));
    if (!fgConfigMgr) {
        return;
    }
    auto fgEntity = fgConfigMgr->GetEntity(0);
    auto fgHandle = fgConfigMgr->Write(fgEntity);
    if (!fgHandle) {
        return;
    }
    fgHandle->fgDetailConfiguration.qualityType = fg_.quality_;
    fgHandle->fgDetailConfiguration.algorithm = fg_.algorithm_;
    fgHandle->fgDetailConfiguration.FGWidth = fg_.width_;
    fgHandle->fgDetailConfiguration.FGHeight = fg_.height_;
    fgHandle->fgDetailConfiguration.withSR = fg_.withSR_;
}