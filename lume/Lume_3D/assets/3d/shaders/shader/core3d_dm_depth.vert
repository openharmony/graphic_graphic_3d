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

// includes

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_depth_vert_layout_common.h"
#define CORE3D_DM_DEPTH_VERT_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"

mat4 getWorldMatrix()
{
    if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_SKIN_BIT) == CORE_SUBMESH_SKIN_BIT) {
        mat4 worldMatrix = (uSkinData.jointMatrices[inIndex.x] * inWeight.x);
        worldMatrix += (uSkinData.jointMatrices[inIndex.y] * inWeight.y);
        worldMatrix += (uSkinData.jointMatrices[inIndex.z] * inWeight.z);
        worldMatrix += (uSkinData.jointMatrices[inIndex.w] * inWeight.w);
        return uMeshMatrix.mesh[gl_InstanceIndex].world * worldMatrix;
    } else {
        return uMeshMatrix.mesh[gl_InstanceIndex].world;
    }
}

/*
vertex shader for basic depth pass.
*/
void main(void)
{
    const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
    const mat4 worldMatrix = getWorldMatrix();
    CORE_VERTEX_OUT(uCameras[cameraIdx].viewProj * (worldMatrix * vec4(inPosition, 1.0)));
}
