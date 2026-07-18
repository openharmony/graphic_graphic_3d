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

#ifndef API_RENDER_MALEOON_IDEVICE_MLN_H
#define API_RENDER_MALEOON_IDEVICE_MLN_H

#include <render/device/intf_device.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#if RENDER_HAS_MALEOON_BACKEND
#include <maleoon/maleoon.h>
#endif

RENDER_BEGIN_NAMESPACE()

#if RENDER_HAS_MALEOON_BACKEND || DOXYGEN

/** Backend extra for Maleoon device creation */
struct BackendExtraMln final : public BackendExtra {
    // Reserved for future Maleoon-specific creation parameters
};

/** Device platform data for Maleoon backend */
struct DevicePlatformDataMln final : DevicePlatformData {
    // Maleoon device handle (opaque)
    MlnDevice mlnDevice {MLN_NULL_HANDLE};
    // Maleoon queue handle (opaque)
    MlnQueue mlnQueue {MLN_NULL_HANDLE};
};

/** Low level Maleoon buffer access. (Usable only with low level engine use-cases) */
struct GpuBufferPlatformDataMln final : public GpuBufferPlatformData {
    MlnResource resource { MLN_NULL_HANDLE };
    MlnDeviceMemory memory { MLN_NULL_HANDLE };

    uint32_t bindMemoryByteSize { 0u };
    uint32_t fullByteSize { 0u };
    uint32_t currentByteOffset { 0u };

    MlnBufferUsageFlags usage { 0 };
    void* mappedData { nullptr };
    uint64_t deviceAddress { 0 };
};

/** Low level Maleoon image access. (Usable only with low level engine use-cases) */
struct GpuImagePlatformDataMln final : public GpuImagePlatformData {
    MlnResource resource { MLN_NULL_HANDLE };
    MlnResourceView resourceView { MLN_NULL_HANDLE };
    MlnResourceView resourceViewBase { MLN_NULL_HANDLE };
    MlnDeviceMemory memory { MLN_NULL_HANDLE };

    MlnFormat format { MLN_FORMAT_UNDEFINED };
    MlnSize3D extent { 0u, 0u, 0u };
    MlnImageType imageType { MLN_IMAGE_TYPE_2D };
    MlnImageUsageFlags usage { 0 };
    MlnSampleCountFlags samples { MLN_SAMPLE_COUNT_1_BIT };
    MlnImageTiling tiling { MLN_IMAGE_TILING_OPTIMAL };
    uint32_t mipLevels { 0u };
    uint32_t arrayLayers { 0u };
};

/** Low level Maleoon sampler access. (Usable only with low level engine use-cases) */
struct GpuSamplerPlatformDataMln final : public GpuSamplerPlatformData {
    MlnSampler sampler { MLN_NULL_HANDLE };
};

/** Recording state passed to ExecuteMLNBackendCommand/ExecuteMLNBackendFrame for Maleoon backend.
*  Since Maleoon is declarative (DG-based) rather than imperative (cmd-based), backend nodes
*  create MlnDataGraph objects and push them to the walker's output list via outDataGraphs.
*  The walker ensures correct command order by flushing any pending batches before this call.
*  The MLN-specific interface passes this as non-const reference, so GS plugins can directly
*  write to outDataGraphs and outDgResources without any const workaround.
*/

/** Maximum number of output resources tracked per DataGraph.
*  Originally sized for graphics RT (4 color + 1 depth + spare). But transfer DG can
*  upload many images per frame (staging often does 60+ COPY_BUF_IMG in frame 0).
*  If the cap is too small, downstream consumers don't get tracked, SG misses
*  dependency edges, and we get first-frame races (texture not uploaded → splash).
*  Bumped to 128 to cover heavy staging frames; per-instance size grows from
*  ~280B to ~4.6KB which is acceptable. */
static constexpr uint32_t MAX_DG_OUTPUT_RESOURCES {128};

/** Per-DG resource tracking info for SchedulingGraph dependency declarations.
*  GS plugins can optionally populate this for advanced output tracking;
*  framework auto-syncs defaults (outputCount=0, ALL_COMMANDS_BIT) for any DG
*  that the plugin did not track. */
struct DataGraphResourceInfo {
    uint32_t outputCount {0};
    MlnPassNodeResourceDescriptor outputs[MAX_DG_OUTPUT_RESOURCES] {};
    MlnAttachmentStoreOp storeOps[MAX_DG_OUTPUT_RESOURCES] {};
    MlnRenderTarget renderTarget { MLN_NULL_HANDLE };
    MlnProgramStageFlags srcStage { MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT };
    MlnProgramStageFlags dstStage { MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT };
};

struct RenderBackendRecordingStateMln final : public RenderBackendRecordingState {
    MlnDevice mlnDevice { MLN_NULL_HANDLE };

    /** Backend node pushes created DGs here. Writable through pointer indirection
     *  even when accessed via const& from the base interface. */
    BASE_NS::vector<MlnDataGraph>* outDataGraphs { nullptr };
    /** Optional per-DG resource tracking. Writable through pointer indirection.
     *  Framework auto-syncs defaults if plugin does not populate. */
    BASE_NS::vector<DataGraphResourceInfo>* outDgResources { nullptr };
};

/** Low level device interface for Maleoon backend.
 *  Resource access only valid with specific methods in IRenderBackendNode and IRenderDataStore.
 */
class ILowLevelDeviceMln : public ILowLevelDevice {
public:
    virtual const DevicePlatformDataMln& GetPlatformDataMln() const = 0;

    /** Get Maleoon buffer. Valid access only during rendering with node and data store methods. */
    virtual GpuBufferPlatformDataMln GetBuffer(RenderHandle handle) const = 0;
    /** Get Maleoon image. Valid access only during rendering with node and data store methods. */
    virtual GpuImagePlatformDataMln GetImage(RenderHandle handle) const = 0;
    /** Get Maleoon sampler. Valid access only during rendering with node and data store methods. */
    virtual GpuSamplerPlatformDataMln GetSampler(RenderHandle handle) const = 0;

protected:
    ILowLevelDeviceMln() = default;
    ~ILowLevelDeviceMln() = default;
};

#endif // RENDER_HAS_MALEOON_BACKEND

RENDER_END_NAMESPACE()

#endif // API_RENDER_MALEOON_IDEVICE_MLN_H
