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

#ifndef SHADERS_COMMON_3D_DM_AREA_LIGHTING_COMMON_H
#define SHADERS_COMMON_3D_DM_AREA_LIGHTING_COMMON_H

const float M_PI = 3.14159265358;

// Adapted from https://alextardif.com/arealights.html

float RectangleSolidAngle(vec3 p, vec3 v0, vec3 v1, vec3 v2, vec3 v3)
{
    vec3 n0 = normalize(cross(v0, v1));
    vec3 n1 = normalize(cross(v1, v2));
    vec3 n2 = normalize(cross(v2, v3));
    vec3 n3 = normalize(cross(v3, v0));

    float g0 = acos(dot(-n0, n1));
    float g1 = acos(dot(-n1, n2));
    float g2 = acos(dot(-n2, n3));
    float g3 = acos(dot(-n3, n0));

    return g0 + g1 + g2 + g3 - 2. * M_PI; // 2.: scale
}

float TracePlane(vec3 o, vec3 d, vec3 planeOrigin, vec3 planeNormal)
{
    return dot(planeNormal, (planeOrigin - o) / dot(planeNormal, d));
}

float Saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

float Sqr(float x)
{
    return x * x;
}

vec3 CalculateRectAreaLight(uint lightIdx, vec3 diff, vec3 spec, vec3 N, vec3 V, vec3 P, float alpha2)
{
    const vec3 xDirHalf = uLightData.lights[lightIdx].dir.xyz * 0.5;
    const vec3 yDirHalf = uLightData.lights[lightIdx].spotLightParams.xyz * 0.5;

    const vec3 lPos = uLightData.lights[lightIdx].pos.xyz;
    const vec3 lDir = cross(xDirHalf, yDirHalf);
    const vec3 rDir = mix(N, reflect(-V, N), 1 - alpha2);

    float faceCheck = dot(lPos - P, -lDir);
    if (faceCheck > 0.0) {
        return vec3(0.0, 0.0, 0.0);
    }

    const vec3 v0 = (lPos - xDirHalf + yDirHalf) - P;
    const vec3 v1 = (lPos - xDirHalf - yDirHalf) - P;
    const vec3 v2 = (lPos + xDirHalf - yDirHalf) - P;
    const vec3 v3 = (lPos + xDirHalf + yDirHalf) - P;

    const float xLen = length(xDirHalf);
    const float yLen = length(yDirHalf);
    const float sAngle = RectangleSolidAngle(P, v0, v1, v2, v3);

    const float noL = sAngle * 0.2 *
                      (Saturate(dot(normalize(v0 - P), N)) + Saturate(dot(normalize(v1 - P), N)) +
                          Saturate(dot(normalize(v2 - P), N)) + Saturate(dot(normalize(v3 - P), N)) +
                          Saturate(dot(normalize(lPos - P), N)));

    const vec3 intPos = P + rDir * TracePlane(P, rDir, lPos, lDir);
    const vec3 intVec = intPos - lPos;
    const vec2 intDist = vec2(dot(intVec, xDirHalf) / xLen, dot(intVec, yDirHalf) / yLen);
    const vec2 nearRect = vec2(clamp(intDist.x, -xLen, xLen), clamp(intDist.y, -yLen, yLen));

    vec3 specularFactor = vec3(0, 0, 0);
    vec3 diffuseFactor = diff / M_PI;

    float roL = dot(rDir, lDir);
    if (roL > 0.0) {
        float specFactor = 1.0 - clamp(length(nearRect - intDist) * Sqr(1.0 - alpha2) * 32.0, 0.0, 1.0);
        specularFactor += spec * specFactor * roL * noL;
    }

    const float range = uLightData.lights[lightIdx].dir.w;

    // Add contribution, luminosity is baked into light color.
    vec3 result = uLightData.lights[lightIdx].color.xyz * (specularFactor + diffuseFactor);

    return result;
}

#endif // SHADERS_COMMON_3D_DM_AREA_LIGHTING_COMMON_H
