// $Copyright
//
// This header is generated from the Maleoon JSON Registry.
//

#pragma once

#include <cstddef>
#include <stdint.h>
#include <vector>
#include <map>
#include <atomic>
#include <string>
#include <memory>
#include <maleoon/maleoon_platform.h>
#include <maleoon/maleoon_defines.h>
#include <maleoon/maleoon_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(MLNAPI_PTR *PFN_MlnVoidFunction)(void);

typedef MlnDevice(MLNAPI_PTR *PFN_MlnCreateDevice)(
    const MlnDeviceDescriptor *descriptor);

typedef PFN_MlnVoidFunction(MLNAPI_PTR *PFN_MlnGetDeviceProcAddr)(
    MlnDevice device, const char *name);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetGpuFeatures)(MlnDevice device,
                                                     MlnGpuFeatures *features);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetGpuProperties)(
    MlnDevice device, MlnGpuProperties *properties);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetGpuFormatProperties)(
    MlnDevice device, MlnFormat format, MlnGpuFormatProperties *properties);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetGpuImageFormatProperties)(
    MlnDevice device, MlnFormat format, MlnImageType type,
    MlnImageTiling tiling, MlnImageUsageFlags usage,
    MlnImageDescriptorFlags flags, MlnGpuImageFormatProperties *properties);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnDeviceWaitIdle)(MlnDevice device);

typedef void(MLNAPI_PTR *PFN_MlnDestroyDevice)(MlnDevice device);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetGpuFragmentShadingRates)(
    MlnDevice device, u32 *pFragmentShadingRateCount,
    MlnGpuFragmentShadingRate *pFragmentShadingRates,
    MlnGpuFragmentShadingRateFeatures *pFragmentShadingRateFeatures,
    MlnGpuFragmentShadingRateProperties *pFragmentShadingRateProperties);

typedef MlnQueue(MLNAPI_PTR *PFN_MlnGetDeviceQueue)(MlnDevice device,
                                                    u32 index);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnQueueSubmit)(
    MlnQueue queue, const MlnSubmitDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnQueueWaitIdle)(MlnQueue queue);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnQueueSetPriority)(
    MlnQueue queue, MlnQueuePriority priority, f32 adjust);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnQueueGetPriority)(
    MlnQueue queue, MlnQueuePriority *priority, f32 *adjust);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetGpuMemoryProperties)(
    MlnDevice device, MlnGpuMemoryProperties *properties);

typedef void(MLNAPI_PTR *PFN_MlnGetResourceMemoryRequirements)(
    MlnDevice device, const MlnResourceMemoryRequirementsDescriptor *descriptor,
    MlnMemoryRequirements *memoryRequirements);

typedef MlnDeviceMemory(MLNAPI_PTR *PFN_MlnAllocateMemory)(
    MlnDevice device, const MlnMemoryAllocateDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnFreeMemory)(MlnDevice device,
                                            MlnDeviceMemory memory);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnMapMemory)(
    MlnDevice device, const MlnMemoryMapDescriptor *descriptor, void **data);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnUnmapMemory)(
    MlnDevice device, const MlnMemoryUnmapDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnFlushMappedMemoryRanges)(
    MlnDevice device, u32 memoryRangeCount,
    const MlnMappedMemoryRangeDescriptor *memoryRanges);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnInvalidateMappedMemoryRanges)(
    MlnDevice device, u32 memoryRangeCount,
    const MlnMappedMemoryRangeDescriptor *memoryRanges);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetNativeBufferProperties)(
    MlnDevice device, const OH_NativeBuffer *buffer,
    MlnNativeBufferProperties *properties);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetMemoryNativeBuffer)(
    MlnDevice device, const MlnMemoryGetNativeBufferDescriptor *descriptor,
    OH_NativeBuffer **pBuffer);

typedef void(MLNAPI_PTR *PFN_MlnGetBufferResourceMemoryRequirements)(
    MlnDevice device, const MlnBufferDescriptor *descriptor,
    MlnMemoryRequirements *memoryRequirements);

typedef void(MLNAPI_PTR *PFN_MlnGetImageResourceMemoryRequirements)(
    MlnDevice device, const MlnImageDescriptor *descriptor,
    MlnMemoryRequirements *memoryRequirements);

typedef MlnResource(MLNAPI_PTR *PFN_MlnCreateBufferResource)(
    MlnDevice device, const MlnBufferDescriptor *descriptor);

typedef MlnResource(MLNAPI_PTR *PFN_MlnCreateImageResource)(
    MlnDevice device, const MlnImageDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnBindResourceMemory)(
    MlnDevice device, const MlnBindResourceMemoryDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyResource)(MlnDevice device,
                                                 MlnResource resource);

typedef MlnSampler(MLNAPI_PTR *PFN_MlnCreateSampler)(
    MlnDevice device, const MlnSamplerDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroySampler)(MlnDevice device,
                                                MlnSampler sampler);

typedef MlnSamplerYcbcrConversion(
    MLNAPI_PTR *PFN_MlnCreateSamplerYcbcrConversion)(
    MlnDevice device, const MlnSamplerYcbcrConversionDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroySamplerYcbcrConversion)(
    MlnDevice device, MlnSamplerYcbcrConversion ycbcrConversion);

typedef MlnDeviceAddress(MLNAPI_PTR *PFN_MlnGetBufferResourceAddress)(
    MlnDevice device, MlnResource resource);

typedef MlnResourceView(MLNAPI_PTR *PFN_MlnCreateBufferResourceView)(
    MlnDevice device, const MlnBufferViewDescriptor *descriptor);

typedef MlnResourceView(MLNAPI_PTR *PFN_MlnCreateImageResourceView)(
    MlnDevice device, const MlnImageViewDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyResourceView)(
    MlnDevice device, MlnResourceView resourceView);

typedef MlnProgramCache(MLNAPI_PTR *PFN_MlnCreateProgramCache)(
    MlnDevice device, const MlnProgramCacheDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetProgramCacheData)(
    MlnDevice device, MlnProgramCache programCache, size_t *cacheSize,
    void *cache);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnMergeProgramCaches)(
    MlnDevice device, MlnProgramCache dstCache, u32 srcCacheCount,
    const MlnProgramCache *srcCaches);

typedef void(MLNAPI_PTR *PFN_MlnDestroyProgramCache)(
    MlnDevice device, MlnProgramCache programCache);

typedef MlnProgramInterface(MLNAPI_PTR *PFN_MlnCreateProgramInterface)(
    MlnDevice device, const MlnProgramInterfaceDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyProgramInterface)(
    MlnDevice device, MlnProgramInterface programInterface);

typedef MlnProgram(MLNAPI_PTR *PFN_MlnCreateGraphicsProgram)(
    MlnDevice device, MlnProgramCache programCache,
    const MlnGraphicsProgramState *state);

typedef MlnProgram(MLNAPI_PTR *PFN_MlnCreateComputeProgram)(
    MlnDevice device, MlnProgramCache programCache,
    const MlnComputeProgramState *state);

typedef MlnProgram(MLNAPI_PTR *PFN_MlnCreateGraphicsProgramAsync)(
    MlnDevice device, MlnProgramCache programCache,
    const MlnGraphicsProgramState *state, MlnPriority priority);

typedef MlnProgram(MLNAPI_PTR *PFN_MlnCreateComputeProgramAsync)(
    MlnDevice device, MlnProgramCache programCache,
    const MlnComputeProgramState *state, MlnPriority priority);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetCreatedProgramAsyncResult)(
    MlnDevice device, MlnProgram program);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnWaitCreatedProgramAsyncResult)(
    MlnDevice device, MlnProgram program);

typedef void(MLNAPI_PTR *PFN_MlnDestroyProgram)(MlnDevice device,
                                                MlnProgram program);

typedef MlnBindingLayout(MLNAPI_PTR *PFN_MlnCreateBindingLayout)(
    MlnDevice device, const MlnBindingLayoutDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyBindingLayout)(
    MlnDevice device, MlnBindingLayout bindingLayout);

typedef MlnBindingSet(MLNAPI_PTR *PFN_MlnCreateBindingSet)(
    MlnDevice device, MlnBindingLayout bindingLayout, u32 variableBindingSize);

typedef void(MLNAPI_PTR *PFN_MlnUpdateBindingSets)(
    MlnDevice device, u32 bindingWriteCount,
    const MlnWriteBindingSet *bindingWrites, u32 bindingCopyCount,
    const MlnCopyBindingSet *bindingCopies);

typedef void(MLNAPI_PTR *PFN_MlnDestroyBindingSet)(MlnDevice device,
                                                   MlnBindingSet bindingSet);

typedef MlnObjectGroup(MLNAPI_PTR *PFN_MlnCreateGraphicsObjectGroup)(
    MlnDevice device, const MlnGraphicsObjectGroupDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnUpdateGraphicsObjectGroup)(
    MlnDevice device, MlnObjectGroup objectGroup,
    const MlnGraphicsObjectGroupUpdateDescriptor *descriptor);

typedef MlnObjectGroup(MLNAPI_PTR *PFN_MlnCreateComputeObjectGroup)(
    MlnDevice device, const MlnComputeObjectGroupDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnUpdateComputeObjectGroup)(
    MlnDevice device, MlnObjectGroup objectGroup,
    const MlnComputeObjectGroupUpdateDescriptor *descriptor);

typedef MlnObjectGroup(MLNAPI_PTR *PFN_MlnCreateTransferObjectGroup)(
    MlnDevice device, const MlnTransferObjectGroupDescriptor *descriptor);

typedef MlnObjectGroup(MLNAPI_PTR *PFN_MlnCreateClearObjectGroup)(
    MlnDevice device, const MlnClearObjectGroupDescriptor *descriptor);

typedef MlnObjectGroup(MLNAPI_PTR *PFN_MlnCreateBarrierObjectGroup)(
    MlnDevice device, const MlnBarrierObjectGroupDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyObjectGroup)(MlnDevice device,
                                                    MlnObjectGroup objectGroup);

typedef MlnDataGraph(MLNAPI_PTR *PFN_MlnCreateGraphicsDataGraph)(
    MlnDevice device, const MlnGraphicsDataGraphDescriptor *descriptor);

typedef MlnDataGraph(MLNAPI_PTR *PFN_MlnCreateComputeDataGraph)(
    MlnDevice device, const MlnComputeDataGraphDescriptor *descriptor);

typedef MlnDataGraph(MLNAPI_PTR *PFN_MlnCreateTransferDataGraph)(
    MlnDevice device, const MlnTransferDataGraphDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyDataGraph)(MlnDevice device,
                                                  MlnDataGraph dataGraph);

typedef MlnSchedulingGraph(MLNAPI_PTR *PFN_MlnCreateSchedulingGraph)(
    MlnDevice device, const MlnSchedulingGraphDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroySchedulingGraph)(
    MlnDevice device, MlnSchedulingGraph schedulingGraph);

typedef MlnTimeline(MLNAPI_PTR *PFN_MlnCreateTimeline)(
    MlnDevice device, const MlnTimelineDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnSignalTimelines)(
    MlnDevice device, const MlnTimelineSignalDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnWaitForTimelines)(
    MlnDevice device, const MlnTimelineWaitDescriptor *descriptor, u64 timeout);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetTimelineValue)(MlnDevice device,
                                                       MlnTimeline timeline,
                                                       u64 *value);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnExportTimelineFd)(
    MlnDevice device, const MlnTimelineExportFdDescriptor *descriptor, int *fd);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnImportTimelineFd)(
    MlnDevice device, const MlnTimelineImportFdDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnImportTimelineEventId)(
    MlnDevice device, const MlnTimelineImportEventIdDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnResetTimeline)(MlnDevice device,
                                                    MlnTimeline timeline,
                                                    u64 initialValue);

typedef void(MLNAPI_PTR *PFN_MlnDestroyTimeline)(MlnDevice device,
                                                 MlnTimeline timeline);

typedef MlnRenderTarget(MLNAPI_PTR *PFN_MlnCreateRenderTarget)(
    MlnDevice device, const MlnRenderTargetDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyRenderTarget)(
    MlnDevice device, MlnRenderTarget renderTarget);

typedef MlnDisplaySurface(MLNAPI_PTR *PFN_MlnCreateDisplaySurface)(
    MlnDevice device, const MlnDisplaySurfaceDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetDisplaySurfaceCapabilities)(
    MlnDevice device, MlnDisplaySurface displaySurface,
    MlnDisplaySurfaceCapabilities *capabilities);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetDisplaySurfaceFormats)(
    MlnDevice device, MlnDisplaySurface displaySurface, u32 *formatCount,
    MlnDisplaySurfaceFormat *formats);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetDisplaySurfacePresentModes)(
    MlnDevice device, MlnDisplaySurface displaySurface, u32 *presentModeCount,
    MlnPresentMode *presentModes);

typedef void(MLNAPI_PTR *PFN_MlnDestroyDisplaySurface)(
    MlnDevice device, MlnDisplaySurface displaySurface);

typedef MlnSwapchain(MLNAPI_PTR *PFN_MlnCreateSwapchain)(
    MlnDevice device, const MlnSwapchainDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnAcquireNextImage)(
    MlnDevice device, MlnSwapchain swapchain, u64 timeout, MlnTimeline timeline,
    u64 timelineValue, u32 *imageIndex);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetSwapchainImages)(
    MlnDevice device, MlnSwapchain swapchain, u32 *imageCount,
    MlnResource *imageResources);

typedef void(MLNAPI_PTR *PFN_MlnDestroySwapchain)(MlnDevice device,
                                                  MlnSwapchain swapchain);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetSwapchainGrallocUsageOHOS)(
    MlnDevice device, MlnFormat format, MlnImageUsageFlags imageUsage,
    u64 *grallocUsage);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnAcquireImageOHOS)(MlnDevice device,
                                                       MlnResource resource,
                                                       s32 nativeFenceFd,
                                                       MlnTimeline timeline,
                                                       u64 timelineValue);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnQueuePresentSwapchain)(
    MlnQueue queue, const MlnPresentDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnQueueSignalReleaseImageOHOS)(
    MlnQueue queue, u32 waitCount, const MlnTimeline *timeline,
    const u64 *waitValues, MlnResource resource, s32 *nativeFenceFd);

typedef MlnPrivateDataSlot(MLNAPI_PTR *PFN_MlnCreatePrivateDataSlot)(
    MlnDevice device, const MlnPrivateDataSlotDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyPrivateDataSlot)(
    MlnDevice device, MlnPrivateDataSlot privateDataSlot);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnSetPrivateData)(
    MlnDevice device, const MlnPrivateDataSetDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetPrivateData)(
    MlnDevice device, const MlnPrivateDataGetDescriptor *descriptor);

typedef MlnObjectGroup(
    MLNAPI_PTR *PFN_MlnCreateAccelerationStructureObjectGroup)(
    MlnDevice device,
    const MlnAccelerationStructureObjectGroupDescriptor *descriptor);

typedef MlnDataGraph(MLNAPI_PTR *PFN_MlnCreateAccelerationStructureDataGraph)(
    MlnDevice device,
    const MlnAccelerationStructureDataGraphDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetAccelerationStructureBuildSizes)(
    MlnDevice device,
    const MlnAccelerationStructureBuildGeometryDescriptor *buildGeometry,
    const u32 *maxPrimitiveCounts,
    MlnAccelerationStructureBuildSizesDescriptor *buildSizes);

typedef MlnAccelerationStructure(
    MLNAPI_PTR *PFN_MlnCreateAccelerationStructure)(
    MlnDevice device, const MlnAccelerationStructureDescriptor *descriptor);

typedef MlnDeviceAddress(
    MLNAPI_PTR *PFN_MlnGetAccelerationStructureDeviceAddress)(
    MlnDevice device, MlnAccelerationStructure accelerationStructure);

typedef void(MLNAPI_PTR *PFN_MlnDestroyAccelerationStructure)(
    MlnDevice device, MlnAccelerationStructure accelerationStructure);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetRayTracingCapabilities)(
    MlnDevice device, MlnRayTracingCapabilities *capabilities);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnCheckAccelerationStructureCompatibility)(
    MlnDevice device, const MlnAccelerationStructureVersion *version,
    MlnAccelerationStructureCompatibility *compatibility);

typedef MlnQueryPool(MLNAPI_PTR *PFN_MlnCreateQueryPool)(
    MlnDevice device, const MlnQueryPoolDescriptor *descriptor);

typedef void(MLNAPI_PTR *PFN_MlnDestroyQueryPool)(MlnDevice device,
                                                  MlnQueryPool queryPool);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnResetQueryPool)(
    MlnDevice device, const MlnResetQueryPoolDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetQueryPoolResult)(
    MlnDevice device, const MlnGetQueryPoolResultDescriptor *descriptor);

typedef MlnStatus(MLNAPI_PTR *PFN_MlnGetGpuCounterProperties)(
    MlnDevice device, MlnCounter counter, MlnGpuCounterProperties *properties);

#ifdef __cplusplus
}
#endif
