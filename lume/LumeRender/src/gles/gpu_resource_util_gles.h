/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef GLES_GPU_RESOURCE_UTIL_GLES_H
#define GLES_GPU_RESOURCE_UTIL_GLES_H

#include <base/containers/string_view.h>
#include <render/namespace.h>

BASE_BEGIN_NAMESPACE()
class ByteArray;
BASE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class GpuBuffer;
class GpuImage;
class GpuSampler;
class IDevice;
namespace GpuResourceUtil {
void CopyGpuBufferGLES(GpuBuffer& buffer, BASE_NS::ByteArray& byteArray);
void DebugBufferNameGLES([[maybe_unused]] const IDevice& device, const GpuBuffer& buffer, BASE_NS::string_view name);
void DebugImageNameGLES([[maybe_unused]] const IDevice& device, const GpuImage& image, BASE_NS::string_view name);
void DebugSamplerNameGLES([[maybe_unused]] const IDevice& device, const GpuSampler& sampler, BASE_NS::string_view name);
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()

#endif // GLES_GPU_RESOURCE_UTIL_GLES_H
