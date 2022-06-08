/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "util/log.h"
#include "vulkan/gpu_image_vk.h"

RENDER_BEGIN_NAMESPACE()
void GpuImageVk::CreatePlatformHwBuffer()
{
    PLUGIN_ASSERT_MSG(false, "hardware buffers not implemented");
}

void GpuImageVk::DestroyPlatformHwBuffer()
{
    PLUGIN_ASSERT_MSG(false, "hardware buffers not implemented");
}
RENDER_END_NAMESPACE()
