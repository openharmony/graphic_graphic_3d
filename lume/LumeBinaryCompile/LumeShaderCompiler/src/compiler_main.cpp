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

// third party headers
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvTools.h>
#include <glslang/Public/ShaderLang.h>
#include <spirv-tools/optimizer.hpp>

// standard library
#include <algorithm>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

// internal
#include "array_view.h"
#include "default_limits.h"
#include "io/dev/FileMonitor.h"
#include "lume/Log.h"
#include "shader_type.h"
#include "spirv_cross.hpp"
#include "spirv_cross_helpers_gles.h"
#include "spirv_opt_extensions.h"

namespace {
constexpr int GLSL_VERSION = 110;

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
    ASTC_4X4_UNORM_BLOCK = 157,
    /** ASTC 4x4 SRGB BLOCK */
    ASTC_4X4_SRGB_BLOCK = 158,
    /** ASTC 5x4 UNORM BLOCK */
    ASTC_5X4_UNORM_BLOCK = 159,
    /** ASTC 5x4 SRGB BLOCK */
    ASTC_5X4_SRGB_BLOCK = 160,
    /** ASTC 5x5 UNORM BLOCK */
    ASTC_5X5_UNORM_BLOCK = 161,
    /** ASTC 5x5 SRGB BLOCK */
    ASTC_5X5_SRGB_BLOCK = 162,
    /** ASTC 6x5 UNORM BLOCK */
    ASTC_6X5_UNORM_BLOCK = 163,
    /** ASTC 6x5 SRGB BLOCK */
    ASTC_6X5_SRGB_BLOCK = 164,
    /** ASTC 6x6 UNORM BLOCK */
    ASTC_6X6_UNORM_BLOCK = 165,
    /** ASTC 6x6 SRGB BLOCK */
    ASTC_6X6_SRGB_BLOCK = 166,
    /** ASTC 8x5 UNORM BLOCK */
    ASTC_8X5_UNORM_BLOCK = 167,
    /** ASTC 8x5 SRGB BLOCK */
    ASTC_8X5_SRGB_BLOCK = 168,
    /** ASTC 8x6 UNORM BLOCK */
    ASTC_8X6_UNORM_BLOCK = 169,
    /** ASTC 8x6 SRGB BLOCK */
    ASTC_8X6_SRGB_BLOCK = 170,
    /** ASTC 8x8 UNORM BLOCK */
    ASTC_8X8_UNORM_BLOCK = 171,
    /** ASTC 8x8 SRGB BLOCK */
    ASTC_8X8_SRGB_BLOCK = 172,
    /** ASTC 10x5 UNORM BLOCK */
    ASTC_10X5_UNORM_BLOCK = 173,
    /** ASTC 10x5 SRGB BLOCK */
    ASTC_10X5_SRGB_BLOCK = 174,
    /** ASTC 10x6 UNORM BLOCK */
    ASTC_10X6_UNORM_BLOCK = 175,
    /** ASTC 10x6 SRGB BLOCK */
    ASTC_10X6_SRGB_BLOCK = 176,
    /** ASTC 10x8 UNORM BLOCK */
    ASTC_10X8_UNORM_BLOCK = 177,
    /** ASTC 10x8 SRGB BLOCK */
    ASTC_10X8_SRGB_BLOCK = 178,
    /** ASTC 10x10 UNORM BLOCK */
    ASTC_10X10_UNORM_BLOCK = 179,
    /** ASTC 10x10 SRGB BLOCK */
    ASTC_10X10_SRGB_BLOCK = 180,
    /** ASTC 12x10 UNORM BLOCK */
    ASTC_12X10_UNORM_BLOCK = 181,
    /** ASTC 12x10 SRGB BLOCK */
    ASTC_12X10_SRGB_BLOCK = 182,
    /** ASTC 12x12 UNORM BLOCK */
    ASTC_12X12_UNORM_BLOCK = 183,
    /** ASTC 12x12 SRGB BLOCK */
    ASTC_12X12_SRGB_BLOCK = 184,
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
namespace PipelineLayoutConstants {
/** Max descriptor set count */
static constexpr uint32_t MAX_DESCRIPTOR_SET_COUNT{ 4u };
/** Invalid index */
static constexpr uint32_t INVALID_INDEX{ ~0u };
} // namespace PipelineLayoutConstants

enum class ImageDimension : uint8_t {
    DIMENSION_1D = 0,
    DIMENSION_2D = 1,
    DIMENSION_3D = 2,
    DIMENSION_CUBE = 3,
    DIMENSION_RECT = 4,
    DIMENSION_BUFFER = 5,
    DIMENSION_SUBPASS = 6,
};

enum ImageFlags {
    IMAGE_DEPTH = 0b00000001,
    IMAGE_ARRAY = 0b00000010,
    IMAGE_MULTISAMPLE = 0b00000100,
    IMAGE_SAMPLED = 0b00001000,
    IMAGE_LOAD_STORE = 0b00010000,
};

static_assert(int(ImageDimension::DIMENSION_1D) == spv::Dim::Dim1D);
static_assert(int(ImageDimension::DIMENSION_CUBE) == spv::Dim::DimCube);
static_assert(int(ImageDimension::DIMENSION_SUBPASS) == spv::Dim::DimSubpassData);

/** Descriptor set layout binding */
struct DescriptorSetLayoutBinding {
    /** Binding */
    uint32_t binding{ PipelineLayoutConstants::INVALID_INDEX };
    /** Descriptor type */
    DescriptorType descriptorType{ DescriptorType::MAX_ENUM };
    /** Descriptor count */
    uint32_t descriptorCount{ 0u };
    /** Stage flags */
    ShaderStageFlags shaderStageFlags;
    ImageDimension imageDimension{ ImageDimension::DIMENSION_1D };
    uint8_t imageFlags;
};

/** Descriptor set layout */
struct DescriptorSetLayout {
    /** Set */
    uint32_t set{ PipelineLayoutConstants::INVALID_INDEX };
    /** Bindings */
    std::vector<DescriptorSetLayoutBinding> bindings;
};

/** Push constant */
struct PushConstant {
    /** Shader stage flags */
    ShaderStageFlags shaderStageFlags;
    /** Byte size */
    uint32_t byteSize{ 0u };
};

/** Pipeline layout */
struct PipelineLayout {
    /** Push constant */
    PushConstant pushConstant;
    /** Descriptor set count */
    uint32_t descriptorSetCount{ 0u };
    /** Descriptor sets */
    DescriptorSetLayout descriptorSetLayouts[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};
};

constexpr const uint32_t RESERVED_CONSTANT_ID_INDEX{ 256u };

/** Vertex input attribute description */
struct VertexInputAttributeDescription {
    /** Location */
    uint32_t location{ ~0u };
    /** Binding */
    uint32_t binding{ ~0u };
    /** Format */
    Format format{ Format::UNDEFINED };
    /** Offset */
    uint32_t offset{ 0u };
};

struct VertexAttributeInfo {
    uint32_t byteSize{ 0u };
    VertexInputAttributeDescription description;
};

struct UVec3 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct ShaderReflectionData {
    array_view<const uint8_t> reflectionData;
};

struct ShaderModuleCreateInfo {
    ShaderStageFlags shaderStageFlags;
    array_view<const uint32_t> spvData;
    ShaderReflectionData reflectionData;
};

class FileIncluder : public glslang::TShader::Includer {
public:
    struct Data {
        std::filesystem::file_time_type modicationTime;
        std::string data;
    };

    FileIncluder(
        const std::filesystem::path& shaderSourcePath, array_view<const std::filesystem::path> shaderIncludePaths) :
        shaderSourcePath_(shaderSourcePath), shaderIncludePaths_(shaderIncludePaths)
    {}

    IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override
    {
        return include(headerName, includerName, inclusionDepth, false);
    }

    IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override
    {
        return include(headerName, includerName, inclusionDepth, true);
    }

    void releaseInclude(IncludeResult* include) override
    {
        delete include;
    }

    void Reset()
    {
        data_.clear();
    }

    std::map<std::string, Data> Files()
    {
        return std::map<std::string, Data>(data_.cbegin(), data_.cend());
    }

private:
    IncludeResult* SearchExistingIncludes(
        const char* headerName, const char* includerName, size_t inclusionDepth, const bool relative)
    {
        std::filesystem::path path;
        if (relative) {
            path.assign(shaderSourcePath_);
            path /= std::filesystem::u8path(includerName);
            path = path.parent_path();
            path /= std::filesystem::u8path(headerName);
            const auto pathAsString = path.make_preferred().u8string();
            if (const auto pos = data_.find(pathAsString); pos != data_.end()) {
                return new (std::nothrow)
                    IncludeResult(pathAsString, pos->second.data.data(), pos->second.data.size(), nullptr);
            }
        }
        for (const auto& includePath : shaderIncludePaths_) {
            path.assign(includePath);
            path /= std::filesystem::u8path(headerName);
            const auto pathAsString = path.make_preferred().u8string();
            if (auto pos = data_.find(pathAsString); pos != data_.end()) {
                return new (std::nothrow)
                    IncludeResult(pathAsString, pos->second.data.data(), pos->second.data.size(), nullptr);
            }
        }
        return nullptr;
    }

    IncludeResult* include(
        const char* headerName, const char* includerName, const size_t inclusionDepth, const bool relative)
    {
        IncludeResult* result = SearchExistingIncludes(headerName, includerName, inclusionDepth, relative);
        if (result) {
            return result;
        }

        std::filesystem::path path;
        bool found = false;
        if (relative) {
            path.assign(shaderSourcePath_);
            path /= std::filesystem::u8path(includerName);
            path = path.parent_path();
            path /= std::filesystem::u8path(headerName);
            found = std::filesystem::exists(path);
        }
        if (!found) {
            for (const auto& includePath : shaderIncludePaths_) {
                path.assign(includePath);
                path /= std::filesystem::u8path(headerName);
                found = std::filesystem::exists(path);
                if (found) {
                    break;
                }
            }
        }
        if (found) {
            const auto pathAsString = path.make_preferred().u8string();
            auto& headerData = data_[pathAsString];
            const auto length = std::filesystem::file_size(path);
            headerData.data.resize(length);
            headerData.modicationTime = std::filesystem::last_write_time(path);
            std::ifstream(path, std::ios_base::binary)
                .read(headerData.data.data(), static_cast<std::streamsize>(length));

            return new (std::nothrow)
                IncludeResult(pathAsString, headerData.data.data(), headerData.data.size(), nullptr);
        }
        return nullptr;
    }

    const std::filesystem::path& shaderSourcePath_;
    const array_view<const std::filesystem::path> shaderIncludePaths_;
    std::unordered_map<std::string, Data> data_;
};

struct CompilationSettings {
    ShaderEnv shaderEnv;
    std::vector<std::filesystem::path> shaderIncludePaths;
    std::optional<spvtools::Optimizer> optimizer;
    const std::filesystem::path& shaderSourcePath;
    const std::filesystem::path& compiledShaderDestinationPath;
    FileIncluder& includer;
};

constexpr uint8_t REFLECTION_TAG[] = { 'r', 'f', 'l', 1 }; // last one is version
struct ReflectionHeader {
    uint8_t tag[sizeof(REFLECTION_TAG)];
    uint16_t type;
    uint16_t offsetPushConstants;
    uint16_t offsetSpecializationConstants;
    uint16_t offsetDescriptorSets;
    uint16_t offsetInputs;
    uint16_t offsetLocalSize;
};

struct Inputs {
    std::filesystem::path shaderSourcesPath;
    std::filesystem::path compiledShaderDestinationPath;
    std::filesystem::path sourceFile;
    std::vector<std::filesystem::path> shaderIncludePaths;
    bool monitorChanges = false;
    bool optimizeSpirv = false;
    bool checkIfChanged = false;
    bool stripDebugInformation = false;
    ShaderEnv envVersion = ShaderEnv::version_vulkan_1_0;
};

template<typename InitFun, typename DeinitFun>
class Scope {
private:
    InitFun* init_;
    DeinitFun* deinit_;

public:
    Scope(InitFun&& initializer, DeinitFun&& deinitalizer) : init_(initializer), deinit_(deinitalizer)
    {
        init_();
    }

    ~Scope()
    {
        deinit_();
    }
};

std::optional<std::string> ReadFileToString(const std::string_view aFilename)
{
    try {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        file.open(std::filesystem::u8path(aFilename), std::ios::in);

        if (file.fail()) {
            LUME_LOG_E("Error opening file %s:", std::string{ aFilename }.c_str());
            return std::nullopt;
        }

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    } catch (std::exception const& ex) {
        LUME_LOG_E("Error reading file: '%s': %s", std::string{ aFilename }.c_str(), ex.what());
        return std::nullopt;
    }
}

glslang::EShTargetLanguageVersion ToSpirVVersion(glslang::EShTargetClientVersion env_version)
{
    if (env_version == glslang::EShTargetVulkan_1_0) {
        return glslang::EShTargetSpv_1_0;
    }
    if (env_version == glslang::EShTargetVulkan_1_1) {
        return glslang::EShTargetSpv_1_3;
    }
    if (env_version == glslang::EShTargetVulkan_1_2) {
        return glslang::EShTargetSpv_1_5;
    }
#if GLSLANG_VERSION >= GLSLANG_VERSION_12_2_0
    if (env_version == glslang::EShTargetVulkan_1_3) {
        return glslang::EShTargetSpv_1_6;
    }
#endif
    return glslang::EShTargetSpv_1_0;
}

std::optional<EShLanguage> ConvertShaderKind(ShaderKind kind)
{
    switch (kind) {
        case ShaderKind::VERTEX:
            return EShLanguage::EShLangVertex;
        case ShaderKind::FRAGMENT:
            return EShLanguage::EShLangFragment;
        case ShaderKind::COMPUTE:
            return EShLanguage::EShLangCompute;
        default:
            return std::nullopt;
    }
}

std::optional<glslang::EShTargetClientVersion> ConvertShaderEnv(ShaderEnv shaderEnv)
{
    switch (shaderEnv) {
        case ShaderEnv::version_vulkan_1_0:
            return glslang::EShTargetClientVersion::EShTargetVulkan_1_0;
        case ShaderEnv::version_vulkan_1_1:
            return glslang::EShTargetClientVersion::EShTargetVulkan_1_1;
        case ShaderEnv::version_vulkan_1_2:
            return glslang::EShTargetClientVersion::EShTargetVulkan_1_2;
#if GLSLANG_VERSION >= GLSLANG_VERSION_12_2_0
        case ShaderEnv::version_vulkan_1_3:
            return glslang::EShTargetClientVersion::EShTargetVulkan_1_3;
#endif
        default:
            return std::nullopt;
    }
}

// setup values which are common for preproccessing and compilation
void CommonShaderInit(glslang::TShader& shader, glslang::EShTargetClientVersion version,
    glslang::EShTargetLanguageVersion languageVersion)
{
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
}

std::optional<std::string> PreProcessShader(const std::string_view source, const ShaderKind kind,
    const std::string_view sourceName, const CompilationSettings& settings)
{
    const std::optional<EShLanguage> stage = ConvertShaderKind(kind);
    if (!stage) {
        LUME_LOG_E("Spirv preprocessing failed '%s'", "ShaderKind not recognized");
        return std::nullopt;
    }

    const std::optional<glslang::EShTargetClientVersion> version = ConvertShaderEnv(settings.shaderEnv);
    if (!version) {
        LUME_LOG_E("Spirv preprocessing failed '%s'", "ShaderEnv not recognized");
        return std::nullopt;
    }

    const glslang::EShTargetLanguageVersion languageVersion = ToSpirVVersion(version.value());

    glslang::TShader shader(stage.value());
    CommonShaderInit(shader, version.value(), languageVersion);

    const char* shaderStrings = source.data();
    const int shaderLengths = static_cast<int>(source.size());
    const char* stringNames = sourceName.data();
    static constexpr std::string_view preamble = "#extension GL_GOOGLE_include_directive : enable\n";
    shader.setStringsWithLengthsAndNames(&shaderStrings, &shaderLengths, &stringNames, 1);
    shader.setPreamble(preamble.data());

    constexpr auto rules =
        static_cast<EShMessages>(EShMsgOnlyPreprocessor | EShMsgSpvRules | EShMsgVulkanRules | EShMsgCascadingErrors);

    std::string output;
    if (!shader.preprocess(&kGLSLangDefaultTResource, GLSL_VERSION, EProfile::ENoProfile, false, false, rules, &output,
            settings.includer)) {
        LUME_LOG_E("Spirv preprocessing of '%s' failed - info log:\n%s", sourceName.data(), shader.getInfoLog());
        LUME_LOG_E("Spirv preprocessing of '%s' failed - debug log: \n%s", sourceName.data(), shader.getInfoDebugLog());
        return std::nullopt;
    }

    output.erase(0u, preamble.size());
    return output;
}

std::optional<std::vector<uint32_t>> CompileShaderToSpirvBinary(
    std::string_view source, ShaderKind kind, std::string_view sourceName, const CompilationSettings& settings)
{
    const std::optional<EShLanguage> stage = ConvertShaderKind(kind);
    if (!stage) {
        LUME_LOG_E("Spirv binary compilation failed '%s'", "ShaderKind not recognized");
        return std::nullopt;
    }

    const std::optional<glslang::EShTargetClientVersion> version = ConvertShaderEnv(settings.shaderEnv);
    if (!version) {
        LUME_LOG_E("Spirv binary compilation failed '%s'", "ShaderEnv not recognized");
        return std::nullopt;
    }

    const glslang::EShTargetLanguageVersion languageVersion = ToSpirVVersion(version.value());

    glslang::TShader shader(stage.value());
    CommonShaderInit(shader, version.value(), languageVersion);

    const char* shaderStrings = source.data();
    const int shaderLengths = static_cast<int>(source.size());
    const char* stringNames = sourceName.data();
    shader.setStringsWithLengthsAndNames(&shaderStrings, &shaderLengths, &stringNames, 1);

    static constexpr std::string_view preamble = "#extension GL_GOOGLE_include_directive : enable\n";
    shader.setPreamble(preamble.data());

    constexpr auto rules = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgCascadingErrors);
    if (!shader.parse(&kGLSLangDefaultTResource, GLSL_VERSION, EProfile::ENoProfile, false, false, rules)) {
        const char* infoLog = shader.getInfoLog();
        const char* debugInfoLog = shader.getInfoDebugLog();
        lume::GetLogger().Write(lume::ILogger::LogLevel::ERROR, __FILE__, __LINE__, infoLog);
        lume::GetLogger().Write(lume::ILogger::LogLevel::ERROR, __FILE__, __LINE__, debugInfoLog);
        LUME_LOG_E("Spirv binary compilation failed while parsing '%s'", sourceName.data());
        return std::nullopt;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault) || !program.mapIO()) {
        const char* infoLog = shader.getInfoLog();
        const char* debugInfoLog = shader.getInfoDebugLog();
        lume::GetLogger().Write(lume::ILogger::LogLevel::ERROR, __FILE__, __LINE__, infoLog);
        lume::GetLogger().Write(lume::ILogger::LogLevel::ERROR, __FILE__, __LINE__, debugInfoLog);
        LUME_LOG_E("Spirv binary compilation failed while linking '%s'", sourceName.data());
        return {};
    }

    std::vector<unsigned int> spirv;
    glslang::SpvOptions spv_options;
    spv_options.generateDebugInfo = false;
    spv_options.disableOptimizer = true;
    spv_options.optimizeSize = false;
    spv::SpvBuildLogger logger;
    glslang::TIntermediate* intermediate = program.getIntermediate(stage.value());
    glslang::GlslangToSpv(*intermediate, spirv, &logger, &spv_options);

    constexpr uint32_t shadercGeneratorWord = 13; // From SPIR-V XML Registry
    constexpr uint32_t generatorWordIndex = 2;    // SPIR-V 2.3: Physical layout
    assert(spirv.size() > generatorWordIndex);
    spirv[generatorWordIndex] = (spirv[generatorWordIndex] & 0xffff) | (shadercGeneratorWord << 16u);
    return spirv;
}

void ProcessResource(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource,
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
    const auto pos = std::find_if(bindings.begin(), bindings.end(),
        [bindingIndex](const DescriptorSetLayoutBinding& binding) { return binding.binding == bindingIndex; });
    if (pos == bindings.end()) {
        const spirv_cross::SPIRType& spirType = compiler.get_type(resource.type_id);

        DescriptorSetLayoutBinding binding;
        binding.binding = bindingIndex;
        binding.descriptorType = type;
        binding.descriptorCount = spirType.array.empty() ? 1 : spirType.array[0];
        binding.shaderStageFlags = shaderStateFlags;
        binding.imageDimension = ImageDimension(0);
        binding.imageFlags = 0;
        if (spirType.basetype == spirv_cross::SPIRType::BaseType::Image ||
            spirv_cross::SPIRType::BaseType::SampledImage) {
            binding.imageDimension = ImageDimension(spirType.image.dim);
            binding.imageFlags = 0;
            if (spirType.image.depth) {
                binding.imageFlags |= ImageFlags::IMAGE_DEPTH;
            }
            if (spirType.image.arrayed) {
                binding.imageFlags |= ImageFlags::IMAGE_ARRAY;
            }
            if (spirType.image.ms) {
                binding.imageFlags |= ImageFlags::IMAGE_MULTISAMPLE;
            }
            if (spirType.image.sampled == 1) {
                binding.imageFlags |= ImageFlags::IMAGE_SAMPLED;
            } else if (spirType.image.sampled == 2) {
                binding.imageFlags |= ImageFlags::IMAGE_LOAD_STORE;
            }
        }
        bindings.emplace_back(binding);
    } else {
        pos->shaderStageFlags |= shaderStateFlags;
    }
}

void ReflectDescriptorSets(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
    ShaderStageFlags shaderStateFlags, DescriptorSetLayout* layouts)
{
    for (const auto& ref : resources.sampled_images) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::COMBINED_IMAGE_SAMPLER, layouts);
    }

    for (const auto& ref : resources.separate_samplers) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::SAMPLER, layouts);
    }

    for (const auto& ref : resources.separate_images) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::SAMPLED_IMAGE, layouts);
    }

    for (const auto& ref : resources.storage_images) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::STORAGE_IMAGE, layouts);
    }

    for (const auto& ref : resources.uniform_buffers) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::UNIFORM_BUFFER, layouts);
    }

    for (const auto& ref : resources.storage_buffers) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::STORAGE_BUFFER, layouts);
    }

    for (const auto& ref : resources.subpass_inputs) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::INPUT_ATTACHMENT, layouts);
    }

    for (const auto& ref : resources.acceleration_structures) {
        ProcessResource(compiler, ref, shaderStateFlags, DescriptorType::ACCELERATION_STRUCTURE, layouts);
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

void ReflectPushContants(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
    const ShaderStageFlags shaderStateFlags, PushConstant& pushConstant)
{
    // NOTE: support for only one push constant
    if (!resources.push_constant_buffers.empty()) {
        pushConstant.shaderStageFlags |= shaderStateFlags;

        const auto ranges = compiler.get_active_buffer_ranges(resources.push_constant_buffers[0].id);
        const uint32_t byteSize = std::accumulate(
            ranges.begin(), ranges.end(), 0u, [](const uint32_t byteSize, const spirv_cross::BufferRange& range) {
                return byteSize + static_cast<uint32_t>(range.range);
            });
        pushConstant.byteSize = std::max(pushConstant.byteSize, byteSize);
    }
}

std::vector<ShaderSpecializationConstant> ReflectSpecializationConstants(
    const spirv_cross::Compiler& compiler, const ShaderStageFlags shaderStateFlags)
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
                ShaderSpecializationConstant{ shaderStateFlags, constant.constant_id, constantType, offset });
            offset += size;
        }
    }
    // sorted based on offset due to offset mapping with shader combinations
    // NOTE: id and name indexing
    std::sort(specializationConstants.begin(), specializationConstants.end(),
        [](const auto& lhs, const auto& rhs) { return (lhs.offset < rhs.offset); });

    return specializationConstants;
}

Format ConvertToVertexInputFormat(const spirv_cross::SPIRType& type)
{
    using BaseType = spirv_cross::SPIRType::BaseType;

    // ivecn: a vector of signed integers
    if (type.basetype == BaseType::Int) {
        switch (type.vecsize) {
            case 1u:
                return Format::R32_SINT;
            case 2u:
                return Format::R32G32_SINT;
            case 3u:
                return Format::R32G32B32_SINT;
            case 4u:
                return Format::R32G32B32A32_SINT;
            default:
                return Format::UNDEFINED;
        }
    }

    // uvecn: a vector of unsigned integers
    if (type.basetype == BaseType::UInt) {
        switch (type.vecsize) {
            case 1u:
                return Format::R32_UINT;
            case 2u:
                return Format::R32G32_UINT;
            case 3u:
                return Format::R32G32B32_UINT;
            case 4u:
                return Format::R32G32B32A32_UINT;
            default:
                return Format::UNDEFINED;
        }
    }

    // halfn: a vector of half-precision floating-point numbers
    if (type.basetype == BaseType::Half) {
        switch (type.vecsize) {
            case 1u:
                return Format::R16_SFLOAT;
            case 2u:
                return Format::R16G16_SFLOAT;
            case 3u:
                return Format::R16G16B16_SFLOAT;
            case 4u:
                return Format::R16G16B16A16_SFLOAT;
            default:
                return Format::UNDEFINED;
        }
    }

    // vecn: a vector of single-precision floating-point numbers
    if (type.basetype == BaseType::Float) {
        switch (type.vecsize) {
            case 1u:
                return Format::R32_SFLOAT;
            case 2u:
                return Format::R32G32_SFLOAT;
            case 3u:
                return Format::R32G32B32_SFLOAT;
            case 4u:
                return Format::R32G32B32A32_SFLOAT;
            default:
                return Format::UNDEFINED;
        }
    }

    return Format::UNDEFINED;
}

std::vector<VertexInputAttributeDescription> ReflectVertexInputs(const spirv_cross::Compiler& compiler,
    const spirv_cross::ShaderResources& resources, ShaderStageFlags /* shaderStateFlags */)
{
    std::vector<VertexInputAttributeDescription> vertexInputAttributes;

    std::vector<VertexAttributeInfo> vertexAttributeInfos;
    std::transform(std::begin(resources.stage_inputs), std::end(resources.stage_inputs),
        std::back_inserter(vertexAttributeInfos), [&compiler](const spirv_cross::Resource& attr) {
            const spirv_cross::SPIRType& attributeType = compiler.get_type(attr.type_id);
            const uint32_t location = compiler.get_decoration(attr.id, spv::DecorationLocation);
            return VertexAttributeInfo{
                // width is in bits so convert to bytes
                attributeType.vecsize * (attributeType.width / 8u), // byteSize
                {
                    // For now, assume that every vertex attribute comes from it's own binding which equals the
                    location,                                  // location
                    location,                                  // binding
                    ConvertToVertexInputFormat(attributeType), // format
                    0u                                         // offset
                },                                             // description
            };
        });

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
        vertexInputAttributes.reserve(vertexAttributeInfos.size());
        std::transform(vertexAttributeInfos.cbegin(), vertexAttributeInfos.cend(),
            std::back_inserter(vertexInputAttributes),
            [](const VertexAttributeInfo& info) { return info.description; });
    }
    return vertexInputAttributes;
}

template<typename T>
void Push(std::vector<uint8_t>& buffer, T data)
{
    buffer.push_back(data & 0xffu);
    if constexpr (sizeof(T) > sizeof(uint8_t)) {
        buffer.push_back((data >> 8u) & 0xffu);
    }
    if constexpr (sizeof(T) > sizeof(uint16_t)) {
        buffer.push_back((data >> 16u) & 0xffu);
    }
    if constexpr (sizeof(T) >= sizeof(uint32_t)) {
        buffer.push_back((data >> 24u) & 0xffu);
    }
}

std::vector<Gles::PushConstantReflection> CreatePushConstantReflection(const spirv_cross::Compiler& compiler,
    const spirv_cross::ShaderResources& resources, ShaderStageFlags shaderStateFlags)
{
    std::vector<Gles::PushConstantReflection> pushConstantReflection;
    Gles::PushConstantReflection base{};
    base.stage = shaderStateFlags;

    for (auto& remap : resources.push_constant_buffers) {
        const auto& blockType = compiler.get_type(remap.base_type_id);
        base.name = compiler.get_name(remap.id);
        (void)(blockType);
        assert((blockType.basetype == spirv_cross::SPIRType::Struct) && "Push constant is not a struct!");

        Gles::ProcessStruct(compiler, base, remap.base_type_id, pushConstantReflection);
    }
    return pushConstantReflection;
}

uint16_t AppendPushConstants(std::vector<uint8_t>& reflection, uint32_t byteSize,
    array_view<const Gles::PushConstantReflection> pushConstantReflection)
{
    const auto offsetPushConstants = static_cast<uint16_t>(reflection.size());
    if (!byteSize) {
        reflection.push_back(0); // zero: no push constants
        return offsetPushConstants;
    }

    reflection.push_back(1); // one: push constants
    Push(reflection, static_cast<uint16_t>(byteSize));

    Push(reflection, static_cast<uint8_t>(pushConstantReflection.size()));
    for (const auto& refl : pushConstantReflection) {
        Push(reflection, refl.type);
        Push(reflection, static_cast<uint16_t>(refl.offset));
        Push(reflection, static_cast<uint16_t>(refl.size));
        Push(reflection, static_cast<uint16_t>(refl.arraySize));
        Push(reflection, static_cast<uint16_t>(refl.arrayStride));
        Push(reflection, static_cast<uint16_t>(refl.matrixStride));
        Push(reflection, static_cast<uint16_t>(refl.name.size()));
        reflection.insert(reflection.end(), std::begin(refl.name), std::end(refl.name));
    }

    return offsetPushConstants;
}

uint16_t AppendSpecializationConstants(
    std::vector<uint8_t>& reflection, array_view<const ShaderSpecializationConstant> specializationConstants)
{
    const auto offsetSpecializationConstants = static_cast<uint16_t>(reflection.size());
    Push(reflection, static_cast<uint32_t>(specializationConstants.size()));

    for (auto const& constant : specializationConstants) {
        Push(reflection, constant.id);
        Push(reflection, static_cast<uint32_t>(constant.type));
    }
    return offsetSpecializationConstants;
}

uint16_t AppendDescriptorSets(std::vector<uint8_t>& reflection, const PipelineLayout& pipelineLayout)
{
    const auto offsetDescriptorSets = static_cast<uint16_t>(reflection.size());

    Push(reflection, static_cast<uint16_t>(pipelineLayout.descriptorSetCount));

    const auto begin = std::begin(pipelineLayout.descriptorSetLayouts);
    const auto end = begin + pipelineLayout.descriptorSetCount;

    std::for_each(begin, end, [&reflection](const DescriptorSetLayout& layout) {
        Push(reflection, static_cast<uint16_t>(layout.set));
        Push(reflection, static_cast<uint16_t>(layout.bindings.size()));
        for (const auto& binding : layout.bindings) {
            Push(reflection, static_cast<uint16_t>(binding.binding));
            Push(reflection, static_cast<uint16_t>(binding.descriptorType));
            Push(reflection, static_cast<uint16_t>(binding.descriptorCount));
            Push(reflection, static_cast<uint8_t>(binding.imageDimension));
            Push(reflection, binding.imageFlags);
        }
    });
    return offsetDescriptorSets;
}

uint16_t AppendVertexInputAttributes(
    std::vector<uint8_t>& reflection, array_view<const VertexInputAttributeDescription> vertexInputAttributes)
{
    const auto offsetInputs = static_cast<uint16_t>(reflection.size());
    const auto size = static_cast<uint16_t>(vertexInputAttributes.size());
    Push(reflection, size);
    for (const auto& input : vertexInputAttributes) {
        Push(reflection, static_cast<uint16_t>(input.location));
        Push(reflection, static_cast<uint16_t>(input.format));
    }
    return offsetInputs;
}

uint16_t AppendExecutionLocalSize(std::vector<uint8_t>& reflection, const spirv_cross::Compiler& compiler)
{
    const auto offsetLocalSize = static_cast<uint16_t>(reflection.size());
    uint32_t size = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0u); // X = 0
    Push(reflection, size);

    size = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1u); // Y = 1
    Push(reflection, size);

    size = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2u); // Z = 2
    Push(reflection, size);
    return offsetLocalSize;
}

std::optional<std::vector<uint8_t>> ReflectSpvBinary(const std::vector<uint32_t>& aBinary, const ShaderKind kind)
{
    const spirv_cross::Compiler compiler(aBinary);

    const auto shaderStateFlags = ShaderStageFlags(kind);

    const spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    PipelineLayout pipelineLayout;
    ReflectDescriptorSets(compiler, resources, shaderStateFlags, pipelineLayout.descriptorSetLayouts);
    pipelineLayout.descriptorSetCount =
        static_cast<uint32_t>(std::count_if(std::begin(pipelineLayout.descriptorSetLayouts),
            std::end(pipelineLayout.descriptorSetLayouts), [](const DescriptorSetLayout& layout) {
                return layout.set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT;
            }));
    ReflectPushContants(compiler, resources, shaderStateFlags, pipelineLayout.pushConstant);

    // some additional information mainly for GL
    const auto pushConstantReflection = CreatePushConstantReflection(compiler, resources, shaderStateFlags);

    const auto specializationConstants = ReflectSpecializationConstants(compiler, shaderStateFlags);

    // NOTE: this is done for all although the name is 'Vertex'InputAttributes
    const auto vertexInputAttributes = ReflectVertexInputs(compiler, resources, shaderStateFlags);

    std::vector<uint8_t> reflection;
    reflection.reserve(512u);
    // tag
    reflection.insert(reflection.end(), std::begin(REFLECTION_TAG), std::end(REFLECTION_TAG));

    // shader type
    Push(reflection, static_cast<uint16_t>(shaderStateFlags.flags));

    // offsets
    // allocate size for the five offsets. data will be added after the offsets.
    reflection.resize(reflection.size() + sizeof(uint16_t) * 5u);

    // push constants
    const uint16_t offsetPushConstants =
        AppendPushConstants(reflection, pipelineLayout.pushConstant.byteSize, pushConstantReflection);

    // specialization constants
    const uint16_t offsetSpecializationConstants = AppendSpecializationConstants(reflection, specializationConstants);

    // descriptor sets
    const uint16_t offsetDescriptorSets = AppendDescriptorSets(reflection, pipelineLayout);

    // inputs
    const uint16_t offsetInputs = AppendVertexInputAttributes(reflection, vertexInputAttributes);

    // local size
    const uint16_t offsetLocalSize =
        (shaderStateFlags & ShaderStageFlagBits::COMPUTE_BIT) ? AppendExecutionLocalSize(reflection, compiler) : 0u;

    // update offsets to real values
    {
        auto ptr = reflection.data() + (sizeof(REFLECTION_TAG) + sizeof(uint16_t));
        *ptr++ = offsetPushConstants & 0xffu;
        *ptr++ = (offsetPushConstants >> 8u) & 0xffu;
        *ptr++ = offsetSpecializationConstants & 0xffu;
        *ptr++ = (offsetSpecializationConstants >> 8u) & 0xffu;
        *ptr++ = offsetDescriptorSets & 0xffu;
        *ptr++ = (offsetDescriptorSets >> 8u) & 0xffu;
        *ptr++ = offsetInputs & 0xffu;
        *ptr++ = (offsetInputs >> 8u) & 0xffu;
        *ptr++ = offsetLocalSize & 0xffu;
        *ptr++ = (offsetLocalSize >> 8u) & 0xffu;
    }

    return reflection;
}

struct Binding {
    uint8_t set;
    uint8_t bind;
};

Binding GetBinding(const Gles::CoreCompiler& compiler, const spirv_cross::ID id)
{
    const uint32_t dset = compiler.get_decoration(id, spv::Decoration::DecorationDescriptorSet);
    const uint32_t dbind = compiler.get_decoration(id, spv::Decoration::DecorationBinding);
    assert(dset < Gles::ResourceLimits::MAX_SETS);
    assert(dbind < Gles::ResourceLimits::MAX_BIND_IN_SET);
    const auto set = static_cast<uint8_t>(dset);
    const auto bind = static_cast<uint8_t>(dbind);
    return { set, bind };
}

void SortSets(PipelineLayout& pipelineLayout)
{
    pipelineLayout.descriptorSetCount = 0;
    for (auto& currSet : pipelineLayout.descriptorSetLayouts) {
        if (currSet.set != PipelineLayoutConstants::INVALID_INDEX) {
            pipelineLayout.descriptorSetCount++;

            // sort binding objects based on binding numbers
            std::sort(currSet.bindings.begin(), currSet.bindings.end(),
                [](auto const& lhs, auto const& rhs) { return (lhs.binding < rhs.binding); });
        }
    }
}

std::string StringifySetAndBinding(const uint8_t set, const uint8_t bind)
{
    std::string ret("s");
    ret.append(std::to_string(uint32_t(set)));
    ret.append("_b");
    ret.append(std::to_string(uint32_t(bind)));
    return ret;
}

void Collect(Gles::CoreCompiler& compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    const uint32_t forceBinding = 0)
{
    for (const auto& remap : resources) {
        const auto binding = GetBinding(compiler, remap.id);
        auto name = StringifySetAndBinding(binding.set, binding.bind);

        // if name is empty it's a block and we need to rename the base_type_id i.e.
        // "uniform <base_type_id> { vec4 foo; } <id>;"
        if (auto origname = compiler.get_name(remap.id); origname.empty()) {
            compiler.set_name(remap.base_type_id, name);
            name.insert(name.begin(), '_');
            compiler.set_name(remap.id, name);
        } else {
            // "uniform <id> vec4 foo;"
            compiler.set_name(remap.id, name);
        }

        compiler.unset_decoration(remap.id, spv::DecorationDescriptorSet);
        compiler.unset_decoration(remap.id, spv::DecorationBinding);
        if (forceBinding > 0) {
            compiler.set_decoration(remap.id, spv::DecorationBinding,
                forceBinding - 1); // will be over-written later. (special handling)
        }
    }
}

struct ShaderModulePlatformDataGLES {
    std::vector<Gles::PushConstantReflection> infos;
};

void CollectRes(
    Gles::CoreCompiler& compiler, const spirv_cross::ShaderResources& res, ShaderModulePlatformDataGLES& plat)
{
    // collect names for later linkage
    static constexpr uint32_t defaultBinding = 11;
    Collect(compiler, res.storage_buffers, defaultBinding + 1);
    Collect(compiler, res.storage_images, defaultBinding + 1);
    Collect(compiler, res.uniform_buffers, 0); // 0 == remove binding decorations (let's the compiler decide)
    Collect(compiler, res.subpass_inputs, 0);  // 0 == remove binding decorations (let's the compiler decide)

    // handle the real sampled images.
    Collect(compiler, res.sampled_images, 0); // 0 == remove binding decorations (let's the compiler decide)

    // and now the "generated ones" (separate image/sampler)
    std::string imageName;
    std::string temp;
    for (auto& remap : compiler.get_combined_image_samplers()) {
        const auto imageBinding = GetBinding(compiler, remap.image_id);
        {
            imageName.resize(imageName.capacity() - 1);
            const auto nameLen =
                snprintf(imageName.data(), imageName.size(), "s%u_b%u", imageBinding.set, imageBinding.bind);
            imageName.resize(nameLen);
        }
        const auto samplerBinding = GetBinding(compiler, remap.sampler_id);

        auto samplerName = StringifySetAndBinding(samplerBinding.set, samplerBinding.bind);

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
        static constexpr auto glesVersion = 320;
        options.version = glesVersion;
        options.es = true;
        options.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Precision::Highp;
        options.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Precision::Highp;
    }

    if (backend == DeviceBackendType::OPENGL) {
        static constexpr auto glVersion = 450;
        options.version = glVersion;
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
    ShaderStageFlags shaderStageFlags;
    DeviceBackendType backend{ DeviceBackendType::OPENGL };
    ShaderModulePlatformDataGLES plat;
    bool ovrEnabled{ false };

    std::string source_;
};

void ProcessShaderModule(Shader& me, const ShaderModuleCreateInfo& createInfo)
{
    // perform reflection.
    auto compiler = Gles::CoreCompiler(createInfo.spvData.data(), createInfo.spvData.size());
    // Set some options.
    SetupSpirvCross(me.shaderStageFlags, &compiler, me.backend, me.ovrEnabled);

    // first step in converting CORE_FLIP_NDC to regular uniform. (specializationconstant -> constant) this makes
    // the compiled glsl more readable, and simpler to post process later.
    Gles::ConvertSpecConstToConstant(compiler, "CORE_FLIP_NDC");

    auto active = compiler.get_active_interface_variables();
    const auto& res = compiler.get_shader_resources(active);
    compiler.set_enabled_interface_variables(std::move(active));

    Gles::ReflectPushConstants(compiler, res, me.plat.infos, me.shaderStageFlags);
    compiler.build_combined_image_samplers();
    CollectRes(compiler, res, me.plat);

    // set "CORE_BACKEND_TYPE" specialization to 1.
    Gles::SetSpecMacro(compiler, "CORE_BACKEND_TYPE", 1U);

    me.source_ = compiler.compile();
    Gles::ConvertConstantToUniform(compiler, me.source_, "CORE_FLIP_NDC");
}

template<typename T>
bool WriteToFile(const array_view<T>& data, const std::filesystem::path& destinationFile)
{
    std::ofstream outputStream(destinationFile, std::ios::out | std::ios::binary);
    if (!outputStream.is_open()) {
        LUME_LOG_E("Could not write file: '%s'", destinationFile.u8string().c_str());
        return false;
    }

    outputStream.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));
    outputStream.close();
    return true;
}

bool UsesMultiviewExtension(std::string_view shaderSource)
{
    static constexpr const std::string_view multiview = "GL_EXT_multiview";
    for (auto pos = shaderSource.find(multiview); pos != std::string_view::npos;
         pos = shaderSource.find(multiview, pos + multiview.size())) {
        if ((shaderSource.rfind("#extension", pos) != std::string_view::npos) &&
            (shaderSource.find("enable", pos + multiview.size()) != std::string::npos)) {
            return true;
        }
    }
    return false;
}

bool CreateGlShader(const std::filesystem::path& outputFilename, const bool multiviewEnabled,
    const DeviceBackendType backendType, const ShaderModuleCreateInfo& info)
{
    try {
        Shader shader;
        shader.shaderStageFlags = info.shaderStageFlags;
        shader.backend = backendType;
        shader.ovrEnabled = multiviewEnabled;
        ProcessShaderModule(shader, info);
        const auto* data = static_cast<const uint8_t*>(static_cast<const void*>(shader.source_.data()));
        if (!WriteToFile(array_view(data, shader.source_.size()), outputFilename)) {
            LUME_LOG_E("Failed to write to file when creating GL Shader: %s", outputFilename.c_str());
            return false;
        }
        return true;
    } catch (std::exception const& e) {
        LUME_LOG_E("Failed to generate GL(ES) shader: %s", e.what());
        return false;
    }
}

void CreateGlShaders(const std::filesystem::path& outputFilename, const std::string_view shaderSource,
    const ShaderKind shaderKind, const array_view<const uint32_t> spvBinary, const ShaderReflectionData& reflectionData)
{
    auto glFile = outputFilename;
    glFile += ".gl";
    const bool multiviewEnabled = UsesMultiviewExtension(shaderSource);

    const auto info = ShaderModuleCreateInfo{
        ShaderStageFlags(shaderKind),
        spvBinary,
        reflectionData,
    };

    CreateGlShader(glFile, multiviewEnabled, DeviceBackendType::OPENGL, info);
    glFile += "es";
    CreateGlShader(glFile, multiviewEnabled, DeviceBackendType::OPENGLES, info);
}

void MyMessageConsumer(
    spv_message_level_t level, const char* source, const spv_position_t& position, const char* message)
{
    const char* levelName = nullptr;
    switch (level) {
        case SPV_MSG_FATAL:
            levelName = "FATAL";
            break;
        case SPV_MSG_INTERNAL_ERROR:
            levelName = "INTERNAL ERROR";
            break;
        case SPV_MSG_ERROR:
            levelName = "ERROR";
            break;
        case SPV_MSG_WARNING:
            levelName = "WARNING";
            break;
        case SPV_MSG_INFO:
            return;
        case SPV_MSG_DEBUG:
            return;
    }

    // Print message to your logging system
    LUME_LOG_E(
        "[%s] %s:%zu:%zu %s", levelName, source ? source : "", position.line, position.column, message ? message : "");
}

bool IsDirty(const std::filesystem::path& outputMetaFilename, const std::uint64_t mask)
{
    LUME_LOG_V("Checking meta data file %s", outputMetaFilename.u8string().c_str());
    auto meta = std::ifstream(outputMetaFilename);
    if (!meta.is_open()) {
        // if there is no meta data with timestamp, always assume dirty.
        return true;
    }

    // if we have a meta data that can guide our heuristics evaluate those to determine the likelihood that the file
    // doesn't need recompilation. This set of heuristics are based on file modified time p.s. you system clock
    // precisions might vary per machine.
    std::string line;
    while (std::getline(meta, line)) {
        const auto delimPos = line.find_last_of(':');
        if (delimPos == std::string::npos) {
            return true;
        }
        const auto file = std::filesystem::u8path(std::string_view(line).substr(0, delimPos));
        if (!std::filesystem::exists(file)) {
            return true;
        }
        // take the unix epoch and simple xor of the compile mask for comparision
        std::uint64_t modifiedTime = 0;
        {
            auto& realLine = line.erase(0U, delimPos + 1U);
            auto [ptr, err] = std::from_chars(realLine.data(), realLine.data() + realLine.size(), modifiedTime);
            if (err == std::errc::invalid_argument) {
                LUME_LOG_E("\"Line\" is not a number.");
            } else if (err == std::errc::result_out_of_range) {
                LUME_LOG_E("Number of \"Line\" is out of range.");
            }
        }
        std::uint64_t nanosSinceUnixEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                std::filesystem::last_write_time(file).time_since_epoch())
                                                .count() ^
            mask;
        if (modifiedTime != nanosSinceUnixEpoch) {
            return true;
        }
    }
    return false;
}

bool RunAllCompilationStages(std::string_view inputFilename, CompilationSettings& settings, const Inputs& params)
{
    try {
        const auto inputFilenamePath = std::filesystem::u8path(inputFilename);
        const auto relativeInputFilename = std::filesystem::relative(inputFilenamePath, settings.shaderSourcePath);
        std::filesystem::path outputFilename = settings.compiledShaderDestinationPath / relativeInputFilename;

        // Make sure the output dir hierarchy exists.
        std::filesystem::create_directories(outputFilename.parent_path());

        // create bitmask of settings that modify the compiled output
        std::uint64_t mask = 0UL;
        mask |= params.stripDebugInformation ? 1UL : 0UL;
        mask |= params.optimizeSpirv ? 2UL : 0UL;
        mask |= std::uint64_t(params.envVersion) << 2UL;

        const std::string relativeFilename = relativeInputFilename.u8string();
        const std::string extension = inputFilenamePath.extension().string();

        // Just copying .shader files to the destination dir.
        if (extension == ".shader") {
            if (!std::filesystem::exists(outputFilename) ||
                !std::filesystem::equivalent(inputFilenamePath, outputFilename)) {
                LUME_LOG_I("  %s", relativeFilename.c_str());
                std::filesystem::copy(
                    inputFilenamePath, outputFilename, std::filesystem::copy_options::overwrite_existing);
            }
            return true;
        }

        ShaderKind shaderKind;
        if (extension == ".vert") {
            shaderKind = ShaderKind::VERTEX;
        } else if (extension == ".frag") {
            shaderKind = ShaderKind::FRAGMENT;
        } else if (extension == ".comp") {
            shaderKind = ShaderKind::COMPUTE;
        } else {
            LUME_LOG_E("Unexpected input file extension %s", extension.c_str());
            return false;
        }

        std::filesystem::path outputMetaFilename = outputFilename;
        outputMetaFilename += ".meta";

        const bool dirty = params.checkIfChanged ? IsDirty(outputMetaFilename, mask) : true;

        // nothing to do
        if (!dirty) {
            return true;
        }

        outputFilename += ".spv";

        LUME_LOG_I("  %s", relativeFilename.c_str());

        LUME_LOG_V("    input: '%s'", inputFilename.data());
        LUME_LOG_V("      dst: '%s'", settings.compiledShaderDestinationPath.u8string().c_str());
        LUME_LOG_V(" relative: '%s'", relativeFilename.c_str());
        LUME_LOG_V("   output: '%s'", outputFilename.u8string().c_str());

        auto shaderSourceOpt = ReadFileToString(inputFilename);
        if (!shaderSourceOpt) {
            LUME_LOG_E("Failed to read file at %s", std::string{ inputFilename }.c_str());
            return false;
        }
        const std::string shaderSource = *std::move(shaderSourceOpt);

        auto preProcessedShaderOpt = PreProcessShader(shaderSource, shaderKind, relativeFilename, settings);
        if (!preProcessedShaderOpt) {
            LUME_LOG_E("Failed to preprocess shader at %s of kind %s", relativeFilename.c_str(),
                ShaderKindToString(shaderKind));
            return false;
        }
        const std::string preProcessedShader = *std::move(preProcessedShaderOpt);

        constexpr bool writePreprocessedShader = false;
        if constexpr (writePreprocessedShader) {
            auto preprocessedFile = outputFilename;
            preprocessedFile += ".pre";
            if (!WriteToFile(array_view(preProcessedShader.data(), preProcessedShader.size()), preprocessedFile)) {
                LUME_LOG_E("Failed to save preprocessed %s", preprocessedFile.u8string().data());
            }
        }

        auto spvBinaryOpt = CompileShaderToSpirvBinary(preProcessedShader, shaderKind, relativeFilename, settings);
        if (!spvBinaryOpt) {
            LUME_LOG_E("Failed to compile shader to spirv binary from %s", relativeFilename.c_str());
            return false;
        }
        auto spvBinary = *std::move(spvBinaryOpt);

        auto reflectionOpt = ReflectSpvBinary(spvBinary, shaderKind);
        if (!reflectionOpt) {
            LUME_LOG_E("Failed to reflect %s", inputFilename.data());
            return false;
        }
        const auto reflection = *std::move(reflectionOpt);

        auto reflectionFile = outputFilename;
        reflectionFile += ".lsb";
        if (!WriteToFile(array_view(reflection.data(), reflection.size()), reflectionFile)) {
            LUME_LOG_E("Failed to save reflection %s", reflectionFile.u8string().data());
            return false;
        }

        // spirv-opt resets the passes everytime so then need to be setup
        if (params.optimizeSpirv) {
            settings.optimizer->RegisterPerformancePasses();
        }

        RegisterStripPreprocessorDebugInfoPass(settings.optimizer);
        settings.optimizer->SetMessageConsumer(MyMessageConsumer);
        if (!settings.optimizer->Run(spvBinary.data(), spvBinary.size(), &spvBinary)) {
            LUME_LOG_E("Failed to optimize %s", inputFilename.data());
            return false;
        }

        // generate gl and gles shaders from optimized binary but with file names intact but with just preprocessor
        // extension directives stripped out
        CreateGlShaders(outputFilename, preProcessedShader, shaderKind, spvBinary, ShaderReflectionData{ reflection });

        // strip out all other debug information like variable names, function names
        if (params.stripDebugInformation) {
            const bool registerPass = settings.optimizer->RegisterPassFromFlag("--strip-debug");
            if (registerPass == false || !settings.optimizer->Run(spvBinary.data(), spvBinary.size(), &spvBinary)) {
                LUME_LOG_E("Failed to strip debug information %s", inputFilename.data());
                return false;
            }
        }

        // write the spirv-binary to disk
        if (!WriteToFile(array_view(spvBinary.data(), spvBinary.size()), outputFilename)) {
            LUME_LOG_E("Failed to write to %s", outputFilename.u8string().c_str());
            return false;
        }

        LUME_LOG_D("  -> %s", outputFilename.u8string().c_str());

        // write meta data so that recompilation can be skipped when there are no changes to files or settings.
        std::ofstream meta = std::ofstream(outputMetaFilename);
        auto includes = settings.includer.Files();

        // store the unix epoch with simple xor of the compile mask for later comparision
        meta << inputFilenamePath.u8string() << ':'
             << (std::chrono::duration_cast<std::chrono::nanoseconds>(
                     std::filesystem::last_write_time(inputFilenamePath).time_since_epoch())
                        .count() ^
                    mask)
             << '\n';

        // store the unix epoch with simple xor of the compile mask for later comparision
        for (const auto& itt : includes) {
            meta << std::filesystem::absolute(itt.first).u8string() << ':'
                 << (std::chrono::duration_cast<std::chrono::nanoseconds>(itt.second.modicationTime.time_since_epoch())
                            .count() ^
                        mask)
                 << '\n';
        }

        return true;
    } catch (std::exception const& e) {
        LUME_LOG_E("Processing file unexpectedly failed with an exception '%s': %s", inputFilename.data(), e.what());
        return false;
    }
}

void ShowUsage()
{
    std::cout << "LumeShaderCompiler - Supported shader types: vertex (.vert), fragment (.frag), compute (.comp)\n\n"
                 "How to use: \n"
                 "LumeShaderCompiler.exe --source <source path> (sets destination path to same as source)\n"
                 "LumeShaderCompiler.exe --source <source path> --destination <destination path>\n"
                 "LumeShaderCompiler.exe --monitor (monitors changes in the source files)\n";
}

std::vector<std::string> FilterByExtension(
    const std::vector<std::string>& aFilenames, const array_view<const std::string_view>& aIncludeExtensions)
{
    std::vector<std::string> filtered;
    for (auto const& file : aFilenames) {
        std::string lowercaseFileExt = std::filesystem::u8path(file).extension().u8string();
        std::transform(lowercaseFileExt.begin(), lowercaseFileExt.end(), lowercaseFileExt.begin(), tolower);
        if (std::any_of(aIncludeExtensions.cbegin(), aIncludeExtensions.cend(),
                [lowercaseExt = std::string_view(lowercaseFileExt)](
                    const std::string_view& extension) { return (lowercaseExt == extension); })) {
            filtered.push_back(file);
        }
    }

    return filtered;
}

struct Args {
    std::string_view name;
    int32_t additionalArguments;
    bool (*handler)(Inputs& params, char* argv[]);
};

constexpr Args ARGS[] = {
    { "--help", 0,
        [](Inputs& /* params */, char* /* argv */[]) {
            ShowUsage();
            return false;
        } },
    { "--sourceFile", 1,
        [](Inputs& params, char* argv[]) {
            params.sourceFile = std::filesystem::u8path(*argv);
            params.sourceFile.make_preferred();
            params.shaderSourcesPath = params.sourceFile;
            params.shaderSourcesPath.remove_filename();
            if (params.compiledShaderDestinationPath.empty()) {
                params.compiledShaderDestinationPath = params.shaderSourcesPath;
            }
            return true;
        } },
    { "--source", 1,
        [](Inputs& params, char* argv[]) {
            params.shaderSourcesPath = std::filesystem::u8path(*argv);
            params.shaderSourcesPath.make_preferred();
            if (params.compiledShaderDestinationPath.empty()) {
                params.compiledShaderDestinationPath = params.shaderSourcesPath;
            }
            return true;
        } },
    { "--destination", 1,
        [](Inputs& params, char* argv[]) {
            params.compiledShaderDestinationPath = std::filesystem::u8path(*argv);
            params.compiledShaderDestinationPath.make_preferred();
            return true;
        } },
    { "--include", 1,
        [](Inputs& params, char* argv[]) {
            params.shaderIncludePaths.emplace_back(std::filesystem::u8path(*argv)).make_preferred();
            return true;
        } },
    { "--monitor", 0,
        [](Inputs& params, char* argv[]) {
            params.monitorChanges = true;
            return true;
        } },
    { "--optimize", 0,
        [](Inputs& params, char* argv[]) {
            params.optimizeSpirv = true;
            return true;
        } },
    { "--check-if-changed", 0,
        [](Inputs& params, char* argv[]) {
            params.checkIfChanged = true;
            return true;
        } },
    { "--strip-debug-information", 0,
        [](Inputs& params, char* argv[]) {
            params.stripDebugInformation = true;
            return true;
        } },
    { "--vulkan", 1,
        [](Inputs& params, char* argv[]) {
            const auto version = std::string_view(*argv);
            if (version == "1.0") {
                params.envVersion = ShaderEnv::version_vulkan_1_0;
            } else if (version == "1.1") {
                params.envVersion = ShaderEnv::version_vulkan_1_1;
            } else if (version == "1.2") {
                params.envVersion = ShaderEnv::version_vulkan_1_2;
#if GLSLANG_VERSION >= GLSLANG_VERSION_12_2_0
            } else if (version == "1.3") {
                params.envVersion = ShaderEnv::version_vulkan_1_3;
#endif
            } else {
                LUME_LOG_E("Invalid vulkan version after --vulkan: %s", std::string{ version }.c_str());
                return false;
            }
            return true;
        } },
};

std::optional<Inputs> Parse(const int argc, char* argv[])
{
    Inputs params;
    const std::filesystem::path currentFolder = std::filesystem::current_path();
    params.shaderSourcesPath = currentFolder;

    for (int i = 1; i < argc;) {
        const auto arg = std::string_view(argv[i]);
        const auto pos =
            std::find_if(std::begin(ARGS), std::end(ARGS), [arg](const Args& item) { return item.name == arg; });
        if (pos == std::end(ARGS)) {
            LUME_LOG_E("Unknown argument %.*s.\n", int(arg.size()), arg.data());
            return std::nullopt;
        }
        if ((i + pos->additionalArguments) >= argc) {
            LUME_LOG_E("%.*s option requires %u additional arguments.\n", int(arg.size()), arg.data(),
                pos->additionalArguments);
            return std::nullopt;
        }

        if (!pos->handler(params, argv + (i + 1))) {
            return std::nullopt;
        }
        i = (i + 1) + pos->additionalArguments;
    }
    if (params.compiledShaderDestinationPath.empty()) {
        params.compiledShaderDestinationPath = currentFolder;
    }

    return params;
}

constexpr std::string_view SUPPORTED_EXTENSIONS[] = { ".vert", ".frag", ".comp", ".shader" };
constexpr auto SUPPORTED_EXTENSIONS_VIEW =
    array_view(SUPPORTED_EXTENSIONS, std::extent_v<decltype(SUPPORTED_EXTENSIONS)>);

struct MonitorResults {
    std::vector<std::string> addedFiles;
    std::vector<std::string> removedFiles;
    std::vector<std::string> modifiedFiles;
};

void HandleFiles(
    FileIncluder& fileIncluder, const MonitorResults& results, CompilationSettings& settings, const Inputs& params)
{
    fileIncluder.Reset();
    if (!results.addedFiles.empty()) {
        LUME_LOG_I("Files added:");
        for (auto const& addedFile : results.addedFiles) {
            if (RunAllCompilationStages(addedFile, settings, params)) {
                LUME_LOG_I(" - %s: success", addedFile.c_str());
            } else {
                LUME_LOG_E(" - %s: failed", addedFile.c_str());
            }
        }
    }

    if (!results.removedFiles.empty()) {
        LUME_LOG_I("Files removed:");
        for (auto const& removedFile : results.removedFiles) {
            std::string relativeFilename = std::filesystem::relative(removedFile, params.shaderSourcesPath).u8string();
            LUME_LOG_I(" - %s", relativeFilename.c_str());
        }
    }

    if (!results.modifiedFiles.empty()) {
        LUME_LOG_I("Files modified:");
        for (auto const& modifiedFile : results.modifiedFiles) {
            if (RunAllCompilationStages(modifiedFile, settings, params)) {
                LUME_LOG_I(" - %s: success", modifiedFile.c_str());
            } else {
                LUME_LOG_E(" - %s: failed", modifiedFile.c_str());
            }
        }
    }
}

void HandleFilesFiltered(FileIncluder& fileIncluder, std::vector<std::string>& modifiedFiles,
    CompilationSettings& settings, const Inputs& params)
{
    fileIncluder.Reset();
    auto pos = std::find_if(modifiedFiles.cbegin(), modifiedFiles.cend(),
        [&sourceFile = params.sourceFile](const std::string& modified) { return modified == sourceFile; });
    if (pos != modifiedFiles.cend()) {
        if (RunAllCompilationStages(*pos, settings, params)) {
            LUME_LOG_I(" - %s: success", pos->c_str());
        } else {
            LUME_LOG_E(" - %s: failed", pos->c_str());
        }
    }
}
} // namespace

int CompilerMain(int argc, char* argv[])
{
    if (argc == 1) {
        ShowUsage();
        return 0;
    }

    const std::optional<Inputs> params = Parse(argc, argv);
    if (!params) {
        LUME_LOG_E("Failed to parse command line parameters.");
        return 1;
    }
    ige::FileMonitor fileMonitor;

    if (!std::filesystem::exists(params->shaderSourcesPath)) {
        LUME_LOG_E("Source path does not exist: '%s'", params->shaderSourcesPath.u8string().c_str());
        return 1;
    }

    // Make sure the destination dir exists.
    std::filesystem::create_directories(params->compiledShaderDestinationPath);

    if (!std::filesystem::exists(params->compiledShaderDestinationPath)) {
        LUME_LOG_E("Destination path does not exist: '%s'", params->compiledShaderDestinationPath.u8string().c_str());
        return 1;
    }

    fileMonitor.AddPath(params->shaderSourcesPath.u8string());
    std::vector<std::string> fileList = [&]() {
        std::vector<std::string> list;
        if (!params->sourceFile.empty()) {
            list.push_back(params->sourceFile.u8string());
        } else {
            list = fileMonitor.GetMonitoredFiles();
        }
        return list;
    }();

    fileList = FilterByExtension(fileList, SUPPORTED_EXTENSIONS_VIEW);

    LUME_LOG_I("     Source path: '%s'", std::filesystem::absolute(params->shaderSourcesPath).u8string().c_str());
    for (auto const& path : params->shaderIncludePaths) {
        LUME_LOG_I("    Include path: '%s'", std::filesystem::absolute(path).u8string().c_str());
    }
    LUME_LOG_I(
        "Destination path: '%s'", std::filesystem::absolute(params->compiledShaderDestinationPath).u8string().c_str());
    LUME_LOG_I("");
    LUME_LOG_I("Processing:");

    std::filesystem::path dest = std::filesystem::absolute(params->compiledShaderDestinationPath);

    int errorCount = 0;
    Scope scope(glslang::InitializeProcess, glslang::FinalizeProcess);

    std::vector<std::filesystem::path> searchPath;
    searchPath.reserve(1U + params->shaderIncludePaths.size());
    searchPath.emplace_back(params->shaderSourcesPath);
    std::transform(params->shaderIncludePaths.cbegin(), params->shaderIncludePaths.cend(),
        std::back_inserter(searchPath), [](const std::filesystem::path& path) { return path; });

    auto fileIncluder = FileIncluder(params->shaderSourcesPath, searchPath);
    auto settings = CompilationSettings{ params->envVersion, searchPath, {}, params->shaderSourcesPath,
        params->compiledShaderDestinationPath, fileIncluder };

    {
        spv_target_env targetEnv = spv_target_env::SPV_ENV_VULKAN_1_0;
        switch (params->envVersion) {
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
        const auto filePath = std::filesystem::u8path(file);
        std::string relativeFilename = std::filesystem::relative(filePath, params->shaderSourcesPath).u8string();
        LUME_LOG_D("Tracked source file: '%s'", relativeFilename.c_str());
        if (!RunAllCompilationStages(file, settings, *params)) {
            LUME_LOG_E("Failed to run some compilation stage on file %s", relativeFilename.c_str());
            errorCount++;
        }
    }

    if (errorCount == 0) {
        LUME_LOG_I("Success.");
    } else {
        LUME_LOG_E("Failed processing %d files.", errorCount);
    }

    if (params->monitorChanges) {
        LUME_LOG_I("Monitoring file changes.");
    }

    // Main loop.
    while (params->monitorChanges) {
        MonitorResults results;

        fileMonitor.ScanModifications(results.addedFiles, results.removedFiles, results.modifiedFiles);
        results.modifiedFiles = FilterByExtension(results.modifiedFiles, SUPPORTED_EXTENSIONS_VIEW);

        if (params->sourceFile.empty()) {
            results.addedFiles = FilterByExtension(results.addedFiles, SUPPORTED_EXTENSIONS_VIEW);
            results.removedFiles = FilterByExtension(results.removedFiles, SUPPORTED_EXTENSIONS_VIEW);
            HandleFiles(fileIncluder, results, settings, *params);
        } else if (!results.modifiedFiles.empty()) {
            HandleFilesFiltered(fileIncluder, results.modifiedFiles, settings, *params);
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return errorCount;
}
