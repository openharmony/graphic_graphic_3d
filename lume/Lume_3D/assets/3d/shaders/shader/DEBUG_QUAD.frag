#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


// includes
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"
#include "3d/shaders/common/3d_dm_lighting_common.h"

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

#define CORE3D_DM_FW_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"

// set
layout(location = 0) out vec4 outColor;


void main(void)
{
    outColor = vec4(0.2f, 0.0f, 0.0f, 0.1f);
}
