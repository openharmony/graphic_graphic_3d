#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_structures_common.h"

// sets and specialization, and inputs

#include "3d/shaders/common/3d_dm_vert_layout_common.h"
#define CORE3D_DM_FW_VERT_INPUT 1
#define CORE3D_DM_FW_VERT_OUTPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"

// in binding = 4 to support env layouts
layout(set = 3, binding = 4, std140) uniform uLightProbeViewMatrices
{
    mat4 uViewMatrices[CORE_MAX_NUM_LIGHT_PROBE_BAKES_PER_ITERATION * 6u];
};

// Must match the push-constant block in 3d_dm_frag_layout_common.h
// (struct name + field name) so the SPIRV-Cross-generated GLES GLSL
// links cleanly with core3d_dm_fw.frag.spv.
layout(push_constant, std430) uniform uLightingPushConstant
{
    DefaultLightProbeDataIndexStruct uLightProbeDataIndexData;
    uint uViewMatrixIndex;
};

layout(constant_id = CORE_DM_CONSTANT_ID_CAMERA_FLAGS) const uint CORE_CAMERA_FLAGS = 0;

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

/*
vertex shader for basic pbr materials.
*/
void main(void)
{
    // Need to add proper culling in CPU side instead of this.
    const uint cullInstanceIdx = GetInstanceIndex();
    if (((CORE_CAMERA_FLAGS & CORE_CAMERA_LIGHT_PROBE_BAKE_BIT) == CORE_CAMERA_LIGHT_PROBE_BAKE_BIT) &&
        ((uMeshMatrix.mesh[cullInstanceIdx].layers.w & CORE_RENDER_MESH_CONTRIBUTE_GI_BIT) == 0u)) {
        // Initialize all outputs so SPIRV-Cross keeps them declared and
        // GLES link finds matching varyings for the fragment stage.
        outPos = vec3(0.0);
        outNormal = vec3(0.0, 0.0, 1.0);
        outTangentW = vec4(0.0, 0.0, 0.0, 1.0);
        outPrevPosI = vec4(0.0);
        outUv = vec4(0.0);
        outColor = vec4(0.0);
        outIndices = 0u;
        CORE_VERTEX_OUT(vec4(0));
        return;
    }

    const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
    mat4 worldMatrix;
    mat3 normalMatrix;
    mat4 prevWorldMatrix;
    GetWorldMatrix(worldMatrix, normalMatrix, prevWorldMatrix);
    const vec4 worldPos = worldMatrix * vec4(inPosition.xyz, 1.0);
    const vec4 projPos =
        uCameras[cameraIdx].proj * uViewMatrices[uViewMatrixIndex] * worldPos;
    CORE_VERTEX_OUT(projPos);

    const uint instanceIdx = GetInstanceIndex();
    outIndices = GetPackFlatIndices(cameraIdx, instanceIdx);

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
