#version 460 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout(set = 0, ,) uniform accelerationStructureEXT myAccStructure;
// includes
#include"Test_include_None-exist.h"
// sets

// in / out
layout(location = 0) out vec4 outColor;
/*
fragment shader for depth pass.
*/
void main(void)
{

}
