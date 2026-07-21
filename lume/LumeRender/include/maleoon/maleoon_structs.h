// $Copyright
//
// This header is generated from the Maleoon JSON Registry.
//

#pragma once

#include <maleoon/maleoon_defines.h>
#include <maleoon/maleoon_enums.h>
#include <maleoon/maleoon_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Base type structure for 2D origin type.
 */
typedef struct MlnOrigin2D {
    s32 x;
    s32 y;
} MlnOrigin2D;

/**
 * @brief Base type structure for 3D origin type.
 */
typedef struct MlnOrigin3D {
    s32 x;
    s32 y;
    s32 z;
} MlnOrigin3D;

/**
 * @brief Base type structure for 2D size type.
 */
typedef struct MlnSize2D {
    u32 width;
    u32 height;
} MlnSize2D;

/**
 * @brief Base type structure for 3D size type.
 */
typedef struct MlnSize3D {
    u32 width;
    u32 height;
    u32 depth;
} MlnSize3D;

/**
 * @brief Base type structure for 2D rect type.
 */
typedef struct MlnRect2D {
    MlnOrigin2D origin;
    MlnSize2D size;
} MlnRect2D;

/**
 * @brief Base type structure for extension block.
 */
typedef struct MlnExtensionBlock {
    MlnExtensionKey key;
    const void* data;
} MlnExtensionBlock;

typedef struct MlnLayerProperties {
    char layerName[MLN_MAX_LAYER_NAME_SIZE];
    u32 specVersion;
    u32 implementationVersion;
    char description[MLN_MAX_DESCRIPTION_SIZE];
} MlnLayerProperties;

/**
 * @brief The descriptor for application.
 */
typedef struct MlnApplicationDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const char* applicationName;
    u32 applicationVersion;
    const char* engineName;
    u32 engineVersion;
} MlnApplicationDescriptor;

/**
 * @brief The descriptor for queue.
 */
typedef struct MlnQueueDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnQueueDescriptorFlags flags;
    MlnQueuePriority priority;
} MlnQueueDescriptor;

/**
 * @brief The descriptor for device.
 */
typedef struct MlnDeviceDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceDescriptorFlags flags;
    const MlnApplicationDescriptor* appDescriptor;
    u32 queueDescriptorCount;
    const MlnQueueDescriptor* queueDescriptors;
    u32 layerCount;
    const char* const * layerNames;
} MlnDeviceDescriptor;

/**
 * @brief The structure for Gpu Limits.
 */
typedef struct MlnGpuLimits {
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
    MlnDeviceSize bufferImageGranularity;
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
    MlnDeviceSize minTexelBufferOffsetAlignment;
    MlnDeviceSize minUniformBufferOffsetAlignment;
    MlnDeviceSize minStorageBufferOffsetAlignment;
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
    MlnDeviceSize optimalBufferCopyOffsetAlignment;
    MlnDeviceSize optimalBufferCopyRowPitchAlignment;
    MlnDeviceSize nonCoherentAtomSize;
    u32 queueCount;
} MlnGpuLimits;

/**
 * @brief The structure for Gpu features.
 */
typedef struct MlnGpuFeatures {
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
} MlnGpuFeatures;

/**
 * @brief The structure for ray tracing capabilities.
 */
typedef struct MlnRayTracingCapabilities {
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
} MlnRayTracingCapabilities;

/**
 * @brief The structure for Gpu properties.
 */
typedef struct MlnGpuProperties {
    u32 driverVersion;
    u8 programCacheUUID[MLN_UUID_SIZE];
    MlnGpuLimits limits;
} MlnGpuProperties;

/**
 * @brief The structure for format-level Gpu capabilities.
 */
typedef struct MlnGpuFormatProperties {
    MlnFormatFeatureFlags linearTilingFeatures;
    MlnFormatFeatureFlags optimalTilingFeatures;
    MlnFormatFeatureFlags bufferFeatures;
} MlnGpuFormatProperties;

/**
 * @brief The structure for Gpu image creation limits for a specific format and
 * parameters.
 */
typedef struct MlnGpuImageFormatProperties {
    u32 maxWidth;
    u32 maxHeight;
    u32 maxDepth;
    u32 maxArrayLayers;
    MlnSampleCountFlags sampleCounts;
    MlnDeviceSize maxResourceSize;
    MlnDeviceSize optimalTilingAlignment;
    MlnDeviceSize linearTilingAlignment;
} MlnGpuImageFormatProperties;

/**
 * @brief The structure for Gpu shading rate.
 */
typedef struct MlnGpuFragmentShadingRate {
    MlnSampleCountFlags sampleCounts;
    MlnSize2D fragmentSize;
} MlnGpuFragmentShadingRate;

/**
 * @brief The structure for Gpu features of fragment shading rate.
 */
typedef struct MlnGpuFragmentShadingRateFeatures {
    b32 primitiveFragmentShadingRate;
} MlnGpuFragmentShadingRateFeatures;

/**
 * @brief The structure for Gpu properties of fragment shading rate.
 */
typedef struct MlnGpuFragmentShadingRateProperties {
    MlnSize2D minFragmentShadingRateAttachmentTexelSize;
    MlnSize2D maxFragmentShadingRateAttachmentTexelSize;
    u32 maxFragmentShadingRateAttachmentTexelSizeAspectRatio;
    b32 primitiveFragmentShadingRateWithMultipleViewports;
    MlnSize2D maxFragmentSize;
    u32 maxFragmentSizeAspectRatio;
    u32 maxFragmentShadingRateCoverageSamples;
    MlnSampleCountFlagBits maxFragmentShadingRateRasterizationSamples;
    b32 fragmentShadingRateWithConservativeRasterization;
    b32 fragmentShadingRateWithFragmentShaderInterlock;
} MlnGpuFragmentShadingRateProperties;

/**
 * @brief The structure for memory type.
 */
typedef struct MlnMemoryType {
    MlnMemoryPropertyFlags propertyFlags;
    u32 heapIndex;
} MlnMemoryType;

/**
 * @brief The structure for memory heap.
 */
typedef struct MlnMemoryHeap {
    MlnDeviceSize size;
    MlnMemoryHeapFlags flags;
} MlnMemoryHeap;

/**
 * @brief The structure for memory properties.
 */
typedef struct MlnGpuMemoryProperties {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 memoryTypeCount;
    MlnMemoryType memoryTypes[MLN_MAX_GPU_MEMORY_TYPES];
    u32 memoryHeapCount;
    MlnMemoryHeap memoryHeaps[MLN_MAX_GPU_MEMORY_HEAPS];
} MlnGpuMemoryProperties;

/**
 * @brief The descriptor for external memory.
 */
typedef struct MlnExternalMemoryDescriptor {
    MlnExternalMemoryHandleType type;
    OH_NativeBuffer* nativeBuffer;
} MlnExternalMemoryDescriptor;

/**
 * @brief The descriptor for allocating memory.
 */
typedef struct MlnMemoryAllocateDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceSize allocationSize;
    u32 memoryTypeIndex;
    MlnExternalMemoryDescriptor* externalMemoryDescriptor;
    MlnResource dedicatedResource;
    MlnMemoryPolicy policy;
} MlnMemoryAllocateDescriptor;

/**
 * @brief The structure for querying the memory requirement of the resource.
 */
typedef struct MlnResourceMemoryRequirementsDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource resource;
    MlnImageAspectFlags aspectMask;
} MlnResourceMemoryRequirementsDescriptor;

/**
 * @brief The structure for bind the memory of the resource.
 */
typedef struct MlnBindResourceMemoryDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource resource;
    MlnImageAspectFlags aspectMask;
    MlnDeviceMemory memory;
    MlnDeviceSize offset;
} MlnBindResourceMemoryDescriptor;

/**
 * @brief The structure for specifying dedicated memory requirements.
 */
typedef struct MlnMemoryDedicatedRequirements {
    b32 prefersDedicatedAllocation;
    b32 requiresDedicatedAllocation;
} MlnMemoryDedicatedRequirements;

/**
 * @brief The structure for querying the memory requirement of the resource.
 */
typedef struct MlnMemoryRequirements {
    MlnDeviceSize size;
    MlnDeviceSize alignment;
    u32 memoryTypeBits;
    MlnMemoryDedicatedRequirements memoryDedicatedRequirements;
} MlnMemoryRequirements;

typedef struct MlnMappedMemoryRangeDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceMemory memory;
    MlnDeviceSize offset;
    MlnDeviceSize size;
} MlnMappedMemoryRangeDescriptor;

/**
 * @brief The descriptor for map memory.
 */
typedef struct MlnMemoryMapDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnMemoryMapDescriptorFlags flags;
    MlnDeviceMemory memory;
    MlnDeviceSize offset;
    MlnDeviceSize size;
} MlnMemoryMapDescriptor;

/**
 * @brief The descriptor for unmap memory.
 */
typedef struct MlnMemoryUnmapDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnMemoryUnmapDescriptorFlags flags;
    MlnDeviceMemory memory;
} MlnMemoryUnmapDescriptor;

/**
 * @brief The descriptor for validation details, such as where to put the log
 * and what kind of parameters are to be verified.
 */
typedef struct MlnValidationLayer {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnValidationLayerFlags flags;
    MlnValidationLayerLogType logType;
    char logFile[MLN_MAX_VALIDATION_LOG_FILE_PATH_SIZE];
    MlnValidationLayerReportLevel reportLevel;
    MlnValidationLayerCheckActionType actionType;
    MlnValidationLayerCheckOptionFlags options;
} MlnValidationLayer;

/**
 * @brief The descriptor for creating buffer resource.
 */
typedef struct MlnBufferDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnBufferDescriptorFlags flags;
    MlnDeviceSize size;
    MlnBufferUsageFlags usage;
    MlnExternalMemoryHandleType externalMemType;
    const void* externalMemory;
} MlnBufferDescriptor;

typedef struct MlnImageFormatList {
    u32 formatCount;
    const MlnFormat* formats;
} MlnImageFormatList;

/**
 * @brief The descriptor for creating image resource.
 */
typedef struct MlnImageDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnImageDescriptorFlags flags;
    MlnImageType imageType;
    MlnFormat format;
    MlnSize3D size;
    u32 mipLevels;
    u32 arrayLayers;
    MlnSampleCountFlags samples;
    MlnImageTiling tiling;
    MlnImageUsageFlags usage;
    MlnImageLayout initialLayout;
    const MlnImageFormatList* formatList;
    MlnImageUsageFlags stencilUsage;
    MlnExternalMemoryHandleType externalMemType;
    const void* externalMemory;
} MlnImageDescriptor;

/**
 * @brief The descriptor for creating sampler.
 */
typedef struct MlnSamplerDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnSamplerDescriptorFlags flags;
    MlnFilter magFilter;
    MlnFilter minFilter;
    MlnSamplerMipmapMode mipmapMode;
    MlnSamplerAddressMode addressModeU;
    MlnSamplerAddressMode addressModeV;
    MlnSamplerAddressMode addressModeW;
    f32 mipLodBias;
    b32 anisotropyEnable;
    f32 maxAnisotropy;
    b32 compareEnable;
    MlnCompareOp compareOp;
    f32 minLod;
    f32 maxLod;
    MlnBorderColor borderColor;
    b32 unnormalizedCoordinates;
    MlnSamplerReductionMode reductionMode;
    MlnSamplerYcbcrConversion conversion;
} MlnSamplerDescriptor;

/**
 * @brief The structure for component mapping.
 */
typedef struct MlnComponentMapping {
    MlnComponentSwizzle r;
    MlnComponentSwizzle g;
    MlnComponentSwizzle b;
    MlnComponentSwizzle a;
} MlnComponentMapping;

/**
 * @brief The descriptor for creating sampler ycbcr conversion.
 */
typedef struct MlnSamplerYcbcrConversionDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnSamplerYcbcrConversionDescriptorFlags flags;
    MlnFormat format;
    MlnSamplerYcbcrModelConversion ycbcrModel;
    MlnSamplerYcbcrRange ycbcrRange;
    MlnComponentMapping components;
    MlnChromaLocation xChromaOffset;
    MlnChromaLocation yChromaOffset;
    MlnFilter chromaFilter;
    b32 forceExplicitReconstruction;
} MlnSamplerYcbcrConversionDescriptor;

/**
 * @brief The properties for native buffer format.
 */
typedef struct MlnNativeBufferFormatProperties {
    MlnFormat format;
    u32 externalFormat;
    MlnFormatFeatureFlags formatFeatures;
    MlnComponentMapping components;
    MlnSamplerYcbcrModelConversion ycbcrModel;
    MlnSamplerYcbcrRange ycbcrRange;
    MlnChromaLocation xChromaOffset;
    MlnChromaLocation yChromaOffset;
} MlnNativeBufferFormatProperties;

/**
 * @brief The properties for native buffer.
 */
typedef struct MlnNativeBufferProperties {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceSize allocationSize;
    u32 memoryTypeBits;
    MlnNativeBufferFormatProperties* formatProperties;
} MlnNativeBufferProperties;

/**
 * @brief The descriptor for native buffer.
 */
typedef struct MlnMemoryGetNativeBufferDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceMemory memory;
} MlnMemoryGetNativeBufferDescriptor;

/**
 * @brief The descriptor for creating buffer view.
 */
typedef struct MlnBufferViewDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnBufferViewDescriptorFlags flags;
    MlnResource bufferResource;
    MlnFormat format;
    MlnDeviceSize offset;
    MlnDeviceSize range;
} MlnBufferViewDescriptor;

/**
 * @brief The structure for image sub-resource range.
 */
typedef struct MlnImageSubresourceRange {
    MlnImageAspectFlags aspectMask;
    u32 baseMipLevel;
    u32 levelCount;
    u32 baseArrayLayer;
    u32 layerCount;
} MlnImageSubresourceRange;

/**
 * @brief The descriptor for creating image view.
 */
typedef struct MlnImageViewDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnImageViewDescriptorFlags flags;
    MlnResource imageResource;
    MlnImageViewType viewType;
    MlnFormat format;
    MlnComponentMapping components;
    MlnImageSubresourceRange subresourceRange;
    MlnFormat decodeMode;
    f32 minLod;
    MlnSamplerYcbcrConversion conversion;
} MlnImageViewDescriptor;

/**
 * @brief The descriptor for creating shader.
 */
typedef struct MlnShaderDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnShaderDescriptorFlags flags;
    size_t sourceSize;
    const u32* source;
} MlnShaderDescriptor;

/**
 * @brief The descriptor for creating program cache.
 */
typedef struct MlnProgramCacheDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnProgramCacheDescriptorFlags flags;
    size_t rawDataSize;
    const void* rawData;
} MlnProgramCacheDescriptor;

/**
 * @brief The descriptor for creating program interface.
 */
typedef struct MlnProgramInterfaceDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnProgramInterfaceDescriptorFlags flags;
    u32 bindingLayoutCount;
    const MlnBindingLayout* bindingLayouts;
} MlnProgramInterfaceDescriptor;

/**
 * @brief The descriptor for each compile time constant.
 */
typedef struct MlnCompileTimeConstantMapEntry {
    u32 constantID;
    u32 offset;
    size_t size;
} MlnCompileTimeConstantMapEntry;

/**
 * @brief The descriptor for creating compile time constant for shader.
 */
typedef struct MlnCompileTimeConstantState {
    u32 mapEntryCount;
    const MlnCompileTimeConstantMapEntry* mapEntries;
    size_t dataSize;
    const void* data;
} MlnCompileTimeConstantState;

/**
 * @brief The descriptor for shaderStage state.
 */
typedef struct MlnShaderStageState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnShaderStageStateFlags flags;
    MlnShaderStageFlags stage;
    const MlnShaderDescriptor* shaderDescriptor;
    const char* name;
    const MlnCompileTimeConstantState* compileTimeConstant;
} MlnShaderStageState;

/**
 * @brief The descriptor for vertex input binding state.
 */
typedef struct MlnVertexInputBindingState {
    u32 binding;
    u32 stride;
    MlnVertexInputRate inputRate;
    u32 divisor;
} MlnVertexInputBindingState;

/**
 * @brief The descriptor for vertex input attribute state.
 */
typedef struct MlnVertexInputAttributeState {
    u32 location;
    u32 binding;
    MlnFormat format;
    u32 offset;
} MlnVertexInputAttributeState;

/**
 * @brief The descriptor for vertex input state.
 */
typedef struct MlnVertexInputState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnVertexInputStateFlags flags;
    u32 vertexBindingStateCount;
    const MlnVertexInputBindingState* vertexBindingStates;
    u32 vertexAttributeStateCount;
    const MlnVertexInputAttributeState* vertexAttributeStates;
} MlnVertexInputState;

/**
 * @brief The descriptor for input assemble state.
 */
typedef struct MlnInputAssemblyState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnInputAssemblyStateFlags flags;
    MlnPrimitiveTopology topology;
    b32 primitiveRestartEnable;
} MlnInputAssemblyState;

/**
 * @brief The structure for viewport.
 */
typedef struct MlnViewport {
    f32 x;
    f32 y;
    f32 width;
    f32 height;
    f32 minDepth;
    f32 maxDepth;
} MlnViewport;

/**
 * @brief The descriptor for viewport state.
 */
typedef struct MlnViewportState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnViewportStateFlags flags;
    u32 viewportCount;
    const MlnViewport* viewports;
    u32 scissorCount;
    const MlnRect2D* scissors;
} MlnViewportState;

/**
 * @brief The structure for depth bias representation.
 */
typedef struct MlnDepthBiasRepresentation {
    MlnDepthBiasRepresentationMode mode;
    b32 depthBiasExact;
} MlnDepthBiasRepresentation;

/**
 * @brief The descriptor for rasterization state.
 */
typedef struct MlnRasterizationState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnRasterizationStateFlags flags;
    b32 depthClampEnable;
    b32 rasterizerDiscardEnable;
    MlnPolygonMode polygonMode;
    MlnCullModeFlags cullMode;
    MlnFrontFace frontFace;
    b32 depthBiasEnable;
    f32 depthBiasConstantFactor;
    f32 depthBiasClamp;
    f32 depthBiasSlopeFactor;
    f32 lineWidth;
    b32 depthClipEnable;
    MlnProvokingVertexMode provokingVertexMode;
    MlnLineRasterizationMode lineRasterizationMode;
    const MlnDepthBiasRepresentation* depthBiasRepresentation;
} MlnRasterizationState;

/**
 * @brief The structure for sample location.
 */
typedef struct MlnSampleLocation {
    f32 x;
    f32 y;
} MlnSampleLocation;

/**
 * @brief The descriptor for sample location.
 */
typedef struct MlnSampleLocationDynamicState {
    MlnSampleCountFlagBits sampleLocationsPerPixel;
    MlnSize2D sampleLocationGridSize;
    u32 sampleLocationCount;
    const MlnSampleLocation* sampleLocations;
} MlnSampleLocationDynamicState;

/**
 * @brief The descriptor for multisample state.
 */
typedef struct MlnMultisampleState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnMultisampleStateFlags flags;
    MlnSampleCountFlagBits rasterizationSamples;
    b32 sampleShadingEnable;
    f32 minSampleShading;
    const MlnSampleMask* sampleMask;
    b32 alphaToCoverageEnable;
    b32 alphaToOneEnable;
    const MlnSampleLocationDynamicState* sampleLocation;
} MlnMultisampleState;

/**
 * @brief The descriptor for stencil operation state.
 */
typedef struct MlnStencilOpState {
    MlnStencilOp failOp;
    MlnStencilOp passOp;
    MlnStencilOp depthFailOp;
    MlnCompareOp compareOp;
    u32 compareMask;
    u32 writeMask;
    u32 reference;
} MlnStencilOpState;

/**
 * @brief The descriptor for depth-stencil state.
 */
typedef struct MlnDepthStencilState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDepthStencilStateFlags flags;
    b32 depthTestEnable;
    b32 depthWriteEnable;
    MlnCompareOp depthCompareOp;
    b32 depthBoundsTestEnable;
    b32 stencilTestEnable;
    MlnStencilOpState front;
    MlnStencilOpState back;
    f32 minDepthBounds;
    f32 maxDepthBounds;
} MlnDepthStencilState;

/**
 * @brief The descriptor for color blend attachment state.
 */
typedef struct MlnColorBlendAttachmentState {
    b32 blendEnable;
    MlnBlendFactor srcColorBlendFactor;
    MlnBlendFactor dstColorBlendFactor;
    MlnBlendOp colorBlendOp;
    MlnBlendFactor srcAlphaBlendFactor;
    MlnBlendFactor dstAlphaBlendFactor;
    MlnBlendOp alphaBlendOp;
    MlnColorComponentFlags colorWriteMask;
} MlnColorBlendAttachmentState;

/**
 * @brief The descriptor for color blend state.
 */
typedef struct MlnColorBlendState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnColorBlendStateFlags flags;
    b32 logicOpEnable;
    MlnLogicOp logicOp;
    u32 attachmentCount;
    const MlnColorBlendAttachmentState* attachments;
    f32 blendConstants[4];
} MlnColorBlendState;

/**
 * @brief The descriptor for rendering state.
 */
typedef struct MlnRenderingState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnRenderingStateFlags flags;
    u32 viewMask;
    u32 colorAttachmentCount;
    const MlnFormat* colorAttachmentFormats;
    const u32* colorAttachmentLocations;
    const u32* colorAttachmentInputIndices;
    MlnFormat depthAttachmentFormat;
    MlnFormat stencilAttachmentFormat;
    u32 depthInputAttachmentIndex;
    u32 stencilInputAttachmentIndex;
} MlnRenderingState;

/**
 * @brief The descriptor for dynamic state.
 */
typedef struct MlnDynamicState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDynamicStateFlags flags;
    u32 dynamicStateCount;
    const MlnDynamicStateType* dynamicStateTypes;
} MlnDynamicState;

/**
 * @brief The descriptor for fragment shading rate state.
 */
typedef struct MlnFragmentShadingRateState {
    MlnSize2D fragmentSize;
    MlnFragmentShadingRateCombinerOp combinerOps[2];
} MlnFragmentShadingRateState;

/**
 * @brief The struct for feedback.
 */
typedef struct MlnCreationFeedback {
    MlnCreationFeedbackFlags flags;
    u64 duration;
} MlnCreationFeedback;

/**
 * @brief The descriptor for feedback state.
 */
typedef struct MlnCreationFeedbackState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnCreationFeedback* feedback;
    u32 stageFeedbackCount;
    MlnCreationFeedback* stageFeedbacks;
} MlnCreationFeedbackState;

/**
 * @brief The state for creating graphics program.
 */
typedef struct MlnGraphicsProgramState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnProgramStateFlags flags;
    u32 stageCount;
    const MlnShaderStageState* stages;
    const MlnVertexInputState* vertexInputState;
    const MlnInputAssemblyState* inputAssemblyState;
    const MlnViewportState* viewportState;
    const MlnRasterizationState* rasterizationState;
    const MlnMultisampleState* multisampleState;
    const MlnDepthStencilState* depthStencilState;
    const MlnColorBlendState* colorBlendState;
    const MlnRenderingState* renderState;
    const MlnDynamicState* dynamicState;
    const MlnFragmentShadingRateState* fragmentShadingRateState;
    MlnProgramInterface programInterface;
    const MlnCreationFeedbackState* feedbackState;
} MlnGraphicsProgramState;

/**
 * @brief The state for creating compute program.
 */
typedef struct MlnComputeProgramState {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnProgramStateFlags flags;
    MlnShaderStageState stage;
    MlnProgramInterface programInterface;
    const MlnCreationFeedbackState* feedbackState;
} MlnComputeProgramState;

/**
 * @brief Binding Slot for binding layout
 */
typedef struct MlnBindingSlot {
    u32 slot;
    MlnBindingType type;
    u32 slotCount;
    MlnShaderStageFlags stageFlags;
    const MlnSampler* immutableSamplers;
} MlnBindingSlot;

/**
 * @brief The descriptor for creating binding layout.
 */
typedef struct MlnBindingLayoutDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnBindingLayoutDescriptorFlags flags;
    u32 bindingCount;
    const MlnBindingSlot* bindings;
    const MlnBindingSlotFlags* bindingFlags;
} MlnBindingLayoutDescriptor;

/**
 * @brief The descriptor for buffer binding.
 */
typedef struct MlnBindingBufferDescriptor {
    MlnResource bufferResource;
    MlnDeviceSize offset;
    MlnDeviceSize range;
} MlnBindingBufferDescriptor;

/**
 * @brief The descriptor for image binding.
 */
typedef struct MlnBindingImageDescriptor {
    MlnSampler sampler;
    MlnResourceView imageResourceView;
    MlnImageLayout imageLayout;
} MlnBindingImageDescriptor;

/**
 * @brief The descriptor for inline uniform binding.
 */
typedef struct MlnBindingInlineUniformDescriptor {
    u32 dataSize;
    const void* data;
} MlnBindingInlineUniformDescriptor;

/**
 * @brief The descriptor for writing to binding set.
 */
typedef struct MlnWriteBindingSet {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnBindingSet dstSet;
    u32 dstBinding;
    u32 dstArrayElement;
    u32 bindingCount;
    MlnBindingType bindingType;
    const MlnBindingImageDescriptor* imageDescriptor;
    const MlnBindingBufferDescriptor* bufferDescriptor;
    const MlnResourceView* texelBufferResourceView;
    const MlnBindingInlineUniformDescriptor* inlineUniformDescriptor;
    const MlnAccelerationStructure* accelerationStructure;
} MlnWriteBindingSet;

/**
 * @brief The structure for copying between binding set.
 */
typedef struct MlnCopyBindingSet {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnBindingSet srcSet;
    u32 srcBinding;
    u32 srcArrayElement;
    MlnBindingSet dstSet;
    u32 dstBinding;
    u32 dstArrayElement;
    u32 bindingCount;
} MlnCopyBindingSet;

/**
 * @brief The descriptor for resource binding sets.
 */
typedef struct MlnResourceBindingSets {
    u32 firstSet;
    u32 bindingSetCount;
    const MlnBindingSet* bindingSets;
    u32 dynamicOffsetCount;
    const u32* dynamicOffsets;
} MlnResourceBindingSets;

/**
 * @brief The structure for program constant.
 */
typedef struct MlnProgramConstants {
    u32 offset;
    u32 size;
    const void* values;
} MlnProgramConstants;

/**
 * @brief The structure for resource program constant.
 */
typedef struct MlnResourceProgramConstants {
    u32 programConstantCount;
    const MlnProgramConstants* programConstants;
} MlnResourceProgramConstants;

/**
 * @brief The structure for resource vertex buffer.
 */
typedef struct MlnResourceVertexBuffers {
    u32 firstBinding;
    u32 bindingCount;
    const MlnResource* bufferResources;
    const MlnDeviceSize* offsets;
    const MlnDeviceSize* sizes;
    const MlnDeviceSize* strides;
} MlnResourceVertexBuffers;

/**
 * @brief The structure for resource index buffer.
 */
typedef struct MlnResourceIndexBuffer {
    MlnResource bufferResource;
    MlnDeviceSize offset;
    MlnDeviceSize size;
    MlnIndexType indexType;
} MlnResourceIndexBuffer;

/**
 * @brief The descriptor for resource set.
 */
typedef struct MlnResourceSetDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResourceSetDescriptorFlags flags;
    u32 dynamicResourceCount;
    const MlnDynamicResourceType* dynamicResource;
    const MlnResourceBindingSets* resourceBindingSets;
    const MlnResourceProgramConstants* resourceProgramConstants;
    const MlnResourceVertexBuffers* resourceVertexBuffers;
    const MlnResourceIndexBuffer* resourceIndexBuffer;
} MlnResourceSetDescriptor;

/**
 * @brief The structure for dynamic view port state.
 */
typedef struct MlnViewportDynamicState {
    u32 firstViewport;
    u32 viewportCount;
    const MlnViewport* viewports;
} MlnViewportDynamicState;

/**
 * @brief The structure for dynamic scissor state.
 */
typedef struct MlnScissorDynamicState {
    u32 firstScissor;
    u32 scissorCount;
    const MlnRect2D* scissors;
} MlnScissorDynamicState;

/**
 * @brief The structure for dynamic depth bounds state.
 */
typedef struct MlnDepthBoundsDynamicState {
    f32 minDepthBounds;
    f32 maxDepthBounds;
} MlnDepthBoundsDynamicState;

/**
 * @brief The structure for dynamic depth bias state.
 */
typedef struct MlnDepthBiasDynamicState {
    f32 depthBiasConstantFactor;
    f32 depthBiasClamp;
    f32 depthBiasSlopeFactor;
} MlnDepthBiasDynamicState;

/**
 * @brief The structure for dynamic stencil operation state.
 */
typedef struct MlnStencilOpDynamicState {
    MlnStencilFaceFlags faceMask;
    MlnStencilOp failOp;
    MlnStencilOp passOp;
    MlnStencilOp depthFailOp;
    MlnCompareOp compareOp;
} MlnStencilOpDynamicState;

/**
 * @brief The structure for dynamic stencil compare mask state.
 */
typedef struct MlnStencilCompareMaskDynamicState {
    MlnStencilFaceFlags faceMask;
    u32 compareMask;
} MlnStencilCompareMaskDynamicState;

/**
 * @brief The structure for dynamic stencil write mask state.
 */
typedef struct MlnStencilWriteMaskDynamicState {
    MlnStencilFaceFlags faceMask;
    u32 writeMask;
} MlnStencilWriteMaskDynamicState;

/**
 * @brief The structure for dynamic stencil reference state.
 */
typedef struct MlnStencilReferenceDynamicState {
    MlnStencilFaceFlags faceMask;
    u32 reference;
} MlnStencilReferenceDynamicState;

/**
 * @brief The structure for dynamic fragment shading rate state.
 */
typedef struct MlnFragmentShadingRateDynamicState {
    MlnSize2D fragmentSize;
    MlnFragmentShadingRateCombinerOp combinerOps[2];
} MlnFragmentShadingRateDynamicState;

/**
 * @brief The descriptor for state set.
 */
typedef struct MlnStateSetDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnStateSetDescriptorFlags flags;
    const MlnDynamicState* dynamicState;
    const MlnViewportDynamicState* viewport;
    const MlnScissorDynamicState* scissor;
    f32 lineWidth;
    const MlnDepthBiasDynamicState* depthBias;
    f32 blendConstants[4];
    const MlnDepthBoundsDynamicState* depthBounds;
    const MlnStencilOpDynamicState* stencilOp;
    const MlnStencilCompareMaskDynamicState* compareMask;
    const MlnStencilWriteMaskDynamicState* writeMask;
    const MlnStencilReferenceDynamicState* reference;
    const MlnFragmentShadingRateDynamicState* fragmentShadingRate;
    MlnPrimitiveTopology topology;
    b32 primitiveRestartEnable;
    b32 rasterizerDiscardEnable;
    MlnCullModeFlags cullMode;
    MlnFrontFace frontFace;
    b32 depthBiasEnable;
    const MlnSampleLocationDynamicState* sampleLocation;
    b32 depthTestEnable;
    b32 depthWriteEnable;
    MlnCompareOp depthCompareOp;
    b32 depthBoundsTestEnable;
    b32 stencilTestEnable;
    u32 viewportCount;
    u32 scissorCount;
    const MlnDeviceSize* bindingStride;
} MlnStateSetDescriptor;

/**
 * @brief The structure for draw command.
 */
typedef struct MlnDrawCommand {
    u32 vertexCount;
    u32 instanceCount;
    u32 firstVertex;
    u32 firstInstance;
} MlnDrawCommand;

/**
 * @brief The structure for indexed draw command.
 */
typedef struct MlnDrawIndexedCommand {
    u32 indexCount;
    u32 instanceCount;
    u32 firstIndex;
    u32 vertexOffset;
    u32 firstInstance;
} MlnDrawIndexedCommand;

/**
 * @brief The structure for indirect draw command.
 */
typedef struct MlnDrawIndirectCommand {
    MlnResource bufferResource;
    MlnDeviceSize offset;
    u32 drawCount;
    u32 stride;
} MlnDrawIndirectCommand;

/**
 * @brief The structure for indexed indirect draw command.
 */
typedef struct MlnDrawIndexedIndirectCommand {
    MlnResource bufferResource;
    MlnDeviceSize offset;
    u32 drawCount;
    u32 stride;
} MlnDrawIndexedIndirectCommand;

/**
 * @brief The structure for indirect count draw command.
 */
typedef struct MlnDrawIndirectCountCommand {
    MlnResource bufferResource;
    MlnDeviceSize offset;
    MlnResource countBufferResource;
    MlnDeviceSize countBufferOffset;
    u32 maxDrawCount;
    u32 stride;
} MlnDrawIndirectCountCommand;

/**
 * @brief The structure for indexed indirect count draw command.
 */
typedef struct MlnDrawIndexedIndirectCountCommand {
    MlnResource bufferResource;
    MlnDeviceSize offset;
    MlnResource countBufferResource;
    MlnDeviceSize countBufferOffset;
    u32 maxDrawCount;
    u32 stride;
} MlnDrawIndexedIndirectCountCommand;

/**
 * @brief The union for draw command.
 */
typedef union MlnGraphicsCommandData {
    const MlnDrawCommand* draw;
    const MlnDrawIndexedCommand* drawIndexed;
    const MlnDrawIndirectCommand* drawIndirect;
    const MlnDrawIndexedIndirectCommand* drawIndexedIndirect;
    const MlnDrawIndirectCountCommand* drawIndirectCount;
    const MlnDrawIndexedIndirectCountCommand* drawIndexedIndirectCount;
} MlnGraphicsCommandData;

/**
 * @brief The structure for draw command.
 */
typedef struct MlnGraphicsCommand {
    MlnGraphicsCommandType type;
    MlnGraphicsCommandData data;
} MlnGraphicsCommand;

/**
 * @brief The descriptor for draw command.
 */
typedef struct MlnGraphicsCommandDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnGraphicsCommandDescriptorFlags flags;
    const MlnGraphicsCommand* command;
    const MlnResourceSetDescriptor* resourceSet;
    const MlnStateSetDescriptor* stateSet;
} MlnGraphicsCommandDescriptor;

/**
 * @brief The structure for binding sets inheritance.
 */
typedef struct MlnResourceBindingSetsInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 firstSet;
    u32 bindingSetCount;
} MlnResourceBindingSetsInheritance;

/**
 * @brief The structure for vertex buffer inheritance.
 */
typedef struct MlnResourceVertexBuffersInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 firstBinding;
    u32 bindingCount;
} MlnResourceVertexBuffersInheritance;

/**
 * @brief The structure for index buffer inheritance.
 */
typedef struct MlnResourceIndexBufferInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
} MlnResourceIndexBufferInheritance;

/**
 * @brief The structure for viewport inheritance.
 */
typedef struct MlnStateViewportsInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 firstViewport;
    u32 viewportCount;
} MlnStateViewportsInheritance;

/**
 * @brief The structure for scissor inheritance.
 */
typedef struct MlnStateScissorsInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 firstScissor;
    u32 scissorCount;
} MlnStateScissorsInheritance;

/**
 * @brief The structure for fragment shading rate inheritance.
 */
typedef struct MlnFragmentShadingRateInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
} MlnFragmentShadingRateInheritance;

/**
 * @brief The descriptor for graphics object group inheritance.
 */
typedef struct MlnGraphicsObjectGroupInheritanceDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnResourceBindingSetsInheritance* bindingSet;
    const MlnResourceVertexBuffersInheritance* vertexBuffer;
    const MlnResourceIndexBufferInheritance* indexBuffer;
    const MlnStateViewportsInheritance* viewport;
    const MlnStateScissorsInheritance* scissor;
    const MlnFragmentShadingRateInheritance* fragmentShadingRate;
} MlnGraphicsObjectGroupInheritanceDescriptor;

/**
 * @brief The descriptor for creating graphics object group.
 */
typedef struct MlnGraphicsObjectGroupDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnGraphicsObjectGroupDescriptorFlags flags;
    MlnProgram program;
    const MlnResourceSetDescriptor* resourceSet;
    const MlnStateSetDescriptor* stateSet;
    u32 commandCount;
    const MlnGraphicsCommandDescriptor* commands;
    const MlnGraphicsObjectGroupInheritanceDescriptor* inheritance;
} MlnGraphicsObjectGroupDescriptor;

/**
 * @brief The descriptor for creating graphics object group.
 */
typedef struct MlnBindingSetsDynamicOffsets {
    u32 firstSet;
    u32 dynamicOffsetCount;
    const u32* dynamicOffsets;
} MlnBindingSetsDynamicOffsets;

/**
 * @brief The union for updating graphics object group modifier data.
 */
typedef union MlnGraphicsObjectGroupModifierData {
    const MlnResourceBindingSets* bindingSet;
    const MlnResourceProgramConstants* programConstant;
    const MlnResourceVertexBuffers* vertexBuffer;
    const MlnResourceIndexBuffer* indexBuffer;
    const MlnDrawCommand* draw;
    const MlnDrawIndexedCommand* drawIndexed;
    const MlnDrawIndirectCommand* drawIndirect;
    const MlnDrawIndexedIndirectCommand* drawIndexedIndirect;
    const MlnDrawIndirectCountCommand* drawIndirectCount;
    const MlnDrawIndexedIndirectCountCommand* drawIndexedIndirectCount;
    const MlnViewportDynamicState* viewport;
    const MlnScissorDynamicState* scissor;
    f32 lineWidth;
    const MlnDepthBiasDynamicState* depthBias;
    f32 blendConstants[4];
    const MlnDepthBoundsDynamicState* depthBound;
    const MlnStencilCompareMaskDynamicState* stencilCompareMask;
    const MlnStencilWriteMaskDynamicState* stencilWriteMask;
    const MlnStencilReferenceDynamicState* stencilReference;
    const MlnBindingSetsDynamicOffsets* dynamicOffsets;
    const MlnSampleLocationDynamicState* sampleLocation;
    u32 viewportCount;
    u32 scissorCount;
    const MlnFragmentShadingRateDynamicState* fragmentShadingRate;
} MlnGraphicsObjectGroupModifierData;

/**
 * @brief The structure for updating graphics object group modifier data.
 */
typedef struct MlnGraphicsObjectGroupModifierContent {
    MlnGraphicsObjectGroupModifierType type;
    MlnGraphicsObjectGroupModifierData data;
} MlnGraphicsObjectGroupModifierContent;

/**
 * @brief The structure for updating graphics object group modifier.
 */
typedef struct MlnGraphicsObjectGroupModifier {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 commandIndex;
    u32 contentCount;
    const MlnGraphicsObjectGroupModifierContent* contents;
} MlnGraphicsObjectGroupModifier;

/**
 * @brief The descriptor for updating graphics object group.
 */
typedef struct MlnGraphicsObjectGroupUpdateDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnGraphicsObjectGroupUpdateDescriptorFlags flags;
    u32 modifierCount;
    const MlnGraphicsObjectGroupModifier* modifiers;
} MlnGraphicsObjectGroupUpdateDescriptor;

/**
 * @brief The structure for compute base group.
 */
typedef struct MlnComputeBaseGroup {
    u32 baseGroupX;
    u32 baseGroupY;
    u32 baseGroupZ;
} MlnComputeBaseGroup;

/**
 * @brief The structure for compute group count.
 */
typedef struct MlnComputeGroupCount {
    u32 groupCountX;
    u32 groupCountY;
    u32 groupCountZ;
} MlnComputeGroupCount;

/**
 * @brief The structure for compute group indirect count.
 */
typedef struct MlnComputeGroupCountIndirect {
    MlnResource bufferResource;
    MlnDeviceSize offset;
} MlnComputeGroupCountIndirect;

/**
 * @brief The union for compute group count.
 */
typedef union MlnComputeCommandData {
    const MlnComputeGroupCount* groupCount;
    const MlnComputeGroupCountIndirect* groupCountIndirect;
} MlnComputeCommandData;

/**
 * @brief The structure for compute command.
 */
typedef struct MlnComputeCommand {
    const MlnComputeBaseGroup* baseGroup;
    MlnComputeCommandType type;
    MlnComputeCommandData data;
} MlnComputeCommand;

/**
 * @brief The descriptor for compute command.
 */
typedef struct MlnComputeCommandDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnComputeCommandDescriptorFlags flags;
    const MlnComputeCommand* command;
    const MlnResourceBindingSets* bindingSet;
    const MlnResourceProgramConstants* programConstant;
} MlnComputeCommandDescriptor;

/**
 * @brief The descriptor for compute object group inheritance.
 */
typedef struct MlnComputeObjectGroupInheritanceDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnResourceBindingSetsInheritance* bindingSet;
} MlnComputeObjectGroupInheritanceDescriptor;

/**
 * @brief The descriptor for creating compute object group.
 */
typedef struct MlnComputeObjectGroupDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnComputeObjectGroupDescriptorFlags flags;
    MlnProgram program;
    const MlnResourceBindingSets* bindingSet;
    const MlnResourceProgramConstants* programConstant;
    u32 commandCount;
    const MlnComputeCommandDescriptor* commands;
    const MlnComputeObjectGroupInheritanceDescriptor* inheritance;
} MlnComputeObjectGroupDescriptor;

/**
 * @brief The union for updating compute object group modifier data.
 */
typedef union MlnComputeObjectGroupModifierData {
    const MlnResourceBindingSets* bindingSet;
    const MlnResourceProgramConstants* programConstant;
    const MlnComputeBaseGroup* baseGroup;
    const MlnComputeGroupCount* groupCount;
    const MlnComputeGroupCountIndirect* groupCountIndirect;
    const MlnBindingSetsDynamicOffsets* dynamicOffsets;
} MlnComputeObjectGroupModifierData;

/**
 * @brief The structure for updating compute object group modifier data.
 */
typedef struct MlnComputeObjectGroupModifierContent {
    MlnComputeObjectGroupModifierType type;
    MlnComputeObjectGroupModifierData data;
} MlnComputeObjectGroupModifierContent;

/**
 * @brief The structure for updating compute object group modifier.
 */
typedef struct MlnComputeObjectGroupModifier {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 commandIndex;
    u32 contentCount;
    const MlnComputeObjectGroupModifierContent* contents;
} MlnComputeObjectGroupModifier;

/**
 * @brief The descriptor for updating compute object group.
 */
typedef struct MlnComputeObjectGroupUpdateDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnComputeObjectGroupUpdateDescriptorFlags flags;
    u32 modifierCount;
    const MlnComputeObjectGroupModifier* modifiers;
} MlnComputeObjectGroupUpdateDescriptor;

/**
 * @brief The region for copying buffer.
 */
typedef struct MlnBufferCopyRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceSize srcOrigin;
    MlnDeviceSize dstOrigin;
    MlnDeviceSize size;
} MlnBufferCopyRegion;

/**
 * @brief The descriptor for copying buffer.
 */
typedef struct MlnCopyBufferDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource srcBufferResource;
    MlnResource dstBufferResource;
    u32 regionCount;
    const MlnBufferCopyRegion* regions;
} MlnCopyBufferDescriptor;

/**
 * @brief The descriptor for filling buffer.
 */
typedef struct MlnFillBufferDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource bufferResource;
    MlnDeviceSize dstOrigin;
    MlnDeviceSize size;
    u32 data;
} MlnFillBufferDescriptor;

/**
 * @brief The descriptor for updating buffer.
 */
typedef struct MlnUpdateBufferDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource bufferResource;
    MlnDeviceSize dstOrigin;
    MlnDeviceSize size;
    const void* data;
} MlnUpdateBufferDescriptor;

/**
 * @brief The structure for image subresources.
 */
typedef struct MlnImageSubresourceLayers {
    MlnImageAspectFlags aspectMask;
    u32 mipLevel;
    u32 baseArrayLayer;
    u32 layerCount;
} MlnImageSubresourceLayers;

/**
 * @brief The region for copying from buffer to image or image to buffer.
 */
typedef struct MlnBufferImageCopyRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceSize bufferOrigin;
    u32 bufferRowLength;
    u32 bufferImageHeight;
    MlnImageSubresourceLayers imageSubresource;
    MlnOrigin3D imageOrigin;
    MlnSize3D imageSize;
} MlnBufferImageCopyRegion;

/**
 * @brief The descriptor for copying from buffer to image.
 */
typedef struct MlnCopyBufferToImageDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource srcBufferResource;
    MlnResource dstImageResource;
    u32 regionCount;
    const MlnBufferImageCopyRegion* regions;
} MlnCopyBufferToImageDescriptor;

/**
 * @brief The region for copying image.
 */
typedef struct MlnImageCopyRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnImageSubresourceLayers srcSubresource;
    MlnOrigin3D srcOrigin;
    MlnImageSubresourceLayers dstSubresource;
    MlnOrigin3D dstOrigin;
    MlnSize3D size;
} MlnImageCopyRegion;

/**
 * @brief The descriptor for copying image.
 */
typedef struct MlnCopyImageDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource srcImageResource;
    MlnResource dstImageResource;
    u32 regionCount;
    const MlnImageCopyRegion* regions;
} MlnCopyImageDescriptor;

/**
 * @brief The descriptor for copying from image to buffer.
 */
typedef struct MlnCopyImageToBufferDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource srcImageResource;
    MlnResource dstBufferResource;
    u32 regionCount;
    const MlnBufferImageCopyRegion* regions;
} MlnCopyImageToBufferDescriptor;

/**
 * @brief The region for blitting image.
 */
typedef struct MlnImageBlitRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnImageSubresourceLayers srcSubresource;
    MlnOrigin3D srcOrigins[2];
    MlnImageSubresourceLayers dstSubresource;
    MlnOrigin3D dstOrigins[2];
} MlnImageBlitRegion;

/**
 * @brief The descriptor for blitting image.
 */
typedef struct MlnBlitImageDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource srcImageResource;
    MlnResource dstImageResource;
    u32 regionCount;
    const MlnImageBlitRegion* regions;
    MlnFilter filter;
} MlnBlitImageDescriptor;

/**
 * @brief The region for resolving image.
 */
typedef struct MlnImageResolveRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnImageSubresourceLayers srcSubresource;
    MlnOrigin3D srcOrigin;
    MlnImageSubresourceLayers dstSubresource;
    MlnOrigin3D dstOrigin;
    MlnSize3D size;
} MlnImageResolveRegion;

/**
 * @brief The descriptor for resolving image.
 */
typedef struct MlnResolveImageDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource srcImageResource;
    MlnResource dstImageResource;
    u32 regionCount;
    const MlnImageResolveRegion* regions;
} MlnResolveImageDescriptor;

/**
 * @brief The parameters for generating Mipmap.
 */
typedef struct MlnImageGenMipMapRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnImageSubresourceLayers srcSubresource;
    MlnImageSubresourceLayers dstSubresource;
    MlnComponentMapping componentMapping;
    u32 needPatchComponentMapping;
    u32 srgbSkipDecode;
} MlnImageGenMipMapRegion;

/**
 * @brief The descriptor for generating Mipmap.
 */
typedef struct MlnGenerateMipmapDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource imageResource;
    u32 regionCount;
    const MlnImageGenMipMapRegion* regions;
} MlnGenerateMipmapDescriptor;

/**
 * @brief The region for clearing image.
 */
typedef struct MlnClearImageRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnImageAspectFlags aspectMask;
    u32 baseMipLevel;
    u32 levelCount;
    u32 baseArrayLayer;
    u32 layerCount;
} MlnClearImageRegion;

/**
 * @brief The union for clear color.
 */
typedef union MlnClearColor {
    f32 f[4];
    s32 s[4];
    u32 u[4];
} MlnClearColor;

/**
 * @brief The structure for clear depth and stencil.
 */
typedef struct MlnClearDepthStencil {
    f32 depth;
    u32 stencil;
} MlnClearDepthStencil;

/**
 * @brief The union for clear value.
 */
typedef union MlnClearValue {
    MlnClearColor color;
    MlnClearDepthStencil depthStencil;
} MlnClearValue;

typedef struct MlnClearImageDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResource imageResource;
    MlnClearValue clearValue;
    u32 regionCount;
    const MlnClearImageRegion* regions;
} MlnClearImageDescriptor;

/**
 * @brief The region for copying query result.
 */
typedef struct MlnQueryPoolResultCopyRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 firstQuery;
    u32 queryCount;
    MlnDeviceSize dstOrigin;
    MlnDeviceSize stride;
} MlnQueryPoolResultCopyRegion;

/**
 * @brief The descriptor for copying query result.
 */
typedef struct MlnCopyQueryPoolResultDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnQueryPool srcQueryPool;
    MlnResource dstBufferResource;
    u32 regionCount;
    const MlnQueryPoolResultCopyRegion* regions;
} MlnCopyQueryPoolResultDescriptor;

/**
 * @brief The region for resetting query.
 */
typedef struct MlnQueryPoolResetRegion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 firstQuery;
    u32 queryCount;
} MlnQueryPoolResetRegion;

/**
 * @brief The descriptor for resetting query.
 */
typedef struct MlnResetQueryPoolDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnQueryPool queryPool;
    u32 regionCount;
    const MlnQueryPoolResetRegion* regions;
} MlnResetQueryPoolDescriptor;

/**
 * @brief The union to describe different types of data transfer command.
 */
typedef union MlnTransferCommandData {
    const MlnCopyBufferDescriptor* copyBufferDesc;
    const MlnCopyImageDescriptor* copyImageDesc;
    const MlnCopyBufferToImageDescriptor* copyBufferToImageDesc;
    const MlnCopyImageToBufferDescriptor* copyImageToBufferDesc;
    const MlnBlitImageDescriptor* blitImageDesc;
    const MlnResolveImageDescriptor* resolveImageDesc;
    const MlnGenerateMipmapDescriptor* generateMipmapDesc;
    const MlnClearImageDescriptor* clearImageDesc;
    const MlnFillBufferDescriptor* fillBufferDesc;
    const MlnUpdateBufferDescriptor* updateBufferDesc;
    const MlnCopyQueryPoolResultDescriptor* copyQueryPoolResultDesc;
    const MlnResetQueryPoolDescriptor* resetQueryPoolDesc;
} MlnTransferCommandData;

/**
 * @brief The descriptor for transfer command.
 */
typedef struct MlnTransferCommandDescriptor {
    MlnTransferCommandType type;
    MlnTransferCommandData data;
} MlnTransferCommandDescriptor;

/**
 * @brief The descriptor for creating transfer object group.
 */
typedef struct MlnTransferObjectGroupDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTransferObjectGroupDescriptorFlags flags;
    u32 commandCount;
    const MlnTransferCommandDescriptor* commands;
} MlnTransferObjectGroupDescriptor;

/**
 * @brief The descriptor for clear attachment.
 */
typedef struct MlnClearAttachment {
    MlnImageAspectFlags aspectMask;
    u32 colorAttachment;
    MlnClearValue clearValue;
} MlnClearAttachment;

/**
 * @brief The descriptor for clear rect.
 */
typedef struct MlnClearRect {
    MlnRect2D rect;
    u32 baseArrayLayer;
    u32 layerCount;
} MlnClearRect;

/**
 * @brief The descriptor for creating clear object group.
 */
typedef struct MlnClearObjectGroupDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnClearObjectGroupDescriptorFlags flags;
    u32 attachmentCount;
    const MlnClearAttachment* attachments;
    u32 rectCount;
    const MlnClearRect* rects;
} MlnClearObjectGroupDescriptor;

/**
 * @brief The memory barrier.
 */
typedef struct MlnMemoryBarrier {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnProgramStageFlags srcStageMask;
    MlnAccessFlags srcAccessMask;
    MlnProgramStageFlags dstStageMask;
    MlnAccessFlags dstAccessMask;
} MlnMemoryBarrier;

/**
 * @brief The buffer memory barrier.
 */
typedef struct MlnBufferMemoryBarrier {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnProgramStageFlags srcStageMask;
    MlnAccessFlags srcAccessMask;
    MlnProgramStageFlags dstStageMask;
    MlnAccessFlags dstAccessMask;
    MlnResource bufferResource;
    MlnDeviceSize offset;
    MlnDeviceSize size;
} MlnBufferMemoryBarrier;

/**
 * @brief The image memory barrier.
 */
typedef struct MlnImageMemoryBarrier {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnProgramStageFlags srcStageMask;
    MlnAccessFlags srcAccessMask;
    MlnProgramStageFlags dstStageMask;
    MlnAccessFlags dstAccessMask;
    MlnImageLayout oldLayout;
    MlnImageLayout newLayout;
    MlnResource imageResource;
    MlnImageSubresourceRange subresourceRange;
} MlnImageMemoryBarrier;

/**
 * @brief The descriptor for creating barrier object group.
 */
typedef struct MlnBarrierObjectGroupDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnBarrierObjectGroupDescriptorFlags flags;
    u32 memoryBarrierCount;
    const MlnMemoryBarrier* memoryBarriers;
    u32 bufferMemoryBarrierCount;
    const MlnBufferMemoryBarrier* bufferMemoryBarriers;
    u32 imageMemoryBarrierCount;
    const MlnImageMemoryBarrier* imageMemoryBarriers;
} MlnBarrierObjectGroupDescriptor;

/**
 * @brief The descriptor for creating graphics data graph.
 */
typedef struct MlnGraphicsDataGraphDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnGraphicsDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const MlnObjectGroup* objectGroups;
    MlnRenderTarget renderTarget;
} MlnGraphicsDataGraphDescriptor;

/**
 * @brief The descriptor for creating compute data graph.
 */
typedef struct MlnComputeDataGraphDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnComputeDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const MlnObjectGroup* objectGroups;
} MlnComputeDataGraphDescriptor;

/**
 * @brief The descriptor for creating transfer data graph.
 */
typedef struct MlnTransferDataGraphDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTransferDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const MlnObjectGroup* objectGroups;
} MlnTransferDataGraphDescriptor;

/**
 * @brief The descriptor for acceleration structure data graph.
 */
typedef struct MlnAccelerationStructureDataGraphDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnAccelerationStructureDataGraphDescriptorFlags flags;
    u32 objectGroupCount;
    const MlnObjectGroup* objectGroups;
} MlnAccelerationStructureDataGraphDescriptor;

/**
 * @brief The data graph range query coolection data.
 */
typedef struct MlnQueryRangeCollection {
    u32 beginObjectGroupIndex;
    u32 beginCommandIndex;
    u32 endObjectGroupIndex;
    u32 endCommandIndex;
} MlnQueryRangeCollection;

/**
 * @brief The data graph point stamp data.
 */
typedef struct MlnQueryPointCollection {
    u32 objectGroupIndex;
    u32 commandIndex;
    MlnProgramStageFlags programStageFlags;
} MlnQueryPointCollection;

/**
 * @brief The data graph sample query collection data.
 */
typedef struct MlnQuerySampleCollection {
    MlnQuerySampleCollectionLocation location;
    MlnQuerySampleCollectionMode mode;
} MlnQuerySampleCollection;

/**
 * @brief The data graph object property query coolection data.
 */
typedef struct MlnQueryObjectPropertyCollection {
    MlnQuerySampleCollectionLocation location;
    u64 objectHandle;
} MlnQueryObjectPropertyCollection;

/**
 * @brief The union for query collection data variants.
 */
typedef union MlnQueryCollectionData {
    const MlnQueryRangeCollection* range;
    const MlnQueryPointCollection* point;
    const MlnQuerySampleCollection* sample;
    const MlnQueryObjectPropertyCollection* objectProperty;
} MlnQueryCollectionData;

/**
 * @brief The descriptor for data graph query.
 */
typedef struct MlnDataGraphQueryDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnQueryPool queryPool;
    u32 query;
    MlnCounterSet counterSet;
    MlnQueryCollectionType collectionType;
    MlnQueryCollectionData data;
} MlnDataGraphQueryDescriptor;

/**
 * @brief The descriptor for conditional rendering range.
 */
typedef struct MlnConditionalRenderingRange {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnConditionalRenderingFlags flags;
    MlnResource buffer;
    MlnDeviceSize offset;
    u32 beginObjectGroupIndex;
    u32 beginCommandIndex;
    u32 endObjectGroupIndex;
    u32 endCommandIndex;
} MlnConditionalRenderingRange;

/**
 * @brief The descriptor for pass node resource.
 */
typedef struct MlnPassNodeResourceDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnPassNodeResourceType type;
    MlnResourceView imageResourceView;
    MlnResource bufferResource;
} MlnPassNodeResourceDescriptor;

/**
 * @brief The descriptor for pass node dependency.
 */
typedef struct MlnPassNodeDependencyDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u64 passId;
    u32 depResourceCount;
    const MlnPassNodeResourceDescriptor* depResources;
    MlnProgramStageFlags srcStage;
    MlnProgramStageFlags dstStage;
    u32 filterMargin;
} MlnPassNodeDependencyDescriptor;

/**
 * @brief The descriptor for pass node workload.
 */
typedef struct MlnPassNodeWorkloadDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnPassNodeHintType hint;
    MlnPassNodeWorkloadType type;
    u32 workload;
} MlnPassNodeWorkloadDescriptor;

/**
 * @brief The descriptor for pass node.
 */
typedef struct MlnPassNodeDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnPassNodeDescriptorFlags flags;
    u64 passId;
    u32 outputCount;
    const MlnPassNodeResourceDescriptor* outputResources;
    const MlnAttachmentStoreOp* storeop;
    u32 depCount;
    const MlnPassNodeDependencyDescriptor* depPasses;
    const MlnPassNodeWorkloadDescriptor* workload;
} MlnPassNodeDescriptor;

/**
 * @brief The descriptor for camera.
 */
typedef struct MlnCameraDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const f32* viewMatrix;
    const f32* projectionMatrix;
} MlnCameraDescriptor;

/**
 * @brief The descriptor for creating scheduling graph.
 */
typedef struct MlnSchedulingGraphDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnSchedulingGraphDescriptorFlags flags;
    u32 passNodeCount;
    const MlnPassNodeDescriptor* passNodes;
    const MlnCameraDescriptor* camera;
} MlnSchedulingGraphDescriptor;

/**
 * @brief The structure for graphics object group inheritance content.
 */
typedef struct MlnGraphicsObjectGroupInheritanceContent {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnResourceBindingSets* bindingSets;
    const MlnResourceVertexBuffers* vertexbuffers;
    const MlnResourceIndexBuffer* indexbuffer;
    const MlnViewportDynamicState* viewport;
    const MlnScissorDynamicState* scissor;
    const MlnFragmentShadingRateDynamicState* fragmentShadingRate;
} MlnGraphicsObjectGroupInheritanceContent;

/**
 * @brief The structure for object group inheritance.
 */
typedef struct MlnGraphicsObjectGroupInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnGraphicsObjectGroupInheritanceContent* content;
    u32 objectGroupCount;
    const MlnObjectGroup* objectGroups;
} MlnGraphicsObjectGroupInheritance;

/**
 * @brief The structure for graphics data graph inheritance.
 */
typedef struct MlnGraphicsDataGraphInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnGraphicsObjectGroupInheritanceContent* content;
    u32 objectGroupInheritanceCount;
    const MlnGraphicsObjectGroupInheritance* objectGroupInheritances;
} MlnGraphicsDataGraphInheritance;

/**
 * @brief The structure for compute object group inheritance content.
 */
typedef struct MlnComputeObjectGroupInheritanceContent {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnResourceBindingSets* bindingSets;
} MlnComputeObjectGroupInheritanceContent;

/**
 * @brief The structure for compute object group inheritance.
 */
typedef struct MlnComputeObjectGroupInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnComputeObjectGroupInheritanceContent* content;
    u32 objectGroupCount;
    const MlnObjectGroup* objectGroups;
} MlnComputeObjectGroupInheritance;

/**
 * @brief The structure for compute data graph inheritance.
 */
typedef struct MlnComputeDataGraphInheritance {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const MlnComputeObjectGroupInheritanceContent* content;
    u32 objectGroupInheritanceCount;
    const MlnComputeObjectGroupInheritance* objectGroupInheritances;
} MlnComputeDataGraphInheritance;

/**
 * @brief The union descriptor for data graph inheritance.
 */
typedef union MlnDataGraphInheritanceDescriptor {
    const MlnGraphicsDataGraphInheritance* graphicsInheritance;
    const MlnComputeDataGraphInheritance* computeInheritance;
} MlnDataGraphInheritanceDescriptor;

/**
 * @brief The descriptor for submitted data graph.
 */
typedef struct MlnDataGraphSubmitDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDataGraphSubmitDescriptorFlags flags;
    MlnDataGraph dataGraph;
    u64 passId;
    const MlnDataGraphInheritanceDescriptor* inheritance;
    u32 queriesCount;
    const MlnDataGraphQueryDescriptor* queries;
    u32 conditionalRenderingRangeCount;
    const MlnConditionalRenderingRange* conditionalRenderingRanges;
} MlnDataGraphSubmitDescriptor;

/**
 * @brief The descriptor for submitted scheduling graph.
 */
typedef struct MlnSchedulingGraphSubmitDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnSchedulingGraphSubmitDescriptorFlags flags;
    MlnSchedulingGraph schedulingGraph;
    u32 dataGraphCount;
    const MlnDataGraphSubmitDescriptor* dataGraphs;
} MlnSchedulingGraphSubmitDescriptor;

/**
 * @brief The descriptor for creating timeline.
 */
typedef struct MlnTimelineDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTimelineDescriptorFlags flags;
    MlnTimelineType type;
    u64 initialValue;
} MlnTimelineDescriptor;

/**
 * @brief The descriptor for the wait operation.
 */
typedef struct MlnTimelineWaitDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTimelineWaitFlags flags;
    u32 timelineCount;
    const MlnTimeline* timelines;
    const u64* values;
} MlnTimelineWaitDescriptor;

/**
 * @brief The descriptor for the signal operation.
 */
typedef struct MlnTimelineSignalDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTimelineSignalFlags flags;
    u32 timelineCount;
    const MlnTimeline* timelines;
    const u64* values;
} MlnTimelineSignalDescriptor;

/**
 * @brief The descriptor for the export operation.
 */
typedef struct MlnTimelineExportFdDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTimelineExportFlags flags;
    MlnTimeline timeline;
} MlnTimelineExportFdDescriptor;

/**
 * @brief The descriptor for the import operation.
 */
typedef struct MlnTimelineImportFdDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTimelineImportFlags flags;
    MlnTimeline timeline;
    int fd;
} MlnTimelineImportFdDescriptor;

/**
 * @brief The descriptor for the import event id.
 */
typedef struct MlnTimelineImportEventIdDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTimeline timeline;
    u32 eventId;
} MlnTimelineImportEventIdDescriptor;

/**
 * @brief The descriptor for submitted time line.
 */
typedef struct MlnTimelineSubmitDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnTimeline timeline;
    u64 value;
    u32 passCount;
    const u64* passIds;
    const MlnProgramStageFlags* stageMasks;
} MlnTimelineSubmitDescriptor;

/**
 * @brief The descriptor for submitted queue task.
 */
typedef struct MlnSubmitDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnSubmitDescriptorFlags flags;
    u32 schedulingGraphCount;
    const MlnSchedulingGraphSubmitDescriptor* schedulingGraphs;
    u32 waitTimelineCount;
    const MlnTimelineSubmitDescriptor* waitTimelines;
    u32 signalTimelineCount;
    const MlnTimelineSubmitDescriptor* signalTimelines;
} MlnSubmitDescriptor;

/**
 * @brief The descriptor for attachment.
 */
typedef struct MlnAttachmentDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnResourceView imageResourceView;
    MlnImageLayout imageLayout;
    MlnResolveModeFlags resolveMode;
    MlnResourceView resolveResourceView;
    MlnImageLayout resolveImageLayout;
    MlnAttachmentLoadOp loadOp;
    MlnAttachmentStoreOp storeOp;
    MlnClearValue clearValue;
} MlnAttachmentDescriptor;

/**
 * @brief The descriptor for fragment shading rate attachment.
 */
typedef struct MlnFragmentShadingRateAttachmentDescriptor {
    MlnResourceView imageResourceView;
    MlnSize2D shadingRateAttachmentTexelSize;
} MlnFragmentShadingRateAttachmentDescriptor;

/**
 * @brief The descriptor for creating render target.
 */
typedef struct MlnRenderTargetDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnRenderTargetDescriptorFlags flags;
    MlnRect2D renderArea;
    u32 layerCount;
    u32 viewMask;
    u32 colorCount;
    const MlnAttachmentDescriptor* colors;
    const MlnAttachmentDescriptor* depth;
    const MlnAttachmentDescriptor* stencil;
    const MlnFragmentShadingRateAttachmentDescriptor* fragmentShadingRate;
} MlnRenderTargetDescriptor;

/**
 * @brief The structure for display surface format.
 */
typedef struct MlnDisplaySurfaceFormat {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnFormat format;
    MlnColorSpace colorSpace;
    MlnLossyTypes lossyType;
} MlnDisplaySurfaceFormat;

/**
 * @brief The structure for display surface capabilities.
 */
typedef struct MlnDisplaySurfaceCapabilities {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 minImageCount;
    u32 maxImageCount;
    MlnSize2D currentExtent;
    MlnSize2D minImageExtent;
    MlnSize2D maxImageExtent;
    u32 maxImageArrayLayers;
    MlnDisplaySurfaceTransformFlagBits currentTransform;
    MlnCompositeAlphaFlags supportedCompositeAlpha;
    MlnImageUsageFlags supportedUsageFlags;
    b32 supportProtectedContent;
} MlnDisplaySurfaceCapabilities;

/**
 * @brief The descriptor for creating display surface.
 */
typedef struct MlnDisplaySurfaceDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDisplaySurfaceDescriptorFlags flags;
    const OHNativeWindow* nativeWindow;
} MlnDisplaySurfaceDescriptor;

/**
 * @brief The descriptor for creating swapchain.
 */
typedef struct MlnSwapchainDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnSwapchainDescriptorFlags flags;
    MlnDisplaySurface displaySurface;
    u32 minImageCount;
    const MlnDisplaySurfaceFormat* surfaceFormat;
    MlnSize2D imageExtent;
    u32 imageArrayLayers;
    MlnImageUsageFlags imageUsage;
    MlnDisplaySurfaceTransformFlagBits preTransform;
    MlnCompositeAlphaFlagBits compositeAlpha;
    MlnPresentMode presentMode;
    b32 clipped;
    b32 protectedContent;
} MlnSwapchainDescriptor;

/**
 * @brief The descriptor for presenting swapchain.
 */
typedef struct MlnPresentDescriptor {
    MlnSwapchain swapchain;
    u32 imageIndex;
    u32 waitCount;
    const MlnTimeline* waitTimelines;
    const u64* waitValues;
    MlnTimeline presentTimeline;
    u64 presentValue;
    u32 updateRegionCount;
    const MlnSize2D* updateRegionSizes;
    const MlnOrigin2D* updateRegionOffsets;
} MlnPresentDescriptor;

/**
 * @brief The descriptor for private data slot.
 */
typedef struct MlnPrivateDataSlotDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnPrivateDataSlotDescriptorFlags flags;
} MlnPrivateDataSlotDescriptor;

/**
 * @brief The descriptor for set private data.
 */
typedef struct MlnPrivateDataSetDescriptor {
    MlnObjectType objectType;
    const void* objectHandle;
    MlnPrivateDataSlot privateDataSlot;
    u64 value;
} MlnPrivateDataSetDescriptor;

/**
 * @brief The descriptor for get private data.
 */
typedef struct MlnPrivateDataGetDescriptor {
    MlnObjectType objectType;
    const void* objectHandle;
    MlnPrivateDataSlot privateDataSlot;
    u64* outValue;
} MlnPrivateDataGetDescriptor;

/**
 * @brief The Positions for AABB.
 */
typedef struct MlnAabbPositions {
    f32 minX;
    f32 minY;
    f32 minZ;
    f32 maxX;
    f32 maxY;
    f32 maxZ;
} MlnAabbPositions;

/**
 * @brief The struct for transform matrix.
 */
typedef struct MlnTransformMatrix {
    f32 matrix[3][4];
} MlnTransformMatrix;

/**
 * @brief The descriptor for ellipsoid culling.
 */
typedef struct MlnEllipsoidCullingDescriptor {
    f32 minAlpha;
    f32 maxOpacity;
    f32 cutOff;
} MlnEllipsoidCullingDescriptor;

/**
 * @brief The structure for acceleration structure instance.
 */
typedef struct MlnAccelerationStructureInstance {
    MlnTransformMatrix transform;
    u32 instanceCustomIndex : 24;
    u32 mask : 8;
    u32 instanceShaderBindingTableRecordOffset : 24;
    MlnAccelerationStructureInstanceFlags flags : 8;
    u64 accelerationStructureReference;
} MlnAccelerationStructureInstance;

/**
 * @brief The structure for ellipsoid parameters.
 */
typedef struct MlnEllipsoid {
    f32 center[3];
    f32 rotation[4];
    f32 scaling[3];
    f32 opacity;
    b32 shCoeffHalfPrecision;
    f32 shCoeffOffset;
} MlnEllipsoid;

/**
 * @brief The descriptor for acceleration structure geometry triangles.
 */
typedef struct MlnAccelerationStructureGeometryTrianglesDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnFormat vertexFormat;
    MlnDeviceAddress vertexData;
    MlnDeviceSize vertexStride;
    u32 maxVertex;
    MlnIndexType indexType;
    MlnDeviceAddress indexData;
    MlnDeviceAddress transformData;
} MlnAccelerationStructureGeometryTrianglesDescriptor;

/**
 * @brief The descriptor for acceleration structure geometry AABBs.
 */
typedef struct MlnAccelerationStructureGeometryAabbsDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceAddress data;
    MlnDeviceSize stride;
} MlnAccelerationStructureGeometryAabbsDescriptor;

/**
 * @brief The descriptor for acceleration structure geometry instances.
 */
typedef struct MlnAccelerationStructureGeometryInstancesDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    b32 arrayOfPointers;
    MlnDeviceAddress data;
} MlnAccelerationStructureGeometryInstancesDescriptor;

/**
 * @brief The descriptor for acceleration structure geometry ellipsoid.
 */
typedef struct MlnAccelerationStructureGeometryEllipsoidDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceAddress ellipsoidData;
    MlnDeviceAddress ellipsoidSHCoeffData;
    u32 ellipsoidCount;
    u32 ellipsoidStride;
    f32 shOrder;
    u32 shCoeffCount;
    u32 shCoeffStride;
} MlnAccelerationStructureGeometryEllipsoidDescriptor;

/**
 * @brief The union for acceleration structure geometry data variants.
 */
typedef union MlnAccelerationStructureGeometryData {
    MlnAccelerationStructureGeometryTrianglesDescriptor triangles;
    MlnAccelerationStructureGeometryAabbsDescriptor aabbs;
    MlnAccelerationStructureGeometryInstancesDescriptor instances;
    MlnAccelerationStructureGeometryEllipsoidDescriptor ellipsoids;
} MlnAccelerationStructureGeometryData;

/**
 * @brief The descriptor for acceleration structure geometry.
 */
typedef struct MlnAccelerationStructureGeometryDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnGeometryType geometryType;
    MlnAccelerationStructureGeometryData geometry;
    MlnGeometryFlags flags;
} MlnAccelerationStructureGeometryDescriptor;

/**
 * @brief The descriptor for building acceleration structure geometry.
 */
typedef struct MlnAccelerationStructureBuildGeometryDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnAccelerationStructureType type;
    MlnBuildAccelerationStructureFlags flags;
    MlnAccelerationStructureBuildMode mode;
    MlnAccelerationStructure srcAccelerationStructure;
    MlnAccelerationStructure dstAccelerationStructure;
    u32 geometryCount;
    const MlnAccelerationStructureGeometryDescriptor* geometries;
    const MlnAccelerationStructureGeometryDescriptor* const *
        geometriesPointers;
    MlnDeviceAddress scratchData;
} MlnAccelerationStructureBuildGeometryDescriptor;

/**
 * @brief The descriptor for acceleration structure build range.
 */
typedef struct MlnAccelerationStructureBuildRangeDescriptor {
    u32 primitiveCount;
    u32 primitiveOffset;
    u32 firstVertex;
    u32 transformOffset;
} MlnAccelerationStructureBuildRangeDescriptor;

/**
 * @brief The command for building acceleration structure.
 */
typedef struct MlnAccelerationStructureBuildCommand {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    u32 infoCount;
    const MlnAccelerationStructureBuildGeometryDescriptor* buildGeometry;
    const MlnAccelerationStructureBuildRangeDescriptor* const * buildRanges;
} MlnAccelerationStructureBuildCommand;

/**
 * @brief The command for copying acceleration structure.
 */
typedef struct MlnAccelerationStructureCopyCommand {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnAccelerationStructureCopyMode mode;
    MlnAccelerationStructure src;
    MlnAccelerationStructure dst;
} MlnAccelerationStructureCopyCommand;

/**
 * @brief The command for serializing acceleration structure.
 */
typedef struct MlnAccelerationStructureSerializeCommand {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnAccelerationStructure src;
    MlnDeviceAddress dst;
} MlnAccelerationStructureSerializeCommand;

/**
 * @brief The command for deserializing acceleration structure.
 */
typedef struct MlnAccelerationStructureDeserializeCommand {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceAddress src;
    MlnAccelerationStructure dst;
} MlnAccelerationStructureDeserializeCommand;

/**
 * @brief The union for acceleration structure command data variants.
 */
typedef union MlnAccelerationStructureCommandData {
    const MlnAccelerationStructureBuildCommand* build;
    const MlnAccelerationStructureCopyCommand* copy;
    const MlnAccelerationStructureSerializeCommand* serialize;
    const MlnAccelerationStructureDeserializeCommand* deserialize;
} MlnAccelerationStructureCommandData;

/**
 * @brief The descriptor for acceleration structure command.
 */
typedef struct MlnAccelerationStructureCommandDescriptor {
    MlnAccelerationStructureCommandType type;
    MlnAccelerationStructureCommandData data;
} MlnAccelerationStructureCommandDescriptor;

/**
 * @brief The descriptor for acceleration structure.
 */
typedef struct MlnAccelerationStructureDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnAccelerationStructureDescriptorFlags flags;
    const u32 primitiveCount;
    MlnResource buffer;
    MlnDeviceSize offset;
    MlnDeviceSize size;
    MlnAccelerationStructureType type;
    MlnDeviceAddress deviceAddress;
} MlnAccelerationStructureDescriptor;

/**
 * @brief The descriptor for acceleration structure object group.
 */
typedef struct MlnAccelerationStructureObjectGroupDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnAccelerationStructureObjectGroupDescriptorFlags flags;
    u32 commandCount;
    const MlnAccelerationStructureCommandDescriptor* commands;
} MlnAccelerationStructureObjectGroupDescriptor;

/**
 * @brief The descriptor for acceleration structure build sizes.
 */
typedef struct MlnAccelerationStructureBuildSizesDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnDeviceSize accelerationStructureSize;
    MlnDeviceSize updateScratchSize;
    MlnDeviceSize buildScratchSize;
} MlnAccelerationStructureBuildSizesDescriptor;

/**
 * @brief The structure for acceleration structure version info.
 */
typedef struct MlnAccelerationStructureVersion {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    const u8* versionData;
} MlnAccelerationStructureVersion;

/**
 * @brief The descriptor for query pool.
 */
typedef struct MlnQueryPoolDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnQueryPoolDescriptorFlags flags;
    MlnQueryType queryType;
    u32 queryCount;
} MlnQueryPoolDescriptor;

/**
 * @brief The structure for counter-level Gpu capabilities.
 */
typedef struct MlnGpuCounterProperties {
    MlnQueryType queryType;
    MlnCounterSet counterSet;
    MlnDeviceSize querySize;
    MlnDeviceSize offset;
    MlnCounterStorage storage;
    MlnCounterUnit unit;
    u32 collectionTypeCount;
    MlnQueryCollectionType collectionTypes[MLN_MAX_COUNTER_TYPES];
    char name[MLN_MAX_COUNTER_NAME_SIZE];
    char description[MLN_MAX_COUNTER_DESCRIPTION_SIZE];
} MlnGpuCounterProperties;

/**
 * @brief The descriptor for getting query result.
 */
typedef struct MlnGetQueryPoolResultDescriptor {
    u32 extensionCount;
    const MlnExtensionBlock* extensions;
    MlnQueryPool srcQueryPool;
    void* pData;
    u32 regionCount;
    const MlnQueryPoolResultCopyRegion* regions;
} MlnGetQueryPoolResultDescriptor;

#ifdef __cplusplus
}
#endif
