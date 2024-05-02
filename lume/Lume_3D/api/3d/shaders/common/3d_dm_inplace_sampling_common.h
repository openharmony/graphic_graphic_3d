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

#ifndef SHADERS__COMMON__3D_DM_INPLANE_SAMPLING_COMMON_H
#define SHADERS__COMMON__3D_DM_INPLANE_SAMPLING_COMMON_H

/*
 * Needs to be included after the descriptor sets are bound.
 */

vec2 GetTransformedUV(const DefaultMaterialUnpackedTexTransformStruct texTransform, const vec2 uvInput)
{
    vec2 uv;
    uv.x = dot(texTransform.rotateScale.xy, uvInput);
    uv.y = dot(texTransform.rotateScale.zw, uvInput);
    uv += texTransform.translate.xy;
    return uv;
}

// transform = high bits (16)
// uv set bit = low bits (0)
uint GetUnpackTexCoordInfo()
{
    return floatBitsToUint(uMaterialData.material[0].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].y);
}
uint GetUnpackTexCoordInfo(uint instanceIdx)
{
    return floatBitsToUint(uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].y);
}

vec2 GetFinalSamplingUV(vec4 inputUv, uint texCoordInfoBit, uint texCoordIdx)
{
    const uint texCoordInfo = GetUnpackTexCoordInfo();
    vec2 uv = (((texCoordInfo >> CORE_MATERIAL_TEXCOORD_INFO_SHIFT) & texCoordInfoBit) == texCoordInfoBit) ? inputUv.zw
                                                                                                           : inputUv.xy;
    const bool doTrans = (((texCoordInfo & 0xffff) & texCoordInfoBit) == texCoordInfoBit);
    if (((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TEXTURE_TRANSFORM_BIT) == CORE_MATERIAL_TEXTURE_TRANSFORM_BIT) &&
        doTrans) {
        DefaultMaterialUnpackedTexTransformStruct texTransform =
            GetUnpackTextureTransform(uMaterialTransformData.material[0].packed[texCoordIdx]);
        uv = GetTransformedUV(texTransform, uv);
    }
    return uv;
}
vec2 GetFinalSamplingUV(vec4 inputUv, uint texCoordInfoBit, uint texCoordIdx, uint instanceIdx)
{
    const uint texCoordInfo = GetUnpackTexCoordInfo(instanceIdx);
    vec2 uv = (((texCoordInfo >> CORE_MATERIAL_TEXCOORD_INFO_SHIFT) & texCoordInfoBit) == texCoordInfoBit) ? inputUv.zw
                                                                                                           : inputUv.xy;
    const bool doTrans = (((texCoordInfo & 0xffff) & texCoordInfoBit) == texCoordInfoBit);
    if (((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TEXTURE_TRANSFORM_BIT) == CORE_MATERIAL_TEXTURE_TRANSFORM_BIT) &&
        doTrans) {
        DefaultMaterialUnpackedTexTransformStruct texTransform =
            GetUnpackTextureTransform(uMaterialTransformData.material[instanceIdx].packed[texCoordIdx]);
        uv = GetTransformedUV(texTransform, uv);
    }
    return uv;
}

vec4 GetBaseColorSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_BASE_BIT, CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX);
    return texture(uSampTextureBase, uv);
}
vec4 GetBaseColorSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_BASE_BIT, CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX, instanceIdx);
    return texture(uSampTextureBase, uv);
}

vec3 GetNormalSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_NORMAL_BIT, CORE_MATERIAL_PACK_TEX_NORMAL_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_NORMAL_IDX], uv).xyz;
}
vec3 GetNormalSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_NORMAL_BIT, CORE_MATERIAL_PACK_TEX_NORMAL_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_NORMAL_IDX], uv).xyz;
}

vec4 GetMaterialSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_MATERIAL_BIT, CORE_MATERIAL_PACK_TEX_MATERIAL_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_MATERIAL_IDX], uv);
}
vec4 GetMaterialSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_MATERIAL_BIT, CORE_MATERIAL_PACK_TEX_MATERIAL_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_MATERIAL_IDX], uv);
}

vec3 GetEmissiveSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_EMISSIVE_BIT, CORE_MATERIAL_PACK_TEX_EMISSIVE_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_EMISSIVE_IDX], uv).xyz;
}
vec3 GetEmissiveSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_EMISSIVE_BIT, CORE_MATERIAL_PACK_TEX_EMISSIVE_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_EMISSIVE_IDX], uv).xyz;
}

float GetAOSample(const vec4 uvInput)
{
    const vec2 uv = GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_AO_BIT, CORE_MATERIAL_PACK_TEX_AO_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_AO_IDX], uv).x;
}
float GetAOSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_AO_BIT, CORE_MATERIAL_PACK_TEX_AO_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_AO_IDX], uv).x;
}

float GetClearcoatSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_BIT, CORE_MATERIAL_PACK_TEX_CLEARCOAT_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_IDX], uv).x;
}
float GetClearcoatSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_BIT, CORE_MATERIAL_PACK_TEX_CLEARCOAT_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_IDX], uv).x;
}

float GetClearcoatRoughnessSample(const vec4 uvInput)
{
    const vec2 uv = GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_ROUGHNESS_BIT,
        CORE_MATERIAL_PACK_TEX_CLEARCOAT_ROUGHNESS_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_ROUGHNESS_IDX], uv).y;
}
float GetClearcoatRoughnessSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_ROUGHNESS_BIT,
        CORE_MATERIAL_PACK_TEX_CLEARCOAT_ROUGHNESS_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_ROUGHNESS_IDX], uv).y;
}

vec3 GetClearcoatNormalSample(const vec4 uvInput)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_NORMAL_BIT, CORE_MATERIAL_PACK_TEX_CLEARCOAT_NORMAL_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_NORMAL_IDX], uv).xyz;
}
vec3 GetClearcoatNormalSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_NORMAL_BIT,
        CORE_MATERIAL_PACK_TEX_CLEARCOAT_NORMAL_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_NORMAL_IDX], uv).xyz;
}

vec3 GetSheenSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_SHEEN_BIT, CORE_MATERIAL_PACK_TEX_SHEEN_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_SHEEN_IDX], uv).xyz;
}
vec3 GetSheenSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_SHEEN_BIT, CORE_MATERIAL_PACK_TEX_SHEEN_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_SHEEN_IDX], uv).xyz;
}

// NOTE: from sheen alpha
float GetSheenRoughnessSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_SHEEN_BIT, CORE_MATERIAL_PACK_TEX_SHEEN_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_SHEEN_IDX], uv).a; // alpha
}
float GetSheenRoughnessSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_SHEEN_BIT, CORE_MATERIAL_PACK_TEX_SHEEN_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_SHEEN_IDX], uv).a; // alpha
}

float GetTransmissionSample(const vec4 uvInput)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_TRANSMISSION_BIT, CORE_MATERIAL_PACK_TEX_TRANSMISSION_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_TRANSMISSION_IDX], uv).r;
}
float GetTransmissionSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_TRANSMISSION_BIT, CORE_MATERIAL_PACK_TEX_TRANSMISSION_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_TRANSMISSION_IDX], uv).r;
}

vec4 GetSpecularSample(const vec4 uvInput)
{
    const vec2 uv =
        GetFinalSamplingUV(uvInput, CORE_MATERIAL_TEXCOORD_INFO_SPECULAR_BIT, CORE_MATERIAL_PACK_TEX_SPECULAR_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_SPECULAR_IDX], uv);
}
vec4 GetSpecularSample(const vec4 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_SPECULAR_BIT, CORE_MATERIAL_PACK_TEX_SPECULAR_UV_IDX, instanceIdx);
    return texture(uSampTextures[CORE_MATERIAL_TEX_SPECULAR_IDX], uv);
}

/*
 * Inplace sampling of material data, similar functions with ubo input in 3d_dm_structures_common.h
 */

uint GetUnpackCameraIndex()
{
    return uGeneralData.indices.x;
}

vec4 GetUnpackViewport()
{
    return uGeneralData.viewportSizeInvViewportSize;
}

vec4 GetUnpackBaseColor(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_BASE_IDX];
}

vec4 GetUnpackMaterial(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_MATERIAL_IDX];
}

float GetUnpackAO(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_AO_IDX].x;
}

float GetUnpackClearcoat(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_IDX].x;
}

// NOTE: sampling from .y
float GetUnpackClearcoatRoughness(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_ROUGHNESS_IDX].y;
}

float GetUnpackClearcoatNormalScale(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_NORMAL_IDX].x;
}

// .xyz = sheen factor, .w = sheen roughness
vec4 GetUnpackSheen(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_SHEEN_IDX];
}

float GetUnpackTransmission(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_TRANSMISSION_IDX].x;
}

// .xyz = specular color, .w = specular strength
vec4 GetUnpackSpecular(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_SPECULAR_IDX];
}

vec3 GetUnpackEmissiveColor(const uint instanceIdx)
{
    const vec4 emissive = uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_EMISSIVE_IDX];
    return emissive.rgb * emissive.a;
}

float GetUnpackNormalScale(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_NORMAL_IDX].x;
}

float GetUnpackAlphaCutoff(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].x;
}

vec4 GetUnpackMaterialTextureInfoSlotFactor(const uint materialIndexSlot, const uint instanceIdx)
{
    const uint maxIndex = min(materialIndexSlot, CORE_MATERIAL_FACTOR_UNIFORM_VEC4_COUNT - 1);
    return uMaterialData.material[instanceIdx].factors[maxIndex].xyzw;
}

vec4 Unpremultiply(in vec4 color)
{
    if (color.a == 0.0) {
        return vec4(0);
    }
    return vec4(color.rgb / color.a, color.a);
}

CORE_RELAXEDP vec4 GetUnpackBaseColorFinalValue(in CORE_RELAXEDP vec4 color, in vec4 uv, in uint instanceIdx)
{
    // NOTE: by the spec with blend mode opaque alpha should be 1.0 from this calculation
    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(uv, instanceIdx) * GetUnpackBaseColor(instanceIdx) * color;
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_OPAQUE_BIT) == CORE_MATERIAL_OPAQUE_BIT) {
        baseColor.a = 1.0;
    } else {
        baseColor = Unpremultiply(baseColor);
    }
    return baseColor;
}

// DEPRECATED: use the one with cameraIdx
vec2 GetFinalCalculatedVelocity(in vec3 pos, in vec3 prevPos)
{
    // NOTE: velocity should be unjittered when reading (or calc without jitter)
    // currently default cameras calculates the same jitter for both frames

    const uint cameraIdx = GetUnpackCameraIndex();
    const vec4 projPos = uCameras[cameraIdx].viewProj * vec4(pos.xyz, 1.0);
    const vec4 projPosPrev = uCameras[cameraIdx].viewProjPrevFrame * vec4(prevPos.xyz, 1.0);

    const vec2 uvPos = (projPos.xy / projPos.w) * 0.5 + 0.5;
    const vec2 oldUvPos = (projPosPrev.xy / projPosPrev.w) * 0.5 + 0.5;
    // better precision for fp16 and expected in parts of engine
    return (uvPos - oldUvPos) * uGeneralData.viewportSizeInvViewportSize.xy;
}

vec2 GetFinalCalculatedVelocity(in vec3 pos, in vec3 prevPos, in uint cameraIdx)
{
    // NOTE: velocity should be unjittered when reading (or calc without jitter)
    // currently default cameras calculates the same jitter for both frames

    const vec4 projPos = uCameras[cameraIdx].viewProj * vec4(pos.xyz, 1.0);
    const vec4 projPosPrev = uCameras[cameraIdx].viewProjPrevFrame * vec4(prevPos.xyz, 1.0);

    const vec2 uvPos = (projPos.xy / projPos.w) * 0.5 + 0.5;
    const vec2 oldUvPos = (projPosPrev.xy / projPosPrev.w) * 0.5 + 0.5;
    // better precision for fp16 and expected in parts of engine
    return (uvPos - oldUvPos) * uGeneralData.viewportSizeInvViewportSize.xy;
}
#endif // SHADERS__COMMON__3D_DM_INPLANE_SAMPLING_COMMON_H
