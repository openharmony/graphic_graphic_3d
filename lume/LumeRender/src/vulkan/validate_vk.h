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

#ifndef VULKAN_VALIDATE_VK_H
#define VULKAN_VALIDATE_VK_H

#include <vulkan/vulkan_core.h>

#include <render/namespace.h>

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
#define VALIDATE_VK_RESULT(vk_func)                                                \
    {                                                                              \
        const VkResult result = (vk_func);                                         \
        if (result != VK_SUCCESS) {                                                \
            PLUGIN_LOG_E("vulkan result is not VK_SUCCESS : %d", (int32_t)result); \
        }                                                                          \
    }
RENDER_END_NAMESPACE()

#endif // VULKAN_VALIDATE_VK_H
