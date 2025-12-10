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
// Texture slot declarations:
const uint texture_1 = 0;
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
    ColorVec3Block(1.0, 0.6, 0.4, v1);

    vec3 v2;
    WorldPositionBlock(v2);

    vec3 v3;
    ColorVec3Block(0.75, 0.55, 0.95, v3);

    vec3 v4;
    ColorVec3Block(0.0, 0.5, 1, v4);

    vec3 v5;
    WorldPositionBlock(v5);

    vec3 v6;
    CameraViewDirBlock(v6);

    float v7;
    ElapsedTimeBlock(v7);

    vec3 v8;
    InputNormalBlock(v8);

    vec3 v9;
    Vec3SwizzleBlock(0.25, 0.5, 0, v9);

    vec3 v10;
    ColorVec3Block(1.0, 0.65, 0.0, v10);

    vec4 v11;
    MultiplyBlock(vec4(1, 1, 1, 1), vec4(0.8, 0.8, 1, 0.6), v11);

    vec3 v12;
    Vec3SwizzleBlock(-0.25, -0.5, 0, v12);

    vec3 v13;
    InputNormalBlock(v13);

    vec3 v14;
    MultiplyBlock(v5, vec3(0.3, 0.3, 0.3), v14);

    vec3 v15;
    VecNormalizeBlock(v6, v15);

    float v16;
    NegateBlock(v7, v16);

    vec3 v17;
    VecNormalizeBlock(v8, v17);

    vec3 v18;
    SubtractBlock(v9, v12, v18);

    vec3 v19;
    rotateXBlock(v7, v14, v19);

    vec3 v20;
    MultiplyBlock(v15, vec3(-1, -1, -1), v20);

    vec3 v21;
    rotateXBlock(v16, v2, v21);

    float v22;
    VecDotBlock(v18, v18, v22);

    vec4 v23;
    MaterialTextureInfoSlotSampleBlock(texture_1, vec2(v19.y, v19.z), v23);

    vec4 v24;
    MaterialTextureInfoSlotSampleBlock(texture_1, vec2(v19.z, v19.x), v24);

    vec4 v25;
    MaterialTextureInfoSlotSampleBlock(texture_1, vec2(v19.x, v19.y), v25);

    float v26;
    VecDotBlock(v17, v20, v26);

    vec3 v27;
    SubtractBlock(v21, v12, v27);

    vec4 v28;
    TriplanarMappingBlock(v23, v24, v25, v13, 8, v28);

    float v29;
    OneMinusBlock(v26, v29);

    float v30;
    VecDotBlock(v18, v27, v30);

    vec4 v31;
    MultiplyBlock(v11, v28, v31);

    float v32;
    PowerBlock(v29, 2.5, v32);

    float v33;
    DivideBlock(v30, v22, v33);

    float v34;
    AddBlock(v32, 0.15, v34);

    float v35;
    ClampBlock(v33, 0, 1, v35);

    float v36;
    MultiplyBlock(v34, 1, v36);

    float v37;
    SmoothStepBlock(v35, 0.70, 1, v37);

    float v38;
    SmoothStepBlock(v35, 0, 0.35, v38);

    float v39;
    SmoothStepBlock(v35, 0.35, 0.70, v39);

    vec3 v40;
    LerpBlock(v4, v3, vec3(v38, v38, v38), v40);

    vec3 v41;
    LerpBlock(v40, v1, vec3(v39, v39, v39), v41);

    vec3 v42;
    LerpBlock(v41, v10, vec3(v37, v37, v37), v42);

    vec3 v43;
    PowerBlock(v42, vec3(2.2, 2.2, 2.2), v43);

    vec3 v44;
    MultiplyBlock(v43, vec3(0.25, 0.25, 0.25), v44);

    vec3 v45;
    MultiplyBlock(v44, vec3(v36, v36, v36), v45);

    vec4 v46;
    AddBlock(vec4(v45.x, v45.y, v45.z, 1), vec4(v31.x, v31.y, v31.z, 1), v46);

    vec3 v47;
    MultiplyBlock(vec3(v46.x, v46.y, v46.z), vec3(2.5, 2.5, 2.5), v47);

    vec3 v48;
    AddBlock(v47, v47, v48);

    OutputColorBlock(vec4(v48.x, v48.y, v48.z, v36));

//>GENERATED_MAIN_SECTION

}
