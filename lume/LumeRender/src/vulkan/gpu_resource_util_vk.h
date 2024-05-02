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
class IDevice;
class GpuBuffer;
class GpuImage;
class GpuSampler;

namespace GpuResourceUtil {
void CopyGpuBufferVk(GpuBuffer& buffer, BASE_NS::ByteArray& byteArray);
void DebugObjectNameVk(
    const IDevice& device, const VkObjectType objectType, const uint64_t castedHandle, const BASE_NS::string_view name);
void DebugBufferNameVk(const IDevice& device, const GpuBuffer& buffer, const BASE_NS::string_view name);
void DebugImageNameVk(const IDevice& device, const GpuImage& image, const BASE_NS::string_view name);
void DebugSamplerNameVk(const IDevice& device, const GpuSampler& sampler, const BASE_NS::string_view name);
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_RESOURCE_UTIL_VK_H
