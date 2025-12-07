#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// include without extra flags
#include "core3d_dm_fw_frag.h"

/*
fragment shader for basic pbr materials.
*/
void main(void)
{
    if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT) {
        outColor = unlitBasic();
    } else if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT_SHADOW_ALPHA) {
        outColor = unlitShadowAlpha();
    } else {
        outColor = pbrBasic();
    }

    if (CORE_POST_PROCESS_FLAGS > 0) {
        vec2 fragUv;
        CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

        InplacePostProcess(fragUv, outColor);
    }
}
