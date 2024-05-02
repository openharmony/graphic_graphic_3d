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

#ifndef DEVICE_DEVICE_H
#define DEVICE_DEVICE_H

#include <atomic>
#include <cstdint>
#include <mutex>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/util/formats.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>
#include <render/device/intf_device.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/shader_manager.h"
#include "device/swapchain.h"

RENDER_BEGIN_NAMESPACE()
class RenderContext;
class GpuResourceManager;

class ShaderModule;
class ComputePipelineStateObject;
class GpuBuffer;
class GpuComputeProgram;
class GpuImage;
class GpuSampler;
class GpuResourceManager;
class GpuComputeProgram;
class GpuShaderProgram;
class GraphicsPipelineStateObject;
class PlatformGpuMemoryAllocator;
class RenderBackend;
class RenderFrameSync;
class NodeContextDescriptorSetManager;
class NodeContextPoolManager;
class ShaderModule;
class Swapchain;
class GpuSemaphore;
struct BackendSpecificImageDesc;
struct GpuAccelerationStructureDesc;
struct GpuBufferDesc;
struct GpuComputeProgramCreateData;
struct GpuImageDesc;
struct GpuImagePlatformData;
struct GpuSamplerDesc;
struct GpuShaderProgramCreateData;
struct PipelineLayout;
struct ShaderModuleCreateInfo;
struct SwapchainCreateInfo;

struct LowLevelRenderPassData {};
struct LowLevelPipelineLayoutData {};

struct DeviceFormatSupportConstants {
    static constexpr uint32_t ALL_FLAGS_SUPPORTED { 0xFFFFu };
    static constexpr uint32_t LINEAR_FORMAT_MAX_IDX { BASE_NS::Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK };
    static constexpr uint32_t LINEAR_FORMAT_MAX_COUNT { LINEAR_FORMAT_MAX_IDX + 1u };
    static constexpr uint32_t ADDITIONAL_FORMAT_START_NUMBER { BASE_NS::Format::BASE_FORMAT_G8B8G8R8_422_UNORM };
    static constexpr uint32_t ADDITIONAL_FORMAT_END_NUMBER { BASE_NS::Format::BASE_FORMAT_G8_B8_R8_3PLANE_444_UNORM };
    static constexpr uint32_t ADDITIONAL_FORMAT_MAX_COUNT {
        (ADDITIONAL_FORMAT_END_NUMBER - ADDITIONAL_FORMAT_START_NUMBER) + 1u
    };
    static constexpr uint32_t ADDITIONAL_FORMAT_BASE_IDX { LINEAR_FORMAT_MAX_COUNT };
};

struct DeviceConstants {
    static constexpr uint32_t MAX_SWAPCHAIN_COUNT { 8U };
};

class Device : public IDevice {
public:
    Device(RenderContext& renderContext, const DeviceCreateInfo& deviceCreateInfo);

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    virtual PlatformGpuMemoryAllocator* GetPlatformGpuMemoryAllocator() = 0;

    // Activate context
    virtual void Activate() = 0;
    // Deactivate context
    virtual void Deactivate() = 0;

    // Allow parallel render node processing  (on GLES checks that coherent mapping allowed)
    virtual bool AllowThreadedProcessing() const = 0;

    // Generally set once to true when device is created.
    // Set to false e.g. when device is lost.
    // Set and Get uses atomics if needed by the platform.
    void SetDeviceStatus(const bool status);
    bool GetDeviceStatus() const;

    // (re-)create swapchain
    void CreateSwapchain(const SwapchainCreateInfo& swapchainCreateInfo) override;
    RenderHandleReference CreateSwapchainHandle(const SwapchainCreateInfo& swapchainCreateInfo,
        const RenderHandleReference& replacedHandle, const BASE_NS::string_view name) override;
    RenderHandleReference CreateSwapchainHandle(const SwapchainCreateInfo& swapchainCreateInfo) override;
    void DestroySwapchain() override;
    void DestroySwapchain(const RenderHandleReference& handle) override;
    // device specific preparation
    virtual BASE_NS::unique_ptr<Swapchain> CreateDeviceSwapchain(const SwapchainCreateInfo& swapchainCreateInfo) = 0;
    virtual void DestroyDeviceSwapchain() = 0;

    RenderHandleReference CreateSwapchainImpl(const SwapchainCreateInfo& swapchainCreateInfo,
        const RenderHandleReference& replacedHandle, const BASE_NS::string_view name);
    void DestroySwapchainImpl(const RenderHandleReference& handle);

    SurfaceTransformFlags GetSurfaceTransformFlags(const RenderHandle& handle) const;

    // Lock/Unlock access for this frame's GPU resources (for safety reasons)
    void SetLockResourceBackendAccess(bool value);
    // Set render backend running
    void SetRenderBackendRunning(bool value);
    bool GetLockResourceBackendAccess() const;

    // Set to true when RenderFrame called
    // Set to false when coming out of RenderFrame
    void SetRenderFrameRunning(bool value);
    bool GetRenderFrameRunning() const;

    // Marks the beginning of a frame rendering. Expected to at least increment frame counter.
    void FrameStart();
    void FrameEnd();

    IGpuResourceManager& GetGpuResourceManager() const override;
    IShaderManager& GetShaderManager() const override;

    void SetBackendConfig(const BackendConfig& config) override;

    uint64_t GetFrameCount() const override;

    const CommonDeviceProperties& GetCommonDeviceProperties() const override;

    DeviceConfiguration GetDeviceConfiguration() const override;

    bool HasSwapchain() const;
    // get swapchain with shallow
    const Swapchain* GetSwapchain(const RenderHandle handle) const;

    struct SwapchainData {
        static constexpr uint32_t MAX_IMAGE_VIEW_COUNT { 5U };

        RenderHandle remappableSwapchainImage {};
        RenderHandle imageViews[MAX_IMAGE_VIEW_COUNT];
        uint32_t imageViewCount { 0U };
    };

    struct InternalSwapchainData {
        static constexpr uint32_t MAX_IMAGE_VIEW_COUNT { 5U };

        uint64_t surfaceHandle { 0 };
        uintptr_t window { 0 };

        BASE_NS::string globalName;
        BASE_NS::string name;
        RenderHandleReference remappableSwapchainImage {};
        RenderHandleReference additionalDepthBufferHandle {};
        RenderHandle remappableAdditionalSwapchainImage {}; // Not owned
        RenderHandleReference imageViews[MAX_IMAGE_VIEW_COUNT];
        uint32_t imageViewCount { 0U };

        BASE_NS::unique_ptr<Swapchain> swapchain;
    };
    SwapchainData GetSwapchainData(const RenderHandle handle) const;

    // Number of command buffers to allocate for each node
    uint32_t GetCommandBufferingCount() const;
    // Get ANDed memory property flags from all memory pools
    MemoryPropertyFlags GetSharedMemoryPropertyFlags() const;

    BASE_NS::Format GetFormatOrFallback(BASE_NS::Format format) const;

    // Platform specific creation

    virtual BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuBufferDesc& desc) = 0;
    virtual BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuAccelerationStructureDesc& desc) = 0;

    // Create gpu image resources
    virtual BASE_NS::unique_ptr<GpuImage> CreateGpuImage(const GpuImageDesc& desc) = 0;
    /** Creates a engine GpuImage resource with platform resources.
     * Does not own the platform resources nor does not destroy them.
     */
    virtual BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const GpuImagePlatformData& platformData) = 0;
    virtual BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData) = 0;
    virtual BASE_NS::vector<BASE_NS::unique_ptr<GpuImage>> CreateGpuImageViews(const Swapchain& platformData) = 0;

    virtual BASE_NS::unique_ptr<GpuSampler> CreateGpuSampler(const GpuSamplerDesc& desc) = 0;

    virtual BASE_NS::unique_ptr<RenderFrameSync> CreateRenderFrameSync() = 0;
    virtual BASE_NS::unique_ptr<RenderBackend> CreateRenderBackend(
        GpuResourceManager& gpuResourceMgr, const CORE_NS::IParallelTaskQueue::Ptr& queue) = 0;

    virtual BASE_NS::unique_ptr<ShaderModule> CreateShaderModule(const ShaderModuleCreateInfo& data) = 0;
    virtual BASE_NS::unique_ptr<ShaderModule> CreateComputeShaderModule(const ShaderModuleCreateInfo& data) = 0;
    virtual BASE_NS::unique_ptr<GpuShaderProgram> CreateGpuShaderProgram(GpuShaderProgramCreateData const& data) = 0;
    virtual BASE_NS::unique_ptr<GpuComputeProgram> CreateGpuComputeProgram(GpuComputeProgramCreateData const& data) = 0;

    virtual BASE_NS::unique_ptr<NodeContextDescriptorSetManager> CreateNodeContextDescriptorSetManager() = 0;
    virtual BASE_NS::unique_ptr<NodeContextPoolManager> CreateNodeContextPoolManager(
        class GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue) = 0;

    virtual BASE_NS::unique_ptr<GraphicsPipelineStateObject> CreateGraphicsPipelineStateObject(
        const GpuShaderProgram& gpuProgram, const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
        const VertexInputDeclarationView& vertexInputDeclarationView,
        const ShaderSpecializationConstantDataView& shaderSpecializationConstantDataView,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDesc, const uint32_t subpassIndex,
        const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData) = 0;

    virtual BASE_NS::unique_ptr<ComputePipelineStateObject> CreateComputePipelineStateObject(
        const GpuComputeProgram& gpuProgram, const PipelineLayout& pipelineLayout,
        const ShaderSpecializationConstantDataView& shaderSpecializationConstantDataView,
        const LowLevelPipelineLayoutData* pipelineLayoutData) = 0;

    virtual BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphore() = 0;
    virtual BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphoreView(const uint64_t handle) = 0;

    // returns a valid queue from the same family or default queue
    virtual GpuQueue GetValidGpuQueue(const GpuQueue& gpuQueue) const = 0;
    virtual uint32_t GetGpuQueueCount() const = 0;

    /** Initialize cache for accelerating pipeline state object creation. Caching is not used unless this function is
     * called.
     * @param initialData Optional cache content returned by GetPipelineCache.
     */
    virtual void InitializePipelineCache(BASE_NS::array_view<const uint8_t> initialData) = 0;
    virtual BASE_NS::vector<uint8_t> GetPipelineCache() const = 0;

protected:
    RenderContext& renderContext_;
    DeviceConfiguration deviceConfiguration_;

    BASE_NS::unique_ptr<GpuResourceManager> gpuResourceMgr_;
    BASE_NS::unique_ptr<ShaderManager> shaderMgr_;

    // multi-swapchain, the built-in default is only with swapchain_ and swapchainData_
    BASE_NS::vector<InternalSwapchainData> swapchains_;
    RenderHandleReference defaultSwapchainHandle_;

    std::atomic<bool> deviceStatus_ { false };
    uint64_t frameCount_ { 0 };

    bool isRenderbackendRunning_ { false };
    bool isBackendResourceAccessLocked_ { false };
    bool isRenderFrameRunning_ { false };

    MemoryPropertyFlags deviceSharedMemoryPropertyFlags_ { 0 };
    CommonDeviceProperties commonDeviceProperties_;
};

// Plaform specific helper
GpuImageDesc GetImageDescFromHwBufferDesc(uintptr_t platformHwBuffer);
RENDER_END_NAMESPACE()

#endif // DEVICE_DEVICE_H
