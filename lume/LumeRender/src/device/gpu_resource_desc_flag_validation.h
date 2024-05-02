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

#ifndef DEVICE_GPU_RESOURCE_DESC_FLAG_VALIDATION_H
#define DEVICE_GPU_RESOURCE_DESC_FLAG_VALIDATION_H

#include <base/util/formats.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** GPU resource desc flag validation
 */
struct GpuResourceDescFlagValidation {
    static constexpr uint32_t ALL_GPU_BUFFER_USAGE_FLAGS = {
        CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT | CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT | CORE_BUFFER_USAGE_INDEX_BUFFER_BIT |
        CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT | CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        CORE_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT | CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT |
        CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT
    };

    static constexpr uint32_t ALL_GPU_IMAGE_USAGE_FLAGS = {
        CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT |
        CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
        CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | CORE_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT
    };

    static constexpr uint32_t ALL_MEMORY_PROPERTY_FLAGS = {
        CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_CACHED_BIT |
        CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | CORE_MEMORY_PROPERTY_PROTECTED_BIT
    };

    static constexpr uint32_t ALL_IMAGE_CREATE_FLAGS = { CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT |
                                                         CORE_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT };
};
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_RESOURCE_DESC_FLAG_VALIDATION_H
