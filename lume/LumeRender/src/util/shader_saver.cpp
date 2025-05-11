/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "util/shader_saver.h"

#include <array>

template<typename Key, typename Value>
struct PairLike {
    Key first;
    Value second;
};

template<typename Key, typename Value, size_t Size>
struct CTMap {
    std::array<PairLike<Key, Value>, Size> data;
    constexpr const Value& at(const Key& key) const
    {
        for (size_t i = 0; i < Size; ++i) {
            if (data[i].first == key) {
                return data[i].second;
            }
        }
        return defaultValue;
    }
    static constexpr Value defaultValue {};
};

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr CTMap<BlendOp, const char*, 5U> blendOpToString { { PairLike<BlendOp, const char*> {
                                                                  CORE_BLEND_OP_ADD, "add" },
    PairLike<BlendOp, const char*> { CORE_BLEND_OP_SUBTRACT, "subtract" },
    PairLike<BlendOp, const char*> { CORE_BLEND_OP_REVERSE_SUBTRACT, "reverse_subtract" },
    PairLike<BlendOp, const char*> { CORE_BLEND_OP_MIN, "min" },
    PairLike<BlendOp, const char*> { CORE_BLEND_OP_MAX, "max" } } };

constexpr CTMap<BlendFactor, const char*, 19U> blendFactorToString { { PairLike<BlendFactor, const char*> {
                                                                           CORE_BLEND_FACTOR_ZERO, "zero" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE, "one" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_SRC_COLOR, "src_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, "one_minus_src_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_DST_COLOR, "dst_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_DST_COLOR, "one_minus_dst_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_SRC_ALPHA, "src_alpha" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, "one_minus_src_alpha" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_DST_ALPHA, "dst_alpha" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, "one_minus_dst_alpha" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_CONSTANT_COLOR, "constant_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, "one_minus_constant_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_CONSTANT_ALPHA, "constant_alpha" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, "one_minus_constant_alpha" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_SRC_ALPHA_SATURATE, "src_alpha_saturate" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_SRC1_COLOR, "src1_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR, "one_minus_src1_color" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_SRC1_ALPHA, "src1_alpha" },
    PairLike<BlendFactor, const char*> { CORE_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, "one_minus_src1_alpha" } } };

constexpr CTMap<Format, const char*, 192U> formatToString { { PairLike<Format, const char*> {
                                                                  BASE_FORMAT_UNDEFINED, "undefined" },
    PairLike<Format, const char*> { BASE_FORMAT_R4G4_UNORM_PACK8, "r4g4_unorm_pack8" },
    PairLike<Format, const char*> { BASE_FORMAT_R4G4B4A4_UNORM_PACK16, "r4g4b4a4_unorm_pack16" },
    PairLike<Format, const char*> { BASE_FORMAT_B4G4R4A4_UNORM_PACK16, "b4g4r4a4_unorm_pack16" },
    PairLike<Format, const char*> { BASE_FORMAT_R5G6B5_UNORM_PACK16, "r5g6b5_unorm_pack16" },
    PairLike<Format, const char*> { BASE_FORMAT_B5G6R5_UNORM_PACK16, "b5g6r5_unorm_pack16" },
    PairLike<Format, const char*> { BASE_FORMAT_R5G5B5A1_UNORM_PACK16, "r5g5b5a1_unorm_pack16" },
    PairLike<Format, const char*> { BASE_FORMAT_B5G5R5A1_UNORM_PACK16, "b5g5r5a1_unorm_pack16" },
    PairLike<Format, const char*> { BASE_FORMAT_A1R5G5B5_UNORM_PACK16, "a1r5g5b5_unorm_pack16" },
    PairLike<Format, const char*> { BASE_FORMAT_R8_UNORM, "r8_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8_SNORM, "r8_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8_USCALED, "r8_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8_SSCALED, "r8_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8_UINT, "r8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8_SINT, "r8_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8_SRGB, "r8_srgb" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8_UNORM, "r8g8_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8_SNORM, "r8g8_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8_USCALED, "r8g8_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8_SSCALED, "r8g8_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8_UINT, "r8g8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8_SINT, "r8g8_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8_SRGB, "r8g8_srgb" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8_UNORM, "r8g8b8_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8_SNORM, "r8g8b8_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8_USCALED, "r8g8b8_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8_SSCALED, "r8g8b8_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8_UINT, "r8g8b8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8_SINT, "r8g8b8_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8_SRGB, "r8g8b8_srgb" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8_UNORM, "b8g8r8_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8_SNORM, "b8g8r8_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8_USCALED, "b8g8r8_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8_SSCALED, "b8g8r8_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8_UINT, "b8g8r8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8_SINT, "b8g8r8_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8_SRGB, "b8g8r8_srgb" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8A8_UNORM, "r8g8b8a8_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8A8_SNORM, "r8g8b8a8_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8A8_USCALED, "r8g8b8a8_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8A8_SSCALED, "r8g8b8a8_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8A8_UINT, "r8g8b8a8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8A8_SINT, "r8g8b8a8_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R8G8B8A8_SRGB, "r8g8b8a8_srgb" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8A8_UNORM, "b8g8r8a8_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8A8_SNORM, "b8g8r8a8_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8A8_USCALED, "b8g8r8a8_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8A8_SSCALED, "b8g8r8a8_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8A8_UINT, "b8g8r8a8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8A8_SINT, "b8g8r8a8_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8A8_SRGB, "b8g8r8a8_srgb" },
    PairLike<Format, const char*> { BASE_FORMAT_A8B8G8R8_UNORM_PACK32, "a8b8g8r8_unorm_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A8B8G8R8_SNORM_PACK32, "a8b8g8r8_snorm_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A8B8G8R8_USCALED_PACK32, "a8b8g8r8_uscaled_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A8B8G8R8_SSCALED_PACK32, "a8b8g8r8_sscaled_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A8B8G8R8_UINT_PACK32, "a8b8g8r8_uint_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A8B8G8R8_SINT_PACK32, "a8b8g8r8_sint_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A8B8G8R8_SRGB_PACK32, "a8b8g8r8_srgb_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2R10G10B10_UNORM_PACK32, "a2r10g10b10_unorm_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2R10G10B10_SNORM_PACK32, "a2r10g10b10_snorm_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2R10G10B10_USCALED_PACK32, "a2r10g10b10_uscaled_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2R10G10B10_SSCALED_PACK32, "a2r10g10b10_sscaled_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2R10G10B10_UINT_PACK32, "a2r10g10b10_uint_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2R10G10B10_SINT_PACK32, "a2r10g10b10_sint_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2B10G10R10_UNORM_PACK32, "a2b10g10r10_unorm_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2B10G10R10_SNORM_PACK32, "a2b10g10r10_snorm_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2B10G10R10_USCALED_PACK32, "a2b10g10r10_uscaled_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2B10G10R10_SSCALED_PACK32, "a2b10g10r10_sscaled_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2B10G10R10_UINT_PACK32, "a2b10g10r10_uint_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_A2B10G10R10_SINT_PACK32, "a2b10g10r10_sint_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_R16_UNORM, "r16_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16_SNORM, "r16_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16_USCALED, "r16_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16_SSCALED, "r16_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16_UINT, "r16_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16_SINT, "r16_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16_SFLOAT, "r16_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16_UNORM, "r16g16_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16_SNORM, "r16g16_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16_USCALED, "r16g16_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16_SSCALED, "r16g16_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16_UINT, "r16g16_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16_SINT, "r16g16_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16_SFLOAT, "r16g16_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16_UNORM, "r16g16b16_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16_SNORM, "r16g16b16_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16_USCALED, "r16g16b16_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16_SSCALED, "r16g16b16_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16_UINT, "r16g16b16_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16_SINT, "r16g16b16_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16_SFLOAT, "r16g16b16_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16A16_UNORM, "r16g16b16a16_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16A16_SNORM, "r16g16b16a16_snorm" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16A16_USCALED, "r16g16b16a16_uscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16A16_SSCALED, "r16g16b16a16_sscaled" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16A16_UINT, "r16g16b16a16_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16A16_SINT, "r16g16b16a16_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R16G16B16A16_SFLOAT, "r16g16b16a16_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R32_UINT, "r32_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32_SINT, "r32_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32_SFLOAT, "r32_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32_UINT, "r32g32_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32_SINT, "r32g32_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32_SFLOAT, "r32g32_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32B32_UINT, "r32g32b32_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32B32_SINT, "r32g32b32_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32B32_SFLOAT, "r32g32b32_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32B32A32_UINT, "r32g32b32a32_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32B32A32_SINT, "r32g32b32a32_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R32G32B32A32_SFLOAT, "r32g32b32a32_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R64_UINT, "r64_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64_SINT, "r64_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64_SFLOAT, "r64_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64_UINT, "r64g64_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64_SINT, "r64g64_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64_SFLOAT, "r64g64_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64B64_UINT, "r64g64b64_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64B64_SINT, "r64g64b64_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64B64_SFLOAT, "r64g64b64_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64B64A64_UINT, "r64g64b64a64_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64B64A64_SINT, "r64g64b64a64_sint" },
    PairLike<Format, const char*> { BASE_FORMAT_R64G64B64A64_SFLOAT, "r64g64b64a64_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_B10G11R11_UFLOAT_PACK32, "b10g11r11_ufloat_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_E5B9G9R9_UFLOAT_PACK32, "e5b9g9r9_ufloat_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_D16_UNORM, "d16_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_X8_D24_UNORM_PACK32, "x8_d24_unorm_pack32" },
    PairLike<Format, const char*> { BASE_FORMAT_D32_SFLOAT, "d32_sfloat" },
    PairLike<Format, const char*> { BASE_FORMAT_S8_UINT, "s8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_D16_UNORM_S8_UINT, "d16_unorm_s8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_D24_UNORM_S8_UINT, "d24_unorm_s8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_D32_SFLOAT_S8_UINT, "d32_sfloat_s8_uint" },
    PairLike<Format, const char*> { BASE_FORMAT_BC1_RGB_UNORM_BLOCK, "bc1_rgb_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC1_RGB_SRGB_BLOCK, "bc1_rgb_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC1_RGBA_UNORM_BLOCK, "bc1_rgba_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC1_RGBA_SRGB_BLOCK, "bc1_rgba_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC2_UNORM_BLOCK, "bc2_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC2_SRGB_BLOCK, "bc2_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC3_UNORM_BLOCK, "bc3_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC3_SRGB_BLOCK, "bc3_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC4_UNORM_BLOCK, "bc4_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC4_SNORM_BLOCK, "bc4_snorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC5_UNORM_BLOCK, "bc5_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC5_SNORM_BLOCK, "bc5_snorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC6H_UFLOAT_BLOCK, "bc6h_ufloat_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC6H_SFLOAT_BLOCK, "bc6h_sfloat_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC7_UNORM_BLOCK, "bc7_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_BC7_SRGB_BLOCK, "bc7_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, "etc2_r8g8b8_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, "etc2_r8g8b8_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, "etc2_r8g8b8a1_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, "etc2_r8g8b8a1_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, "etc2_r8g8b8a8_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, "etc2_r8g8b8a8_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_EAC_R11_UNORM_BLOCK, "eac_r11_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_EAC_R11_SNORM_BLOCK, "eac_r11_snorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_EAC_R11G11_UNORM_BLOCK, "eac_r11g11_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_EAC_R11G11_SNORM_BLOCK, "eac_r11g11_snorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, "astc_4x4_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_4x4_SRGB_BLOCK, "astc_4x4_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, "astc_5x4_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_5x4_SRGB_BLOCK, "astc_5x4_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, "astc_5x5_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_5x5_SRGB_BLOCK, "astc_5x5_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, "astc_6x5_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_6x5_SRGB_BLOCK, "astc_6x5_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, "astc_6x6_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, "astc_6x6_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, "astc_8x5_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_8x5_SRGB_BLOCK, "astc_8x5_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, "astc_8x6_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_8x6_SRGB_BLOCK, "astc_8x6_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, "astc_8x8_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_8x8_SRGB_BLOCK, "astc_8x8_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, "astc_10x5_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x5_SRGB_BLOCK, "astc_10x5_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, "astc_10x6_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x6_SRGB_BLOCK, "astc_10x6_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, "astc_10x8_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x8_SRGB_BLOCK, "astc_10x8_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, "astc_10x10_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_10x10_SRGB_BLOCK, "astc_10x10_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, "astc_12x10_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_12x10_SRGB_BLOCK, "astc_12x10_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, "astc_12x12_unorm_block" },
    PairLike<Format, const char*> { BASE_FORMAT_ASTC_12x12_SRGB_BLOCK, "astc_12x12_srgb_block" },
    PairLike<Format, const char*> { BASE_FORMAT_G8B8G8R8_422_UNORM, "g8b8g8r8_422_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_B8G8R8G8_422_UNORM, "b8g8r8g8_422_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_G8_B8_R8_3PLANE_420_UNORM, "g8_b8_r8_3plane_420_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_G8_B8R8_2PLANE_420_UNORM, "g8_b8r8_2plane_420_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_G8_B8_R8_3PLANE_422_UNORM, "g8_b8_r8_3plane_422_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_G8_B8R8_2PLANE_422_UNORM, "g8_b8r8_2plane_422_unorm" },
    PairLike<Format, const char*> { BASE_FORMAT_G8_B8_R8_3PLANE_444_UNORM, "g8_b8_r8_3plane_444_unorm" } } };

constexpr CTMap<StencilOp, const char*, 8U> stencilOpToString { { PairLike<StencilOp, const char*> {
                                                                      CORE_STENCIL_OP_KEEP, "keep" },
    PairLike<StencilOp, const char*> { CORE_STENCIL_OP_ZERO, "zero" },
    PairLike<StencilOp, const char*> { CORE_STENCIL_OP_REPLACE, "replace" },
    PairLike<StencilOp, const char*> { CORE_STENCIL_OP_INCREMENT_AND_CLAMP, "increment_and_clamp" },
    PairLike<StencilOp, const char*> { CORE_STENCIL_OP_DECREMENT_AND_CLAMP, "decrement_and_clamp" },
    PairLike<StencilOp, const char*> { CORE_STENCIL_OP_INVERT, "invert" },
    PairLike<StencilOp, const char*> { CORE_STENCIL_OP_INCREMENT_AND_WRAP, "increment_and_wrap" },
    PairLike<StencilOp, const char*> { CORE_STENCIL_OP_DECREMENT_AND_WRAP, "decrement_and_wrap" } } };

constexpr CTMap<CompareOp, const char*, 8U> compareOpToString { { PairLike<CompareOp, const char*> {
                                                                      CORE_COMPARE_OP_NEVER, "never" },
    PairLike<CompareOp, const char*> { CORE_COMPARE_OP_LESS, "less" },
    PairLike<CompareOp, const char*> { CORE_COMPARE_OP_EQUAL, "equal" },
    PairLike<CompareOp, const char*> { CORE_COMPARE_OP_LESS_OR_EQUAL, "less_or_equal" },
    PairLike<CompareOp, const char*> { CORE_COMPARE_OP_GREATER, "greater" },
    PairLike<CompareOp, const char*> { CORE_COMPARE_OP_NOT_EQUAL, "not_equal" },
    PairLike<CompareOp, const char*> { CORE_COMPARE_OP_GREATER_OR_EQUAL, "greater_or_equal" },
    PairLike<CompareOp, const char*> { CORE_COMPARE_OP_ALWAYS, "always" } } };

constexpr CTMap<LogicOp, const char*, 16U> logicOpToString { { PairLike<LogicOp, const char*> {
                                                                   CORE_LOGIC_OP_CLEAR, "clear" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_AND, "and" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_AND_REVERSE, "and_reverse" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_COPY, "copy" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_AND_INVERTED, "and_inverted" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_NO_OP, "no_op" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_XOR, "xor" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_OR, "or" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_NOR, "nor" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_EQUIVALENT, "equivalent" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_INVERT, "invert" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_OR_REVERSE, "or_reverse" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_COPY_INVERTED, "copy_inverted" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_OR_INVERTED, "or_inverted" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_NAND, "nand" },
    PairLike<LogicOp, const char*> { CORE_LOGIC_OP_SET, "set" } } };

constexpr CTMap<PrimitiveTopology, const char*, 12U> primitiveTopologyToString { {
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_POINT_LIST, "point_list" },
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_LINE_LIST, "line_list" },
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP, "line_strip" },
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "triangle_list" },
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, "triangle_strip" },
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, "triangle_fan" },
    PairLike<PrimitiveTopology, const char*> {
        CORE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY, "line_list_with_adjacency" },
    PairLike<PrimitiveTopology, const char*> {
        CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY, "line_strip_with_adjacency" },
    PairLike<PrimitiveTopology, const char*> {
        CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, "triangle_list_with_adjacency" },
    PairLike<PrimitiveTopology, const char*> {
        CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, "triangle_strip_with_adjacency" },
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST, "patch_list" },
    PairLike<PrimitiveTopology, const char*> { CORE_PRIMITIVE_TOPOLOGY_MAX_ENUM, "max_enum" },

} };

constexpr CTMap<DescriptorType, const char*, 13U> descriptorTypeToString { {
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_SAMPLER, "sampler" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "combined_image_sampler" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, "sampled_image" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, "storage_image" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, "uniform_texel_buffer" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, "storage_texel_buffer" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, "uniform_buffer" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, "storage_buffer" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "uniform_buffer_dynamic" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, "storage_buffer_dynamic" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, "input_attachment" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE, "acceleration_structure" },
    PairLike<DescriptorType, const char*> { CORE_DESCRIPTOR_TYPE_MAX_ENUM, "max_enum" },
} };

constexpr const char* rgbaBitNames[] = { "r_bit", "g_bit", "b_bit", "a_bit" };

constexpr const char* graphicsStateBitNames[] = {
    "input_assembly_bit",
    "rasterization_state_bit",
    "depth_stencil_state_bit",
    "color_blend_state_bit",
};

constexpr const char* cullModeBitNames[] = {
    "none",
    "front_bit",
    "back_bit",
    "front_and_back",
};

// generic bits to string helper, takes in the nameSet that is used to output string
// (assumes non skipping bit order)
string BitsToString(uint32_t field, array_view<const char* const> nameSet)
{
    bool isFirst = true;
    string ret;
    for (uint32_t i = 0; i < nameSet.size(); ++i) {
        if (field & (1 << i)) {
            if (!isFirst) {
                ret.append("|");
            }
            ret.append(nameSet[i]);
            isFirst = false;
        }
    }

    return ret;
}

string ShaderStageBitsToString(uint32_t field)
{
    if (field == 0x0000001f) {
        return "all_graphics";
    } else if (field == 0x7fffffff) {
        return "all";
    }

    string ret {};
    if (field & 0x00000001) {
        ret.append("vertex_bit");
    }
    if (field & 0x00000010) {
        if (!ret.empty()) {
            ret.append("|");
        }
        ret.append("fragment_bit");
    }
    if (field & 0x00000020) {
        if (!ret.empty()) {
            ret.append("|");
        }
        ret.append("compute_bit");
    }

    return ret;
}

string CullModeToString(uint32_t mode)
{
    for (uint32_t i = 0; i < 4U; ++i) {
        if (mode == i) {
            return cullModeBitNames[i];
        }
    }
    return {};
}

json::standalone_value MakeRasterizationStateJson(const GraphicsState::RasterizationState& rasterizationState)
{
    json::standalone_value rasterizationStateJson = json::standalone_value::object();
    rasterizationStateJson["cullModeFlags"] = CullModeToString(rasterizationState.cullModeFlags);
    rasterizationStateJson["depthBiasClamp"] = rasterizationState.depthBiasClamp;
    rasterizationStateJson["depthBiasConstantFactor"] = rasterizationState.depthBiasConstantFactor;
    rasterizationStateJson["depthBiasSlopeFactor"] = rasterizationState.depthBiasSlopeFactor;
    rasterizationStateJson["enableDepthBias"] = json::standalone_value(rasterizationState.enableDepthBias);
    rasterizationStateJson["enableDepthClamp"] = json::standalone_value(rasterizationState.enableDepthClamp);
    rasterizationStateJson["enableRasterizerDiscard"] =
        json::standalone_value(rasterizationState.enableRasterizerDiscard);
    rasterizationStateJson["lineWidth"] = rasterizationState.lineWidth;

    switch (rasterizationState.frontFace) {
        case CORE_FRONT_FACE_CLOCKWISE:
            rasterizationStateJson["frontFace"] = "clockwise";
            break;
        case CORE_FRONT_FACE_COUNTER_CLOCKWISE:
        default:
            rasterizationStateJson["frontFace"] = "counter_clockwise";
            break;
    }
    switch (rasterizationState.polygonMode) {
        case CORE_POLYGON_MODE_FILL:
        default:
            rasterizationStateJson["polygonMode"] = "fill";
            break;
        case CORE_POLYGON_MODE_LINE:
            rasterizationStateJson["polygonMode"] = "line";
            break;
        case CORE_POLYGON_MODE_POINT:
            rasterizationStateJson["polygonMode"] = "point";
            break;
    }

    return rasterizationStateJson;
}

json::standalone_value MakeDepthStencilStateJson(const GraphicsState::DepthStencilState& depthStencilState)
{
    json::standalone_value depthStencilStateJson = json::standalone_value::object();
    depthStencilStateJson["depthCompareOp"] = compareOpToString.at(depthStencilState.depthCompareOp);
    depthStencilStateJson["enableDepthBoundsTest"] = json::standalone_value(depthStencilState.enableDepthBoundsTest);
    depthStencilStateJson["enableDepthTest"] = json::standalone_value(depthStencilState.enableDepthTest);
    depthStencilStateJson["enableDepthWrite"] = json::standalone_value(depthStencilState.enableDepthWrite);
    depthStencilStateJson["enableStencilTest"] = json::standalone_value(depthStencilState.enableStencilTest);
    depthStencilStateJson["maxDepthBounds"] = depthStencilState.maxDepthBounds;
    depthStencilStateJson["minDepthBounds"] = depthStencilState.minDepthBounds;

    json::standalone_value backStencilOpStateJson = json::standalone_value::object();
    backStencilOpStateJson["compareOp"] = compareOpToString.at(depthStencilState.backStencilOpState.compareOp);
    backStencilOpStateJson["depthFailOp"] = stencilOpToString.at(depthStencilState.backStencilOpState.depthFailOp);
    backStencilOpStateJson["failOp"] = stencilOpToString.at(depthStencilState.backStencilOpState.failOp);
    backStencilOpStateJson["passOp"] = stencilOpToString.at(depthStencilState.backStencilOpState.passOp);
    backStencilOpStateJson["compareMask"] = string(BASE_NS::to_hex(depthStencilState.backStencilOpState.compareMask));
    backStencilOpStateJson["reference"] = string(BASE_NS::to_hex(depthStencilState.backStencilOpState.reference));
    backStencilOpStateJson["writeMask"] = BitsToString(depthStencilState.backStencilOpState.writeMask, rgbaBitNames);
    depthStencilStateJson["backStencilOpState"] = backStencilOpStateJson;

    json::standalone_value frontStencilOpStateJson = json::standalone_value::object();
    frontStencilOpStateJson["compareOp"] = compareOpToString.at(depthStencilState.frontStencilOpState.compareOp);
    frontStencilOpStateJson["depthFailOp"] = stencilOpToString.at(depthStencilState.frontStencilOpState.depthFailOp);
    frontStencilOpStateJson["failOp"] = stencilOpToString.at(depthStencilState.frontStencilOpState.failOp);
    frontStencilOpStateJson["passOp"] = stencilOpToString.at(depthStencilState.frontStencilOpState.passOp);
    frontStencilOpStateJson["compareMask"] = string(BASE_NS::to_hex(depthStencilState.frontStencilOpState.compareMask));
    frontStencilOpStateJson["reference"] = string(BASE_NS::to_hex(depthStencilState.frontStencilOpState.reference));
    frontStencilOpStateJson["writeMask"] = BitsToString(depthStencilState.frontStencilOpState.writeMask, rgbaBitNames);
    depthStencilStateJson["frontStencilOpState"] = frontStencilOpStateJson;

    return depthStencilStateJson;
}

json::standalone_value MakeColorBlendStateJson(const GraphicsState::ColorBlendState& colorBlendState)
{
    json::standalone_value colorBlendStateJson = json::standalone_value::object();
    colorBlendStateJson["colorAttachments"] = json::standalone_value::array();
    for (uint32_t i = 0; i < colorBlendState.colorAttachmentCount; ++i) {
        json::standalone_value attachmentJson = json::standalone_value::object();
        attachmentJson["alphaBlendOp"] = blendOpToString.at(colorBlendState.colorAttachments[i].alphaBlendOp);
        attachmentJson["colorBlendOp"] = blendOpToString.at(colorBlendState.colorAttachments[i].colorBlendOp);
        attachmentJson["colorWriteMask"] =
            BitsToString(colorBlendState.colorAttachments[i].colorWriteMask, rgbaBitNames);
        attachmentJson["dstAlphaBlendFactor"] =
            blendFactorToString.at(colorBlendState.colorAttachments[i].dstAlphaBlendFactor);
        attachmentJson["dstColorBlendFactor"] =
            blendFactorToString.at(colorBlendState.colorAttachments[i].dstColorBlendFactor);
        attachmentJson["enableBlend"] = json::standalone_value(colorBlendState.colorAttachments[i].enableBlend);
        attachmentJson["srcAlphaBlendFactor"] =
            blendFactorToString.at(colorBlendState.colorAttachments[i].srcAlphaBlendFactor);
        attachmentJson["srcColorBlendFactor"] =
            blendFactorToString.at(colorBlendState.colorAttachments[i].srcColorBlendFactor);

        colorBlendStateJson["colorAttachments"].array_.emplace_back(attachmentJson);
    }
    colorBlendStateJson["colorBlendConstants"] = colorBlendState.colorBlendConstants;
    colorBlendStateJson["enableLogicOp"] = json::standalone_value(colorBlendState.enableLogicOp);
    colorBlendStateJson["logicOp"] = logicOpToString.at(colorBlendState.logicOp);

    return colorBlendStateJson;
}

json::standalone_value MakeInputAssemblyJson(const GraphicsState::InputAssembly& inputAssembly)
{
    json::standalone_value inputAssemblyJson = json::standalone_value::object();
    inputAssemblyJson["enablePrimitiveRestart"] = json::standalone_value(inputAssembly.enablePrimitiveRestart);
    inputAssemblyJson["primitiveTopology"] = primitiveTopologyToString.at(inputAssembly.primitiveTopology);

    return inputAssemblyJson;
}
} // namespace

IShaderManager::ShaderOutWriteResult SaveGraphicsState(const IShaderManager::ShaderGraphicsStateSaveInfo& saveInfo)
{
    // .shadergs file type
    IShaderManager::ShaderOutWriteResult r;
    if (saveInfo.states.size() != saveInfo.stateVariants.size()) {
        r.error = "ShaderLoader::SaveShaderStates - states and stateVariants are not equal in size.\n";
        r.success = false;
        return r;
    }

    json::standalone_value sgJson = json::standalone_value::object();
    sgJson["combatibility_info"] = json::standalone_value::object();
    sgJson["combatibility_info"]["version"] = "22.00";
    sgJson["combatibility_info"]["type"] = "shaderstate";
    sgJson["shaderStates"] = json::standalone_value::array();

    // each GraphicsState is serialized with VariantData(ShaderStateLoaderVariantData)
    for (uint32_t i = 0; i < saveInfo.stateVariants.size(); ++i) {
        json::standalone_value stateInfoJson = json::standalone_value::object();
        const auto& stateVariant = saveInfo.stateVariants.at(i);
        stateInfoJson["baseShaderState"] = stateVariant.baseShaderState;
        stateInfoJson["baseVariantName"] = stateVariant.baseVariantName;
        stateInfoJson["variantName"] = stateVariant.variantName;
        stateInfoJson["renderSlot"] = stateVariant.renderSlot;
        stateInfoJson["renderSlotDefaultState"] = json::standalone_value(stateVariant.renderSlotDefaultState);
        stateInfoJson["stateFlags"] = BitsToString(stateVariant.stateFlags, graphicsStateBitNames);

        // NOTE: assume that there is same amount of states as variants.
        const auto& state = saveInfo.states.at(i);
        json::standalone_value singleStateJson = json::standalone_value::object();

        // rasterization state
        singleStateJson["rasterizationState"] = MakeRasterizationStateJson(state.rasterizationState);
        // depth stencil state
        singleStateJson["depthStencilState"] = MakeDepthStencilStateJson(state.depthStencilState);
        // color blend state
        singleStateJson["colorBlendState"] = MakeColorBlendStateJson(state.colorBlendState);
        // input assembly
        singleStateJson["inputAssembly"] = MakeInputAssemblyJson(state.inputAssembly);

        stateInfoJson["state"] = BASE_NS::move(singleStateJson);
        sgJson["shaderStates"].array_.push_back(BASE_NS::move(stateInfoJson));
    }

    r.result = to_string(sgJson);
    r.success = true;

    return r;
}

IShaderManager::ShaderOutWriteResult SaveVextexInputDeclarations(
    const IShaderManager::ShaderVertexInputDeclarationsSaveInfo& saveInfo)
{
    // .shadervid file type
    IShaderManager::ShaderOutWriteResult r;

    json::standalone_value vidsJson = json::standalone_value::object();
    vidsJson["combatibility_info"] = json::standalone_value::object();
    vidsJson["combatibility_info"]["version"] = "22.00";
    vidsJson["combatibility_info"]["type"] = "vertexinputdeclaration";
    vidsJson["vertexInputState"] = json::standalone_value::object();
    json::standalone_value vertexInputStateJson = vidsJson["vertexInputState"];
    vertexInputStateJson["vertexInputBindingDescriptions"] = json::standalone_value::array();
    vertexInputStateJson["vertexInputAttributeDescriptions"] = json::standalone_value::array();

    const auto& vid = saveInfo.vid;
    for (auto bd : vid.bindingDescriptions) {
        json::standalone_value bindingDescriptionJson = json::standalone_value::object();
        bindingDescriptionJson["binding"] = bd.binding;
        bindingDescriptionJson["stride"] = bd.stride;
        switch (bd.vertexInputRate) {
            case CORE_VERTEX_INPUT_RATE_VERTEX:
                bindingDescriptionJson["vertexInputRate"] = "vertex";
                break;
            case CORE_VERTEX_INPUT_RATE_INSTANCE:
                bindingDescriptionJson["vertexInputRate"] = "instance";
                break;
            default:
                bindingDescriptionJson["vertexInputRate"] = "vertex";
                break;
        }
        vertexInputStateJson["vertexInputBindingDescriptions"].array_.emplace_back(bindingDescriptionJson);
    }

    for (auto ad : vid.attributeDescriptions) {
        json::standalone_value attrDescriptionJson = json::standalone_value::object();
        attrDescriptionJson["location"] = ad.location;
        attrDescriptionJson["binding"] = ad.binding;
        attrDescriptionJson["format"] = formatToString.at(ad.format);
        attrDescriptionJson["offset"] = ad.offset;
        vertexInputStateJson["vertexInputAttributeDescriptions"].array_.emplace_back(attrDescriptionJson);
    }

    vidsJson["vertexInputState"] = BASE_NS::move(vertexInputStateJson);

    r.result = to_string(vidsJson);
    r.success = true;

    return r;
}

IShaderManager::ShaderOutWriteResult SavePipelineLayouts(const IShaderManager::ShaderPipelineLayoutSaveInfo& saveInfo)
{
    // .shaderpl file type
    IShaderManager::ShaderOutWriteResult r;

    json::standalone_value pipelineLayoutJson = json::standalone_value::object();
    pipelineLayoutJson["combatibility_info"] = json::standalone_value::object();
    pipelineLayoutJson["combatibility_info"]["version"] = "22.00";
    pipelineLayoutJson["combatibility_info"]["type"] = "pipelinelayout";
    pipelineLayoutJson["descriptorSetLayouts"] = json::standalone_value::array();

    for (const auto& descriptorSetLayout : saveInfo.layout.descriptorSetLayouts) {
        if (descriptorSetLayout.set == PipelineLayoutConstants::INVALID_INDEX) {
            continue;
        }
        json::standalone_value descriptorSetJson = json::standalone_value::object();
        descriptorSetJson["set"] = descriptorSetLayout.set;
        descriptorSetJson["bindings"] = json::standalone_value::array();
        for (const auto& binding : descriptorSetLayout.bindings) {
            json::standalone_value bindingJson = json::standalone_value::object();
            bindingJson["binding"] = binding.binding;
            bindingJson["descriptorType"] = descriptorTypeToString.at(binding.descriptorType);
            bindingJson["descriptorCount"] = binding.descriptorCount;
            bindingJson["shaderStageFlags"] = ShaderStageBitsToString(binding.shaderStageFlags);
            descriptorSetJson["bindings"].array_.emplace_back(bindingJson);
        }

        descriptorSetJson["byteSize"] = saveInfo.layout.pushConstant.byteSize;

        descriptorSetJson["shaderStageFlags"] = ShaderStageBitsToString(saveInfo.layout.pushConstant.shaderStageFlags);

        pipelineLayoutJson["descriptorSetLayouts"].array_.push_back(BASE_NS::move(descriptorSetJson));
    }

    r.result = to_string(pipelineLayoutJson);
    r.success = true;

    return r;
}

IShaderManager::ShaderOutWriteResult SaveVariants(const IShaderManager::ShaderVariantsSaveInfo& saveInfo)
{
    // .shader file type
    IShaderManager::ShaderOutWriteResult r;
    json::standalone_value shadersJson = json::standalone_value::object();
    shadersJson["combatibility_info"] = json::standalone_value::object();
    shadersJson["combatibility_info"]["version"] = "22.00";
    shadersJson["combatibility_info"]["type"] = "shader";
    shadersJson["category"] = string(saveInfo.category);
    // empty means that it is itself, NOTE: will be adjusted later.
    shadersJson["baseShader"] = "";
    shadersJson["shaders"] = json::standalone_value::array();

    for (const auto& shader : saveInfo.shaders) {
        json::standalone_value shaderJson = json::standalone_value::object();
        shaderJson["materialMetadata"] = shader.materialMetadata;
        shaderJson["displayName"] = shader.displayName;
        shaderJson["variantName"] = shader.variantName;
        shaderJson["renderSlot"] = shader.renderSlot;
        shaderJson["renderSlotDefaultShader"] = json::standalone_value(shader.renderSlotDefaultShader);
        for (const auto& s : shader.shaders) {
            switch (s.shaderType) {
                case CORE_SHADER_STAGE_VERTEX_BIT:
                    shaderJson["vert"] = s.shaderSpvPath;
                    break;
                case CORE_SHADER_STAGE_FRAGMENT_BIT:
                    shaderJson["frag"] = s.shaderSpvPath;
                    break;
                case CORE_SHADER_STAGE_COMPUTE_BIT:
                    shaderJson["comp"] = s.shaderSpvPath;
                    break;
            }
        }
        shaderJson["vertexInputDeclaration"] = shader.vertexInputDeclaration;
        shaderJson["pipelineLayout"] = shader.pipelineLayout;
        shaderJson["state"] = json::standalone_value::object();
        // graphicsState
        shaderJson["state"]["rasterizationState"] = MakeRasterizationStateJson(shader.graphicsState.rasterizationState);
        shaderJson["state"]["depthStencilState"] = MakeDepthStencilStateJson(shader.graphicsState.depthStencilState);
        shaderJson["state"]["colorBlendState"] = MakeColorBlendStateJson(shader.graphicsState.colorBlendState);
        shaderJson["state"]["inputAssembly"] = MakeInputAssemblyJson(shader.graphicsState.inputAssembly);
        shaderJson["stateFlags"] = BitsToString(shader.stateFlags, graphicsStateBitNames);
        shadersJson["shaders"].array_.push_back(BASE_NS::move(shaderJson));
    }

    r.result = to_string(shadersJson);
    r.success = true;
    return r;
}

RENDER_END_NAMESPACE()
