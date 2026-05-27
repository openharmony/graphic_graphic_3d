/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MALEOON_DEVICE_MLN_H
#define MALEOON_DEVICE_MLN_H

#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/util/formats.h>
#include <render/device/intf_device.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/maleoon/intf_device_mln.h>

#include "device/device.h"

#include <maleoon/maleoon.h>

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
struct ShaderModuleCreateInfo;

class LowLevelDeviceMln;

class DeviceMln final : public Device {
public:
    explicit DeviceMln(RenderContext& renderContext);
    ~DeviceMln() override;

    // From IDevice
    DeviceBackendType GetBackendType() const override;
    const DevicePlatformData& GetPlatformData() const override;
    FormatProperties GetFormatProperties(BASE_NS::Format format) const override;
    AsBuildSizes GetAccelerationStructureBuildSizes(const AsBuildGeometryInfo& geometry,
        BASE_NS::array_view<const AsGeometryTrianglesInfo> triangles,
        BASE_NS::array_view<const AsGeometryAabbsInfo> aabbs,
        BASE_NS::array_view<const AsGeometryInstancesInfo> instances) const override;
    ILowLevelDevice& GetLowLevelDevice() const override;
    void WaitForIdle() override;

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

    BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuBufferDesc& desc) override;
    BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuAccelerationStructureDesc& desc) override;
    BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const BackendSpecificBufferDesc& desc) override;

    BASE_NS::unique_ptr<GpuImage> CreateGpuImage(const GpuImageDesc& desc) override;
    BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const GpuImagePlatformData& platformData) override;
    BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData) override;
    BASE_NS::vector<BASE_NS::unique_ptr<GpuImage>> CreateGpuImageViews(const Swapchain& platformData) override;

    BASE_NS::unique_ptr<GpuSampler> CreateGpuSampler(const GpuSamplerDesc& desc) override;

    BASE_NS::unique_ptr<RenderFrameSync> CreateRenderFrameSync() override;

    BASE_NS::unique_ptr<RenderBackend> CreateRenderBackend(
        GpuResourceManager& gpuResourceMgr, CORE_NS::ITaskQueue* queue) override;

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
        BASE_NS::array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, uint32_t subpassIndex,
        const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData) override;

    BASE_NS::unique_ptr<ComputePipelineStateObject> CreateComputePipelineStateObject(
        const GpuComputeProgram& gpuProgram, const PipelineLayout& pipelineLayout,
        const ShaderSpecializationConstantDataView& specializationConstants,
        const LowLevelPipelineLayoutData* pipelineLayoutData) override;

    BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphore() override;
    BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphoreView(uint64_t handle) override;

    const MlnDevice& GetMlnDevice() const { return mlnDevice_; }
    const MlnQueue& GetMlnQueue() const { return mlnQueue_; }
    MlnProgramCache GetMlnProgramCache() const { return mlnProgramCache_; }
    const MlnGpuMemoryProperties& GetCachedMemoryProperties() const { return cachedMemoryProps_; }

private:
    void InitMaleoonDevice();
    void QueryFormatProperties();

    DevicePlatformDataMln platData_;
    BASE_NS::unique_ptr<LowLevelDeviceMln> lowLevelDevice_;
    BASE_NS::vector<FormatProperties> formatProperties_;

    MlnDevice mlnDevice_ { MLN_NULL_HANDLE };
    MlnQueue mlnQueue_ { MLN_NULL_HANDLE };
    MlnProgramCache mlnProgramCache_ { MLN_NULL_HANDLE };
    MlnGpuMemoryProperties cachedMemoryProps_ {};  // OPT #5: cached at init, avoid per-resource query
};

BASE_NS::unique_ptr<Device> CreateDeviceMln(RenderContext& renderContext);

// Wrapper for low level device access
class LowLevelDeviceMln final : public ILowLevelDeviceMln {
public:
    explicit LowLevelDeviceMln(DeviceMln& deviceMln);
    ~LowLevelDeviceMln() override = default;

    DeviceBackendType GetBackendType() const override;
    const DevicePlatformDataMln& GetPlatformDataMln() const override;

    GpuBufferPlatformDataMln GetBuffer(RenderHandle handle) const override;
    GpuImagePlatformDataMln GetImage(RenderHandle handle) const override;
    GpuSamplerPlatformDataMln GetSampler(RenderHandle handle) const override;

private:
    DeviceMln& deviceMln_;
    GpuResourceManager& gpuResourceMgr_;
};

RENDER_END_NAMESPACE()

#endif // MALEOON_DEVICE_MLN_H
