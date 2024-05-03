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

#ifndef SHADERS__COMMON__3D_DEFAULT_MATERIAL_PACKING_COMMON_H
#define SHADERS__COMMON__3D_DEFAULT_MATERIAL_PACKING_COMMON_H

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"

#ifdef VULKAN

// octahedron-normal vector encoding
// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec2 OctWrap(in vec2 v)
{
    return (1.0 - abs(v.yx)) * vec2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
}
vec2 NormalOctEncode(in vec3 normal)
{
    vec3 n = normal;
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}
vec3 NormalOctDecode(in vec2 f)
{
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.xy += vec2((n.x >= 0.0) ? -t : t, (n.y >= 0.0) ? -t : t);
    return normalize(n);
}

#define CORE_MATERIAL_DF_SHADOW_RECEIVER_BIT (1 << 0)
#define CORE_MATERIAL_DF_PUNCTUAL_LIGHT_RECEIVER_BIT (1 << 1)
#define CORE_MATERIAL_DF_INDIRECT_LIGHT_RECEIVER_BIT (1 << 2)
#define CORE_MATERIAL_DF_MATERIAL_FLAG_SHIFT 3
#define CORE_MATERIAL_DF_MATERIAL_TYPE_MASK 7
#define CORE_MATERIAL_DF_REFLECTANCE_COEFF 10.0

uint GetDeferredMaterialFlagsFromMaterialFlags(uint materialFlags)
{
    uint mat = 0;
    mat |= ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT)
               ? CORE_MATERIAL_DF_SHADOW_RECEIVER_BIT
               : 0;
    mat |= ((materialFlags & CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) == CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT)
               ? CORE_MATERIAL_DF_PUNCTUAL_LIGHT_RECEIVER_BIT
               : 0;
    mat |= ((materialFlags & CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) == CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT)
               ? CORE_MATERIAL_DF_INDIRECT_LIGHT_RECEIVER_BIT
               : 0;
    return mat;
}

uint GetMaterialFlagsFromDeferredMaterialFlags(uint materialFlags)
{
    uint mat = 0;
    mat |= ((materialFlags & CORE_MATERIAL_DF_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_DF_SHADOW_RECEIVER_BIT)
               ? CORE_MATERIAL_SHADOW_RECEIVER_BIT
               : 0;
    mat |=
        ((materialFlags & CORE_MATERIAL_DF_PUNCTUAL_LIGHT_RECEIVER_BIT) == CORE_MATERIAL_DF_PUNCTUAL_LIGHT_RECEIVER_BIT)
            ? CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT
            : 0;
    mat |=
        ((materialFlags & CORE_MATERIAL_DF_INDIRECT_LIGHT_RECEIVER_BIT) == CORE_MATERIAL_DF_INDIRECT_LIGHT_RECEIVER_BIT)
            ? CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT
            : 0;
    return mat;
}

#define ENABLE_NORMAL_OCTAHEDRON_PACKING 1

vec4 GetPackColor(in vec4 col)
{
    return col;
}

vec4 GetPackBaseColorWithAo(in vec3 col, in float ao)
{
    return vec4(col.xyz, ao);
}

vec3 GetPackNormal(in vec3 normal)
{
#if (ENABLE_NORMAL_OCTAHEDRON_PACKING == 1)
    return vec3(NormalOctEncode(normal), 0.0);
#else
    return vec3(normal * 0.5 + 0.5);
#endif
}

vec4 GetPackMaterialWithFlags(in vec4 material, in uint materialType, in uint materialFlags)
{
    vec4 packed = material;
    const uint mf = GetDeferredMaterialFlagsFromMaterialFlags(materialFlags);
    const uint mat = (mf << CORE_MATERIAL_DF_MATERIAL_FLAG_SHIFT) | materialType;
    packed.r = float(mat) * (1.0 / 255.0);
    if (materialType == CORE_MATERIAL_METALLIC_ROUGHNESS) {
        // move reflectance and modify range
        packed.a = clamp(packed.a * CORE_MATERIAL_DF_REFLECTANCE_COEFF, 0.0, 1.0);
    }
    return packed;
}

vec2 GetPackVelocity(in vec2 velocity)
{
    return velocity;
}

vec4 GetPackVelocityAndNormal(in vec2 velocity, in vec3 normal)
{
    return vec4(velocity, NormalOctEncode(normal));
}

vec4 GetPackPbrColor(in vec3 color, in CORE_RELAXEDP float alpha)
{
    // zero to hdr max clamp and alpha multiply
    return vec4(clamp(color.rgb, 0.0, CORE3D_HDR_FLOAT_CLAMP_MAX_VALUE) * alpha, alpha);
}

// unpack

float GetUnpackDepthBuffer(in float depthSample)
{
    return depthSample;
}

void GetUnpackBaseColorWithAo(in vec4 col, out vec3 color, out float ao)
{
    color.rgb = col.rgb;
    ao = col.a;
}

vec3 GetUnpackNormal(in vec3 normal)
{
#if (ENABLE_NORMAL_OCTAHEDRON_PACKING == 1)
    return NormalOctDecode(normal.xy);
#else
    return normal.xyz * 2.0 - 1.0;
#endif
}

void GetUnpackMaterialWithFlags(in vec4 inMat, out vec4 material, out uint materialType, out uint materialFlags)
{
    material = inMat;
    const uint mat = uint(round(material.r * 255.0));
    materialType = mat & CORE_MATERIAL_DF_MATERIAL_TYPE_MASK;
    materialFlags = GetMaterialFlagsFromDeferredMaterialFlags(mat >> CORE_MATERIAL_DF_MATERIAL_FLAG_SHIFT);
    if (materialType == CORE_MATERIAL_METALLIC_ROUGHNESS) {
        // move reflectance and modify range
        material.a = material.a * (1.0 / CORE_MATERIAL_DF_REFLECTANCE_COEFF);
    }
}

vec2 GetUnpackVelocity(in vec2 velocity)
{
    return velocity;
}

void GetUnpackVelocityAndNormal(in vec4 packed, out vec2 velocity, out vec3 normal)
{
    velocity = packed.xy;
    normal = NormalOctDecode(packed.zw);
}

#else

#endif

#endif // SHADERS__COMMON__3D_DEFAULT_MATERIAL_PACKING_COMMON_H
