#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_depth_vert_layout_common.h"
#define CORE3D_DM_DEPTH_VERT_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"

uint GetInstanceIndex()
{
    uint instanceIdx = 0;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_GPU_INSTANCING_BIT) == CORE_MATERIAL_GPU_INSTANCING_BIT) {
        instanceIdx = gl_InstanceIndex;
    }
    return instanceIdx;
}

mat4 getWorldMatrix()
{
    const uint instanceIdx = GetInstanceIndex();
    if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_SKIN_BIT) == CORE_SUBMESH_SKIN_BIT) {
        mat4 worldMatrix = (uSkinData.jointMatrices[inIndex.x] * inWeight.x);
        worldMatrix += (uSkinData.jointMatrices[inIndex.y] * inWeight.y);
        worldMatrix += (uSkinData.jointMatrices[inIndex.z] * inWeight.z);
        worldMatrix += (uSkinData.jointMatrices[inIndex.w] * inWeight.w);
        return uMeshMatrix.mesh[instanceIdx].world * worldMatrix;
    } else {
        return uMeshMatrix.mesh[instanceIdx].world;
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
