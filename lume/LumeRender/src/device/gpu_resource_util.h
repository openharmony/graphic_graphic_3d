/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
class Device;
class GpuBuffer;
class GpuImage;
class GpuSampler;
class GpuResourceManager;

namespace GpuResourceUtil {
void CopyGpuResource(const Device& device, const GpuResourceManager& gpuResourceMgr, const RenderHandle handle,
    BASE_NS::ByteArray& byteArray);
void DebugBufferName(const Device& device, const GpuBuffer& buffer, const BASE_NS::string_view name);
void DebugImageName(const Device& device, const GpuImage& image, const BASE_NS::string_view name);
void DebugSamplerName(const Device& device, const GpuSampler& sampler, const BASE_NS::string_view name);
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_RESOURCE_UTIL_H
