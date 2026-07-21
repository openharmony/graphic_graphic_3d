// $Copyright
//
// This header is generated from the Maleoon JSON Registry.
//

#pragma once

#include <maleoon/maleoon_defines.h>
#include <maleoon/maleoon_enums.h>
#include <maleoon/maleoon_types.h>
#include <maleoon/maleoon_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create new device.
 */
MLNAPI_ATTR MlnDevice MLNAPI_CALL
MlnCreateDevice(const MlnDeviceDescriptor* descriptor);

/**
 * @brief Get device-specific function pointer.
 */
MLNAPI_ATTR PFN_MlnVoidFunction MLNAPI_CALL
MlnGetDeviceProcAddr(MlnDevice device, const char* name);

/**
 * @brief Get Gpu-specific features.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetGpuFeatures(MlnDevice device,
                                                    MlnGpuFeatures* features);

/**
 * @brief Get Gpu-specific properties.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnGetGpuProperties(MlnDevice device, MlnGpuProperties* properties);

/**
 * @brief Get Gpu-specific format properties.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetGpuFormatProperties(
    MlnDevice device, MlnFormat format, MlnGpuFormatProperties* properties);

/**
 * @brief Get Gpu-specific image format properties.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetGpuImageFormatProperties(
    MlnDevice device, MlnFormat format, MlnImageType type,
    MlnImageTiling tiling, MlnImageUsageFlags usage,
    MlnImageDescriptorFlags flags, MlnGpuImageFormatProperties* properties);

/**
 * @brief Wait on the host for the completion of outstanding queue operations
 * for all queues on a given device.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnDeviceWaitIdle(MlnDevice device);

/**
 * @brief Destroy device.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyDevice(MlnDevice device);

/**
 * @brief Get available shading rates for GPU, and get GPU-specific features and
 * properties related to fragment shading rate.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetGpuFragmentShadingRates(
    MlnDevice device, u32* pFragmentShadingRateCount,
    MlnGpuFragmentShadingRate* pFragmentShadingRates,
    MlnGpuFragmentShadingRateFeatures* pFragmentShadingRateFeatures,
    MlnGpuFragmentShadingRateProperties* pFragmentShadingRateProperties);

/**
 * @brief Get a queue handle from device.
 */
MLNAPI_ATTR MlnQueue MLNAPI_CALL MlnGetDeviceQueue(MlnDevice device, u32 index);

/**
 * @brief Submit tasks to a queue.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnQueueSubmit(MlnQueue queue, const MlnSubmitDescriptor* descriptor);

/**
 * @brief Wait on the host for the completion of outstanding queue operations
 * for a given queue.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnQueueWaitIdle(MlnQueue queue);

/**
 * @brief Set the queue priority.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnQueueSetPriority(MlnQueue queue,
                                                      MlnQueuePriority priority,
                                                      f32 adjust);

/**
 * @brief Get the queue priority.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnQueueGetPriority(MlnQueue queue, MlnQueuePriority* priority, f32* adjust);

/**
 * @brief Get Gpu-specific memory properties.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnGetGpuMemoryProperties(MlnDevice device, MlnGpuMemoryProperties* properties);

/**
 * @brief Get memory requirements of a resource.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnGetResourceMemoryRequirements(
    MlnDevice device, const MlnResourceMemoryRequirementsDescriptor* descriptor,
    MlnMemoryRequirements* memoryRequirements);

/**
 * @brief Allocate device memory object.
 */
MLNAPI_ATTR MlnDeviceMemory MLNAPI_CALL MlnAllocateMemory(
    MlnDevice device, const MlnMemoryAllocateDescriptor* descriptor);

/**
 * @brief Free device memory object.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnFreeMemory(MlnDevice device,
                                           MlnDeviceMemory memory);

/**
 * @brief Map device memory object.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnMapMemory(
    MlnDevice device, const MlnMemoryMapDescriptor* descriptor, void** data);

/**
 * @brief Unmap device memory object.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnUnmapMemory(MlnDevice device, const MlnMemoryUnmapDescriptor* descriptor);

/**
 * @brief Flush device memory range.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnFlushMappedMemoryRanges(MlnDevice device, u32 memoryRangeCount,
                           const MlnMappedMemoryRangeDescriptor* memoryRanges);

/**
 * @brief Invalidate device memory range.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnInvalidateMappedMemoryRanges(
    MlnDevice device, u32 memoryRangeCount,
    const MlnMappedMemoryRangeDescriptor* memoryRanges);

/**
 * @brief Get native buffer properties.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnGetNativeBufferProperties(MlnDevice device, const OH_NativeBuffer* buffer,
                             MlnNativeBufferProperties* properties);

/**
 * @brief Get native buffer.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetMemoryNativeBuffer(
    MlnDevice device, const MlnMemoryGetNativeBufferDescriptor* descriptor,
    OH_NativeBuffer** pBuffer);

/**
 * @brief Get buffer memory requirements of a resource.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnGetBufferResourceMemoryRequirements(
    MlnDevice device, const MlnBufferDescriptor* descriptor,
    MlnMemoryRequirements* memoryRequirements);

/**
 * @brief Get image memory requirements of a resource.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnGetImageResourceMemoryRequirements(
    MlnDevice device, const MlnImageDescriptor* descriptor,
    MlnMemoryRequirements* memoryRequirements);

/**
 * @brief Create new buffer resource.
 */
MLNAPI_ATTR MlnResource MLNAPI_CALL MlnCreateBufferResource(
    MlnDevice device, const MlnBufferDescriptor* descriptor);

/**
 * @brief Create new image resource.
 */
MLNAPI_ATTR MlnResource MLNAPI_CALL
MlnCreateImageResource(MlnDevice device, const MlnImageDescriptor* descriptor);

/**
 * @brief Attach memory to a resource.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnBindResourceMemory(
    MlnDevice device, const MlnBindResourceMemoryDescriptor* descriptor);

/**
 * @brief Destroy resource.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyResource(MlnDevice device,
                                                MlnResource resource);

/**
 * @brief Create new sampler.
 */
MLNAPI_ATTR MlnSampler MLNAPI_CALL
MlnCreateSampler(MlnDevice device, const MlnSamplerDescriptor* descriptor);

/**
 * @brief Destroy sampler.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroySampler(MlnDevice device,
                                               MlnSampler sampler);

/**
 * @brief Create new sampler YCbCr conversion.
 */
MLNAPI_ATTR MlnSamplerYcbcrConversion MLNAPI_CALL
MlnCreateSamplerYcbcrConversion(
    MlnDevice device, const MlnSamplerYcbcrConversionDescriptor* descriptor);

/**
 * @brief Destroy sampler YCbCr conversion.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroySamplerYcbcrConversion(
    MlnDevice device, MlnSamplerYcbcrConversion ycbcrConversion);

/**
 * @brief Get buffer resource address.
 */
MLNAPI_ATTR MlnDeviceAddress MLNAPI_CALL
MlnGetBufferResourceAddress(MlnDevice device, MlnResource resource);

/**
 * @brief Create new buffer resource view.
 */
MLNAPI_ATTR MlnResourceView MLNAPI_CALL MlnCreateBufferResourceView(
    MlnDevice device, const MlnBufferViewDescriptor* descriptor);

/**
 * @brief Create new image resource view.
 */
MLNAPI_ATTR MlnResourceView MLNAPI_CALL MlnCreateImageResourceView(
    MlnDevice device, const MlnImageViewDescriptor* descriptor);

/**
 * @brief Destroy resource view.
 */
MLNAPI_ATTR void MLNAPI_CALL
MlnDestroyResourceView(MlnDevice device, MlnResourceView resourceView);

/**
 * @brief Create new program cache.
 */
MLNAPI_ATTR MlnProgramCache MLNAPI_CALL MlnCreateProgramCache(
    MlnDevice device, const MlnProgramCacheDescriptor* descriptor);

/**
 * @brief Get program cache data.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnGetProgramCacheData(MlnDevice device, MlnProgramCache programCache,
                       size_t* cacheSize, void* cache);

/**
 * @brief Merge program cache data.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnMergeProgramCaches(MlnDevice device, MlnProgramCache dstCache,
                      u32 srcCacheCount, const MlnProgramCache* srcCaches);

/**
 * @brief Destroy program cache.
 */
MLNAPI_ATTR void MLNAPI_CALL
MlnDestroyProgramCache(MlnDevice device, MlnProgramCache programCache);

/**
 * @brief Create new program interface.
 */
MLNAPI_ATTR MlnProgramInterface MLNAPI_CALL MlnCreateProgramInterface(
    MlnDevice device, const MlnProgramInterfaceDescriptor* descriptor);

/**
 * @brief Destroy program interface.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyProgramInterface(
    MlnDevice device, MlnProgramInterface programInterface);

/**
 * @brief Create new graphics program.
 */
MLNAPI_ATTR MlnProgram MLNAPI_CALL
MlnCreateGraphicsProgram(MlnDevice device, MlnProgramCache programCache,
                         const MlnGraphicsProgramState* state);

/**
 * @brief Create new compute program.
 */
MLNAPI_ATTR MlnProgram MLNAPI_CALL
MlnCreateComputeProgram(MlnDevice device, MlnProgramCache programCache,
                        const MlnComputeProgramState* state);

/**
 * @brief Create new graphics program asynchronously.
 */
MLNAPI_ATTR MlnProgram MLNAPI_CALL MlnCreateGraphicsProgramAsync(
    MlnDevice device, MlnProgramCache programCache,
    const MlnGraphicsProgramState* state, MlnPriority priority);

/**
 * @brief Create new compute program asynchronously.
 */
MLNAPI_ATTR MlnProgram MLNAPI_CALL MlnCreateComputeProgramAsync(
    MlnDevice device, MlnProgramCache programCache,
    const MlnComputeProgramState* state, MlnPriority priority);

/**
 * @brief Get async program's compilation result before using this program.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnGetCreatedProgramAsyncResult(MlnDevice device, MlnProgram program);

/**
 * @brief Wait for async program's compilation result until it is completed.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnWaitCreatedProgramAsyncResult(MlnDevice device, MlnProgram program);

/**
 * @brief Destroy program.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyProgram(MlnDevice device,
                                               MlnProgram program);

/**
 * @brief Create new binding layout.
 */
MLNAPI_ATTR MlnBindingLayout MLNAPI_CALL MlnCreateBindingLayout(
    MlnDevice device, const MlnBindingLayoutDescriptor* descriptor);

/**
 * @brief Destroy binding layout.
 */
MLNAPI_ATTR void MLNAPI_CALL
MlnDestroyBindingLayout(MlnDevice device, MlnBindingLayout bindingLayout);

/**
 * @brief Create new binding set.
 */
MLNAPI_ATTR MlnBindingSet MLNAPI_CALL MlnCreateBindingSet(
    MlnDevice device, MlnBindingLayout bindingLayout, u32 variableBindingSize);

/**
 * @brief Update binding set with a combination of write and copy operations.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnUpdateBindingSets(
    MlnDevice device, u32 bindingWriteCount,
    const MlnWriteBindingSet* bindingWrites, u32 bindingCopyCount,
    const MlnCopyBindingSet* bindingCopies);

/**
 * @brief Destroy binding set.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyBindingSet(MlnDevice device,
                                                  MlnBindingSet bindingSet);

/**
 * @brief Create new graphics object group.
 */
MLNAPI_ATTR MlnObjectGroup MLNAPI_CALL MlnCreateGraphicsObjectGroup(
    MlnDevice device, const MlnGraphicsObjectGroupDescriptor* descriptor);

/**
 * @brief Update graphics object group.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnUpdateGraphicsObjectGroup(
    MlnDevice device, MlnObjectGroup objectGroup,
    const MlnGraphicsObjectGroupUpdateDescriptor* descriptor);

/**
 * @brief Create new compute object group.
 */
MLNAPI_ATTR MlnObjectGroup MLNAPI_CALL MlnCreateComputeObjectGroup(
    MlnDevice device, const MlnComputeObjectGroupDescriptor* descriptor);

/**
 * @brief Update compute object group.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnUpdateComputeObjectGroup(
    MlnDevice device, MlnObjectGroup objectGroup,
    const MlnComputeObjectGroupUpdateDescriptor* descriptor);

/**
 * @brief Create new transfer object group.
 */
MLNAPI_ATTR MlnObjectGroup MLNAPI_CALL MlnCreateTransferObjectGroup(
    MlnDevice device, const MlnTransferObjectGroupDescriptor* descriptor);

/**
 * @brief Create new clear object group.
 */
MLNAPI_ATTR MlnObjectGroup MLNAPI_CALL MlnCreateClearObjectGroup(
    MlnDevice device, const MlnClearObjectGroupDescriptor* descriptor);

/**
 * @brief Create new barrier object group.
 */
MLNAPI_ATTR MlnObjectGroup MLNAPI_CALL MlnCreateBarrierObjectGroup(
    MlnDevice device, const MlnBarrierObjectGroupDescriptor* descriptor);

/**
 * @brief Destroy object group.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyObjectGroup(MlnDevice device,
                                                   MlnObjectGroup objectGroup);

/**
 * @brief Create new graphics data graph.
 */
MLNAPI_ATTR MlnDataGraph MLNAPI_CALL MlnCreateGraphicsDataGraph(
    MlnDevice device, const MlnGraphicsDataGraphDescriptor* descriptor);

/**
 * @brief Create new compute data graph.
 */
MLNAPI_ATTR MlnDataGraph MLNAPI_CALL MlnCreateComputeDataGraph(
    MlnDevice device, const MlnComputeDataGraphDescriptor* descriptor);

/**
 * @brief Create new transfer data graph.
 */
MLNAPI_ATTR MlnDataGraph MLNAPI_CALL MlnCreateTransferDataGraph(
    MlnDevice device, const MlnTransferDataGraphDescriptor* descriptor);

/**
 * @brief Destroy data graph.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyDataGraph(MlnDevice device,
                                                 MlnDataGraph dataGraph);

/**
 * @brief Create new scheduling graph.
 */
MLNAPI_ATTR MlnSchedulingGraph MLNAPI_CALL MlnCreateSchedulingGraph(
    MlnDevice device, const MlnSchedulingGraphDescriptor* descriptor);

/**
 * @brief Destroy scheduling graph.
 */
MLNAPI_ATTR void MLNAPI_CALL
MlnDestroySchedulingGraph(MlnDevice device, MlnSchedulingGraph schedulingGraph);

/**
 * @brief Create new timeline.
 */
MLNAPI_ATTR MlnTimeline MLNAPI_CALL
MlnCreateTimeline(MlnDevice device, const MlnTimelineDescriptor* descriptor);

/**
 * @brief Signal timelines with particular counter value on the host.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnSignalTimelines(
    MlnDevice device, const MlnTimelineSignalDescriptor* descriptor);

/**
 * @brief Wait for timeline to reach particular counter values on the host.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnWaitForTimelines(
    MlnDevice device, const MlnTimelineWaitDescriptor* descriptor, u64 timeout);

/**
 * @brief Query the current counter value of a timeline.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetTimelineValue(MlnDevice device,
                                                      MlnTimeline timeline,
                                                      u64* value);

/**
 * @brief Export a POSIX file descriptor representing the payload of a timeline.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnExportTimelineFd(
    MlnDevice device, const MlnTimelineExportFdDescriptor* descriptor, int* fd);

/**
 * @brief Import a timeline payload from a POSIX file descriptor.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnImportTimelineFd(
    MlnDevice device, const MlnTimelineImportFdDescriptor* descriptor);

/**
 * @brief Import an event id to timeline for HTS or FFTS synchronization.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnImportTimelineEventId(
    MlnDevice device, const MlnTimelineImportEventIdDescriptor* descriptor);

/**
 * @brief Reset a timeline to a initialValue.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnResetTimeline(MlnDevice device,
                                                   MlnTimeline timeline,
                                                   u64 initialValue);

/**
 * @brief Destroy timeline.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyTimeline(MlnDevice device,
                                                MlnTimeline timeline);

/**
 * @brief Create new render target.
 */
MLNAPI_ATTR MlnRenderTarget MLNAPI_CALL MlnCreateRenderTarget(
    MlnDevice device, const MlnRenderTargetDescriptor* descriptor);

/**
 * @brief Destroy render target.
 */
MLNAPI_ATTR void MLNAPI_CALL
MlnDestroyRenderTarget(MlnDevice device, MlnRenderTarget renderTarget);

/**
 * @brief Create new display surface.
 */
MLNAPI_ATTR MlnDisplaySurface MLNAPI_CALL MlnCreateDisplaySurface(
    MlnDevice device, const MlnDisplaySurfaceDescriptor* descriptor);

/**
 * @brief Get display surface capabilities.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetDisplaySurfaceCapabilities(
    MlnDevice device, MlnDisplaySurface displaySurface,
    MlnDisplaySurfaceCapabilities* capabilities);

/**
 * @brief Get display surface formats supported.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnGetDisplaySurfaceFormats(MlnDevice device, MlnDisplaySurface displaySurface,
                            u32* formatCount, MlnDisplaySurfaceFormat* formats);

/**
 * @brief Get display surface present mode.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetDisplaySurfacePresentModes(
    MlnDevice device, MlnDisplaySurface displaySurface, u32* presentModeCount,
    MlnPresentMode* presentModes);

/**
 * @brief Destroy display surface.
 */
MLNAPI_ATTR void MLNAPI_CALL
MlnDestroyDisplaySurface(MlnDevice device, MlnDisplaySurface displaySurface);

/**
 * @brief Create new swapchain.
 */
MLNAPI_ATTR MlnSwapchain MLNAPI_CALL
MlnCreateSwapchain(MlnDevice device, const MlnSwapchainDescriptor* descriptor);

/**
 * @brief Acquire next image from swapchain.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnAcquireNextImage(MlnDevice device, MlnSwapchain swapchain, u64 timeout,
                    MlnTimeline timeline, u64 timelineValue, u32* imageIndex);

/**
 * @brief Get all images from a swapchain.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL
MlnGetSwapchainImages(MlnDevice device, MlnSwapchain swapchain, u32* imageCount,
                      MlnResource* imageResources);

/**
 * @brief Destroy swapchain.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroySwapchain(MlnDevice device,
                                                 MlnSwapchain swapchain);

/**
 * @brief Get swapchain usage, internal use, it is not exposed to applications.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetSwapchainGrallocUsageOHOS(
    MlnDevice device, MlnFormat format, MlnImageUsageFlags imageUsage,
    u64* grallocUsage);

/**
 * @brief Acquire image, internal use, it is not exposed to applications.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnAcquireImageOHOS(MlnDevice device,
                                                      MlnResource resource,
                                                      s32 nativeFenceFd,
                                                      MlnTimeline timeline,
                                                      u64 timelineValue);

/**
 * @brief Present swapchain image.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnQueuePresentSwapchain(
    MlnQueue queue, const MlnPresentDescriptor* descriptor);

/**
 * @brief Queue signal release image, internal use, it is not exposed to
 * applications.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnQueueSignalReleaseImageOHOS(
    MlnQueue queue, u32 waitCount, const MlnTimeline* timeline,
    const u64* waitValues, MlnResource resource, s32* nativeFenceFd);

/**
 * @brief Create a private data slot.
 */
MLNAPI_ATTR MlnPrivateDataSlot MLNAPI_CALL MlnCreatePrivateDataSlot(
    MlnDevice device, const MlnPrivateDataSlotDescriptor* descriptor);

/**
 * @brief Destroy a private data slot.
 */
MLNAPI_ATTR void MLNAPI_CALL
MlnDestroyPrivateDataSlot(MlnDevice device, MlnPrivateDataSlot privateDataSlot);

/**
 * @brief Set private data on an object.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnSetPrivateData(
    MlnDevice device, const MlnPrivateDataSetDescriptor* descriptor);

/**
 * @brief Get private data from an object.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetPrivateData(
    MlnDevice device, const MlnPrivateDataGetDescriptor* descriptor);

/**
 * @brief Create acceleration structure object group.
 */
MLNAPI_ATTR MlnObjectGroup MLNAPI_CALL
MlnCreateAccelerationStructureObjectGroup(
    MlnDevice device,
    const MlnAccelerationStructureObjectGroupDescriptor* descriptor);

/**
 * @brief Create acceleration structure data graph.
 */
MLNAPI_ATTR MlnDataGraph MLNAPI_CALL MlnCreateAccelerationStructureDataGraph(
    MlnDevice device,
    const MlnAccelerationStructureDataGraphDescriptor* descriptor);

/**
 * @brief Query Acceleration structure build size.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetAccelerationStructureBuildSizes(
    MlnDevice device,
    const MlnAccelerationStructureBuildGeometryDescriptor* buildGeometry,
    const u32* maxPrimitiveCounts,
    MlnAccelerationStructureBuildSizesDescriptor* buildSizes);

/**
 * @brief Create acceleration structure.
 */
MLNAPI_ATTR MlnAccelerationStructure MLNAPI_CALL MlnCreateAccelerationStructure(
    MlnDevice device, const MlnAccelerationStructureDescriptor* descriptor);

/**
 * @brief Get acceleration structure device address.
 */
MLNAPI_ATTR MlnDeviceAddress MLNAPI_CALL
MlnGetAccelerationStructureDeviceAddress(
    MlnDevice device, MlnAccelerationStructure accelerationStructure);

/**
 * @brief Destroy acceleration structure.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyAccelerationStructure(
    MlnDevice device, MlnAccelerationStructure accelerationStructure);

/**
 * @brief Query acceleration structure limits for the device.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetRayTracingCapabilities(
    MlnDevice device, MlnRayTracingCapabilities* capabilities);

/**
 * @brief Check acceleration structure compatibility.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnCheckAccelerationStructureCompatibility(
    MlnDevice device, const MlnAccelerationStructureVersion* version,
    MlnAccelerationStructureCompatibility* compatibility);

/**
 * @brief Create query pool.
 */
MLNAPI_ATTR MlnQueryPool MLNAPI_CALL
MlnCreateQueryPool(MlnDevice device, const MlnQueryPoolDescriptor* descriptor);

/**
 * @brief Destroy query pool.
 */
MLNAPI_ATTR void MLNAPI_CALL MlnDestroyQueryPool(MlnDevice device,
                                                 MlnQueryPool queryPool);

/**
 * @brief Reset query ranges within a query pool.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnResetQueryPool(
    MlnDevice device, const MlnResetQueryPoolDescriptor* descriptor);

/**
 * @brief Get query result from query ranges within a query pool.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetQueryPoolResult(
    MlnDevice device, const MlnGetQueryPoolResultDescriptor* descriptor);

/**
 * @brief Get Gpu-specific counter properties.
 */
MLNAPI_ATTR MlnStatus MLNAPI_CALL MlnGetGpuCounterProperties(
    MlnDevice device, MlnCounter counter, MlnGpuCounterProperties* properties);

#ifdef __cplusplus
}
#endif
