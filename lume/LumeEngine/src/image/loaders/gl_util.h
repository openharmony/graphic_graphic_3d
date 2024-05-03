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

#ifndef CORE_GL_UTIL_H
#define CORE_GL_UTIL_H

#include <cstdint>

#include <base/namespace.h>
#include <base/util/formats.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()

// Define Some GL enums so that this can be used even without gl headers.
constexpr unsigned int GL_NONE = 0x0;
constexpr unsigned int GL_INVALID_VALUE = 0x0501;

//
// 8 bits per component formats.
//
constexpr unsigned int GL_R8 = 0x8229;
constexpr unsigned int GL_RG8 = 0x822B;
constexpr unsigned int GL_RGB8 = 0x8051;
constexpr unsigned int GL_RGBA8 = 0x8058;
constexpr unsigned int GL_R8_SNORM = 0x8F94;
constexpr unsigned int GL_RG8_SNORM = 0x8F95;
constexpr unsigned int GL_RGB8_SNORM = 0x8F96;
constexpr unsigned int GL_RGBA8_SNORM = 0x8F97;
constexpr unsigned int GL_R8UI = 0x8232;
constexpr unsigned int GL_RG8UI = 0x8238;
constexpr unsigned int GL_RGB8UI = 0x8D7D;
constexpr unsigned int GL_RGBA8UI = 0x8D7C;
constexpr unsigned int GL_R8I = 0x8231;
constexpr unsigned int GL_RG8I = 0x8237;
constexpr unsigned int GL_RGB8I = 0x8D8F;
constexpr unsigned int GL_RGBA8I = 0x8D8E;
constexpr unsigned int GL_SR8 = 0x8FBD;
constexpr unsigned int GL_SRG8 = 0x8FBE;
constexpr unsigned int GL_SRGB8 = 0x8C41;
constexpr unsigned int GL_SRGB8_ALPHA8 = 0x8C43;

//
// 16 bits per component formats.
//
constexpr unsigned int GL_R16 = 0x822A;
constexpr unsigned int GL_RG16 = 0x822C;
constexpr unsigned int GL_RGB16 = 0x8054;
constexpr unsigned int GL_RGBA16 = 0x805B;
constexpr unsigned int GL_R16_SNORM = 0x8F98;
constexpr unsigned int GL_RG16_SNORM = 0x8F99;
constexpr unsigned int GL_RGB16_SNORM = 0x8F9A;
constexpr unsigned int GL_RGBA16_SNORM = 0x8F9B;

constexpr unsigned int GL_R16UI = 0x8234;
constexpr unsigned int GL_RG16UI = 0x823A;
constexpr unsigned int GL_RGB16UI = 0x8D77;
constexpr unsigned int GL_RGBA16UI = 0x8D76;

constexpr unsigned int GL_R16I = 0x8233;
constexpr unsigned int GL_RG16I = 0x8239;
constexpr unsigned int GL_RGB16I = 0x8D89;
constexpr unsigned int GL_RGBA16I = 0x8D88;

constexpr unsigned int GL_R16F = 0x822D;
constexpr unsigned int GL_RG16F = 0x822F;
constexpr unsigned int GL_RGB16F = 0x881B;
constexpr unsigned int GL_RGBA16F = 0x881A;

//
// 32 bits per component formats.
//
constexpr unsigned int GL_R32UI = 0x8236;
constexpr unsigned int GL_RG32UI = 0x823C;
constexpr unsigned int GL_RGB32UI = 0x8D71;
constexpr unsigned int GL_RGBA32UI = 0x8D70;

constexpr unsigned int GL_R32I = 0x8235;
constexpr unsigned int GL_RG32I = 0x823B;
constexpr unsigned int GL_RGB32I = 0x8D83;
constexpr unsigned int GL_RGBA32I = 0x8D82;

constexpr unsigned int GL_R32F = 0x822E;
constexpr unsigned int GL_RG32F = 0x8230;
constexpr unsigned int GL_RGB32F = 0x8815;
constexpr unsigned int GL_RGBA32F = 0x8814;

//
// Packed formats.
//
constexpr unsigned int GL_R3_G3_B2 = 0x2A10;
constexpr unsigned int GL_RGB4 = 0x804F;
constexpr unsigned int GL_RGB5 = 0x8050;
constexpr unsigned int GL_RGB565 = 0x8D62;
constexpr unsigned int GL_RGB10 = 0x8052;
constexpr unsigned int GL_RGB12 = 0x8053;
constexpr unsigned int GL_RGBA2 = 0x8055;
constexpr unsigned int GL_RGBA4 = 0x8056;
constexpr unsigned int GL_RGBA12 = 0x805A;
constexpr unsigned int GL_RGB5_A1 = 0x8057;
constexpr unsigned int GL_RGB10_A2 = 0x8059;
constexpr unsigned int GL_RGB10_A2UI = 0x906F;
constexpr unsigned int GL_R11F_G11F_B10F = 0x8C3A;
constexpr unsigned int GL_RGB9_E5 = 0x8C3D;

//
// Luminance formats.
//
constexpr unsigned int GL_LUMINANCE4 = 0x803F;
constexpr unsigned int GL_LUMINANCE8 = 0x8040;
constexpr unsigned int GL_LUMINANCE8_SNORM = 0x9015;
constexpr unsigned int GL_SLUMINANCE8 = 0x8C47;
constexpr unsigned int GL_LUMINANCE8UI_EXT = 0x8D80;
constexpr unsigned int GL_LUMINANCE8I_EXT = 0x8D92;
constexpr unsigned int GL_LUMINANCE12 = 0x8041;
constexpr unsigned int GL_LUMINANCE16 = 0x8042;
constexpr unsigned int GL_LUMINANCE16_SNORM = 0x9019;
constexpr unsigned int GL_LUMINANCE16UI_EXT = 0x8D7A;
constexpr unsigned int GL_LUMINANCE16I_EXT = 0x8D8C;
constexpr unsigned int GL_LUMINANCE16F_ARB = 0x881E;
constexpr unsigned int GL_LUMINANCE32UI_EXT = 0x8D74;
constexpr unsigned int GL_LUMINANCE32I_EXT = 0x8D86;
constexpr unsigned int GL_LUMINANCE32F_ARB = 0x8818;

//
// Luminance/Alpha
//
constexpr unsigned int GL_LUMINANCE4_ALPHA4 = 0x8043;
constexpr unsigned int GL_LUMINANCE6_ALPHA2 = 0x8044;
constexpr unsigned int GL_LUMINANCE8_ALPHA8 = 0x8045;
constexpr unsigned int GL_LUMINANCE8_ALPHA8_SNORM = 0x9016;
constexpr unsigned int GL_SLUMINANCE8_ALPHA8 = 0x8C45;
constexpr unsigned int GL_LUMINANCE_ALPHA8UI_EXT = 0x8D81;
constexpr unsigned int GL_LUMINANCE_ALPHA8I_EXT = 0x8D93;
constexpr unsigned int GL_LUMINANCE12_ALPHA4 = 0x8046;
constexpr unsigned int GL_LUMINANCE12_ALPHA12 = 0x8047;
constexpr unsigned int GL_LUMINANCE16_ALPHA16 = 0x8048;
constexpr unsigned int GL_LUMINANCE16_ALPHA16_SNORM = 0x901A;
constexpr unsigned int GL_LUMINANCE_ALPHA16UI_EXT = 0x8D7B;
constexpr unsigned int GL_LUMINANCE_ALPHA16I_EXT = 0x8D8D;
constexpr unsigned int GL_LUMINANCE_ALPHA16F_ARB = 0x881F;
constexpr unsigned int GL_LUMINANCE_ALPHA32UI_EXT = 0x8D75;
constexpr unsigned int GL_LUMINANCE_ALPHA32I_EXT = 0x8D87;
constexpr unsigned int GL_LUMINANCE_ALPHA32F_ARB = 0x8819;

//
// Intensity
//
constexpr unsigned int GL_INTENSITY4 = 0x804A;
constexpr unsigned int GL_INTENSITY8 = 0x804B;
constexpr unsigned int GL_INTENSITY8_SNORM = 0x9017;
constexpr unsigned int GL_INTENSITY8UI_EXT = 0x8D7F;
constexpr unsigned int GL_INTENSITY8I_EXT = 0x8D91;
constexpr unsigned int GL_INTENSITY12 = 0x804C;
constexpr unsigned int GL_INTENSITY16 = 0x804D;
constexpr unsigned int GL_INTENSITY16_SNORM = 0x901B;
constexpr unsigned int GL_INTENSITY16UI_EXT = 0x8D79;
constexpr unsigned int GL_INTENSITY16I_EXT = 0x8D8B;
constexpr unsigned int GL_INTENSITY16F_ARB = 0x881D;
constexpr unsigned int GL_INTENSITY32UI_EXT = 0x8D73;
constexpr unsigned int GL_INTENSITY32I_EXT = 0x8D85;
constexpr unsigned int GL_INTENSITY32F_ARB = 0x8817;

//
// Generic compressed.
//
constexpr unsigned int GL_COMPRESSED_RED = 0x8225;
constexpr unsigned int GL_COMPRESSED_ALPHA = 0x84E9;
constexpr unsigned int GL_COMPRESSED_LUMINANCE = 0x84EA;
constexpr unsigned int GL_COMPRESSED_SLUMINANCE = 0x8C4A;
constexpr unsigned int GL_COMPRESSED_LUMINANCE_ALPHA = 0x84EB;
constexpr unsigned int GL_COMPRESSED_SLUMINANCE_ALPHA = 0x8C4B;
constexpr unsigned int GL_COMPRESSED_INTENSITY = 0x84EC;
constexpr unsigned int GL_COMPRESSED_RG = 0x8226;
constexpr unsigned int GL_COMPRESSED_RGB = 0x84ED;
constexpr unsigned int GL_COMPRESSED_RGBA = 0x84EE;
constexpr unsigned int GL_COMPRESSED_SRGB = 0x8C48;
constexpr unsigned int GL_COMPRESSED_SRGB_ALPHA = 0x8C49;

//
// S3TC/DXT/BC compressed.
//
constexpr unsigned int GL_COMPRESSED_RGB_S3TC_DXT1_EXT = 0x83F0;
constexpr unsigned int GL_COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1;
constexpr unsigned int GL_COMPRESSED_RGBA_S3TC_DXT3_EXT = 0x83F2;
constexpr unsigned int GL_COMPRESSED_RGBA_S3TC_DXT5_EXT = 0x83F3;

constexpr unsigned int GL_COMPRESSED_SRGB_S3TC_DXT1_EXT = 0x8C4C;
constexpr unsigned int GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT = 0x8C4D;
constexpr unsigned int GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT = 0x8C4E;
constexpr unsigned int GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT = 0x8C4F;

constexpr unsigned int GL_COMPRESSED_LUMINANCE_LATC1_EXT = 0x8C70;
constexpr unsigned int GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT = 0x8C72;
constexpr unsigned int GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT = 0x8C71;
constexpr unsigned int GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT = 0x8C73;

constexpr unsigned int GL_COMPRESSED_RED_RGTC1 = 0x8DBB;
constexpr unsigned int GL_COMPRESSED_RG_RGTC2 = 0x8DBD;
constexpr unsigned int GL_COMPRESSED_SIGNED_RED_RGTC1 = 0x8DBC;
constexpr unsigned int GL_COMPRESSED_SIGNED_RG_RGTC2 = 0x8DBE;

constexpr unsigned int GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT = 0x8E8E;
constexpr unsigned int GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F;
constexpr unsigned int GL_COMPRESSED_RGBA_BPTC_UNORM = 0x8E8C;
constexpr unsigned int GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM = 0x8E8D;

//
// ETC compressed.
//
constexpr unsigned int GL_ETC1_RGB8_OES = 0x8D64;

constexpr unsigned int GL_COMPRESSED_RGB8_ETC2 = 0x9274;
constexpr unsigned int GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9276;
constexpr unsigned int GL_COMPRESSED_RGBA8_ETC2_EAC = 0x9278;

constexpr unsigned int GL_COMPRESSED_SRGB8_ETC2 = 0x9275;
constexpr unsigned int GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9277;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = 0x9279;

constexpr unsigned int GL_COMPRESSED_R11_EAC = 0x9270;
constexpr unsigned int GL_COMPRESSED_RG11_EAC = 0x9272;
constexpr unsigned int GL_COMPRESSED_SIGNED_R11_EAC = 0x9271;
constexpr unsigned int GL_COMPRESSED_SIGNED_RG11_EAC = 0x9273;

//
// ASTC compressed.
//
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_4x4_KHR = 0x93B0;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_5x4_KHR = 0x93B1;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_5x5_KHR = 0x93B2;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_6x5_KHR = 0x93B3;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_6x6_KHR = 0x93B4;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_8x5_KHR = 0x93B5;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_8x6_KHR = 0x93B6;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_8x8_KHR = 0x93B7;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_10x5_KHR = 0x93B8;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_10x6_KHR = 0x93B9;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_10x8_KHR = 0x93BA;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_10x10_KHR = 0x93BB;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_12x10_KHR = 0x93BC;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_12x12_KHR = 0x93BD;

constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR = 0x93D0;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR = 0x93D1;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR = 0x93D2;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR = 0x93D3;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR = 0x93D4;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR = 0x93D5;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR = 0x93D6;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR = 0x93D7;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR = 0x93D8;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR = 0x93D9;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR = 0x93DA;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR = 0x93DB;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR = 0x93DC;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR = 0x93DD;

constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_3x3x3_OES = 0x93C0;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_4x3x3_OES = 0x93C1;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_4x4x3_OES = 0x93C2;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_4x4x4_OES = 0x93C3;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_5x4x4_OES = 0x93C4;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_5x5x4_OES = 0x93C5;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_5x5x5_OES = 0x93C6;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_6x5x5_OES = 0x93C7;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_6x6x5_OES = 0x93C8;
constexpr unsigned int GL_COMPRESSED_RGBA_ASTC_6x6x6_OES = 0x93C9;

constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES = 0x93E0;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES = 0x93E1;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES = 0x93E2;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES = 0x93E3;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES = 0x93E4;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES = 0x93E5;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES = 0x93E6;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES = 0x93E7;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES = 0x93E8;
constexpr unsigned int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES = 0x93E9;

struct GlImageFormatInfo {
    BASE_NS::Format coreFormat;
    BASE_NS::Format coreFormatForceSrgb;
    BASE_NS::Format coreFormatForceLinear;

    unsigned int glFormat;

    bool compressed;
    uint8_t blockWidth;
    uint8_t blockHeight;
    uint8_t blockDepth;
    uint32_t bitsPerBlock;
};

// NOTE: Only a subset of formats are supported. E.g. 3D formats are not listed.
static constexpr GlImageFormatInfo GL_IMAGE_FORMATS[] = {
    // 8 bits per component.
    { BASE_NS::Format::BASE_FORMAT_R8_UNORM, BASE_NS::Format::BASE_FORMAT_R8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8_UNORM, GL_R8, false, 1, 1, 1, 8 },
    { BASE_NS::Format::BASE_FORMAT_R8G8_UNORM, BASE_NS::Format::BASE_FORMAT_R8G8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8_UNORM, GL_RG8, false, 1, 1, 1, 16 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8_UNORM, BASE_NS::Format::BASE_FORMAT_R8G8B8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8_UNORM, GL_RGB8, false, 1, 1, 1, 24 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM, BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM, GL_RGBA8, false, 1, 1, 1, 32 },

    { BASE_NS::Format::BASE_FORMAT_R8_UINT, BASE_NS::Format::BASE_FORMAT_R8_SRGB, BASE_NS::Format::BASE_FORMAT_R8_UINT,
        GL_R8UI, false, 1, 1, 1, 8 },
    { BASE_NS::Format::BASE_FORMAT_R8G8_UINT, BASE_NS::Format::BASE_FORMAT_R8G8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8_UINT, GL_RG8UI, false, 1, 1, 1, 16 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8_UINT, BASE_NS::Format::BASE_FORMAT_R8G8B8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8_UINT, GL_RGB8UI, false, 1, 1, 1, 24 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UINT, BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UINT, GL_RGBA8UI, false, 1, 1, 1, 32 },

    { BASE_NS::Format::BASE_FORMAT_R8_SINT, BASE_NS::Format::BASE_FORMAT_R8_SRGB, BASE_NS::Format::BASE_FORMAT_R8_SINT,
        GL_R8I, false, 1, 1, 1, 8 },
    { BASE_NS::Format::BASE_FORMAT_R8G8_SINT, BASE_NS::Format::BASE_FORMAT_R8G8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8_SINT, GL_RG8I, false, 1, 1, 1, 16 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8_SINT, BASE_NS::Format::BASE_FORMAT_R8G8B8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8_SINT, GL_RGB8I, false, 1, 1, 1, 24 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SINT, BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SINT, GL_RGBA8I, false, 1, 1, 1, 32 },

    { BASE_NS::Format::BASE_FORMAT_R8_SRGB, BASE_NS::Format::BASE_FORMAT_R8_SRGB, BASE_NS::Format::BASE_FORMAT_R8_UNORM,
        GL_SR8, false, 1, 1, 1, 8 },
    { BASE_NS::Format::BASE_FORMAT_R8G8_SRGB, BASE_NS::Format::BASE_FORMAT_R8G8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8_UNORM, GL_SRG8, false, 1, 1, 1, 16 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8_SRGB, BASE_NS::Format::BASE_FORMAT_R8G8B8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8_UNORM, GL_SRGB8, false, 1, 1, 1, 24 },
    { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB, BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB,
        BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM, GL_SRGB8_ALPHA8, false, 1, 1, 1, 32 },

    // 16 bits per component.
    { BASE_NS::Format::BASE_FORMAT_R16_UNORM, BASE_NS::Format::BASE_FORMAT_R16_UNORM,
        BASE_NS::Format::BASE_FORMAT_R16_UNORM, GL_R16, false, 1, 1, 1, 16 },
    { BASE_NS::Format::BASE_FORMAT_R16G16_UNORM, BASE_NS::Format::BASE_FORMAT_R16G16_UNORM,
        BASE_NS::Format::BASE_FORMAT_R16G16_UNORM, GL_RG16, false, 1, 1, 1, 32 },
    { BASE_NS::Format::BASE_FORMAT_R16G16B16_UNORM, BASE_NS::Format::BASE_FORMAT_R16G16B16_UNORM,
        BASE_NS::Format::BASE_FORMAT_R16G16B16_UNORM, GL_RGB16, false, 1, 1, 1, 48 },
    { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_UNORM, BASE_NS::Format::BASE_FORMAT_R16G16B16A16_UNORM,
        BASE_NS::Format::BASE_FORMAT_R16G16B16A16_UNORM, GL_RGBA16, false, 1, 1, 1, 64 },

    { BASE_NS::Format::BASE_FORMAT_R16_SFLOAT, BASE_NS::Format::BASE_FORMAT_R16_UNORM,
        BASE_NS::Format::BASE_FORMAT_R16_UNORM, GL_R16F, false, 1, 1, 1, 16 },
    { BASE_NS::Format::BASE_FORMAT_R16G16_SFLOAT, BASE_NS::Format::BASE_FORMAT_R16G16_SFLOAT,
        BASE_NS::Format::BASE_FORMAT_R16G16_SFLOAT, GL_RG16F, false, 1, 1, 1, 32 },
    { BASE_NS::Format::BASE_FORMAT_R16G16B16_SFLOAT, BASE_NS::Format::BASE_FORMAT_R16G16B16_SFLOAT,
        BASE_NS::Format::BASE_FORMAT_R16G16B16_SFLOAT, GL_RGB16F, false, 1, 1, 1, 48 },
    { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SFLOAT, BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SFLOAT,
        BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SFLOAT, GL_RGBA16F, false, 1, 1, 1, 64 },

    // 32 bits per component.
    { BASE_NS::Format::BASE_FORMAT_R32_SFLOAT, BASE_NS::Format::BASE_FORMAT_R32_SFLOAT,
        BASE_NS::Format::BASE_FORMAT_R32_SFLOAT, GL_R32F, false, 1, 1, 1, 32 },
    { BASE_NS::Format::BASE_FORMAT_R32G32_SFLOAT, BASE_NS::Format::BASE_FORMAT_R32G32_SFLOAT,
        BASE_NS::Format::BASE_FORMAT_R32G32_SFLOAT, GL_RG32F, false, 1, 1, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_R32G32B32_SFLOAT, BASE_NS::Format::BASE_FORMAT_R32G32B32_SFLOAT,
        BASE_NS::Format::BASE_FORMAT_R32G32B32_SFLOAT, GL_RGB32F, false, 1, 1, 1, 96 },
    { BASE_NS::Format::BASE_FORMAT_R32G32B32A32_SFLOAT, BASE_NS::Format::BASE_FORMAT_R32G32B32A32_SFLOAT,
        BASE_NS::Format::BASE_FORMAT_R32G32B32A32_SFLOAT, GL_RGBA32F, false, 1, 1, 1, 128 },

    // Packed formats.
    { BASE_NS::Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32, BASE_NS::Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32,
        BASE_NS::Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32, GL_R11F_G11F_B10F, false, 1, 1, 1, 32 },

    // Compressed formats.
    { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, GL_ETC1_RGB8_OES, true, 4, 4, 1,
        64 }, // ETC1 is compatible with ETC2

    { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, GL_COMPRESSED_RGB8_ETC2, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, true, 4, 4,
        1, 64 },
    { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, GL_COMPRESSED_RGBA8_ETC2_EAC, true, 4, 4, 1, 128 },

    { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ETC2, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, true, 4,
        4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, true, 4, 4, 1,
        128 },

    { BASE_NS::Format::BASE_FORMAT_EAC_R11_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_EAC_R11_UNORM_BLOCK,
        BASE_NS::Format::BASE_FORMAT_EAC_R11_UNORM_BLOCK, GL_COMPRESSED_R11_EAC, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_EAC_R11G11_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_EAC_R11G11_UNORM_BLOCK,
        BASE_NS::Format::BASE_FORMAT_EAC_R11G11_UNORM_BLOCK, GL_COMPRESSED_RG11_EAC, true, 4, 4, 1, 128 },

    { BASE_NS::Format::BASE_FORMAT_EAC_R11_SNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_EAC_R11_SNORM_BLOCK,
        BASE_NS::Format::BASE_FORMAT_EAC_R11_SNORM_BLOCK, GL_COMPRESSED_SIGNED_R11_EAC, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_EAC_R11G11_SNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_EAC_R11G11_SNORM_BLOCK,
        BASE_NS::Format::BASE_FORMAT_EAC_R11G11_SNORM_BLOCK, GL_COMPRESSED_SIGNED_RG11_EAC, true, 4, 4, 1, 128 },

    // ASTC formats.
    { BASE_NS::Format::BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_4x4_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_4x4_KHR, true, 4, 4, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_5x4_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_5x4_KHR, true, 5, 4, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_5x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_5x5_KHR, true, 5, 5, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_6x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_6x5_KHR, true, 6, 5, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_6x6_KHR, true, 6, 6, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_8x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_8x5_KHR, true, 8, 5, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_8x6_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_8x6_KHR, true, 8, 6, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_8x8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_8x8_KHR, true, 8, 8, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_10x5_KHR, true, 10, 5, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x6_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_10x6_KHR, true, 10, 6, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_10x8_KHR, true, 10, 8, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x10_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_10x10_KHR, true, 10, 10, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_12x10_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_12x10_KHR, true, 12, 10, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, GL_COMPRESSED_RGBA_ASTC_12x12_KHR, true, 12, 12, 1, 128 },

    { BASE_NS::Format::BASE_FORMAT_ASTC_4x4_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_4x4_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR, true, 4, 4, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_5x4_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_5x4_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR, true, 5, 4, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_5x5_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_5x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR, true, 5, 5, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_6x5_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_6x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR, true, 6, 5, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR, true, 6, 6, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_8x5_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_8x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR, true, 8, 5, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_8x6_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_8x6_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR, true, 8, 6, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_8x8_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_8x8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR, true, 8, 8, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x5_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x5_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR, true, 10, 5, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x6_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x6_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR, true, 10, 6, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x8_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x8_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR, true, 10, 8, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_10x10_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_10x10_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR, true, 10, 10, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_12x10_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_12x10_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, true, 12, 10, 1,
        128 },
    { BASE_NS::Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR, true, 12, 12, 1,
        128 },

    // S3TC/DXT/BC formats.
    { BASE_NS::Format::BASE_FORMAT_BC1_RGB_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_BC1_RGB_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC1_RGB_UNORM_BLOCK, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_BC1_RGBA_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_BC1_RGBA_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC1_RGBA_UNORM_BLOCK, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_BC2_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_BC2_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC2_UNORM_BLOCK, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, true, 4, 4, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_BC3_UNORM_BLOCK, BASE_NS::Format::BASE_FORMAT_BC3_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC3_UNORM_BLOCK, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, true, 4, 4, 1, 128 },

    { BASE_NS::Format::BASE_FORMAT_BC1_RGB_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_BC1_RGB_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC1_RGB_UNORM_BLOCK, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_BC1_RGBA_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_BC1_RGBA_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC1_RGBA_UNORM_BLOCK, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, true, 4, 4, 1, 64 },
    { BASE_NS::Format::BASE_FORMAT_BC2_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_BC2_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC2_UNORM_BLOCK, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, true, 4, 4, 1, 128 },
    { BASE_NS::Format::BASE_FORMAT_BC3_SRGB_BLOCK, BASE_NS::Format::BASE_FORMAT_BC3_SRGB_BLOCK,
        BASE_NS::Format::BASE_FORMAT_BC3_UNORM_BLOCK, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, true, 4, 4, 1, 128 },

    // Terminator
    { BASE_NS::Format::BASE_FORMAT_UNDEFINED, BASE_NS::Format::BASE_FORMAT_UNDEFINED,
        BASE_NS::Format::BASE_FORMAT_UNDEFINED, GL_NONE, false, 0, 0, 0, 0 },
};

GlImageFormatInfo GetFormatInfo(const uint32_t glFormat)
{
    int i = 0;
    for (;; i++) {
        if ((GL_IMAGE_FORMATS[i].coreFormat == BASE_NS::Format::BASE_FORMAT_UNDEFINED) ||
            (glFormat == GL_IMAGE_FORMATS[i].glFormat)) {
            break;
        }
    }
    return GL_IMAGE_FORMATS[i];
}
CORE_END_NAMESPACE()

#endif // CORE_GL_UTIL_H
