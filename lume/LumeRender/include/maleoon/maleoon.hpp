#pragma once
#include <cstdint>
#include "maleoon.h"  // low-level C API

namespace mln {

// ===== Enum Aliases =====
using Status = MlnStatus;
using ObjectType = MlnObjectType;
using ExtensionKey = MlnExtensionKey;
using FragmentShadingRateCombinerOp = MlnFragmentShadingRateCombinerOp;
using LayerFlag = MlnLayerFlag;
using DeviceDescriptorFlagBits = MlnDeviceDescriptorFlagBits;
using ValidationLayerFlagBits = MlnValidationLayerFlagBits;
using ValidationLayerLogType = MlnValidationLayerLogType;
using ValidationLayerReportLevel = MlnValidationLayerReportLevel;
using ValidationLayerCheckActionType = MlnValidationLayerCheckActionType;
using ValidationLayerCheckOptionFlagBits =
    MlnValidationLayerCheckOptionFlagBits;
using QueuePriority = MlnQueuePriority;
using QueueDescriptorFlagBits = MlnQueueDescriptorFlagBits;
using SubmitDescriptorFlagBits = MlnSubmitDescriptorFlagBits;
using MemoryHeapFlagBits = MlnMemoryHeapFlagBits;
using MemoryPropertyFlagBits = MlnMemoryPropertyFlagBits;
using MemoryMapDescriptorFlagBits = MlnMemoryMapDescriptorFlagBits;
using MemoryUnmapDescriptorFlagBits = MlnMemoryUnmapDescriptorFlagBits;
using ExternalMemoryHandleType = MlnExternalMemoryHandleType;
using MemoryPolicy = MlnMemoryPolicy;
using BufferUsageFlagBits = MlnBufferUsageFlagBits;
using BufferDescriptorFlagBits = MlnBufferDescriptorFlagBits;
using ImageType = MlnImageType;
using Format = MlnFormat;
using ImageDescriptorFlagBits = MlnImageDescriptorFlagBits;
using SampleCountFlagBits = MlnSampleCountFlagBits;
using ImageTiling = MlnImageTiling;
using ImageUsageFlagBits = MlnImageUsageFlagBits;
using ImageLayout = MlnImageLayout;
using FormatFeatureFlagBits = MlnFormatFeatureFlagBits;
using Filter = MlnFilter;
using SamplerMipmapMode = MlnSamplerMipmapMode;
using SamplerAddressMode = MlnSamplerAddressMode;
using CompareOp = MlnCompareOp;
using BorderColor = MlnBorderColor;
using SamplerReductionMode = MlnSamplerReductionMode;
using SamplerDescriptorFlagBits = MlnSamplerDescriptorFlagBits;
using SamplerYcbcrConversionDescriptorFlagBits =
    MlnSamplerYcbcrConversionDescriptorFlagBits;
using ImageAspectFlagBits = MlnImageAspectFlagBits;
using ComponentSwizzle = MlnComponentSwizzle;
using BufferViewDescriptorFlagBits = MlnBufferViewDescriptorFlagBits;
using ImageViewDescriptorFlagBits = MlnImageViewDescriptorFlagBits;
using ImageViewType = MlnImageViewType;
using ShaderDescriptorFlagBits = MlnShaderDescriptorFlagBits;
using ShaderStageFlagBits = MlnShaderStageFlagBits;
using ProgramInterfaceDescriptorFlagBits =
    MlnProgramInterfaceDescriptorFlagBits;
using ProgramCacheHeaderVersion = MlnProgramCacheHeaderVersion;
using ProgramCacheDescriptorFlagBits = MlnProgramCacheDescriptorFlagBits;
using ProgramStageFlagBits = MlnProgramStageFlagBits;
using ProgramStateFlagBits = MlnProgramStateFlagBits;
using ShaderStageStateFlagBits = MlnShaderStageStateFlagBits;
using InputAssemblyStateFlagBits = MlnInputAssemblyStateFlagBits;
using VertexInputStateFlagBits = MlnVertexInputStateFlagBits;
using ViewportStateFlagBits = MlnViewportStateFlagBits;
using RasterizationStateFlagBits = MlnRasterizationStateFlagBits;
using MultisampleStateFlagBits = MlnMultisampleStateFlagBits;
using DepthStencilStateFlagBits = MlnDepthStencilStateFlagBits;
using ColorBlendStateFlagBits = MlnColorBlendStateFlagBits;
using DynamicStateFlagBits = MlnDynamicStateFlagBits;
using CreationFeedbackFlagBits = MlnCreationFeedbackFlagBits;
using DynamicStateType = MlnDynamicStateType;
using VertexInputRate = MlnVertexInputRate;
using PrimitiveTopology = MlnPrimitiveTopology;
using PolygonMode = MlnPolygonMode;
using FrontFace = MlnFrontFace;
using BlendFactor = MlnBlendFactor;
using StencilOp = MlnStencilOp;
using LogicOp = MlnLogicOp;
using BlendOp = MlnBlendOp;
using CullModeFlagBits = MlnCullModeFlagBits;
using StencilFaceFlagBits = MlnStencilFaceFlagBits;
using ColorComponentFlagBits = MlnColorComponentFlagBits;
using Priority = MlnPriority;
using LineRasterizationMode = MlnLineRasterizationMode;
using ProvokingVertexMode = MlnProvokingVertexMode;
using DepthBiasRepresentationMode = MlnDepthBiasRepresentationMode;
using AccessFlagBits = MlnAccessFlagBits;
using BindingType = MlnBindingType;
using BindingLayoutDescriptorFlagBits = MlnBindingLayoutDescriptorFlagBits;
using BindingSlotFlagBits = MlnBindingSlotFlagBits;
using SamplerYcbcrModelConversion = MlnSamplerYcbcrModelConversion;
using SamplerYcbcrRange = MlnSamplerYcbcrRange;
using ChromaLocation = MlnChromaLocation;
using GraphicsObjectGroupDescriptorFlagBits =
    MlnGraphicsObjectGroupDescriptorFlagBits;
using ComputeObjectGroupDescriptorFlagBits =
    MlnComputeObjectGroupDescriptorFlagBits;
using TransferObjectGroupDescriptorFlagBits =
    MlnTransferObjectGroupDescriptorFlagBits;
using ClearObjectGroupDescriptorFlagBits =
    MlnClearObjectGroupDescriptorFlagBits;
using BarrierObjectGroupDescriptorFlagBits =
    MlnBarrierObjectGroupDescriptorFlagBits;
using RenderingStateFlagBits = MlnRenderingStateFlagBits;
using ResourceSetDescriptorFlagBits = MlnResourceSetDescriptorFlagBits;
using StateSetDescriptorFlagBits = MlnStateSetDescriptorFlagBits;
using GraphicsCommandDescriptorFlagBits = MlnGraphicsCommandDescriptorFlagBits;
using ComputeCommandDescriptorFlagBits = MlnComputeCommandDescriptorFlagBits;
using GraphicsObjectGroupUpdateDescriptorFlagBits =
    MlnGraphicsObjectGroupUpdateDescriptorFlagBits;
using ComputeObjectGroupUpdateDescriptorFlagBits =
    MlnComputeObjectGroupUpdateDescriptorFlagBits;
using IndexType = MlnIndexType;
using DynamicResourceType = MlnDynamicResourceType;
using GraphicsCommandType = MlnGraphicsCommandType;
using ComputeCommandType = MlnComputeCommandType;
using GraphicsObjectGroupModifierType = MlnGraphicsObjectGroupModifierType;
using ComputeObjectGroupModifierType = MlnComputeObjectGroupModifierType;
using TransferCommandType = MlnTransferCommandType;
using ConditionalRenderingFlagBits = MlnConditionalRenderingFlagBits;
using GraphicsDataGraphDescriptorFlagBits =
    MlnGraphicsDataGraphDescriptorFlagBits;
using ComputeDataGraphDescriptorFlagBits =
    MlnComputeDataGraphDescriptorFlagBits;
using TransferDataGraphDescriptorFlagBits =
    MlnTransferDataGraphDescriptorFlagBits;
using DataGraphSubmitDescriptorFlagBits = MlnDataGraphSubmitDescriptorFlagBits;
using PassNodeResourceType = MlnPassNodeResourceType;
using PassNodeHintType = MlnPassNodeHintType;
using PassNodeWorkloadType = MlnPassNodeWorkloadType;
using PassNodeDescriptorFlagBits = MlnPassNodeDescriptorFlagBits;
using SchedulingGraphDescriptorFlagBits = MlnSchedulingGraphDescriptorFlagBits;
using SchedulingGraphSubmitDescriptorFlagBits =
    MlnSchedulingGraphSubmitDescriptorFlagBits;
using TimelineType = MlnTimelineType;
using TimelineWaitFlagBits = MlnTimelineWaitFlagBits;
using TimelineSignalFlagBits = MlnTimelineSignalFlagBits;
using TimelineExportFlagBits = MlnTimelineExportFlagBits;
using TimelineImportFlagBits = MlnTimelineImportFlagBits;
using TimelineDescriptorFlagBits = MlnTimelineDescriptorFlagBits;
using ResolveModeFlagBits = MlnResolveModeFlagBits;
using AttachmentLoadOp = MlnAttachmentLoadOp;
using AttachmentStoreOp = MlnAttachmentStoreOp;
using RenderTargetDescriptorFlagBits = MlnRenderTargetDescriptorFlagBits;
using PrivateDataSlotDescriptorFlagBits = MlnPrivateDataSlotDescriptorFlagBits;
using ColorSpace = MlnColorSpace;
using LossyTypes = MlnLossyTypes;
using DisplaySurfaceTransformFlagBits = MlnDisplaySurfaceTransformFlagBits;
using SwapchainDescriptorFlagBits = MlnSwapchainDescriptorFlagBits;
using CompositeAlphaFlagBits = MlnCompositeAlphaFlagBits;
using DisplayModeDescriptorFlagBits = MlnDisplayModeDescriptorFlagBits;
using DisplaySurfaceDescriptorFlagBits = MlnDisplaySurfaceDescriptorFlagBits;
using PresentMode = MlnPresentMode;
using AccelerationStructureInstanceFlagBits =
    MlnAccelerationStructureInstanceFlagBits;
using GeometryFlagBits = MlnGeometryFlagBits;
using GeometryType = MlnGeometryType;
using AccelerationStructureType = MlnAccelerationStructureType;
using BuildAccelerationStructureFlagBits =
    MlnBuildAccelerationStructureFlagBits;
using AccelerationStructureBuildMode = MlnAccelerationStructureBuildMode;
using AccelerationStructureCommandType = MlnAccelerationStructureCommandType;
using AccelerationStructureCopyMode = MlnAccelerationStructureCopyMode;
using AccelerationStructureDescriptorFlagBits =
    MlnAccelerationStructureDescriptorFlagBits;
using AccelerationStructureObjectGroupDescriptorFlagBits =
    MlnAccelerationStructureObjectGroupDescriptorFlagBits;
using AccelerationStructureDataGraphDescriptorFlagBits =
    MlnAccelerationStructureDataGraphDescriptorFlagBits;
using AccelerationStructureCompatibility =
    MlnAccelerationStructureCompatibility;
using QueryType = MlnQueryType;
using CounterSet = MlnCounterSet;
using Counter = MlnCounter;
using QueryCollectionType = MlnQueryCollectionType;
using QuerySampleCollectionMode = MlnQuerySampleCollectionMode;
using QuerySampleCollectionLocation = MlnQuerySampleCollectionLocation;
using CounterStorage = MlnCounterStorage;
using CounterUnit = MlnCounterUnit;
using QueryPoolDescriptorFlagBits = MlnQueryPoolDescriptorFlagBits;

// ===== Flag Aliases =====
using AccelerationStructureDataGraphDescriptorFlagBits =
    MlnAccelerationStructureDataGraphDescriptorFlagBits;
using AccelerationStructureDataGraphDescriptorFlags =
    MlnAccelerationStructureDataGraphDescriptorFlags;
using AccelerationStructureDescriptorFlagBits =
    MlnAccelerationStructureDescriptorFlagBits;
using AccelerationStructureDescriptorFlags =
    MlnAccelerationStructureDescriptorFlags;
using AccelerationStructureInstanceFlagBits =
    MlnAccelerationStructureInstanceFlagBits;
using AccelerationStructureInstanceFlags =
    MlnAccelerationStructureInstanceFlags;
using AccelerationStructureObjectGroupDescriptorFlagBits =
    MlnAccelerationStructureObjectGroupDescriptorFlagBits;
using AccelerationStructureObjectGroupDescriptorFlags =
    MlnAccelerationStructureObjectGroupDescriptorFlags;
using AccessFlagBits = MlnAccessFlagBits;
using AccessFlags = MlnAccessFlags;
using BarrierObjectGroupDescriptorFlagBits =
    MlnBarrierObjectGroupDescriptorFlagBits;
using BarrierObjectGroupDescriptorFlags = MlnBarrierObjectGroupDescriptorFlags;
using BindingLayoutDescriptorFlagBits = MlnBindingLayoutDescriptorFlagBits;
using BindingLayoutDescriptorFlags = MlnBindingLayoutDescriptorFlags;
using BindingSlotFlagBits = MlnBindingSlotFlagBits;
using BindingSlotFlags = MlnBindingSlotFlags;
using BufferDescriptorFlagBits = MlnBufferDescriptorFlagBits;
using BufferDescriptorFlags = MlnBufferDescriptorFlags;
using BufferUsageFlagBits = MlnBufferUsageFlagBits;
using BufferUsageFlags = MlnBufferUsageFlags;
using BufferViewDescriptorFlagBits = MlnBufferViewDescriptorFlagBits;
using BufferViewDescriptorFlags = MlnBufferViewDescriptorFlags;
using BuildAccelerationStructureFlagBits =
    MlnBuildAccelerationStructureFlagBits;
using BuildAccelerationStructureFlags = MlnBuildAccelerationStructureFlags;
using ClearObjectGroupDescriptorFlagBits =
    MlnClearObjectGroupDescriptorFlagBits;
using ClearObjectGroupDescriptorFlags = MlnClearObjectGroupDescriptorFlags;
using ColorBlendStateFlagBits = MlnColorBlendStateFlagBits;
using ColorBlendStateFlags = MlnColorBlendStateFlags;
using ColorComponentFlagBits = MlnColorComponentFlagBits;
using ColorComponentFlags = MlnColorComponentFlags;
using CompositeAlphaFlagBits = MlnCompositeAlphaFlagBits;
using CompositeAlphaFlags = MlnCompositeAlphaFlags;
using ComputeCommandDescriptorFlagBits = MlnComputeCommandDescriptorFlagBits;
using ComputeCommandDescriptorFlags = MlnComputeCommandDescriptorFlags;
using ComputeDataGraphDescriptorFlagBits =
    MlnComputeDataGraphDescriptorFlagBits;
using ComputeDataGraphDescriptorFlags = MlnComputeDataGraphDescriptorFlags;
using ComputeObjectGroupDescriptorFlagBits =
    MlnComputeObjectGroupDescriptorFlagBits;
using ComputeObjectGroupDescriptorFlags = MlnComputeObjectGroupDescriptorFlags;
using ComputeObjectGroupUpdateDescriptorFlagBits =
    MlnComputeObjectGroupUpdateDescriptorFlagBits;
using ComputeObjectGroupUpdateDescriptorFlags =
    MlnComputeObjectGroupUpdateDescriptorFlags;
using ConditionalRenderingFlagBits = MlnConditionalRenderingFlagBits;
using ConditionalRenderingFlags = MlnConditionalRenderingFlags;
using CreationFeedbackFlagBits = MlnCreationFeedbackFlagBits;
using CreationFeedbackFlags = MlnCreationFeedbackFlags;
using CullModeFlagBits = MlnCullModeFlagBits;
using CullModeFlags = MlnCullModeFlags;
using DataGraphSubmitDescriptorFlagBits = MlnDataGraphSubmitDescriptorFlagBits;
using DataGraphSubmitDescriptorFlags = MlnDataGraphSubmitDescriptorFlags;
using DepthStencilStateFlagBits = MlnDepthStencilStateFlagBits;
using DepthStencilStateFlags = MlnDepthStencilStateFlags;
using DeviceDescriptorFlagBits = MlnDeviceDescriptorFlagBits;
using DeviceDescriptorFlags = MlnDeviceDescriptorFlags;
using DisplayModeDescriptorFlagBits = MlnDisplayModeDescriptorFlagBits;
using DisplayModeDescriptorFlags = MlnDisplayModeDescriptorFlags;
using DisplaySurfaceDescriptorFlagBits = MlnDisplaySurfaceDescriptorFlagBits;
using DisplaySurfaceDescriptorFlags = MlnDisplaySurfaceDescriptorFlags;
using DisplaySurfaceTransformFlagBits = MlnDisplaySurfaceTransformFlagBits;
using DisplaySurfaceTransformFlags = MlnDisplaySurfaceTransformFlags;
using DynamicStateFlagBits = MlnDynamicStateFlagBits;
using DynamicStateFlags = MlnDynamicStateFlags;
using Flags = MlnFlags;
using FormatFeatureFlagBits = MlnFormatFeatureFlagBits;
using FormatFeatureFlags = MlnFormatFeatureFlags;
using GeometryFlagBits = MlnGeometryFlagBits;
using GeometryFlags = MlnGeometryFlags;
using GraphicsCommandDescriptorFlagBits = MlnGraphicsCommandDescriptorFlagBits;
using GraphicsCommandDescriptorFlags = MlnGraphicsCommandDescriptorFlags;
using GraphicsDataGraphDescriptorFlagBits =
    MlnGraphicsDataGraphDescriptorFlagBits;
using GraphicsDataGraphDescriptorFlags = MlnGraphicsDataGraphDescriptorFlags;
using GraphicsObjectGroupDescriptorFlagBits =
    MlnGraphicsObjectGroupDescriptorFlagBits;
using GraphicsObjectGroupDescriptorFlags =
    MlnGraphicsObjectGroupDescriptorFlags;
using GraphicsObjectGroupUpdateDescriptorFlagBits =
    MlnGraphicsObjectGroupUpdateDescriptorFlagBits;
using GraphicsObjectGroupUpdateDescriptorFlags =
    MlnGraphicsObjectGroupUpdateDescriptorFlags;
using ImageAspectFlagBits = MlnImageAspectFlagBits;
using ImageAspectFlags = MlnImageAspectFlags;
using ImageDescriptorFlagBits = MlnImageDescriptorFlagBits;
using ImageDescriptorFlags = MlnImageDescriptorFlags;
using ImageUsageFlagBits = MlnImageUsageFlagBits;
using ImageUsageFlags = MlnImageUsageFlags;
using ImageViewDescriptorFlagBits = MlnImageViewDescriptorFlagBits;
using ImageViewDescriptorFlags = MlnImageViewDescriptorFlags;
using InputAssemblyStateFlagBits = MlnInputAssemblyStateFlagBits;
using InputAssemblyStateFlags = MlnInputAssemblyStateFlags;
using MemoryHeapFlagBits = MlnMemoryHeapFlagBits;
using MemoryHeapFlags = MlnMemoryHeapFlags;
using MemoryMapDescriptorFlagBits = MlnMemoryMapDescriptorFlagBits;
using MemoryMapDescriptorFlags = MlnMemoryMapDescriptorFlags;
using MemoryPropertyFlagBits = MlnMemoryPropertyFlagBits;
using MemoryPropertyFlags = MlnMemoryPropertyFlags;
using MemoryUnmapDescriptorFlagBits = MlnMemoryUnmapDescriptorFlagBits;
using MemoryUnmapDescriptorFlags = MlnMemoryUnmapDescriptorFlags;
using MultisampleStateFlagBits = MlnMultisampleStateFlagBits;
using MultisampleStateFlags = MlnMultisampleStateFlags;
using PassNodeDescriptorFlagBits = MlnPassNodeDescriptorFlagBits;
using PassNodeDescriptorFlags = MlnPassNodeDescriptorFlags;
using PrivateDataSlotDescriptorFlagBits = MlnPrivateDataSlotDescriptorFlagBits;
using PrivateDataSlotDescriptorFlags = MlnPrivateDataSlotDescriptorFlags;
using ProgramCacheDescriptorFlagBits = MlnProgramCacheDescriptorFlagBits;
using ProgramCacheDescriptorFlags = MlnProgramCacheDescriptorFlags;
using ProgramInterfaceDescriptorFlagBits =
    MlnProgramInterfaceDescriptorFlagBits;
using ProgramInterfaceDescriptorFlags = MlnProgramInterfaceDescriptorFlags;
using ProgramStageFlagBits = MlnProgramStageFlagBits;
using ProgramStageFlags = MlnProgramStageFlags;
using ProgramStateFlagBits = MlnProgramStateFlagBits;
using ProgramStateFlags = MlnProgramStateFlags;
using QueryPoolDescriptorFlagBits = MlnQueryPoolDescriptorFlagBits;
using QueryPoolDescriptorFlags = MlnQueryPoolDescriptorFlags;
using QueueDescriptorFlagBits = MlnQueueDescriptorFlagBits;
using QueueDescriptorFlags = MlnQueueDescriptorFlags;
using RasterizationStateFlagBits = MlnRasterizationStateFlagBits;
using RasterizationStateFlags = MlnRasterizationStateFlags;
using RenderTargetDescriptorFlagBits = MlnRenderTargetDescriptorFlagBits;
using RenderTargetDescriptorFlags = MlnRenderTargetDescriptorFlags;
using RenderingStateFlagBits = MlnRenderingStateFlagBits;
using RenderingStateFlags = MlnRenderingStateFlags;
using ResolveModeFlagBits = MlnResolveModeFlagBits;
using ResolveModeFlags = MlnResolveModeFlags;
using ResourceSetDescriptorFlagBits = MlnResourceSetDescriptorFlagBits;
using ResourceSetDescriptorFlags = MlnResourceSetDescriptorFlags;
using SampleCountFlagBits = MlnSampleCountFlagBits;
using SampleCountFlags = MlnSampleCountFlags;
using SamplerDescriptorFlagBits = MlnSamplerDescriptorFlagBits;
using SamplerDescriptorFlags = MlnSamplerDescriptorFlags;
using SamplerYcbcrConversionDescriptorFlagBits =
    MlnSamplerYcbcrConversionDescriptorFlagBits;
using SamplerYcbcrConversionDescriptorFlags =
    MlnSamplerYcbcrConversionDescriptorFlags;
using SchedulingGraphDescriptorFlagBits = MlnSchedulingGraphDescriptorFlagBits;
using SchedulingGraphDescriptorFlags = MlnSchedulingGraphDescriptorFlags;
using SchedulingGraphSubmitDescriptorFlagBits =
    MlnSchedulingGraphSubmitDescriptorFlagBits;
using SchedulingGraphSubmitDescriptorFlags =
    MlnSchedulingGraphSubmitDescriptorFlags;
using ShaderDescriptorFlagBits = MlnShaderDescriptorFlagBits;
using ShaderDescriptorFlags = MlnShaderDescriptorFlags;
using ShaderStageFlagBits = MlnShaderStageFlagBits;
using ShaderStageFlags = MlnShaderStageFlags;
using ShaderStageStateFlagBits = MlnShaderStageStateFlagBits;
using ShaderStageStateFlags = MlnShaderStageStateFlags;
using StateSetDescriptorFlagBits = MlnStateSetDescriptorFlagBits;
using StateSetDescriptorFlags = MlnStateSetDescriptorFlags;
using StencilFaceFlagBits = MlnStencilFaceFlagBits;
using StencilFaceFlags = MlnStencilFaceFlags;
using SubmitDescriptorFlagBits = MlnSubmitDescriptorFlagBits;
using SubmitDescriptorFlags = MlnSubmitDescriptorFlags;
using SwapchainDescriptorFlagBits = MlnSwapchainDescriptorFlagBits;
using SwapchainDescriptorFlags = MlnSwapchainDescriptorFlags;
using TimelineDescriptorFlagBits = MlnTimelineDescriptorFlagBits;
using TimelineDescriptorFlags = MlnTimelineDescriptorFlags;
using TimelineExportFlagBits = MlnTimelineExportFlagBits;
using TimelineExportFlags = MlnTimelineExportFlags;
using TimelineImportFlagBits = MlnTimelineImportFlagBits;
using TimelineImportFlags = MlnTimelineImportFlags;
using TimelineSignalFlagBits = MlnTimelineSignalFlagBits;
using TimelineSignalFlags = MlnTimelineSignalFlags;
using TimelineWaitFlagBits = MlnTimelineWaitFlagBits;
using TimelineWaitFlags = MlnTimelineWaitFlags;
using TransferDataGraphDescriptorFlagBits =
    MlnTransferDataGraphDescriptorFlagBits;
using TransferDataGraphDescriptorFlags = MlnTransferDataGraphDescriptorFlags;
using TransferObjectGroupDescriptorFlagBits =
    MlnTransferObjectGroupDescriptorFlagBits;
using TransferObjectGroupDescriptorFlags =
    MlnTransferObjectGroupDescriptorFlags;
using ValidationLayerCheckOptionFlagBits =
    MlnValidationLayerCheckOptionFlagBits;
using ValidationLayerCheckOptionFlags = MlnValidationLayerCheckOptionFlags;
using ValidationLayerFlagBits = MlnValidationLayerFlagBits;
using ValidationLayerFlags = MlnValidationLayerFlags;
using VertexInputStateFlagBits = MlnVertexInputStateFlagBits;
using VertexInputStateFlags = MlnVertexInputStateFlags;
using ViewportStateFlagBits = MlnViewportStateFlagBits;
using ViewportStateFlags = MlnViewportStateFlags;

// ===== Basetype Aliases =====
using Flags = MlnFlags;
using Flags64 = MlnFlags64;
using SampleMask = MlnSampleMask;
using DeviceSize = MlnDeviceSize;
using DeviceAddress = MlnDeviceAddress;

// ===== Forward Declarations =====
struct Origin2D;
struct Origin3D;
struct Size2D;
struct Size3D;
struct Rect2D;
struct ExtensionBlock;
struct LayerProperties;
struct ApplicationDescriptor;
struct QueueDescriptor;
struct DeviceDescriptor;
struct GpuLimits;
struct GpuFeatures;
struct RayTracingCapabilities;
struct GpuProperties;
struct GpuFormatProperties;
struct GpuImageFormatProperties;
struct GpuFragmentShadingRate;
struct GpuFragmentShadingRateFeatures;
struct GpuFragmentShadingRateProperties;
struct MemoryType;
struct MemoryHeap;
struct GpuMemoryProperties;
struct ExternalMemoryDescriptor;
struct MemoryAllocateDescriptor;
struct ResourceMemoryRequirementsDescriptor;
struct BindResourceMemoryDescriptor;
struct MemoryDedicatedRequirements;
struct MemoryRequirements;
struct MappedMemoryRangeDescriptor;
struct MemoryMapDescriptor;
struct MemoryUnmapDescriptor;
struct ValidationLayer;
struct BufferDescriptor;
struct ImageFormatList;
struct ImageDescriptor;
struct SamplerDescriptor;
struct ComponentMapping;
struct SamplerYcbcrConversionDescriptor;
struct NativeBufferFormatProperties;
struct NativeBufferProperties;
struct MemoryGetNativeBufferDescriptor;
struct BufferViewDescriptor;
struct ImageSubresourceRange;
struct ImageViewDescriptor;
struct ShaderDescriptor;
struct ProgramCacheDescriptor;
struct ProgramInterfaceDescriptor;
struct CompileTimeConstantMapEntry;
struct CompileTimeConstantState;
struct ShaderStageState;
struct VertexInputBindingState;
struct VertexInputAttributeState;
struct VertexInputState;
struct InputAssemblyState;
struct Viewport;
struct ViewportState;
struct DepthBiasRepresentation;
struct RasterizationState;
struct SampleLocation;
struct SampleLocationDynamicState;
struct MultisampleState;
struct StencilOpState;
struct DepthStencilState;
struct ColorBlendAttachmentState;
struct ColorBlendState;
struct RenderingState;
struct DynamicState;
struct FragmentShadingRateState;
struct CreationFeedback;
struct CreationFeedbackState;
struct GraphicsProgramState;
struct ComputeProgramState;
struct BindingSlot;
struct BindingLayoutDescriptor;
struct BindingBufferDescriptor;
struct BindingImageDescriptor;
struct BindingInlineUniformDescriptor;
struct WriteBindingSet;
struct CopyBindingSet;
struct ResourceBindingSets;
struct ProgramConstants;
struct ResourceProgramConstants;
struct ResourceVertexBuffers;
struct ResourceIndexBuffer;
struct ResourceSetDescriptor;
struct ViewportDynamicState;
struct ScissorDynamicState;
struct DepthBoundsDynamicState;
struct DepthBiasDynamicState;
struct StencilOpDynamicState;
struct StencilCompareMaskDynamicState;
struct StencilWriteMaskDynamicState;
struct StencilReferenceDynamicState;
struct FragmentShadingRateDynamicState;
struct StateSetDescriptor;
struct DrawCommand;
struct DrawIndexedCommand;
struct DrawIndirectCommand;
struct DrawIndexedIndirectCommand;
struct DrawIndirectCountCommand;
struct DrawIndexedIndirectCountCommand;
union GraphicsCommandData;
struct GraphicsCommand;
struct GraphicsCommandDescriptor;
struct ResourceBindingSetsInheritance;
struct ResourceVertexBuffersInheritance;
struct ResourceIndexBufferInheritance;
struct StateViewportsInheritance;
struct StateScissorsInheritance;
struct FragmentShadingRateInheritance;
struct GraphicsObjectGroupInheritanceDescriptor;
struct GraphicsObjectGroupDescriptor;
struct BindingSetsDynamicOffsets;
union GraphicsObjectGroupModifierData;
struct GraphicsObjectGroupModifierContent;
struct GraphicsObjectGroupModifier;
struct GraphicsObjectGroupUpdateDescriptor;
struct ComputeBaseGroup;
struct ComputeGroupCount;
struct ComputeGroupCountIndirect;
union ComputeCommandData;
struct ComputeCommand;
struct ComputeCommandDescriptor;
struct ComputeObjectGroupInheritanceDescriptor;
struct ComputeObjectGroupDescriptor;
union ComputeObjectGroupModifierData;
struct ComputeObjectGroupModifierContent;
struct ComputeObjectGroupModifier;
struct ComputeObjectGroupUpdateDescriptor;
struct BufferCopyRegion;
struct CopyBufferDescriptor;
struct FillBufferDescriptor;
struct UpdateBufferDescriptor;
struct ImageSubresourceLayers;
struct BufferImageCopyRegion;
struct CopyBufferToImageDescriptor;
struct ImageCopyRegion;
struct CopyImageDescriptor;
struct CopyImageToBufferDescriptor;
struct ImageBlitRegion;
struct BlitImageDescriptor;
struct ImageResolveRegion;
struct ResolveImageDescriptor;
struct ImageGenMipMapRegion;
struct GenerateMipmapDescriptor;
struct ClearImageRegion;
union ClearColor;
struct ClearDepthStencil;
union ClearValue;
struct ClearImageDescriptor;
struct QueryPoolResultCopyRegion;
struct CopyQueryPoolResultDescriptor;
struct QueryPoolResetRegion;
struct ResetQueryPoolDescriptor;
union TransferCommandData;
struct TransferCommandDescriptor;
struct TransferObjectGroupDescriptor;
struct ClearAttachment;
struct ClearRect;
struct ClearObjectGroupDescriptor;
struct MemoryBarrier;
struct BufferMemoryBarrier;
struct ImageMemoryBarrier;
struct BarrierObjectGroupDescriptor;
struct GraphicsDataGraphDescriptor;
struct ComputeDataGraphDescriptor;
struct TransferDataGraphDescriptor;
struct AccelerationStructureDataGraphDescriptor;
struct QueryRangeCollection;
struct QueryPointCollection;
struct QuerySampleCollection;
struct QueryObjectPropertyCollection;
union QueryCollectionData;
struct DataGraphQueryDescriptor;
struct ConditionalRenderingRange;
struct PassNodeResourceDescriptor;
struct PassNodeDependencyDescriptor;
struct PassNodeWorkloadDescriptor;
struct PassNodeDescriptor;
struct CameraDescriptor;
struct SchedulingGraphDescriptor;
struct GraphicsObjectGroupInheritanceContent;
struct GraphicsObjectGroupInheritance;
struct GraphicsDataGraphInheritance;
struct ComputeObjectGroupInheritanceContent;
struct ComputeObjectGroupInheritance;
struct ComputeDataGraphInheritance;
union DataGraphInheritanceDescriptor;
struct DataGraphSubmitDescriptor;
struct SchedulingGraphSubmitDescriptor;
struct TimelineDescriptor;
struct TimelineWaitDescriptor;
struct TimelineSignalDescriptor;
struct TimelineExportFdDescriptor;
struct TimelineImportFdDescriptor;
struct TimelineImportEventIdDescriptor;
struct TimelineSubmitDescriptor;
struct SubmitDescriptor;
struct AttachmentDescriptor;
struct FragmentShadingRateAttachmentDescriptor;
struct RenderTargetDescriptor;
struct DisplaySurfaceFormat;
struct DisplaySurfaceCapabilities;
struct DisplaySurfaceDescriptor;
struct SwapchainDescriptor;
struct PresentDescriptor;
struct PrivateDataSlotDescriptor;
struct PrivateDataSetDescriptor;
struct PrivateDataGetDescriptor;
struct AabbPositions;
struct TransformMatrix;
struct EllipsoidCullingDescriptor;
struct AccelerationStructureInstance;
struct Ellipsoid;
struct AccelerationStructureGeometryTrianglesDescriptor;
struct AccelerationStructureGeometryAabbsDescriptor;
struct AccelerationStructureGeometryInstancesDescriptor;
struct AccelerationStructureGeometryEllipsoidDescriptor;
union AccelerationStructureGeometryData;
struct AccelerationStructureGeometryDescriptor;
struct AccelerationStructureBuildGeometryDescriptor;
struct AccelerationStructureBuildRangeDescriptor;
struct AccelerationStructureBuildCommand;
struct AccelerationStructureCopyCommand;
struct AccelerationStructureSerializeCommand;
struct AccelerationStructureDeserializeCommand;
union AccelerationStructureCommandData;
struct AccelerationStructureCommandDescriptor;
struct AccelerationStructureDescriptor;
struct AccelerationStructureObjectGroupDescriptor;
struct AccelerationStructureBuildSizesDescriptor;
struct AccelerationStructureVersion;
struct QueryPoolDescriptor;
struct GpuCounterProperties;
struct GetQueryPoolResultDescriptor;

// ===== Handle Wrapper Classes =====
class DeviceMemory {
public:
    using ctype = MlnDeviceMemory;
    DeviceMemory() = default;
    explicit DeviceMemory(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class Resource {
public:
    using ctype = MlnResource;
    Resource() = default;
    explicit Resource(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class ResourceView {
public:
    using ctype = MlnResourceView;
    ResourceView() = default;
    explicit ResourceView(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class Sampler {
public:
    using ctype = MlnSampler;
    Sampler() = default;
    explicit Sampler(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class SamplerYcbcrConversion {
public:
    using ctype = MlnSamplerYcbcrConversion;
    SamplerYcbcrConversion() = default;
    explicit SamplerYcbcrConversion(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class ProgramCache {
public:
    using ctype = MlnProgramCache;
    ProgramCache() = default;
    explicit ProgramCache(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class ProgramInterface {
public:
    using ctype = MlnProgramInterface;
    ProgramInterface() = default;
    explicit ProgramInterface(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class Program {
public:
    using ctype = MlnProgram;
    Program() = default;
    explicit Program(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class BindingLayout {
public:
    using ctype = MlnBindingLayout;
    BindingLayout() = default;
    explicit BindingLayout(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class BindingSet {
public:
    using ctype = MlnBindingSet;
    BindingSet() = default;
    explicit BindingSet(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class ObjectGroup {
public:
    using ctype = MlnObjectGroup;
    ObjectGroup() = default;
    explicit ObjectGroup(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class DataGraph {
public:
    using ctype = MlnDataGraph;
    DataGraph() = default;
    explicit DataGraph(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class SchedulingGraph {
public:
    using ctype = MlnSchedulingGraph;
    SchedulingGraph() = default;
    explicit SchedulingGraph(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class Timeline {
public:
    using ctype = MlnTimeline;
    Timeline() = default;
    explicit Timeline(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class RenderTarget {
public:
    using ctype = MlnRenderTarget;
    RenderTarget() = default;
    explicit RenderTarget(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class AccelerationStructure {
public:
    using ctype = MlnAccelerationStructure;
    AccelerationStructure() = default;
    explicit AccelerationStructure(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class PrivateDataSlot {
public:
    using ctype = MlnPrivateDataSlot;
    PrivateDataSlot() = default;
    explicit PrivateDataSlot(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class QueryPool {
public:
    using ctype = MlnQueryPool;
    QueryPool() = default;
    explicit QueryPool(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class DisplaySurface {
public:
    using ctype = MlnDisplaySurface;
    DisplaySurface() = default;
    explicit DisplaySurface(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class Swapchain {
public:
    using ctype = MlnSwapchain;
    Swapchain() = default;
    explicit Swapchain(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }

private:
    ctype m_handle{};
};

class Queue {
public:
    using ctype = MlnQueue;
    Queue() = default;
    explicit Queue(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }
    Status QueueSubmit(const SubmitDescriptor* descriptor);
    Status QueueWaitIdle();
    Status QueueSetPriority(QueuePriority priority, f32 adjust);
    Status QueueGetPriority(QueuePriority* priority, f32* adjust);
    Status QueuePresentSwapchain(const PresentDescriptor* descriptor);
    Status QueueSignalReleaseImageOHOS(u32 waitCount, const Timeline* timeline,
                                       const u64* waitValues, Resource resource,
                                       s32* nativeFenceFd);

private:
    ctype m_handle{};
};

class Device {
public:
    using ctype = MlnDevice;
    Device() = default;
    explicit Device(ctype handle) : m_handle(handle) {}
    ctype get() const { return m_handle; }
    operator ctype() const noexcept { return m_handle; }
    PFN_MlnVoidFunction GetDeviceProcAddr(const char* name);
    Status GetGpuFeatures(GpuFeatures* features);
    Status GetGpuProperties(GpuProperties* properties);
    Status GetGpuFormatProperties(Format format,
                                  GpuFormatProperties* properties);
    Status GetGpuImageFormatProperties(Format format, ImageType type,
                                       ImageTiling tiling,
                                       ImageUsageFlags usage,
                                       ImageDescriptorFlags flags,
                                       GpuImageFormatProperties* properties);
    Status DeviceWaitIdle();
    Status GetGpuFragmentShadingRates(
        u32* pFragmentShadingRateCount,
        GpuFragmentShadingRate* pFragmentShadingRates,
        GpuFragmentShadingRateFeatures* pFragmentShadingRateFeatures,
        GpuFragmentShadingRateProperties* pFragmentShadingRateProperties);
    Queue GetDeviceQueue(u32 index);
    Status GetGpuMemoryProperties(GpuMemoryProperties* properties);
    void GetResourceMemoryRequirements(
        const ResourceMemoryRequirementsDescriptor* descriptor,
        MemoryRequirements* memoryRequirements);
    DeviceMemory AllocateMemory(const MemoryAllocateDescriptor* descriptor);
    void FreeMemory(DeviceMemory memory);
    Status MapMemory(const MemoryMapDescriptor* descriptor, void** data);
    Status UnmapMemory(const MemoryUnmapDescriptor* descriptor);
    Status FlushMappedMemoryRanges(
        u32 memoryRangeCount, const MappedMemoryRangeDescriptor* memoryRanges);
    Status InvalidateMappedMemoryRanges(
        u32 memoryRangeCount, const MappedMemoryRangeDescriptor* memoryRanges);
    Status GetNativeBufferProperties(const OH_NativeBuffer* buffer,
                                     NativeBufferProperties* properties);
    Status GetMemoryNativeBuffer(
        const MemoryGetNativeBufferDescriptor* descriptor,
        OH_NativeBuffer** pBuffer);
    void GetBufferResourceMemoryRequirements(
        const BufferDescriptor* descriptor,
        MemoryRequirements* memoryRequirements);
    void GetImageResourceMemoryRequirements(
        const ImageDescriptor* descriptor,
        MemoryRequirements* memoryRequirements);
    Resource CreateBufferResource(const BufferDescriptor* descriptor);
    Resource CreateImageResource(const ImageDescriptor* descriptor);
    Status BindResourceMemory(const BindResourceMemoryDescriptor* descriptor);
    void DestroyResource(Resource resource);
    Sampler CreateSampler(const SamplerDescriptor* descriptor);
    void DestroySampler(Sampler sampler);
    SamplerYcbcrConversion CreateSamplerYcbcrConversion(
        const SamplerYcbcrConversionDescriptor* descriptor);
    void DestroySamplerYcbcrConversion(SamplerYcbcrConversion ycbcrConversion);
    DeviceAddress GetBufferResourceAddress(Resource resource);
    ResourceView CreateBufferResourceView(
        const BufferViewDescriptor* descriptor);
    ResourceView CreateImageResourceView(const ImageViewDescriptor* descriptor);
    void DestroyResourceView(ResourceView resourceView);
    ProgramCache CreateProgramCache(const ProgramCacheDescriptor* descriptor);
    Status GetProgramCacheData(ProgramCache programCache, size_t* cacheSize,
                               void* cache);
    Status MergeProgramCaches(ProgramCache dstCache, u32 srcCacheCount,
                              const ProgramCache* srcCaches);
    void DestroyProgramCache(ProgramCache programCache);
    ProgramInterface CreateProgramInterface(
        const ProgramInterfaceDescriptor* descriptor);
    void DestroyProgramInterface(ProgramInterface programInterface);
    Program CreateGraphicsProgram(ProgramCache programCache,
                                  const GraphicsProgramState* state);
    Program CreateComputeProgram(ProgramCache programCache,
                                 const ComputeProgramState* state);
    Program CreateGraphicsProgramAsync(ProgramCache programCache,
                                       const GraphicsProgramState* state,
                                       Priority priority);
    Program CreateComputeProgramAsync(ProgramCache programCache,
                                      const ComputeProgramState* state,
                                      Priority priority);
    Status GetCreatedProgramAsyncResult(Program program);
    Status WaitCreatedProgramAsyncResult(Program program);
    void DestroyProgram(Program program);
    BindingLayout CreateBindingLayout(
        const BindingLayoutDescriptor* descriptor);
    void DestroyBindingLayout(BindingLayout bindingLayout);
    BindingSet CreateBindingSet(BindingLayout bindingLayout,
                                u32 variableBindingSize);
    void UpdateBindingSets(u32 bindingWriteCount,
                           const WriteBindingSet* bindingWrites,
                           u32 bindingCopyCount,
                           const CopyBindingSet* bindingCopies);
    void DestroyBindingSet(BindingSet bindingSet);
    ObjectGroup CreateGraphicsObjectGroup(
        const GraphicsObjectGroupDescriptor* descriptor);
    Status UpdateGraphicsObjectGroup(
        ObjectGroup objectGroup,
        const GraphicsObjectGroupUpdateDescriptor* descriptor);
    ObjectGroup CreateComputeObjectGroup(
        const ComputeObjectGroupDescriptor* descriptor);
    Status UpdateComputeObjectGroup(
        ObjectGroup objectGroup,
        const ComputeObjectGroupUpdateDescriptor* descriptor);
    ObjectGroup CreateTransferObjectGroup(
        const TransferObjectGroupDescriptor* descriptor);
    ObjectGroup CreateClearObjectGroup(
        const ClearObjectGroupDescriptor* descriptor);
    ObjectGroup CreateBarrierObjectGroup(
        const BarrierObjectGroupDescriptor* descriptor);
    void DestroyObjectGroup(ObjectGroup objectGroup);
    DataGraph CreateGraphicsDataGraph(
        const GraphicsDataGraphDescriptor* descriptor);
    DataGraph CreateComputeDataGraph(
        const ComputeDataGraphDescriptor* descriptor);
    DataGraph CreateTransferDataGraph(
        const TransferDataGraphDescriptor* descriptor);
    void DestroyDataGraph(DataGraph dataGraph);
    SchedulingGraph CreateSchedulingGraph(
        const SchedulingGraphDescriptor* descriptor);
    void DestroySchedulingGraph(SchedulingGraph schedulingGraph);
    Timeline CreateTimeline(const TimelineDescriptor* descriptor);
    Status SignalTimelines(const TimelineSignalDescriptor* descriptor);
    Status WaitForTimelines(const TimelineWaitDescriptor* descriptor,
                            u64 timeout);
    Status GetTimelineValue(Timeline timeline, u64* value);
    Status ExportTimelineFd(const TimelineExportFdDescriptor* descriptor,
                            int* fd);
    Status ImportTimelineFd(const TimelineImportFdDescriptor* descriptor);
    Status ImportTimelineEventId(
        const TimelineImportEventIdDescriptor* descriptor);
    Status ResetTimeline(Timeline timeline, u64 initialValue);
    void DestroyTimeline(Timeline timeline);
    RenderTarget CreateRenderTarget(const RenderTargetDescriptor* descriptor);
    void DestroyRenderTarget(RenderTarget renderTarget);
    DisplaySurface CreateDisplaySurface(
        const DisplaySurfaceDescriptor* descriptor);
    Status GetDisplaySurfaceCapabilities(
        DisplaySurface displaySurface,
        DisplaySurfaceCapabilities* capabilities);
    Status GetDisplaySurfaceFormats(DisplaySurface displaySurface,
                                    u32* formatCount,
                                    DisplaySurfaceFormat* formats);
    Status GetDisplaySurfacePresentModes(DisplaySurface displaySurface,
                                         u32* presentModeCount,
                                         PresentMode* presentModes);
    void DestroyDisplaySurface(DisplaySurface displaySurface);
    Swapchain CreateSwapchain(const SwapchainDescriptor* descriptor);
    Status AcquireNextImage(Swapchain swapchain, u64 timeout, Timeline timeline,
                            u64 timelineValue, u32* imageIndex);
    Status GetSwapchainImages(Swapchain swapchain, u32* imageCount,
                              Resource* imageResources);
    void DestroySwapchain(Swapchain swapchain);
    Status GetSwapchainGrallocUsageOHOS(Format format,
                                        ImageUsageFlags imageUsage,
                                        u64* grallocUsage);
    Status AcquireImageOHOS(Resource resource, s32 nativeFenceFd,
                            Timeline timeline, u64 timelineValue);
    PrivateDataSlot CreatePrivateDataSlot(
        const PrivateDataSlotDescriptor* descriptor);
    void DestroyPrivateDataSlot(PrivateDataSlot privateDataSlot);
    Status SetPrivateData(const PrivateDataSetDescriptor* descriptor);
    Status GetPrivateData(const PrivateDataGetDescriptor* descriptor);
    ObjectGroup CreateAccelerationStructureObjectGroup(
        const AccelerationStructureObjectGroupDescriptor* descriptor);
    DataGraph CreateAccelerationStructureDataGraph(
        const AccelerationStructureDataGraphDescriptor* descriptor);
    Status GetAccelerationStructureBuildSizes(
        const AccelerationStructureBuildGeometryDescriptor* buildGeometry,
        const u32* maxPrimitiveCounts,
        AccelerationStructureBuildSizesDescriptor* buildSizes);
    AccelerationStructure CreateAccelerationStructure(
        const AccelerationStructureDescriptor* descriptor);
    DeviceAddress GetAccelerationStructureDeviceAddress(
        AccelerationStructure accelerationStructure);
    void DestroyAccelerationStructure(
        AccelerationStructure accelerationStructure);
    Status GetRayTracingCapabilities(RayTracingCapabilities* capabilities);
    Status CheckAccelerationStructureCompatibility(
        const AccelerationStructureVersion* version,
        AccelerationStructureCompatibility* compatibility);
    QueryPool CreateQueryPool(const QueryPoolDescriptor* descriptor);
    void DestroyQueryPool(QueryPool queryPool);
    Status ResetQueryPool(const ResetQueryPoolDescriptor* descriptor);
    Status GetQueryPoolResult(const GetQueryPoolResultDescriptor* descriptor);
    Status GetGpuCounterProperties(Counter counter,
                                   GpuCounterProperties* properties);

private:
    ctype m_handle{};
};

// ===== Structs & Unions =====
struct Origin2D {
    s32 x;
    s32 y;
};

struct Origin3D {
    s32 x;
    s32 y;
    s32 z;
};

struct Size2D {
    u32 width;
    u32 height;
};

struct Size3D {
    u32 width;
    u32 height;
    u32 depth;
};

struct Rect2D {
    Origin2D origin;
    Size2D size;
};

struct ExtensionBlock {
    ExtensionKey key;
    const void* data;
};

struct LayerProperties {
    char layerName[MLN_MAX_LAYER_NAME_SIZE];
    u32 specVersion;
    u32 implementationVersion;
    char description[MLN_MAX_DESCRIPTION_SIZE];
};

struct ApplicationDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const char* applicationName;
    u32 applicationVersion;
    const char* engineName;
    u32 engineVersion;
};

struct QueueDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    QueueDescriptorFlags flags;
    QueuePriority priority;
};

struct DeviceDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceDescriptorFlags flags;
    const ApplicationDescriptor* appDescriptor;
    u32 queueDescriptorCount;
    const QueueDescriptor* queueDescriptors;
    u32 layerCount;
    const char* const * layerNames;
};

struct GpuLimits {
    u32 maxImageDimension1D;
    u32 maxImageDimension2D;
    u32 maxImageDimension3D;
    u32 maxImageDimensionCube;
    u32 maxImageArrayLayers;
    u32 maxTexelBufferElements;
    u32 maxUniformBufferRange;
    u32 maxStorageBufferRange;
    u32 maxProgramConstantsSize;
    u32 maxMemoryAllocationCount;
    u32 maxSamplerAllocationCount;
    DeviceSize bufferImageGranularity;
    u32 maxBoundBindingSets;
    u32 maxPerStageBindingSamplers;
    u32 maxPerStageBindingUniformBuffers;
    u32 maxPerStageBindingStorageBuffers;
    u32 maxPerStageBindingSampledImages;
    u32 maxPerStageBindingStorageImages;
    u32 maxPerStageBindingInputAttachments;
    u32 maxPerStageResources;
    u32 maxBindingSetSamplers;
    u32 maxBindingSetUniformBuffers;
    u32 maxBindingSetUniformBuffersDynamic;
    u32 maxBindingSetStorageBuffers;
    u32 maxBindingSetStorageBuffersDynamic;
    u32 maxBindingSetSampledImages;
    u32 maxBindingSetStorageImages;
    u32 maxBindingSetInputAttachments;
    u32 maxVertexInputAttributes;
    u32 maxVertexInputBindings;
    u32 maxVertexInputAttributeOffset;
    u32 maxVertexInputBindingStride;
    u32 maxVertexOutputComponents;
    u32 maxFragmentInputComponents;
    u32 maxFragmentOutputAttachments;
    u32 maxFragmentDualSrcAttachments;
    u32 maxFragmentCombinedOutputResources;
    u32 maxComputeSharedMemorySize;
    u32 maxComputeWorkGroupCount[3];
    u32 maxComputeWorkGroupInvocations;
    u32 subPixelPrecisionBits;
    u32 subTexelPrecisionBits;
    u32 mipmapPrecisionBits;
    u32 maxDrawIndexedIndexValue;
    u32 maxDrawIndirectCount;
    f32 maxSamplerLodBias;
    f32 maxSamplerAnisotropy;
    u32 maxViewports;
    u32 maxViewportDimensions[2];
    f32 viewportBoundsRange[2];
    u32 viewportSubPixelBits;
    size_t minMemoryMapAlignment;
    DeviceSize minTexelBufferOffsetAlignment;
    DeviceSize minUniformBufferOffsetAlignment;
    DeviceSize minStorageBufferOffsetAlignment;
    s32 minTexelOffset;
    u32 maxTexelOffset;
    s32 minTexelGatherOffset;
    u32 maxTexelGatherOffset;
    f32 minInterpolationOffset;
    f32 maxInterpolationOffset;
    u32 subPixelInterpolationOffsetBits;
    u32 maxRenderTargetWidth;
    u32 maxRenderTargetHeight;
    u32 maxRenderTargetLayers;
    u32 maxColorAttachments;
    u32 maxSampleMaskWords;
    b32 timestampComputeAndGraphics;
    f32 timestampPeriod;
    u32 maxClipDistances;
    u32 maxCullDistances;
    u32 maxCombinedClipAndCullDistances;
    u32 discreteQueuePriorities;
    f32 pointSizeRange[2];
    f32 lineWidthRange[2];
    f32 pointSizeGranularity;
    f32 lineWidthGranularity;
    b32 strictLines;
    b32 standardSampleLocations;
    DeviceSize optimalBufferCopyOffsetAlignment;
    DeviceSize optimalBufferCopyRowPitchAlignment;
    DeviceSize nonCoherentAtomSize;
    u32 queueCount;
};

struct GpuFeatures {
    b32 multiDrawIndirect;
    b32 drawIndirectFirstInstance;
    b32 samplerAnisotropy;
    b32 textureCompressionETC2;
    b32 textureCompressionASTC_LDR;
    b32 textureCompressionBC;
    b32 occlusionQueryPrecise;
    b32 programStatisticsQuery;
    b32 vertexProgramStoresAndAtomics;
    b32 fragmentStoresAndAtomics;
    b32 shaderImageGatherExtended;
    b32 shaderStorageImageExtendedFormats;
    b32 shaderStorageImageMultisample;
    b32 shaderStorageImageReadWithoutFormat;
    b32 shaderStorageImageWriteWithoutFormat;
    b32 shaderUniformBufferArrayDynamicIndexing;
    b32 shaderSampledImageArrayDynamicIndexing;
    b32 shaderStorageBufferArrayDynamicIndexing;
    b32 shaderStorageImageArrayDynamicIndexing;
    b32 shaderClipDistance;
    b32 shaderCullDistance;
    b32 shaderFloat64;
    b32 shaderInt64;
    b32 shaderInt16;
    b32 shaderResourceResidency;
    b32 shaderResourceMinLod;
    b32 variableMultisampleRate;
    b32 inheritedQueries;
};

struct RayTracingCapabilities {
    u64 maxGeometriesPerStructure;
    u64 maxInstancesPerStructure;
    u64 maxPrimitivesPerGeometry;
    u32 maxAccelerationStructuresPerStage;
    u32 maxAccelerationStructuresPerStageAfterUpdate;
    u32 maxAccelerationStructuresPerBindingSet;
    u32 maxAccelerationStructuresPerBindingSetAfterUpdate;
    u32 minScratchOffsetAlignment;
    u32 shaderGroupHandleByteSize;
    u32 maxRecursionDepth;
    u32 maxShaderGroupByteStride;
    u32 shaderGroupBaseByteAlignment;
    u32 shaderGroupHandleCaptureReplayByteSize;
    u32 maxTraceRaysInvocations;
    u32 shaderGroupHandleByteAlignment;
    u32 maxHitAttributeByteSize;
    u32 maxMicromapOpacity2StateLevel;
    u32 maxMicromapOpacity4StateLevel;
};

struct GpuProperties {
    u32 driverVersion;
    u8 programCacheUUID[MLN_UUID_SIZE];
    GpuLimits limits;
};

struct GpuFormatProperties {
    FormatFeatureFlags linearTilingFeatures;
    FormatFeatureFlags optimalTilingFeatures;
    FormatFeatureFlags bufferFeatures;
};

struct GpuImageFormatProperties {
    u32 maxWidth;
    u32 maxHeight;
    u32 maxDepth;
    u32 maxArrayLayers;
    SampleCountFlags sampleCounts;
    DeviceSize maxResourceSize;
    DeviceSize optimalTilingAlignment;
    DeviceSize linearTilingAlignment;
};

struct GpuFragmentShadingRate {
    SampleCountFlags sampleCounts;
    Size2D fragmentSize;
};

struct GpuFragmentShadingRateFeatures {
    b32 primitiveFragmentShadingRate;
};

struct GpuFragmentShadingRateProperties {
    Size2D minFragmentShadingRateAttachmentTexelSize;
    Size2D maxFragmentShadingRateAttachmentTexelSize;
    u32 maxFragmentShadingRateAttachmentTexelSizeAspectRatio;
    b32 primitiveFragmentShadingRateWithMultipleViewports;
    Size2D maxFragmentSize;
    u32 maxFragmentSizeAspectRatio;
    u32 maxFragmentShadingRateCoverageSamples;
    SampleCountFlagBits maxFragmentShadingRateRasterizationSamples;
    b32 fragmentShadingRateWithConservativeRasterization;
    b32 fragmentShadingRateWithFragmentShaderInterlock;
};

struct MemoryType {
    MemoryPropertyFlags propertyFlags;
    u32 heapIndex;
};

struct MemoryHeap {
    DeviceSize size;
    MemoryHeapFlags flags;
};

struct GpuMemoryProperties {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 memoryTypeCount;
    MemoryType memoryTypes[MLN_MAX_GPU_MEMORY_TYPES];
    u32 memoryHeapCount;
    MemoryHeap memoryHeaps[MLN_MAX_GPU_MEMORY_HEAPS];
};

struct ExternalMemoryDescriptor {
    ExternalMemoryHandleType type;
    OH_NativeBuffer* nativeBuffer;
};

struct MemoryAllocateDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceSize allocationSize;
    u32 memoryTypeIndex;
    ExternalMemoryDescriptor* externalMemoryDescriptor;
    Resource dedicatedResource;
    MemoryPolicy policy;
};

struct ResourceMemoryRequirementsDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource resource;
    ImageAspectFlags aspectMask;
};

struct BindResourceMemoryDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource resource;
    ImageAspectFlags aspectMask;
    DeviceMemory memory;
    DeviceSize offset;
};

struct MemoryDedicatedRequirements {
    b32 prefersDedicatedAllocation;
    b32 requiresDedicatedAllocation;
};

struct MemoryRequirements {
    DeviceSize size;
    DeviceSize alignment;
    u32 memoryTypeBits;
    MemoryDedicatedRequirements memoryDedicatedRequirements;
};

struct MappedMemoryRangeDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceMemory memory;
    DeviceSize offset;
    DeviceSize size;
};

struct MemoryMapDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    MemoryMapDescriptorFlags flags;
    DeviceMemory memory;
    DeviceSize offset;
    DeviceSize size;
};

struct MemoryUnmapDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    MemoryUnmapDescriptorFlags flags;
    DeviceMemory memory;
};

struct ValidationLayer {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ValidationLayerFlags flags;
    ValidationLayerLogType logType;
    char logFile[MLN_MAX_VALIDATION_LOG_FILE_PATH_SIZE];
    ValidationLayerReportLevel reportLevel;
    ValidationLayerCheckActionType actionType;
    ValidationLayerCheckOptionFlags options;
};

struct BufferDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    BufferDescriptorFlags flags;
    DeviceSize size;
    BufferUsageFlags usage;
    ExternalMemoryHandleType externalMemType;
    const void* externalMemory;
};

struct ImageFormatList {
    u32 formatCount;
    const Format* formats;
};

struct ImageDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ImageDescriptorFlags flags;
    ImageType imageType;
    Format format;
    Size3D size;
    u32 mipLevels;
    u32 arrayLayers;
    SampleCountFlags samples;
    ImageTiling tiling;
    ImageUsageFlags usage;
    ImageLayout initialLayout;
    const ImageFormatList* formatList;
    ImageUsageFlags stencilUsage;
    ExternalMemoryHandleType externalMemType;
    const void* externalMemory;
};

struct SamplerDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    SamplerDescriptorFlags flags;
    Filter magFilter;
    Filter minFilter;
    SamplerMipmapMode mipmapMode;
    SamplerAddressMode addressModeU;
    SamplerAddressMode addressModeV;
    SamplerAddressMode addressModeW;
    f32 mipLodBias;
    b32 anisotropyEnable;
    f32 maxAnisotropy;
    b32 compareEnable;
    CompareOp compareOp;
    f32 minLod;
    f32 maxLod;
    BorderColor borderColor;
    b32 unnormalizedCoordinates;
    SamplerReductionMode reductionMode;
    SamplerYcbcrConversion conversion;
};

struct ComponentMapping {
    ComponentSwizzle r;
    ComponentSwizzle g;
    ComponentSwizzle b;
    ComponentSwizzle a;
};

struct SamplerYcbcrConversionDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    SamplerYcbcrConversionDescriptorFlags flags;
    Format format;
    SamplerYcbcrModelConversion ycbcrModel;
    SamplerYcbcrRange ycbcrRange;
    ComponentMapping components;
    ChromaLocation xChromaOffset;
    ChromaLocation yChromaOffset;
    Filter chromaFilter;
    b32 forceExplicitReconstruction;
};

struct NativeBufferFormatProperties {
    Format format;
    u32 externalFormat;
    FormatFeatureFlags formatFeatures;
    ComponentMapping components;
    SamplerYcbcrModelConversion ycbcrModel;
    SamplerYcbcrRange ycbcrRange;
    ChromaLocation xChromaOffset;
    ChromaLocation yChromaOffset;
};

struct NativeBufferProperties {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceSize allocationSize;
    u32 memoryTypeBits;
    NativeBufferFormatProperties* formatProperties;
};

struct MemoryGetNativeBufferDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceMemory memory;
};

struct BufferViewDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    BufferViewDescriptorFlags flags;
    Resource bufferResource;
    Format format;
    DeviceSize offset;
    DeviceSize range;
};

struct ImageSubresourceRange {
    ImageAspectFlags aspectMask;
    u32 baseMipLevel;
    u32 levelCount;
    u32 baseArrayLayer;
    u32 layerCount;
};

struct ImageViewDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ImageViewDescriptorFlags flags;
    Resource imageResource;
    ImageViewType viewType;
    Format format;
    ComponentMapping components;
    ImageSubresourceRange subresourceRange;
    Format decodeMode;
    f32 minLod;
    SamplerYcbcrConversion conversion;
};

struct ShaderDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ShaderDescriptorFlags flags;
    size_t sourceSize;
    const u32* source;
};

struct ProgramCacheDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ProgramCacheDescriptorFlags flags;
    size_t rawDataSize;
    const void* rawData;
};

struct ProgramInterfaceDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ProgramInterfaceDescriptorFlags flags;
    u32 bindingLayoutCount;
    const BindingLayout* bindingLayouts;
};

struct CompileTimeConstantMapEntry {
    u32 constantID;
    u32 offset;
    size_t size;
};

struct CompileTimeConstantState {
    u32 mapEntryCount;
    const CompileTimeConstantMapEntry* mapEntries;
    size_t dataSize;
    const void* data;
};

struct ShaderStageState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ShaderStageStateFlags flags;
    ShaderStageFlags stage;
    const ShaderDescriptor* shaderDescriptor;
    const char* name;
    const CompileTimeConstantState* compileTimeConstant;
};

struct VertexInputBindingState {
    u32 binding;
    u32 stride;
    VertexInputRate inputRate;
    u32 divisor;
};

struct VertexInputAttributeState {
    u32 location;
    u32 binding;
    Format format;
    u32 offset;
};

struct VertexInputState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    VertexInputStateFlags flags;
    u32 vertexBindingStateCount;
    const VertexInputBindingState* vertexBindingStates;
    u32 vertexAttributeStateCount;
    const VertexInputAttributeState* vertexAttributeStates;
};

struct InputAssemblyState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    InputAssemblyStateFlags flags;
    PrimitiveTopology topology;
    b32 primitiveRestartEnable;
};

struct Viewport {
    f32 x;
    f32 y;
    f32 width;
    f32 height;
    f32 minDepth;
    f32 maxDepth;
};

struct ViewportState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ViewportStateFlags flags;
    u32 viewportCount;
    const Viewport* viewports;
    u32 scissorCount;
    const Rect2D* scissors;
};

struct DepthBiasRepresentation {
    DepthBiasRepresentationMode mode;
    b32 depthBiasExact;
};

struct RasterizationState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    RasterizationStateFlags flags;
    b32 depthClampEnable;
    b32 rasterizerDiscardEnable;
    PolygonMode polygonMode;
    CullModeFlags cullMode;
    FrontFace frontFace;
    b32 depthBiasEnable;
    f32 depthBiasConstantFactor;
    f32 depthBiasClamp;
    f32 depthBiasSlopeFactor;
    f32 lineWidth;
    b32 depthClipEnable;
    ProvokingVertexMode provokingVertexMode;
    LineRasterizationMode lineRasterizationMode;
    const DepthBiasRepresentation* depthBiasRepresentation;
};

struct SampleLocation {
    f32 x;
    f32 y;
};

struct SampleLocationDynamicState {
    SampleCountFlagBits sampleLocationsPerPixel;
    Size2D sampleLocationGridSize;
    u32 sampleLocationCount;
    const SampleLocation* sampleLocations;
};

struct MultisampleState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    MultisampleStateFlags flags;
    SampleCountFlagBits rasterizationSamples;
    b32 sampleShadingEnable;
    f32 minSampleShading;
    const SampleMask* sampleMask;
    b32 alphaToCoverageEnable;
    b32 alphaToOneEnable;
    const SampleLocationDynamicState* sampleLocation;
};

struct StencilOpState {
    StencilOp failOp;
    StencilOp passOp;
    StencilOp depthFailOp;
    CompareOp compareOp;
    u32 compareMask;
    u32 writeMask;
    u32 reference;
};

struct DepthStencilState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DepthStencilStateFlags flags;
    b32 depthTestEnable;
    b32 depthWriteEnable;
    CompareOp depthCompareOp;
    b32 depthBoundsTestEnable;
    b32 stencilTestEnable;
    StencilOpState front;
    StencilOpState back;
    f32 minDepthBounds;
    f32 maxDepthBounds;
};

struct ColorBlendAttachmentState {
    b32 blendEnable;
    BlendFactor srcColorBlendFactor;
    BlendFactor dstColorBlendFactor;
    BlendOp colorBlendOp;
    BlendFactor srcAlphaBlendFactor;
    BlendFactor dstAlphaBlendFactor;
    BlendOp alphaBlendOp;
    ColorComponentFlags colorWriteMask;
};

struct ColorBlendState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ColorBlendStateFlags flags;
    b32 logicOpEnable;
    LogicOp logicOp;
    u32 attachmentCount;
    const ColorBlendAttachmentState* attachments;
    f32 blendConstants[4];
};

struct RenderingState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    RenderingStateFlags flags;
    u32 viewMask;
    u32 colorAttachmentCount;
    const Format* colorAttachmentFormats;
    const u32* colorAttachmentLocations;
    const u32* colorAttachmentInputIndices;
    Format depthAttachmentFormat;
    Format stencilAttachmentFormat;
    u32 depthInputAttachmentIndex;
    u32 stencilInputAttachmentIndex;
};

struct DynamicState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DynamicStateFlags flags;
    u32 dynamicStateCount;
    const DynamicStateType* dynamicStateTypes;
};

struct FragmentShadingRateState {
    Size2D fragmentSize;
    FragmentShadingRateCombinerOp combinerOps[2];
};

struct CreationFeedback {
    CreationFeedbackFlags flags;
    u64 duration;
};

struct CreationFeedbackState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    CreationFeedback* feedback;
    u32 stageFeedbackCount;
    CreationFeedback* stageFeedbacks;
};

struct GraphicsProgramState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ProgramStateFlags flags;
    u32 stageCount;
    const ShaderStageState* stages;
    const VertexInputState* vertexInputState;
    const InputAssemblyState* inputAssemblyState;
    const ViewportState* viewportState;
    const RasterizationState* rasterizationState;
    const MultisampleState* multisampleState;
    const DepthStencilState* depthStencilState;
    const ColorBlendState* colorBlendState;
    const RenderingState* renderState;
    const DynamicState* dynamicState;
    const FragmentShadingRateState* fragmentShadingRateState;
    ProgramInterface programInterface;
    const CreationFeedbackState* feedbackState;
};

struct ComputeProgramState {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ProgramStateFlags flags;
    ShaderStageState stage;
    ProgramInterface programInterface;
    const CreationFeedbackState* feedbackState;
};

struct BindingSlot {
    u32 slot;
    BindingType type;
    u32 slotCount;
    ShaderStageFlags stageFlags;
    const Sampler* immutableSamplers;
};

struct BindingLayoutDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    BindingLayoutDescriptorFlags flags;
    u32 bindingCount;
    const BindingSlot* bindings;
    const BindingSlotFlags* bindingFlags;
};

struct BindingBufferDescriptor {
    Resource bufferResource;
    DeviceSize offset;
    DeviceSize range;
};

struct BindingImageDescriptor {
    Sampler sampler;
    ResourceView imageResourceView;
    ImageLayout imageLayout;
};

struct BindingInlineUniformDescriptor {
    u32 dataSize;
    const void* data;
};

struct WriteBindingSet {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    BindingSet dstSet;
    u32 dstBinding;
    u32 dstArrayElement;
    u32 bindingCount;
    BindingType bindingType;
    const BindingImageDescriptor* imageDescriptor;
    const BindingBufferDescriptor* bufferDescriptor;
    const ResourceView* texelBufferResourceView;
    const BindingInlineUniformDescriptor* inlineUniformDescriptor;
    const AccelerationStructure* accelerationStructure;
};

struct CopyBindingSet {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    BindingSet srcSet;
    u32 srcBinding;
    u32 srcArrayElement;
    BindingSet dstSet;
    u32 dstBinding;
    u32 dstArrayElement;
    u32 bindingCount;
};

struct ResourceBindingSets {
    u32 firstSet;
    u32 bindingSetCount;
    const BindingSet* bindingSets;
    u32 dynamicOffsetCount;
    const u32* dynamicOffsets;
};

struct ProgramConstants {
    u32 offset;
    u32 size;
    const void* values;
};

struct ResourceProgramConstants {
    u32 programConstantCount;
    const ProgramConstants* programConstants;
};

struct ResourceVertexBuffers {
    u32 firstBinding;
    u32 bindingCount;
    const Resource* bufferResources;
    const DeviceSize* offsets;
    const DeviceSize* sizes;
    const DeviceSize* strides;
};

struct ResourceIndexBuffer {
    Resource bufferResource;
    DeviceSize offset;
    DeviceSize size;
    IndexType indexType;
};

struct ResourceSetDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ResourceSetDescriptorFlags flags;
    u32 dynamicResourceCount;
    const DynamicResourceType* dynamicResource;
    const ResourceBindingSets* resourceBindingSets;
    const ResourceProgramConstants* resourceProgramConstants;
    const ResourceVertexBuffers* resourceVertexBuffers;
    const ResourceIndexBuffer* resourceIndexBuffer;
};

struct ViewportDynamicState {
    u32 firstViewport;
    u32 viewportCount;
    const Viewport* viewports;
};

struct ScissorDynamicState {
    u32 firstScissor;
    u32 scissorCount;
    const Rect2D* scissors;
};

struct DepthBoundsDynamicState {
    f32 minDepthBounds;
    f32 maxDepthBounds;
};

struct DepthBiasDynamicState {
    f32 depthBiasConstantFactor;
    f32 depthBiasClamp;
    f32 depthBiasSlopeFactor;
};

struct StencilOpDynamicState {
    StencilFaceFlags faceMask;
    StencilOp failOp;
    StencilOp passOp;
    StencilOp depthFailOp;
    CompareOp compareOp;
};

struct StencilCompareMaskDynamicState {
    StencilFaceFlags faceMask;
    u32 compareMask;
};

struct StencilWriteMaskDynamicState {
    StencilFaceFlags faceMask;
    u32 writeMask;
};

struct StencilReferenceDynamicState {
    StencilFaceFlags faceMask;
    u32 reference;
};

struct FragmentShadingRateDynamicState {
    Size2D fragmentSize;
    FragmentShadingRateCombinerOp combinerOps[2];
};

struct StateSetDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    StateSetDescriptorFlags flags;
    const DynamicState* dynamicState;
    const ViewportDynamicState* viewport;
    const ScissorDynamicState* scissor;
    f32 lineWidth;
    const DepthBiasDynamicState* depthBias;
    f32 blendConstants[4];
    const DepthBoundsDynamicState* depthBounds;
    const StencilOpDynamicState* stencilOp;
    const StencilCompareMaskDynamicState* compareMask;
    const StencilWriteMaskDynamicState* writeMask;
    const StencilReferenceDynamicState* reference;
    const FragmentShadingRateDynamicState* fragmentShadingRate;
    PrimitiveTopology topology;
    b32 primitiveRestartEnable;
    b32 rasterizerDiscardEnable;
    CullModeFlags cullMode;
    FrontFace frontFace;
    b32 depthBiasEnable;
    const SampleLocationDynamicState* sampleLocation;
    b32 depthTestEnable;
    b32 depthWriteEnable;
    CompareOp depthCompareOp;
    b32 depthBoundsTestEnable;
    b32 stencilTestEnable;
    u32 viewportCount;
    u32 scissorCount;
    const DeviceSize* bindingStride;
};

struct DrawCommand {
    u32 vertexCount;
    u32 instanceCount;
    u32 firstVertex;
    u32 firstInstance;
};

struct DrawIndexedCommand {
    u32 indexCount;
    u32 instanceCount;
    u32 firstIndex;
    u32 vertexOffset;
    u32 firstInstance;
};

struct DrawIndirectCommand {
    Resource bufferResource;
    DeviceSize offset;
    u32 drawCount;
    u32 stride;
};

struct DrawIndexedIndirectCommand {
    Resource bufferResource;
    DeviceSize offset;
    u32 drawCount;
    u32 stride;
};

struct DrawIndirectCountCommand {
    Resource bufferResource;
    DeviceSize offset;
    Resource countBufferResource;
    DeviceSize countBufferOffset;
    u32 maxDrawCount;
    u32 stride;
};

struct DrawIndexedIndirectCountCommand {
    Resource bufferResource;
    DeviceSize offset;
    Resource countBufferResource;
    DeviceSize countBufferOffset;
    u32 maxDrawCount;
    u32 stride;
};

union GraphicsCommandData {
    const DrawCommand* draw;
    const DrawIndexedCommand* drawIndexed;
    const DrawIndirectCommand* drawIndirect;
    const DrawIndexedIndirectCommand* drawIndexedIndirect;
    const DrawIndirectCountCommand* drawIndirectCount;
    const DrawIndexedIndirectCountCommand* drawIndexedIndirectCount;
};

struct GraphicsCommand {
    GraphicsCommandType type;
    GraphicsCommandData data;
};

struct GraphicsCommandDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    GraphicsCommandDescriptorFlags flags;
    const GraphicsCommand* command;
    const ResourceSetDescriptor* resourceSet;
    const StateSetDescriptor* stateSet;
};

struct ResourceBindingSetsInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 firstSet;
    u32 bindingSetCount;
};

struct ResourceVertexBuffersInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 firstBinding;
    u32 bindingCount;
};

struct ResourceIndexBufferInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
};

struct StateViewportsInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 firstViewport;
    u32 viewportCount;
};

struct StateScissorsInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 firstScissor;
    u32 scissorCount;
};

struct FragmentShadingRateInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
};

struct GraphicsObjectGroupInheritanceDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const ResourceBindingSetsInheritance* bindingSet;
    const ResourceVertexBuffersInheritance* vertexBuffer;
    const ResourceIndexBufferInheritance* indexBuffer;
    const StateViewportsInheritance* viewport;
    const StateScissorsInheritance* scissor;
    const FragmentShadingRateInheritance* fragmentShadingRate;
};

struct GraphicsObjectGroupDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    GraphicsObjectGroupDescriptorFlags flags;
    Program program;
    const ResourceSetDescriptor* resourceSet;
    const StateSetDescriptor* stateSet;
    u32 commandCount;
    const GraphicsCommandDescriptor* commands;
    const GraphicsObjectGroupInheritanceDescriptor* inheritance;
};

struct BindingSetsDynamicOffsets {
    u32 firstSet;
    u32 dynamicOffsetCount;
    const u32* dynamicOffsets;
};

union GraphicsObjectGroupModifierData {
    const ResourceBindingSets* bindingSet;
    const ResourceProgramConstants* programConstant;
    const ResourceVertexBuffers* vertexBuffer;
    const ResourceIndexBuffer* indexBuffer;
    const DrawCommand* draw;
    const DrawIndexedCommand* drawIndexed;
    const DrawIndirectCommand* drawIndirect;
    const DrawIndexedIndirectCommand* drawIndexedIndirect;
    const DrawIndirectCountCommand* drawIndirectCount;
    const DrawIndexedIndirectCountCommand* drawIndexedIndirectCount;
    const ViewportDynamicState* viewport;
    const ScissorDynamicState* scissor;
    f32 lineWidth;
    const DepthBiasDynamicState* depthBias;
    f32 blendConstants[4];
    const DepthBoundsDynamicState* depthBound;
    const StencilCompareMaskDynamicState* stencilCompareMask;
    const StencilWriteMaskDynamicState* stencilWriteMask;
    const StencilReferenceDynamicState* stencilReference;
    const BindingSetsDynamicOffsets* dynamicOffsets;
    const SampleLocationDynamicState* sampleLocation;
    u32 viewportCount;
    u32 scissorCount;
    const FragmentShadingRateDynamicState* fragmentShadingRate;
};

struct GraphicsObjectGroupModifierContent {
    GraphicsObjectGroupModifierType type;
    GraphicsObjectGroupModifierData data;
};

struct GraphicsObjectGroupModifier {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 commandIndex;
    u32 contentCount;
    const GraphicsObjectGroupModifierContent* contents;
};

struct GraphicsObjectGroupUpdateDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    GraphicsObjectGroupUpdateDescriptorFlags flags;
    u32 modifierCount;
    const GraphicsObjectGroupModifier* modifiers;
};

struct ComputeBaseGroup {
    u32 baseGroupX;
    u32 baseGroupY;
    u32 baseGroupZ;
};

struct ComputeGroupCount {
    u32 groupCountX;
    u32 groupCountY;
    u32 groupCountZ;
};

struct ComputeGroupCountIndirect {
    Resource bufferResource;
    DeviceSize offset;
};

union ComputeCommandData {
    const ComputeGroupCount* groupCount;
    const ComputeGroupCountIndirect* groupCountIndirect;
};

struct ComputeCommand {
    const ComputeBaseGroup* baseGroup;
    ComputeCommandType type;
    ComputeCommandData data;
};

struct ComputeCommandDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ComputeCommandDescriptorFlags flags;
    const ComputeCommand* command;
    const ResourceBindingSets* bindingSet;
    const ResourceProgramConstants* programConstant;
};

struct ComputeObjectGroupInheritanceDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const ResourceBindingSetsInheritance* bindingSet;
};

struct ComputeObjectGroupDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ComputeObjectGroupDescriptorFlags flags;
    Program program;
    const ResourceBindingSets* bindingSet;
    const ResourceProgramConstants* programConstant;
    u32 commandCount;
    const ComputeCommandDescriptor* commands;
    const ComputeObjectGroupInheritanceDescriptor* inheritance;
};

union ComputeObjectGroupModifierData {
    const ResourceBindingSets* bindingSet;
    const ResourceProgramConstants* programConstant;
    const ComputeBaseGroup* baseGroup;
    const ComputeGroupCount* groupCount;
    const ComputeGroupCountIndirect* groupCountIndirect;
    const BindingSetsDynamicOffsets* dynamicOffsets;
};

struct ComputeObjectGroupModifierContent {
    ComputeObjectGroupModifierType type;
    ComputeObjectGroupModifierData data;
};

struct ComputeObjectGroupModifier {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 commandIndex;
    u32 contentCount;
    const ComputeObjectGroupModifierContent* contents;
};

struct ComputeObjectGroupUpdateDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ComputeObjectGroupUpdateDescriptorFlags flags;
    u32 modifierCount;
    const ComputeObjectGroupModifier* modifiers;
};

struct BufferCopyRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceSize srcOrigin;
    DeviceSize dstOrigin;
    DeviceSize size;
};

struct CopyBufferDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource srcBufferResource;
    Resource dstBufferResource;
    u32 regionCount;
    const BufferCopyRegion* regions;
};

struct FillBufferDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource bufferResource;
    DeviceSize dstOrigin;
    DeviceSize size;
    u32 data;
};

struct UpdateBufferDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource bufferResource;
    DeviceSize dstOrigin;
    DeviceSize size;
    const void* data;
};

struct ImageSubresourceLayers {
    ImageAspectFlags aspectMask;
    u32 mipLevel;
    u32 baseArrayLayer;
    u32 layerCount;
};

struct BufferImageCopyRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceSize bufferOrigin;
    u32 bufferRowLength;
    u32 bufferImageHeight;
    ImageSubresourceLayers imageSubresource;
    Origin3D imageOrigin;
    Size3D imageSize;
};

struct CopyBufferToImageDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource srcBufferResource;
    Resource dstImageResource;
    u32 regionCount;
    const BufferImageCopyRegion* regions;
};

struct ImageCopyRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ImageSubresourceLayers srcSubresource;
    Origin3D srcOrigin;
    ImageSubresourceLayers dstSubresource;
    Origin3D dstOrigin;
    Size3D size;
};

struct CopyImageDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource srcImageResource;
    Resource dstImageResource;
    u32 regionCount;
    const ImageCopyRegion* regions;
};

struct CopyImageToBufferDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource srcImageResource;
    Resource dstBufferResource;
    u32 regionCount;
    const BufferImageCopyRegion* regions;
};

struct ImageBlitRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ImageSubresourceLayers srcSubresource;
    Origin3D srcOrigins[2];
    ImageSubresourceLayers dstSubresource;
    Origin3D dstOrigins[2];
};

struct BlitImageDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource srcImageResource;
    Resource dstImageResource;
    u32 regionCount;
    const ImageBlitRegion* regions;
    Filter filter;
};

struct ImageResolveRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ImageSubresourceLayers srcSubresource;
    Origin3D srcOrigin;
    ImageSubresourceLayers dstSubresource;
    Origin3D dstOrigin;
    Size3D size;
};

struct ResolveImageDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource srcImageResource;
    Resource dstImageResource;
    u32 regionCount;
    const ImageResolveRegion* regions;
};

struct ImageGenMipMapRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ImageSubresourceLayers srcSubresource;
    ImageSubresourceLayers dstSubresource;
    ComponentMapping componentMapping;
    u32 needPatchComponentMapping;
    u32 srgbSkipDecode;
};

struct GenerateMipmapDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource imageResource;
    u32 regionCount;
    const ImageGenMipMapRegion* regions;
};

struct ClearImageRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ImageAspectFlags aspectMask;
    u32 baseMipLevel;
    u32 levelCount;
    u32 baseArrayLayer;
    u32 layerCount;
};

union ClearColor {
    f32 f[4];
    s32 s[4];
    u32 u[4];
};

struct ClearDepthStencil {
    f32 depth;
    u32 stencil;
};

union ClearValue {
    ClearColor color;
    ClearDepthStencil depthStencil;
};

struct ClearImageDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Resource imageResource;
    ClearValue clearValue;
    u32 regionCount;
    const ClearImageRegion* regions;
};

struct QueryPoolResultCopyRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 firstQuery;
    u32 queryCount;
    DeviceSize dstOrigin;
    DeviceSize stride;
};

struct CopyQueryPoolResultDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    QueryPool srcQueryPool;
    Resource dstBufferResource;
    u32 regionCount;
    const QueryPoolResultCopyRegion* regions;
};

struct QueryPoolResetRegion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 firstQuery;
    u32 queryCount;
};

struct ResetQueryPoolDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    QueryPool queryPool;
    u32 regionCount;
    const QueryPoolResetRegion* regions;
};

union TransferCommandData {
    const CopyBufferDescriptor* copyBufferDesc;
    const CopyImageDescriptor* copyImageDesc;
    const CopyBufferToImageDescriptor* copyBufferToImageDesc;
    const CopyImageToBufferDescriptor* copyImageToBufferDesc;
    const BlitImageDescriptor* blitImageDesc;
    const ResolveImageDescriptor* resolveImageDesc;
    const GenerateMipmapDescriptor* generateMipmapDesc;
    const ClearImageDescriptor* clearImageDesc;
    const FillBufferDescriptor* fillBufferDesc;
    const UpdateBufferDescriptor* updateBufferDesc;
    const CopyQueryPoolResultDescriptor* copyQueryPoolResultDesc;
    const ResetQueryPoolDescriptor* resetQueryPoolDesc;
};

struct TransferCommandDescriptor {
    TransferCommandType type;
    TransferCommandData data;
};

struct TransferObjectGroupDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    TransferObjectGroupDescriptorFlags flags;
    u32 commandCount;
    const TransferCommandDescriptor* commands;
};

struct ClearAttachment {
    ImageAspectFlags aspectMask;
    u32 colorAttachment;
    ClearValue clearValue;
};

struct ClearRect {
    Rect2D rect;
    u32 baseArrayLayer;
    u32 layerCount;
};

struct ClearObjectGroupDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ClearObjectGroupDescriptorFlags flags;
    u32 attachmentCount;
    const ClearAttachment* attachments;
    u32 rectCount;
    const ClearRect* rects;
};

struct MemoryBarrier {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ProgramStageFlags srcStageMask;
    AccessFlags srcAccessMask;
    ProgramStageFlags dstStageMask;
    AccessFlags dstAccessMask;
};

struct BufferMemoryBarrier {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ProgramStageFlags srcStageMask;
    AccessFlags srcAccessMask;
    ProgramStageFlags dstStageMask;
    AccessFlags dstAccessMask;
    Resource bufferResource;
    DeviceSize offset;
    DeviceSize size;
};

struct ImageMemoryBarrier {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ProgramStageFlags srcStageMask;
    AccessFlags srcAccessMask;
    ProgramStageFlags dstStageMask;
    AccessFlags dstAccessMask;
    ImageLayout oldLayout;
    ImageLayout newLayout;
    Resource imageResource;
    ImageSubresourceRange subresourceRange;
};

struct BarrierObjectGroupDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    BarrierObjectGroupDescriptorFlags flags;
    u32 memoryBarrierCount;
    const MemoryBarrier* memoryBarriers;
    u32 bufferMemoryBarrierCount;
    const BufferMemoryBarrier* bufferMemoryBarriers;
    u32 imageMemoryBarrierCount;
    const ImageMemoryBarrier* imageMemoryBarriers;
};

struct GraphicsDataGraphDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    GraphicsDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const ObjectGroup* objectGroups;
    RenderTarget renderTarget;
};

struct ComputeDataGraphDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ComputeDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const ObjectGroup* objectGroups;
};

struct TransferDataGraphDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    TransferDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const ObjectGroup* objectGroups;
};

struct AccelerationStructureDataGraphDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    AccelerationStructureDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const ObjectGroup* objectGroups;
};

struct QueryRangeCollection {
    u32 beginObjectGroupIndex;
    u32 beginCommandIndex;
    u32 endObjectGroupIndex;
    u32 endCommandIndex;
};

struct QueryPointCollection {
    u32 objectGroupIndex;
    u32 commandIndex;
    ProgramStageFlags programStageFlags;
};

struct QuerySampleCollection {
    QuerySampleCollectionLocation location;
    QuerySampleCollectionMode mode;
};

struct QueryObjectPropertyCollection {
    QuerySampleCollectionLocation location;
    u64 objectHandle;
};

union QueryCollectionData {
    const QueryRangeCollection* range;
    const QueryPointCollection* point;
    const QuerySampleCollection* sample;
    const QueryObjectPropertyCollection* objectProperty;
};

struct DataGraphQueryDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    QueryPool queryPool;
    u32 query;
    CounterSet counterSet;
    QueryCollectionType collectionType;
    QueryCollectionData data;
};

struct ConditionalRenderingRange {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ConditionalRenderingFlags flags;
    Resource buffer;
    DeviceSize offset;
    u32 beginObjectGroupIndex;
    u32 beginCommandIndex;
    u32 endObjectGroupIndex;
    u32 endCommandIndex;
};

struct PassNodeResourceDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    PassNodeResourceType type;
    ResourceView imageResourceView;
    Resource bufferResource;
};

struct PassNodeDependencyDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u64 passId;
    u32 depResourceCount;
    const PassNodeResourceDescriptor* depResources;
    ProgramStageFlags srcStage;
    ProgramStageFlags dstStage;
    u32 filterMargin;
};

struct PassNodeWorkloadDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    PassNodeHintType hint;
    PassNodeWorkloadType type;
    u32 workload;
};

struct PassNodeDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    PassNodeDescriptorFlags flags;
    u64 passId;
    u32 outputCount;
    const PassNodeResourceDescriptor* outputResources;
    const AttachmentStoreOp* storeop;
    u32 depCount;
    const PassNodeDependencyDescriptor* depPasses;
    const PassNodeWorkloadDescriptor* workload;
};

struct CameraDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const f32* viewMatrix;
    const f32* projectionMatrix;
};

struct SchedulingGraphDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    SchedulingGraphDescriptorFlags flags;
    u32 passNodeCount;
    const PassNodeDescriptor* passNodes;
    const CameraDescriptor* camera;
};

struct GraphicsObjectGroupInheritanceContent {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const ResourceBindingSets* bindingSets;
    const ResourceVertexBuffers* vertexbuffers;
    const ResourceIndexBuffer* indexbuffer;
    const ViewportDynamicState* viewport;
    const ScissorDynamicState* scissor;
    const FragmentShadingRateDynamicState* fragmentShadingRate;
};

struct GraphicsObjectGroupInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const GraphicsObjectGroupInheritanceContent* content;
    u32 objectGroupCount;
    const ObjectGroup* objectGroups;
};

struct GraphicsDataGraphInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const GraphicsObjectGroupInheritanceContent* content;
    u32 objectGroupInheritanceCount;
    const GraphicsObjectGroupInheritance* objectGroupInheritances;
};

struct ComputeObjectGroupInheritanceContent {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const ResourceBindingSets* bindingSets;
};

struct ComputeObjectGroupInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const ComputeObjectGroupInheritanceContent* content;
    u32 objectGroupCount;
    const ObjectGroup* objectGroups;
};

struct ComputeDataGraphInheritance {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const ComputeObjectGroupInheritanceContent* content;
    u32 objectGroupInheritanceCount;
    const ComputeObjectGroupInheritance* objectGroupInheritances;
};

union DataGraphInheritanceDescriptor {
    const GraphicsDataGraphInheritance* graphicsInheritance;
    const ComputeDataGraphInheritance* computeInheritance;
};

struct DataGraphSubmitDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DataGraphSubmitDescriptorFlags flags;
    DataGraph dataGraph;
    u64 passId;
    const DataGraphInheritanceDescriptor* inheritance;
    u32 queriesCount;
    const DataGraphQueryDescriptor* queries;
    u32 conditionalRenderingRangeCount;
    const ConditionalRenderingRange* conditionalRenderingRanges;
};

struct SchedulingGraphSubmitDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    SchedulingGraphSubmitDescriptorFlags flags;
    SchedulingGraph schedulingGraph;
    u32 dataGraphCount;
    const DataGraphSubmitDescriptor* dataGraphs;
};

struct TimelineDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    TimelineDescriptorFlags flags;
    TimelineType type;
    u64 initialValue;
};

struct TimelineWaitDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    TimelineWaitFlags flags;
    u32 timelineCount;
    const Timeline* timelines;
    const u64* values;
};

struct TimelineSignalDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    TimelineSignalFlags flags;
    u32 timelineCount;
    const Timeline* timelines;
    const u64* values;
};

struct TimelineExportFdDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    TimelineExportFlags flags;
    Timeline timeline;
};

struct TimelineImportFdDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    TimelineImportFlags flags;
    Timeline timeline;
    int fd;
};

struct TimelineImportEventIdDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Timeline timeline;
    u32 eventId;
};

struct TimelineSubmitDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Timeline timeline;
    u64 value;
    u32 passCount;
    const u64* passIds;
    const ProgramStageFlags* stageMasks;
};

struct SubmitDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    SubmitDescriptorFlags flags;
    u32 schedulingGraphCount;
    const SchedulingGraphSubmitDescriptor* schedulingGraphs;
    u32 waitTimelineCount;
    const TimelineSubmitDescriptor* waitTimelines;
    u32 signalTimelineCount;
    const TimelineSubmitDescriptor* signalTimelines;
};

struct AttachmentDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    ResourceView imageResourceView;
    ImageLayout imageLayout;
    ResolveModeFlags resolveMode;
    ResourceView resolveResourceView;
    ImageLayout resolveImageLayout;
    AttachmentLoadOp loadOp;
    AttachmentStoreOp storeOp;
    ClearValue clearValue;
};

struct FragmentShadingRateAttachmentDescriptor {
    ResourceView imageResourceView;
    Size2D shadingRateAttachmentTexelSize;
};

struct RenderTargetDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    RenderTargetDescriptorFlags flags;
    Rect2D renderArea;
    u32 layerCount;
    u32 viewMask;
    u32 colorCount;
    const AttachmentDescriptor* colors;
    const AttachmentDescriptor* depth;
    const AttachmentDescriptor* stencil;
    const FragmentShadingRateAttachmentDescriptor* fragmentShadingRate;
};

struct DisplaySurfaceFormat {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Format format;
    ColorSpace colorSpace;
    LossyTypes lossyType;
};

struct DisplaySurfaceCapabilities {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 minImageCount;
    u32 maxImageCount;
    Size2D currentExtent;
    Size2D minImageExtent;
    Size2D maxImageExtent;
    u32 maxImageArrayLayers;
    DisplaySurfaceTransformFlagBits currentTransform;
    CompositeAlphaFlags supportedCompositeAlpha;
    ImageUsageFlags supportedUsageFlags;
    b32 supportProtectedContent;
};

struct DisplaySurfaceDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DisplaySurfaceDescriptorFlags flags;
    const OHNativeWindow* nativeWindow;
};

struct SwapchainDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    SwapchainDescriptorFlags flags;
    DisplaySurface displaySurface;
    u32 minImageCount;
    const DisplaySurfaceFormat* surfaceFormat;
    Size2D imageExtent;
    u32 imageArrayLayers;
    ImageUsageFlags imageUsage;
    DisplaySurfaceTransformFlagBits preTransform;
    CompositeAlphaFlagBits compositeAlpha;
    PresentMode presentMode;
    b32 clipped;
    b32 protectedContent;
};

struct PresentDescriptor {
    Swapchain swapchain;
    u32 imageIndex;
    u32 waitCount;
    const Timeline* waitTimelines;
    const u64* waitValues;
    Timeline presentTimeline;
    u64 presentValue;
    u32 updateRegionCount;
    const Size2D* updateRegionSizes;
    const Origin2D* updateRegionOffsets;
};

struct PrivateDataSlotDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    PrivateDataSlotDescriptorFlags flags;
};

struct PrivateDataSetDescriptor {
    ObjectType objectType;
    const void* objectHandle;
    PrivateDataSlot privateDataSlot;
    u64 value;
};

struct PrivateDataGetDescriptor {
    ObjectType objectType;
    const void* objectHandle;
    PrivateDataSlot privateDataSlot;
    u64* outValue;
};

struct AabbPositions {
    f32 minX;
    f32 minY;
    f32 minZ;
    f32 maxX;
    f32 maxY;
    f32 maxZ;
};

struct TransformMatrix {
    f32 matrix[3];
};

struct EllipsoidCullingDescriptor {
    f32 minAlpha;
    f32 maxOpacity;
    f32 cutOff;
};

struct AccelerationStructureInstance {
    TransformMatrix transform;
    u32 instanceCustomIndex;
    u32 mask;
    u32 instanceShaderBindingTableRecordOffset;
    AccelerationStructureInstanceFlags flags;
    u64 accelerationStructureReference;
};

struct Ellipsoid {
    f32 center[3];
    f32 rotation[4];
    f32 scaling[3];
    f32 opacity;
    b32 shCoeffHalfPrecision;
    f32 shCoeffOffset;
};

struct AccelerationStructureGeometryTrianglesDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    Format vertexFormat;
    DeviceAddress vertexData;
    DeviceSize vertexStride;
    u32 maxVertex;
    IndexType indexType;
    DeviceAddress indexData;
    DeviceAddress transformData;
};

struct AccelerationStructureGeometryAabbsDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceAddress data;
    DeviceSize stride;
};

struct AccelerationStructureGeometryInstancesDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    b32 arrayOfPointers;
    DeviceAddress data;
};

struct AccelerationStructureGeometryEllipsoidDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceAddress ellipsoidData;
    DeviceAddress ellipsoidSHCoeffData;
    u32 ellipsoidCount;
    u32 ellipsoidStride;
    f32 shOrder;
    u32 shCoeffCount;
    u32 shCoeffStride;
};

union AccelerationStructureGeometryData {
    AccelerationStructureGeometryTrianglesDescriptor triangles;
    AccelerationStructureGeometryAabbsDescriptor aabbs;
    AccelerationStructureGeometryInstancesDescriptor instances;
    AccelerationStructureGeometryEllipsoidDescriptor ellipsoids;
};

struct AccelerationStructureGeometryDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    GeometryType geometryType;
    AccelerationStructureGeometryData geometry;
    GeometryFlags flags;
};

struct AccelerationStructureBuildGeometryDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    AccelerationStructureType type;
    BuildAccelerationStructureFlags flags;
    AccelerationStructureBuildMode mode;
    AccelerationStructure srcAccelerationStructure;
    AccelerationStructure dstAccelerationStructure;
    u32 geometryCount;
    const AccelerationStructureGeometryDescriptor* geometries;
    const AccelerationStructureGeometryDescriptor* const * geometriesPointers;
    DeviceAddress scratchData;
};

struct AccelerationStructureBuildRangeDescriptor {
    u32 primitiveCount;
    u32 primitiveOffset;
    u32 firstVertex;
    u32 transformOffset;
};

struct AccelerationStructureBuildCommand {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    u32 infoCount;
    const AccelerationStructureBuildGeometryDescriptor* buildGeometry;
    const AccelerationStructureBuildRangeDescriptor* const * buildRanges;
};

struct AccelerationStructureCopyCommand {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    AccelerationStructureCopyMode mode;
    AccelerationStructure src;
    AccelerationStructure dst;
};

struct AccelerationStructureSerializeCommand {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    AccelerationStructure src;
    DeviceAddress dst;
};

struct AccelerationStructureDeserializeCommand {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceAddress src;
    AccelerationStructure dst;
};

union AccelerationStructureCommandData {
    const AccelerationStructureBuildCommand* build;
    const AccelerationStructureCopyCommand* copy;
    const AccelerationStructureSerializeCommand* serialize;
    const AccelerationStructureDeserializeCommand* deserialize;
};

struct AccelerationStructureCommandDescriptor {
    AccelerationStructureCommandType type;
    AccelerationStructureCommandData data;
};

struct AccelerationStructureDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    AccelerationStructureDescriptorFlags flags;
    const u32 primitiveCount;
    Resource buffer;
    DeviceSize offset;
    DeviceSize size;
    AccelerationStructureType type;
    DeviceAddress deviceAddress;
};

struct AccelerationStructureObjectGroupDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    AccelerationStructureObjectGroupDescriptorFlags flags;
    u32 commandCount;
    const AccelerationStructureCommandDescriptor* commands;
};

struct AccelerationStructureBuildSizesDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    DeviceSize accelerationStructureSize;
    DeviceSize updateScratchSize;
    DeviceSize buildScratchSize;
};

struct AccelerationStructureVersion {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    const u8* versionData;
};

struct QueryPoolDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    QueryPoolDescriptorFlags flags;
    QueryType queryType;
    u32 queryCount;
};

struct GpuCounterProperties {
    QueryType queryType;
    CounterSet counterSet;
    DeviceSize querySize;
    DeviceSize offset;
    CounterStorage storage;
    CounterUnit unit;
    u32 collectionTypeCount;
    QueryCollectionType collectionTypes[MLN_MAX_COUNTER_TYPES];
    char name[MLN_MAX_COUNTER_NAME_SIZE];
    char description[MLN_MAX_COUNTER_DESCRIPTION_SIZE];
};

struct GetQueryPoolResultDescriptor {
    u32 extensionCount;
    const ExtensionBlock* extensions;
    QueryPool srcQueryPool;
    void* pData;
    u32 regionCount;
    const QueryPoolResultCopyRegion* regions;
};

// ===== Method Definitions =====
PFN_MlnVoidFunction Device::GetDeviceProcAddr(const char* name)
{
    return ::MlnGetDeviceProcAddr(m_handle, name);
}

Status Device::GetGpuFeatures(GpuFeatures* features)
{
    return ::MlnGetGpuFeatures(m_handle,
                               reinterpret_cast<MlnGpuFeatures*>(features));
}

Status Device::GetGpuProperties(GpuProperties* properties)
{
    return ::MlnGetGpuProperties(
        m_handle, reinterpret_cast<MlnGpuProperties*>(properties));
}

Status Device::GetGpuFormatProperties(Format format,
                                      GpuFormatProperties* properties)
{
    return ::MlnGetGpuFormatProperties(
        m_handle, format,
        reinterpret_cast<MlnGpuFormatProperties*>(properties));
}

Status Device::GetGpuImageFormatProperties(Format format, ImageType type,
                                           ImageTiling tiling,
                                           ImageUsageFlags usage,
                                           ImageDescriptorFlags flags,
                                           GpuImageFormatProperties* properties)
{
    return ::MlnGetGpuImageFormatProperties(
        m_handle, format, type, tiling, usage, flags,
        reinterpret_cast<MlnGpuImageFormatProperties*>(properties));
}

Status Device::DeviceWaitIdle() { return ::MlnDeviceWaitIdle(m_handle); }

Status Device::GetGpuFragmentShadingRates(
    u32* pFragmentShadingRateCount,
    GpuFragmentShadingRate* pFragmentShadingRates,
    GpuFragmentShadingRateFeatures* pFragmentShadingRateFeatures,
    GpuFragmentShadingRateProperties* pFragmentShadingRateProperties)
{
    return ::MlnGetGpuFragmentShadingRates(
        m_handle, pFragmentShadingRateCount,
        reinterpret_cast<MlnGpuFragmentShadingRate*>(pFragmentShadingRates),
        reinterpret_cast<MlnGpuFragmentShadingRateFeatures*>(
            pFragmentShadingRateFeatures),
        reinterpret_cast<MlnGpuFragmentShadingRateProperties*>(
            pFragmentShadingRateProperties));
}

Queue Device::GetDeviceQueue(u32 index)
{
    return Queue(::MlnGetDeviceQueue(m_handle, index));
}

Status Device::GetGpuMemoryProperties(GpuMemoryProperties* properties)
{
    return ::MlnGetGpuMemoryProperties(
        m_handle, reinterpret_cast<MlnGpuMemoryProperties*>(properties));
}

void Device::GetResourceMemoryRequirements(
    const ResourceMemoryRequirementsDescriptor* descriptor,
    MemoryRequirements* memoryRequirements)
{
    ::MlnGetResourceMemoryRequirements(
        m_handle,
        reinterpret_cast<const MlnResourceMemoryRequirementsDescriptor*>(
            descriptor),
        reinterpret_cast<MlnMemoryRequirements*>(memoryRequirements));
}

DeviceMemory Device::AllocateMemory(const MemoryAllocateDescriptor* descriptor)
{
    return DeviceMemory(::MlnAllocateMemory(
        m_handle,
        reinterpret_cast<const MlnMemoryAllocateDescriptor*>(descriptor)));
}

void Device::FreeMemory(DeviceMemory memory)
{
    ::MlnFreeMemory(m_handle, static_cast<MlnDeviceMemory>(memory));
}

Status Device::MapMemory(const MemoryMapDescriptor* descriptor, void** data)
{
    return ::MlnMapMemory(
        m_handle, reinterpret_cast<const MlnMemoryMapDescriptor*>(descriptor),
        data);
}

Status Device::UnmapMemory(const MemoryUnmapDescriptor* descriptor)
{
    return ::MlnUnmapMemory(
        m_handle,
        reinterpret_cast<const MlnMemoryUnmapDescriptor*>(descriptor));
}

Status Device::FlushMappedMemoryRanges(
    u32 memoryRangeCount, const MappedMemoryRangeDescriptor* memoryRanges)
{
    return ::MlnFlushMappedMemoryRanges(
        m_handle, memoryRangeCount,
        reinterpret_cast<const MlnMappedMemoryRangeDescriptor*>(memoryRanges));
}

Status Device::InvalidateMappedMemoryRanges(
    u32 memoryRangeCount, const MappedMemoryRangeDescriptor* memoryRanges)
{
    return ::MlnInvalidateMappedMemoryRanges(
        m_handle, memoryRangeCount,
        reinterpret_cast<const MlnMappedMemoryRangeDescriptor*>(memoryRanges));
}

Status Device::GetNativeBufferProperties(const OH_NativeBuffer* buffer,
                                         NativeBufferProperties* properties)
{
    return ::MlnGetNativeBufferProperties(
        m_handle, buffer,
        reinterpret_cast<MlnNativeBufferProperties*>(properties));
}

Status Device::GetMemoryNativeBuffer(
    const MemoryGetNativeBufferDescriptor* descriptor,
    OH_NativeBuffer** pBuffer)
{
    return ::MlnGetMemoryNativeBuffer(
        m_handle,
        reinterpret_cast<const MlnMemoryGetNativeBufferDescriptor*>(descriptor),
        pBuffer);
}

void Device::GetBufferResourceMemoryRequirements(
    const BufferDescriptor* descriptor, MemoryRequirements* memoryRequirements)
{
    ::MlnGetBufferResourceMemoryRequirements(
        m_handle, reinterpret_cast<const MlnBufferDescriptor*>(descriptor),
        reinterpret_cast<MlnMemoryRequirements*>(memoryRequirements));
}

void Device::GetImageResourceMemoryRequirements(
    const ImageDescriptor* descriptor, MemoryRequirements* memoryRequirements)
{
    ::MlnGetImageResourceMemoryRequirements(
        m_handle, reinterpret_cast<const MlnImageDescriptor*>(descriptor),
        reinterpret_cast<MlnMemoryRequirements*>(memoryRequirements));
}

Resource Device::CreateBufferResource(const BufferDescriptor* descriptor)
{
    return Resource(::MlnCreateBufferResource(
        m_handle, reinterpret_cast<const MlnBufferDescriptor*>(descriptor)));
}

Resource Device::CreateImageResource(const ImageDescriptor* descriptor)
{
    return Resource(::MlnCreateImageResource(
        m_handle, reinterpret_cast<const MlnImageDescriptor*>(descriptor)));
}

Status Device::BindResourceMemory(
    const BindResourceMemoryDescriptor* descriptor)
{
    return ::MlnBindResourceMemory(
        m_handle,
        reinterpret_cast<const MlnBindResourceMemoryDescriptor*>(descriptor));
}

void Device::DestroyResource(Resource resource)
{
    ::MlnDestroyResource(m_handle, static_cast<MlnResource>(resource));
}

Sampler Device::CreateSampler(const SamplerDescriptor* descriptor)
{
    return Sampler(::MlnCreateSampler(
        m_handle, reinterpret_cast<const MlnSamplerDescriptor*>(descriptor)));
}

void Device::DestroySampler(Sampler sampler)
{
    ::MlnDestroySampler(m_handle, static_cast<MlnSampler>(sampler));
}

SamplerYcbcrConversion Device::CreateSamplerYcbcrConversion(
    const SamplerYcbcrConversionDescriptor* descriptor)
{
    return SamplerYcbcrConversion(::MlnCreateSamplerYcbcrConversion(
        m_handle, reinterpret_cast<const MlnSamplerYcbcrConversionDescriptor*>(
                      descriptor)));
}

void Device::DestroySamplerYcbcrConversion(
    SamplerYcbcrConversion ycbcrConversion)
{
    ::MlnDestroySamplerYcbcrConversion(
        m_handle, static_cast<MlnSamplerYcbcrConversion>(ycbcrConversion));
}

DeviceAddress Device::GetBufferResourceAddress(Resource resource)
{
    return ::MlnGetBufferResourceAddress(m_handle,
                                         static_cast<MlnResource>(resource));
}

ResourceView Device::CreateBufferResourceView(
    const BufferViewDescriptor* descriptor)
{
    return ResourceView(::MlnCreateBufferResourceView(
        m_handle,
        reinterpret_cast<const MlnBufferViewDescriptor*>(descriptor)));
}

ResourceView Device::CreateImageResourceView(
    const ImageViewDescriptor* descriptor)
{
    return ResourceView(::MlnCreateImageResourceView(
        m_handle, reinterpret_cast<const MlnImageViewDescriptor*>(descriptor)));
}

void Device::DestroyResourceView(ResourceView resourceView)
{
    ::MlnDestroyResourceView(m_handle,
                             static_cast<MlnResourceView>(resourceView));
}

ProgramCache Device::CreateProgramCache(
    const ProgramCacheDescriptor* descriptor)
{
    return ProgramCache(::MlnCreateProgramCache(
        m_handle,
        reinterpret_cast<const MlnProgramCacheDescriptor*>(descriptor)));
}

Status Device::GetProgramCacheData(ProgramCache programCache, size_t* cacheSize,
                                   void* cache)
{
    return ::MlnGetProgramCacheData(
        m_handle, static_cast<MlnProgramCache>(programCache), cacheSize, cache);
}

Status Device::MergeProgramCaches(ProgramCache dstCache, u32 srcCacheCount,
                                  const ProgramCache* srcCaches)
{
    return ::MlnMergeProgramCaches(
        m_handle, static_cast<MlnProgramCache>(dstCache), srcCacheCount,
        reinterpret_cast<const MlnProgramCache*>(srcCaches));
}

void Device::DestroyProgramCache(ProgramCache programCache)
{
    ::MlnDestroyProgramCache(m_handle,
                             static_cast<MlnProgramCache>(programCache));
}

ProgramInterface Device::CreateProgramInterface(
    const ProgramInterfaceDescriptor* descriptor)
{
    return ProgramInterface(::MlnCreateProgramInterface(
        m_handle,
        reinterpret_cast<const MlnProgramInterfaceDescriptor*>(descriptor)));
}

void Device::DestroyProgramInterface(ProgramInterface programInterface)
{
    ::MlnDestroyProgramInterface(
        m_handle, static_cast<MlnProgramInterface>(programInterface));
}

Program Device::CreateGraphicsProgram(ProgramCache programCache,
                                      const GraphicsProgramState* state)
{
    return Program(::MlnCreateGraphicsProgram(
        m_handle, static_cast<MlnProgramCache>(programCache),
        reinterpret_cast<const MlnGraphicsProgramState*>(state)));
}

Program Device::CreateComputeProgram(ProgramCache programCache,
                                     const ComputeProgramState* state)
{
    return Program(::MlnCreateComputeProgram(
        m_handle, static_cast<MlnProgramCache>(programCache),
        reinterpret_cast<const MlnComputeProgramState*>(state)));
}

Program Device::CreateGraphicsProgramAsync(ProgramCache programCache,
                                           const GraphicsProgramState* state,
                                           Priority priority)
{
    return Program(::MlnCreateGraphicsProgramAsync(
        m_handle, static_cast<MlnProgramCache>(programCache),
        reinterpret_cast<const MlnGraphicsProgramState*>(state), priority));
}

Program Device::CreateComputeProgramAsync(ProgramCache programCache,
                                          const ComputeProgramState* state,
                                          Priority priority)
{
    return Program(::MlnCreateComputeProgramAsync(
        m_handle, static_cast<MlnProgramCache>(programCache),
        reinterpret_cast<const MlnComputeProgramState*>(state), priority));
}

Status Device::GetCreatedProgramAsyncResult(Program program)
{
    return ::MlnGetCreatedProgramAsyncResult(m_handle,
                                             static_cast<MlnProgram>(program));
}

Status Device::WaitCreatedProgramAsyncResult(Program program)
{
    return ::MlnWaitCreatedProgramAsyncResult(m_handle,
                                              static_cast<MlnProgram>(program));
}

void Device::DestroyProgram(Program program)
{
    ::MlnDestroyProgram(m_handle, static_cast<MlnProgram>(program));
}

BindingLayout Device::CreateBindingLayout(
    const BindingLayoutDescriptor* descriptor)
{
    return BindingLayout(::MlnCreateBindingLayout(
        m_handle,
        reinterpret_cast<const MlnBindingLayoutDescriptor*>(descriptor)));
}

void Device::DestroyBindingLayout(BindingLayout bindingLayout)
{
    ::MlnDestroyBindingLayout(m_handle,
                              static_cast<MlnBindingLayout>(bindingLayout));
}

BindingSet Device::CreateBindingSet(BindingLayout bindingLayout,
                                    u32 variableBindingSize)
{
    return BindingSet(::MlnCreateBindingSet(
        m_handle, static_cast<MlnBindingLayout>(bindingLayout),
        variableBindingSize));
}

void Device::UpdateBindingSets(u32 bindingWriteCount,
                               const WriteBindingSet* bindingWrites,
                               u32 bindingCopyCount,
                               const CopyBindingSet* bindingCopies)
{
    ::MlnUpdateBindingSets(
        m_handle, bindingWriteCount,
        reinterpret_cast<const MlnWriteBindingSet*>(bindingWrites),
        bindingCopyCount,
        reinterpret_cast<const MlnCopyBindingSet*>(bindingCopies));
}

void Device::DestroyBindingSet(BindingSet bindingSet)
{
    ::MlnDestroyBindingSet(m_handle, static_cast<MlnBindingSet>(bindingSet));
}

ObjectGroup Device::CreateGraphicsObjectGroup(
    const GraphicsObjectGroupDescriptor* descriptor)
{
    return ObjectGroup(::MlnCreateGraphicsObjectGroup(
        m_handle,
        reinterpret_cast<const MlnGraphicsObjectGroupDescriptor*>(descriptor)));
}

Status Device::UpdateGraphicsObjectGroup(
    ObjectGroup objectGroup,
    const GraphicsObjectGroupUpdateDescriptor* descriptor)
{
    return ::MlnUpdateGraphicsObjectGroup(
        m_handle, static_cast<MlnObjectGroup>(objectGroup),
        reinterpret_cast<const MlnGraphicsObjectGroupUpdateDescriptor*>(
            descriptor));
}

ObjectGroup Device::CreateComputeObjectGroup(
    const ComputeObjectGroupDescriptor* descriptor)
{
    return ObjectGroup(::MlnCreateComputeObjectGroup(
        m_handle,
        reinterpret_cast<const MlnComputeObjectGroupDescriptor*>(descriptor)));
}

Status Device::UpdateComputeObjectGroup(
    ObjectGroup objectGroup,
    const ComputeObjectGroupUpdateDescriptor* descriptor)
{
    return ::MlnUpdateComputeObjectGroup(
        m_handle, static_cast<MlnObjectGroup>(objectGroup),
        reinterpret_cast<const MlnComputeObjectGroupUpdateDescriptor*>(
            descriptor));
}

ObjectGroup Device::CreateTransferObjectGroup(
    const TransferObjectGroupDescriptor* descriptor)
{
    return ObjectGroup(::MlnCreateTransferObjectGroup(
        m_handle,
        reinterpret_cast<const MlnTransferObjectGroupDescriptor*>(descriptor)));
}

ObjectGroup Device::CreateClearObjectGroup(
    const ClearObjectGroupDescriptor* descriptor)
{
    return ObjectGroup(::MlnCreateClearObjectGroup(
        m_handle,
        reinterpret_cast<const MlnClearObjectGroupDescriptor*>(descriptor)));
}

ObjectGroup Device::CreateBarrierObjectGroup(
    const BarrierObjectGroupDescriptor* descriptor)
{
    return ObjectGroup(::MlnCreateBarrierObjectGroup(
        m_handle,
        reinterpret_cast<const MlnBarrierObjectGroupDescriptor*>(descriptor)));
}

void Device::DestroyObjectGroup(ObjectGroup objectGroup)
{
    ::MlnDestroyObjectGroup(m_handle, static_cast<MlnObjectGroup>(objectGroup));
}

DataGraph Device::CreateGraphicsDataGraph(
    const GraphicsDataGraphDescriptor* descriptor)
{
    return DataGraph(::MlnCreateGraphicsDataGraph(
        m_handle,
        reinterpret_cast<const MlnGraphicsDataGraphDescriptor*>(descriptor)));
}

DataGraph Device::CreateComputeDataGraph(
    const ComputeDataGraphDescriptor* descriptor)
{
    return DataGraph(::MlnCreateComputeDataGraph(
        m_handle,
        reinterpret_cast<const MlnComputeDataGraphDescriptor*>(descriptor)));
}

DataGraph Device::CreateTransferDataGraph(
    const TransferDataGraphDescriptor* descriptor)
{
    return DataGraph(::MlnCreateTransferDataGraph(
        m_handle,
        reinterpret_cast<const MlnTransferDataGraphDescriptor*>(descriptor)));
}

void Device::DestroyDataGraph(DataGraph dataGraph)
{
    ::MlnDestroyDataGraph(m_handle, static_cast<MlnDataGraph>(dataGraph));
}

SchedulingGraph Device::CreateSchedulingGraph(
    const SchedulingGraphDescriptor* descriptor)
{
    return SchedulingGraph(::MlnCreateSchedulingGraph(
        m_handle,
        reinterpret_cast<const MlnSchedulingGraphDescriptor*>(descriptor)));
}

void Device::DestroySchedulingGraph(SchedulingGraph schedulingGraph)
{
    ::MlnDestroySchedulingGraph(
        m_handle, static_cast<MlnSchedulingGraph>(schedulingGraph));
}

Timeline Device::CreateTimeline(const TimelineDescriptor* descriptor)
{
    return Timeline(::MlnCreateTimeline(
        m_handle, reinterpret_cast<const MlnTimelineDescriptor*>(descriptor)));
}

Status Device::SignalTimelines(const TimelineSignalDescriptor* descriptor)
{
    return ::MlnSignalTimelines(
        m_handle,
        reinterpret_cast<const MlnTimelineSignalDescriptor*>(descriptor));
}

Status Device::WaitForTimelines(const TimelineWaitDescriptor* descriptor,
                                u64 timeout)
{
    return ::MlnWaitForTimelines(
        m_handle,
        reinterpret_cast<const MlnTimelineWaitDescriptor*>(descriptor),
        timeout);
}

Status Device::GetTimelineValue(Timeline timeline, u64* value)
{
    return ::MlnGetTimelineValue(m_handle, static_cast<MlnTimeline>(timeline),
                                 value);
}

Status Device::ExportTimelineFd(const TimelineExportFdDescriptor* descriptor,
                                int* fd)
{
    return ::MlnExportTimelineFd(
        m_handle,
        reinterpret_cast<const MlnTimelineExportFdDescriptor*>(descriptor), fd);
}

Status Device::ImportTimelineFd(const TimelineImportFdDescriptor* descriptor)
{
    return ::MlnImportTimelineFd(
        m_handle,
        reinterpret_cast<const MlnTimelineImportFdDescriptor*>(descriptor));
}

Status Device::ImportTimelineEventId(
    const TimelineImportEventIdDescriptor* descriptor)
{
    return ::MlnImportTimelineEventId(
        m_handle, reinterpret_cast<const MlnTimelineImportEventIdDescriptor*>(
                      descriptor));
}

Status Device::ResetTimeline(Timeline timeline, u64 initialValue)
{
    return ::MlnResetTimeline(m_handle, static_cast<MlnTimeline>(timeline),
                              initialValue);
}

void Device::DestroyTimeline(Timeline timeline)
{
    ::MlnDestroyTimeline(m_handle, static_cast<MlnTimeline>(timeline));
}

RenderTarget Device::CreateRenderTarget(
    const RenderTargetDescriptor* descriptor)
{
    return RenderTarget(::MlnCreateRenderTarget(
        m_handle,
        reinterpret_cast<const MlnRenderTargetDescriptor*>(descriptor)));
}

void Device::DestroyRenderTarget(RenderTarget renderTarget)
{
    ::MlnDestroyRenderTarget(m_handle,
                             static_cast<MlnRenderTarget>(renderTarget));
}

DisplaySurface Device::CreateDisplaySurface(
    const DisplaySurfaceDescriptor* descriptor)
{
    return DisplaySurface(::MlnCreateDisplaySurface(
        m_handle,
        reinterpret_cast<const MlnDisplaySurfaceDescriptor*>(descriptor)));
}

Status Device::GetDisplaySurfaceCapabilities(
    DisplaySurface displaySurface, DisplaySurfaceCapabilities* capabilities)
{
    return ::MlnGetDisplaySurfaceCapabilities(
        m_handle, static_cast<MlnDisplaySurface>(displaySurface),
        reinterpret_cast<MlnDisplaySurfaceCapabilities*>(capabilities));
}

Status Device::GetDisplaySurfaceFormats(DisplaySurface displaySurface,
                                        u32* formatCount,
                                        DisplaySurfaceFormat* formats)
{
    return ::MlnGetDisplaySurfaceFormats(
        m_handle, static_cast<MlnDisplaySurface>(displaySurface), formatCount,
        reinterpret_cast<MlnDisplaySurfaceFormat*>(formats));
}

Status Device::GetDisplaySurfacePresentModes(DisplaySurface displaySurface,
                                             u32* presentModeCount,
                                             PresentMode* presentModes)
{
    return ::MlnGetDisplaySurfacePresentModes(
        m_handle, static_cast<MlnDisplaySurface>(displaySurface),
        presentModeCount, presentModes);
}

void Device::DestroyDisplaySurface(DisplaySurface displaySurface)
{
    ::MlnDestroyDisplaySurface(m_handle,
                               static_cast<MlnDisplaySurface>(displaySurface));
}

Swapchain Device::CreateSwapchain(const SwapchainDescriptor* descriptor)
{
    return Swapchain(::MlnCreateSwapchain(
        m_handle, reinterpret_cast<const MlnSwapchainDescriptor*>(descriptor)));
}

Status Device::AcquireNextImage(Swapchain swapchain, u64 timeout,
                                Timeline timeline, u64 timelineValue,
                                u32* imageIndex)
{
    return ::MlnAcquireNextImage(m_handle, static_cast<MlnSwapchain>(swapchain),
                                 timeout, static_cast<MlnTimeline>(timeline),
                                 timelineValue, imageIndex);
}

Status Device::GetSwapchainImages(Swapchain swapchain, u32* imageCount,
                                  Resource* imageResources)
{
    return ::MlnGetSwapchainImages(
        m_handle, static_cast<MlnSwapchain>(swapchain), imageCount,
        reinterpret_cast<MlnResource*>(imageResources));
}

void Device::DestroySwapchain(Swapchain swapchain)
{
    ::MlnDestroySwapchain(m_handle, static_cast<MlnSwapchain>(swapchain));
}

Status Device::GetSwapchainGrallocUsageOHOS(Format format,
                                            ImageUsageFlags imageUsage,
                                            u64* grallocUsage)
{
    return ::MlnGetSwapchainGrallocUsageOHOS(m_handle, format, imageUsage,
                                             grallocUsage);
}

Status Device::AcquireImageOHOS(Resource resource, s32 nativeFenceFd,
                                Timeline timeline, u64 timelineValue)
{
    return ::MlnAcquireImageOHOS(
        m_handle, static_cast<MlnResource>(resource), nativeFenceFd,
        static_cast<MlnTimeline>(timeline), timelineValue);
}

PrivateDataSlot Device::CreatePrivateDataSlot(
    const PrivateDataSlotDescriptor* descriptor)
{
    return PrivateDataSlot(::MlnCreatePrivateDataSlot(
        m_handle,
        reinterpret_cast<const MlnPrivateDataSlotDescriptor*>(descriptor)));
}

void Device::DestroyPrivateDataSlot(PrivateDataSlot privateDataSlot)
{
    ::MlnDestroyPrivateDataSlot(
        m_handle, static_cast<MlnPrivateDataSlot>(privateDataSlot));
}

Status Device::SetPrivateData(const PrivateDataSetDescriptor* descriptor)
{
    return ::MlnSetPrivateData(
        m_handle,
        reinterpret_cast<const MlnPrivateDataSetDescriptor*>(descriptor));
}

Status Device::GetPrivateData(const PrivateDataGetDescriptor* descriptor)
{
    return ::MlnGetPrivateData(
        m_handle,
        reinterpret_cast<const MlnPrivateDataGetDescriptor*>(descriptor));
}

ObjectGroup Device::CreateAccelerationStructureObjectGroup(
    const AccelerationStructureObjectGroupDescriptor* descriptor)
{
    return ObjectGroup(::MlnCreateAccelerationStructureObjectGroup(
        m_handle,
        reinterpret_cast<const MlnAccelerationStructureObjectGroupDescriptor*>(
            descriptor)));
}

DataGraph Device::CreateAccelerationStructureDataGraph(
    const AccelerationStructureDataGraphDescriptor* descriptor)
{
    return DataGraph(::MlnCreateAccelerationStructureDataGraph(
        m_handle,
        reinterpret_cast<const MlnAccelerationStructureDataGraphDescriptor*>(
            descriptor)));
}

Status Device::GetAccelerationStructureBuildSizes(
    const AccelerationStructureBuildGeometryDescriptor* buildGeometry,
    const u32* maxPrimitiveCounts,
    AccelerationStructureBuildSizesDescriptor* buildSizes)
{
    return ::MlnGetAccelerationStructureBuildSizes(
        m_handle,
        reinterpret_cast<
            const MlnAccelerationStructureBuildGeometryDescriptor*>(
            buildGeometry),
        maxPrimitiveCounts,
        reinterpret_cast<MlnAccelerationStructureBuildSizesDescriptor*>(
            buildSizes));
}

AccelerationStructure Device::CreateAccelerationStructure(
    const AccelerationStructureDescriptor* descriptor)
{
    return AccelerationStructure(::MlnCreateAccelerationStructure(
        m_handle, reinterpret_cast<const MlnAccelerationStructureDescriptor*>(
                      descriptor)));
}

DeviceAddress Device::GetAccelerationStructureDeviceAddress(
    AccelerationStructure accelerationStructure)
{
    return ::MlnGetAccelerationStructureDeviceAddress(
        m_handle, static_cast<MlnAccelerationStructure>(accelerationStructure));
}

void Device::DestroyAccelerationStructure(
    AccelerationStructure accelerationStructure)
{
    ::MlnDestroyAccelerationStructure(
        m_handle, static_cast<MlnAccelerationStructure>(accelerationStructure));
}

Status Device::GetRayTracingCapabilities(RayTracingCapabilities* capabilities)
{
    return ::MlnGetRayTracingCapabilities(
        m_handle, reinterpret_cast<MlnRayTracingCapabilities*>(capabilities));
}

Status Device::CheckAccelerationStructureCompatibility(
    const AccelerationStructureVersion* version,
    AccelerationStructureCompatibility* compatibility)
{
    return ::MlnCheckAccelerationStructureCompatibility(
        m_handle,
        reinterpret_cast<const MlnAccelerationStructureVersion*>(version),
        compatibility);
}

QueryPool Device::CreateQueryPool(const QueryPoolDescriptor* descriptor)
{
    return QueryPool(::MlnCreateQueryPool(
        m_handle, reinterpret_cast<const MlnQueryPoolDescriptor*>(descriptor)));
}

void Device::DestroyQueryPool(QueryPool queryPool)
{
    ::MlnDestroyQueryPool(m_handle, static_cast<MlnQueryPool>(queryPool));
}

Status Device::ResetQueryPool(const ResetQueryPoolDescriptor* descriptor)
{
    return ::MlnResetQueryPool(
        m_handle,
        reinterpret_cast<const MlnResetQueryPoolDescriptor*>(descriptor));
}

Status Device::GetQueryPoolResult(
    const GetQueryPoolResultDescriptor* descriptor)
{
    return ::MlnGetQueryPoolResult(
        m_handle,
        reinterpret_cast<const MlnGetQueryPoolResultDescriptor*>(descriptor));
}

Status Device::GetGpuCounterProperties(Counter counter,
                                       GpuCounterProperties* properties)
{
    return ::MlnGetGpuCounterProperties(
        m_handle, counter,
        reinterpret_cast<MlnGpuCounterProperties*>(properties));
}

Status Queue::QueueSubmit(const SubmitDescriptor* descriptor)
{
    return ::MlnQueueSubmit(
        m_handle, reinterpret_cast<const MlnSubmitDescriptor*>(descriptor));
}

Status Queue::QueueWaitIdle() { return ::MlnQueueWaitIdle(m_handle); }

Status Queue::QueueSetPriority(QueuePriority priority, f32 adjust)
{
    return ::MlnQueueSetPriority(m_handle, priority, adjust);
}

Status Queue::QueueGetPriority(QueuePriority* priority, f32* adjust)
{
    return ::MlnQueueGetPriority(m_handle, priority, adjust);
}

Status Queue::QueuePresentSwapchain(const PresentDescriptor* descriptor)
{
    return ::MlnQueuePresentSwapchain(
        m_handle, reinterpret_cast<const MlnPresentDescriptor*>(descriptor));
}

Status Queue::QueueSignalReleaseImageOHOS(u32 waitCount,
                                          const Timeline* timeline,
                                          const u64* waitValues,
                                          Resource resource, s32* nativeFenceFd)
{
    return ::MlnQueueSignalReleaseImageOHOS(
        m_handle, waitCount, reinterpret_cast<const MlnTimeline*>(timeline),
        waitValues, static_cast<MlnResource>(resource), nativeFenceFd);
}

// ===== Global Functions =====
Device CreateDevice(const DeviceDescriptor* descriptor)
{
    return Device(::MlnCreateDevice(
        reinterpret_cast<const MlnDeviceDescriptor*>(descriptor)));
}

void DestroyDevice(Device device)
{
    ::MlnDestroyDevice(static_cast<MlnDevice>(device));
}

}  // namespace mln
