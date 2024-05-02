#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_multiview : enable

// NOTE: multiview extension is enabled

// includes

#include "3d/shaders/common/3d_dm_structures_common.h"

// sets and specialization, and inputs

#include "3d/shaders/common/3d_dm_vert_layout_common.h"
#define CORE3D_DM_FW_VERT_INPUT 1
#define CORE3D_DM_FW_VERT_OUTPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"

uint GetInstanceIndex()
{
    uint instanceIdx = 0;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_GPU_INSTANCING_BIT) == CORE_MATERIAL_GPU_INSTANCING_BIT) {
        instanceIdx = gl_InstanceIndex;
    }
    return instanceIdx;
}

void GetWorldMatrix(out mat4 worldMatrix, out mat3 normalMatrix, out mat4 prevWorldMatrix)
{
    const uint instanceIdx = GetInstanceIndex();
    if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_SKIN_BIT) == CORE_SUBMESH_SKIN_BIT) {
        mat4 world = (uSkinData.jointMatrices[inIndex.x] * inWeight.x);
        world += (uSkinData.jointMatrices[inIndex.y] * inWeight.y);
        world += (uSkinData.jointMatrices[inIndex.z] * inWeight.z);
        world += (uSkinData.jointMatrices[inIndex.w] * inWeight.w);
        worldMatrix = uMeshMatrix.mesh[instanceIdx].world * world;
        normalMatrix = mat3(uMeshMatrix.mesh[instanceIdx].normalWorld * world);
        if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_VELOCITY_BIT) == CORE_SUBMESH_VELOCITY_BIT) {
            const uvec4 offIndex = inIndex + CORE_DEFAULT_MATERIAL_PREV_JOINT_OFFSET;
            mat4 prevWorld = (uSkinData.jointMatrices[offIndex.x] * inWeight.x);
            prevWorld += (uSkinData.jointMatrices[offIndex.y] * inWeight.y);
            prevWorld += (uSkinData.jointMatrices[offIndex.z] * inWeight.z);
            prevWorld += (uSkinData.jointMatrices[offIndex.w] * inWeight.w);
            prevWorldMatrix = uMeshMatrix.mesh[instanceIdx].prevWorld * prevWorld;
        }
    } else {
        worldMatrix = uMeshMatrix.mesh[instanceIdx].world;
        normalMatrix = mat3(uMeshMatrix.mesh[instanceIdx].normalWorld);
        if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_VELOCITY_BIT) == CORE_SUBMESH_VELOCITY_BIT) {
            prevWorldMatrix = uMeshMatrix.mesh[instanceIdx].prevWorld;
        }
    }
}

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
vertex shader for basic pbr materials.
*/
void main(void)
{
    const uint cameraIdx = GetMultiViewCameraIndex();
    mat4 worldMatrix;
    mat3 normalMatrix;
    mat4 prevWorldMatrix;
    GetWorldMatrix(worldMatrix, normalMatrix, prevWorldMatrix);
    const vec4 worldPos = worldMatrix * vec4(inPosition.xyz, 1.0);
    const vec4 projPos = uCameras[cameraIdx].viewProj * worldPos;
    CORE_VERTEX_OUT(projPos);

    outIndices = GetPackFlatIndices(cameraIdx, gl_InstanceIndex);

    outPos.xyz = worldPos.xyz;
    outPrevPosI = vec4(0.0, 0.0, 0.0, 0.0);
    if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_VELOCITY_BIT) == CORE_SUBMESH_VELOCITY_BIT) {
        outPrevPosI.xyz = (prevWorldMatrix * vec4(inPosition.xyz, 1.0)).xyz;
    }

    outNormal = normalize(normalMatrix * inNormal.xyz);
    if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_TANGENTS_BIT) == CORE_SUBMESH_TANGENTS_BIT) {
        outTangentW = vec4(normalize(normalMatrix * inTangent.xyz), inTangent.w);
    } else {
        outTangentW = vec4(0.0, 0.0, 0.0, 1.0);
    }

    outUv.xy = inUv0;
    if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_SECOND_TEXCOORD_BIT) == CORE_SUBMESH_SECOND_TEXCOORD_BIT) {
        outUv.zw = inUv1;
    }

    if ((CORE_SUBMESH_FLAGS & CORE_SUBMESH_VERTEX_COLORS_BIT) == CORE_SUBMESH_VERTEX_COLORS_BIT) {
        outColor = inColor;
    } else {
        outColor = vec4(1.0);
    }
}
