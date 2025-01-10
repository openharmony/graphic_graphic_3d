#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"

#define CORE3D_DM_DEPTH_FRAG_LAYOUT 1
#include "3d/shaders/common/3d_dm_depth_layout_common.h"
#define CORE3D_DM_DEPTH_ALPHA_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_depth_common.h"

// sets

// in / out

layout(depth_less) out float gl_FragDepth;

/*
fragment shader for depth pass.
*/
void main(void)
{
    const uint instanceIdx = GetMaterialInstanceIndex(inIndices);
    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(inUv, instanceIdx) * GetUnpackBaseColor(instanceIdx);
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    } else if (baseColor.a < 0.5) {
        discard;
    }
}
