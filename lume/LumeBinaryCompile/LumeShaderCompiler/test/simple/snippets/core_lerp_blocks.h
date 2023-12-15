void GenericLerp(in vec2 a, in vec2 b, in float t, out vec2 c)
{
    c = mix(a, b, t);
}

void GenericLerp(in vec3 a, in vec3 b, in float t, out vec3 c)
{
    c = mix(a, b, t);
}

void GenericLerp(in vec4 a, in vec4 b, in float t, out vec4 c)
{
    c = mix(a, b, t);
}
