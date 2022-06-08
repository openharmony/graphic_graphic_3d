/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_VALIDATE_VK_H
#define VULKAN_VALIDATE_VK_H

#include <vulkan/vulkan_core.h>

#include <render/namespace.h>

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
#define VALIDATE_VK_RESULT(vk_func)                                                                       \
    {                                                                                                     \
        const VkResult result = (vk_func);                                                                \
        PLUGIN_UNUSED(result);                                                                            \
        PLUGIN_ASSERT_MSG(result == VK_SUCCESS, "vulkan result is not VK_SUCCESS : %d", (int32_t)result); \
    }
RENDER_END_NAMESPACE()

#endif // CORE__GFX__VULKAN__VALIDATE_VK_H
