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
#ifndef GLES_SPIRV_CROSS_HELPER_STRUCTS_H
#define GLES_SPIRV_CROSS_HELPER_STRUCTS_H

#include <base/containers/string.h>
#include <render/device/pipeline_state_desc.h>

RENDER_BEGIN_NAMESPACE()
namespace Gles {
// Bind limits.
// https://www.khronos.org/registry/vulkan/specs/1.1/html/chap31.html#limits-minmax
// maxBoundDescriptorSets                is 4     (so there can be at most 4 sets...)
// maxPerStageDescriptorUniformBuffers   is 12
// maxPerStageDescriptorStorageBuffers   is 4
// maxPerStageDescriptorStorageImages    is 4
// maxPerStageDescriptorSamplers         is 16
// maxPerStageDescriptorSampledImages    is 16
// maxDescriptorSetSamplers              is 96    (all stages)
// maxDescriptorSetSampledImages         is 96    (all stages)
// maxColorAttachments                   is 4     (subpass inputs?)
// maxVertexInputAttributes              is 16
// maxVertexInputBindings                is 16
// gles = GL_MAX_TEXTURE_IMAGE_UNITS 16 (so 16 texture units can be active, ie combined samplers/sampledimages and
// subpass inputs) NOTE: The macro allows for 16 sets, with 16 binds per type (uniform buffer, storage buffer)
struct ResourceLimits {
    // 4 slots and 16 binds = 64 possible binds.
    static constexpr uint32_t MAX_SETS { 4 };
    static constexpr uint32_t MAX_BIND_IN_SET { 16 };
    static constexpr uint32_t MAX_BINDS { MAX_SETS * MAX_BIND_IN_SET };

    static constexpr uint32_t MAX_VERTEXINPUT_ATTRIBUTES { 16 };
    static constexpr uint32_t MAX_UNIFORM_BUFFERS_IN_STAGE { 12 };
    static constexpr uint32_t MAX_STORAGE_BUFFERS_IN_STAGE { 4 };
    static constexpr uint32_t MAX_SAMPLERS_IN_STAGE { 16 };
    static constexpr uint32_t MAX_IMAGES_IN_STAGE { 16 };
    static constexpr uint32_t MAX_STORAGE_IMAGES_IN_STAGE { 4 };
    static constexpr uint32_t MAX_INPUT_ATTACHMENTS_IN_STAGE { 4 };
    static constexpr uint32_t MAX_SAMPLERS_IN_PROGRAM { MAX_SAMPLERS_IN_STAGE + MAX_SAMPLERS_IN_STAGE };
    static constexpr uint32_t MAX_IMAGES_IN_PROGRAM { MAX_IMAGES_IN_STAGE + MAX_IMAGES_IN_STAGE };
};
static constexpr int32_t INVALID_LOCATION = -1;
struct SpecConstantInfo {
    enum class Types { INVALID = 0, BOOL, UINT32, INT32, FLOAT };
    Types constantType = Types::INVALID;
    uint32_t constantId;
    uint32_t vectorSize;
    uint32_t columns;
    BASE_NS::string name;
};
struct PushConstantReflection {
    ShaderStageFlags stage;
    int32_t location { INVALID_LOCATION };
    uint32_t type;
    BASE_NS::string name;
    size_t offset;
    size_t size;
    size_t arraySize;
    size_t arrayStride;
    size_t matrixStride;
};
} // namespace Gles
RENDER_END_NAMESPACE()
#endif // GLES_SPIRV_CROSS_HELPER_STRUCTS_H
