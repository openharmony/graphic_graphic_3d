/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MATH_H
#define MATH_H

#include "common/cloud_structs.h"

float remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
    return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

// clamps the input value to (inMin, inMax) and performs a remap
float clampRemap(in float val, in float inMin, in float inMax, in float outMin, in float outMax)
{
    float clVal = clamp(val, inMin, inMax);
    return (clVal - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
}

// swaps a and b
void swap(inout float a, inout float b)
{
    float t = b;
    b = a;
    a = t;
}

// calculates three weights (w1, w2, w3) for a parameter t in [0, 1]
vec3 lerp3(float t)
{
    float x = clamp(1 - t * 2, 0, 1);
    float z = clamp((t - 0.5) * 2, 0, 1);
    float y = 1 - x - z;
    return vec3(x, y, z);
}

float hash(float n)
{
    return fract(sin(n) * 1e4);
}
float hash(vec2 p)
{
    return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x))));
}

float noise(float x)
{
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

float noise(vec2 x)
{
    vec2 i = floor(x);
    vec2 f = fract(x);

    // Four corners in 2D of a tile
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

vec3 ScreenSpaceDither(vec2 vScreenPos, float g_flTime)
{
    // Iestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified for VR
    vec3 vDither = vec3(dot(vec2(171.0, 231.0), vScreenPos.xy + g_flTime));
    vDither.rgb = fract(vDither.rgb / vec3(103.0, 71.0, 97.0)) - vec3(0.5, 0.5, 0.5);
    return (vDither.rgb / 255.0) * 0.375;
}

float mod289(float x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
vec4 mod289(vec4 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
vec4 perm(vec4 x)
{
    return mod289(((x * 34.0) + 1.0) * x);
}

float noise(vec3 p)
{
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float LinearizeDepth(float d, float zNear, float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

vec2 IntersectLayer(in Ray ray, in vec2 h)
{
    vec2 d = (h - ray.o.y);
    return d / ray.dir.y;
}

vec2 IntersectLayer(in Ray ray, in vec3 center, in float radius, in vec2 h, in vec3 up)
{
    vec2 d = (h - ray.o.y);
    return d / ray.dir.y;
}

float ComputeSamples(in Ray ray, in vec3 up)
{
    // calculate the cosine (mirrored) between the ray and up axis
    float cosTheta = abs(dot(ray.dir, vec3(0, 1, 0)));

    // take the range [0, 0.5] and transform it into [0, 1]
    float grada = clamp(cosTheta * 2.0, 0, 1);

    // take the range [0.5, 1] and transform it into [1, 0]
    float gradb = 1 - (clamp(cosTheta * 2.0, 1, 2) - 1);

    // just need to multiply the gradients together and you have a value that
    // indicates 45 degrees at the zenith and nadir
    float grad = smoothstep(0, 1, grada * gradb);

    return (1 - cosTheta);
}

vec2 LatitudeLongitude(vec3 p, float r)
{
    float lat1 = asin(p.z / r);
    float lon1 = atan(p.y, p.x);
    return vec2(lat1, lon1);
}

vec2 LatitudeLongitudeSubtract(vec2 p0, vec2 p1)
{
    return mod(HALF_PI + (p1 - p0), PI) - HALF_PI;
}

vec2 WeathermapPosition(vec3 pos)
{
    const vec3 planetCenter = vec3(0, -DEFAULT_PLANET_RADIUS, 0);
    const vec2 p0 = LatitudeLongitude(vec3(0, 0, 0) + planetCenter, DEFAULT_PLANET_RADIUS);
    const vec2 p1 = LatitudeLongitude(pos + planetCenter, length(pos - planetCenter));
    const float one_degree_radians = 0.01745329252;
    vec2 uv = LatitudeLongitudeSubtract(p0, p1) / one_degree_radians / weatherMapScale;
    return 0.5 + uv.yx;
}

float override(float val, float mapped)
{
    if (mapped >= 0) {
        return mix(val, 1, mapped);
    } else {
        return mix(val, 0, -mapped);
    }
}

struct Hit {
    float tmin;
    float tmax;
};

bool BBoxIntersect(const vec3 boxMin, const vec3 boxMax, const Ray r, out Hit hit)
{
    vec3 tbot = r.dir * (boxMin - r.o);
    vec3 ttop = r.dir * (boxMax - r.o);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    float t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    float t1 = min(t.x, t.y);
    hit.tmin = t0;
    hit.tmax = t1;
    return t1 > max(t0, 0.0);
}

/*
A ray-sphere intersect
This was previously used in the atmosphere as well, but it's only used for the planet intersect now, since the
atmosphere has this ray sphere intersect built in
*/

highp vec3 RaySphereIntersect(highp vec3 start, // starting position of the ray
    highp vec3 dir,                             // the direction of the ray
    highp float radius                          // and the sphere radius
)
{
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, start);
    float c = dot(start, start) - (radius * radius);
    float d = (b * b) - 4.0 * a * c;
    if (d < 0.0)
        return vec3(0.0, 0.0, 0.0);

    return vec3((-b - sqrt(d)) / (2.0 * a), (-b + sqrt(d)) / (2.0 * a), 1.0);
}

bool FindAtmosphereIntersections(Ray ray, vec3 planetPosition, vec3 radius, out vec2 tIntersect)
{
    // find the intersction points for the planet
    vec3 planet = RaySphereIntersect(ray.o - planetPosition, ray.dir, radius.x);

    // find the intersction points for the troposphere
    vec3 atmosphere1 = RaySphereIntersect(ray.o - planetPosition, ray.dir, radius.y);
    vec3 atmosphere2 = RaySphereIntersect(ray.o - planetPosition, ray.dir, radius.z);

    // if we are above the troposphere
    if (dot(ray.o - planetPosition, ray.o - planetPosition) > (radius.z * radius.z)) {
        float maxDistance = 1e12;
        if (atmosphere2.y > 0)
            maxDistance = max(0.02, min(maxDistance, atmosphere2.y));
        if (atmosphere1.y > 0)
            maxDistance = max(0.02, min(maxDistance, atmosphere1.y));
        if (atmosphere1.x > 0)
            maxDistance = max(0.02, min(maxDistance, atmosphere1.x));
        tIntersect = vec2(atmosphere2.x, maxDistance);
        if (atmosphere2.x < 0)
            return false;
        if (atmosphere1.x < 0)
            return false;
        return true;
    }
    // if we are between the troposphere
    else if (dot(ray.o - planetPosition, ray.o - planetPosition) > (radius.y * radius.y)) {
        float maxDistance = 1e12;

        // restrain the maximum distance to the end of the troposphere
        if (atmosphere2.x > 0)
            maxDistance = max(0.0, min(maxDistance, atmosphere2.x));
        if (atmosphere2.y > 0)
            maxDistance = max(0.0, min(maxDistance, atmosphere2.y));

        // restrain the maximum distance to the start of the troposphere
        if (atmosphere1.x != atmosphere1.y) {
            if (atmosphere1.x > 0)
                maxDistance = max(0.0, min(maxDistance, atmosphere1.x));
            if (atmosphere1.y > 0)
                maxDistance = max(0.0, min(maxDistance, atmosphere1.y));
        }

        // restrain the maximum distance to the planet, this should never happen in theory though.
        if (planet.x != planet.y) {
            if (planet.x > 0)
                maxDistance = max(0.0, min(maxDistance, planet.x));
            if (planet.y > 0)
                maxDistance = max(0.0, min(maxDistance, planet.y));
        }

        tIntersect = vec2(0.0, max(1, maxDistance));
        return true;
    }
    // if we are below the troposphere
    else if (dot(ray.o - planetPosition, ray.o - planetPosition) > (radius.x * radius.x)) {
        if (planet.y > 0 || planet.x > 0) {
            return false;
        }

        tIntersect = vec2(atmosphere1.y, atmosphere2.y);
        return true;
    }
    // if we are surface of the planet
    else {
        return false;
    }

    return false;
}

float Luminance(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}
#endif // MATH_H
