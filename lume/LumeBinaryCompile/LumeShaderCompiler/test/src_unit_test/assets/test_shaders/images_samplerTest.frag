#version 460 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable


layout(set = 0, binding = 0) uniform sampler2D samplerA;
layout(set = 0, binding = 1,rgba8) uniform readonly image2D imageA;
layout(set = 0, binding = 2,rgba8) uniform writeonly image2D imageB;
layout(input_attachment_index=0,set = 0, binding = 3) uniform subpassInput  mySubpassInput;

layout(set = 1, binding = 0) uniform texture2D texImage;

layout(set = 0, binding = 4) uniform accelerationStructureEXT myAccStructure;
// includes

// sets

// in / out
layout(location = 0) out vec4 outColor;

/*
fragment shader for depth pass.
*/
void main(void)
{
vec2 texCoords=vec2(0.1,0.1);
vec4 colorA=texture(samplerA, texCoords);
vec4 colorB=subpassLoad(mySubpassInput);
imageStore(imageB, ivec2(gl_FragCoord.xy), colorA);
outColor=colorA;

}
