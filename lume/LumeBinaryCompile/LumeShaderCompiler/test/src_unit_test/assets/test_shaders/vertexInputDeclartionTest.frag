#version 460 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

// sets

// in / out
layout(location = 0) flat in int outColor0;
layout(location = 1) flat in ivec2 outColor1;
layout(location = 2) flat in ivec3 outColor2;
layout(location = 3) flat in ivec4 outColor3;

layout(location = 4) flat in uvec2 UColor2;
layout(location = 5) flat in uvec3 UColor3;
layout(location = 6) flat in uint outColorvec1;


layout(location = 7)  in lowp vec2 outColorvec2;
layout(location = 8)  in lowp vec3 outColorvec3;
layout(location = 9)  in lowp vec4 outColorvec4;
layout(location = 10)  in lowp float outColorvec5;
layout(location = 99)  in  float nobindingPos;
/*
fragment shader for depth pass.
*/
void main(void)
{

}
