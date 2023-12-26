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

#ifndef SHADERS__COMMON__CORE3D_ENV_BLOCKS_H
#define SHADERS__COMMON__CORE3D_ENV_BLOCKS_H

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

/**
 * Environment sampling based on flags (CORE_DEFAULT_ENV_TYPE)
 * The value is return in out vec3 color
 * Uses data from common sets (which needs to be defined with descriptor sets)
 * uGeneralData (camera index)
 * uCameras (cameras for near/far plane)
 * uEnvironmentData (orientation, lod level, and factors)
 */
void EnvironmentMapSampleBlock(
    in uint environmentType, in vec2 uv, in samplerCube cubeMap, in sampler2D texMap, out vec4 color)
{
    color = vec4(0.0, 0.0, 0.0, 1.0);
    CORE_RELAXEDP const float lodLevel = uEnvironmentData.values.y;
    if ((environmentType == CORE_BACKGROUND_TYPE_CUBEMAP) ||
        (environmentType == CORE_BACKGROUND_TYPE_EQUIRECTANGULAR)) {
        // NOTE: would be nicer to calculate in the vertex shader

        // remove translation from view
        const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
        mat4 viewProjInv = uCameras[cameraIdx].viewInv;
        viewProjInv[3] = vec4(0.0, 0.0, 0.0, 1.0);
        viewProjInv = viewProjInv * uCameras[cameraIdx].projInv;

        vec4 farPlane = viewProjInv * vec4(inUv.x, inUv.y, 1.0, 1.0);
        farPlane.xyz = farPlane.xyz / farPlane.w;

        vec4 nearPlane = viewProjInv * vec4(inUv.x, inUv.y, 0.0, 1.0);
        nearPlane.xyz = nearPlane.xyz / nearPlane.w;

        const vec3 worldView = mat3(uEnvironmentData.envRotation) * normalize(farPlane.xyz - nearPlane.xyz);

        if (environmentType == CORE_BACKGROUND_TYPE_CUBEMAP) {
            color.rgb = unpackEnvMap(textureLod(cubeMap, worldView, lodLevel));
        } else {
            const vec2 texCoord = vec2(atan(worldView.z, worldView.x) + CORE3D_DEFAULT_ENV_PI, acos(worldView.y)) /
                vec2(2.0 * CORE3D_DEFAULT_ENV_PI, CORE3D_DEFAULT_ENV_PI);
            color = textureLod(texMap, texCoord, lodLevel);
        }
    } else if (environmentType == CORE_BACKGROUND_TYPE_IMAGE) {
        const vec2 texCoord = (inUv + vec2(1.0)) * 0.5;
        color = textureLod(texMap, texCoord, lodLevel);
    }

    color.xyz *= uEnvironmentData.envMapColorFactor.xyz;
}
#endif // SHADERS__COMMON__CORE3D_ENV_BLOCKS_H
