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

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t s8;

typedef uint8_t u8;

typedef int16_t s16;

typedef uint16_t u16;

typedef int32_t s32;

typedef uint32_t u32;

typedef int64_t s64;

typedef uint64_t u64;

typedef float f32;

typedef double f64;

typedef uint32_t b32;

typedef uint32_t MlnFlags;

typedef uint64_t MlnFlags64;

typedef uint32_t MlnSampleMask;

typedef uint64_t MlnDeviceSize;

typedef uint64_t MlnDeviceAddress;

struct OH_NativeBuffer;

typedef struct NativeWindow OHNativeWindow;

typedef MlnFlags MlnCompositeAlphaFlags;

typedef MlnFlags MlnDisplaySurfaceTransformFlags;

typedef MlnFlags MlnSwapchainDescriptorFlags;

typedef MlnFlags MlnDisplayModeDescriptorFlags;

typedef MlnFlags MlnDisplaySurfaceDescriptorFlags;

typedef MlnFlags MlnDeviceDescriptorFlags;

typedef MlnFlags MlnValidationLayerCheckOptionFlags;

typedef MlnFlags MlnValidationLayerFlags;

typedef MlnFlags MlnQueueDescriptorFlags;

typedef MlnFlags MlnSubmitDescriptorFlags;

typedef MlnFlags MlnMemoryPropertyFlags;

typedef MlnFlags MlnMemoryHeapFlags;

typedef MlnFlags MlnMemoryMapDescriptorFlags;

typedef MlnFlags MlnMemoryUnmapDescriptorFlags;

typedef MlnFlags MlnBufferDescriptorFlags;

typedef MlnFlags MlnBufferUsageFlags;

typedef MlnFlags MlnBufferViewDescriptorFlags;

typedef MlnFlags MlnImageDescriptorFlags;

typedef MlnFlags MlnImageUsageFlags;

typedef MlnFlags MlnImageViewDescriptorFlags;

typedef MlnFlags MlnImageAspectFlags;

typedef MlnFlags MlnFormatFeatureFlags;

typedef MlnFlags MlnSamplerDescriptorFlags;

typedef MlnFlags MlnSampleCountFlags;

typedef MlnFlags MlnSamplerYcbcrConversionDescriptorFlags;

typedef MlnFlags MlnShaderDescriptorFlags;

typedef MlnFlags MlnShaderStageFlags;

typedef MlnFlags MlnProgramCacheDescriptorFlags;

typedef MlnFlags MlnProgramInterfaceDescriptorFlags;

typedef MlnFlags64 MlnProgramStageFlags;

typedef MlnFlags MlnProgramStateFlags;

typedef MlnFlags MlnShaderStageStateFlags;

typedef MlnFlags MlnInputAssemblyStateFlags;

typedef MlnFlags MlnVertexInputStateFlags;

typedef MlnFlags MlnViewportStateFlags;

typedef MlnFlags MlnRasterizationStateFlags;

typedef MlnFlags MlnMultisampleStateFlags;

typedef MlnFlags MlnDepthStencilStateFlags;

typedef MlnFlags MlnColorBlendStateFlags;

typedef MlnFlags MlnDynamicStateFlags;

typedef MlnFlags MlnCreationFeedbackFlags;

typedef MlnFlags MlnBindingSlotFlags;

typedef MlnFlags MlnBindingLayoutDescriptorFlags;

typedef MlnFlags MlnGraphicsObjectGroupDescriptorFlags;

typedef MlnFlags MlnComputeObjectGroupDescriptorFlags;

typedef MlnFlags MlnTransferObjectGroupDescriptorFlags;

typedef MlnFlags MlnClearObjectGroupDescriptorFlags;

typedef MlnFlags MlnBarrierObjectGroupDescriptorFlags;

typedef MlnFlags MlnAccelerationStructureObjectGroupDescriptorFlags;

typedef MlnFlags MlnRenderingStateFlags;

typedef MlnFlags MlnResourceSetDescriptorFlags;

typedef MlnFlags MlnStateSetDescriptorFlags;

typedef MlnFlags MlnGraphicsCommandDescriptorFlags;

typedef MlnFlags MlnComputeCommandDescriptorFlags;

typedef MlnFlags MlnGraphicsObjectGroupUpdateDescriptorFlags;

typedef MlnFlags MlnComputeObjectGroupUpdateDescriptorFlags;

typedef MlnFlags MlnCullModeFlags;

typedef MlnFlags MlnStencilFaceFlags;

typedef MlnFlags MlnColorComponentFlags;

typedef MlnFlags MlnAccessFlags;

typedef MlnFlags MlnGraphicsDataGraphDescriptorFlags;

typedef MlnFlags MlnComputeDataGraphDescriptorFlags;

typedef MlnFlags MlnTransferDataGraphDescriptorFlags;

typedef MlnFlags MlnAccelerationStructureDataGraphDescriptorFlags;

typedef MlnFlags MlnDataGraphSubmitDescriptorFlags;

typedef MlnFlags MlnConditionalRenderingFlags;

typedef MlnFlags MlnPassNodeDescriptorFlags;

typedef MlnFlags MlnSchedulingGraphDescriptorFlags;

typedef MlnFlags MlnSchedulingGraphSubmitDescriptorFlags;

typedef MlnFlags MlnTimelineDescriptorFlags;

typedef MlnFlags MlnTimelineWaitFlags;

typedef MlnFlags MlnTimelineSignalFlags;

typedef MlnFlags MlnTimelineExportFlags;

typedef MlnFlags MlnTimelineImportFlags;

typedef MlnFlags MlnResolveModeFlags;

typedef MlnFlags MlnRenderTargetDescriptorFlags;

typedef MlnFlags MlnPrivateDataSlotDescriptorFlags;

typedef MlnFlags MlnAccelerationStructureInstanceFlags;

typedef MlnFlags MlnGeometryFlags;

typedef MlnFlags MlnBuildAccelerationStructureFlags;

typedef MlnFlags MlnAccelerationStructureDescriptorFlags;

typedef MlnFlags MlnQueryPoolDescriptorFlags;

MLN_DEFINE_HANDLE(MlnDevice);

MLN_DEFINE_HANDLE(MlnQueue);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnDeviceMemory);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnResource);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnResourceView);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnSampler);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnSamplerYcbcrConversion);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnProgramCache);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnProgramInterface);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnProgram);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnBindingLayout);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnBindingSet);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnObjectGroup);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnDataGraph);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnSchedulingGraph);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnTimeline);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnRenderTarget);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnAccelerationStructure);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnPrivateDataSlot);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnQueryPool);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnDisplaySurface);

MLN_DEFINE_NON_DISPATCHABLE_HANDLE(MlnSwapchain);

#ifdef __cplusplus
}
#endif
