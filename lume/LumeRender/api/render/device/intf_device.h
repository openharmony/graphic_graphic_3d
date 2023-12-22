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

#ifndef API_RENDER_DEVICE_IDEVICE_H
#define API_RENDER_DEVICE_IDEVICE_H

#include <cstdint>

#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IGpuResourceManager;
class IShaderManager;

struct BackendExtra {};
struct RenderBackendRecordingState {};

/** Device configuration flag bits */
enum DeviceConfigurationFlagBits {
    /** Enable pipeline caching */
    CORE_DEVICE_CONFIGURATION_PIPELINE_CACHE_BIT = 1 << 0,
};

/** Swapchain flag bits */
enum SwapchainFlagBits {
    /** Color buffer */
    CORE_SWAPCHAIN_COLOR_BUFFER_BIT = 0x00000001,
    /** Depth buffer */
    CORE_SWAPCHAIN_DEPTH_BUFFER_BIT = 0x00000002,
};
/** Container for swapchain flag bits */
using SwapchainFlags = uint32_t;

/** Swapchain create info */
struct SwapchainCreateInfo {
    /** Surface handle */
    uint64_t surfaceHandle { 0 };
    /** Vertical sync enabled */
    bool vsync { true };
    /** Prefer sRBG format */
    bool preferSrgbFormat { true };
    /** Swapchain flags */
    SwapchainFlags swapchainFlags { SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT };
};

struct DeviceConfiguration {
    /** Maps to buffering count e.g. coherent gpu buffers and command buffers (vulkan)
     * default value is 3, supported values depends from the backend.
     * Should always be bigger or equal to swapchain count. Waits earlier in the frame.
     */
    uint32_t bufferingCount { 3u };
    /** Maps to desired swapchain count default value is 3,
     * and supported values depend from the backend.
     */
    uint32_t swapchainImageCount { 3u };

    /**
     * Integrated devices (mobile gpus, integrated desktop chips) expose all memory pools with host visible flags.
     * If all pools have requiredIntegratedMemoryFlags then this optimization will be enabled and
     * staging buffers are not used for this data. The data is directly mapped once for writing.
     * Set to zero (0u) to disable all staging optimizations.
     */
    uint32_t requiredIntegratedMemoryFlags { CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                             CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT };

    /** Additional options for device creation.
     */
    uint32_t configurationFlags { CORE_DEVICE_CONFIGURATION_PIPELINE_CACHE_BIT };
};

/** Device backend type */
enum class DeviceBackendType {
    /** Vulkan backend */
    VULKAN,
    /** GLES backend */
    OPENGLES,
    /** OpenGL backend */
    OPENGL
};

/** @ingroup group_gfx_idevice */
/** Device create info */
struct DeviceCreateInfo {
    /** Backend type to be created */
    DeviceBackendType backendType { DeviceBackendType::VULKAN };
    /** Device configuration */
    DeviceConfiguration deviceConfiguration;
    /** Backend configuration pointer */
    const BackendExtra* backendConfiguration { nullptr };
};

/** @ingroup group_gfx_ilowleveldevice */
/** ILowLevelDevice, to provide low-level access to platform device and resources. */
class ILowLevelDevice {
public:
    /** Returns backend type */
    virtual DeviceBackendType GetBackendType() const = 0;

protected:
    ILowLevelDevice() = default;
    virtual ~ILowLevelDevice() = default;
};

/** @ingroup group_gfx_idevice */
struct DevicePlatformData {};

/** @ingroup group_gfx_idevice */
/** IDevice */
class IDevice {
public:
    /** Create a swapchain for graphics API use.
     *  @param swapchainCreateInfo Swapchain creation info.
     */
    virtual void CreateSwapchain(const SwapchainCreateInfo& swapchainCreateInfo) = 0;

    /** Destroys swapchain
     */
    virtual void DestroySwapchain() = 0;

    /** Returns backend type (vulkan, gles, etc) */
    virtual DeviceBackendType GetBackendType() const = 0;

    /** Returns device platform data which needs to be cast for correct device. */
    virtual const DevicePlatformData& GetPlatformData() const = 0;

    /** Get GPU resource manager interface. */
    virtual IGpuResourceManager& GetGpuResourceManager() const = 0;
    /** Get shader manager interface. */
    virtual IShaderManager& GetShaderManager() const = 0;

    /** Returns supported flags for given format. */
    virtual FormatProperties GetFormatProperties(const BASE_NS::Format format) const = 0;

    /** Returns acceleration structure build sizes. Only a single geometry data should be valid.
     *  @param geometryInfo Build geometry info for size calculations.
     *  @param triangles Build geometry info for size calculations.
     *  @param aabb Build geometry info for size calculations.
     *  @param instances Build geometry info for size calculations.
     */
    virtual AccelerationStructureBuildSizes GetAccelerationStructureBuildSizes(
        const AccelerationStructureBuildGeometryInfo& geometry,
        BASE_NS::array_view<const AccelerationStructureGeometryTrianglesInfo> triangles,
        BASE_NS::array_view<const AccelerationStructureGeometryAabbsInfo> aabbs,
        BASE_NS::array_view<const AccelerationStructureGeometryInstancesInfo> instances) const = 0;

    /** Get frame count. */
    virtual uint64_t GetFrameCount() const = 0;

    /** Wait for the GPU to become idle.
     *  Needs to be called from render thread where renderFrame is called.
     *
     *  This is a hazardous method and should only be called with extreme caution.
     *  Do not call this method per frame.
     */
    virtual void WaitForIdle() = 0;

    /** Get low level device interface. Needs to be cast to correct ILowLevelDeviceXX based on backend type. */
    virtual ILowLevelDevice& GetLowLevelDevice() const = 0;

protected:
    IDevice() = default;
    virtual ~IDevice() = default;
};

/** @ingroup group_gfx_idevice */
struct GpuBufferPlatformData {};
struct GpuImagePlatformData {};
struct GpuSamplerPlatformData {};
struct GpuAccelerationStructurePlatformData {};
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_IDEVICE_H
