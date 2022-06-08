/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_GPU_RESOURCE_UTIL_VK_H
#define VULKAN_GPU_RESOURCE_UTIL_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/string_view.h>
#include <render/namespace.h>

BASE_BEGIN_NAMESPACE()
class ByteArray;
BASE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class Device;
class GpuBuffer;
class GpuImage;
class GpuSampler;

namespace GpuResourceUtil {
void CopyGpuBufferVk(GpuBuffer& buffer, BASE_NS::ByteArray& byteArray);
void DebugObjectNameVk(
    const Device& device, const VkObjectType objectType, const uint64_t castedHandle, const BASE_NS::string_view name);
void DebugBufferNameVk(const Device& device, const GpuBuffer& buffer, const BASE_NS::string_view name);
void DebugImageNameVk(const Device& device, const GpuImage& image, const BASE_NS::string_view name);
void DebugSamplerNameVk(const Device& device, const GpuSampler& sampler, const BASE_NS::string_view name);
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_RESOURCE_UTIL_VK_H
