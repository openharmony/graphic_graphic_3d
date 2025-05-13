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
#ifndef SHADERS_COMMON_CLOUD_COMMON_H
#define SHADERS_COMMON_CLOUD_COMMON_H

#ifdef VULKAN
float overrideCoverage()
{
    return uParams.params[0].x;
}

float overridePercipitation()
{
    return uParams.params[0].y;
}

float overrideCloudType()
{
    return uParams.params[0].z;
}

float CLOUD_DOMAIN_LOW_RES()
{
    return uParams.params[0].w;
}

float zmin()
{
    return uParams.params[1].x;
}

float zmax()
{
    return uParams.params[1].y;
}

float silverIntensity()
{
    return uParams.params[1].z;
}

float silverSpread()
{
    return uParams.params[1].w;
}

vec3 uSunPos()
{
    return uParams.params[2].xyz;
}

float CLOUD_DOMAIN_CURL_RES()
{
    return uParams.params[2].w;
}

vec3 uAtmosphereColor()
{
    return uParams.params[3].xyz;
}

float CLOUD_DOMAIN_HIGH_RES()
{
    return uParams.params[3].w;
}

float AnvilBias()
{
    return clamp(uParams.params[4].x, 0, 1);
}

float brightness()
{
    return uParams.params[4].y;
}

float eccintricity()
{
    return uParams.params[4].z;
}

float ambientStrength()
{
    return uParams.params[5].x;
}

float directStrength()
{
    return uParams.params[5].y;
}

float lightStep()
{
    return uParams.params[5].z;
}

float scatteringDistance()
{
    return uParams.params[5].w;
}

float CLOUD_DOMAIN_CURL_AMPLITUDE()
{
    return uParams.params[6].w;
}

float time()
{
    return uPc.params[0].x;
}

float windSpeed()
{
    return uPc.params[0].y;
}

float globalDensity()
{
    return uPc.params[0].z;
}

vec3 uAmbientColor()
{
    return uPc.params[1].xyz;
}

vec3 uSunColor()
{
    return uPc.params[2].xyz;
}
/*
vec3 uAtmosphereColor()
{
    return uPc.params[3].xyz;
}*/

vec3 windDir()
{
    return vec3(uPc.params[1].w, uPc.params[2].w, uPc.params[3].w);
}

vec3 uCameraPos()
{
    vec4 near = uPc.view[2];
    near.xyz /= near.w;
    return near.xyz - uPc.params[3].xyz;
}

Ray CreateRay()
{
    Ray ray;
    vec2 coord = vec2(inUv.x, inUv.y);
    vec2 clip = vec2(coord - 0.5) * vec2(2.0, 2.0);
    vec4 far = uPc.view * vec4(clip, 1.0, 1.0);
    vec4 near = uPc.view[2];
    near.xyz /= near.w;
    far.xyz /= far.w;
    ray.o = near.xyz - uPc.params[3].xzy;
    ray.dir = normalize(far.xyz - near.xyz);
    return ray;
}

Ray CreateRay(uint cameraIdx)
{
    // avoid precision loss, by decomposing the matrix
    vec3 position = vec3(uCameras[cameraIdx].view[3].xzy);

    mat4x4 a = mat4(vec4(uCameras[cameraIdx].view[0].xyzw), vec4(uCameras[cameraIdx].view[1].xyzw),
        vec4(uCameras[cameraIdx].view[2].xyzw), vec4(0.0, 0.0, 0.0, 1.0));

    // assume affine matrix, so just take the transpose
    mat4x4 v = transpose(a) * uCameras[cameraIdx].projInv;

    Ray ray;
    vec2 coord = vec2(inUv.x, inUv.y);
    vec2 clip = vec2(coord - 0.5) * vec2(2.0, 2.0);
    vec4 far = v * vec4(clip, 1.0, 1.0);
    vec4 near = v[2];
    near.xyz /= near.w;
    far.xyz /= far.w;
    ray.o = near.xyz - position;
    ray.dir = normalize(far.xyz - near.xyz);

    return ray;
}
#endif // VULKAN
#endif // SHADERS_COMMON_CLOUD_COMMON_H