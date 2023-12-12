/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_multiview : enable

// NOTE: multiview extension is enabled

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets
#include "3d/shaders/common/3d_dm_env_frag_layout_common.h"

// in / out

layout(location = 0) out vec2 outUv;
layout(location = 1) out flat uint outIndices; // packed camera

uint GetMultiViewCameraIndex()
{
    const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
    const uint count = uCameras[cameraIdx].multiViewIndices[0];
    uint newCameraIdx = cameraIdx;
    // NOTE: when gl_ViewIndex is 0 the "main" camera is used and no multi-view indexing
    if ((count > 0) && (gl_ViewIndex <= count) && (gl_ViewIndex != 0)) {
        newCameraIdx = uCameras[cameraIdx].multiViewIndices[gl_ViewIndex];
    }
    return newCameraIdx;
}

/*
 * Fullscreen env triangle.
*/
void main(void)
{
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = 1.0 - float((gl_VertexIndex & 2) << 1);
    CORE_VERTEX_OUT(vec4(x, y, 1.0, 1.0));
    outUv = vec2(x, y);

    const uint cameraIdx = GetMultiViewCameraIndex();
    outIndices = GetPackFlatIndices(cameraIdx, 0);
}
