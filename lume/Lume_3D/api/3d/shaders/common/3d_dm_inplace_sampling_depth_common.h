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

#ifndef SHADERS_COMMON_3D_DM_INPLANE_SAMPLING_DEPTH_COMMON_H
#define SHADERS_COMMON_3D_DM_INPLANE_SAMPLING_DEPTH_COMMON_H

/*
 * Needs to be included after the descriptor sets are bound.
 */

uint GetInstanceIndex()
{
    uint instanceIdx = 0U;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_GPU_INSTANCING_BIT) == CORE_MATERIAL_GPU_INSTANCING_BIT) {
        instanceIdx = GetUnpackFlatIndicesInstanceIdx(inIndices);
    }
    return instanceIdx;
}

uint GetMaterialInstanceIndex(const uint indices)
{
    uint instanceIdx = 0U;
    if (((CORE_MATERIAL_FLAGS & CORE_MATERIAL_GPU_INSTANCING_BIT) == CORE_MATERIAL_GPU_INSTANCING_BIT) &&
        ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_GPU_INSTANCING_MATERIAL_BIT) ==
            CORE_MATERIAL_GPU_INSTANCING_MATERIAL_BIT)) {
        instanceIdx = GetUnpackFlatIndicesInstanceIdx(indices);
    }
    return instanceIdx;
}

vec2 GetTransformedUV(const DefaultMaterialUnpackedTexTransformStruct texTransform, const vec2 uvInput)
{
    vec2 uv;
    uv.x = dot(texTransform.rotateScale.xy, uvInput);
    uv.y = dot(texTransform.rotateScale.zw, uvInput);
    uv += texTransform.translate.xy;
    return uv;
}

uint GetUnpackTexCoordInfo(uint instanceIdx)
{
    return floatBitsToUint(uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].y);
}

vec2 GetFinalSamplingUV(vec2 inputUv, uint texCoordInfoBit, uint texCoordIdx, uint instanceIdx)
{
    const uint texCoordInfo = GetUnpackTexCoordInfo(instanceIdx);
    vec2 uv = inputUv;
    const bool doTrans = (((texCoordInfo & 0xffff) & texCoordInfoBit) == texCoordInfoBit);
    if (((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TEXTURE_TRANSFORM_BIT) == CORE_MATERIAL_TEXTURE_TRANSFORM_BIT) &&
        doTrans) {
        DefaultMaterialUnpackedTexTransformStruct texTransform =
            GetUnpackTextureTransform(uMaterialTransformData.material[instanceIdx].packed[texCoordIdx]);
        uv = GetTransformedUV(texTransform, uv);
    }
    return uv;
}

vec4 GetBaseColorSample(const vec2 uvInput, const uint instanceIdx)
{
    const vec2 uv = GetFinalSamplingUV(
        uvInput, CORE_MATERIAL_TEXCOORD_INFO_BASE_BIT, CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX, instanceIdx);
    return texture(uSampTextureBase, uv);
}

vec4 GetUnpackBaseColor(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_BASE_IDX];
}

float GetUnpackAlphaCutoff(const uint instanceIdx)
{
    return uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].x;
}

#endif // SHADERS_COMMON_3D_DM_INPLANE_SAMPLING_DEPTH_COMMON_H
