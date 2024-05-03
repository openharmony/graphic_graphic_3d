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

#include "platform_vk.h"

RENDER_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::unordered_map;
using BASE_NS::vector;
namespace {
static constexpr string_view DEVICE_EXTENSION_EXTERNAL_MEMORY { VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME };
static constexpr string_view DEVICE_EXTENSION_EXTERNAL_MEMORY_CAPABILITIES {
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME
};
static constexpr string_view DEVICE_EXTENSION_GET_MEMORY_REQUIREMENTS2 {
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
};
static constexpr string_view DEVICE_EXTENSION_SAMPLER_YCBCR_CONVERSION {
    VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME
};
static constexpr string_view DEVICE_EXTENSION_QUEUE_FAMILY_FOREIGN { VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME };
static constexpr string_view DEVICE_EXTENSION_OPENHARMONY_EXTERNAL_MEMORY_OHOS_NATIVE_BUFFER {
    VK_OHOS_NATIVE_BUFFER_EXTENSION_NAME
};
} // namespace

void GetPlatformDeviceExtensions(vector<string_view>& extensions)
{
    extensions.push_back(DEVICE_EXTENSION_EXTERNAL_MEMORY);
    extensions.push_back(DEVICE_EXTENSION_EXTERNAL_MEMORY_CAPABILITIES);
    extensions.push_back(DEVICE_EXTENSION_GET_MEMORY_REQUIREMENTS2);
    extensions.push_back(DEVICE_EXTENSION_SAMPLER_YCBCR_CONVERSION);
    extensions.push_back(DEVICE_EXTENSION_QUEUE_FAMILY_FOREIGN);
    extensions.push_back(DEVICE_EXTENSION_OPENHARMONY_EXTERNAL_MEMORY_OHOS_NATIVE_BUFFER);
}

PlatformDeviceExtensions GetEnabledPlatformDeviceExtensions(
    const unordered_map<string, uint32_t>& enabledDeviceExtensions)
{
    return PlatformDeviceExtensions { enabledDeviceExtensions.contains(
        DEVICE_EXTENSION_OPENHARMONY_EXTERNAL_MEMORY_OHOS_NATIVE_BUFFER) };
}
RENDER_END_NAMESPACE()
