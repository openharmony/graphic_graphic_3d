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

#ifndef VULKAN_DEVICE_VK_H
#define VULKAN_DEVICE_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/formats.h>
#include <render/device/intf_device.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "device/device.h"
#include "platform_vk.h"
#include "vulkan/swapchain_vk.h"

RENDER_BEGIN_NAMESPACE()
class ComputePipelineStateObject;
class GraphicsPipelineStateObject;
class GpuBuffer;
class GpuComputeProgram;
class GpuImage;
class GpuResourceManager;
class GpuSemaphore;
class GpuSampler;
class GpuShaderProgram;
class LowLevelDeviceVk;
class NodeContextDescriptorSetManager;
class NodeContextPoolManager;
class PlatformGpuMemoryAllocator;
class RenderFrameSync;
class RenderBackend;
class RenderContext;
class ShaderModule;
class Swapchain;

struct GpuImagePlatformData;
struct SwapchainCreateInfo;
struct BackendSpecificImageDesc;
struct GpuAccelerationStructureDesc;
struct GpuBufferDesc;
struct GpuComputeProgramCreateData;
struct GpuImageDesc;
struct GpuSamplerDesc;
struct GpuShaderProgramCreateData;
struct PipelineLayout;
struct RenderHandle;
struct ShaderModuleCreateInfo;
struct QueueProperties;

struct LowLevelQueueInfo {
    VkQueueFlags queueFlags { 0 };
    uint32_t queueFamilyIndex { ~0u };
    uint32_t queueCount { 0 };
    float priority { 1.0f };
};

struct LowLevelGpuQueueVk {
    VkQueue queue { VK_NULL_HANDLE };

    LowLevelQueueInfo queueInfo;
};

struct DevicePlatformInternalDataVk {
    BASE_NS::vector<BASE_NS::Format> supportedDepthFormats;
};

struct DebugFunctionUtilitiesVk {
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT { nullptr };
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT { nullptr };
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT { nullptr };

    VkDebugUtilsMessengerEXT debugMessenger { VK_NULL_HANDLE };
    VkDebugReportCallbackEXT debugCallback { VK_NULL_HANDLE };
};

class DeviceVk final : public Device {
public:
    DeviceVk(RenderContext& renderContext, DeviceCreateInfo const& createInfo);
    ~DeviceVk() override;

    // From IDevice
    DeviceBackendType GetBackendType() const override;
    const DevicePlatformData& GetPlatformData() const override;
    FormatProperties GetFormatProperties(const BASE_NS::Format format) const override;
    AccelerationStructureBuildSizes GetAccelerationStructureBuildSizes(
        const AccelerationStructureBuildGeometryInfo& geometry,
        BASE_NS::array_view<const AccelerationStructureGeometryTrianglesInfo> triangles,
        BASE_NS::array_view<const AccelerationStructureGeometryAabbsInfo> aabbs,
        BASE_NS::array_view<const AccelerationStructureGeometryInstancesInfo> instances) const override;
    ILowLevelDevice& GetLowLevelDevice() const override;
    void WaitForIdle() override;

    const DevicePlatformInternalDataVk& GetPlatformInternalDataVk() const;
    const DevicePlatformDataVk& GetPlatformDataVk() const;

    PlatformGpuMemoryAllocator* GetPlatformGpuMemoryAllocator() override;

    BASE_NS::unique_ptr<Swapchain> CreateDeviceSwapchain(const SwapchainCreateInfo& swapchainCreateInfo) override;
    void DestroyDeviceSwapchain() override;

    void Activate() override;
    void Deactivate() override;

    bool AllowThreadedProcessing() const override;

    GpuQueue GetValidGpuQueue(const GpuQueue& gpuQueue) const override;
    uint32_t GetGpuQueueCount() const override;

    void InitializePipelineCache(BASE_NS::array_view<const uint8_t> initialData) override;
    BASE_NS::vector<uint8_t> GetPipelineCache() const override;

    LowLevelGpuQueueVk GetGpuQueue(const GpuQueue& gpuQueue) const;
    LowLevelGpuQueueVk GetPresentationGpuQueue() const;
    BASE_NS::vector<LowLevelGpuQueueVk> GetLowLevelGpuQueues() const;

    BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuBufferDesc& desc) override;
    BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuAccelerationStructureDesc& desc) override;

    BASE_NS::unique_ptr<GpuImage> CreateGpuImage(const GpuImageDesc& desc) override;
    BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const GpuImagePlatformData& platformData) override;
    BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData) override;
    BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const GpuImagePlatformData& platformData, const uintptr_t hwBuffer);
    BASE_NS::vector<BASE_NS::unique_ptr<GpuImage>> CreateGpuImageViews(const Swapchain& platformData) override;

    BASE_NS::unique_ptr<GpuSampler> CreateGpuSampler(const GpuSamplerDesc& desc) override;

    BASE_NS::unique_ptr<RenderFrameSync> CreateRenderFrameSync() override;

    BASE_NS::unique_ptr<RenderBackend> CreateRenderBackend(
        GpuResourceManager& gpuResourceMgr, const CORE_NS::IParallelTaskQueue::Ptr& queue) override;

    BASE_NS::unique_ptr<ShaderModule> CreateShaderModule(const ShaderModuleCreateInfo& data) override;
    BASE_NS::unique_ptr<ShaderModule> CreateComputeShaderModule(const ShaderModuleCreateInfo& data) override;
    BASE_NS::unique_ptr<GpuShaderProgram> CreateGpuShaderProgram(const GpuShaderProgramCreateData& data) override;
    BASE_NS::unique_ptr<GpuComputeProgram> CreateGpuComputeProgram(const GpuComputeProgramCreateData& data) override;

    BASE_NS::unique_ptr<NodeContextDescriptorSetManager> CreateNodeContextDescriptorSetManager() override;
    BASE_NS::unique_ptr<NodeContextPoolManager> CreateNodeContextPoolManager(
        class GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue) override;

    BASE_NS::unique_ptr<GraphicsPipelineStateObject> CreateGraphicsPipelineStateObject(
        const GpuShaderProgram& gpuProgram, const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
        const VertexInputDeclarationView& vertexInputDeclaration,
        const ShaderSpecializationConstantDataView& specializationConstants,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex,
        const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData) override;

    BASE_NS::unique_ptr<ComputePipelineStateObject> CreateComputePipelineStateObject(
        const GpuComputeProgram& gpuProgram, const PipelineLayout& pipelineLayout,
        const ShaderSpecializationConstantDataView& specializationConstants,
        const LowLevelPipelineLayoutData* pipelineLayoutData) override;

    BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphore() override;
    BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphoreView(const uint64_t handle) override;

    struct FeatureConfigurations {
        float minSampleShading { 0.25f };
    };
    const FeatureConfigurations& GetFeatureConfigurations() const;

    struct CommonDeviceExtensions {
        bool swapchain { false };

        // external_memory and external_memory_capabilities
        bool externalMemory { false };
        bool getMemoryRequirements2 { false };
        bool samplerYcbcrConversion { false };
        bool queueFamilyForeign { false };

        bool renderPass2 { false };
        bool fragmentShadingRate { false };
        bool multiView { false };
        bool descriptorIndexing { false };
    };
    const CommonDeviceExtensions& GetCommonDeviceExtensions() const;
    const PlatformDeviceExtensions& GetPlatformDeviceExtensions() const;
    bool HasDeviceExtension(const BASE_NS::string_view extensionName) const;

    const DebugFunctionUtilitiesVk& GetDebugFunctionUtilities() const;
    void CreateDebugFunctions();

    struct ExtFunctions {
        // VK_KHR_sampler_ycbcr_conversion or Vulkan 1.1
        PFN_vkCreateSamplerYcbcrConversion vkCreateSamplerYcbcrConversion { nullptr };
        PFN_vkDestroySamplerYcbcrConversion vkDestroySamplerYcbcrConversion { nullptr };

        // VK_KHR_get_memory_requirements2 or Vulkan 1.1
        PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2 { nullptr };
        // VK_KHR_get_physical_device_properties2 or Vulkan 1.1
        PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 { nullptr };
        PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 { nullptr };

        // VK_KHR_create_renderpass2 or Vulkan 1.2
        PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR { nullptr };

        // VK_KHR_swapchain
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR { nullptr };

#if (RENDER_VULKAN_FSR_ENABLED == 1)
        // VK_KHR_fragment_shading_rate
        PFN_vkCmdSetFragmentShadingRateKHR vkCmdSetFragmentShadingRateKHR { nullptr };
#endif

#if (RENDER_VULKAN_RT_ENABLED == 1)
        // VK_KHR_acceleration_structure
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR { nullptr };
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR { nullptr };
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR { nullptr };
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR { nullptr };
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR { nullptr };
#endif
    };
    const ExtFunctions& GetExtFunctions() const;
    void CreateExtFunctions();

    const PlatformExtFunctions& GetPlatformExtFunctions() const;
    void CreatePlatformExtFunctions();

private:
    BASE_NS::vector<QueueProperties> CheckExternalConfig(const BackendExtraVk* backendConfiguration);
    void CreateInstance();
    void CreatePhysicalDevice();
    void CreateDevice(const BackendExtraVk* backendExtra, const BASE_NS::vector<LowLevelQueueInfo>& availableQueues);
    void SortAvailableQueues(const BASE_NS::vector<LowLevelQueueInfo>& availableQueues);

    BASE_NS::unique_ptr<PlatformGpuMemoryAllocator> platformGpuMemoryAllocator_;

    DevicePlatformDataVk plat_;
    bool ownInstanceAndDevice_ { true };
    DevicePlatformInternalDataVk platInternal_;

    FeatureConfigurations featureConfigurations_;

    struct LowLevelGpuQueues {
        BASE_NS::vector<LowLevelGpuQueueVk> graphicsQueues;
        BASE_NS::vector<LowLevelGpuQueueVk> computeQueues;
        BASE_NS::vector<LowLevelGpuQueueVk> transferQueues;

        LowLevelGpuQueueVk defaultQueue;
    };
    LowLevelGpuQueues lowLevelGpuQueues_;

    uint32_t gpuQueueCount_ { 0 };

    BASE_NS::unordered_map<BASE_NS::string, uint32_t> extensions_;
    CommonDeviceExtensions commonDeviceExtensions_;
    PlatformDeviceExtensions platformDeviceExtensions_;
    BASE_NS::unique_ptr<LowLevelDeviceVk> lowLevelDevice_;

    BASE_NS::vector<FormatProperties> formatProperties_;

    DebugFunctionUtilitiesVk debugFunctionUtilities_;
    ExtFunctions extFunctions_;
    PlatformExtFunctions platformExtFunctions_;
};

BASE_NS::unique_ptr<Device> CreateDeviceVk(RenderContext& renderContext, DeviceCreateInfo const& createInfo);

// Wrapper for low level device access
class LowLevelDeviceVk final : public ILowLevelDeviceVk {
public:
    explicit LowLevelDeviceVk(DeviceVk& deviceVk);
    ~LowLevelDeviceVk() = default;

    DeviceBackendType GetBackendType() const override;
    const DevicePlatformDataVk& GetPlatformDataVk() const override;

    GpuBufferPlatformDataVk GetBuffer(RenderHandle handle) const override;
    GpuImagePlatformDataVk GetImage(RenderHandle handle) const override;
    GpuSamplerPlatformDataVk GetSampler(RenderHandle handle) const override;

private:
    DeviceVk& deviceVk_;
    GpuResourceManager& gpuResourceMgr_;
};
RENDER_END_NAMESPACE()

#endif // VULKAN_DEVICE_VK_H
