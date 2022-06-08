/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
class Device;
namespace GpuResourceUtil {
void CopyGpuBufferGLES(GpuBuffer& buffer, BASE_NS::ByteArray& byteArray);
void DebugBufferNameGLES(const Device& device, const GpuBuffer& buffer, const BASE_NS::string_view name);
void DebugImageNameGLES(const Device& device, const GpuImage& image, const BASE_NS::string_view name);
void DebugSamplerNameGLES(const Device& device, const GpuSampler& sampler, const BASE_NS::string_view name);
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()

#endif // GLES_GPU_RESOURCE_UTIL_GLES_H
