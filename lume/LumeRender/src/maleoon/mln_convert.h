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

#ifndef MALEOON_MLN_CONVERT_H
#define MALEOON_MLN_CONVERT_H

#include <base/util/formats.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_state_desc.h>

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

// Format conversion: Lume BASE_NS::Format ↔ MlnFormat
// Both enums use identical integer values (Vulkan-derived), so static_cast is safe.
inline MlnFormat ToMlnFormat(BASE_NS::Format format)
{
    return static_cast<MlnFormat>(format);
}

inline BASE_NS::Format FromMlnFormat(MlnFormat format)
{
    return static_cast<BASE_NS::Format>(format);
}

// FormatFeatureFlags conversion: Lume CORE_FORMAT_FEATURE_*_BIT values match MLN_FORMAT_FEATURE_*_BIT values exactly.
inline MlnFormatFeatureFlags ToMlnFormatFeatureFlags(FormatFeatureFlags flags)
{
    return static_cast<MlnFormatFeatureFlags>(flags);
}

inline FormatFeatureFlags FromMlnFormatFeatureFlags(MlnFormatFeatureFlags flags)
{
    return static_cast<FormatFeatureFlags>(flags);
}

// Convert MlnGpuFormatProperties → Lume FormatProperties
inline FormatProperties FromMlnFormatProperties(const MlnGpuFormatProperties& mlnProps)
{
    FormatProperties props{};
    props.linearTilingFeatures = FromMlnFormatFeatureFlags(mlnProps.linearTilingFeatures);
    props.optimalTilingFeatures = FromMlnFormatFeatureFlags(mlnProps.optimalTilingFeatures);
    props.bufferFeatures = FromMlnFormatFeatureFlags(mlnProps.bufferFeatures);
    props.bytesPerPixel = 0u;  // Maleoon doesn't provide this; caller should compute if needed
    return props;
}

// All resource-related enum conversions below use static_cast because
// Lume CORE_* and Maleoon MLN_* values are both Vulkan-derived with identical integers
// for the common flags (bits 0x001..0x100). However, RT-related buffer usage bits
// differ between CORE and MLN, so they are remapped explicitly below.

// Buffer usage flags
inline MlnBufferUsageFlags ToMlnBufferUsageFlags(BufferUsageFlags flags)
{
    MlnBufferUsageFlags out = static_cast<MlnBufferUsageFlags>(flags & 0x000001FFu);
    if (flags & CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        out |= MLN_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }
    if (flags & CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT) {
        out |= MLN_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT;
    }
    if (flags & CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT) {
        out |= MLN_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT;
    }
    return out;
}

// Image usage flags
inline MlnImageUsageFlags ToMlnImageUsageFlags(ImageUsageFlags flags)
{
    return static_cast<MlnImageUsageFlags>(flags);
}

// Image type
inline MlnImageType ToMlnImageType(ImageType type)
{
    return static_cast<MlnImageType>(type);
}

// Image view type
inline MlnImageViewType ToMlnImageViewType(ImageViewType type)
{
    return static_cast<MlnImageViewType>(type);
}

// Image tiling
inline MlnImageTiling ToMlnImageTiling(ImageTiling tiling)
{
    return static_cast<MlnImageTiling>(tiling);
}

// Sample count flags
inline MlnSampleCountFlags ToMlnSampleCountFlags(SampleCountFlags flags)
{
    if (flags == 0) {
        return MLN_SAMPLE_COUNT_1_BIT;
    }
    return static_cast<MlnSampleCountFlags>(flags);
}

// Resolve mode flags (CORE and MLN enums have different bit positions)
inline MlnResolveModeFlagBits ToMlnResolveMode(ResolveModeFlags flags)
{
    if (flags == 0) {  // CORE_RESOLVE_MODE_NONE
        return MLN_RESOLVE_MODE_NONE;
    }
    // CORE: SAMPLE_ZERO=1, AVERAGE=2, MIN=4, MAX=8
    // MLN:  SAMPLE_ZERO=2, AVERAGE=4, MIN=8, MAX=16
    return static_cast<MlnResolveModeFlagBits>(static_cast<uint32_t>(flags) << 1u);
}

// Memory property flags
inline MlnMemoryPropertyFlags ToMlnMemoryPropertyFlags(MemoryPropertyFlags flags)
{
    return static_cast<MlnMemoryPropertyFlags>(flags);
}

// Filter
inline MlnFilter ToMlnFilter(Filter filter)
{
    return static_cast<MlnFilter>(filter);
}

// Sampler mipmap mode (Lume uses Filter enum for mipMapMode)
inline MlnSamplerMipmapMode ToMlnSamplerMipmapMode(Filter filter)
{
    return static_cast<MlnSamplerMipmapMode>(filter);
}

// Sampler address mode
inline MlnSamplerAddressMode ToMlnSamplerAddressMode(SamplerAddressMode mode)
{
    return static_cast<MlnSamplerAddressMode>(mode);
}

// Compare op
inline MlnCompareOp ToMlnCompareOp(CompareOp op)
{
    return static_cast<MlnCompareOp>(op);
}

// Border color
inline MlnBorderColor ToMlnBorderColor(BorderColor color)
{
    return static_cast<MlnBorderColor>(color);
}

// Component swizzle
inline MlnComponentSwizzle ToMlnComponentSwizzle(ComponentSwizzle swizzle)
{
    return static_cast<MlnComponentSwizzle>(swizzle);
}

// Image aspect flags (from Lume ImageAspectFlags)
inline MlnImageAspectFlags ToMlnImageAspectFlags(uint32_t flags)
{
    return static_cast<MlnImageAspectFlags>(flags);
}

// Shader stage flags: Lume VERTEX=0x01,FRAGMENT=0x10,COMPUTE=0x20; MLN VERTEX=0x01,FRAGMENT=0x02,COMPUTE=0x04
inline MlnShaderStageFlagBits ToMlnShaderStageFlagBits(ShaderStageFlags flags)
{
    MlnShaderStageFlagBits result = static_cast<MlnShaderStageFlagBits>(0);
    if (flags & CORE_SHADER_STAGE_VERTEX_BIT) {
        result = static_cast<MlnShaderStageFlagBits>(result | MLN_SHADER_STAGE_VERTEX_BIT);
    }
    if (flags & CORE_SHADER_STAGE_FRAGMENT_BIT) {
        result = static_cast<MlnShaderStageFlagBits>(result | MLN_SHADER_STAGE_FRAGMENT_BIT);
    }
    if (flags & CORE_SHADER_STAGE_COMPUTE_BIT) {
        result = static_cast<MlnShaderStageFlagBits>(result | MLN_SHADER_STAGE_COMPUTE_BIT);
    }
    return result;
}

// Pipeline state enums -- all Vulkan-derived, safe to static_cast
inline MlnPrimitiveTopology ToMlnPrimitiveTopology(PrimitiveTopology topology)
{
    return static_cast<MlnPrimitiveTopology>(topology);
}

inline MlnPolygonMode ToMlnPolygonMode(PolygonMode mode)
{
    return static_cast<MlnPolygonMode>(mode);
}

inline MlnCullModeFlags ToMlnCullModeFlags(CullModeFlags flags)
{
    return static_cast<MlnCullModeFlags>(flags);
}

inline MlnFrontFace ToMlnFrontFace(FrontFace face)
{
    return static_cast<MlnFrontFace>(face);
}

inline MlnBlendFactor ToMlnBlendFactor(BlendFactor factor)
{
    return static_cast<MlnBlendFactor>(factor);
}

inline MlnBlendOp ToMlnBlendOp(BlendOp op)
{
    return static_cast<MlnBlendOp>(op);
}

inline MlnStencilOp ToMlnStencilOp(StencilOp op)
{
    return static_cast<MlnStencilOp>(op);
}

inline MlnLogicOp ToMlnLogicOp(LogicOp op)
{
    return static_cast<MlnLogicOp>(op);
}

inline MlnColorComponentFlags ToMlnColorComponentFlags(ColorComponentFlags flags)
{
    return static_cast<MlnColorComponentFlags>(flags);
}

inline MlnDynamicStateType ToMlnDynamicStateType(DynamicStateEnum state)
{
    return static_cast<MlnDynamicStateType>(state);
}

RENDER_END_NAMESPACE()

#endif  // MALEOON_MLN_CONVERT_H
