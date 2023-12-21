/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <string>

#include "array_view.h"
#include "shader_type.h"

/** Shader stage flag bits */
enum class ShaderStageFlagBits {
    /** Vertex bit */
    VERTEX_BIT = 0x00000001,
    /** Fragment bit */
    FRAGMENT_BIT = 0x00000010,
    /** Compute bit */
    COMPUTE_BIT = 0x00000020,
    /** All graphics */
    ALL_GRAPHICS = 0x0000001F,
    /** All */
    ALL = 0x7FFFFFFF,
    /** Max enumeration */
    FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
};

inline ShaderStageFlagBits ShaderKindToStageFlags(ShaderKind kind)
{
    switch (kind) {
        case ShaderKind::VERTEX:
            return ShaderStageFlagBits::VERTEX_BIT;
        case ShaderKind::FRAGMENT:
            return ShaderStageFlagBits::FRAGMENT_BIT;
        case ShaderKind::COMPUTE:
            return ShaderStageFlagBits::COMPUTE_BIT;
        default:
            return ShaderStageFlagBits::ALL_GRAPHICS;
    }
}

constexpr ShaderStageFlagBits operator~(ShaderStageFlagBits bits) noexcept
{
    return static_cast<ShaderStageFlagBits>(~static_cast<std::underlying_type_t<ShaderStageFlagBits>>(bits));
}

/** Shader stage flags */
struct ShaderStageFlags {
    ShaderStageFlagBits flags {};

    ShaderStageFlags() noexcept = default;
    ShaderStageFlags(ShaderStageFlagBits bits) noexcept : flags(bits) {}
    ShaderStageFlags(ShaderKind kind) noexcept
    {
        flags = ShaderKindToStageFlags(kind);
    }

    ShaderStageFlags operator&(ShaderStageFlagBits rhs) const noexcept
    {
        return { static_cast<ShaderStageFlagBits>(static_cast<std::underlying_type_t<ShaderStageFlagBits>>(flags) &
                                                  static_cast<std::underlying_type_t<ShaderStageFlagBits>>(rhs)) };
    }

    ShaderStageFlags operator~() const noexcept
    {
        return { static_cast<ShaderStageFlagBits>(~static_cast<std::underlying_type_t<ShaderStageFlagBits>>(flags)) };
    }

    ShaderStageFlags& operator&=(ShaderStageFlagBits rhs) noexcept
    {
        flags = static_cast<ShaderStageFlagBits>(static_cast<std::underlying_type_t<ShaderStageFlagBits>>(flags) &
                                                 static_cast<std::underlying_type_t<ShaderStageFlagBits>>(rhs));
        return *this;
    }

    ShaderStageFlags& operator|=(ShaderStageFlagBits rhs) noexcept
    {
        flags = static_cast<ShaderStageFlagBits>(static_cast<std::underlying_type_t<ShaderStageFlagBits>>(flags) |
                                                 static_cast<std::underlying_type_t<ShaderStageFlagBits>>(rhs));
        return *this;
    }

    ShaderStageFlags& operator|=(ShaderStageFlags rhs) noexcept
    {
        flags = static_cast<ShaderStageFlagBits>(static_cast<std::underlying_type_t<ShaderStageFlagBits>>(flags) |
                                                 static_cast<std::underlying_type_t<ShaderStageFlagBits>>(rhs.flags));
        return *this;
    }

    bool operator==(ShaderStageFlagBits rhs) const noexcept
    {
        return static_cast<std::underlying_type_t<ShaderStageFlagBits>>(flags) ==
               static_cast<std::underlying_type_t<ShaderStageFlagBits>>(rhs);
    }

    operator bool() const noexcept
    {
        return static_cast<std::underlying_type_t<ShaderStageFlagBits>>(flags) != 0;
    }
};

/** Constant */
struct ShaderSpecializationConstant {
    enum class Type : uint32_t {
        INVALID = 0,
        BOOL,
        UINT32,
        INT32,
        FLOAT,
    };
    /** Shader stage */
    ShaderStageFlags shaderStage;
    /** ID */
    uint32_t id;
    /** Size */
    Type type;
    /** Offset */
    uint32_t offset;
};

/** Shader specialization constant data view */
struct ShaderSpecializationConstantDataView {
    /** Array of shader specialization constants */
    array_view<const ShaderSpecializationConstant> constants;
    /** Data */
    array_view<const uint32_t> data;
};

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
    uint32_t offset;
    std::string name;
};
struct PushConstantReflection {
    ShaderStageFlags stage;
    int32_t location { INVALID_LOCATION };
    uint32_t type;
    std::string name;
    size_t offset;
    size_t size;
    size_t arraySize;
    size_t arrayStride;
    size_t matrixStride;
};
} // namespace Gles

#endif // GLES_SPIRV_CROSS_HELPER_STRUCTS_H
