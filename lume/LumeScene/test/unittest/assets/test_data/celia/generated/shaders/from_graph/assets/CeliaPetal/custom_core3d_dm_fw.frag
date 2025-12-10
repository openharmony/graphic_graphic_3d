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
    WorldPositionBlock(v2);

    vec3 v3;
    Vec3SwizzleBlock(1.5, -0.35, 1, v3);

    vec3 v4;
    WorldPositionBlock(v4);

    vec3 v5;
    Vec3SwizzleBlock(-0.5, 1.15, 0.23, v5);

    vec3 v6;
    CameraViewDirBlock(v6);

    vec3 v7;
    ColorVec3Block(0.75, 0.55, 0.95, v7);

    vec3 v8;
    InputNormalBlock(v8);

    vec3 v9;
    ColorVec3Block(0.35, 0.65, 1, v9);

    vec3 v10;
    Vec3SwizzleBlock(0.16, 0.14, -2, v10);

    vec3 v11;
    ColorVec3Block(1, 0.65, 0.4, v11);

    vec3 v12;
    ColorVec3Block(1, 1, 0.7, v12);

    vec3 v13;
    rotateXBlock(v1, v5, v13);

    vec3 v14;
    NegateBlock(v6, v14);

    vec3 v15;
    VecNormalizeBlock(v8, v15);

    vec3 v16;
    SubtractBlock(v3, v10, v16);

    vec3 v17;
    SubtractBlock(v2, v10, v17);

    float v18;
    VecDistanceBlock(v4, v13, v18);

    vec3 v19;
    VecNormalizeBlock(v14, v19);

    float v20;
    VecDotBlock(v16, v16, v20);

    float v21;
    VecDotBlock(v16, v17, v21);

    float v22;
    DivideBlock(v18, 3, v22);

    vec3 v23;
    MultiplyBlock(v19, v15, v23);

    float v24;
    DivideBlock(v21, v20, v24);

    float v25;
    ClampBlock(v22, 0, 1, v25);

    float v26;
    VecDotBlock(v15, v23, v26);

    float v27;
    ClampBlock(v24, 0, 1, v27);

    float v28;
    SmoothStepBlock(v25, 0.38, 0.67, v28);

    float v29;
    SmoothStepBlock(v25, 0.67, 0.88, v29);

    float v30;
    SmoothStepBlock(v25, 0, 0.38, v30);

    float v31;
    ClampBlock(v26, 0.7, 1, v31);

    float v32;
    SmoothStepBlock(v27, 0.07, 0.8, v32);

    vec3 v33;
    LerpBlock(v12, v11, vec3(v30, v30, v30), v33);

    float v34;
    AddBlock(v31, 0.5, v34);

    float v35;
    MultiplyBlock(v32, 1, v35);

    vec3 v36;
    LerpBlock(vec3(0.1, 0.1, 0.1), vec3(1, 1, 1), vec3(v32, v32, v32), v36);

    vec3 v37;
    LerpBlock(v33, v7, vec3(v28, v28, v28), v37);

    vec3 v38;
    LerpBlock(v37, v9, vec3(v29, v29, v29), v38);

    vec3 v39;
    MultiplyBlock(v38, v36, v39);

    vec3 v40;
    PowerBlock(v39, vec3(3, 3, 3), v40);

    vec3 v41;
    MultiplyBlock(vec3(v40.x, v40.y, v40.z), vec3(v34, v34, v34), v41);

    OutputColorBlock(vec4(v41.x, v41.y, v41.z, v35));

//>GENERATED_MAIN_SECTION

}
