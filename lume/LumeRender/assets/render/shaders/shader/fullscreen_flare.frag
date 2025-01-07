#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

struct PushConstantStruct {
    vec4 texSizeInvTexSize;
    vec4 rt;
};

layout(push_constant, std430) uniform uPushConstant
{
    PushConstantStruct uPc;
};

float RandomDither(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float Noise(float t)
{
    return RandomDither(t.xx);
}

float Noise(vec2 t)
{
    return RandomDither(t);
}

float RegularPolygon(vec2 p, int N)
{ // Produces a regular polygon defined by N points on the unit circle.
    float a = atan(p.x, p.y) + .2;
    float b = 6.28319 / float(N);
    return smoothstep(.5, .51, cos(floor(.5 + a / b) * b - a) * length(p.xy));
}

float RenderRing(float r, float offsetFromCenter, float fade)
{
    float radiusVal = 0.001 - pow(r - offsetFromCenter, 1.0 / fade) + sin(r * 30.0);
    radiusVal = max(radiusVal, 0.0); // filter negative vals.
    return radiusVal;
}

float RenderHexagon(vec2 uv, vec2 lightPos, vec2 offsetFromCenter, float hexagonIntervalDist, float size)
{
    const int hexagonPoints = 6;
    vec2 hexagonCoord = uv * size + lightPos * hexagonIntervalDist * size + offsetFromCenter;
    float hexValue = 0.01 - RegularPolygon(hexagonCoord, hexagonPoints);
    hexValue = max(hexValue, 0.0); // filter negative vals.
    return hexValue;
}

vec3 Lensflare(vec2 uv, vec2 pos, float time)
{
    vec2 main = (pos - uv);
    vec2 uvd = uv * (length(uv));

    float ang = atan(main.x, main.y);
    float dist = length(main);
    dist = pow(dist, .1);
    float n = Noise(vec2(ang * 16.0, dist * 32.0));

    float f0 = min(1.0 / (length(uv - pos) * 16.0 + 1.0), 0.3);

    f0 = f0 + f0 * (sin(Noise(sin(ang * 2. + pos.x) * 4.0 - cos(ang * 3. + pos.y + time)) * 16.) * .1 + dist * .1 + .8);

    float f1 = max(0.01 - pow(length(uv + 1.2 * pos), 1.9), .0) * 7.0;

    float f2 = max(1.0 / (1.0 + 32.0 * pow(length(uvd + 0.8 * pos), 2.0)), .0) * 00.25;
    float f22 = max(1.0 / (1.0 + 32.0 * pow(length(uvd + 0.85 * pos), 2.0)), .0) * 00.23;
    float f23 = max(1.0 / (1.0 + 32.0 * pow(length(uvd + 0.9 * pos), 2.0)), .0) * 00.21;

    vec2 uvx = mix(uv, uvd, -0.5);

    float f4 = max(0.01 - pow(length(uvx + 0.4 * pos), 2.4), .0) * 6.0;
    float f42 = max(0.01 - pow(length(uvx + 0.45 * pos), 2.4), .0) * 5.0;
    float f43 = max(0.01 - pow(length(uvx + 0.5 * pos), 2.4), .0) * 3.0;

    uvx = mix(uv, uvd, -.4);

    float f5 = max(0.01 - pow(length(uvx + 0.2 * pos), 5.5), .0) * 2.0;
    float f52 = max(0.01 - pow(length(uvx + 0.4 * pos), 5.5), .0) * 2.0;
    float f53 = max(0.01 - pow(length(uvx + 0.6 * pos), 5.5), .0) * 2.0;

    uvx = mix(uv, uvd, -0.5);

    float f6 = max(0.01 - pow(length(uvx - 0.3 * pos), 1.6), .0) * 6.0;
    float f62 = max(0.01 - pow(length(uvx - 0.325 * pos), 1.6), .0) * 3.0;
    float f63 = max(0.01 - pow(length(uvx - 0.35 * pos), 1.6), .0) * 5.0;

    vec3 c = vec3(.0);

    c.r += f2 + f4 + f5 + f6;
    c.g += f22 + f42 + f52 + f62;
    c.b += f23 + f43 + f53 + f63;
    c = c * 1.3 - vec3(length(uvd) * .05);
    c += vec3(f0) * 0.15;

    float sizes[] = { 7., 9., 12., 15., 8.5 };

    for (int i = 0; i < 5; ++i) {
        // Draws hexagons in order closest to light to farthest.
        vec2 hexagon_offset = -pos * vec2(1.5) * float(i) + pos * 1.22;
        float hexagon_size = 11.0 - 2.0 * float(i);
        float hexagon_fade = 0.25 * pow(float(i), 2.5);
        float hexagon = RenderHexagon(uv, -pos, -hexagon_offset, dist, sizes[i]) * hexagon_fade;
        c += vec3(hexagon, hexagon, hexagon);
    }

    if (true) {
        int i = 0;
        float size = 0.4;
        float ring_fade = 70.0 * float(2);
        vec2 ring_offset =
            // vec2(0.0);//<== rings around light source.
            -pos * vec2(2.5) * float(i); //<== ring halos in light ray.
        float ring_size = size * 5.0 * float(i);
        float lt_coord = length(uv - pos - ring_offset);
        float ring_radius = lt_coord + ring_size;
        float ring = RenderRing(ring_radius, ring_size, ring_fade);
        c += vec3(ring, ring, ring);
    }

    return c;
}

vec3 Lensflare2(vec2 uv, vec2 pos, float time)
{
    vec2 main = pos - uv;
    vec2 direction = normalize(main);

    float dist = length(main);

    vec2 point = vec2(0.5, 0.5);
    float l = length(uv - point);

    // return vec3(1 - clamp( l / 0.3, 0, 1));

    int i = 1;
    vec2 hexagon_offset = point - uv;
    float hexagon_size = 11.0 - 2.0 * float(i);
    float hexagon_fade = 0.25 * pow(float(i), 2.5);
    float hexagon = max(0.01 - RenderHexagon(uv, pos, vec2(0, 0), 0.0, 0.3), 0);

    return vec3(1 - RegularPolygon((uv - (vec2(0.5, 0.5) + direction * -0.2)) * 9, 6)) * 0.1;
}

vec3 ColorModifier(vec3 color, float factor, float factor2) // color modifier
{
    float w = color.x + color.y + color.z;
    return mix(color, vec3(w) * factor, w * factor2);
}

//   uniform int uGhosts; // number of ghost samples
//   uniform float uGhostDispersal; // dispersion factor
//   noperspective in vec2 vTexcoord;

vec4 Plane(vec2 point, vec2 normal)
{
    vec4 p = vec4(point.xy, 0, 1);
    vec4 n = vec4(normal.xy, 0, 1);
    return vec4(n.x, n.y, n.z, -dot(p, n));
}

float Diag(vec2 p0, vec2 p1)
{
    vec2 d = abs(p1 - p0);
    return max(d.x, d.y);
}

float Line(vec2 p, vec2 p0, vec2 p1)
{
    /*
    float stp = diag(p,p0);
    float n = diag(p0,p1);
    float t = (n==0.0)? 0.0 : stp/n;

    vec2 pt = mix(p0, p1, t);
    vec2 d = abs(p - round(pt));
    return (d.x < 0.5
         && d.y < 0.5
         && stp <= n)
          ? 1.0 : 0.0 ;
    */

    vec2 dir = normalize(p0 - p1);

    vec2 normal = -dir.xy;
    vec4 plane2 = Plane(p0, normal);
    float d = dot(vec4(p, 0, 1), plane2);
    if (d < 0)
        return 0;

    vec2 normal1 = dir.yx * vec2(1, -1);
    vec4 plane1 = Plane(p0, normal1);
    float d1 = abs(dot(vec4(p, 0, 1), plane1)) / 1;
    return 1 - clamp(d1, 0, 1);
}

vec4 Grid()
{
    if (mod(gl_FragCoord.x, 32) > 1.0 && mod(gl_FragCoord.y, 32) > 1.0) {
        return vec4(0, 0, 0, 0);
    } else {
        return vec4(0.1, 0.1, 0.1, 0.1);
    }
}

vec4 Guidlines()
{
    vec2 rt_min = floor(uPc.rt.xy * 0.5);
    vec2 rt_max = floor(uPc.rt.xy * 0.5) + 1.f;

    if (gl_FragCoord.x >= rt_min.x && gl_FragCoord.x <= rt_max.x) {
        return vec4(1.0, 0.0, 0.0, 0.0);
    } else if (gl_FragCoord.y >= rt_min.y && gl_FragCoord.y <= rt_max.y) {
        return vec4(1.0, 0.0, 0.0, 0.0);
    } else {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
}

float Point(vec2 p, vec2 p1)
{
    vec2 d = (p1 - p);
    return 1 - clamp(length(d) / 10, 0, 1);
}

vec4 DrawFlareGuid(vec2 source, vec2 end)
{
    vec4 guides = vec4(0, 0, 0, 0);
    guides += vec4(Line(gl_FragCoord.xy, source, end), 0, 0, 1);
    guides += vec4(Point(gl_FragCoord.xy, source), 0, 0, 1);
    guides += vec4(Point(gl_FragCoord.xy, end), 0, 0, 1);
    return guides;
}

void main(void)
{
    vec2 aspectRatio = vec2(uPc.rt.x / uPc.rt.y, 1);
    float iTime = uPc.texSizeInvTexSize.x;

    // float x = 0.5 + sin(iTime)*.5;
    // float y = 0.5 + cos(iTime*.913)*.5;

    vec2 rtSize = uPc.rt.xy;
    vec2 projectSunPos = uPc.texSizeInvTexSize.yz;
    vec3 c = Lensflare((inUv.xy - 0.5) * aspectRatio, (projectSunPos - 0.5) * aspectRatio, iTime);
    vec3 color = vec3(1.4, 1.2, 1.0) * c;
    color -= Noise(inUv.xy) * .015;
    color = ColorModifier(color, .5, .1);

    vec2 center = uPc.rt.xy * 0.5;

    // debugging
    vec4 guides = vec4(0, 0, 0, 0);
    // guides += grid();
    // guides += guidlines();
    // guides += drawFlareGuid(projectSunPos * rtSize, center);

    // guides += drawFlareGuid(center - vec2(100,50), center);

    const float intensity = uPc.rt.z;
    outColor = guides + vec4(color, 0) * intensity;
}
