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

#ifndef API_RENDER_DEVICE_PIPELINE_STATE_DESC_H
#define API_RENDER_DEVICE_PIPELINE_STATE_DESC_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/util/formats.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_render_pipelinestatedesc
 *  @{
 */
/** Size 2D */
struct Size2D {
    /** Width */
    uint32_t width { 0 };
    /** Height */
    uint32_t height { 0 };
};

/** Size 3D */
struct Size3D {
    /** Width */
    uint32_t width { 0 };
    /** Height */
    uint32_t height { 0 };
    /** Depth */
    uint32_t depth { 0 };
};

/** Offset 3D */
struct Offset3D {
    /** X offset */
    int32_t x { 0 };
    /** Y offset */
    int32_t y { 0 };
    /** Z offset */
    int32_t z { 0 };
};

/** Surface transform flag bits */
enum SurfaceTransformFlagBits {
    /** Identity bit */
    CORE_SURFACE_TRANSFORM_IDENTITY_BIT = 0x00000001,
    /** Rotate 90 bit */
    CORE_SURFACE_TRANSFORM_ROTATE_90_BIT = 0x00000002,
    /** Rotate 180 bit */
    CORE_SURFACE_TRANSFORM_ROTATE_180_BIT = 0x00000004,
    /** Rotate 270 bit */
    CORE_SURFACE_TRANSFORM_ROTATE_270_BIT = 0x00000008,
};
/** Container for surface transform flag bits */
using SurfaceTransformFlags = uint32_t;

/** Index type */
enum IndexType {
    /** UINT16 */
    CORE_INDEX_TYPE_UINT16 = 0,
    /** UINT32 */
    CORE_INDEX_TYPE_UINT32 = 1,
};

/** Memory property flag bits */
enum MemoryPropertyFlagBits {
    /** Device local bit */
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001,
    /** Host visible bit */
    CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002,
    /** Host visible bit */
    CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004,
    /** Host cached bit, always preferred not required for allocation */
    CORE_MEMORY_PROPERTY_HOST_CACHED_BIT = 0x00000008,
    /** Lazily allocated bit, always preferred not required for allocation */
    CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT = 0x00000010,
    /** Protected bit, always preferred not required for allocation */
    CORE_MEMORY_PROPERTY_PROTECTED_BIT = 0x00000020,
};
/** Container for memory property flag bits */
using MemoryPropertyFlags = uint32_t;

enum FormatFeatureFlagBits {
    /** Sampled image bit */
    CORE_FORMAT_FEATURE_SAMPLED_IMAGE_BIT = 0x00000001,
    /** Storage image bit */
    CORE_FORMAT_FEATURE_STORAGE_IMAGE_BIT = 0x00000002,
    /** Image atomic bit */
    CORE_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT = 0x00000004,
    /** Uniform texel buffer bit */
    CORE_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000008,
    /** Storage texel buffer bit */
    CORE_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT = 0x00000010,
    /** Storage texel buffer atomic bit */
    CORE_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT = 0x00000020,
    /** Vertex buffer bit */
    CORE_FORMAT_FEATURE_VERTEX_BUFFER_BIT = 0x00000040,
    /** Color attachment bit */
    CORE_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT = 0x00000080,
    /** Color attachment blend bit */
    CORE_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT = 0x00000100,
    /** Depth stencil attachment bit */
    CORE_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000200,
    /** Blit src bit */
    CORE_FORMAT_FEATURE_BLIT_SRC_BIT = 0x00000400,
    /** Blit dst bit */
    CORE_FORMAT_FEATURE_BLIT_DST_BIT = 0x00000800,
    /** Sampled image filter linear bit */
    CORE_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT = 0x00001000,
    /** Transfer src bit */
    CORE_FORMAT_FEATURE_TRANSFER_SRC_BIT = 0x00004000,
    /** Transfer dst bit */
    CORE_FORMAT_FEATURE_TRANSFER_DST_BIT = 0x00008000,
    /** Fragment shading rate attachment bit */
    CORE_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT = 0x40000000,
};
/** Format feature flags */
using FormatFeatureFlags = uint32_t;

/** Format properties */
struct FormatProperties {
    /** Linear tiling feature flags */
    FormatFeatureFlags linearTilingFeatures { 0u };
    /** Optimal tiling feature flags */
    FormatFeatureFlags optimalTilingFeatures { 0u };
    /** Buffer feature flags */
    FormatFeatureFlags bufferFeatures { 0u };
    /** Bytes per pixel */
    uint32_t bytesPerPixel { 0u };
};

/** Fragment shading rate properties */
struct FragmentShadingRateProperties {
    /** Min fragment shading rate attachment texel size */
    Size2D minFragmentShadingRateAttachmentTexelSize;
    /** Max fragment shading rate attachment texel size */
    Size2D maxFragmentShadingRateAttachmentTexelSize;
    /** Max fragment size */
    Size2D maxFragmentSize;
};

/** Image layout */
enum ImageLayout {
    /** Undefined */
    CORE_IMAGE_LAYOUT_UNDEFINED = 0,
    /** General */
    CORE_IMAGE_LAYOUT_GENERAL = 1,
    /** Color attachment optimal */
    CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    /** Depth stencil attachment optimal */
    CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
    /** Depth stencil read only optimal */
    CORE_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
    /** Shader read only optimal */
    CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
    /** Transfer source optimal */
    CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
    /** Transfer destination optimal */
    CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
    /** Preinitialized */
    CORE_IMAGE_LAYOUT_PREINITIALIZED = 8,
    /** Depth read only stencil attachment optimal */
    CORE_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
    /** Depth attachment stencil read only optimal */
    CORE_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
    /** Present source */
    CORE_IMAGE_LAYOUT_PRESENT_SRC = 1000001002,
    /** Shared present source */
    CORE_IMAGE_LAYOUT_SHARED_PRESENT = 1000111000,
    /** Shared present source KHR */
    CORE_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL = 1000164003,
    /** Max enumeration */
    CORE_IMAGE_LAYOUT_MAX_ENUM = 0x7FFFFFFF
};

/** Image aspect flag bits */
enum ImageAspectFlagBits {
    /** Color bit */
    CORE_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
    /** Depth bit */
    CORE_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
    /** Stencil bit */
    CORE_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,
    /** Metadata bit */
    CORE_IMAGE_ASPECT_METADATA_BIT = 0x00000008,
    /** Aspect plane 0 */
    CORE_IMAGE_ASPECT_PLANE_0_BIT = 0x00000010,
    /** Aspect plane 1 */
    CORE_IMAGE_ASPECT_PLANE_1_BIT = 0x00000020,
    /** Aspect plane 2 */
    CORE_IMAGE_ASPECT_PLANE_2_BIT = 0x00000040,
};
/** Container for image aspect flag bits */
using ImageAspectFlags = uint32_t;

/** Access flag bits */
enum AccessFlagBits {
    /** Indirect command read bit */
    CORE_ACCESS_INDIRECT_COMMAND_READ_BIT = 0x00000001,
    /** Index read bit */
    CORE_ACCESS_INDEX_READ_BIT = 0x00000002,
    /** Vertex attribute read bit */
    CORE_ACCESS_VERTEX_ATTRIBUTE_READ_BIT = 0x00000004,
    /** Uniform read bit */
    CORE_ACCESS_UNIFORM_READ_BIT = 0x00000008,
    /** Input attachment read bit */
    CORE_ACCESS_INPUT_ATTACHMENT_READ_BIT = 0x00000010,
    /** Shader read bit */
    CORE_ACCESS_SHADER_READ_BIT = 0x00000020,
    /** Shader write bit */
    CORE_ACCESS_SHADER_WRITE_BIT = 0x00000040,
    /** Color attachment read bit */
    CORE_ACCESS_COLOR_ATTACHMENT_READ_BIT = 0x00000080,
    /** Color attachment write bit */
    CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,
    /** Depth stencil attachment read bit */
    CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
    /** Depth stencil attachment write bit */
    CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
    /** Transfer read bit */
    CORE_ACCESS_TRANSFER_READ_BIT = 0x00000800,
    /** Transfer write bit */
    CORE_ACCESS_TRANSFER_WRITE_BIT = 0x00001000,
    /** Host read bit */
    CORE_ACCESS_HOST_READ_BIT = 0x00002000,
    /** Host write bit */
    CORE_ACCESS_HOST_WRITE_BIT = 0x00004000,
    /** Memory read bit */
    CORE_ACCESS_MEMORY_READ_BIT = 0x00008000,
    /** Memory write bit */
    CORE_ACCESS_MEMORY_WRITE_BIT = 0x00010000,
    /** Fragment shading rate attachment read bit */
    CORE_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT = 0x00800000,
};
/** Container for access flag bits */
using AccessFlags = uint32_t;

/** Pipeline stage flag bits */
enum PipelineStageFlagBits {
    /** Top of pipe bit */
    CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001,
    /** Draw indirect bit */
    CORE_PIPELINE_STAGE_DRAW_INDIRECT_BIT = 0x00000002,
    /** Vertex input bit */
    CORE_PIPELINE_STAGE_VERTEX_INPUT_BIT = 0x00000004,
    /** Vertex shader bit */
    CORE_PIPELINE_STAGE_VERTEX_SHADER_BIT = 0x00000008,
    /** Fragment shader bit */
    CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT = 0x00000080,
    /** Early fragment tests bit */
    CORE_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
    /** Late fragment tests bit */
    CORE_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = 0x00000200,
    /** Color attachment output bit */
    CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
    /** Compute shader bit */
    CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800,
    /** Transfer bit */
    CORE_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000,
    /** Bottom of pipe bit */
    CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000,
    /** Host bit */
    CORE_PIPELINE_STAGE_HOST_BIT = 0x00004000,
    /** All graphics bit */
    CORE_PIPELINE_STAGE_ALL_GRAPHICS_BIT = 0x00008000,
    /** All commands bit */
    CORE_PIPELINE_STAGE_ALL_COMMANDS_BIT = 0x00010000,
    /** Fragment shading rate attacchment bit */
    CORE_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT = 0x00400000,
};
/** Container for pipeline stage flag bits */
using PipelineStageFlags = uint32_t;

/** GPU queue */
struct GpuQueue {
    /** Queue type */
    enum class QueueType {
        /** Undefined queue type */
        UNDEFINED = 0x0,
        /** Graphics */
        GRAPHICS = 0x1,
        /** Compute */
        COMPUTE = 0x2,
        /** Transfer */
        TRANSFER = 0x4,
    };

    /** Type of queue */
    QueueType type { QueueType::UNDEFINED };
    /** Index of queue */
    uint32_t index { 0 };
};

/** Descriptor type */
enum DescriptorType {
    /** Sampler */
    CORE_DESCRIPTOR_TYPE_SAMPLER = 0,
    /** Combined image sampler */
    CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    /** Sampled image */
    CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    /** Storage image */
    CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    /** Uniform texel buffer */
    CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    /** Storage texel buffer */
    CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    /** Uniform buffer */
    CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    /** Storage buffer */
    CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    /** Dynamic uniform buffer */
    CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    /** Dynamic storage buffer */
    CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    /** Input attachment */
    CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
    /** Acceleration structure */
    CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE = 1000150000,
    /** Max enumeration */
    CORE_DESCRIPTOR_TYPE_MAX_ENUM = 0x7FFFFFFF
};

/** Additional descriptor flag bits */
enum AdditionalDescriptorFlagBits {
    /** Immutable sampler is used */
    CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT = 0x00000001,
};
/** Container for sampler descriptor flag bits */
using AdditionalDescriptorFlags = uint32_t;

/** Shader stage flag bits */
enum ShaderStageFlagBits {
    /** Vertex bit */
    CORE_SHADER_STAGE_VERTEX_BIT = 0x00000001,
    /** Fragment bit */
    CORE_SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
    /** Compute bit */
    CORE_SHADER_STAGE_COMPUTE_BIT = 0x00000020,
    /** All graphics */
    CORE_SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
    /** All */
    CORE_SHADER_STAGE_ALL = 0x7FFFFFFF,
};
/** Shader stage flags */
using ShaderStageFlags = uint32_t;

/** Pipeline bind point */
enum PipelineBindPoint {
    /** Bind point graphics */
    CORE_PIPELINE_BIND_POINT_GRAPHICS = 0,
    /** Bind point compute */
    CORE_PIPELINE_BIND_POINT_COMPUTE = 1,
    /** Max enumeration */
    CORE_PIPELINE_BIND_POINT_MAX_ENUM = 0x7FFFFFFF
};

/** Query pipeline statistic flag bits */
enum QueryPipelineStatisticFlagBits {
    /** Input assembly vertices bit */
    CORE_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT = 0x00000001,
    /** Input assembly primitives bit */
    CORE_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT = 0x00000002,
    /** Vertex shader invocations bit */
    CORE_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT = 0x00000004,
    /* Geometry shader invocations bit */
    CORE_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT = 0x00000008,
    /* Geometry shader primitives bit */
    CORE_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT = 0x00000010,
    /** Clipping invocations bit */
    CORE_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT = 0x00000020,
    /** Clipping primitives bit */
    CORE_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT = 0x00000040,
    /** Fragment shader invocations bit */
    CORE_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT = 0x00000080,
    /* Tesselation control shader patches bit */
    CORE_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT = 0x00000100,
    /* Tesselation evaluation shader invocations bit */
    CORE_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT = 0x00000200,
    /** Compute shader invocations bit */
    CORE_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT = 0x00000400,
};
/** Query pipeline statistic flags */
using QueryPipelineStatisticFlags = uint32_t;

/** Query type */
enum QueryType {
    /** Timestamp */
    CORE_QUERY_TYPE_TIMESTAMP = 2,
    /** Max enumeration */
    CORE_QUERY_TYPE_MAX_ENUM = 0x7FFFFFFF
};

/** Vertex input rate */
enum VertexInputRate {
    /** Vertex */
    CORE_VERTEX_INPUT_RATE_VERTEX = 0,
    /** Instance */
    CORE_VERTEX_INPUT_RATE_INSTANCE = 1,
};

/** Polygon mode */
enum PolygonMode {
    /** Fill */
    CORE_POLYGON_MODE_FILL = 0,
    /** Line */
    CORE_POLYGON_MODE_LINE = 1,
    /** Point */
    CORE_POLYGON_MODE_POINT = 2,
};

/** Cull mode flag bits */
enum CullModeFlagBits {
    /** None */
    CORE_CULL_MODE_NONE = 0,
    /** Front bit */
    CORE_CULL_MODE_FRONT_BIT = 0x00000001,
    /** Back bit */
    CORE_CULL_MODE_BACK_BIT = 0x00000002,
    /** Front and back bit */
    CORE_CULL_MODE_FRONT_AND_BACK = 0x00000003,
};
/** Cull mode flags */
using CullModeFlags = uint32_t;

/** Front face */
enum FrontFace {
    /** Counter clockwise */
    CORE_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    /** Clockwise */
    CORE_FRONT_FACE_CLOCKWISE = 1,
};

/** Stencil face flag bits */
enum StencilFaceFlagBits {
    /** Face front bit */
    CORE_STENCIL_FACE_FRONT_BIT = 0x00000001,
    /** Face back bit */
    CORE_STENCIL_FACE_BACK_BIT = 0x00000002,
    /** Front and back */
    CORE_STENCIL_FRONT_AND_BACK = 0x00000003,
};
/** Lume stencil face flags */
using StencilFaceFlags = uint32_t;

/** Compare op */
enum CompareOp {
    /** Never */
    CORE_COMPARE_OP_NEVER = 0,
    /** Less */
    CORE_COMPARE_OP_LESS = 1,
    /** Equal */
    CORE_COMPARE_OP_EQUAL = 2,
    /** Less or equal */
    CORE_COMPARE_OP_LESS_OR_EQUAL = 3,
    /** Greater */
    CORE_COMPARE_OP_GREATER = 4,
    /** Not equal */
    CORE_COMPARE_OP_NOT_EQUAL = 5,
    /** Greater or equal */
    CORE_COMPARE_OP_GREATER_OR_EQUAL = 6,
    /** Always */
    CORE_COMPARE_OP_ALWAYS = 7,
};

/** Stencil op */
enum StencilOp {
    /** Keep */
    CORE_STENCIL_OP_KEEP = 0,
    /** Zero */
    CORE_STENCIL_OP_ZERO = 1,
    /** Replace */
    CORE_STENCIL_OP_REPLACE = 2,
    /** Increment and clamp */
    CORE_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
    /** Decrement and clamp */
    CORE_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
    /** Invert */
    CORE_STENCIL_OP_INVERT = 5,
    /** Increment and wrap */
    CORE_STENCIL_OP_INCREMENT_AND_WRAP = 6,
    /** Decrement and wrap */
    CORE_STENCIL_OP_DECREMENT_AND_WRAP = 7,
};

/** Primitive topology */
enum PrimitiveTopology {
    /** Point list */
    CORE_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    /** Line list */
    CORE_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    /** Line strip */
    CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    /** Triangle list */
    CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    /** Triangle strip */
    CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    /** Triangle fan */
    CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,
    /** Line list with adjacency */
    CORE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY = 6,
    /** Line strip with adjacency */
    CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY = 7,
    /** Triangle list with adjacency */
    CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY = 8,
    /** Triangle strip with adjacency */
    CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    /** Patch list */
    CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST = 10,
};

/** Blend factor */
enum BlendFactor {
    /** Zero */
    CORE_BLEND_FACTOR_ZERO = 0,
    /** One */
    CORE_BLEND_FACTOR_ONE = 1,
    /** Source color */
    CORE_BLEND_FACTOR_SRC_COLOR = 2,
    /** One minus source color */
    CORE_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
    /** Destination color */
    CORE_BLEND_FACTOR_DST_COLOR = 4,
    /** One minus destination color */
    CORE_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
    /** Source alpha */
    CORE_BLEND_FACTOR_SRC_ALPHA = 6,
    /** One minus source alpha */
    CORE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
    /** Destination alpha */
    CORE_BLEND_FACTOR_DST_ALPHA = 8,
    /** One minus destination alpha */
    CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
    /** Constant color */
    CORE_BLEND_FACTOR_CONSTANT_COLOR = 10,
    /** One minus constant color */
    CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
    /** Constant alpha */
    CORE_BLEND_FACTOR_CONSTANT_ALPHA = 12,
    /** One minus constant alpha */
    CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
    /** Source alpha saturate */
    CORE_BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
    /** Source one color */
    CORE_BLEND_FACTOR_SRC1_COLOR = 15,
    /** One minus source one color */
    CORE_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR = 16,
    /** Source one alpha */
    CORE_BLEND_FACTOR_SRC1_ALPHA = 17,
    /** One minus source one alpha */
    CORE_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA = 18,
};

/** Blend op */
enum BlendOp {
    /** Add */
    CORE_BLEND_OP_ADD = 0,
    /** Subtract */
    CORE_BLEND_OP_SUBTRACT = 1,
    /** Reverse subtract */
    CORE_BLEND_OP_REVERSE_SUBTRACT = 2,
    /** Min */
    CORE_BLEND_OP_MIN = 3,
    /** Max */
    CORE_BLEND_OP_MAX = 4,
};

/** Color component flag bits */
enum ColorComponentFlagBits {
    /** Red bit */
    CORE_COLOR_COMPONENT_R_BIT = 0x00000001,
    /** Green bit */
    CORE_COLOR_COMPONENT_G_BIT = 0x00000002,
    /** Blue bit */
    CORE_COLOR_COMPONENT_B_BIT = 0x00000004,
    /** Alpha bit */
    CORE_COLOR_COMPONENT_A_BIT = 0x00000008,
};
/** Color component flags */
using ColorComponentFlags = uint32_t;

/** Logic op */
enum LogicOp {
    /** Clear */
    CORE_LOGIC_OP_CLEAR = 0,
    /** And */
    CORE_LOGIC_OP_AND = 1,
    /** And reverse */
    CORE_LOGIC_OP_AND_REVERSE = 2,
    /** Copy */
    CORE_LOGIC_OP_COPY = 3,
    /** And inverted */
    CORE_LOGIC_OP_AND_INVERTED = 4,
    /** No op */
    CORE_LOGIC_OP_NO_OP = 5,
    /** Xor */
    CORE_LOGIC_OP_XOR = 6,
    /** Or */
    CORE_LOGIC_OP_OR = 7,
    /** Nor */
    CORE_LOGIC_OP_NOR = 8,
    /** Equivalent */
    CORE_LOGIC_OP_EQUIVALENT = 9,
    /** Invert */
    CORE_LOGIC_OP_INVERT = 10,
    /** Or reverse */
    CORE_LOGIC_OP_OR_REVERSE = 11,
    /** Copy inverted */
    CORE_LOGIC_OP_COPY_INVERTED = 12,
    /** Or inverted */
    CORE_LOGIC_OP_OR_INVERTED = 13,
    /** Nand */
    CORE_LOGIC_OP_NAND = 14,
    /** Set */
    CORE_LOGIC_OP_SET = 15,
};

/** Attachment load op */
enum AttachmentLoadOp {
    /** Load */
    CORE_ATTACHMENT_LOAD_OP_LOAD = 0,
    /** Clear */
    CORE_ATTACHMENT_LOAD_OP_CLEAR = 1,
    /** Dont care */
    CORE_ATTACHMENT_LOAD_OP_DONT_CARE = 2,
};

/** Attachment store op */
enum AttachmentStoreOp {
    /** Store */
    CORE_ATTACHMENT_STORE_OP_STORE = 0,
    /** Dont care */
    CORE_ATTACHMENT_STORE_OP_DONT_CARE = 1,
};

/** Clear color value */
union ClearColorValue {
    ClearColorValue() = default;
    ~ClearColorValue() = default;
    constexpr ClearColorValue(float r, float g, float b, float a) : float32 { r, g, b, a } {};
    constexpr ClearColorValue(int32_t r, int32_t g, int32_t b, int32_t a) : int32 { r, g, b, a } {};
    constexpr ClearColorValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a) : uint32 { r, g, b, a } {};
    /** Float32 array of 4 */
    float float32[4];
    /** int32 array of 4 */
    int32_t int32[4];
    /** uint32 array of 4 */
    uint32_t uint32[4];
};

/** Clear depth stencil value */
struct ClearDepthStencilValue {
    /** Depth, default value 1.0f */
    float depth;
    /** Stencil */
    uint32_t stencil;
};

/** Clear value */
union ClearValue {
    ClearValue() = default;
    ~ClearValue() = default;
    constexpr explicit ClearValue(const ClearColorValue& color) : color { color } {};
    constexpr explicit ClearValue(const ClearDepthStencilValue& depthStencil) : depthStencil { depthStencil } {};
    constexpr ClearValue(float r, float g, float b, float a) : color { r, g, b, a } {};
    constexpr ClearValue(int32_t r, int32_t g, int32_t b, int32_t a) : color { r, g, b, a } {};
    constexpr ClearValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a) : color { r, g, b, a } {};
    constexpr ClearValue(float depth, uint32_t stencil) : depthStencil { depth, stencil } {};

    /** Color */
    ClearColorValue color;
    /** Depth stencil */
    ClearDepthStencilValue depthStencil;
};

/** Dynamic state flag bits
 * Dynamic state flags only map to basic dynamic states.
 * Use DynamicStateEnum s when creating the actual pso objects.
 */
enum DynamicStateFlagBits {
    /** Undefined */
    CORE_DYNAMIC_STATE_UNDEFINED = 0,
    /** Viewport */
    CORE_DYNAMIC_STATE_VIEWPORT = (1 << 0),
    /** Scissor */
    CORE_DYNAMIC_STATE_SCISSOR = (1 << 1),
    /** Line width */
    CORE_DYNAMIC_STATE_LINE_WIDTH = (1 << 2),
    /** Depth bias */
    CORE_DYNAMIC_STATE_DEPTH_BIAS = (1 << 3),
    /** Blend constants */
    CORE_DYNAMIC_STATE_BLEND_CONSTANTS = (1 << 4),
    /** Depth bounds */
    CORE_DYNAMIC_STATE_DEPTH_BOUNDS = (1 << 5),
    /** Stencil compare mask */
    CORE_DYNAMIC_STATE_STENCIL_COMPARE_MASK = (1 << 6),
    /** Stencil write mask */
    CORE_DYNAMIC_STATE_STENCIL_WRITE_MASK = (1 << 7),
    /** Stencil reference */
    CORE_DYNAMIC_STATE_STENCIL_REFERENCE = (1 << 8),
};
/** Dynamic state flags */
using DynamicStateFlags = uint32_t;

/** Dynamic states */
enum DynamicStateEnum {
    /** Viewport */
    CORE_DYNAMIC_STATE_ENUM_VIEWPORT = 0,
    /** Scissor */
    CORE_DYNAMIC_STATE_ENUM_SCISSOR = 1,
    /** Line width */
    CORE_DYNAMIC_STATE_ENUM_LINE_WIDTH = 2,
    /** Depth bias */
    CORE_DYNAMIC_STATE_ENUM_DEPTH_BIAS = 3,
    /** Blend constants */
    CORE_DYNAMIC_STATE_ENUM_BLEND_CONSTANTS = 4,
    /** Depth bounds */
    CORE_DYNAMIC_STATE_ENUM_DEPTH_BOUNDS = 5,
    /** Stencil compare mask */
    CORE_DYNAMIC_STATE_ENUM_STENCIL_COMPARE_MASK = 6,
    /** Stencil write mask */
    CORE_DYNAMIC_STATE_ENUM_STENCIL_WRITE_MASK = 7,
    /** Stencil reference */
    CORE_DYNAMIC_STATE_ENUM_STENCIL_REFERENCE = 8,
    /** Fragment shading rate */
    CORE_DYNAMIC_STATE_ENUM_FRAGMENT_SHADING_RATE = 1000226000,
    /** Max enumeration */
    CORE_DYNAMIC_STATE_ENUM_MAX_ENUM = 0x7FFFFFFF
};

/** Resolve mode flag bits */
enum ResolveModeFlagBits {
    /** None. No resolve mode done */
    CORE_RESOLVE_MODE_NONE = 0,
    /** Resolve value is same as sample zero */
    CORE_RESOLVE_MODE_SAMPLE_ZERO_BIT = 0x00000001,
    /** Resolve value is same as average of all samples */
    CORE_RESOLVE_MODE_AVERAGE_BIT = 0x00000002,
    /** Resolve value is same as min of all samples */
    CORE_RESOLVE_MODE_MIN_BIT = 0x00000004,
    /** Resolve value is same as max of all samples */
    CORE_RESOLVE_MODE_MAX_BIT = 0x00000008,
};
/** Resolve mode flags */
using ResolveModeFlags = uint32_t;

/** SubpassContents Specifies how commands in the first subpass are provided
 * CORE_SUBPASS_CONTENTS_INLINE == subpasses are recorded to the same render command list
 * CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS == subpasses are recorded to secondary command lists
 */
enum SubpassContents {
    /** Inline */
    CORE_SUBPASS_CONTENTS_INLINE = 0,
    /* Secondary command lists */
    CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS = 1,
};

/** Fragment shading rate combiner op */
enum FragmentShadingRateCombinerOp {
    /** Keep */
    CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP = 0,
    /** Replace */
    CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE = 1,
    /** Min */
    CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN = 2,
    /** Max */
    CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX = 3,
    /** Mul */
    CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL = 4,
};

/** Fragment shading rate combiner operations for commands */
struct FragmentShadingRateCombinerOps {
    /** First combiner (combine pipeline and primitive shading rates) */
    FragmentShadingRateCombinerOp op1 { CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP };
    /** Second combiner (combine the first with attachment fragment shading rate) */
    FragmentShadingRateCombinerOp op2 { CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP };
};

/** Pipeline state constants */
struct PipelineStateConstants {
    /** GPU buffer whole size */
    static constexpr uint32_t GPU_BUFFER_WHOLE_SIZE { ~0u };
    /** GPU image all mip levels */
    static constexpr uint32_t GPU_IMAGE_ALL_MIP_LEVELS { ~0u };
    /** GPU image all layers */
    static constexpr uint32_t GPU_IMAGE_ALL_LAYERS { ~0u };
    /** Max vertex buffer count */
    static constexpr uint32_t MAX_VERTEX_BUFFER_COUNT { 8u };

    /** Max input attachment count */
    static constexpr uint32_t MAX_INPUT_ATTACHMENT_COUNT { 8u };
    /** Max color attachment count */
    static constexpr uint32_t MAX_COLOR_ATTACHMENT_COUNT { 8u };
    /** Max resolve attachment count */
    static constexpr uint32_t MAX_RESOLVE_ATTACHMENT_COUNT { 4u };
    /** Max render pass attachment count */
    static constexpr uint32_t MAX_RENDER_PASS_ATTACHMENT_COUNT { 8u };
    /** Max render node gpu wait signals */
    static constexpr uint32_t MAX_RENDER_NODE_GPU_WAIT_SIGNALS { 4u };
};

/** Viewport descriptor
 * x and y are upper left corner (x,y) */
struct ViewportDesc {
    /** X point */
    float x { 0.0f };
    /** Y point */
    float y { 0.0f };
    /** Width */
    float width { 0.0f };
    /** Height */
    float height { 0.0f };

    /** Min depth (0.0f - 1.0f) */
    float minDepth { 0.0f };
    /** Max depth (0.0f - 1.0f) */
    float maxDepth { 1.0f };
};

/** Scissor descriptor */
struct ScissorDesc {
    /** X offset */
    int32_t offsetX { 0 };
    /** Y offset */
    int32_t offsetY { 0 };
    /** Extent width */
    uint32_t extentWidth { 0 };
    /** Extent height */
    uint32_t extentHeight { 0 };
};

/** Image subresource layers */
struct ImageSubresourceLayers {
    /** Image aspect flags */
    ImageAspectFlags imageAspectFlags { 0 };
    /** Mip level */
    uint32_t mipLevel { 0 };
    /** Base array layer */
    uint32_t baseArrayLayer { 0 };
    /** Layer count */
    uint32_t layerCount { PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
};

/** Image subresource range */
struct ImageSubresourceRange {
    /** Image aspect flags */
    ImageAspectFlags imageAspectFlags { 0 };
    /** Base mip level */
    uint32_t baseMipLevel { 0 };
    /** Level count */
    uint32_t levelCount { PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS };
    /** Base array layer */
    uint32_t baseArrayLayer { 0 };
    /** Layer count */
    uint32_t layerCount { PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
};

/** Image blit */
struct ImageBlit {
    /** Source subresource */
    ImageSubresourceLayers srcSubresource;
    /** Source offsets (top left and bottom right coordinates) */
    Size3D srcOffsets[2u];
    /** Destination subresource */
    ImageSubresourceLayers dstSubresource;
    /** Destination offsets (top left and bottom right coordinates) */
    Size3D dstOffsets[2u];
};

/** Memory barrier with pipeline stages */
struct GeneralBarrier {
    /** Access flags */
    AccessFlags accessFlags { 0 };
    /** Pipeline stage flags */
    PipelineStageFlags pipelineStageFlags { 0 };
};

/** Buffer resource barrier */
struct BufferResourceBarrier {
    /** Access flags */
    AccessFlags accessFlags { 0 };
    /** Pipeline stage flags */
    PipelineStageFlags pipelineStageFlags { 0 };
};

/** Image resource barrier */
struct ImageResourceBarrier {
    /** Access flags */
    AccessFlags accessFlags { 0 };
    /** Pipeline stage flags */
    PipelineStageFlags pipelineStageFlags { 0 };
    /** Image layout */
    ImageLayout imageLayout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
};

/** Buffer copy */
struct BufferCopy {
    /** Offset to source buffer */
    uint32_t srcOffset { 0 };
    /** Offset to destination buffer */
    uint32_t dstOffset { 0 };
    /** Size of copy */
    uint32_t size { 0 };
};

/** Buffer image copy */
struct BufferImageCopy {
    /** Buffer offset */
    uint32_t bufferOffset { 0 };
    /** Buffer row length */
    uint32_t bufferRowLength { 0 };
    /** Buffer image height */
    uint32_t bufferImageHeight { 0 };
    /** Image subresource */
    ImageSubresourceLayers imageSubresource;
    /** Image offset */
    Size3D imageOffset;
    /** Image extent */
    Size3D imageExtent;
};

/** Image copy */
struct ImageCopy {
    /** Src image subresource */
    ImageSubresourceLayers srcSubresource;
    /** Src offset */
    Offset3D srcOffset;
    /** Dst image subresource */
    ImageSubresourceLayers dstSubresource;
    /** Dst offset */
    Offset3D dstOffset;
    /** Texel size of copy */
    Size3D extent;
};

/** GPU resource state in pipeline */
struct GpuResourceState {
    /** Shader stage flags */
    ShaderStageFlags shaderStageFlags { 0 };

    /** Access flags */
    AccessFlags accessFlags { 0 };
    /** Pipeline stage flags */
    PipelineStageFlags pipelineStageFlags { 0 };

    /** GPU queue */
    GpuQueue gpuQueue {};
};

/** Render pass descriptor */
struct RenderPassDesc {
    /** Render area */
    struct RenderArea {
        /** X offset */
        int32_t offsetX { 0 };
        /** Y offset */
        int32_t offsetY { 0 };
        /** Extent width */
        uint32_t extentWidth { 0u };
        /** Extent height */
        uint32_t extentHeight { 0u };
    };

    /** Attachment descriptor */
    struct AttachmentDesc {
        /** Layer for layered image */
        uint32_t layer { 0u };
        /** Mip level to target on mipped image */
        uint32_t mipLevel { 0u };

        // following values are not needed in render pass compatibility
        /** Load operation */
        AttachmentLoadOp loadOp { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE };
        /** Store operation */
        AttachmentStoreOp storeOp { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE };

        /** Stencil load operation */
        AttachmentLoadOp stencilLoadOp { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE };
        /** Stencil store operation */
        AttachmentStoreOp stencilStoreOp { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE };

        /** Clear value (union ClearColorValue or ClearDepthStencilValue) */
        ClearValue clearValue {};
    };

    /** Attachment count */
    uint32_t attachmentCount { 0u };

    /** Attachment handles */
    RenderHandle attachmentHandles[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    /** Attachments */
    AttachmentDesc attachments[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];

    /** Render area */
    RenderArea renderArea;

    /** Subpass count */
    uint32_t subpassCount { 0u };
    /** Subpass contents */
    SubpassContents subpassContents { SubpassContents::CORE_SUBPASS_CONTENTS_INLINE };
};

/** Render pass descriptor with render handle references */
struct RenderPassDescWithHandleReference {
    /** Attachment count */
    uint32_t attachmentCount { 0u };

    /** Attachment handles */
    RenderHandleReference attachmentHandles[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    /** Attachments */
    RenderPassDesc::AttachmentDesc attachments[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];

    /** Render area */
    RenderPassDesc::RenderArea renderArea;

    /** Subpass count */
    uint32_t subpassCount { 0u };
    /** Subpass contents */
    SubpassContents subpassContents { SubpassContents::CORE_SUBPASS_CONTENTS_INLINE };
};

/** Render pass subpass descriptor */
struct RenderPassSubpassDesc {
    // indices to render pass attachments
    /** Depth attachment index */
    uint32_t depthAttachmentIndex { ~0u };
    /** Depth resolve attachment index */
    uint32_t depthResolveAttachmentIndex { ~0u };
    /** Input attachment indices */
    uint32_t inputAttachmentIndices[PipelineStateConstants::MAX_INPUT_ATTACHMENT_COUNT] {};
    /** Color attachment indices */
    uint32_t colorAttachmentIndices[PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT] {};
    /** Resolve attachment indices */
    uint32_t resolveAttachmentIndices[PipelineStateConstants::MAX_RESOLVE_ATTACHMENT_COUNT] {};
    /** Fragment shading rate attachment index */
    uint32_t fragmentShadingRateAttachmentIndex { ~0u };

    // attachment counts in this subpass
    /** Depth attachment count in this subpass */
    uint32_t depthAttachmentCount { 0u };
    /** Depth resolve attachment count in this subpass */
    uint32_t depthResolveAttachmentCount { 0u };
    /** Input attachment count in this subpass */
    uint32_t inputAttachmentCount { 0u };
    /** Color attachment count in this subpass */
    uint32_t colorAttachmentCount { 0u };
    /** Resolve attachment count in this subpass */
    uint32_t resolveAttachmentCount { 0u };
    /** Fragmend shading rate attachment count in this subpass */
    uint32_t fragmentShadingRateAttachmentCount { 0u };

    /** Depth resolve mode flag bit */
    ResolveModeFlagBits depthResolveModeFlagBit { ResolveModeFlagBits::CORE_RESOLVE_MODE_NONE };
    /** Stencil resolve mode flag bit */
    ResolveModeFlagBits stencilResolveModeFlagBit { ResolveModeFlagBits::CORE_RESOLVE_MODE_NONE };

    /** Shading rate texel size for subpass (will be clamped to device limits automatically if not set accordingly) */
    Size2D shadingRateTexelSize { 1u, 1u };

    /** Multi-view bitfield of view indices. Multi-view is ignored while zero. */
    uint32_t viewMask { 0u };
};

/** Render pass */
struct RenderPass {
    /** Render pass descriptor */
    RenderPassDesc renderPassDesc;
    /** Subpass start index */
    uint32_t subpassStartIndex { 0u };
    /** Subpass descriptor */
    RenderPassSubpassDesc subpassDesc;
};

/** Render pass with render handle references */
struct RenderPassWithHandleReference {
    /** Render pass descriptor */
    RenderPassDescWithHandleReference renderPassDesc;
    /** Subpass start index */
    uint32_t subpassStartIndex { 0u };
    /** Subpass descriptor */
    RenderPassSubpassDesc subpassDesc;
};

/** Forced graphics state flag bits */
enum GraphicsStateFlagBits {
    /** Input assembly bit */
    CORE_GRAPHICS_STATE_INPUT_ASSEMBLY_BIT = 0x00000001,
    /** Rastarization state bit */
    CORE_GRAPHICS_STATE_RASTERIZATION_STATE_BIT = 0x00000002,
    /** Depth stencil state bit */
    CORE_GRAPHICS_STATE_DEPTH_STENCIL_STATE_BIT = 0x00000004,
    /** Color blend state bit */
    CORE_GRAPHICS_STATE_COLOR_BLEND_STATE_BIT = 0x00000008,
};
/** Container for graphics state flag bits */
using GraphicsStateFlags = uint32_t;

/** Graphics state */
struct GraphicsState {
    /** Stencil operation state */
    struct StencilOpState {
        /** Fail operation */
        StencilOp failOp { StencilOp::CORE_STENCIL_OP_KEEP };
        /** Pass operation */
        StencilOp passOp { StencilOp::CORE_STENCIL_OP_KEEP };
        /** Depth fail operation */
        StencilOp depthFailOp { StencilOp::CORE_STENCIL_OP_KEEP };
        /** Compare operation */
        CompareOp compareOp { CompareOp::CORE_COMPARE_OP_NEVER };
        /** Compare mask */
        uint32_t compareMask { 0 };
        /** Write mask */
        uint32_t writeMask { 0 };
        /** Reference */
        uint32_t reference { 0 };
    };

    /** Rasterization state */
    struct RasterizationState {
        /** Enable depth clamp */
        bool enableDepthClamp { false };
        /** Enable depth bias */
        bool enableDepthBias { false };
        /** Enable rasterizer discard */
        bool enableRasterizerDiscard { false };

        /** Polygon mode */
        PolygonMode polygonMode { PolygonMode::CORE_POLYGON_MODE_FILL };
        /** Cull mode flags */
        CullModeFlags cullModeFlags { CullModeFlagBits::CORE_CULL_MODE_NONE };
        /** Front face */
        FrontFace frontFace { FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE };

        /** Depth bias constant factor */
        float depthBiasConstantFactor { 0.0f };
        /** Depth bias clamp */
        float depthBiasClamp { 0.0f };
        /** Depth bias slope factor */
        float depthBiasSlopeFactor { 0.0f };

        /** Line width */
        float lineWidth { 1.0f };
    };

    /** Depth stencil state */
    struct DepthStencilState {
        /** Enable depth test */
        bool enableDepthTest { false };
        /** Enable depth write */
        bool enableDepthWrite { false };
        /** Enable depth bounds test */
        bool enableDepthBoundsTest { false };
        /** Enable stencil test */
        bool enableStencilTest { false };

        /** Min depth bounds */
        float minDepthBounds { 0.0f };
        /** Max depth bounds */
        float maxDepthBounds { 1.0f };

        /** Depth compare operation */
        CompareOp depthCompareOp { CompareOp::CORE_COMPARE_OP_NEVER };
        /** Front stencil operation state */
        StencilOpState frontStencilOpState {};
        /** Back stencil operation state */
        StencilOpState backStencilOpState {};
    };

    /** Color blend state */
    struct ColorBlendState {
        /** Attachment */
        struct Attachment {
            /** Enable blend */
            bool enableBlend { false };
            /** Color write mask */
            ColorComponentFlags colorWriteMask { ColorComponentFlagBits::CORE_COLOR_COMPONENT_R_BIT |
                                                 ColorComponentFlagBits::CORE_COLOR_COMPONENT_G_BIT |
                                                 ColorComponentFlagBits::CORE_COLOR_COMPONENT_B_BIT |
                                                 ColorComponentFlagBits::CORE_COLOR_COMPONENT_A_BIT };

            /** Source color blend factor */
            BlendFactor srcColorBlendFactor { BlendFactor::CORE_BLEND_FACTOR_ONE };
            /** Destination color blend factor */
            BlendFactor dstColorBlendFactor { BlendFactor::CORE_BLEND_FACTOR_ONE };
            /** Color blend operation */
            BlendOp colorBlendOp { BlendOp::CORE_BLEND_OP_ADD };

            /** Source alpha blend factor */
            BlendFactor srcAlphaBlendFactor { BlendFactor::CORE_BLEND_FACTOR_ONE };
            /** Destination alpha blend factor */
            BlendFactor dstAlphaBlendFactor { BlendFactor::CORE_BLEND_FACTOR_ONE };
            /** Alpha blend operation */
            BlendOp alphaBlendOp { BlendOp::CORE_BLEND_OP_ADD };
        };

        /** Enable logic operation */
        bool enableLogicOp { false };
        /** Logic operation */
        LogicOp logicOp { LogicOp::CORE_LOGIC_OP_NO_OP };

        /** Color blend constants (R, G, B, A) */
        float colorBlendConstants[4u] { 0.0f, 0.0f, 0.0f, 0.0f };
        /** Color attachment count */
        uint32_t colorAttachmentCount { 0 };
        /** Color attachments */
        Attachment colorAttachments[PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT];
    };

    /** Input assembly */
    struct InputAssembly {
        /** Enable primitive restart */
        bool enablePrimitiveRestart { false };
        /** Primitive topology */
        PrimitiveTopology primitiveTopology { PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
    };

    /** Input assembly */
    InputAssembly inputAssembly;
    /** Rasterization state */
    RasterizationState rasterizationState;
    /** Depth stencil state */
    DepthStencilState depthStencilState;
    /** Color blend state */
    ColorBlendState colorBlendState;
};

/** Vertex input declaration */
struct VertexInputDeclaration {
    /** Vertex input binding description */
    struct VertexInputBindingDescription {
        /** Binding */
        uint32_t binding { ~0u };
        /** Stride */
        uint32_t stride { 0u };
        /** Vertex input rate */
        VertexInputRate vertexInputRate { VertexInputRate::CORE_VERTEX_INPUT_RATE_VERTEX };
    };

    /** Vertex input attribute description */
    struct VertexInputAttributeDescription {
        /** Location */
        uint32_t location { ~0u };
        /** Binding */
        uint32_t binding { ~0u };
        /** Format */
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
        /** Offset */
        uint32_t offset { 0u };
    };
};

/** Vertex input declaration data */
struct VertexInputDeclarationData {
    /** Binding description count */
    uint32_t bindingDescriptionCount { 0 };
    /** Attribute description count */
    uint32_t attributeDescriptionCount { 0 };

    /** Array of binding descriptions */
    VertexInputDeclaration::VertexInputBindingDescription
        bindingDescriptions[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
    /** Array of attribute descriptions */
    VertexInputDeclaration::VertexInputAttributeDescription
        attributeDescriptions[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
};

/** Vertex input declaration view */
struct VertexInputDeclarationView {
    /** Array of binding descriptions */
    BASE_NS::array_view<const VertexInputDeclaration::VertexInputBindingDescription> bindingDescriptions;
    /** Array of attribute descriptions */
    BASE_NS::array_view<const VertexInputDeclaration::VertexInputAttributeDescription> attributeDescriptions;
};

/** Shader specialization */
struct ShaderSpecialization {
    /** Constant */
    struct Constant {
        enum class Type : uint32_t {
            INVALID = 0,
            /** 32 bit boolean value */
            BOOL = 1,
            /** 32 bit unsigned integer value */
            UINT32 = 2,
            /** 32 bit signed integer value */
            INT32 = 3,
            /** 32 bit floating-point value */
            FLOAT = 4,
        };
        /** Shader stage */
        ShaderStageFlags shaderStage;
        /** ID */
        uint32_t id;
        /** Type */
        Type type;
        /** Offset */
        uint32_t offset;
    };
};

/** Shader specialization constant view */
struct ShaderSpecializationConstantView {
    /** Array of shader specialization constants */
    BASE_NS::array_view<const ShaderSpecialization::Constant> constants;
};

/** Shader specialization constant data view */
struct ShaderSpecializationConstantDataView {
    /** Array of shader specialization constants */
    BASE_NS::array_view<const ShaderSpecialization::Constant> constants;
    /** Data */
    BASE_NS::array_view<const uint32_t> data;
};

/** Buffer offset */
struct BufferOffset {
    /** Buffer handle */
    RenderHandle handle;
    /** Buffer offset in bytes */
    uint32_t offset { 0 };
};

/** Accleration structure types */
enum AccelerationStructureType : uint32_t {
    /** Acceleration structure type top level */
    CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL = 0,
    /** Acceleration structure type bottom level */
    CORE_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL = 1,
    /** Acceleration structure type generic */
    CORE_ACCELERATION_STRUCTURE_TYPE_GENERIC = 2,
};

struct AccelerationStructureBuildRangeInfo {
    uint32_t primitiveCount { 0u };
    uint32_t primitiveOffset { 0u };
    uint32_t firstVertex { 0u };
    uint32_t transformOffset { 0u };
};

/** Additional parameters for geometry */
enum GeometryFlagBits : uint32_t {
    /** The geometry does not invoke the any-hit shaders even if present in a hit group */
    GEOMETRY_OPAQUE_BIT = 0x00000001,
    /** Indicates that the implementation must only call the any-hit shader a single time for each primitive
     * in this geometry.
     */
    GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT = 0x00000002,
};
/** Geometry flags */
using GeometryFlags = uint32_t;

/** Acceleratoin structure geometry triangles data */
struct AccelerationStructureGeometryTrianglesInfo {
    /** Vertex format */
    BASE_NS::Format vertexFormat { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
    /** Vertex stride, bytes between each vertex */
    uint32_t vertexStride { 0u };
    /** Highest index of a vertex for building geom */
    uint32_t maxVertex { 0u };
    /** Index type */
    IndexType indexType { IndexType::CORE_INDEX_TYPE_UINT32 };
    /** Index count */
    uint32_t indexCount { 0u };

    /** Geometry flags */
    GeometryFlags geometryFlags { 0u };
};

/** Acceleratoin structure geometry triangles data */
struct AccelerationStructureGeometryTrianglesData {
    /** Triangles info */
    AccelerationStructureGeometryTrianglesInfo info;

    /** Vertex buffer with offset */
    BufferOffset vertexData;
    /** Index buffer with offset */
    BufferOffset indexData;
    /** Transform buffer (4x3 matrices), additional */
    BufferOffset transformData;
};

/** Acceleration structure geometry AABBs info */
struct AccelerationStructureGeometryAabbsInfo {
    /** Stride, bytes between each AABB (must be a multiple of 8) */
    uint32_t stride { 0u };
};

/** Acceleration structure geometry AABBs data */
struct AccelerationStructureGeometryAabbsData {
    /** AABBs info */
    AccelerationStructureGeometryAabbsInfo info;
    /** Buffer resource and offset for AabbPositions */
    BufferOffset data;
};

/** Acceleration structure geometry instances info */
struct AccelerationStructureGeometryInstancesInfo {
    /** Specifies whether data is used as an array of addresses or just an array */
    bool arrayOfPointers { false };
};

/** Acceleration structure geometry instances data */
struct AccelerationStructureGeometryInstancesData {
    /** Instances info */
    AccelerationStructureGeometryInstancesInfo info;
    /** Buffer resource and offset for structures */
    BufferOffset data;
};

/** Build acceleration structure flag bits */
enum BuildAccelerationStructureFlagBits : uint32_t {
    /** Can be updated */
    CORE_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT = 0x00000001,
    /** Alloc compaction bit. Can be used as a source for compaction. */
    CORE_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT = 0x00000002,
    /** Prefer fast trace. Prioritizes trace performance for build time. */
    CORE_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT = 0x00000004,
    /** Prefer fast build. Prioritizes fast build for trace performance. */
    CORE_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT = 0x00000008,
    /** Prefer low memory. Prioritizes the size of the scratch memory and the final acceleration structure,
     * potentially at the expense of build time or trace performance */
    CORE_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT = 0x00000010,
};
/** Build acceleration structure flags */
using BuildAccelerationStructureFlags = uint32_t;

/** Build acceleration structure mode */
enum BuildAccelerationStructureMode : uint32_t {
    /** The destination acceleration structure will be built using the specified geometries */
    CORE_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD = 0,
    /** The destination acceleration structure will be built using data in a source acceleration structure,
     * updated by the specified geometries. */
    CORE_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE = 1,
};

/** Acceleration structure build geometry info */
struct AccelerationStructureBuildGeometryInfo {
    /** Acceleration structure type */
    AccelerationStructureType type { AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL };
    /** Additional flags */
    BuildAccelerationStructureFlags flags { 0 };
    /** Build mode */
    BuildAccelerationStructureMode mode {
        BuildAccelerationStructureMode::CORE_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD
    };
};

/** Acceleration structure build geometry data */
struct AccelerationStructureBuildGeometryData {
    /** Geometry info */
    AccelerationStructureBuildGeometryInfo info;

    /** Handle to existing acceleration structure which is used a src for dst update. */
    RenderHandle srcAccelerationStructure;
    /** Handle to dst acceleration structure which is to be build. */
    RenderHandle dstAccelerationStructure;
    /** Handle and offset to build scratch data. */
    BufferOffset scratchBuffer;
};

/** Acceleration structure build sizes.
 * The build sizes can be queried from device with AccelerationStructureBuildGeometryInfo.
 */
struct AccelerationStructureBuildSizes {
    /** Acceleration structure size */
    uint32_t accelerationStructureSize { 0u };
    /** Update scratch size */
    uint32_t updateScratchSize { 0u };
    /** Build scratch size */
    uint32_t buildScratchSize { 0u };
};

/** Geometry type.
 */
enum GeometryType : uint32_t {
    /** Triangles */
    CORE_GEOMETRY_TYPE_TRIANGLES = 0,
    /** AABBs */
    CORE_GEOMETRY_TYPE_AABBS = 1,
    /** Instances */
    CORE_GEOMETRY_TYPE_INSTANCES = 2,
};

/** Geometry instance flags.
 */
enum GeometryInstanceFlagBits : uint32_t {
    /** Disable face culling */
    CORE_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT = 0x00000001,
    /** Invert the facing */
    CORE_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT = 0x00000002,
    /** Force geometry to be treated as opaque */
    CORE_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT = 0x00000004,
    /** Removes opaque bit from geometry. Can be overriden with SPIR-v OpaqueKHR flag */
    CORE_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT = 0x00000008,
};
using GeometryInstanceFlags = uint32_t;

/** Acceleration structure instance info.
 */
struct AccelerationStructureInstanceInfo {
    /** Affine transform matrix (4x3 used) */
    // BASE_NS::Math::Mat4X4 transform{};
    float transform[4][4];
    /** User specified index accessable in ray shaders with InstanceCustomIndexKHR (24 bits) */
    uint32_t instanceCustomIndex { 0u };
    /** Mask, a visibility mask for geometry (8 bits). Instance may only be hit if cull mask & instance.mask != 0. */
    uint8_t mask { 0u };
    /** GeometryInstanceFlags for this instance */
    GeometryInstanceFlags flags { 0u };
};

/** Acceleration structure instance data.
 */
struct AccelerationStructureInstanceData {
    /** Instance info */
    AccelerationStructureInstanceInfo info;

    /** Acceleration structure handle */
    RenderHandle accelerationStructureReference;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_PIPELINE_STATE_DESC_H
