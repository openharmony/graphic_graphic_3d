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

#ifndef SHADERS_COMMON_3D_DM_INPLACE_ENV_COMMON_H
#define SHADERS_COMMON_3D_DM_INPLACE_ENV_COMMON_H

#ifdef VULKAN

#include "3d/shaders/common/3d_dm_inplace_fog_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"

/*
 * Needs to be included after the descriptor set descriptions
 */

#define CORE3D_DEFAULT_ENV_PI 3.1415926535897932384626433832795

/**
 * Outputs the default environment type (CORE_DEFAULT_ENV_TYPE) with default material env
 */
void EnvironmentTypeBlock(out uint environmentType)
{
    environmentType = CORE_DEFAULT_ENV_TYPE;
}

vec3 GetEnvMapSample(in samplerCube cubeMap, in vec3 worldView, in float lodLevel)
{
    return unpackEnvMap(textureLod(cubeMap, worldView, lodLevel));
}

void GetBlendedMultiEnvMapSample(
    in uvec4 multiEnvIndices, in vec3 worldView, in float lodLevel, in float blendVal, out vec3 color, out vec3 factor)
{
    vec3 cube1 = GetEnvMapSample(uImgCubeSampler, worldView, lodLevel);
    vec3 cube2 = GetEnvMapSample(uImgCubeSamplerBlender, worldView, lodLevel);
    const uint env1Idx = min(multiEnvIndices.y, CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT - 1);
    const uint env2Idx = min(multiEnvIndices.z, CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT - 1);
    const vec3 factor1 = uEnvironmentDataArray[env1Idx].envMapColorFactor.xyz;
    const vec3 factor2 = uEnvironmentDataArray[env2Idx].envMapColorFactor.xyz;
    color.rgb = mix(cube1.rgb, cube2, blendVal);
    factor.rgb = mix(factor1.rgb, factor2, blendVal);
}

/**
 * Environment sampling based on flags (CORE_DEFAULT_ENV_TYPE)
 * The value is return in out vec3 color
 * Uses data from common sets (which needs to be defined with descriptor sets)
 * uGeneralData (camera index)
 * uCameras (cameras for near/far plane)
 * uEnvironmentData (orientation, lod level, and factors)
 */
void InplaceEnvironmentBlock(in uint environmentType, in uint cameraIdx, in vec2 uv, out vec4 color)
{
    color = vec4(0.0, 0.0, 0.0, 1.0);
    const DefaultMaterialEnvironmentStruct envData = uEnvironmentDataArray[0U];
    CORE_RELAXEDP const float lodLevel = envData.values.y;

    // NOTE: would be nicer to calculate in the vertex shader

    // remove translation from view
    mat4 viewProjInv = uCameras[cameraIdx].viewInv;
    viewProjInv[3] = vec4(0.0, 0.0, 0.0, 1.0);
    viewProjInv = viewProjInv * uCameras[cameraIdx].projInv;

    vec4 farPlane = viewProjInv * vec4(uv.x, uv.y, 1.0, 1.0);
    farPlane.xyz = farPlane.xyz / farPlane.w;

    vec3 colorFactor = envData.envMapColorFactor.xyz;

    if ((environmentType == CORE_BACKGROUND_TYPE_CUBEMAP) ||
        (environmentType == CORE_BACKGROUND_TYPE_EQUIRECTANGULAR)) {
        vec4 nearPlane = viewProjInv * vec4(uv.x, uv.y, 0.0, 1.0);
        nearPlane.xyz = nearPlane.xyz / nearPlane.w;

        const vec3 worldView = mat3(envData.envRotation) * normalize(farPlane.xyz - nearPlane.xyz);

        if (environmentType == CORE_BACKGROUND_TYPE_CUBEMAP) {
            const uvec4 multiEnvIndices = envData.multiEnvIndices;
            if (multiEnvIndices.x > 0) {
                vec3 col = vec3(0.0);
                GetBlendedMultiEnvMapSample(
                    multiEnvIndices, worldView, lodLevel, envData.blendFactor.x, col, colorFactor);
                color.rgb = col;
            } else {
                color.rgb = GetEnvMapSample(uImgCubeSampler, worldView, lodLevel);
            }
        } else {
            const vec2 texCoord = vec2(atan(worldView.z, worldView.x) + CORE3D_DEFAULT_ENV_PI, acos(worldView.y)) /
                                  vec2(2.0 * CORE3D_DEFAULT_ENV_PI, CORE3D_DEFAULT_ENV_PI);
            color = textureLod(uImgSampler, texCoord, lodLevel);
        }
    } else if (environmentType == CORE_BACKGROUND_TYPE_IMAGE) {
        const vec2 texCoord = (uv + vec2(1.0)) * 0.5;
        color = textureLod(uImgSampler, texCoord, lodLevel);
    }

    color.xyz *= colorFactor.xyz;

    // fog
    const vec3 camPos = uCameras[cameraIdx].viewInv[3].xyz;
    vec3 fogColor = color.rgb;
    InplaceFogBlock(CORE_CAMERA_FLAGS, farPlane.xyz, camPos, color, fogColor);
    color.rgb = fogColor.rgb;
}

#else

#endif

#endif // SHADERS_COMMON_3D_DM_INPLACE_ENV_COMMON_H
