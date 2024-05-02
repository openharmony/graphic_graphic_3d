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

#ifndef SHADERS__COMMON__3D_DM_BRDF_COMMON_H
#define SHADERS__COMMON__3D_DM_BRDF_COMMON_H

/*
BRDF functions.
Follows http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
Some mobile optimizations from https://google.github.io/filament/Filament.html
*/
#define CORE_BRDF_PI 3.14159265359
// Avoid divisions by 0 on devices that do not support denormals (from Filament documentation)
#define CORE_BRDF_MIN_ROUGHNESS 0.089
#define CORE_BRDF_EPSILON 0.0001

#define CORE_HDR_FLOAT_CLAMP_MAX_VALUE 64512.0

float dLambert()
{
    return (1.0 / CORE_BRDF_PI);
}

// Sebastien Lagarde and Charles de Rousiers. 2014. Moving Frostbite to PBR.
// https://www.ea.com/frostbite/news/moving-frostbite-to-pb
float EnvSpecularAo(float ao, float NoV, float roughness)
{
    return clamp(pow(NoV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao, 0.0, 1.0);
}

float SpecularHorizonOcclusion(vec3 R, vec3 N)
{
    const float horizon = min(1.0 + dot(R, N), 1.0);
    return horizon * horizon;
}

float dAshikhmin(float roughness, float NoH)
{
    const float r2 = roughness * roughness;
    const float cos2h = NoH * NoH;
    const float sin2h = 1.0 - cos2h;
    const float sin4h = sin2h * sin2h;
    return (sin4h + 4.0 * exp(-cos2h / (sin2h * r2))) / (CORE_BRDF_PI * (1.0 + 4.0 * r2) * sin4h);
}

// includes microfaced BRDF denominator and geometry term
float vAshikhmin(float NoV, float NoL)
{
    return 1.0 / (4.0 * (NoL + NoV - NoL * NoL));
}

float dCharlie(float roughness, float NoH)
{
    const float invR = 1.0 / roughness;
    const float cos2h = NoH * NoH;
    const float sin2h = 1.0 - cos2h; // NOTE: should max to fp16
    return (2.0 + invR) * pow(sin2h, invR * 0.5) / (2.0 * CORE_BRDF_PI);
}

// compensation for underlaying surface
vec3 f0ClearcoatToSurface(const vec3 f0)
{
    // approximation of ior with value of 1.5
    return clamp(f0 * (f0 * 0.526868 + 0.529324) - 0.0482256, 0.0, 1.0);
}

// Approximation for sheen integral on hemisphere
float EnvBRDFApproxSheen(float NoV, float alpha)
{
    const float c = 1.0 - NoV;
    const float c3 = c * c * c;
    return 0.6558446 * c3 + 1.0 / (4.1652655 + exp(-7.9729136 * alpha + 6.3351689));
}

vec3 fSchlick(vec3 f0, float VoH)
{
    // optimized for scalars
    const float p = pow(1.0 - VoH, 5.0);
    return p + f0 * (1.0 - p);
}

float fSchlickSingle(float f0, float VoH)
{
    const float p = pow(1.0 - VoH, 5.0);
    return p + f0 * (1.0 - p);
}

// f0.xyz = f0, f0.w = f90
vec3 fSchlick(vec4 f0, float VoH)
{
    const float p = pow(1.0 - VoH, 5.0);
    return (f0.w * p) + f0.xyz * (1.0 - p);
}

// Normal distribution function, GGX (Trowbridge-Reitz), Walter et al. 2007.
float dGGX(float alpha2, float NoH)
{
    const float f = (NoH * alpha2 - NoH) * NoH + 1.0;
    return alpha2 / (CORE_BRDF_PI * (f * f));
}

// Normal distribution function for anisotropic based on Burley 2012 (Physically-Based Shading at Disney)
float dGGXAnisotropic(float at, float ab, float NoH, float ToH, float BoH, float anisotropy)
{
    float a2 = at * ab;
    vec3 d = vec3(ab * ToH, at * BoH, a2 * NoH);
    float d2 = dot(d, d);
    if (d2 == 0) {
        return 0.0;
    }
    float w2 = a2 / d2;
    return a2 * w2 * w2 * (1.0 / CORE_BRDF_PI);
}

// Geometric shadowing with combined BRDF denominator
float vGGXWithCombinedDenominator(float alpha2, float NoV, float NoL)
{
    const float gv = NoV + sqrt((NoV - NoV * alpha2) * NoV + alpha2);
    const float gl = NoL + sqrt((NoL - NoL * alpha2) * NoL + alpha2);
    return min(1.0 / (gv * gl), CORE_HDR_FLOAT_CLAMP_MAX_VALUE);
}

// "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
// Kelemen 2001
float vKelemen(float LoH)
{
    return min(0.25 / (LoH * LoH), CORE_HDR_FLOAT_CLAMP_MAX_VALUE);
}

float vGGXAnisotropic(
    float at, float ab, float NoL, float NoV, float ToL, float ToV, float BoL, float BoV, float anisotropy)
{
    float gv = NoL * length(vec3(at * ToV, ab * BoV, NoV));
    float gl = NoV * length(vec3(at * ToL, ab * BoL, NoL));
    float v = 0.5 / (gv + gl);
    return clamp(v, 0.0, 1.0);
}

vec3 microfacedSpecularBrdf(vec3 f0, float alpha2, float NoL, float NoV, float NoH, float VoH)
{
    // NOTE: test correlate k for "hotness" remapping
    float D = dGGX(alpha2, NoH);
    float G = vGGXWithCombinedDenominator(alpha2, NoV, NoL);
    vec3 F = fSchlick(f0, VoH);
    return F * (D * G);
}

float microfacedSpecularBrdfClearcoat(
    float f0, float alpha2, float NoL, float NoH, float VoH, float clearcoat, out float fcc)
{
    // NOTE: test correlate k for "hotness" remapping
    float D = dGGX(alpha2, NoH);
    float G = vKelemen(NoH);
    float F = fSchlickSingle(f0, VoH) * clearcoat;
    fcc = F;
    return F * D * G;
}

vec3 microfacedSpecularBrdfAnisotropic(vec3 f0, float alpha, float NoL, float NoV, float NoH, float VoH, float ToL,
    float ToV, float ToH, float BoL, float BoV, float BoH, float anisotropy)
{
    const float at = max(alpha * (1.0 + anisotropy), CORE_BRDF_EPSILON);
    const float ab = max(alpha * (1.0 - anisotropy), CORE_BRDF_EPSILON);

    float D = dGGXAnisotropic(at, ab, NoH, ToH, BoH, anisotropy);
    float V = vGGXAnisotropic(at, ab, NoL, NoV, BoV, ToV, ToL, BoL, anisotropy);
    vec3 F = fSchlick(f0, VoH);
    return F * (V * D);
}

float diffuseCoeff()
{
    return dLambert();
}

#endif // SHADERS__COMMON__3D_DM_BRDF_COMMON_H
