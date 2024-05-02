/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_DEVICE_GPU_RESOURCE_DESC_H
#define API_RENDER_DEVICE_GPU_RESOURCE_DESC_H

#include <cstdint>

#include <base/util/formats.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** Image type */
enum ImageType {
    /** 1D */
    CORE_IMAGE_TYPE_1D = 0,
    /** 2D */
    CORE_IMAGE_TYPE_2D = 1,
    /** 3D */
    CORE_IMAGE_TYPE_3D = 2,
};

/** Image view type */
enum ImageViewType {
    /** 1D */
    CORE_IMAGE_VIEW_TYPE_1D = 0,
    /** 2D */
    CORE_IMAGE_VIEW_TYPE_2D = 1,
    /** 3D */
    CORE_IMAGE_VIEW_TYPE_3D = 2,
    /** Cube */
    CORE_IMAGE_VIEW_TYPE_CUBE = 3,
    /** 1D array */
    CORE_IMAGE_VIEW_TYPE_1D_ARRAY = 4,
    /** 2D array */
    CORE_IMAGE_VIEW_TYPE_2D_ARRAY = 5,
    /** Cube array */
    CORE_IMAGE_VIEW_TYPE_CUBE_ARRAY = 6,
};

/** Image tiling */
enum ImageTiling {
    /** Optimal */
    CORE_IMAGE_TILING_OPTIMAL = 0,
    /** Linear */
    CORE_IMAGE_TILING_LINEAR = 1,
};

/** Image usage flag bits */
enum ImageUsageFlagBits {
    /** Transfer source bit */
    CORE_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    /** Transfer destination bit */
    CORE_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002,
    /** Sampled bit */
    CORE_IMAGE_USAGE_SAMPLED_BIT = 0x00000004,
    /** Storage bit */
    CORE_IMAGE_USAGE_STORAGE_BIT = 0x00000008,
    /** Color attachment bit */
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
    /** Depth stencil attachment bit */
    CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
    /** Transient attachment bit */
    CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT = 0x00000040,
    /** Input attachment bit */
    CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT = 0x00000080,
    /** Fragment shading rate attachment bit */
    CORE_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT = 0x00000100,
};
/** Container for image usage flag bits */
using ImageUsageFlags = uint32_t;

/** Engine image creation flag bits */
enum EngineImageCreationFlagBits {
    /** Dynamic barriers */
    CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS = 0x00000001,
    /** Reset state on frame borders */
    CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS = 0x00000002,
    /** Generate mips */
    CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS = 0x00000004,
    /** Scale image when created from data */
    CORE_ENGINE_IMAGE_CREATION_SCALE = 0x00000008,
    /** Destroy is deferred to the end of the current frame */
    CORE_ENGINE_IMAGE_CREATION_DEFERRED_DESTROY = 0x00000010,
};
/** Container for engine image creation flag bits */
using EngineImageCreationFlags = uint32_t;

/** Engine sampler creation flag bits */
enum EngineSamplerCreationFlagBits {
    /** Create ycbcr sampler */
    CORE_ENGINE_SAMPLER_CREATION_YCBCR = 0x00000001,
};
/** Container for engine sampler creation flag bits */
using EngineSamplerCreationFlags = uint32_t;

/** Image create flag bits */
enum ImageCreateFlagBits {
    /** Cube compatible bit */
    CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT = 0x00000010,
    /** 2D array compatible bit */
    CORE_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT = 0x00000020,
};
/** Container for image create flag bits */
using ImageCreateFlags = uint32_t;

/** Engine buffer creation flag bits */
enum EngineBufferCreationFlagBits {
    /** Dynamic barriers */
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS = 0x00000001,
    /** Dynamic ring buffer */
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER = 0x00000002,
    /** Single shot staging */
    CORE_ENGINE_BUFFER_CREATION_SINGLE_SHOT_STAGING = 0x00000004,
    /** Hint to enable memory optimizations if possible (e.g. on integrated might use direct copies without staging) */
    CORE_ENGINE_BUFFER_CREATION_ENABLE_MEMORY_OPTIMIZATIONS = 0x00000008,
    /** Immediately created GPU resource */
    CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE = 0x00000010,
    /** Destroy is deferred to the end of the current frame */
    CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY = 0x00000020,
    /** Buffer is mappable outside renderer (i.e. outside render nodes). Slower, do not used if not necessary. */
    CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER = 0x00000040,
};
/** Container for engine buffer creation flag bits */
using EngineBufferCreationFlags = uint32_t;

/** Buffer usage flag bits */
enum BufferUsageFlagBits {
    /** Transfer source bit */
    CORE_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    /** Transfer destination bit */
    CORE_BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002,
    /** Uniform texel buffer bit */
    CORE_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
    /** Storage texel buffer bit */
    CORE_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
    /** Uniform buffer bit */
    CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x00000010,
    /** Storage buffer bit */
    CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
    /** Index buffer bit */
    CORE_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040,
    /** Vertex buffer bit */
    CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080,
    /** Indirect buffer bit */
    CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT = 0x00000100,
    /** Acceleration structure binding table bit */
    CORE_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT = 0x00000400,
    /** Shader device address bit */
    CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT = 0x00020000,
    /** Acceleration structure build input read only bit */
    CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT = 0x00080000,
    /** Acceleration structure storage bit */
    CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT = 0x00100000,
};
/** Container for buffer usage flag bits */
using BufferUsageFlags = uint32_t;

/** Filter */
enum Filter {
    /** Nearest */
    CORE_FILTER_NEAREST = 0,
    /** Linear */
    CORE_FILTER_LINEAR = 1,
};

/** Sampler address mode */
enum SamplerAddressMode {
    /** Repeat */
    CORE_SAMPLER_ADDRESS_MODE_REPEAT = 0,
    /** Mirrored repeat */
    CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
    /** Clamp to edge */
    CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    /** Clamp to border */
    CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
    /** Mirror clamp to edge */
    CORE_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
};

/** Border color */
enum BorderColor {
    /** Float transparent black */
    CORE_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 0,
    /** Int transparent black */
    CORE_BORDER_COLOR_INT_TRANSPARENT_BLACK = 1,
    /** Float opaque black */
    CORE_BORDER_COLOR_FLOAT_OPAQUE_BLACK = 2,
    /** Int opaque black */
    CORE_BORDER_COLOR_INT_OPAQUE_BLACK = 3,
    /** Float opaque white */
    CORE_BORDER_COLOR_FLOAT_OPAQUE_WHITE = 4,
    /** Int opaque white */
    CORE_BORDER_COLOR_INT_OPAQUE_WHITE = 5,
};

/** Sample count flag bits */
enum SampleCountFlagBits : uint32_t {
    /** 1 bit */
    CORE_SAMPLE_COUNT_1_BIT = 0x00000001,
    /** 2 bit */
    CORE_SAMPLE_COUNT_2_BIT = 0x00000002,
    /** 4 bit */
    CORE_SAMPLE_COUNT_4_BIT = 0x00000004,
    /** 8 bit */
    CORE_SAMPLE_COUNT_8_BIT = 0x00000008,
    /** 16 bit */
    CORE_SAMPLE_COUNT_16_BIT = 0x00000010,
    /** 32 bit */
    CORE_SAMPLE_COUNT_32_BIT = 0x00000020,
    /** 64 bit */
    CORE_SAMPLE_COUNT_64_BIT = 0x00000040,
};
/** Sample count flags */
using SampleCountFlags = uint32_t;

/** Component swizzle */
enum ComponentSwizzle : uint32_t {
    /** Swizzle identity */
    CORE_COMPONENT_SWIZZLE_IDENTITY = 0,
    /** Swizzle zero */
    CORE_COMPONENT_SWIZZLE_ZERO = 1,
    /** Swizzle one */
    CORE_COMPONENT_SWIZZLE_ONE = 2,
    /** Swizzle red */
    CORE_COMPONENT_SWIZZLE_R = 3,
    /** Swizzle green */
    CORE_COMPONENT_SWIZZLE_G = 4,
    /** Swizzle blue */
    CORE_COMPONENT_SWIZZLE_B = 5,
    /** Swizzle alpha */
    CORE_COMPONENT_SWIZZLE_A = 6,
};

/** Acceleration structure create flag bits */
enum AccelerationStructureCreateFlagBits {};
/** Container for acceleration create flag bits */
using AccelerationStructureCreateFlags = uint32_t;

/** Engine acceleration structure creation flag bits */
enum EngineAccelerationStructureCreationFlagBits {};
/** Container for engine acceleration structure creation flag bits */
using EngineAccelerationStructureCreationFlags = uint32_t;

/**
 * UBO:
 * - BufferUsageFlags:
 * CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT
 * - EngineBufferCreationFlags
 * CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER
 * NOTE: automatically buffered with map-method (ring buffer hidden inside) (the binding offset is updated with
 * UpdateDescriptorSets)
 * - MemoryPropertyFlags
 * CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT
 *
 * VBO:
 * - BufferUsageFlags:
 * CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT
 * - MemoryPropertyFlags:
 * CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
 *
 * SSBO:
 * - BufferUsageFlags:
 * CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT
 * MemoryPropertyFlags
 * CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
 *
 * EngineBufferCreationFlags:
 *
 * CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS, set this flag only for dynamic state changing data (e.g. SSBOs etc)
 * This flag is not needed for regular vertex buffers. Staging handles the copies and barriers automatically.
 *
 * CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, set this for dynamic buffer mapping
 * Dynamic ring buffer updates offsets automatically when buffer is mapped and descriptor set is updated.
 *
 * CORE_ENGINE_BUFFER_CREATION_SINGLE_SHOT_STAGING, tells memory allocator that this resource can be forget after this
 * frame Engine internally flags staging buffers with this flag, Do not use if you cannot guarentee that the gpu buffer
 * is destroyed immediately
 *
 * CORE_ENGINE_BUFFER_CREATION_ENABLE_MEMORY_OPTIMIZATIONS, can enable some memory optimizations.
 * E.g. on some platforms all memory can be host visible (except lazy allocations).
 * With this flag you can make sure that your CORE_BUFFER_USAGE_TRANSFER_DST_BIT buffer
 * Might by-pass staging buffers and enable direct cpu->gpu memcpy on some platforms and use less memory.
 *
 * CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE, creates the resource immediately/instantly so it can be mapped.
 * Should only be used when instant mapping is required.
 *
 * CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY, the resource is destroyed at the end of the frame
 * after Destroy has been called to the GpuResourceHandle.
 * Should only be used when object lifetime cannot be guaranteed in some scoped section.
 * Might increase memory consumption.
 *
 * CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER, the resource is mappable outside
 * renderer. Normal mapping is suggested to be done within render nodes or with default staging memory copy.
 *
 */
/** \addtogroup group_gpuresourcedesc
 *  @{
 */
/** GPU buffer descriptor */
struct GpuBufferDesc {
    /** Usage flags */
    BufferUsageFlags usageFlags { 0 };
    /** Memory property flags */
    MemoryPropertyFlags memoryPropertyFlags { 0 };
    /** Engine creation flags */
    EngineBufferCreationFlags engineCreationFlags { 0 };
    /** Byte size */
    uint32_t byteSize { 0 };
    /** View format (for texel buffer view) */
    BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
};

/** RGBA component mapping */
struct ComponentMapping {
    /* Swizzle for red */
    ComponentSwizzle r { CORE_COMPONENT_SWIZZLE_IDENTITY };
    /* Swizzle for green */
    ComponentSwizzle g { CORE_COMPONENT_SWIZZLE_IDENTITY };
    /* Swizzle for blue */
    ComponentSwizzle b { CORE_COMPONENT_SWIZZLE_IDENTITY };
    /* Swizzle for alpha */
    ComponentSwizzle a { CORE_COMPONENT_SWIZZLE_IDENTITY };
};

/**
 * EngineImageCreationFlags:
 *
 * CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, set this flag only for dynamic state changing data (e.g. render targets)
 * default value (zero, 0), is usually always the correct one (e.g. loaded textures, sampled textures)
 *
 * CORE_ENGINE_IMAGE_CREATION_SCALE, set this if a simple image scaling is needed when staging the
 * Images to the GPU. Linear single pass scaling is used.
 * Final image size is taken from the GpuImageDesc,
 * the input size is taken from the IImageContainer or BufferImageCopies which are given to Create -method.
 */
/** \addtogroup group_gpuresourcedesc
 *  @{
 */
/** GPU image descriptor */
struct GpuImageDesc {
    /** Image type */
    ImageType imageType { ImageType::CORE_IMAGE_TYPE_2D };
    /** Image view type */
    ImageViewType imageViewType { ImageViewType::CORE_IMAGE_VIEW_TYPE_2D };
    /** Format */
    BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
    /** Image tiling */
    ImageTiling imageTiling { ImageTiling::CORE_IMAGE_TILING_OPTIMAL };
    /** Usage tiling */
    ImageUsageFlags usageFlags { 0 };
    /** Memory property flags */
    MemoryPropertyFlags memoryPropertyFlags { 0 };

    /** Create flags */
    ImageCreateFlags createFlags { 0 };

    /** Engine creation flags */
    EngineImageCreationFlags engineCreationFlags { 0 };

    /** Width */
    uint32_t width { 1u };
    /** Height */
    uint32_t height { 1u };
    /** Depth */
    uint32_t depth { 1u };

    /** Mip count */
    uint32_t mipCount { 1u };
    /** Layer count */
    uint32_t layerCount { 1u };

    /** Sample count flags */
    SampleCountFlags sampleCountFlags { SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT };

    /** RGBA component mapping */
    ComponentMapping componentMapping;
};

/** GPU sampler descriptor */
struct GpuSamplerDesc {
    /** Magnification filter */
    Filter magFilter { Filter::CORE_FILTER_NEAREST };
    /** Minification filter */
    Filter minFilter { Filter::CORE_FILTER_NEAREST };
    /** Mip map filter mode */
    Filter mipMapMode { Filter::CORE_FILTER_NEAREST };

    /** Address mode U */
    SamplerAddressMode addressModeU { SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT };
    /** Address mode V */
    SamplerAddressMode addressModeV { SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT };
    /** Address mode W */
    SamplerAddressMode addressModeW { SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT };

    /** Engine creation flags */
    EngineSamplerCreationFlags engineCreationFlags { 0 };

    /** Mip lod bias */
    float mipLodBias { 0.0f };

    /** Enable anisotropy */
    bool enableAnisotropy { false };
    /** Max anisotropy */
    float maxAnisotropy { 1.0f };

    /** Enable compare operation */
    bool enableCompareOp { false };
    /** Compare operation */
    CompareOp compareOp { CompareOp::CORE_COMPARE_OP_NEVER };

    /** Minimum lod */
    float minLod { 0.0f };
    /** Maximum lod */
    float maxLod { 0.0f };

    /** Border color */
    BorderColor borderColor { BorderColor::CORE_BORDER_COLOR_FLOAT_OPAQUE_BLACK };

    /** Enable unnormalized coordinates */
    bool enableUnnormalizedCoordinates { false };
};

/** GPU acceleration structure descriptor */
struct GpuAccelerationStructureDesc {
    /* Acceleration structure type */
    AccelerationStructureType accelerationStructureType {
        AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL
    };
    /* Buffer desc which holds the buffer info */
    GpuBufferDesc bufferDesc;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_GPU_RESOURCE_DESC_H
