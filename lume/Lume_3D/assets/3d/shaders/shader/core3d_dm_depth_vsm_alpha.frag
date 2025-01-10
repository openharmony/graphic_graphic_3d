#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets
#define CORE3D_DM_DEPTH_FRAG_LAYOUT 1
#include "3d/shaders/common/3d_dm_depth_layout_common.h"

// in / out
#define CORE3D_DM_DEPTH_ALPHA_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_depth_common.h"

layout (location = 0) out vec4 outColor;

layout (depth_less) out float gl_FragDepth;

#define CORE_ADJUST_VSM_MOMENTS 1

/*
fragment shader for VSM depth pass.
*/
void main(void)
{
    const uint instanceIdx = GetInstanceIndex();
    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(inUv, instanceIdx) * GetUnpackBaseColor(instanceIdx);
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            gl_FragDepth = 0.0;
            discard;
        }
    } else if (baseColor.a < 0.5) {
        discard;
    }

    const float moment1 = gl_FragCoord.z;
    float moment2 = moment1 * moment1;

#if (CORE_ADJUST_VSM_MOMENTS == 1)
    // adjust moments
    float dx = dFdx(moment1);
    float dy = dFdy(moment1);
    moment2 += 0.25*(dx*dx+dy*dy);
#endif

    outColor = vec4(moment1, moment2, 0.0, 0.0);
}
