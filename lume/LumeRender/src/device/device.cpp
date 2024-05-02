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

#include "device.h"

#include <algorithm>
#include <cinttypes>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/util/formats.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_device.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "datastore/render_data_store_pod.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_manager.h"
#include "device/shader_manager.h"
#include "device/swapchain.h"
#include "render_context.h"
#include "util/log.h"
#include "util/string_util.h"

#if (RENDER_HAS_VULKAN_BACKEND)
#include "vulkan/device_vk.h"
#include "vulkan/swapchain_vk.h"
#endif

#if (RENDER_HAS_GL_BACKEND) || (RENDER_HAS_GLES_BACKEND)
#include "gles/device_gles.h"
#include "gles/swapchain_gles.h"
#endif

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
class IGpuResourceManager;
class IShaderManager;
PLUGIN_STATIC_ASSERT(BASE_NS::Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK == 184u);
PLUGIN_STATIC_ASSERT(BASE_NS::Format::BASE_FORMAT_G8B8G8R8_422_UNORM == 1000156000u);

namespace {
static constexpr uint32_t MIN_BUFFERING_COUNT { 2 };
static constexpr uint32_t MAX_BUFFERING_COUNT { 4 };

constexpr const Format FALLBACK_FORMATS[] = {
    BASE_FORMAT_UNDEFINED,

    // R4G4 UNORM PACK8
    BASE_FORMAT_R8_UNORM,

    // R4G4B4A4 UNORM PACK16
    BASE_FORMAT_R16_UNORM,
    // B4G4R4A4 UNORM PACK16
    BASE_FORMAT_R4G4B4A4_UNORM_PACK16,

    // R5G6B5 UNORM PACK16
    BASE_FORMAT_R16_UNORM,
    // B5G6R5 UNORM PACK16
    BASE_FORMAT_R5G6B5_UNORM_PACK16,

    // R5G5B5A1 UNORM PACK16
    BASE_FORMAT_R5G6B5_UNORM_PACK16,
    // B5G5R5A1 UNORM PACK16
    BASE_FORMAT_R5G5B5A1_UNORM_PACK16,
    // A1R5G5B5 UNORM PACK16
    BASE_FORMAT_R5G5B5A1_UNORM_PACK16,

    // R8 UNORM
    BASE_FORMAT_UNDEFINED, // undefined
    // R8 SNORM
    BASE_FORMAT_R8_UNORM,
    // R8 USCALED
    BASE_FORMAT_R8_UNORM,
    // R8 SSCALED
    BASE_FORMAT_R8_SNORM,
    // R8 UINT
    BASE_FORMAT_R8_UNORM,
    // R8 SINT
    BASE_FORMAT_R8_UINT,
    // R8 SRGB
    BASE_FORMAT_R8_UNORM,

    // R8G8 UNORM
    BASE_FORMAT_R8G8B8A8_UNORM, // fallback to 32 bits
    // R8G8 SNORM
    BASE_FORMAT_R8G8_UNORM,
    // R8G8 USCALED
    BASE_FORMAT_R8G8_UNORM,
    // R8G8 SSCALED
    BASE_FORMAT_R8G8_SNORM,
    // R8G8 UINT
    BASE_FORMAT_R8G8_UNORM,
    // R8G8 SINT
    BASE_FORMAT_R8G8_UINT,
    // R8G8 SRGB
    BASE_FORMAT_R8G8_UNORM,

    // R8G8B8 UNORM
    BASE_FORMAT_R8G8B8A8_UNORM, // fallback to 32 bits
    // R8G8B8 SNORM
    BASE_FORMAT_R8G8B8_UNORM,
    // R8G8B8 USCALED
    BASE_FORMAT_R8G8B8_UNORM,
    // R8G8B8 SSCALED
    BASE_FORMAT_R8G8B8_SNORM,
    // R8G8B8 UINT
    BASE_FORMAT_R8G8B8_UNORM,
    // R8G8B8 SINT
    BASE_FORMAT_R8G8B8_UINT,
    // R8G8B8 SRGB
    BASE_FORMAT_R8G8B8_UNORM,

    // B8G8R8 UNORM
    BASE_FORMAT_B8G8R8A8_UNORM, // fallback to 32 bits
    // B8G8R8 SNORM
    BASE_FORMAT_B8G8R8_UNORM,
    // B8G8R8 USCALED
    BASE_FORMAT_B8G8R8_UNORM,
    // B8G8R8 SSCALED
    BASE_FORMAT_B8G8R8_SNORM,
    // B8G8R8 UINT
    BASE_FORMAT_B8G8R8_UNORM,
    // B8G8R8 SINT
    BASE_FORMAT_B8G8R8_UINT,
    // B8G8R8 SRGB
    BASE_FORMAT_B8G8R8_UNORM,

    // R8G8B8A8 UNORM
    BASE_FORMAT_UNDEFINED, // undefined
    // R8G8B8A8 SNORM
    BASE_FORMAT_R8G8B8A8_UNORM,
    // R8G8B8A8 USCALED
    BASE_FORMAT_R8G8B8A8_UNORM,
    // R8G8B8A8 SSCALED
    BASE_FORMAT_R8G8B8A8_SNORM,
    // R8G8B8A8 UINT
    BASE_FORMAT_R8G8B8A8_UNORM,
    // R8G8B8A8 SINT
    BASE_FORMAT_R8G8B8A8_UINT,
    // R8G8B8A8 SRGB
    BASE_FORMAT_R8G8B8A8_UNORM,

    // B8G8R8A8 UNORM
    BASE_FORMAT_R8G8B8A8_UNORM,
    // B8G8R8A8 SNORM
    BASE_FORMAT_B8G8R8A8_UNORM,
    // B8G8R8A8 USCALED
    BASE_FORMAT_B8G8R8A8_UNORM,
    // B8G8R8A8 SSCALED
    BASE_FORMAT_B8G8R8A8_SNORM,
    // B8G8R8A8 UINT
    BASE_FORMAT_B8G8R8A8_UNORM,
    // B8G8R8A8 SINT
    BASE_FORMAT_B8G8R8A8_UINT,
    // FORMAT B8G8R8A8 SRGB
    BASE_FORMAT_B8G8R8A8_SRGB,

    // A8B8G8R8 UNORM PACK32
    BASE_FORMAT_R8G8B8A8_UNORM,
    // A8B8G8R8 SNORM PACK32
    BASE_FORMAT_A8B8G8R8_UNORM_PACK32,
    // A8B8G8R8 USCALED PACK32
    BASE_FORMAT_A8B8G8R8_UNORM_PACK32,
    // A8B8G8R8 SSCALED PACK32
    BASE_FORMAT_A8B8G8R8_SNORM_PACK32,
    // A8B8G8R8 UINT PACK32
    BASE_FORMAT_A8B8G8R8_UNORM_PACK32,
    // A8B8G8R8 SINT PACK32
    BASE_FORMAT_A8B8G8R8_UINT_PACK32,
    // A8B8G8R8 SRGB PACK32
    BASE_FORMAT_A8B8G8R8_UNORM_PACK32,

    // A2R10G10B10 UNORM PACK32
    BASE_FORMAT_R8G8B8A8_UNORM,
    // A2R10G10B10 SNORM PACK32
    BASE_FORMAT_A2R10G10B10_UNORM_PACK32,
    // A2R10G10B10 USCALED PACK32
    BASE_FORMAT_A2R10G10B10_UNORM_PACK32,
    // A2R10G10B10 SSCALED PACK32
    BASE_FORMAT_A2R10G10B10_SNORM_PACK32,
    // A2R10G10B10 UINT PACK32
    BASE_FORMAT_A2R10G10B10_UNORM_PACK32,
    // A2R10G10B10 SINT PACK32
    BASE_FORMAT_A2R10G10B10_UINT_PACK32,

    // A2B10G10R10 UNORM PACK32
    BASE_FORMAT_R8G8B8A8_UNORM,
    // A2B10G10R10 SNORM PACK32
    BASE_FORMAT_A2B10G10R10_UNORM_PACK32,
    // A2B10G10R10 USCALED PACK32
    BASE_FORMAT_A2B10G10R10_UNORM_PACK32,
    // A2B10G10R10 SSCALED PACK32
    BASE_FORMAT_A2B10G10R10_SNORM_PACK32,
    // A2B10G10R10 UINT PACK32
    BASE_FORMAT_A2B10G10R10_UNORM_PACK32,
    // A2B10G10R10 SINT PACK32
    BASE_FORMAT_A2B10G10R10_UINT_PACK32,

    // R16 UNORM
    BASE_FORMAT_R8_UNORM, // fallback to 8 bit channel
    // R16 SNORM
    BASE_FORMAT_R16_UNORM,
    // R16 USCALED
    BASE_FORMAT_R16_UNORM,
    // R16 SSCALED
    BASE_FORMAT_R16_SNORM,
    // R16 UINT
    BASE_FORMAT_R16_UNORM,
    // R16 SINT
    BASE_FORMAT_R16_UINT,
    // R16 SFLOAT
    BASE_FORMAT_R16_UNORM,

    // R16G16 UNORM
    BASE_FORMAT_R16G16B16A16_UNORM, // fallback to 4 channel
    // R16G16 SNORM
    BASE_FORMAT_R16G16_UNORM,
    // R16G16 USCALED
    BASE_FORMAT_R16G16_UNORM,
    // R16G16 SSCALED
    BASE_FORMAT_R16G16_SNORM,
    // R16G16 UINT
    BASE_FORMAT_R16G16_UNORM,
    // R16G16 SINT
    BASE_FORMAT_R16G16_UINT,
    // R16G16 SFLOAT
    BASE_FORMAT_R16G16_UNORM,

    // R16G16B16 UNORM
    BASE_FORMAT_R16G16B16A16_UNORM, // fallback to 4 channel
    // R16G16B16 SNORM
    BASE_FORMAT_R16G16B16_UNORM,
    // R16G16B16 USCALED
    BASE_FORMAT_R16G16B16_UNORM,
    // R16G16B16 SSCALED
    BASE_FORMAT_R16G16B16_SNORM,
    // R16G16B16 UINT
    BASE_FORMAT_R16G16B16_UNORM,
    // R16G16B16 SINT
    BASE_FORMAT_R16G16B16_UINT,
    // R16G16B16 SFLOAT
    BASE_FORMAT_R16G16B16_UNORM,

    // R16G16B16A16 UNORM
    BASE_FORMAT_R8G8B8A8_UNORM, // fallback to 8 bit channels
    // R16G16B16A16 SNORM
    BASE_FORMAT_R16G16B16A16_UNORM,
    // R16G16B16A16 USCALED
    BASE_FORMAT_R16G16B16A16_UNORM,
    // R16G16B16A16 SSCALED
    BASE_FORMAT_R16G16B16A16_SNORM,
    // R16G16B16A16 UINT
    BASE_FORMAT_R16G16B16A16_UNORM,
    // R16G16B16A16 SINT
    BASE_FORMAT_R16G16B16A16_UINT,
    // R16G16B16A16 SFLOAT
    BASE_FORMAT_R16G16B16A16_UNORM,

    // R32 UINT
    BASE_FORMAT_R16_UINT, // fallback to 16 bit channel
    // R32 SINT
    BASE_FORMAT_R32_UINT,
    // R32 SFLOAT
    BASE_FORMAT_R32_UINT,

    // R32G32 UINT
    BASE_FORMAT_R16G16_UINT, // fallback to 16 bit channels
    // R32G32 SINT
    BASE_FORMAT_R32G32_UINT,
    // R32G32 SFLOAT
    BASE_FORMAT_R32G32_UINT,

    // R32G32B32 UINT
    BASE_FORMAT_R32G32B32A32_UINT, // fallback to 4 channels
    // R32G32B32 SINT
    BASE_FORMAT_R32G32B32_UINT,
    // R32G32B32 SFLOAT
    BASE_FORMAT_R32G32B32_UINT,

    // R32G32B32A32 UINT
    BASE_FORMAT_R16G16B16A16_UINT, // fallback to 16 bit channels
    // R32G32B32A32 SINT
    BASE_FORMAT_R32G32B32A32_UINT,
    // R32G32B32A32 SFLOAT
    BASE_FORMAT_R32G32B32A32_UINT,

    // R64 UINT
    BASE_FORMAT_R32_UINT, // fallback to 32 bit channel
    // R64 SINT
    BASE_FORMAT_R64_UINT,
    // R64 SFLOAT
    BASE_FORMAT_R64_UINT,

    // R64G64 UINT
    BASE_FORMAT_R64G64B64A64_UINT, // fallback to 4 channels
    // R64G64 SINT
    BASE_FORMAT_R64G64_UINT,
    // R64G64 SFLOAT
    BASE_FORMAT_R64G64_UINT,

    // R64G64B64 UINT
    BASE_FORMAT_R64G64B64A64_UINT, // fallback to 4 channels
    // R64G64B64 SINT
    BASE_FORMAT_R64G64B64_UINT,
    // R64G64B64 SFLOAT
    BASE_FORMAT_R64G64B64_UINT,

    // R64G64B64A64 UINT
    BASE_FORMAT_R32G32B32A32_UINT, // fallback to 32 bit channels
    // R64G64B64A64 SINT
    BASE_FORMAT_R64G64B64A64_UINT,
    // R32G32B32A32 SFLOAT
    BASE_FORMAT_R64G64B64A64_UINT,

    // B10G11R11 UFLOAT PACK32
    BASE_FORMAT_R16G16B16A16_SFLOAT, // fallback to 4 channel, 16 bit HDR

    // E5B9G9R9 UFLOAT PACK32
    BASE_FORMAT_R16G16B16A16_SFLOAT, // fallback to 4 channel, 16 bit HDR

    // D16 UNORM
    BASE_FORMAT_D32_SFLOAT, // fallback to 32 bit depth only
    // X8 D24 UNORM PACK32
    BASE_FORMAT_D32_SFLOAT,
    // D32 SFLOAT
    BASE_FORMAT_X8_D24_UNORM_PACK32,
    // S8 UINT
    BASE_FORMAT_D24_UNORM_S8_UINT,
    // D16 UNORM S8 UINT
    BASE_FORMAT_D24_UNORM_S8_UINT,
    // D24 UNORM S8 UINT
    BASE_FORMAT_D32_SFLOAT_S8_UINT,
    // D32 SFLOAT S8 UINT
    BASE_FORMAT_D24_UNORM_S8_UINT,

    // BC1 RGB UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC1 RGB SRGB BLOCK
    BASE_FORMAT_BC1_RGB_UNORM_BLOCK,
    // BC1 RGBA UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC1 RGBA SRGB BLOCK
    BASE_FORMAT_BC1_RGBA_UNORM_BLOCK,
    // BC2 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC2 SRGB BLOCK
    BASE_FORMAT_BC2_UNORM_BLOCK,
    // BC3 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC3 SRGB BLOCK
    BASE_FORMAT_BC3_UNORM_BLOCK,
    // BC4 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC4 SNORM BLOCK
    BASE_FORMAT_BC4_UNORM_BLOCK,
    // BC5 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC5 SNORM BLOCK
    BASE_FORMAT_BC5_UNORM_BLOCK,
    // BC6H UFLOAT BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC6H SFLOAT BLOCK
    BASE_FORMAT_BC6H_UFLOAT_BLOCK,
    // BC7 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // BC7 SRGB BLOCK
    BASE_FORMAT_BC7_UNORM_BLOCK,

    // ETC2 R8G8B8 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ETC2 R8G8B8 SRGB BLOCK
    BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
    // ETC2 R8G8B8A1 UNORM BLOCK
    BASE_FORMAT_UNDEFINED,
    // ETC2 R8G8B8A1 SRGB BLOCK
    BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
    // ETC2 R8G8B8A8 UNORM BLOCK
    BASE_FORMAT_UNDEFINED,
    // ETC2 R8G8B8A8 SRGB BLOCK
    BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,

    // EAC R11 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // EAC R11 SNORM BLOCK
    BASE_FORMAT_EAC_R11_UNORM_BLOCK,
    // EAC R11G11 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // EAC R11G11 SNORM BLOCK
    BASE_FORMAT_EAC_R11G11_UNORM_BLOCK,

    // ASTC 4x4 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 4x4 SRGB BLOCK
    BASE_FORMAT_ASTC_4x4_UNORM_BLOCK,
    // ASTC 5x4 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 5x4 SRGB BLOCK
    BASE_FORMAT_ASTC_5x4_UNORM_BLOCK,
    // ASTC 5x5 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 5x5 SRGB BLOCK
    BASE_FORMAT_ASTC_5x5_UNORM_BLOCK,
    // ASTC 6x5 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 6x5 SRGB BLOCK
    BASE_FORMAT_ASTC_6x5_UNORM_BLOCK,
    // ASTC 6x6 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 6x6 SRGB BLOCK
    BASE_FORMAT_ASTC_6x6_UNORM_BLOCK,
    // ASTC 8x5 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 8x5 SRGB BLOCK
    BASE_FORMAT_ASTC_8x5_UNORM_BLOCK,
    // ASTC 8x6 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 8x6 SRGB BLOCK
    BASE_FORMAT_ASTC_8x6_UNORM_BLOCK,
    // ASTC 8x8 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 8x8 SRGB BLOCK
    BASE_FORMAT_ASTC_8x8_UNORM_BLOCK,
    // ASTC 10x5 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 10x5 SRGB BLOCK
    BASE_FORMAT_ASTC_10x5_UNORM_BLOCK,
    // ASTC 10x6 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 10x6 SRGB BLOCK
    BASE_FORMAT_ASTC_10x6_UNORM_BLOCK,
    // ASTC 10x8 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 10x8 SRGB BLOCK
    BASE_FORMAT_ASTC_10x8_UNORM_BLOCK,
    // ASTC 10x10 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 10x10 SRGB BLOCK
    BASE_FORMAT_ASTC_10x10_UNORM_BLOCK,
    // ASTC 12x10 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 12x10 SRGB BLOCK
    BASE_FORMAT_ASTC_12x10_UNORM_BLOCK,
    // ASTC 12x12 UNORM BLOCK
    BASE_FORMAT_UNDEFINED, // undefined
    // ASTC 12x12 SRGB BLOCK
    BASE_FORMAT_ASTC_12x12_UNORM_BLOCK,
};

constexpr const auto LINEAR_FORMAT_COUNT = BASE_NS::Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK + 1u;
PLUGIN_STATIC_ASSERT(BASE_NS::countof(FALLBACK_FORMATS) == LINEAR_FORMAT_COUNT);

void CreateDepthBuffer(const DeviceBackendType backendType, const Swapchain& swapchain,
    GpuResourceManager& gpuResourceManager, Device::InternalSwapchainData& swapchainData)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_I("RENDER_VALIDATION: Default swapchain depth buffer created.");
#endif
    swapchainData.additionalDepthBufferHandle = gpuResourceManager.Create(
        DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER_DEPTH, swapchain.GetDescDepthBuffer());
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

RenderHandleReference Device::CreateSwapchainImpl(
    const SwapchainCreateInfo& swapchainCreateInfo, const RenderHandleReference& replacedHandle, const string_view name)
{
    Activate();
    // NOTE: with optimal implementation shouldn't be needed here
    WaitForIdle();

    uint32_t swapchainIdx = static_cast<uint32_t>(swapchains_.size());
    bool replace = false;
    // check if the handle needs to be replaced
    RenderHandleReference finalReplaceHandle = replacedHandle;
    {
        const RenderHandle rawReplaceHandle = finalReplaceHandle.GetHandle();
        const bool checkReplaceHandle = RenderHandleUtil::IsValid(rawReplaceHandle);
        const bool checkReplaceWindow = (swapchainCreateInfo.window.window != 0);
        const bool checkReplaceSurface = (swapchainCreateInfo.surfaceHandle != 0);
        for (uint32_t idx = 0; idx < swapchains_.size(); ++idx) {
            const auto& cmpSwap = swapchains_[idx];
            // if window is the same, or surface is the same, or handle is the same
            // -> re-use the old handle
            replace = replace || (checkReplaceWindow && (cmpSwap.window == swapchainCreateInfo.window.window));
            replace = replace || (checkReplaceSurface && (cmpSwap.surfaceHandle == swapchainCreateInfo.surfaceHandle));
            replace = replace ||
                      (checkReplaceHandle && ((cmpSwap.remappableSwapchainImage.GetHandle() == rawReplaceHandle) ||
                                                 (cmpSwap.remappableAdditionalSwapchainImage == rawReplaceHandle)));
            if (replace) {
#if (RENDER_VALIDATION_ENABLED == 1)
                {
                    RenderHandle printHandle = cmpSwap.remappableSwapchainImage.GetHandle();
                    if (!RenderHandleUtil::IsValid(printHandle)) {
                        printHandle = cmpSwap.remappableAdditionalSwapchainImage;
                    }
                    PLUGIN_LOG_I("RENDER_VALIDATION: Replacing old swapchain handle %" PRIx64, printHandle.id);
                }
#endif
                swapchainIdx = idx;
                finalReplaceHandle = swapchains_[swapchainIdx].remappableSwapchainImage;
                swapchains_[swapchainIdx] = {};
                DestroyDeviceSwapchain(); // NOTE: GLES handling
                break;
            }
        }
    }

    if ((!replace) && (swapchains_.size() == DeviceConstants::MAX_SWAPCHAIN_COUNT)) {
        PLUGIN_LOG_W("Only (%u) swapchains supported.", DeviceConstants::MAX_SWAPCHAIN_COUNT);
        Deactivate();
        return {};
    }

    if (!replace) {
        swapchains_.push_back({});
    }
    auto& swapchainData = swapchains_[swapchainIdx];
    swapchainData = {};
    swapchainData.swapchain = CreateDeviceSwapchain(swapchainCreateInfo);
    if (!swapchainData.swapchain) {
        Deactivate();
        return {};
    }

    {
        vector<unique_ptr<GpuImage>> swapchainGpuImages = CreateGpuImageViews(*swapchainData.swapchain);
        Deactivate();

        // create shallow handle with handle in the name
        GpuImageDesc shallowDesc = swapchainData.swapchain->GetDesc();
        shallowDesc.width = 2u;
        shallowDesc.height = 2u;
        swapchainData.surfaceHandle = swapchainCreateInfo.surfaceHandle;
        swapchainData.window = swapchainCreateInfo.window.window;
        swapchainData.globalName = name;
        swapchainData.name = DefaultEngineGpuResourceConstants::CORE_DEFAULT_SWAPCHAIN +
                             to_hex(swapchainData.swapchain->GetSurfaceHandle()) + '_';
        swapchainData.remappableSwapchainImage =
            gpuResourceMgr_->CreateSwapchainImage(finalReplaceHandle, name, shallowDesc);
        PLUGIN_ASSERT(SwapchainData::MAX_IMAGE_VIEW_COUNT >= static_cast<uint32_t>(swapchainGpuImages.size()));
        swapchainData.imageViewCount = static_cast<uint32_t>(swapchainGpuImages.size());
        for (uint32_t idx = 0; idx < swapchainGpuImages.size(); ++idx) {
            const auto& ref = swapchainGpuImages[idx];
            swapchainData.imageViews[idx] = gpuResourceMgr_->CreateView(
                swapchainData.name + to_string(idx), ref->GetDesc(), ref->GetBasePlatformData());
        }

        Activate();
    }
    {
        Deactivate();
        if (swapchainData.imageViewCount > 0U) {
            const RenderHandle firstSwapchain = swapchainData.imageViews[0U].GetHandle();
            gpuResourceMgr_->RemapGpuImageHandle(swapchainData.remappableSwapchainImage.GetHandle(), firstSwapchain);
            // check for default swapchain existance and remap
            if (!defaultSwapchainHandle_) {
                const RenderHandle shallowHandle =
                    gpuResourceMgr_->GetImageRawHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER);
                gpuResourceMgr_->RemapGpuImageHandle(shallowHandle, firstSwapchain);
                swapchainData.remappableAdditionalSwapchainImage = shallowHandle;
                // store, that we use default built-in swapchain for backbuffer
                defaultSwapchainHandle_ = swapchainData.remappableSwapchainImage;
            }
        }
    }

    // configure automatically backbuffer as swapchain
    {
        IRenderDataStoreManager& rdsm = renderContext_.GetRenderDataStoreManager();
        auto dataStorePod = static_cast<IRenderDataStorePod*>(rdsm.GetRenderDataStore(RenderDataStorePod::TYPE_NAME));
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
    if ((defaultSwapchainHandle_.GetHandle() == swapchainData.remappableSwapchainImage.GetHandle()) &&
        (swapchainCreateInfo.swapchainFlags & SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT)) {
        CreateDepthBuffer(GetBackendType(), *swapchainData.swapchain, *gpuResourceMgr_, swapchainData);
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((defaultSwapchainHandle_.GetHandle() != swapchainData.remappableSwapchainImage.GetHandle()) &&
        (swapchainCreateInfo.swapchainFlags & SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT)) {
        PLUGIN_LOG_W("RENDER_VALIDATION: Automatic swapchain depth buffer creation supported for default swapchain.");
    }
#endif

    return swapchainData.remappableSwapchainImage;
}

RenderHandleReference Device::CreateSwapchainHandle(
    const SwapchainCreateInfo& swapchainCreateInfo, const RenderHandleReference& replacedHandle, const string_view name)
{
    if (replacedHandle.GetHandle() == defaultSwapchainHandle_.GetHandle()) {
        // first safety destroy for legacy default swapchain
        DestroySwapchainImpl(defaultSwapchainHandle_);
    }
    return CreateSwapchainImpl(swapchainCreateInfo, replacedHandle, name);
}

RenderHandleReference Device::CreateSwapchainHandle(const SwapchainCreateInfo& swapchainCreateInfo)
{
    // first safety destroy for legacy default swapchain
    DestroySwapchainImpl(defaultSwapchainHandle_);
    return CreateSwapchainImpl(swapchainCreateInfo, {}, {});
}

void Device::CreateSwapchain(const SwapchainCreateInfo& swapchainCreateInfo)
{
    // first safety destroy for legacy default swapchain
    DestroySwapchainImpl(defaultSwapchainHandle_);
    CreateSwapchainImpl(swapchainCreateInfo, {}, {});
}

void Device::DestroySwapchainImpl(const RenderHandleReference& handle)
{
    // NOTE: the destruction should be deferred, but we expect this to be called from rendering thread
    if (isRenderbackendRunning_) {
        // NOTE: we are currently only sending error message and not trying to prevent
        PLUGIN_LOG_E("DestroySwapchain called while RenderFrame is running");
    }

    if (handle) {
        const RenderHandle rawHandle = handle.GetHandle();
        for (auto iter = swapchains_.cbegin(); iter != swapchains_.cend(); ++iter) {
            if (iter->remappableSwapchainImage.GetHandle() == rawHandle) {
                Activate();
                WaitForIdle();

                swapchains_.erase(iter);

                DestroyDeviceSwapchain();
                // remove swapchain configuration from the backbuffer
                if ((handle.GetHandle() == defaultSwapchainHandle_.GetHandle()) || (!handle)) {
                    IRenderDataStoreManager& rdsm = renderContext_.GetRenderDataStoreManager();
                    auto dataStorePod =
                        static_cast<IRenderDataStorePod*>(rdsm.GetRenderDataStore(RenderDataStorePod::TYPE_NAME));
                    if (dataStorePod) {
                        auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
                        PLUGIN_ASSERT(dataView.size_bytes() == sizeof(NodeGraphBackBufferConfiguration));
                        NodeGraphBackBufferConfiguration ngbbc =
                            *(const NodeGraphBackBufferConfiguration*)dataView.data();
                        if (ngbbc.backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN) {
                            ngbbc.backBufferType = NodeGraphBackBufferConfiguration::BackBufferType::UNDEFINED;
                            ngbbc.present = false;
                        }
                        dataStorePod->Set("NodeGraphBackBufferConfiguration", arrayviewU8(ngbbc));
                    }
                }
                Deactivate();

                // destroy default swapchain handle if it was in use
                if (handle.GetHandle() == defaultSwapchainHandle_.GetHandle()) {
                    defaultSwapchainHandle_ = {};
                }

                // element erased -> break
                break;
            }
        }
    }
}

void Device::DestroySwapchain()
{
    DestroySwapchainImpl(defaultSwapchainHandle_);
}

void Device::DestroySwapchain(const RenderHandleReference& handle)
{
    DestroySwapchainImpl(handle);
}

SurfaceTransformFlags Device::GetSurfaceTransformFlags(const RenderHandle& handle) const
{
    // NOTE: we lock data hear to check for the transforms flags
    // the flags should be stored, and the data should not be locked for every frame access
    if (RenderHandleUtil::IsSwapchain(handle)) {
        for (const auto& swapchain : swapchains_) {
            if (((swapchain.remappableSwapchainImage.GetHandle() == handle) ||
                    (swapchain.remappableAdditionalSwapchainImage == handle)) &&
                (swapchain.swapchain)) {
                return swapchain.swapchain->GetSurfaceTransformFlags();
            }
        }
    }
    return 0u;
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
};
uint64_t Device::GetFrameCount() const
{
    return frameCount_;
}

const CommonDeviceProperties& Device::GetCommonDeviceProperties() const
{
    return commonDeviceProperties_;
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
    return (!swapchains_.empty());
}

const Swapchain* Device::GetSwapchain(const RenderHandle handle) const
{
    const RenderHandle cmpHandle = RenderHandleUtil::IsValid(handle) ? handle : defaultSwapchainHandle_.GetHandle();
    // NOTE: returns a pointer to data which could be destroyed
    // if the API documentation and restrictions are not applied by the user
    for (const auto& swapData : swapchains_) {
        if (((swapData.remappableSwapchainImage.GetHandle() == cmpHandle) ||
                (swapData.remappableAdditionalSwapchainImage == cmpHandle)) &&
            (swapData.swapchain)) {
            return swapData.swapchain.get();
        }
    }
    return nullptr;
}

Device::SwapchainData Device::GetSwapchainData(const RenderHandle handle) const
{
    PLUGIN_ASSERT(isRenderbackendRunning_);

    const RenderHandle cmpHandle = RenderHandleUtil::IsValid(handle) ? handle : defaultSwapchainHandle_.GetHandle();
    for (const auto& swapData : swapchains_) {
        if (((swapData.remappableSwapchainImage.GetHandle() == cmpHandle) ||
                (swapData.remappableAdditionalSwapchainImage == cmpHandle)) &&
            (swapData.swapchain)) {
            SwapchainData sd;
            // comparison handle should be a valid handle here and is used in image re-mapping
            sd.remappableSwapchainImage = cmpHandle;
            PLUGIN_ASSERT(swapData.imageViewCount <= SwapchainData::MAX_IMAGE_VIEW_COUNT);
            sd.imageViewCount = swapData.imageViewCount;
            for (uint32_t idx = 0; idx < swapData.imageViewCount; ++idx) {
                sd.imageViews[idx] = swapData.imageViews[idx].GetHandle();
            }
            return sd;
        }
    }
    return {};
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

void Device::SetRenderFrameRunning(const bool value)
{
    isRenderFrameRunning_ = value;
}

bool Device::GetRenderFrameRunning() const
{
    return isRenderFrameRunning_;
}

IGpuResourceManager& Device::GetGpuResourceManager() const
{
    return *gpuResourceMgr_;
}

IShaderManager& Device::GetShaderManager() const
{
    return *shaderMgr_;
}

void Device::SetBackendConfig(const BackendConfig& config) {}

Format Device::GetFormatOrFallback(const Format inputFormat) const
{
    Format format = inputFormat;
    if (static_cast<uint32_t>(format) < LINEAR_FORMAT_COUNT) {
        auto properties = GetFormatProperties(format);
#if (RENDER_VALIDATION_ENABLED == 1)
        uint32_t fallbackCount = 0U;
#endif
        while (format != Format::BASE_FORMAT_UNDEFINED && !properties.linearTilingFeatures &&
               !properties.optimalTilingFeatures) {
            format = FALLBACK_FORMATS[format];
            properties = GetFormatProperties(format);
#if (RENDER_VALIDATION_ENABLED == 1)
            fallbackCount++;
#endif
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if (fallbackCount > 0U) {
            PLUGIN_LOG_I("RENDER_VALIDATION: input format (%u) fallback format (%u)",
                static_cast<uint32_t>(inputFormat), static_cast<uint32_t>(format));
        }
#endif
    }
    return format;
}
RENDER_END_NAMESPACE()
