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

#ifndef DEVICE_GPU_RESOURCE_MANAGER_H
#define DEVICE_GPU_RESOURCE_MANAGER_H

#include <mutex>
#include <shared_mutex>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_buffer.h"
#include "device/gpu_image.h"
#include "device/gpu_resource_handle_util.h" // for EngineResourceHandle

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuAccelerationStructure;
class GpuBufferManager;
class GpuImageManager;
class GpuSampler;
class GpuSamplerManager;
class GpuResourceCache;
class GpuResourceManagerBase;
template<typename ResourceType, typename CreateInfoType>
class GpuResourceManagerTyped;
struct GpuImagePlatformData;

/** Staging Gpu resource copy struct */
struct StagingCopyStruct {
    /** Data type enumeration */
    enum class DataType : uint8_t {
        /** Vector */
        DATA_TYPE_VECTOR = 0,
        /** Image container */
        DATA_TYPE_IMAGE_CONTAINER = 1,
        /** Direct copy from src, no CPU data with stagingData or imageContainerPtr */
        DATA_TYPE_DIRECT_SRC_COPY = 2,
        /** Resource to resource copy with graphics commands */
        DATA_TYPE_SRC_TO_DST_COPY = 2,
    };
    /** Copy type enumeration */
    enum class CopyType : uint8_t {
        /** Undefined */
        UNDEFINED = 0,
        /** Buffer to buffer */
        BUFFER_TO_BUFFER,
        /** Buffer to image */
        BUFFER_TO_IMAGE,
        /** Image to buffer */
        IMAGE_TO_BUFFER,
        /** Image to image */
        IMAGE_TO_IMAGE,
        /** Cpu to buffer */
        CPU_TO_BUFFER,
    };

    /** Data type, default vector */
    DataType dataType { DataType::DATA_TYPE_VECTOR };

    /** Source handle */
    RenderHandleReference srcHandle;
    /** Destination handle */
    RenderHandleReference dstHandle;

    /** Begin index */
    uint32_t beginIndex { 0 };
    /** Count */
    uint32_t count { 0 };

    /** Staging data */
    BASE_NS::vector<uint8_t> stagingData;
    /** Image container pointer */
    CORE_NS::IImageContainer::Ptr imageContainerPtr;

    /** Optional format for scaling */
    BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };

    /** Staging buffer bytesize. */
    uint32_t stagingBufferByteSize { 0u };

    /** Invalid operation which should be ignored. Might be a copy that has been removed during processing. */
    bool invalidOperation { false };
};

/** Staging image scaling info struct. Uses the same scaling image for all with the format */
struct StagingImageScalingStruct {
    /** Handle to the resource which is created just before staging */
    RenderHandleReference handle;
    /** Format of the scaling image */
    BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
    /** Maximum width of the scaling image */
    uint32_t maxWidth { 0u };
    /** Maximum height of the scaling image */
    uint32_t maxHeight { 0u };
};

/** All staging scaling image data */
struct ScalingImageDataStruct {
    /** Scaling resource infos (used for fast iteration) */
    BASE_NS::vector<StagingImageScalingStruct> scalingImages;
    /** Map with format to image scaling struct index */
    BASE_NS::unordered_map<uint32_t, size_t> formatToScalingImages;
};

/** Staging Gpu resource consume struct, this contains all staged resources for this frame */
struct StagingConsumeStruct {
    /** Buffer to buffer */
    BASE_NS::vector<StagingCopyStruct> bufferToBuffer;
    /** Buffer to image */
    BASE_NS::vector<StagingCopyStruct> bufferToImage;
    /** Image to buffer */
    BASE_NS::vector<StagingCopyStruct> imageToBuffer;
    /** Image to image */
    BASE_NS::vector<StagingCopyStruct> imageToImage;
    /** Direct CPU copy to buffer */
    BASE_NS::vector<StagingCopyStruct> cpuToBuffer;

    /** Buffer copies */
    BASE_NS::vector<BufferCopy> bufferCopies;
    /** Buffer image copies */
    BASE_NS::vector<BufferImageCopy> bufferImageCopies;
    /** Image copies */
    BASE_NS::vector<ImageCopy> imageCopies;

    /** Scaling image data */
    ScalingImageDataStruct scalingImageData;
};

/** Per frame work loop:
 *
 * 1. renderer.cpp calls HandlePendingAllocations() before any RenderNode-method calls
 * 2. renderer.cpp calls LockFrameStagingData() before RenderNode::ExecuteFrame call
 * 3. renderer.cpp calls HandlePendingAllocations() before RenderNode::ExecuteFrame call
 * 4. RenderBackendXX.cpp calls renderBackendHandleRemapping() before going through render command lists
 *
 * NOTE:
 *    - There are no allocations or deallocations after RenderNode::ExecuteFrame()
 *
 * Creation process:
 *
 * 1. Create-method is called. It will call StoreAllocation-method.
 * 2. StoreAllocation-method checks:
 *     - if replacing old resource with new (when creating with a same name)
 *     - if there exists handles (array slots) for recycle/reuse
 *     - if needs to Create new handle (push_backs several vectors)
 *     - push into pending allocation list
 *     - quite a lot of processing due to replacing handles and removing old staging ops etc.
 * 3. handlePendingAllocation is called from Render.cpp
 *     - allocates the actual new gpu resources
 *
 * Destruction process:
 *
 * 1. Destroy-method is called
 *     - invalidates nameToClientHandle
 *     - push into pending deallocation list
 * 2. handlePendingAllocation is called from Render.cpp
 *     - deallocates the actual gpu resources
 *     - sends the array index (handle array index) for recycling
 *
 * NOTE: It is safe to Destroy in RenderNode::ExecuteFrame()
 * The client handle has been added to render command list
 * and it is not invalidated in this particular frame.
 *
 * NOTE: Simplification would come from not able to replace handles with staging
 */
class GpuResourceManager final : public IGpuResourceManager {
public:
    static GpuBufferDesc GetStagingBufferDesc(const uint32_t byteSize);

    enum GpuResourceManagerFlagBits : uint32_t {
        /** Use direct copy with integrated GPUs were device memory is always host visible.
         * If this flag is set. Device is checked for availability.
         **/
        GPU_RESOURCE_MANAGER_OPTIMIZE_STAGING_MEMORY = (1 << 0),
    };
    using GpuResourceManagerFlags = uint32_t;

    struct CreateInfo {
        GpuResourceManagerFlags flags { 0 };
    };

    GpuResourceManager(Device& device, const CreateInfo& createInfo);
    ~GpuResourceManager() override;

    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;

    RenderHandleReference Get(const RenderHandle& handle) const override;

    RenderHandleReference Create(const BASE_NS::string_view name, const GpuBufferDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuBufferDesc& desc,
        const BASE_NS::array_view<const uint8_t> data) override;

    RenderHandleReference Create(const GpuBufferDesc& desc, const BASE_NS::array_view<const uint8_t> data) override;
    RenderHandleReference Create(const RenderHandleReference& replacedHandle, const GpuBufferDesc& desc) override;
    RenderHandleReference Create(const GpuBufferDesc& desc) override;

    RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc,
        const BASE_NS::array_view<const uint8_t> data) override;
    RenderHandleReference Create(const RenderHandleReference& replacedHandle, const GpuImageDesc& desc) override;
    RenderHandleReference Create(
        const BASE_NS::string_view name, const GpuImageDesc& desc, CORE_NS::IImageContainer::Ptr image) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc,
        const BASE_NS::array_view<const uint8_t> data,
        const BASE_NS::array_view<const BufferImageCopy> bufferImageCopies) override;

    /* Create a GpuImage with unique image name from external texture. Immediate resource creation not deferred. */
    RenderHandleReference CreateView(const BASE_NS::string_view name, const GpuImageDesc& desc,
        const BackendSpecificImageDesc& backendSpecificData) override;
    /* Create gpu image view for platform resource. Immediate creation not deferred. */
    RenderHandleReference CreateView(
        const BASE_NS::string_view name, const GpuImageDesc& desc, const GpuImagePlatformData& gpuImagePlatformData);

    RenderHandleReference Create(const GpuImageDesc& desc) override;
    RenderHandleReference Create(const GpuImageDesc& desc, const BASE_NS::array_view<const uint8_t> data) override;
    RenderHandleReference Create(const GpuImageDesc& desc, const BASE_NS::array_view<const uint8_t> data,
        const BASE_NS::array_view<const BufferImageCopy> bufferImageCopies) override;
    RenderHandleReference Create(const GpuImageDesc& desc, CORE_NS::IImageContainer::Ptr image) override;

    RenderHandleReference Create(const GpuAccelerationStructureDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuAccelerationStructureDesc& desc) override;
    RenderHandleReference Create(
        const RenderHandleReference& replacedHandle, const GpuAccelerationStructureDesc& desc) override;

    /** Remap gpu image handle to some valid gpu image handle. Resources are not destroyed.
    @param clientHandle A valid client image handle.
    @param clientHandleGpuResource A valid gpu image handle with a valid gpu resource.
    clientHandle resource need to be created with CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS
    */
    void RemapGpuImageHandle(const RenderHandle& clientHandle, const RenderHandle& clientHandleGpuResource);

    RenderHandleReference Create(const BASE_NS::string_view name, const GpuSamplerDesc& desc) override;
    RenderHandleReference Create(const RenderHandleReference& replacedHandle, const GpuSamplerDesc& desc) override;
    RenderHandleReference Create(const GpuSamplerDesc& desc) override;

    RenderHandleReference CreateSwapchainImage(
        const RenderHandleReference& replacedHandle, const BASE_NS::string_view name, const GpuImageDesc& desc);

    /** Does not have GPU backed data. Will be remapped to other handle */
    RenderHandleReference CreateShallowHandle(const GpuImageDesc& desc);

    RenderHandleReference GetBufferHandle(const BASE_NS::string_view name) const override;
    RenderHandleReference GetImageHandle(const BASE_NS::string_view name) const override;
    RenderHandleReference GetSamplerHandle(const BASE_NS::string_view name) const override;

    RenderHandle GetBufferRawHandle(const BASE_NS::string_view name) const;
    RenderHandle GetImageRawHandle(const BASE_NS::string_view name) const;
    RenderHandle GetSamplerRawHandle(const BASE_NS::string_view name) const;

    GpuBufferDesc GetBufferDescriptor(const RenderHandleReference& handle) const override;
    GpuImageDesc GetImageDescriptor(const RenderHandleReference& handle) const override;
    GpuSamplerDesc GetSamplerDescriptor(const RenderHandleReference& handle) const override;
    GpuAccelerationStructureDesc GetAccelerationStructureDescriptor(const RenderHandleReference& handle) const override;

    GpuBufferDesc GetBufferDescriptor(const RenderHandle& handle) const;
    GpuImageDesc GetImageDescriptor(const RenderHandle& handle) const;
    GpuSamplerDesc GetSamplerDescriptor(const RenderHandle& handle) const;
    GpuAccelerationStructureDesc GetAccelerationStructureDescriptor(const RenderHandle& handle) const;

    /** Forward allocation/deallocation requests to actual resource managers. Not thread safe.
    Called only from Renderer. */
    void HandlePendingAllocations();
    /** Called from the Renderer after the frame has been rendered with the backend. */
    void EndFrame();

    /** Special un-safe call from render backends, to re-map swapchain.
    This method is un-safe and should only be called from specific location(s). */
    void RenderBackendImmediateRemapGpuImageHandle(
        const RenderHandle& clientHandle, const RenderHandle& clientHandleGpuResource);

    /** The staging data is locked for this frame consumable sets. */
    void LockFrameStagingData();

    bool HasStagingData() const;
    StagingConsumeStruct ConsumeStagingData();

    struct StateDestroyConsumeStruct {
        // gpu image or gpu buffers
        BASE_NS::vector<RenderHandle> resources;
    };
    StateDestroyConsumeStruct ConsumeStateDestroyData();

    void* MapBuffer(const RenderHandleReference& handle) const;
    void* MapBufferMemory(const RenderHandleReference& handle) const override;
    void UnmapBuffer(const RenderHandleReference& handle) const override;

    void* MapBuffer(const RenderHandle& handle) const;
    void* MapBufferMemory(const RenderHandle& handle) const;
    void UnmapBuffer(const RenderHandle& handle) const;

    void WaitForIdleAndDestroyGpuResources() override;

    EngineResourceHandle GetGpuHandle(const RenderHandle& clientHandle) const;

    GpuBuffer* GetBuffer(const RenderHandle& gpuHandle) const;
    GpuImage* GetImage(const RenderHandle& gpuHandle) const;
    GpuSampler* GetSampler(const RenderHandle& gpuHandle) const;

    template<typename T>
    T* GetBuffer(const RenderHandle& handle) const
    {
        return static_cast<T*>(GetBuffer(handle));
    }
    template<typename T>
    T* GetImage(const RenderHandle& handle) const
    {
        return static_cast<T*>(GetImage(handle));
    }
    template<typename T>
    T* GetSampler(const RenderHandle& handle) const
    {
        return static_cast<T*>(GetSampler(handle));
    }

    // for render graph to resizing (non-locked access only valid in certain positions)
    uint32_t GetBufferHandleCount() const;
    uint32_t GetImageHandleCount() const;

    bool IsGpuBuffer(const RenderHandleReference& handle) const override;
    bool IsGpuImage(const RenderHandleReference& handle) const override;
    bool IsGpuSampler(const RenderHandleReference& handle) const override;
    bool IsGpuAccelerationStructure(const RenderHandleReference& handle) const override;
    bool IsSwapchain(const RenderHandleReference& handle) const override;
    bool IsMappableOutsideRender(const RenderHandleReference& handle) const override;
    bool IsGpuBuffer(const RenderHandle& handle) const;
    bool IsGpuImage(const RenderHandle& handle) const;
    bool IsGpuSampler(const RenderHandle& handle) const;
    bool IsGpuAccelerationStructure(const RenderHandle& handle) const;
    bool IsSwapchain(const RenderHandle& handle) const;
    bool IsValid(const RenderHandle& handle) const;

    FormatProperties GetFormatProperties(const RenderHandleReference& handle) const override;
    FormatProperties GetFormatProperties(const RenderHandle& handle) const;
    FormatProperties GetFormatProperties(const BASE_NS::Format format) const override;

    GpuImageDesc CreateGpuImageDesc(const CORE_NS::IImageContainer::ImageDesc& desc) const override;

    BASE_NS::string GetName(const RenderHandle& handle) const;
    BASE_NS::string GetName(const RenderHandleReference& handle) const override;

    BASE_NS::vector<RenderHandleReference> GetGpuBufferHandles() const override;
    BASE_NS::vector<RenderHandleReference> GetGpuImageHandles() const override;
    BASE_NS::vector<RenderHandleReference> GetGpuSamplerHandles() const override;

    void SetDefaultGpuBufferCreationFlags(const BufferUsageFlags usageFlags) override;
    void SetDefaultGpuImageCreationFlags(const ImageUsageFlags usageFlags) override;

    IGpuResourceCache& GetGpuResourceCache() const override;

    ImageAspectFlags GetImageAspectFlags(const RenderHandle& handle) const;
    ImageAspectFlags GetImageAspectFlags(const RenderHandleReference& handle) const override;
    ImageAspectFlags GetImageAspectFlags(const BASE_NS::Format format) const override;

private:
    Device& device_;

    GpuResourceManagerFlags gpuResourceMgrFlags_ { 0 };

    BASE_NS::unique_ptr<GpuResourceManagerTyped<GpuBuffer, GpuBufferDesc>> gpuBufferMgr_;
    BASE_NS::unique_ptr<GpuResourceManagerTyped<GpuImage, GpuImageDesc>> gpuImageMgr_;
    BASE_NS::unique_ptr<GpuResourceManagerTyped<GpuSampler, GpuSamplerDesc>> gpuSamplerMgr_;

    BASE_NS::unique_ptr<GpuResourceCache> gpuResourceCache_;

    union ResourceDescriptor {
        explicit ResourceDescriptor(const GpuBufferDesc& descriptor) : combinedBufDescriptor { {}, descriptor } {}
        explicit ResourceDescriptor(const GpuImageDesc& descriptor) : imageDescriptor(descriptor) {}
        explicit ResourceDescriptor(const GpuSamplerDesc& descriptor) : samplerDescriptor(descriptor) {}
        explicit ResourceDescriptor(const GpuAccelerationStructureDesc& descriptor) : combinedBufDescriptor(descriptor)
        {}
        // used for GpuBufferDesc as well
        GpuAccelerationStructureDesc combinedBufDescriptor;
        GpuImageDesc imageDescriptor;
        GpuSamplerDesc samplerDescriptor;
    };

    // combine alloc and de-alloc
    // removed is a handle that is removed before pending allocation is called (i.e. destroyed)
    enum class AllocType : uint8_t {
        UNDEFINED = 0,
        ALLOC = 1,
        DEALLOC = 2,
        REMOVED = 3,
    };

    struct OperationDescription {
        OperationDescription(
            RenderHandle handle, const ResourceDescriptor& descriptor, AllocType allocType, uint32_t optResourceIndex)
            : handle(handle), descriptor(descriptor), allocType(allocType), optionalResourceIndex(optResourceIndex),
              optionalStagingVectorIndex(~0u), optionalStagingCopyType(StagingCopyStruct::CopyType::UNDEFINED)
        {}

        RenderHandle handle;
        ResourceDescriptor descriptor;
        AllocType allocType { AllocType::UNDEFINED };
        uint32_t optionalResourceIndex { ~0u };
        uint32_t optionalStagingVectorIndex { ~0u };
        StagingCopyStruct::CopyType optionalStagingCopyType { StagingCopyStruct::CopyType::UNDEFINED };
    };
    struct RemapDescription {
        RenderHandle shallowClientHandle;
        RenderHandle resourceClientHandle;
    };
    using BufferVector = BASE_NS::vector<BASE_NS::unique_ptr<GpuBuffer>>;
    using ImageVector = BASE_NS::vector<BASE_NS::unique_ptr<GpuImage>>;

    struct PendingData {
        BASE_NS::vector<OperationDescription> allocations;
        // optional
        BASE_NS::vector<BASE_NS::unique_ptr<GpuBuffer>> buffers; // pre-created
        BASE_NS::vector<BASE_NS::unique_ptr<GpuImage>> images;   // pre-created
        BASE_NS::vector<RemapDescription> remaps;
    };

    mutable std::mutex allocationMutex_;
    struct AdditionalData {
        uintptr_t resourcePtr { 0 };
        uint32_t indexToPendingData { ~0u }; // cleared to default when processed
    };

    struct PerManagerStore {
        const RenderHandleType handleType { RenderHandleType::UNDEFINED };
        GpuResourceManagerBase* mgr { nullptr };

        // client access lock
        mutable std::shared_mutex clientMutex {};

        // the following needs be locked with clientMutex
        BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToClientIndex {};
        // resource descriptions, accessed with RenderHandle array index
        BASE_NS::vector<ResourceDescriptor> descriptions {};

        // handles might be invalid (when invalid generation is stored in high-bits, i.e. the handle is invalid)
        BASE_NS::vector<RenderHandleReference> clientHandles {};

        // index to pending data, ptr to possible gpu buffer mapped data
        BASE_NS::vector<AdditionalData> additionalData {};

        // handle ids which can be re-used
        BASE_NS::vector<uint64_t> availableHandleIds {};

        PendingData pendingData {};

        // the following are not locked (ever)

        // handles might be invalid (when invalid generation is stored in high - bits, i.e.the handle is invalid)
        BASE_NS::vector<EngineResourceHandle> gpuHandles {};
        // ogl object name tagging not supported ATM
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
        BASE_NS::vector<RenderHandle> debugTagAllocations {};
#endif
    };
    PerManagerStore bufferStore_ { RenderHandleType::GPU_BUFFER };
    PerManagerStore imageStore_ { RenderHandleType::GPU_IMAGE };
    PerManagerStore samplerStore_ { RenderHandleType::GPU_SAMPLER };

    // needs to be locked when called
    static uint64_t GetNextAvailableHandleId(PerManagerStore& mgrStore);

    struct StoreAllocationData {
        RenderHandleReference handle;
        size_t allocationIndex { ~0u };
    };
    struct StoreAllocationInfo {
        ResourceDescriptor descriptor;
        BASE_NS::string_view name;
        RenderHandle replacedHandle {};
        RenderHandleType type {};
        uint32_t optResourceIndex { ~0u };
        uint32_t addHandleFlags { 0u };
        AllocType allocType { AllocType::ALLOC };
    };

    void HandlePendingAllocationsImpl(const bool isEndFrame);
    void Destroy(const RenderHandle& handle);
    // Destroy staging buffers. Not thread safe. Called from gpu resource manager
    void DestroyFrameStaging();

    // needs to be locked when called
    StoreAllocationData CreateBuffer(
        const BASE_NS::string_view name, const RenderHandle& replacedHandle, const GpuBufferDesc& desc);
    // needs to be locked when called
    StoreAllocationData CreateImage(
        const BASE_NS::string_view name, const RenderHandle& replacedHandle, const GpuImageDesc& desc);
    RenderHandleReference CreateStagingBuffer(const GpuBufferDesc& desc);
    // needs to be locked when called
    StoreAllocationData CreateAccelerationStructure(
        const BASE_NS::string_view name, const RenderHandle& replacedHandle, const GpuAccelerationStructureDesc& desc);

    RenderHandle CreateClientHandle(const RenderHandleType type, const ResourceDescriptor& resourceDescriptor,
        const uint64_t handleId, const uint32_t hasNameId, const uint32_t additionalInfoFlags);

    // needs to be locked when called
    StoreAllocationData StoreAllocation(PerManagerStore& store, const StoreAllocationInfo& info);

    void CreateGpuResource(const OperationDescription& op, const uint32_t arrayIndex,
        const RenderHandleType resourceType, const uintptr_t preCreateData);

    // needs to be locked when called
    void DestroyGpuResource(const OperationDescription& operation, const uint32_t arrayIndex,
        const RenderHandleType resourceType, PerManagerStore& store);

    RenderHandleReference GetHandle(const PerManagerStore& store, const BASE_NS::string_view name) const;
    RenderHandle GetRawHandle(const PerManagerStore& store, const BASE_NS::string_view name) const;

    EngineResourceHandle GetGpuHandle(const PerManagerStore& store, const RenderHandle& clientHandle) const;

    // destroydHandle store needs to be locked, staging locked inside and bufferstore if not already locked
    void RemoveStagingOperations(const OperationDescription& destroyAlloc);
    // needs to be locked outside
    void Destroy(PerManagerStore& store, const RenderHandle& handle);
    // needs to be locked when called
    void DestroyImmediate(PerManagerStore& store, const RenderHandle& handle);

    void HandlePendingRemappings(const BASE_NS::vector<RemapDescription>& pendingShallowRemappings,
        BASE_NS::vector<EngineResourceHandle>& gpuHandles);

    PendingData CommitPendingData(PerManagerStore& store);

    // needs to be locked when called
    uint32_t GetPendingOptionalResourceIndex(
        const PerManagerStore& store, const RenderHandle& handle, const BASE_NS::string_view name);

    // locked inside
    BASE_NS::vector<RenderHandleReference> GetHandles(const PerManagerStore& store) const;

    void DebugPrintValidCounts();

    // list of handles that state data should be destroyed
    // (e.g. handle is replaced with a new resource or destroyed)
    // information is passed to render graph
    StateDestroyConsumeStruct clientHandleStateDestroy_;

    mutable std::mutex stagingMutex_;
    StagingConsumeStruct stagingOperations_;   // needs to be locked
    StagingConsumeStruct perFrameStagingData_; // per frame data after LockFrameStagingData()
    BASE_NS::vector<RenderHandleReference> perFrameStagingBuffers_;
    BASE_NS::vector<RenderHandleReference> perFrameStagingScalingImages_;

    // combined with bitwise OR buffer usage flags
    BufferUsageFlags defaultBufferUsageFlags_ { 0u };
    // combined with bitwise OR image usage flags
    ImageUsageFlags defaultImageUsageFlags_ { 0u };

    // ogl object name tagging not supported ATM
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    void ProcessDebugTags();
#endif
};

class RenderNodeGpuResourceManager final : public IRenderNodeGpuResourceManager {
public:
    explicit RenderNodeGpuResourceManager(GpuResourceManager& gpuResourceManager);
    ~RenderNodeGpuResourceManager();

    RenderHandleReference Get(const RenderHandle& handle) const override;

    RenderHandleReference Create(const GpuBufferDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuBufferDesc& desc) override;
    RenderHandleReference Create(const RenderHandleReference& handle, const GpuBufferDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuBufferDesc& desc,
        const BASE_NS::array_view<const uint8_t> data) override;

    RenderHandleReference Create(const GpuImageDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc) override;
    RenderHandleReference Create(const RenderHandleReference& handle, const GpuImageDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc,
        const BASE_NS::array_view<const uint8_t> data) override;

    RenderHandleReference Create(const GpuSamplerDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuSamplerDesc& desc) override;
    RenderHandleReference Create(const RenderHandleReference& handle, const GpuSamplerDesc& desc) override;

    RenderHandleReference Create(const GpuAccelerationStructureDesc& desc) override;
    RenderHandleReference Create(const BASE_NS::string_view name, const GpuAccelerationStructureDesc& desc) override;
    RenderHandleReference Create(
        const RenderHandleReference& handle, const GpuAccelerationStructureDesc& desc) override;

    RenderHandle GetBufferHandle(const BASE_NS::string_view name) const override;
    RenderHandle GetImageHandle(const BASE_NS::string_view name) const override;
    RenderHandle GetSamplerHandle(const BASE_NS::string_view name) const override;

    GpuBufferDesc GetBufferDescriptor(const RenderHandle& handle) const override;
    GpuImageDesc GetImageDescriptor(const RenderHandle& handle) const override;
    GpuSamplerDesc GetSamplerDescriptor(const RenderHandle& handle) const override;
    GpuAccelerationStructureDesc GetAccelerationStructureDescriptor(const RenderHandle& handle) const override;

    bool HasStagingData() const;
    StagingConsumeStruct ConsumeStagingData();

    void* MapBuffer(const RenderHandle& handle) const override;
    void* MapBufferMemory(const RenderHandle& handle) const override;
    void UnmapBuffer(const RenderHandle& handle) const override;

    GpuResourceManager& GetGpuResourceManager();

    bool IsValid(const RenderHandle& handle) const override;
    bool IsGpuBuffer(const RenderHandle& handle) const override;
    bool IsGpuImage(const RenderHandle& handle) const override;
    bool IsGpuSampler(const RenderHandle& handle) const override;
    bool IsGpuAccelerationStructure(const RenderHandle& handle) const override;
    bool IsSwapchain(const RenderHandle& handle) const override;

    FormatProperties GetFormatProperties(const RenderHandle& handle) const override;
    FormatProperties GetFormatProperties(const BASE_NS::Format format) const override;

    BASE_NS::string GetName(const RenderHandle& handle) const override;

    IGpuResourceCache& GetGpuResourceCache() const override;

    ImageAspectFlags GetImageAspectFlags(const RenderHandle& handle) const override;
    ImageAspectFlags GetImageAspectFlags(const BASE_NS::Format format) const override;

private:
    GpuResourceManager& gpuResourceMgr_;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_RESOURCE_MANAGER_H
