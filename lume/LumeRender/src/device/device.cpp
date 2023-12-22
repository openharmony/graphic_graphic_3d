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

#include "device.h"

#include <algorithm>
#include <cstdint>

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_manager.h"
#include "device/shader_manager.h"
#include "device/swapchain.h"
#include "render_context.h"
#include "util/log.h"
#include "util/string_util.h"

#if (RENDER_HAS_VULKAN_BACKEND)
#include "vulkan/device_vk.h"
#endif

#if (RENDER_HAS_GL_BACKEND) || (RENDER_HAS_GLES_BACKEND)
#include "gles/device_gles.h"
#include "gles/swapchain_gles.h"
#endif

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
static constexpr uint32_t MIN_BUFFERING_COUNT { 2 };
static constexpr uint32_t MAX_BUFFERING_COUNT { 4 };

void CreateDepthBuffer(const DeviceBackendType backendType, const Swapchain& swapchain,
    GpuResourceManager& gpuResourceManager, vector<RenderHandleReference>& defaultGpuResources)
{
// NOTE: special handling of CORE_DEFAULT_BACKBUFFER_DEPTH for different backend types.
#if (RENDER_HAS_VULKAN_BACKEND)
    if (backendType == DeviceBackendType::VULKAN) {
        gpuResourceManager.Create(
            DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER_DEPTH, swapchain.GetDescDepthBuffer());
    }
#endif
#if (RENDER_HAS_GL_BACKEND) || (RENDER_HAS_GLES_BACKEND)
    if ((backendType == DeviceBackendType::OPENGL) || (backendType == DeviceBackendType::OPENGLES)) {
        auto& platformData = static_cast<const SwapchainGLES&>(swapchain).GetPlatformData();
        defaultGpuResources.emplace_back(
            gpuResourceManager.CreateView(DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER_DEPTH,
                swapchain.GetDescDepthBuffer(), platformData.depthPlatformData));
    }
#endif
}
} // namespace

Device::Device(RenderContext& renderContext, const DeviceCreateInfo& deviceCreateInfo) : renderContext_(renderContext)
{
    if ((deviceConfiguration_.bufferingCount < MIN_BUFFERING_COUNT) ||
        (deviceConfiguration_.bufferingCount > MAX_BUFFERING_COUNT)) {
        deviceConfiguration_.bufferingCount =
            std::clamp(deviceConfiguration_.bufferingCount, MIN_BUFFERING_COUNT, MAX_BUFFERING_COUNT);
        PLUGIN_LOG_D("buffering count clamped to: %u", deviceConfiguration_.bufferingCount);
    }
}

void Device::CreateSwapchain(const SwapchainCreateInfo& swapchainCreateInfo)
{
    Activate();
    CreateDeviceSwapchain(swapchainCreateInfo);
    if (!swapchain_) {
        Deactivate();
        return;
    }

    {
        vector<unique_ptr<GpuImage>> swapchainGpuImages = CreateGpuImageViews(*swapchain_);
        Deactivate();
        for (uint32_t idx = 0; idx < swapchainGpuImages.size(); ++idx) {
            const auto& ref = swapchainGpuImages[idx];
            defaultGpuResources_.emplace_back(
                gpuResourceMgr_->CreateView(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SWAPCHAIN + to_string(idx),
                    ref->GetDesc(), ref->GetBasePlatformData()));
        }
        Activate();
    }
    {
        Deactivate();
        const RenderHandle firstSwapchain = gpuResourceMgr_->GetImageRawHandle(
            DefaultEngineGpuResourceConstants::CORE_DEFAULT_SWAPCHAIN + string_view("0"));
        const RenderHandle shallowHandle =
            gpuResourceMgr_->GetImageRawHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER);
        gpuResourceMgr_->RemapGpuImageHandle(shallowHandle, firstSwapchain);
        SetBackbufferHandle(shallowHandle); // store backbuffer client handle
    }

    // configure automatically backbuffer as swapchain
    {
        IRenderDataStoreManager& rdsm = renderContext_.GetRenderDataStoreManager();
        auto dataStorePod = static_cast<IRenderDataStorePod*>(rdsm.GetRenderDataStore("RenderDataStorePod"));
        if (dataStorePod) {
            auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
            PLUGIN_ASSERT(dataView.size_bytes() == sizeof(NodeGraphBackBufferConfiguration));
            NodeGraphBackBufferConfiguration ngbbc = *(const NodeGraphBackBufferConfiguration*)dataView.data();
            StringUtil::CopyStringToArray(DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER,
                ngbbc.backBufferName, NodeGraphBackBufferConfiguration::CORE_MAX_BACK_BUFFER_NAME_LENGTH);
            ngbbc.backBufferType = NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN;
            ngbbc.present = true;
            dataStorePod->Set("NodeGraphBackBufferConfiguration", arrayviewU8(ngbbc));
        }
    }

    if (swapchainCreateInfo.swapchainFlags & SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT) {
        CreateDepthBuffer(GetBackendType(), *swapchain_, *gpuResourceMgr_, defaultGpuResources_);
    }
}

void Device::DestroySwapchain()
{
    // NOTE: Clear image resource handles.
    Activate();
    DestroyDeviceSwapchain();
    SetBackbufferHandle({});
    // remove swapchain configuration from the backbuffer
    {
        IRenderDataStoreManager& rdsm = renderContext_.GetRenderDataStoreManager();
        auto dataStorePod = static_cast<IRenderDataStorePod*>(rdsm.GetRenderDataStore("RenderDataStorePod"));
        if (dataStorePod) {
            auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
            PLUGIN_ASSERT(dataView.size_bytes() == sizeof(NodeGraphBackBufferConfiguration));
            NodeGraphBackBufferConfiguration ngbbc = *(const NodeGraphBackBufferConfiguration*)dataView.data();
            if (ngbbc.backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN) {
                ngbbc.backBufferType = NodeGraphBackBufferConfiguration::BackBufferType::UNDEFINED;
                ngbbc.present = false;
            }
            dataStorePod->Set("NodeGraphBackBufferConfiguration", arrayviewU8(ngbbc));
        }
    }
    Deactivate();
}

void Device::FrameStart()
{
    ++frameCount_;
}

void Device::FrameEnd() {}

void Device::SetDeviceStatus(const bool status)
{
    deviceStatus_.store(status);
}

bool Device::GetDeviceStatus() const
{
    return deviceStatus_.load();
}

uint64_t Device::GetFrameCount() const
{
    return frameCount_;
}

MemoryPropertyFlags Device::GetSharedMemoryPropertyFlags() const
{
    return deviceSharedMemoryPropertyFlags_;
}

DeviceConfiguration Device::GetDeviceConfiguration() const
{
    return deviceConfiguration_;
}

uint32_t Device::GetCommandBufferingCount() const
{
    return deviceConfiguration_.bufferingCount;
}

bool Device::HasSwapchain() const
{
    return (swapchain_) ? true : false;
}

const Swapchain* Device::GetSwapchain() const
{
    return swapchain_.get();
}

void Device::SetBackbufferHandle(const RenderHandle handle)
{
    backbufferHandle_ = handle;
}

RenderHandle Device::GetBackbufferHandle() const
{
    return backbufferHandle_;
}

void Device::SetLockResourceBackendAccess(const bool value)
{
    isBackendResourceAccessLocked_ = value;
}

bool Device::GetLockResourceBackendAccess() const
{
    return isBackendResourceAccessLocked_;
}

void Device::SetRenderBackendRunning(const bool value)
{
    isRenderbackendRunning_ = value;
}

IGpuResourceManager& Device::GetGpuResourceManager() const
{
    return *gpuResourceMgr_;
}

IShaderManager& Device::GetShaderManager() const
{
    return *shaderMgr_;
}
RENDER_END_NAMESPACE()
