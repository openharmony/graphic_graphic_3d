#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (constant_id = 0) const int CORE_BACKEND_TYPE = 1;
layout (constant_id = 1) const uint NUM_SAMPLES = 64;
layout (constant_id = 2) const float ConstantFloat = 2.0;
layout (constant_id = 3) const bool  testBool = true;

struct PointLight{
vec3 position;
vec3 color;
float intensity;
mat4 testMat;
};

layout (push_constant) uniform PushConstants{
vec4 color;
float FloatValue;
vec4 sampleColors[16];
uint NUM_SAMPLE_COLOR ;
PointLight memberStruct0;
mat4 testMat;
} pushConstants;

layout(std140, set = 0, binding = 0) uniform SamplesUBO {
   vec3 samples[NUM_SAMPLES];
} S;

// includes

// sets

// in / out
layout(location = 0) out vec4 outColor;
/*
fragment shader for depth pass.
*/
void main(void)
{
outColor=pushConstants.color;
}
