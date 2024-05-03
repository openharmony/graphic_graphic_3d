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

#ifndef SHADERS__COMMON__3D_DM_INDIRECT_LIGHTING_COMMON_H
#define SHADERS__COMMON__3D_DM_INDIRECT_LIGHTING_COMMON_H

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"

#ifdef VULKAN

#define CORE3D_BRDF_PI 3.14159265359
// Avoid divisions by 0 on devices that do not support denormals (from Filament documentation)
#define CORE3D_BRDF_MIN_ROUGHNESS 0.089
#define CORE3D_HDR_FLOAT_CLAMP_MAX_VALUE 64512.0
#define CORE3D_PBR_LIGHTING_EPSILON 0.0001

/*
Unpack environment color.
*/
// RGBD or just RGB -> ALPHA = 1.0
vec3 unpackEnvMap(vec4 envColorRgbd)
{
    return envColorRgbd.xyz * (1.0 / envColorRgbd.a);
}

// RGBD or just RGB -> ALPHA = 1.0
vec3 unpackIblRadiance(vec4 envColorRgbd)
{
    return envColorRgbd.xyz * (1.0 / envColorRgbd.a);
}

// build-in (1.0 / PI)
vec3 unpackIblIrradianceSH(vec3 n, vec4 sh[CORE_DEFAULT_MATERIAL_MAX_SH_VEC3_VALUE_COUNT])
{
    // use 3 bands spherical harmonics
    return max(vec3(0.0), sh[0].xyz

                              + sh[1].xyz * n.y + sh[2].xyz * n.z + sh[3].xyz * n.x

                              + sh[4].xyz * (n.x * n.y) + sh[5].xyz * (n.z * n.y) +
                              sh[6].xyz * ((3.0 * n.z * n.z) - 1.0) + sh[7].xyz * (n.x * n.z) +
                              sh[8].xyz * (n.x * n.x - n.y * n.y));
}

// https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
// alpha taken into account
vec3 EnvBRDFApprox(vec3 f0, float roughness, float NoV)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
    // f0 has baked transparency, ab.y needs to take alpha into account
    // 2% f0, takes into account alpha
    const float f90 = clamp(50.0 * max(f0.x, max(f0.y, f0.z)), 0.0, 1.0);
    return f0 * ab.x + ab.y * f90;
}

// https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
// Approximation of EnvBRDFApprox for non metallic (diaelectric) materials with reflectance value of 0.04
float EnvBRDFApproxNonmetal(float Roughness, float NoV)
{
    // Same as EnvBRDFApprox( 0.04, Roughness, NoV )
    const vec2 c0 = { -1, -0.0275 };
    const vec2 c1 = { 1, 0.0425 };
    vec2 r = Roughness * c0 + c1;
    return min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y; // 9.28 : param
}

#endif // VULKAN

#endif // SHADERS__COMMON__3D_DM_INDIRECT_LIGHTING_COMMON_H
