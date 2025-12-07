#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_brdf_common.h"

#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"

// sets

#include "3d/shaders/common/3d_dm_frag_layout_common.h"

// inplace

#include "3d/shaders/common/3d_dm_lighting_common.h"

// in / out

#define CORE3D_DM_FW_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

// Texture slot declarations:
const uint Shadow = 0;
// Included dependencies:
#include "studio-fragment-shader-functions/core-3d/core-3d-dm.h"
#include "studio-fragment-shader-functions/core-3d/core-3d-env.h"
#include "studio-fragment-shader-functions/render/base.h"
#include "studio-fragment-shader-functions/render/post-process.h"
#include "studio-standard-shader-functions/conditionals.h"
#include "studio-standard-shader-functions/scalar-op/angles.h"
#include "studio-standard-shader-functions/scalar-op/basic.h"
#include "studio-standard-shader-functions/scalar-op/clamp.h"
#include "studio-standard-shader-functions/scalar-op/exppow.h"
#include "studio-standard-shader-functions/scalar-op/round.h"
#include "studio-standard-shader-functions/scalar-op/trignometry.h"
#include "studio-standard-shader-functions/vector-op.h"
//>GENERATED_DECLARATIONS_SECTION

/*
 * fragment shader template for basic pbr materials
*/
void main(void)
{
        vec4 v1;
    InputUvBlock(v1);

    vec4 v2;
    MaterialTextureInfoSlotSampleBlock(Shadow, vec2(v1.x, v1.y), v2);

    vec3 v3;
    MultiplyBlock(vec3(v2.x, v2.y, v2.z), vec3(v2.w, v2.w, v2.w), v3);

    OutputColorBlock(vec4(v3.x, v3.y, v3.z, v2.w));

//>GENERATED_MAIN_SECTION

}
