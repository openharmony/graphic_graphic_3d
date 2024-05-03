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

#ifndef DEVICE_GPU_RESOURCE_UTIL_H
#define DEVICE_GPU_RESOURCE_UTIL_H

#include <base/containers/string_view.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

BASE_BEGIN_NAMESPACE()
class ByteArray;
BASE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class IDevice;
class GpuBuffer;
class GpuImage;
class GpuSampler;
class GpuResourceManager;

namespace GpuResourceUtil {
void CopyGpuResource(const IDevice& device, const GpuResourceManager& gpuResourceMgr, const RenderHandle handle,
    BASE_NS::ByteArray& byteArray);
void DebugBufferName(const IDevice& device, const GpuBuffer& buffer, const BASE_NS::string_view name);
void DebugImageName(const IDevice& device, const GpuImage& image, const BASE_NS::string_view name);
void DebugSamplerName(const IDevice& device, const GpuSampler& sampler, const BASE_NS::string_view name);
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_RESOURCE_UTIL_H
