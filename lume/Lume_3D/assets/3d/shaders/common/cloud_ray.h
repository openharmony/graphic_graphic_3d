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
#ifndef CLOUDS_RAY_H
#define CLOUDS_RAY_H

#include "common/cloud_structs.h"

// gets the ray origin and direction for the current fragment
Ray calcRay()
{
    // calc image plane
    float ar = width / height;
    float angle = tan(radians(fov / 2));
    vec2 imagePlane = (i.uv * 2 - vec2(1)) * vec2(angle) * vec2(ar, 1);

    // extract camera space
    mat3 cameraToWorld = transpose(mat3(V));

    // ray origin is position of camera
    vec3 o = uCameraPos;

    // transform direction with view matrix
    vec3 pLocal = vec3(imagePlane, -1);
    vec3 pWorld = cameraToWorld * pLocal;
    vec3 dir = normalize(pWorld);

    return Ray(o, dir);
}

#endif // CLOUDS_RAY_H