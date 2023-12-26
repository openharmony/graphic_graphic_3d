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

#include "default_limits.h"
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvTools.h>
#include <spirv-tools/optimizer.hpp>

#include "spirv_cross.hpp"

// #include "preprocess/preprocess.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

#include "io/dev/FileMonitor.h"
#include "lume/Log.h"
#include "shader_type.h"
#include "spirv_cross_helpers_gles.h"

using namespace std::chrono_literals;

// Enumerations from Engine which should match: Format, DescriptorType, ShaderStageFlagBits
/** Format */
enum class Format {
    /** Undefined */
    UNDEFINED = 0,
    /** R4G4 UNORM PACK8 */
    R4G4_UNORM_PACK8 = 1,
    /** R4G4B4A4 UNORM PACK16 */
    R4G4B4A4_UNORM_PACK16 = 2,
    /** B4G4R4A4 UNORM PACK16 */
    B4G4R4A4_UNORM_PACK16 = 3,
    /** R5G6B5 UNORM PACK16 */
    R5G6B5_UNORM_PACK16 = 4,
    /** B5G6R5 UNORM PACK16 */
    B5G6R5_UNORM_PACK16 = 5,
    /** R5G5B5A1 UNORM PACK16 */
    R5G5B5A1_UNORM_PACK16 = 6,
    /** B5G5R5A1 UNORM PACK16 */
    B5G5R5A1_UNORM_PACK16 = 7,
    /** A1R5G5B5 UNORM PACK16 */
    A1R5G5B5_UNORM_PACK16 = 8,
    /** R8 UNORM */
    R8_UNORM = 9,
    /** R8 SNORM */
    R8_SNORM = 10,
    /** R8 USCALED */
    R8_USCALED = 11,
    /** R8 SSCALED */
    R8_SSCALED = 12,
    /** R8 UINT */
    R8_UINT = 13,
    /** R8 SINT */
    R8_SINT = 14,
    /** R8 SRGB */
    R8_SRGB = 15,
    /** R8G8 UNORM */
    R8G8_UNORM = 16,
    /** R8G8 SNORM */
    R8G8_SNORM = 17,
    /** R8G8 USCALED */
    R8G8_USCALED = 18,
    /** R8G8 SSCALED */
    R8G8_SSCALED = 19,
    /** R8G8 UINT */
    R8G8_UINT = 20,
    /** R8G8 SINT */
    R8G8_SINT = 21,
    /** R8G8 SRGB */
    R8G8_SRGB = 22,
    /** R8G8B8 UNORM */
    R8G8B8_UNORM = 23,
    /** R8G8B8 SNORM */
    R8G8B8_SNORM = 24,
    /** R8G8B8 USCALED */
    R8G8B8_USCALED = 25,
    /** R8G8B8 SSCALED */
    R8G8B8_SSCALED = 26,
    /** R8G8B8 UINT */
    R8G8B8_UINT = 27,
    /** R8G8B8 SINT */
    R8G8B8_SINT = 28,
    /** R8G8B8 SRGB */
    R8G8B8_SRGB = 29,
    /** B8G8R8 UNORM */
    B8G8R8_UNORM = 30,
    /** B8G8R8 SNORM */
    B8G8R8_SNORM = 31,
    /** B8G8R8 UINT */
    B8G8R8_UINT = 34,
    /** B8G8R8 SINT */
    B8G8R8_SINT = 35,
    /** B8G8R8 SRGB */
    B8G8R8_SRGB = 36,
    /** R8G8B8A8 UNORM */
    R8G8B8A8_UNORM = 37,
    /** R8G8B8A8 SNORM */
    R8G8B8A8_SNORM = 38,
    /** R8G8B8A8 USCALED */
    R8G8B8A8_USCALED = 39,
    /** R8G8B8A8 SSCALED */
    R8G8B8A8_SSCALED = 40,
    /** R8G8B8A8 UINT */
    R8G8B8A8_UINT = 41,
    /** R8G8B8A8 SINT */
    R8G8B8A8_SINT = 42,
    /** R8G8B8A8 SRGB */
    R8G8B8A8_SRGB = 43,
    /** B8G8R8A8 UNORM */
    B8G8R8A8_UNORM = 44,
    /** B8G8R8A8 SNORM */
    B8G8R8A8_SNORM = 45,
    /** B8G8R8A8 UINT */
    B8G8R8A8_UINT = 48,
    /** B8G8R8A8 SINT */
    B8G8R8A8_SINT = 49,
    /** FORMAT B8G8R8A8 SRGB */
    B8G8R8A8_SRGB = 50,
    /** A8B8G8R8 UNORM PACK32 */
    A8B8G8R8_UNORM_PACK32 = 51,
    /** A8B8G8R8 SNORM PACK32 */
    A8B8G8R8_SNORM_PACK32 = 52,
    /** A8B8G8R8 USCALED PACK32 */
    A8B8G8R8_USCALED_PACK32 = 53,
    /** A8B8G8R8 SSCALED PACK32 */
    A8B8G8R8_SSCALED_PACK32 = 54,
    /** A8B8G8R8 UINT PACK32 */
    A8B8G8R8_UINT_PACK32 = 55,
    /** A8B8G8R8 SINT PACK32 */
    A8B8G8R8_SINT_PACK32 = 56,
    /** A8B8G8R8 SRGB PACK32 */
    A8B8G8R8_SRGB_PACK32 = 57,
    /** A2R10G10B10 UNORM PACK32 */
    A2R10G10B10_UNORM_PACK32 = 58,
    /** A2R10G10B10 UINT PACK32 */
    A2R10G10B10_UINT_PACK32 = 62,
    /** A2R10G10B10 SINT PACK32 */
    A2R10G10B10_SINT_PACK32 = 63,
    /** A2B10G10R10 UNORM PACK32 */
    A2B10G10R10_UNORM_PACK32 = 64,
    /** A2B10G10R10 SNORM PACK32 */
    A2B10G10R10_SNORM_PACK32 = 65,
    /** A2B10G10R10 USCALED PACK32 */
    A2B10G10R10_USCALED_PACK32 = 66,
    /** A2B10G10R10 SSCALED PACK32 */
    A2B10G10R10_SSCALED_PACK32 = 67,
    /** A2B10G10R10 UINT PACK32 */
    A2B10G10R10_UINT_PACK32 = 68,
    /** A2B10G10R10 SINT PACK32 */
    A2B10G10R10_SINT_PACK32 = 69,
    /** R16 UNORM */
    R16_UNORM = 70,
    /** R16 SNORM */
    R16_SNORM = 71,
    /** R16 USCALED */
    R16_USCALED = 72,
    /** R16 SSCALED */
    R16_SSCALED = 73,
    /** R16 UINT */
    R16_UINT = 74,
    /** R16 SINT */
    R16_SINT = 75,
    /** R16 SFLOAT */
    R16_SFLOAT = 76,
    /** R16G16 UNORM */
    R16G16_UNORM = 77,
    /** R16G16 SNORM */
    R16G16_SNORM = 78,
    /** R16G16 USCALED */
    R16G16_USCALED = 79,
    /** R16G16 SSCALED */
    R16G16_SSCALED = 80,
    /** R16G16 UINT */
    R16G16_UINT = 81,
    /** R16G16 SINT */
    R16G16_SINT = 82,
    /** R16G16 SFLOAT */
    R16G16_SFLOAT = 83,
    /** R16G16B16 UNORM */
    R16G16B16_UNORM = 84,
    /** R16G16B16 SNORM */
    R16G16B16_SNORM = 85,
    /** R16G16B16 USCALED */
    R16G16B16_USCALED = 86,
    /** R16G16B16 SSCALED */
    R16G16B16_SSCALED = 87,
    /** R16G16B16 UINT */
    R16G16B16_UINT = 88,
    /** R16G16B16 SINT */
    R16G16B16_SINT = 89,
    /** R16G16B16 SFLOAT */
    R16G16B16_SFLOAT = 90,
    /** R16G16B16A16 UNORM */
    R16G16B16A16_UNORM = 91,
    /** R16G16B16A16 SNORM */
    R16G16B16A16_SNORM = 92,
    /** R16G16B16A16 USCALED */
    R16G16B16A16_USCALED = 93,
    /** R16G16B16A16 SSCALED */
    R16G16B16A16_SSCALED = 94,
    /** R16G16B16A16 UINT */
    R16G16B16A16_UINT = 95,
    /** R16G16B16A16 SINT */
    R16G16B16A16_SINT = 96,
    /** R16G16B16A16 SFLOAT */
    R16G16B16A16_SFLOAT = 97,
    /** R32 UINT */
    R32_UINT = 98,
    /** R32 SINT */
    R32_SINT = 99,
    /** R32 SFLOAT */
    R32_SFLOAT = 100,
    /** R32G32 UINT */
    R32G32_UINT = 101,
    /** R32G32 SINT */
    R32G32_SINT = 102,
    /** R32G32 SFLOAT */
    R32G32_SFLOAT = 103,
    /** R32G32B32 UINT */
    R32G32B32_UINT = 104,
    /** R32G32B32 SINT */
    R32G32B32_SINT = 105,
    /** R32G32B32 SFLOAT */
    R32G32B32_SFLOAT = 106,
    /** R32G32B32A32 UINT */
    R32G32B32A32_UINT = 107,
    /** R32G32B32A32 SINT */
    R32G32B32A32_SINT = 108,
    /** R32G32B32A32 SFLOAT */
    R32G32B32A32_SFLOAT = 109,
    /** B10G11R11 UFLOAT PACK32 */
    B10G11R11_UFLOAT_PACK32 = 122,
    /** E5B9G9R9 UFLOAT PACK32 */
    E5B9G9R9_UFLOAT_PACK32 = 123,
    /** D16 UNORM */
    D16_UNORM = 124,
    /** X8 D24 UNORM PACK32 */
    X8_D24_UNORM_PACK32 = 125,
    /** D32 SFLOAT */
    D32_SFLOAT = 126,
    /** S8 UINT */
    S8_UINT = 127,
    /** D24 UNORM S8 UINT */
    D24_UNORM_S8_UINT = 129,
    /** BC1 RGB UNORM BLOCK */
    BC1_RGB_UNORM_BLOCK = 131,
    /** BC1 RGB SRGB BLOCK */
    BC1_RGB_SRGB_BLOCK = 132,
    /** BC1 RGBA UNORM BLOCK */
    BC1_RGBA_UNORM_BLOCK = 133,
    /** BC1 RGBA SRGB BLOCK */
    BC1_RGBA_SRGB_BLOCK = 134,
    /** BC2 UNORM BLOCK */
    BC2_UNORM_BLOCK = 135,
    /** BC2 SRGB BLOCK */
    BC2_SRGB_BLOCK = 136,
    /** BC3 UNORM BLOCK */
    BC3_UNORM_BLOCK = 137,
    /** BC3 SRGB BLOCK */
    BC3_SRGB_BLOCK = 138,
    /** BC4 UNORM BLOCK */
    BC4_UNORM_BLOCK = 139,
    /** BC4 SNORM BLOCK */
    BC4_SNORM_BLOCK = 140,
    /** BC5 UNORM BLOCK */
    BC5_UNORM_BLOCK = 141,
    /** BC5 SNORM BLOCK */
    BC5_SNORM_BLOCK = 142,
    /** BC6H UFLOAT BLOCK */
    BC6H_UFLOAT_BLOCK = 143,
    /** BC6H SFLOAT BLOCK */
    BC6H_SFLOAT_BLOCK = 144,
    /** BC7 UNORM BLOCK */
    BC7_UNORM_BLOCK = 145,
    /** BC7 SRGB BLOCK */
    BC7_SRGB_BLOCK = 146,
    /** ETC2 R8G8B8 UNORM BLOCK */
    ETC2_R8G8B8_UNORM_BLOCK = 147,
    /** ETC2 R8G8B8 SRGB BLOCK */
    ETC2_R8G8B8_SRGB_BLOCK = 148,
    /** ETC2 R8G8B8A1 UNORM BLOCK */
    ETC2_R8G8B8A1_UNORM_BLOCK = 149,
    /** ETC2 R8G8B8A1 SRGB BLOCK */
    ETC2_R8G8B8A1_SRGB_BLOCK = 150,
    /** ETC2 R8G8B8A8 UNORM BLOCK */
    ETC2_R8G8B8A8_UNORM_BLOCK = 151,
    /** ETC2 R8G8B8A8 SRGB BLOCK */
    ETC2_R8G8B8A8_SRGB_BLOCK = 152,
    /** EAC R11 UNORM BLOCK */
    EAC_R11_UNORM_BLOCK = 153,
    /** EAC R11 SNORM BLOCK */
    EAC_R11_SNORM_BLOCK = 154,
    /** EAC R11G11 UNORM BLOCK */
    EAC_R11G11_UNORM_BLOCK = 155,
    /** EAC R11G11 SNORM BLOCK */
    EAC_R11G11_SNORM_BLOCK = 156,
    /** ASTC 4x4 UNORM BLOCK */
    ASTC_4x4_UNORM_BLOCK = 157,
    /** ASTC 4x4 SRGB BLOCK */
    ASTC_4x4_SRGB_BLOCK = 158,
    /** ASTC 5x4 UNORM BLOCK */
    ASTC_5x4_UNORM_BLOCK = 159,
    /** ASTC 5x4 SRGB BLOCK */
    ASTC_5x4_SRGB_BLOCK = 160,
    /** ASTC 5x5 UNORM BLOCK */
    ASTC_5x5_UNORM_BLOCK = 161,
    /** ASTC 5x5 SRGB BLOCK */
    ASTC_5x5_SRGB_BLOCK = 162,
    /** ASTC 6x5 UNORM BLOCK */
    ASTC_6x5_UNORM_BLOCK = 163,
    /** ASTC 6x5 SRGB BLOCK */
    ASTC_6x5_SRGB_BLOCK = 164,
    /** ASTC 6x6 UNORM BLOCK */
    ASTC_6x6_UNORM_BLOCK = 165,
    /** ASTC 6x6 SRGB BLOCK */
    ASTC_6x6_SRGB_BLOCK = 166,
    /** ASTC 8x5 UNORM BLOCK */
    ASTC_8x5_UNORM_BLOCK = 167,
    /** ASTC 8x5 SRGB BLOCK */
    ASTC_8x5_SRGB_BLOCK = 168,
    /** ASTC 8x6 UNORM BLOCK */
    ASTC_8x6_UNORM_BLOCK = 169,
    /** ASTC 8x6 SRGB BLOCK */
    ASTC_8x6_SRGB_BLOCK = 170,
    /** ASTC 8x8 UNORM BLOCK */
    ASTC_8x8_UNORM_BLOCK = 171,
    /** ASTC 8x8 SRGB BLOCK */
    ASTC_8x8_SRGB_BLOCK = 172,
    /** ASTC 10x5 UNORM BLOCK */
    ASTC_10x5_UNORM_BLOCK = 173,
    /** ASTC 10x5 SRGB BLOCK */
    ASTC_10x5_SRGB_BLOCK = 174,
    /** ASTC 10x6 UNORM BLOCK */
    ASTC_10x6_UNORM_BLOCK = 175,
    /** ASTC 10x6 SRGB BLOCK */
    ASTC_10x6_SRGB_BLOCK = 176,
    /** ASTC 10x8 UNORM BLOCK */
    ASTC_10x8_UNORM_BLOCK = 177,
    /** ASTC 10x8 SRGB BLOCK */
    ASTC_10x8_SRGB_BLOCK = 178,
    /** ASTC 10x10 UNORM BLOCK */
    ASTC_10x10_UNORM_BLOCK = 179,
    /** ASTC 10x10 SRGB BLOCK */
    ASTC_10x10_SRGB_BLOCK = 180,
    /** ASTC 12x10 UNORM BLOCK */
    ASTC_12x10_UNORM_BLOCK = 181,
    /** ASTC 12x10 SRGB BLOCK */
    ASTC_12x10_SRGB_BLOCK = 182,
    /** ASTC 12x12 UNORM BLOCK */
    ASTC_12x12_UNORM_BLOCK = 183,
    /** ASTC 12x12 SRGB BLOCK */
    ASTC_12x12_SRGB_BLOCK = 184,
    /** G8B8G8R8 422 UNORM */
    G8B8G8R8_422_UNORM = 1000156000,
    /** B8G8R8G8 422 UNORM */
    B8G8R8G8_422_UNORM = 1000156001,
    /** G8 B8 R8 3PLANE 420 UNORM */
    G8_B8_R8_3PLANE_420_UNORM = 1000156002,
    /** G8 B8R8 2PLANE 420 UNORM */
    G8_B8R8_2PLANE_420_UNORM = 1000156003,
    /** G8 B8 R8 3PLANE 422 UNORM */
    G8_B8_R8_3PLANE_422_UNORM = 1000156004,
    /** G8 B8R8 2PLANE 422 UNORM */
    G8_B8R8_2PLANE_422_UNORM = 1000156005,
    /** Max enumeration */
    MAX_ENUM = 0x7FFFFFFF
};

enum class DescriptorType {
    /** Sampler */
    SAMPLER = 0,
    /** Combined image sampler */
    COMBINED_IMAGE_SAMPLER = 1,
    /** Sampled image */
    SAMPLED_IMAGE = 2,
    /** Storage image */
    STORAGE_IMAGE = 3,
    /** Uniform texel buffer */
    UNIFORM_TEXEL_BUFFER = 4,
    /** Storage texel buffer */
    STORAGE_TEXEL_BUFFER = 5,
    /** Uniform buffer */
    UNIFORM_BUFFER = 6,
    /** Storage buffer */
    STORAGE_BUFFER = 7,
    /** Dynamic uniform buffer */
    UNIFORM_BUFFER_DYNAMIC = 8,
    /** Dynamic storage buffer */
    STORAGE_BUFFER_DYNAMIC = 9,
    /** Input attachment */
    INPUT_ATTACHMENT = 10,
    /** Acceleration structure */
    ACCELERATION_STRUCTURE = 1000150000,
    /** Max enumeration */
    MAX_ENUM = 0x7FFFFFFF
};

/** Vertex input rate */
enum class VertexInputRate {
    /** Vertex */
    VERTEX = 0,
    /** Instance */
    INSTANCE = 1,
    /** Max enumeration */
    MAX_ENUM = 0x7FFFFFFF
};

/** Pipeline layout constants */
struct PipelineLayoutConstants {
    /** Max descriptor set count */
    static constexpr uint32_t MAX_DESCRIPTOR_SET_COUNT { 4u };
    /** Max dynamic descriptor offset count */
    static constexpr uint32_t MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT { 16u };
    /** Invalid index */
    static constexpr uint32_t INVALID_INDEX { ~0u };
    /** Max push constant byte size */
    static constexpr uint32_t MAX_PUSH_CONSTANT_BYTE_SIZE { 128u };
};

/** Descriptor set layout binding */
struct DescriptorSetLayoutBinding {
    /** Binding */
    uint32_t binding { PipelineLayoutConstants::INVALID_INDEX };
    /** Descriptor type */
    DescriptorType descriptorType { DescriptorType::MAX_ENUM };
    /** Descriptor count */
    uint32_t descriptorCount { 0 };
    /** Stage flags */
    ShaderStageFlags shaderStageFlags;
};

/** Descriptor set layout */
struct DescriptorSetLayout {
    /** Set */
    uint32_t set { PipelineLayoutConstants::INVALID_INDEX };
    /** Bindings */
    std::vector<DescriptorSetLayoutBinding> bindings;
};

/** Push constant */
struct PushConstant {
    /** Shader stage flags */
    ShaderStageFlags shaderStageFlags;
    /** Byte size */
    uint32_t byteSize { 0 };
};

/** Pipeline layout */
struct PipelineLayout {
    /** Push constant */
    PushConstant pushConstant;
    /** Descriptor set count */
    uint32_t descriptorSetCount { 0 };
    /** Descriptor sets */
    DescriptorSetLayout descriptorSetLayouts[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] {};
};

constexpr const uint32_t RESERVED_CONSTANT_ID_INDEX { 256 };

/** Vertex input declaration */
struct VertexInputDeclaration {
    /** Vertex input binding description */
    struct VertexInputBindingDescription {
        /** Binding */
        uint32_t binding { ~0u };
        /** Stride */
        uint32_t stride { 0u };
        /** Vertex input rate */
        VertexInputRate vertexInputRate { VertexInputRate::MAX_ENUM };
    };

    /** Vertex input attribute description */
    struct VertexInputAttributeDescription {
        /** Location */
        uint32_t location { ~0u };
        /** Binding */
        uint32_t binding { ~0u };
        /** Format */
        Format format { Format::UNDEFINED };
        /** Offset */
        uint32_t offset { 0u };
    };
};

struct VertexAttributeInfo {
    uint32_t byteSize { 0 };
    VertexInputDeclaration::VertexInputAttributeDescription description;
};

struct UVec3 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct ShaderReflectionData {
    array_view<const uint8_t> reflectionData;

    bool IsValid() const;
    ShaderStageFlags GetStageFlags() const;
    PipelineLayout GetPipelineLayout() const;
    std::vector<ShaderSpecializationConstant> GetSpecializationConstants() const;
    std::vector<VertexInputDeclaration::VertexInputAttributeDescription> GetInputDescriptions() const;
    UVec3 GetLocalSize() const;
};

struct ShaderModuleCreateInfo {
    ShaderStageFlags shaderStageFlags;
    array_view<const uint8_t> spvData;
    ShaderReflectionData reflectionData;
};

struct CompilationSettings {
    ShaderEnv shaderEnv;
    std::vector<std::filesystem::path> shaderIncludePaths;
    std::optional<spvtools::Optimizer> optimizer;
    std::filesystem::path& shaderSourcePath;
    std::filesystem::path& compiledShaderDestinationPath;
};

constexpr uint8_t REFLECTION_TAG[] = { 'r', 'f', 'l', 0 };
struct ReflectionHeader {
    uint8_t tag[sizeof(REFLECTION_TAG)];
    uint16_t type;
    uint16_t offsetPushConstants;
    uint16_t offsetSpecializationConstants;
    uint16_t offsetDescriptorSets;
    uint16_t offsetInputs;
    uint16_t offsetLocalSize;
};

class scope {
private:
    std::function<void()> init;
    std::function<void()> deinit;

public:
    scope(const std::function<void()>&& initializer, const std::function<void()>&& deinitalizer)
        : init(initializer), deinit(deinitalizer)
    {
        init();
    }

    ~scope()
    {
        deinit();
    }
};

bool ShaderReflectionData::IsValid() const
{
    if (reflectionData.size() < sizeof(ReflectionHeader)) {
        return false;
    }
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    return memcmp(header.tag, REFLECTION_TAG, sizeof(REFLECTION_TAG)) == 0;
}

ShaderStageFlags ShaderReflectionData::GetStageFlags() const
{
    ShaderStageFlags flags;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    flags = static_cast<ShaderStageFlagBits>(header.type);
    return flags;
}

PipelineLayout ShaderReflectionData::GetPipelineLayout() const
{
    PipelineLayout pipelineLayout;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetPushConstants && header.offsetPushConstants < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetPushConstants;
        const auto constants = *ptr;
        if (constants) {
            pipelineLayout.pushConstant.shaderStageFlags = static_cast<ShaderStageFlagBits>(header.type);
            pipelineLayout.pushConstant.byteSize = static_cast<uint32_t>(*(ptr + 1) | (*(ptr + 2) << 8));
        }
    }
    if (header.offsetDescriptorSets && header.offsetDescriptorSets < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetDescriptorSets;
        pipelineLayout.descriptorSetCount = static_cast<uint32_t>(*(ptr) | (*(ptr + 1) << 8));
        ptr += 2;
        for (auto i = 0u; i < pipelineLayout.descriptorSetCount; ++i) {
            // write to correct set location
            const uint32_t set = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
            assert(set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
            auto& layout = pipelineLayout.descriptorSetLayouts[set];
            layout.set = set;
            ptr += 2;
            const auto bindings = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
            ptr += 2;
            for (auto j = 0u; j < bindings; ++j) {
                DescriptorSetLayoutBinding binding;
                binding.binding = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
                ptr += 2;
                binding.descriptorType = static_cast<DescriptorType>(*ptr | (*(ptr + 1) << 8));
                ptr += 2;
                binding.descriptorCount = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
                ptr += 2;
                binding.shaderStageFlags = static_cast<ShaderStageFlagBits>(header.type);
                layout.bindings.push_back(binding);
            }
        }
    }
    return pipelineLayout;
}

std::vector<ShaderSpecializationConstant> ShaderReflectionData::GetSpecializationConstants() const
{
    std::vector<ShaderSpecializationConstant> constants;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetSpecializationConstants && header.offsetSpecializationConstants < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetSpecializationConstants;
        const auto size = *ptr | *(ptr + 1) << 8 | *(ptr + 2) << 16 | *(ptr + 3) << 24;
        ptr += 4;
        for (auto i = 0; i < size; ++i) {
            ShaderSpecializationConstant constant;
            constant.shaderStage = static_cast<ShaderStageFlagBits>(header.type);
            constant.id = static_cast<uint32_t>(*ptr | *(ptr + 1) << 8 | *(ptr + 2) << 16 | *(ptr + 3) << 24);
            ptr += 4;
            constant.type = static_cast<ShaderSpecializationConstant::Type>(
                *ptr | *(ptr + 1) << 8 | *(ptr + 2) << 16 | *(ptr + 3) << 24);
            ptr += 4;
            constant.offset = 0;
            constants.push_back(constant);
        }
    }
    return constants;
}

std::vector<VertexInputDeclaration::VertexInputAttributeDescription> ShaderReflectionData::GetInputDescriptions() const
{
    std::vector<VertexInputDeclaration::VertexInputAttributeDescription> inputs;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetInputs && header.offsetInputs < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetInputs;
        const auto size = *(ptr) | (*(ptr + 1) << 8);
        ptr += 2;
        for (auto i = 0; i < size; ++i) {
            VertexInputDeclaration::VertexInputAttributeDescription desc;
            desc.location = static_cast<uint32_t>(*(ptr) | (*(ptr + 1) << 8));
            ptr += 2;
            desc.binding = desc.location;
            desc.format = static_cast<Format>(*(ptr) | (*(ptr + 1) << 8));
            ptr += 2;
            desc.offset = 0;
            inputs.push_back(desc);
        }
    }
    return inputs;
}

UVec3 ShaderReflectionData::GetLocalSize() const
{
    UVec3 sizes;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetLocalSize && header.offsetLocalSize < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetLocalSize;
        sizes.x = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8) | (*(ptr + 2)) << 16 | (*(ptr + 3)) << 24);
        ptr += 4;
        sizes.y = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8) | (*(ptr + 2)) << 16 | (*(ptr + 3)) << 24);
        ptr += 4;
        sizes.z = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8) | (*(ptr + 2)) << 16 | (*(ptr + 3)) << 24);
        ptr += 4;
    }
    return sizes;
}

std::string readFileToString(std::string_view aFilename)
{
    std::stringstream ss;
    std::ifstream file;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        file.open(aFilename.data(), std::ios::in);

        if (!file.fail()) {
            ss << file.rdbuf();
            return ss.str();
        }
    } catch (std::exception const& ex) {
        LUME_LOG_E("Error reading file: '%s': %s", aFilename.data(), ex.what());
        return {};
    }
    return {};
}

class FileIncluder : public glslang::TShader::Includer {
public:
    const CompilationSettings& settings;
    FileIncluder(const CompilationSettings& compilationSettings) : settings(compilationSettings) {}

private:
    virtual IncludeResult* include(
        const char* headerName, const char* includerName, size_t inclusionDepth, bool relative)
    {
        std::filesystem::path path;
        bool found = false;
        if (relative == true) {
            path.append(settings.shaderSourcePath.c_str());
            path.append(includerName);
            path = path.parent_path();
            path.append(headerName);
            found = std::filesystem::exists(path);
        }

        for (int i = 0; i < settings.shaderIncludePaths.size() && found == false; ++i) {
            path.assign(settings.shaderIncludePaths[i]);
            path.append(headerName);
            found = std::filesystem::exists(path);
        }

        if (found == true) {
            auto str = path.string();

            std::ifstream file(path);
            file.seekg(0, file.end);
            std::streampos length = file.tellg();
            file.seekg(0, file.beg);

            char* memory = new (std::nothrow) char[length + std::streampos(1)];
            if (memory == 0) {
                return nullptr;
            }

            char* last = std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), memory);
            IncludeResult* result = new (std::nothrow) IncludeResult(str, memory, std::distance(memory, last), 0);
            if (result == 0) {
                delete memory;
                return nullptr;
            }

            return result;
        }

        return nullptr;
    }

    virtual IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth)
    {
        return include(headerName, includerName, inclusionDepth, false);
    }

    virtual IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth)
    {
        return include(headerName, includerName, inclusionDepth, true);
    }

    virtual void releaseInclude(IncludeResult* include)
    {
        delete include;
    }
};

glslang::EShTargetLanguageVersion ToSpirVVersion(glslang::EShTargetClientVersion env_version)
{
    if (env_version == glslang::EShTargetVulkan_1_0) {
        return glslang::EShTargetSpv_1_0;
    } else if (env_version == glslang::EShTargetVulkan_1_1) {
        return glslang::EShTargetSpv_1_3;
    } else if (env_version == glslang::EShTargetVulkan_1_2) {
        return glslang::EShTargetSpv_1_5;
#if GLSLANG_VERSION >= GLSLANG_VERSION_12_2_0
    } else if (env_version == glslang::EShTargetVulkan_1_3) {
        return glslang::EShTargetSpv_1_6;
#endif
    } else {
        return glslang::EShTargetSpv_1_0;
    }
}

std::string preProcessShader(
    std::string_view aSource, ShaderKind aKind, std::string_view aSourceName, const CompilationSettings& settings)
{
    glslang::EShTargetLanguageVersion languageVersion;
    glslang::EShTargetClientVersion version;
    EShLanguage stage;
    switch (aKind) {
        case ShaderKind::VERTEX:
            stage = EShLanguage::EShLangVertex;
            break;
        case ShaderKind::FRAGMENT:
            stage = EShLanguage::EShLangFragment;
            break;
        case ShaderKind::COMPUTE:
            stage = EShLanguage::EShLangCompute;
            break;
        default:
            LUME_LOG_E("Spirv preprocessing compilation failed '%s'", "ShaderKind not recognized");
            return {};
    }

    switch (settings.shaderEnv) {
        case ShaderEnv::version_vulkan_1_0:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_0;
            break;
        case ShaderEnv::version_vulkan_1_1:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_1;
            break;
        case ShaderEnv::version_vulkan_1_2:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_2;
            break;
#if GLSLANG_VERSION >= GLSLANG_VERSION_12_2_0
        case ShaderEnv::version_vulkan_1_3:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_3;
            break;
#endif
        default:
            LUME_LOG_E("Spirv preprocessing compilation failed '%s'", "ShaderEnv not recognized");
            return {};
    }

    languageVersion = ToSpirVVersion(version);

    FileIncluder includer(settings);
    glslang::TShader shader(stage);
    const char* shader_strings = aSource.data();
    const int shader_lengths = static_cast<int>(aSource.size());
    const char* string_names = aSourceName.data();
    std::string_view preamble = "#extension GL_GOOGLE_include_directive : enable\n";
    shader.setStringsWithLengthsAndNames(&shader_strings, &shader_lengths, &string_names, 1);
    shader.setPreamble(preamble.data());
    shader.setEntryPoint("main");
    shader.setAutoMapBindings(false);
    shader.setAutoMapLocations(false);
    shader.setShiftImageBinding(0);
    shader.setShiftSamplerBinding(0);
    shader.setShiftTextureBinding(0);
    shader.setShiftUboBinding(0);
    shader.setShiftSsboBinding(0);
    shader.setShiftUavBinding(0);
    shader.setEnvClient(glslang::EShClient::EShClientVulkan, version);
    shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, languageVersion);
    shader.setInvertY(false);
    shader.setNanMinMaxClamp(false);

    std::string output;
    const EShMessages rules =
        static_cast<EShMessages>(EShMsgOnlyPreprocessor | EShMsgSpvRules | EShMsgVulkanRules | EShMsgCascadingErrors);
    if (!shader.preprocess(
            &kGLSLangDefaultTResource, 110, EProfile::ENoProfile, false, false, rules, &output, includer)) {
        LUME_LOG_E("Spirv preprocessing compilation failed '%s':\n%s", aSourceName.data(), shader.getInfoLog());
        LUME_LOG_E("Spirv preprocessing compilation failed '%s':\n%s", aSourceName.data(), shader.getInfoDebugLog());

        output = { output.begin() + preamble.size(), output.end() };
        return {};
    }

    output = { output.begin() + preamble.size(), output.end() };
    return output;
}

std::vector<uint32_t> compileShaderToSpirvBinary(
    std::string_view aSource, ShaderKind aKind, std::string_view aSourceName, const CompilationSettings& settings)
{
    glslang::EShTargetLanguageVersion languageVersion;
    glslang::EShTargetClientVersion version;
    EShLanguage stage;
    switch (aKind) {
        case ShaderKind::VERTEX:
            stage = EShLanguage::EShLangVertex;
            break;
        case ShaderKind::FRAGMENT:
            stage = EShLanguage::EShLangFragment;
            break;
        case ShaderKind::COMPUTE:
            stage = EShLanguage::EShLangCompute;
            break;
        default:
            LUME_LOG_E("Spirv binary compilation failed '%s'", "ShaderKind not recognized");
            return {};
    }

    switch (settings.shaderEnv) {
        case ShaderEnv::version_vulkan_1_0:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_0;
            break;
        case ShaderEnv::version_vulkan_1_1:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_1;
            break;
        case ShaderEnv::version_vulkan_1_2:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_2;
            break;
#if GLSLANG_VERSION >= GLSLANG_VERSION_12_2_0
        case ShaderEnv::version_vulkan_1_3:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_3;
            break;
#endif
        default:
            LUME_LOG_E("Spirv binary compilation failed '%s'", "ShaderEnv not recognized");
            return {};
    }

    languageVersion = ToSpirVVersion(version);

    glslang::TShader shader(stage);
    const char* shader_strings = aSource.data();
    const int shader_lengths = static_cast<int>(aSource.size());
    const char* string_names = aSourceName.data();
    shader.setStringsWithLengthsAndNames(&shader_strings, &shader_lengths, &string_names, 1);
    shader.setPreamble("#extension GL_GOOGLE_include_directive : enable\n");
    shader.setEntryPoint("main");
    shader.setAutoMapBindings(false);
    shader.setAutoMapLocations(false);
    shader.setShiftImageBinding(0);
    shader.setShiftSamplerBinding(0);
    shader.setShiftTextureBinding(0);
    shader.setShiftUboBinding(0);
    shader.setShiftSsboBinding(0);
    shader.setShiftUavBinding(0);
    shader.setEnvClient(glslang::EShClient::EShClientVulkan, version);
    shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, languageVersion);
    shader.setInvertY(false);
    shader.setNanMinMaxClamp(false);

    const EShMessages rules = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgCascadingErrors);
    if (!shader.parse(&kGLSLangDefaultTResource, 110, EProfile::ENoProfile, false, false, rules)) {
        LUME_LOG_E("Spirv binary compilation failed '%s':\n%s", aSourceName.data(), shader.getInfoLog());
        LUME_LOG_E("Spirv binary compilation failed '%s':\n%s", aSourceName.data(), shader.getInfoDebugLog());
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault) || !program.mapIO()) {
        LUME_LOG_E("Spirv binary compilation failed '%s':\n%s", aSourceName.data(), program.getInfoLog());
        LUME_LOG_E("Spirv binary compilation failed '%s':\n%s", aSourceName.data(), program.getInfoDebugLog());
        return {};
    }

    std::vector<unsigned int> spirv;
    glslang::SpvOptions spv_options;
    spv_options.generateDebugInfo = false;
    spv_options.disableOptimizer = true;
    spv_options.optimizeSize = false;
    spv::SpvBuildLogger logger;
    glslang::TIntermediate* intermediate = program.getIntermediate(stage);
    glslang::GlslangToSpv(*intermediate, spirv, &logger, &spv_options);

    const uint32_t shaderc_generator_word = 13; // From SPIR-V XML Registry
    const uint32_t generator_word_index = 2;    // SPIR-V 2.3: Physical layout
    assert(spirv.size() > generator_word_index);
    spirv[generator_word_index] = (spirv[generator_word_index] & 0xffff) | (shaderc_generator_word << 16);
    return spirv;
}

void processResource(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource,
    ShaderStageFlags shaderStateFlags, DescriptorType type, DescriptorSetLayout* layouts)
{
    const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

    assert(set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
    if (set >= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        return;
    }
    DescriptorSetLayout& layout = layouts[set];
    layout.set = set;

    // Collect bindings.
    const uint32_t bindingIndex = compiler.get_decoration(resource.id, spv::DecorationBinding);
    auto& bindings = layout.bindings;
    if (auto pos = std::find_if(bindings.begin(), bindings.end(),
            [bindingIndex](const DescriptorSetLayoutBinding& binding) { return binding.binding == bindingIndex; });
        pos == bindings.end()) {
        const spirv_cross::SPIRType& spirType = compiler.get_type(resource.type_id);

        DescriptorSetLayoutBinding binding;
        binding.binding = bindingIndex;
        binding.descriptorType = type;
        binding.descriptorCount = spirType.array.empty() ? 1 : spirType.array[0];
        binding.shaderStageFlags = shaderStateFlags;

        bindings.emplace_back(binding);
    } else {
        pos->shaderStageFlags |= shaderStateFlags;
    }
}

void reflectDescriptorSets(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
    ShaderStageFlags shaderStateFlags, DescriptorSetLayout* layouts)
{
    for (const auto& ref : resources.sampled_images) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::COMBINED_IMAGE_SAMPLER, layouts);
    }

    for (const auto& ref : resources.separate_samplers) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::SAMPLER, layouts);
    }

    for (const auto& ref : resources.separate_images) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::SAMPLED_IMAGE, layouts);
    }

    for (const auto& ref : resources.storage_images) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::STORAGE_IMAGE, layouts);
    }

    for (const auto& ref : resources.uniform_buffers) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::UNIFORM_BUFFER, layouts);
    }

    for (const auto& ref : resources.storage_buffers) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::STORAGE_BUFFER, layouts);
    }

    for (const auto& ref : resources.subpass_inputs) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::INPUT_ATTACHMENT, layouts);
    }

    for (const auto& ref : resources.acceleration_structures) {
        processResource(compiler, ref, shaderStateFlags, DescriptorType::ACCELERATION_STRUCTURE, layouts);
    }

    std::sort(layouts, layouts + PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT,
        [](const DescriptorSetLayout& lhs, const DescriptorSetLayout& rhs) { return (lhs.set < rhs.set); });

    std::for_each(
        layouts, layouts + PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT, [](DescriptorSetLayout& layout) {
            std::sort(layout.bindings.begin(), layout.bindings.end(),
                [](const DescriptorSetLayoutBinding& lhs, const DescriptorSetLayoutBinding& rhs) {
                    return (lhs.binding < rhs.binding);
                });
        });
}

void reflectPushContants(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
    ShaderStageFlags shaderStateFlags, PushConstant& pushConstant)
{
    // NOTE: support for only one push constant
    if (resources.push_constant_buffers.size() > 0) {
        pushConstant.shaderStageFlags |= shaderStateFlags;

        const auto ranges = compiler.get_active_buffer_ranges(resources.push_constant_buffers[0].id);
        const uint32_t byteSize = std::accumulate(
            ranges.begin(), ranges.end(), 0u, [](uint32_t byteSize, const spirv_cross::BufferRange& range) {
                return byteSize + static_cast<uint32_t>(range.range);
            });
        pushConstant.byteSize = std::max(pushConstant.byteSize, byteSize);
    }
}

std::vector<ShaderSpecializationConstant> reflectSpecializationConstants(
    const spirv_cross::Compiler& compiler, ShaderStageFlags shaderStateFlags)
{
    std::vector<ShaderSpecializationConstant> specializationConstants;
    uint32_t offset = 0;
    for (auto const& constant : compiler.get_specialization_constants()) {
        if (constant.constant_id < RESERVED_CONSTANT_ID_INDEX) {
            const spirv_cross::SPIRConstant& spirvConstant = compiler.get_constant(constant.id);
            const auto type = compiler.get_type(spirvConstant.constant_type);
            ShaderSpecializationConstant::Type constantType = ShaderSpecializationConstant::Type::INVALID;
            if (type.basetype == spirv_cross::SPIRType::Boolean) {
                constantType = ShaderSpecializationConstant::Type::BOOL;
            } else if (type.basetype == spirv_cross::SPIRType::UInt) {
                constantType = ShaderSpecializationConstant::Type::UINT32;
            } else if (type.basetype == spirv_cross::SPIRType::Int) {
                constantType = ShaderSpecializationConstant::Type::INT32;
            } else if (type.basetype == spirv_cross::SPIRType::Float) {
                constantType = ShaderSpecializationConstant::Type::FLOAT;
            } else {
                assert(false && "Unhandled specialization constant type");
            }
            const uint32_t size = spirvConstant.vector_size() * spirvConstant.columns() * sizeof(uint32_t);
            specializationConstants.push_back(
                ShaderSpecializationConstant { shaderStateFlags, constant.constant_id, constantType, offset });
            offset += size;
        }
    }
    // sorted based on offset due to offset mapping with shader combinations
    // NOTE: id and name indexing
    std::sort(specializationConstants.begin(), specializationConstants.end(),
        [](const auto& lhs, const auto& rhs) { return (lhs.offset < rhs.offset); });

    return specializationConstants;
}

Format convertToVertexInputFormat(const spirv_cross::SPIRType& type)
{
    using BaseType = spirv_cross::SPIRType::BaseType;

    // ivecn: a vector of signed integers
    if (type.basetype == BaseType::Int) {
        switch (type.vecsize) {
            case 1:
                return Format::R32_SINT;
            case 2:
                return Format::R32G32_SINT;
            case 3:
                return Format::R32G32B32_SINT;
            case 4:
                return Format::R32G32B32A32_SINT;
        }
    }

    // uvecn: a vector of unsigned integers
    if (type.basetype == BaseType::UInt) {
        switch (type.vecsize) {
            case 1:
                return Format::R32_UINT;
            case 2:
                return Format::R32G32_UINT;
            case 3:
                return Format::R32G32B32_UINT;
            case 4:
                return Format::R32G32B32A32_UINT;
        }
    }

    // halfn: a vector of half-precision floating-point numbers
    if (type.basetype == BaseType::Half) {
        switch (type.vecsize) {
            case 1:
                return Format::R16_SFLOAT;
            case 2:
                return Format::R16G16_SFLOAT;
            case 3:
                return Format::R16G16B16_SFLOAT;
            case 4:
                return Format::R16G16B16A16_SFLOAT;
        }
    }

    // vecn: a vector of single-precision floating-point numbers
    if (type.basetype == BaseType::Float) {
        switch (type.vecsize) {
            case 1:
                return Format::R32_SFLOAT;
            case 2:
                return Format::R32G32_SFLOAT;
            case 3:
                return Format::R32G32B32_SFLOAT;
            case 4:
                return Format::R32G32B32A32_SFLOAT;
        }
    }

    return Format::UNDEFINED;
}

void reflectVertexInputs(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
    ShaderStageFlags /* shaderStateFlags */,
    std::vector<VertexInputDeclaration::VertexInputAttributeDescription>& vertexInputAttributes)
{
    std::vector<VertexAttributeInfo> vertexAttributeInfos;

    // Vertex input attributes.
    for (auto& attr : resources.stage_inputs) {
        const spirv_cross::SPIRType attributeType = compiler.get_type(attr.type_id);

        VertexAttributeInfo info;

        // For now, assume that every vertex attribute comes from it's own binding which equals the location.
        info.description.location = compiler.get_decoration(attr.id, spv::DecorationLocation);
        info.description.binding = info.description.location;
        info.description.format = convertToVertexInputFormat(attributeType);
        info.description.offset = 0;

        info.byteSize = attributeType.vecsize * (attributeType.width / 8);

        vertexAttributeInfos.emplace_back(std::move(info));
    }

    // Sort input attributes by binding and location.
    std::sort(std::begin(vertexAttributeInfos), std::end(vertexAttributeInfos),
        [](const VertexAttributeInfo& aA, const VertexAttributeInfo& aB) {
            if (aA.description.binding < aB.description.binding) {
                return true;
            }

            return aA.description.location < aB.description.location;
        });

    // Create final attributes.
    if (!vertexAttributeInfos.empty()) {
        for (auto& info : vertexAttributeInfos) {
            vertexInputAttributes.push_back(info.description);
        }
    }
}

template<typename T>
void push(std::vector<uint8_t>& buffer, T data)
{
    buffer.push_back(data & 0xff);
    if constexpr (sizeof(T) > 1) {
        buffer.push_back((data >> 8) & 0xff);
    }
    if constexpr (sizeof(T) > 2) {
        buffer.push_back((data >> 16) & 0xff);
    }
    if constexpr (sizeof(T) > 3) {
        buffer.push_back((data >> 24) & 0xff);
    }
}

std::vector<uint8_t> reflectSpvBinary(const std::vector<uint32_t>& aBinary, ShaderKind aKind)
{
    const spirv_cross::Compiler compiler(aBinary);

    const auto shaderStateFlags = ShaderStageFlags(aKind);

    const spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    PipelineLayout pipelineLayout;
    reflectDescriptorSets(compiler, resources, shaderStateFlags, pipelineLayout.descriptorSetLayouts);
    pipelineLayout.descriptorSetCount =
        static_cast<uint32_t>(std::count_if(std::begin(pipelineLayout.descriptorSetLayouts),
            std::end(pipelineLayout.descriptorSetLayouts), [](const DescriptorSetLayout& layout) {
                return layout.set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT;
            }));
    reflectPushContants(compiler, resources, shaderStateFlags, pipelineLayout.pushConstant);

    // some additional information mainly for GL
    std::vector<Gles::PushConstantReflection> pushConstantReflection;
    for (auto& remap : resources.push_constant_buffers) {
        const auto& blockType = compiler.get_type(remap.base_type_id);
        auto name = compiler.get_name(remap.id);
        (void)(blockType);
        assert((blockType.basetype == spirv_cross::SPIRType::Struct) && "Push constant is not a struct!");
        Gles::ProcessStruct(std::string_view(name.data(), name.size()), 0, compiler, remap.base_type_id,
            pushConstantReflection, shaderStateFlags);
    }

    auto specializationConstants = reflectSpecializationConstants(compiler, shaderStateFlags);

    // NOTE: this is done for all although the name is 'Vertex'InputAttributes
    std::vector<VertexInputDeclaration::VertexInputAttributeDescription> vertexInputAttributes;
    reflectVertexInputs(compiler, resources, shaderStateFlags, vertexInputAttributes);

    std::vector<uint8_t> reflection;
    reflection.reserve(512u);
    constexpr uint8_t TAG[] = { 'r', 'f', 'l', 0 }; // last one is version
    uint16_t type = 0;
    uint16_t offsetPushConstants = 0;
    uint16_t offsetSpecializationConstants = 0;
    uint16_t offsetDescriptorSets = 0;
    uint16_t offsetInputs = 0;
    uint16_t offsetLocalSize = 0;
    // tag
    {
        reflection.insert(reflection.end(), std::begin(TAG), std::end(TAG));
    }
    // shader type
    {
        push(reflection, static_cast<uint16_t>(shaderStateFlags.flags));
    }
    // offsets
    {
        reflection.resize(reflection.size() + sizeof(uint16_t) * 5);
    }
    // push constants
    {
        offsetPushConstants = static_cast<uint16_t>(reflection.size());
        if (pipelineLayout.pushConstant.byteSize) {
            reflection.push_back(1);
            push(reflection, static_cast<uint16_t>(pipelineLayout.pushConstant.byteSize));

            push(reflection, static_cast<uint8_t>(pushConstantReflection.size()));
            for (const auto& refl : pushConstantReflection) {
                push(reflection, refl.type);
                push(reflection, static_cast<uint16_t>(refl.offset));
                push(reflection, static_cast<uint16_t>(refl.size));
                push(reflection, static_cast<uint16_t>(refl.arraySize));
                push(reflection, static_cast<uint16_t>(refl.arrayStride));
                push(reflection, static_cast<uint16_t>(refl.matrixStride));
                push(reflection, static_cast<uint16_t>(refl.name.size()));
                reflection.insert(reflection.end(), std::begin(refl.name), std::end(refl.name));
            }
        } else {
            reflection.push_back(0);
        }
    }
    // specialization constants
    {
        offsetSpecializationConstants = static_cast<uint16_t>(reflection.size());
        {
            const auto size = static_cast<uint32_t>(specializationConstants.size());
            push(reflection, static_cast<uint32_t>(specializationConstants.size()));
        }
        for (auto const& constant : specializationConstants) {
            push(reflection, static_cast<uint32_t>(constant.id));
            push(reflection, static_cast<uint32_t>(constant.type));
        }
    }
    // descriptor sets
    {
        offsetDescriptorSets = static_cast<uint16_t>(reflection.size());
        {
            push(reflection, static_cast<uint16_t>(pipelineLayout.descriptorSetCount));
        }
        auto begin = std::begin(pipelineLayout.descriptorSetLayouts);
        auto end = begin;
        std::advance(end, pipelineLayout.descriptorSetCount);
        std::for_each(begin, end, [&reflection](const DescriptorSetLayout& layout) {
            push(reflection, static_cast<uint16_t>(layout.set));
            push(reflection, static_cast<uint16_t>(layout.bindings.size()));
            for (const auto& binding : layout.bindings) {
                push(reflection, static_cast<uint16_t>(binding.binding));
                push(reflection, static_cast<uint16_t>(binding.descriptorType));
                push(reflection, static_cast<uint16_t>(binding.descriptorCount));
            }
        });
    }
    // inputs
    {
        offsetInputs = static_cast<uint16_t>(reflection.size());
        const auto size = static_cast<uint16_t>(vertexInputAttributes.size());
        push(reflection, size);
        for (const auto& input : vertexInputAttributes) {
            push(reflection, static_cast<uint16_t>(input.location));
            push(reflection, static_cast<uint16_t>(input.format));
        }
    }
    // local size
    if (shaderStateFlags & ShaderStageFlagBits::COMPUTE_BIT) {
        offsetLocalSize = static_cast<uint16_t>(reflection.size());
        uint32_t size = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
        push(reflection, size);

        size = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
        push(reflection, size);

        size = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
        push(reflection, size);
    }
    // update offsets to real values
    {
        auto ptr = reflection.data() + (sizeof(TAG) + sizeof(type));
        *ptr++ = offsetPushConstants & 0xff;
        *ptr++ = (offsetPushConstants >> 8) & 0xff;
        *ptr++ = offsetSpecializationConstants & 0xff;
        *ptr++ = (offsetSpecializationConstants >> 8) & 0xff;
        *ptr++ = offsetDescriptorSets & 0xff;
        *ptr++ = (offsetDescriptorSets >> 8) & 0xff;
        *ptr++ = offsetInputs & 0xff;
        *ptr++ = (offsetInputs >> 8) & 0xff;
        *ptr++ = offsetLocalSize & 0xff;
        *ptr++ = (offsetLocalSize >> 8) & 0xff;
    }

    return reflection;
}

struct Binding {
    uint8_t set;
    uint8_t bind;
};

Binding get_binding(Gles::CoreCompiler& compiler, spirv_cross::ID id)
{
    const uint32_t dset = compiler.get_decoration(id, spv::Decoration::DecorationDescriptorSet);
    const uint32_t dbind = compiler.get_decoration(id, spv::Decoration::DecorationBinding);
    assert(dset < Gles::ResourceLimits::MAX_SETS);
    assert(dbind < Gles::ResourceLimits::MAX_BIND_IN_SET);
    const uint8_t set = static_cast<uint8_t>(dset);
    const uint8_t bind = static_cast<uint8_t>(dbind);
    return { set, bind };
}

void SortSets(PipelineLayout& pipelineLayout)
{
    pipelineLayout.descriptorSetCount = 0;
    for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
        DescriptorSetLayout& currSet = pipelineLayout.descriptorSetLayouts[idx];
        if (currSet.set != PipelineLayoutConstants::INVALID_INDEX) {
            pipelineLayout.descriptorSetCount++;
            std::sort(currSet.bindings.begin(), currSet.bindings.end(),
                [](auto const& lhs, auto const& rhs) { return (lhs.binding < rhs.binding); });
        }
    }
}

void Collect(Gles::CoreCompiler& compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    const uint32_t forceBinding = 0)
{
    std::string name;

    for (const auto& remap : resources) {
        const auto binding = get_binding(compiler, remap.id);

        name.resize(name.capacity() - 1);
        const auto nameLen = sprintf(name.data(), "s%u_b%u", binding.set, binding.bind);
        name.resize(nameLen);

        // if name is empty it's a block and we need to rename the base_type_id i.e.
        // uniform <base_type_id> { vec4 foo; } <id>;
        if (auto origname = compiler.get_name(remap.id); origname.empty()) {
            compiler.set_name(remap.base_type_id, name);
            name.insert(name.begin(), '_');
            compiler.set_name(remap.id, name);
        } else {
            // uniform <id> vec4 foo;
            compiler.set_name(remap.id, name);
        }

        compiler.unset_decoration(remap.id, spv::DecorationDescriptorSet);
        compiler.unset_decoration(remap.id, spv::DecorationBinding);
        if (forceBinding > 0) {
            compiler.set_decoration(
                remap.id, spv::DecorationBinding, forceBinding - 1); // will be over-written later. (special handling)
        }
    }
}

struct ShaderModulePlatformDataGLES {
    std::vector<Gles::PushConstantReflection> infos;
};

void CollectRes(
    Gles::CoreCompiler& compiler, const spirv_cross::ShaderResources& res, ShaderModulePlatformDataGLES& plat_)
{
    // collect names for later linkage
    static constexpr uint32_t DefaultBinding = 11;
    Collect(compiler, res.storage_buffers, DefaultBinding + 1);
    Collect(compiler, res.storage_images, DefaultBinding + 1);
    Collect(compiler, res.uniform_buffers, 0); // 0 == remove binding decorations (let's the compiler decide)
    Collect(compiler, res.subpass_inputs, 0);  // 0 == remove binding decorations (let's the compiler decide)

    // handle the real sampled images.
    Collect(compiler, res.sampled_images, 0); // 0 == remove binding decorations (let's the compiler decide)

    // and now the "generated ones" (separate image/sampler)
    std::string imageName;
    std::string samplerName;
    std::string temp;
    for (auto& remap : compiler.get_combined_image_samplers()) {
        const auto imageBinding = get_binding(compiler, remap.image_id);
        {
            imageName.resize(imageName.capacity() - 1);
            const auto nameLen = sprintf(imageName.data(), "s%u_b%u", imageBinding.set, imageBinding.bind);
            imageName.resize(nameLen);
        }
        const auto samplerBinding = get_binding(compiler, remap.sampler_id);
        {
            samplerName.resize(samplerName.capacity() - 1);
            const auto nameLen = sprintf(samplerName.data(), "s%u_b%u", samplerBinding.set, samplerBinding.bind);
            samplerName.resize(nameLen);
        }

        temp.reserve(imageName.size() + samplerName.size() + 1);
        temp.clear();
        temp.append(imageName);
        temp.append(1, '_');
        temp.append(samplerName);
        compiler.set_name(remap.combined_id, temp);
    }
}

/** Device backend type */
enum class DeviceBackendType {
    /** Vulkan backend */
    VULKAN,
    /** GLES backend */
    OPENGLES,
    /** OpenGL backend */
    OPENGL
};

void SetupSpirvCross(ShaderStageFlags stage, Gles::CoreCompiler* compiler, DeviceBackendType backend, bool ovrEnabled)
{
    spirv_cross::CompilerGLSL::Options options;

    if (backend == DeviceBackendType::OPENGLES) {
        options.version = 320;
        options.es = true;
        options.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Precision::Highp;
        options.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Precision::Highp;
    }

    if (backend == DeviceBackendType::OPENGL) {
        options.version = 450;
        options.es = false;
    }

#if defined(CORE_USE_SEPARATE_SHADER_OBJECTS) && (CORE_USE_SEPARATE_SHADER_OBJECTS == 1)
    if (stage & (CORE_SHADER_STAGE_VERTEX_BIT | CORE_SHADER_STAGE_FRAGMENT_BIT)) {
        options.separate_shader_objects = true;
    }
#endif

    options.ovr_multiview_view_count = ovrEnabled ? 1 : 0;

    compiler->set_common_options(options);
}

struct Shader {
    ShaderStageFlags shaderStageFlags_;
    DeviceBackendType backend_;
    ShaderModulePlatformDataGLES plat_;
    bool ovrEnabled;

    std::string source_;
};

void ProcessShaderModule(Shader& me, const ShaderModuleCreateInfo& createInfo)
{
    // perform reflection.
    auto compiler = Gles::CoreCompiler(reinterpret_cast<const uint32_t*>(createInfo.spvData.data()),
        static_cast<uint32_t>(createInfo.spvData.size() / sizeof(uint32_t)));
    // Set some options.
    SetupSpirvCross(me.shaderStageFlags_, &compiler, me.backend_, me.ovrEnabled);

    // first step in converting CORE_FLIP_NDC to regular uniform. (specializationconstant -> constant) this makes the
    // compiled glsl more readable, and simpler to post process later.
    Gles::ConvertSpecConstToConstant(compiler, "CORE_FLIP_NDC");
    // const auto& res = compiler.get_shader_resources();

    auto active = compiler.get_active_interface_variables();
    const auto& res = compiler.get_shader_resources(active);
    compiler.set_enabled_interface_variables(std::move(active));

    Gles::ReflectPushConstants(compiler, res, me.plat_.infos, me.shaderStageFlags_);
    compiler.build_combined_image_samplers();
    CollectRes(compiler, res, me.plat_);

    // set "CORE_BACKEND_TYPE" specialization to 1.
    Gles::SetSpecMacro(compiler, "CORE_BACKEND_TYPE", 1U);

    me.source_ = compiler.compile();
    Gles::ConvertConstantToUniform(compiler, me.source_, "CORE_FLIP_NDC");
}

template<typename T>
bool writeToFile(const array_view<T>& data, std::filesystem::path aDestinationFile)
{
    std::ofstream outputStream(aDestinationFile, std::ios::out | std::ios::binary);
    if (outputStream.is_open()) {
        outputStream.write((const char*)data.data(), data.size() * sizeof(T));
        outputStream.close();
        return true;
    } else {
        LUME_LOG_E("Could not write file: '%s'", aDestinationFile.string().c_str());
        return false;
    }
}

bool runAllCompilationStages(std::string_view inputFilename, CompilationSettings& settings)
{
    try {
        // std::string inputFilename = aFile;
        const std::filesystem::path relativeInputFilename =
            std::filesystem::relative(inputFilename, settings.shaderSourcePath);
        const std::string relativeFilename = relativeInputFilename.string();
        const std::string extension = std::filesystem::path(inputFilename).extension().string();
        std::filesystem::path outputFilename = settings.compiledShaderDestinationPath / relativeInputFilename;

        // Make sure the output dir hierarchy exists.
        std::filesystem::create_directories(outputFilename.parent_path());

        // Just copying json files to the destination dir.
        if (extension == ".json") {
            if (!std::filesystem::exists(outputFilename) ||
                !std::filesystem::equivalent(inputFilename, outputFilename)) {
                LUME_LOG_I("  %s", relativeFilename.c_str());
                std::filesystem::copy(inputFilename, outputFilename, std::filesystem::copy_options::overwrite_existing);
            }
            return true;
        } else {
            LUME_LOG_I("  %s", relativeFilename.c_str());
            outputFilename += ".spv";

            LUME_LOG_V("    input: '%s'", inputFilename.data());
            LUME_LOG_V("      dst: '%s'", settings.compiledShaderDestinationPath.string().c_str());
            LUME_LOG_V(" relative: '%s'", relativeFilename.c_str());
            LUME_LOG_V("   output: '%s'", outputFilename.string().c_str());

            if (std::string shaderSource = readFileToString(inputFilename); !shaderSource.empty()) {
                ShaderKind shaderKind;
                if (extension == ".vert") {
                    shaderKind = ShaderKind::VERTEX;
                } else if (extension == ".frag") {
                    shaderKind = ShaderKind::FRAGMENT;
                } else if (extension == ".comp") {
                    shaderKind = ShaderKind::COMPUTE;
                } else {
                    return false;
                }

                if (std::string preProcessedShader =
                        preProcessShader(shaderSource, shaderKind, relativeFilename, settings);
                    !preProcessedShader.empty()) {
                    if (true) {
                        auto reflectionFile = outputFilename;
                        reflectionFile += ".pre";
                        if (!writeToFile(
                                array_view(preProcessedShader.data(), preProcessedShader.size()), reflectionFile)) {
                            LUME_LOG_E("Failed to save reflection %s", reflectionFile.string().data());
                        }
                    }

                    if (std::vector<uint32_t> spvBinary =
                            compileShaderToSpirvBinary(preProcessedShader, shaderKind, relativeFilename, settings);
                        !spvBinary.empty()) {
                        const auto reflection = reflectSpvBinary(spvBinary, shaderKind);
                        if (reflection.empty()) {
                            LUME_LOG_E("Failed to reflect %s", inputFilename.data());
                        } else {
                            auto reflectionFile = outputFilename;
                            reflectionFile += ".lsb";
                            if (!writeToFile(array_view(reflection.data(), reflection.size()), reflectionFile)) {
                                LUME_LOG_E("Failed to save reflection %s", reflectionFile.string().data());
                            }
                        }

                        if (settings.optimizer) {
                            // spirv-opt resets the passes everytime so then need to be setup
                            settings.optimizer->RegisterPerformancePasses();
                            if (!settings.optimizer->Run(spvBinary.data(), spvBinary.size(), &spvBinary)) {
                                LUME_LOG_E("Failed to optimize %s", inputFilename.data());
                            }
                        }

                        if (writeToFile(array_view(spvBinary.data(), spvBinary.size()), outputFilename)) {
                            LUME_LOG_D("  -> %s", outputFilename.string().c_str());
                            auto glFile = outputFilename;
                            glFile += ".gl";
                            try {
                                bool multiviewEnabled = false;
                                if (shaderKind == ShaderKind::VERTEX) {
                                    static constexpr const std::string_view multiview = "GL_EXT_multiview";
                                    for (auto pos = shaderSource.find(multiview); pos != std::string::npos;
                                         pos = shaderSource.find(multiview, pos + multiview.size())) {
                                        if ((shaderSource.rfind("#extension", pos) != std::string::npos) &&
                                            (shaderSource.find("enabled", pos + multiview.size()) !=
                                                std::string::npos)) {
                                            multiviewEnabled = true;
                                            break;
                                        }
                                    }
                                }
                                Shader shader;
                                shader.shaderStageFlags_ =
                                    shaderKind == ShaderKind::VERTEX
                                        ? ShaderStageFlagBits::VERTEX_BIT
                                        : (shaderKind == ShaderKind::FRAGMENT ? ShaderStageFlagBits::FRAGMENT_BIT
                                                                              : ShaderStageFlagBits::COMPUTE_BIT);

                                shader.backend_ = DeviceBackendType::OPENGL;
                                shader.ovrEnabled = multiviewEnabled;
                                ShaderModuleCreateInfo info;
                                info.shaderStageFlags =
                                    shaderKind == ShaderKind::VERTEX
                                        ? ShaderStageFlagBits::VERTEX_BIT
                                        : (shaderKind == ShaderKind::FRAGMENT ? ShaderStageFlagBits::FRAGMENT_BIT
                                                                              : ShaderStageFlagBits::COMPUTE_BIT);
                                info.spvData =
                                    array_view(static_cast<const uint8_t*>(static_cast<const void*>(spvBinary.data())),
                                        spvBinary.size() * sizeof(decltype(spvBinary)::value_type));
                                info.reflectionData.reflectionData =
                                    array_view(static_cast<const uint8_t*>(static_cast<const void*>(reflection.data())),
                                        reflection.size() * sizeof(decltype(reflection)::value_type));
                                ProcessShaderModule(shader, info);
                                writeToFile(array_view(static_cast<const uint8_t*>(
                                                           static_cast<const void*>(shader.source_.data())),
                                                shader.source_.size()),
                                    glFile);
                            } catch (std::exception const& e) {
                                LUME_LOG_E("Failed to generate GL shader for '%s': %s", inputFilename.data(), e.what());
                            }

                            auto glesFile = glFile;
                            glesFile += "es";
                            try {
                                bool multiviewEnabled = false;
                                if (shaderKind == ShaderKind::VERTEX) {
                                    static constexpr const std::string_view multiview = "GL_EXT_multiview";
                                    for (auto pos = shaderSource.find(multiview); pos != std::string::npos;
                                         pos = shaderSource.find(multiview, pos + multiview.size())) {
                                        if ((shaderSource.rfind("#extension", pos) != std::string::npos) &&
                                            (shaderSource.find("enabled", pos + multiview.size()) !=
                                                std::string::npos)) {
                                            multiviewEnabled = true;
                                            break;
                                        }
                                    }
                                }
                                Shader shader;
                                shader.shaderStageFlags_ =
                                    shaderKind == ShaderKind::VERTEX
                                        ? ShaderStageFlagBits::VERTEX_BIT
                                        : (shaderKind == ShaderKind::FRAGMENT ? ShaderStageFlagBits::FRAGMENT_BIT
                                                                              : ShaderStageFlagBits::COMPUTE_BIT);

                                shader.backend_ = DeviceBackendType::OPENGLES;
                                shader.ovrEnabled = multiviewEnabled;
                                ShaderModuleCreateInfo info;
                                info.shaderStageFlags =
                                    shaderKind == ShaderKind::VERTEX
                                        ? ShaderStageFlagBits::VERTEX_BIT
                                        : (shaderKind == ShaderKind::FRAGMENT ? ShaderStageFlagBits::FRAGMENT_BIT
                                                                              : ShaderStageFlagBits::COMPUTE_BIT);
                                info.spvData =
                                    array_view(static_cast<const uint8_t*>(static_cast<const void*>(spvBinary.data())),
                                        spvBinary.size() * sizeof(decltype(spvBinary)::value_type));
                                info.reflectionData.reflectionData =
                                    array_view(static_cast<const uint8_t*>(static_cast<const void*>(reflection.data())),
                                        reflection.size() * sizeof(decltype(reflection)::value_type));
                                ProcessShaderModule(shader, info);
                                writeToFile(array_view(static_cast<const uint8_t*>(
                                                           static_cast<const void*>(shader.source_.data())),
                                                shader.source_.size()),
                                    glesFile);
                            } catch (std::exception const& e) {
                                LUME_LOG_E(
                                    "Failed to generate GLES shader for '%s': %s", inputFilename.data(), e.what());
                            }

                            return true;
                        }
                    }
                }
            }
        }
    } catch (std::exception const& e) {
        LUME_LOG_E("Processing file failed '%s': %s", inputFilename.data(), e.what());
    }
    return false;
}

void show_usage()
{
    std::cout << "LumeShaderCompiler - Supported shader types: vertex (.vert), fragment (.frag), compute (.comp)"
              << std::endl
              << std::endl;
    std::cout << "How to use: \n"
                 "LumeShaderCompiler.exe --source <source path> (sets destination path to same as source)\n"
                 "LumeShaderCompiler.exe --source <source path> --destination <destination path>\n"
                 "LumeShaderCompiler.exe --monitor (monitors changes in the source files)"
              << std::endl;
}

std::vector<std::string> filterByExtension(
    const std::vector<std::string>& aFilenames, const std::vector<std::string_view>& aIncludeExtensions)
{
    std::vector<std::string> filtered;
    for (auto const& file : aFilenames) {
        std::string lowercaseFileExt = std::filesystem::path(file).extension().string();
        std::transform(lowercaseFileExt.begin(), lowercaseFileExt.end(), lowercaseFileExt.begin(), tolower);

        for (auto const& extension : aIncludeExtensions) {
            if (lowercaseFileExt == extension) {
                filtered.push_back(file);
                break;
            }
        }
    }

    return filtered;
}

int main(int argc, char* argv[])
{
    if (argc == 1) {
        show_usage();
        return 0;
    }

    std::filesystem::path const currentFolder = std::filesystem::current_path();
    std::filesystem::path shaderSourcesPath = currentFolder;
    std::filesystem::path compiledShaderDestinationPath;
    std::vector<std::filesystem::path> shaderIncludePaths;
    std::filesystem::path sourceFile;

    bool monitorChanges = false;
    bool optimizeSpirv = false;
    ShaderEnv envVersion = ShaderEnv::version_vulkan_1_0;
    for (int i = 1; i < argc; ++i) {
        const auto arg = std::string_view(argv[i]);
        if (arg == "--help") {
            show_usage();
            return 0;
        } else if (arg == "--sourceFile") {
            if (i + 1 < argc) {
                sourceFile = argv[++i];
                sourceFile.make_preferred();
                shaderSourcesPath = sourceFile;
                shaderSourcesPath.remove_filename();
                if (compiledShaderDestinationPath.empty()) {
                    compiledShaderDestinationPath = shaderSourcesPath;
                }
            } else {
                LUME_LOG_E("--sourceFile option requires one argument.\n");
                return 1;
            }
        } else if (arg == "--source") {
            if (i + 1 < argc) {
                shaderSourcesPath = argv[++i];
                shaderSourcesPath.make_preferred();
                if (compiledShaderDestinationPath.empty()) {
                    compiledShaderDestinationPath = shaderSourcesPath;
                }
            } else {
                LUME_LOG_E("--source option requires one argument.");
                return 1;
            }
        } else if (arg == "--destination") {
            if (i + 1 < argc) {
                compiledShaderDestinationPath = argv[++i];
                compiledShaderDestinationPath.make_preferred();
            } else {
                LUME_LOG_E("--destination option requires one argument.");
                return 1;
            }
        } else if (arg == "--include") {
            if (i + 1 < argc) {
                shaderIncludePaths.emplace_back(argv[++i]).make_preferred();
            } else {
                LUME_LOG_E("--include option requires one argument.");
                return 1;
            }

        } else if (arg == "--monitor") {
            monitorChanges = true;
        } else if (arg == "--optimize") {
            optimizeSpirv = true;
        } else if (arg == "--vulkan") {
            if (i + 1 < argc) {
                const auto version = std::string_view(argv[++i]);
                if (version == "1.0") {
                    envVersion = ShaderEnv::version_vulkan_1_0;
                } else if (version == "1.1") {
                    envVersion = ShaderEnv::version_vulkan_1_1;
                } else if (version == "1.2") {
                    envVersion = ShaderEnv::version_vulkan_1_2;
#ifdef GLSLANG_VERSION >= GLSLANG_VERSION_12_2_0
                } else if (version == "1.3") {
                    envVersion = ShaderEnv::version_vulkan_1_3;
#endif
                } else {
                    LUME_LOG_E("Invalid argument for option --vulkan.");
                    return 1;
                }
            } else {
                LUME_LOG_E("--vulkan option requires one argument.");
                return 1;
            }
        }
    }

    if (compiledShaderDestinationPath.empty()) {
        compiledShaderDestinationPath = currentFolder;
    }

    ige::FileMonitor fileMonitor;

    if (!std::filesystem::exists(shaderSourcesPath)) {
        LUME_LOG_E("Source path does not exist: '%s'", shaderSourcesPath.string().c_str());
        return 1;
    }

    // Make sure the destination dir exists.
    std::filesystem::create_directories(compiledShaderDestinationPath);

    if (!std::filesystem::exists(compiledShaderDestinationPath)) {
        LUME_LOG_E("Destination path does not exist: '%s'", compiledShaderDestinationPath.string().c_str());
        return 1;
    }

    fileMonitor.addPath(shaderSourcesPath.string());
    std::vector<std::string> fileList = [&]() {
        std::vector<std::string> list;
        if (!sourceFile.empty()) {
            list.push_back(sourceFile.u8string());
        } else {
            list = fileMonitor.getMonitoredFiles();
        }
        return list;
    }();

    const std::vector<std::string_view> supportedFileTypes = { ".vert", ".frag", ".comp", ".json" };
    fileList = filterByExtension(fileList, supportedFileTypes);

    LUME_LOG_I("     Source path: '%s'", std::filesystem::absolute(shaderSourcesPath).string().c_str());
    for (auto const& path : shaderIncludePaths) {
        LUME_LOG_I("    Include path: '%s'", std::filesystem::absolute(path).string().c_str());
    }
    LUME_LOG_I("Destination path: '%s'", std::filesystem::absolute(compiledShaderDestinationPath).string().c_str());
    LUME_LOG_I("");
    LUME_LOG_I("Processing:");

    int errorCount = 0;
    scope scope([]() { glslang::InitializeProcess(); }, []() { glslang::FinalizeProcess(); });

    std::vector<std::filesystem::path> searchPath;
    searchPath.reserve(searchPath.size() + 1 + shaderIncludePaths.size());
    searchPath.emplace_back(shaderSourcesPath.string());
    for (auto const& path : shaderIncludePaths) {
        searchPath.emplace_back(path.string());
    }

    auto settings =
        CompilationSettings { envVersion, searchPath, {}, shaderSourcesPath, compiledShaderDestinationPath };

    if (optimizeSpirv) {
        spv_target_env targetEnv = spv_target_env::SPV_ENV_VULKAN_1_0;
        switch (envVersion) {
            case ShaderEnv::version_vulkan_1_0:
                targetEnv = spv_target_env::SPV_ENV_VULKAN_1_0;
                break;
            case ShaderEnv::version_vulkan_1_1:
                targetEnv = spv_target_env::SPV_ENV_VULKAN_1_1;
                break;
            case ShaderEnv::version_vulkan_1_2:
                targetEnv = spv_target_env::SPV_ENV_VULKAN_1_2;
                break;
            case ShaderEnv::version_vulkan_1_3:
                targetEnv = spv_target_env::SPV_ENV_VULKAN_1_3;
                break;
            default:
                break;
        }
        settings.optimizer.emplace(targetEnv);
    }

    // Startup compilation.
    for (auto const& file : fileList) {
        std::string relativeFilename = std::filesystem::relative(file, shaderSourcesPath).string();
        LUME_LOG_D("Tracked source file: '%s'", relativeFilename.c_str());
        if (!runAllCompilationStages(file, settings)) {
            errorCount++;
        }
    }

    if (errorCount == 0) {
        LUME_LOG_I("Success.");
    } else {
        LUME_LOG_E("Failed: %d", errorCount);
    }

    if (monitorChanges) {
        LUME_LOG_I("Monitoring file changes.");
    }

    // Main loop.
    while (monitorChanges) {
        std::vector<std::string> addedFiles, removedFiles, modifiedFiles;
        fileMonitor.scanModifications(addedFiles, removedFiles, modifiedFiles);
        modifiedFiles = filterByExtension(modifiedFiles, supportedFileTypes);

        if (sourceFile.empty()) {
            addedFiles = filterByExtension(addedFiles, supportedFileTypes);
            removedFiles = filterByExtension(removedFiles, supportedFileTypes);

            if (!addedFiles.empty()) {
                LUME_LOG_I("Files added:");
                for (auto const& addedFile : addedFiles) {
                    runAllCompilationStages(addedFile, settings);
                }
            }

            if (!removedFiles.empty()) {
                LUME_LOG_I("Files removed:");
                for (auto const& removedFile : removedFiles) {
                    std::string relativeFilename = std::filesystem::relative(removedFile, shaderSourcesPath).string();
                    LUME_LOG_I("  %s", relativeFilename.c_str());
                }
            }

            if (!modifiedFiles.empty()) {
                LUME_LOG_I("Files modified:");
                for (auto const& modifiedFile : modifiedFiles) {
                    runAllCompilationStages(modifiedFile, settings);
                }
            }
        } else if (!modifiedFiles.empty()) {
            if (auto pos = std::find_if(modifiedFiles.cbegin(), modifiedFiles.cend(),
                    [&sourceFile](const std::string& modified) { return modified == sourceFile; });
                pos != modifiedFiles.cend()) {
                runAllCompilationStages(*pos, settings);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return errorCount;
}