
/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "scene_adapter/sr_module.h"

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

#include <parameters.h>
#include <plugin_sr/ecs/systems/intf_sr_system.h>
#include <plugin_sr/ecs/components/sr_component.h>

#define LOW_QUALITY 3.0
#define BALANCED_QUALITY 2.0
#define HIGH_QUALITY 1.5
#define ULTRA_QUALITY 1.0
static constexpr BASE_NS::Uid UID_SR_PLUGIN { "c075f146-30bb-4df2-90f5-73069de404dc" };

namespace OHOS::Render3D {
SRData SRModule::sr_;
BASE_NS::refcnt_ptr<CORE_NS::IEcs> SRModule::ecs_;
CORE_NS::EntityReference SRModule::srConfigEntity_;

namespace {
float QualityToRate(const QualityTypeSR q)
{
    float rate;
    switch (q) {
        case QualityTypeSR::LOW:
            rate = LOW_QUALITY;
            break;
        case QualityTypeSR::BALANCED:
            rate = BALANCED_QUALITY;
            break;
        case QualityTypeSR::HIGH:
            rate = HIGH_QUALITY;
            break;
        case QualityTypeSR::ULTRA:
            rate = ULTRA_QUALITY;
            break;
        default:
            rate = ULTRA_QUALITY;
            break;
    }

    return rate;
}

SR::SRComponent::FlagBits MethodToFlagBit(const MethodTypeSR m)
{
    switch (m) {
        case MethodTypeSR::BILINEAR:
            return SR::SRComponent::FlagBits::BILINEAR_BIT;
        case MethodTypeSR::LUT:
            return SR::SRComponent::FlagBits::LUT_BIT;
        case MethodTypeSR::HGSR1:
             return SR::SRComponent::FlagBits::HGSR1_BIT;
        default:
            return SR::SRComponent::FlagBits::BILINEAR_BIT;
    }
}
} // namespace

RENDER_NS::RenderHandleReference CreateRenderNodeGraph(BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_,
    const BASE_NS::string_view rngPath)
{
    RENDER_NS::IRenderNodeGraphManager& graphManager = renderContext_->GetRenderNodeGraphManager();
    const RENDER_NS::RenderHandleReference handle =
        graphManager.LoadAndCreate(
            RENDER_NS::IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, rngPath);
    return handle;
}

struct CreatedImageData {
    BASE_NS::Math::UVec2 size { 0, 0 };
    RENDER_NS::RenderHandleReference handle;
    CORE_NS::EntityReference entity;
};

SRModule::SRModule() {}

void SRModule::Init(BASE_NS::shared_ptr<SCENE_NS::IInternalScene> scene,
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext,
    BASE_NS::shared_ptr<CORE_NS::IEngine> engine,
    BASE_NS::refcnt_ptr<CORE_NS::IEcs> ecs)
{
    static constexpr BASE_NS::Uid custom_system_uid { SR::ISRSystem::UID };
    
    ecs_ = ecs;
    CORE_NS::ISystem* srSystem = ecs_->GetSystem(custom_system_uid);
    srSystem->Initialize();

    auto* srConfigMgr = static_cast<SR::ISRComponentManager*>(
        (*ecs_).GetComponentManager(SR::ISRComponentManager::UID));
    if (!srConfigMgr) return;
    auto& entityMgr = ecs_->GetEntityManager();
    srConfigEntity_ = entityMgr.CreateReferenceCounted();
    srConfigMgr->Create(srConfigEntity_);
}

void SRModule::Update(BASE_NS::shared_ptr<SCENE_NS::IScene> scene) {}

bool SRModule::EnableSR()
{
    BASE_NS::array_view<const Core::IPlugin* const> plugins = Core::GetPluginRegister().GetPlugins();
    for (auto plugin : plugins) {
        if (plugin->version.uid == UID_SR_PLUGIN) {
            sr_.enable_ = true;
            return true;
        }
    }
    return false;
}

const SRData SRModule::InitConfig(
    BASE_NS::shared_ptr<SCENE_NS::IInternalScene> scene,
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext)
{
    auto* srConfigMgr = static_cast<SR::ISRComponentManager*>(
        (*ecs_).GetComponentManager(SR::ISRComponentManager::UID));
    if (!srConfigMgr) {
        return sr_;
    }

    auto srEntity = srConfigMgr->GetEntity(1);
    auto srHandle = srConfigMgr->Write(srEntity);
    if (!srHandle) {
        return sr_;
    }

    SR::SRComponent& testComponent = *srHandle;
    const auto method = testComponent.algorithm;
    const auto quality = testComponent.sraRate;
    if (method == "sr_lut") {
        sr_.type_ = MethodTypeSR::LUT;
    } else if (method == "sr_hgsr1") {
        sr_.type_ = MethodTypeSR::HGSR1;
    } else {
        sr_.type_ = MethodTypeSR::BILINEAR;
    }

    RENDER_NS::RenderHandleReference rng;
    switch (sr_.type_) {
        case MethodTypeSR::LUT:
            rng = CreateRenderNodeGraph(renderContext, "sr_rofs://rendernodegraphs/sr_lut.rng");
            break;
        case MethodTypeSR::HGSR1:
            rng = CreateRenderNodeGraph(renderContext, "sr_rofs://rendernodegraphs/sr_hgsr1.rng");
            break;
        default:
            rng = CreateRenderNodeGraph(renderContext, "sr_rofs://rendernodegraphs/sr_bilinear.rng");
            break;
    }
    if (!rng) {
        return sr_;
    }
    
    scene->AppendCustomRenderNodeGraph(rng);
    if (quality == "low") {
        sr_.rate_ = QualityToRate(QualityTypeSR::LOW);
    } else if (quality == "balanced") {
        sr_.rate_ = QualityToRate(QualityTypeSR::BALANCED);
    } else if (quality == "high") {
        sr_.rate_ = QualityToRate(QualityTypeSR::HIGH);
    } else {
        sr_.rate_ = QualityToRate(QualityTypeSR::ULTRA);
    }

    return sr_;
}

const SRData SRModule::GetConfig()
{
    return sr_;
}

void SRModule::SetWindowSize(int width, int height)
{
    if (width < 0 || height < 0) return;
    sr_.width_ = width;
    sr_.height_ = height;
}

bool SRModule::Enable()
{
    return sr_.enable_;
}

RENDER_NS::RenderHandleReference SRModule::CreateGpuResource(
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc, float width, float height)
{
    RENDER_NS::IGpuResourceManager& gpuResourceMgr = rc->GetDevice().GetGpuResourceManager();
    RENDER_NS::GpuImageDesc desc;
    desc.width = static_cast<int>(width);
    desc.height = static_cast<int>(height);
    desc.depth = 1;
    desc.format = BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB;
    desc.memoryPropertyFlags = RENDER_NS::MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.usageFlags = RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT |
        RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
    desc.imageType = RENDER_NS::ImageType::CORE_IMAGE_TYPE_2D;
    desc.imageTiling = RENDER_NS::ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    desc.imageViewType = RENDER_NS::ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags =
        RENDER_NS::EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    auto handle = gpuResourceMgr.Create("srInput", desc);
    return handle;
}

void SRModule::AttachComponent()
{
    if (!ecs_ || !srConfigEntity_) return;
    auto* srConfigMgr = static_cast<SR::ISRComponentManager*>(
        (*ecs_).GetComponentManager(SR::ISRComponentManager::UID));
    if (!srConfigMgr) return;
    auto srHandle = srConfigMgr->Write(srConfigEntity_);
    if (!srHandle) return;

    SR::SRComponent& srComponent = *srHandle;

    srComponent.enableFlags = MethodToFlagBit(sr_.type_);

    srComponent.generalConfiguration.outputWidth = sr_.width_;
    srComponent.generalConfiguration.outputHeight = sr_.height_;
    srComponent.generalConfiguration.inputWidth = static_cast<int32_t>(sr_.width_ / sr_.rate_);
    srComponent.generalConfiguration.inputHeight = static_cast<int32_t>(sr_.height_ / sr_.rate_);
}

} // namespace OHOS::Render3D