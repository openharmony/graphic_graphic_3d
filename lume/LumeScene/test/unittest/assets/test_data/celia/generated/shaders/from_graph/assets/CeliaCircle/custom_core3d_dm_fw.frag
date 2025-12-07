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

struct CustomPropsStruct {
    vec4 MetalRoughness;
};
layout(set=1, binding=4) uniform u_props{CustomPropsStruct props; };
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
        // Exposing props:
    vec4 MetalRoughness = props.MetalRoughness;

    vec3 v1;
    ColorVec3Block(0.75, 0.55, 0.95, v1);

    vec3 v2;
    WorldPositionBlock(v2);

    float v3;
    ElapsedTimeBlock(v3);

    vec4 v4;
    InputUvBlock(v4);

    vec3 v5;
    ColorVec3Block(1, 0.6, 0.4, v5);

    vec3 v6;
    Vec3SwizzleBlock(-0.25, -1.25, 0, v6);

    vec3 v7;
    ColorVec3Block(0.0, 0.5, 1.0, v7);

    vec3 v8;
    ColorVec3Block(1, 0.65, 0.0, v8);

    vec3 v9;
    Vec3SwizzleBlock(0.25, 1.25, 0, v9);

    float v10;
    NegateBlock(v3, v10);

    float v11;
    MultiplyBlock(v3, 3.14, v11);

    vec3 v12;
    SubtractBlock(v9, v6, v12);

    vec3 v13;
    rotateXBlock(v10, v2, v13);

    float v14;
    MultiplyBlock(v11, 0.33, v14);

    float v15;
    VecDotBlock(v12, v12, v15);

    vec3 v16;
    SubtractBlock(v13, v6, v16);

    float v17;
    SinBlock(v14, v17);

    float v18;
    VecDotBlock(v12, v16, v18);

    float v19;
    ASinBlock(v17, v19);

    float v20;
    DivideBlock(v18, v15, v20);

    float v21;
    DivideBlock(v19, 1.57, v21);

    float v22;
    ClampBlock(v20, 0, 1, v22);

    float v23;
    SubtractBlock(v21, 0.3, v23);

    float v24;
    SmoothStepBlock(v22, 0.7, 1, v24);

    float v25;
    SmoothStepBlock(v22, 0, 0.1, v25);

    float v26;
    SmoothStepBlock(v22, 0.1, 0.7, v26);

    float v27;
    ClampBlock(v23, 0, 1, v27);

    vec3 v28;
    LerpBlock(v7, v1, vec3(v25, v25, v25), v28);

    float v29;
    SmoothStepBlock(v4.x, v27, 0.9, v29);

    vec3 v30;
    LerpBlock(v28, v5, vec3(v26, v26, v26), v30);

    float v31;
    MultiplyBlock(v29, v27, v31);

    vec3 v32;
    LerpBlock(v30, v8, vec3(v24, v24, v24), v32);

    vec3 v33;
    MultiplyBlock(vec3(v31, v31, v31), vec3(18, 24, 36), v33);

    vec3 v34;
    PowerBlock(v32, vec3(2.2, 2.2, 2.2), v34);

    float v35;
    AddBlock(v32.x, v32.y, v35);

    vec3 v36;
    MultiplyBlock(v34, vec3(0.25, 0.25, 0.25), v36);

    float v37;
    AddBlock(v35, v32.z, v37);

    float v38;
    DivideBlock(v37, 3, v38);

    vec3 v39;
    LerpBlock(v36, vec3(v38, v38, v38), vec3(0.15, 0.15, 0.15), v39);

    vec3 v40;
    MultiplyBlock(vec3(v39.x, v39.y, v39.z), vec3(1.25, 1.25, 1.25), v40);

    vec3 v41;
    AddBlock(v40, v33, v41);

    vec3 v42;
    AddBlock(v40, v41, v42);

    vec3 v43;
    MultiplyBlock(v42, vec3(0.5, 0.5, 0.5), v43);

    OutputColorBlock(vec4(v43.x, v43.y, v43.z, 0.4));

//>GENERATED_MAIN_SECTION

}
