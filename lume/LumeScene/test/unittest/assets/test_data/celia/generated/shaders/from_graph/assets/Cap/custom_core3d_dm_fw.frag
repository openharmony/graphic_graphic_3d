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
        float v1;
    ElapsedTimeBlock(v1);

    vec3 v2;
    ColorVec3Block(18, 24, 36, v2);

    float v3;
    MultiplyBlock(v1, 3.14, v3);

    vec3 v4;
    MultiplyBlock(v2, vec3(0.34, 0.34, 0.34), v4);

    float v5;
    MultiplyBlock(v3, 0.33, v5);

    float v6;
    SinBlock(v5, v6);

    float v7;
    ASinBlock(v6, v7);

    float v8;
    DivideBlock(v7, 1.57, v8);

    float v9;
    SubtractBlock(v8, 0.3, v9);

    float v10;
    ClampBlock(v9, 0, 1, v10);

    OutputColorBlock(vec4(v4.x, v4.y, v4.z, v10));

//>GENERATED_MAIN_SECTION

}
