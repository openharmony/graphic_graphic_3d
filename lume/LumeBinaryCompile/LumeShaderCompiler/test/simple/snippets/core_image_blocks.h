void GenericImageSample(in vec2 inUv, in sampler2D inSampler, out vec4 outColor)
{
    outColor = texture(inSampler, inUv);
}
