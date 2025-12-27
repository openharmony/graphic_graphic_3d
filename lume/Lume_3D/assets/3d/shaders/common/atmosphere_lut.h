/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ATMOSPHERE_LUT_H
#define ATMOSPHERE_LUT_H

const float M_PI = 3.14159265358;

const vec3 GROUND_ALBEDO = vec3(0.3, 0.3, 0.3);

const uint SUN_TRANSMITTANCE_STEPS = 40;
const uint ATMOSPHERE_RADIUS_KM = 6471;
const uint GROUND_RADIUS_KM = 6371;
const float CAMERA_HEIGHT_SCALE = 1e-3;

const float RAYLEIGH_ABSORPTION_BASE = 0.0;

const uint MULTIPLE_SCATTERING_STEPS = 100;
const uint AERIAL_PER_SLICE_MARCH_COUNT = 1;
const int SQRT_SAMPLES = 8;

struct AtmosphericConfig {
    vec3 rayleighScatteringBase;
    vec3 ozoneAbsorptionBase;
    float mieScatteringBase;
    float mieAbsorptionBase;
    vec3 groundColor;
    float skyViewBrightness;
};

float Safeacos(const float x)
{
    return acos(clamp(x, -1.0, 1.0));
}

void GetScatteringValues(vec3 pos, const AtmosphericConfig atmosphericConfig, out vec3 rayleighScattering,
    out float mieScattering, out vec3 extinction)
{
    float altitudeKM = (length(pos) - GROUND_RADIUS_KM);
    // Note: Paper gets these switched up
    float rayleighDensity = exp(-altitudeKM / 8.0);
    float mieDensity = exp(-altitudeKM / 1.2);

    rayleighScattering = (atmosphericConfig.rayleighScatteringBase * CAMERA_HEIGHT_SCALE) * rayleighDensity;
    float rayleighAbsorption = RAYLEIGH_ABSORPTION_BASE * rayleighDensity;

    mieScattering = (atmosphericConfig.mieScatteringBase * CAMERA_HEIGHT_SCALE) * mieDensity;
    float mieAbsorption = (atmosphericConfig.mieAbsorptionBase * CAMERA_HEIGHT_SCALE) * mieDensity;

    vec3 ozoneAbsorption =
        (atmosphericConfig.ozoneAbsorptionBase * CAMERA_HEIGHT_SCALE) * max(0.0, 1.0 - abs(altitudeKM - 25.0) / 15.0);

    extinction = rayleighScattering + rayleighAbsorption + mieScattering + mieAbsorption + ozoneAbsorption;
}

// From https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code.
float RayIntersectSphere(vec3 ro, vec3 rd, float rad)
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - rad * rad;
    if (c > 0.0f && b > 0.0) {
        return -1.0;
    }
    float discr = b * b - c;
    if (discr < 0.0) {
        return -1.0;
    }
    // Special case: inside sphere, use far discriminant
    if (discr > b * b) {
        return (-b + sqrt(discr));
    }
    return -b - sqrt(discr);
}

// Ray-sphere intersection that returns both entry and exit distances
vec2 RayIntersectSphere2D(vec3 start, vec3 dir, float radius)
{
    // Ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, start);
    float c = dot(start, start) - (radius * radius);
    float d = (b * b) - 4.0 * a * c;
    if (d < 0.0) {
        return vec2(1e5, -1e5);
    }
    return vec2((-b - sqrt(d)) / (2.0 * a), (-b + sqrt(d)) / (2.0 * a)); // 2.0: scale
}

vec3 GetSphericalDir(float theta, float phi)
{
    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    return vec3(sinPhi * sinTheta, cosPhi, sinPhi * cosTheta);
}

float GetMiePhase(float cosTheta)
{
    const float g = 0.8;
    const float scale = 3.0 / (8.0 * M_PI);

    float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
    float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

    return scale * num / denom;
}

float GetRayleighPhase(float cosTheta)
{
    const float k = 3.0 / (16.0 * M_PI);
    return k * (1.0 + cosTheta * cosTheta);
}

// Get (r, mu) from UV coordinates using the Bruneton's 2017 mapping
// This provides better sampling near the horizon
void GetRMuFromTransmittanceTextureUv(vec2 uv, out float r, out float mu)
{
    // h: height of atmosphere above ground level
    float h = sqrt(ATMOSPHERE_RADIUS_KM * ATMOSPHERE_RADIUS_KM - GROUND_RADIUS_KM * GROUND_RADIUS_KM);
    float xMu = uv.x;
    float xR = uv.y;

    // rho: horizontal distance from earth center to the point at radius r
    float rho = h * xR;
    r = sqrt(rho * rho + GROUND_RADIUS_KM * GROUND_RADIUS_KM);

    // Distance bounds for ray from point at radius r to atmosphere boundary
    float dMin = ATMOSPHERE_RADIUS_KM - r;
    float dMax = rho + h;
    float d = dMin + xMu * (dMax - dMin);

    // Cosine of view angle
    mu = (d == 0.0) ? 1.0 : (h * h - rho * rho - d * d) / (2.0 * r * d);
    mu = clamp(mu, -1.0, 1.0);
}

// Inverse of the Bruneton 2017 transmittance mapping
// Converts (radius, cosine_zenith_angle) back to UV coordinates for LUT sampling
vec2 GetTransmittanceTextureUvFromRMu(float r, float mu)
{
    float h = sqrt(ATMOSPHERE_RADIUS_KM * ATMOSPHERE_RADIUS_KM - GROUND_RADIUS_KM * GROUND_RADIUS_KM);

    float rho = sqrt(max(0.0, r * r - GROUND_RADIUS_KM * GROUND_RADIUS_KM));

    float dMin = ATMOSPHERE_RADIUS_KM - r; // Minimum distance (straight up)
    float dMax = rho + h;                  // Maximum distance (tangent to ground)

    // Solve for distance d using quadratic formula
    // Original: mu = (h^2 - rho^2 - d^2) / (2 * r * d)
    // Rearranged: d^2 + 2*r*mu*d - (h^2 - rho^2) = 0
    // Applying quadratic formula gives discriminant = r^2*mu^2 + h^2 - rho^2
    // Solution: d = -r*mu + sqrt(discriminant)

    // discriminant = r^2*mu^2 + ATMOSPHERE_RADIUS_KM^2 - GROUND_RADIUS_KM^2 - r^2 + GROUND_RADIUS_KM^2

    // Expand and simplify (GROUND_RADIUS_KM cancels out and r^2 factors out)
    float discriminant = r * r * (mu * mu - 1.0) + ATMOSPHERE_RADIUS_KM * ATMOSPHERE_RADIUS_KM;

    // Take positive root (distance must be positive)
    float d = max(0.0, -r * mu + sqrt(max(0.0, discriminant)));

    // Convert back to normalized UV coordinates
    float xMu = clamp((d - dMin) / (dMax - dMin), 0.0, 1.0); // Angle coordinate
    float xR = clamp(rho / h, 0.0, 1.0);                        // Height coordinate

    return vec2(xMu, xR);
}

float DistanceToTopAtmosphereBoundary(float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + ATMOSPHERE_RADIUS_KM * ATMOSPHERE_RADIUS_KM;
    return max(0.0, -r * mu + sqrt(max(0.0, discriminant)));
}

vec3 ComputeTransmittanceToTopAtmosphereBoundary(float r, float mu, const AtmosphericConfig atmosphericConfig)
{
    vec3 pos = vec3(0.0, r, 0.0);
    vec3 dir = vec3(sqrt(max(0.0, 1.0 - mu * mu)), mu, 0.0);

    // Early exit if ray hits ground
    if (mu < 0.0 && r * r * (mu * mu - 1.0) + GROUND_RADIUS_KM * GROUND_RADIUS_KM >= 0.0) {
        return vec3(0.0);
    }

    float atmoDist = DistanceToTopAtmosphereBoundary(r, mu);
    float dt = atmoDist / SUN_TRANSMITTANCE_STEPS;

    vec3 transmittance = vec3(1.0);

    // Use trapezoidal integration for better accuracy
    for (float i = 0.5; i < SUN_TRANSMITTANCE_STEPS; i += 1.0) {
        float t = i * dt;
        vec3 samplePos = pos + t * dir;

        vec3 rayleighScattering;
        vec3 extinction;
        float mieScattering;
        GetScatteringValues(samplePos, atmosphericConfig, rayleighScattering, mieScattering, extinction);

        transmittance *= exp(-dt * extinction);
    }

    return transmittance;
}

#endif // ATMOSPHERE_LUT_H
