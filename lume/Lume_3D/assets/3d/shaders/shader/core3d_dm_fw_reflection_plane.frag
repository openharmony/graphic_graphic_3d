#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// include without extra flags
#include "core3d_dm_fw_reflection_plane_frag.h"

/**
 * fragment shader for plane reflector
 */
void main(void)
{
    vec2 fragUv;
    CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, GetUnpackViewport().zw);
    outColor = PlaneReflector(fragUv);
    if (CORE_POST_PROCESS_FLAGS > 0) {
        InplacePostProcess(fragUv, outColor);
    }
}
