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
#ifndef SHADERS_COMMON_CLOUD_LAYOUT_H
#define SHADERS_COMMON_CLOUD_LAYOUT_H

#include "common/cloud_structs.h"

#ifdef VULKAN
layout(set = 0, binding = 0) uniform sampler uSampler; // linear, clamp
layout(set = 0, binding = 1) uniform textureCube uTexCube;      // skyTex_ (sky.ktx), not used
layout(set = 0, binding = 2) uniform sampler uSamplerCube;      // linear, mip nearest, repeat
layout(set = 0, binding = 3) uniform texture3D uCloudBaseTex;   // lowFrequencyTex_ (LowFrequency.ktx)
layout(set = 0, binding = 4) uniform texture3D uCloudDetailTex; // highFrequencyTex_ (HighFerequency.ktx)
layout(set = 0, binding = 5) uniform texture2D uTurbulenceTex;  // curlnoiseTex_
layout(set = 0, binding = 6, std140) uniform u
{
    UniformStruct uParams;
};
layout(set = 0, binding = 7) uniform sampler uSamplerWeather; // linear, mip nearest, border color
layout(set = 0, binding = 8) uniform texture2D uCirrusTex;    // cirrusTex_

layout(set = 1, binding = 0, std140) uniform uCameraMatrices
{
    DefaultCameraMatrixStruct uCameras[CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT];
};
layout(set = 1, binding = 1) uniform texture2D uCloudMapTex; // RenderNodeWeather output (weatherMap.png)

#endif // VULKAN
#endif // SHADERS_COMMON_CLOUD_LAYOUT_H