#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_control_flow_attributes : enable

// specialization

// includes
#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"
#include "render/shaders/common/render_compatibility_common.h"

// sky
#include "common/atmosphere_lut.h"
// sets

#include "3d/shaders/common/3d_dm_env_frag_layout_common.h"
#define CORE3D_USE_SCENE_FOG_IN_ENV
#include "3d/shaders/common/3d_dm_inplace_env_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"

// in / out

layout(location = 0) in vec2 inUv;
layout(location = 1) in flat uint inIndices;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

// constants
const float PI = 3.1415926535897932384626433832795;
const float DEFAULT_PLANET_RADIUS = 6371e3;
const float DEFAULT_ATMOS_RADIUS = 6471e3;
const vec3 DEFAULT_PLANET_POS = vec3(0.0, -DEFAULT_PLANET_RADIUS, 0.0);
const float DEFAULT_PLANET_BIAS = 5;
const vec3 DEFAULT_RAY_BETA = vec3(5.5e-6, 13.0e-6, 22.4e-6);
const vec3 DEFAULT_MIE_BETA = vec3(4e-6);
const vec3 DEFAULT_AMBIENT_BETA = vec3(.5e-10, .5e-10, .5e-10);
const vec3 DEFAULT_ABSORPTION_BETA = vec3(2.04e-5, 4.97e-5, 1.95e-6);
const float DEFAULT_G = 0.7;
const float DEFAULT_HEIGHT_RAY = 8e3;
const float DEFAULT_HEIGHT_MIE = 1.2e3;
const float DEFAULT_HEIGHT_ABSORPTION = 30e3;
const float DEFAULT_ABSORPTION_FALLOFF = 4e3;
const int DEFAULT_PRIMARY_STEPS = 12;
const int DEFAULT_LIGHT_STEPS = 4;

// structs
struct Ray {
    vec3 o;
    vec3 dir;
};

struct ScatteringResult {
    vec3 mie;
    vec3 ray;
    vec3 ambient;
    vec3 opacity;
};

struct ScatteringLightSource {
    vec3 dir;
    vec3 intensity;
};

struct ScatteringParams {
    vec3 planetPosition; // the position of the planet
    float planetRadius;  // the radius of the planet
    float atmoRadius;    // the radius of the atmosphere
    vec3 betaRay;        // the amount rayleigh scattering scatters the colors (for earth: causes the blue atmosphere)
    vec3 betaMie;        // the amount mie scattering scatters colors
    vec3 betaAbsorption; // how much air is absorbed
    vec3 betaAmbient;    // the amount of scattering that always occurs, cna help make the back side of the atmosphere a
                         // bit brighter
    float
        g; // the direction mie scatters the light in (like a cone). closer to -1 means more towards a single direction
    float heightRay;         // how high do you have to go before there is no rayleigh scattering?
    float heightMie;         // the same, but for mie
    float heightAbsorption;  // the height at which the most absorption happens
    float absorptionFalloff; // how fast the absorption falls off from the absorption height
    int numStepsPrimary;     // the amount of steps along the 'primary' ray, more looks better but slower
    int numStepsLight;       // the amount of steps along the light ray, more looks better but slower
};

ScatteringParams DefaultParameters()
{
    return ScatteringParams(DEFAULT_PLANET_POS, // position of the planet
        DEFAULT_PLANET_RADIUS,                  // radius of the planet in meters
        DEFAULT_ATMOS_RADIUS,                   // radius of the atmosphere in meters
        DEFAULT_RAY_BETA,                       // Rayleigh scattering coefficient
        DEFAULT_MIE_BETA,                       // Mie scattering coefficient
        DEFAULT_ABSORPTION_BETA,                // Absorbtion coefficient
        DEFAULT_AMBIENT_BETA, // ambient scattering, turned off for now. This causes the air to glow a bit when no light
                              // reaches it
        DEFAULT_G,            // Mie preferred scattering direction
        DEFAULT_HEIGHT_RAY,   // Rayleigh scale height
        DEFAULT_HEIGHT_MIE,   // Mie scale height
        DEFAULT_HEIGHT_ABSORPTION,  // the height at which the most absorption happens
        DEFAULT_ABSORPTION_FALLOFF, // how fast the absorption falls off from the absorption height
        DEFAULT_PRIMARY_STEPS,      // steps in the ray direction
        DEFAULT_LIGHT_STEPS);
}


float Hash(vec2 p) {
    return fract(sin(dot(p.xy, vec2(5.34, 7.13))) * 5865.273458);   
}

vec2 Hash2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(123.4, 748.6)), dot(p, vec2(547.3, 659.3)))) * 5232.85324);   
}

vec2 Hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

Ray CreateRay(uint cameraIdx)
{
    // avoid precision loss, by decomposing the matrix
    vec3 position = vec3(uCameras[cameraIdx].view[3].xzy);

    mat4x4 a = mat4(vec4(uCameras[cameraIdx].view[0].xyzw), vec4(uCameras[cameraIdx].view[1].xyzw),
        vec4(uCameras[cameraIdx].view[2].xyzw), vec4(0.0, 0.0, 0.0, 1.0));

    // assume affine matrix, so just take the transpose
    mat4x4 v = transpose(a) * uCameras[cameraIdx].projInv;

    Ray ray;
    vec2 coord = vec2(inUv.x, inUv.y);
#if CORE_BACKEND_TYPE
    vec2 clip = vec2(coord - 0.5) * vec2(2.0, 2.0);
#else
    vec2 clip = vec2(coord);
#endif
    vec4 far = v * vec4(clip, 1.0, 1.0);
    vec4 near = v[2];
    near.xyz /= near.w;
    far.xyz /= far.w;
    ray.o = near.xyz - position;
    ray.dir = normalize(far.xyz - near.xyz);

    return ray;
}

vec3 GetSunPos()
{
    // w component is BG_TYPE_SKY flag used
    return uEnvironmentDataArray[0U].packedSun.xyz;
}

vec3 GetValFromTransmittanceLUT(vec3 viewPos, vec3 sunDir)
{
    ivec2 bufferRes = textureSize(uImgTLutSampler, 0);
    float height = length(viewPos);
    vec3 up = viewPos / height;
    float sunCosZenithAngle = dot(sunDir, up);

    // Use Bruneton 2017 mapping for proper sampling
    vec2 uv = GetTransmittanceTextureUvFromRMu(height, sunCosZenithAngle);

    return texture(uImgTLutSampler, uv).rgb;
}

// Modified from https://github.com/PanosK92/SpartanEngine/blob/master/data/shaders/skysphere.hlsl
vec3 CalculateStars(vec3 rd, float sun_elevation, float time)
{
    float brightness = 0.0;
    vec2 star_uv = rd.xz / (rd.y + 1.0);

    vec2 id = floor(star_uv * 700.0 + 234.0);
    vec2 hash = Hash22(id);
    brightness = step(0.999f, hash.x);

    float twinkleSpeed = 3.0;
    float twinkle = 0.5f + 0.5f * sin(time * twinkleSpeed + hash.y * 6.28318f);
    twinkle = mix(0.3, 1.0, twinkle); // Ensures twinkle never drops below 30% brightness
    brightness *= twinkle;

    float star_factor = clamp(-sun_elevation * 10.0f, 0.0, 1.0);
    brightness *= star_factor;

    return vec3(brightness, brightness, brightness);
}

// Moon rendering based on https://www.shadertoy.com/view/4lBXRR
float Noise(vec2 p) {
    vec2 n = floor(p);
    vec2 f = fract(p);
    f = f * f * f * (3.0 - 2.0 * f);
    vec2 add = vec2(1.0, 0.0);
    float h = mix(mix(Hash(n + add.yy), Hash(n + add.xy), f.x), 
                  mix(Hash(n + add.yx), Hash(n + add.xx), f.x), f.y);
    return h;
}

float Fbm(vec2 p) {
    float h = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++) {
        h += Noise(p) * a;
        p *= 2.0;
        a *= 0.5;
    }
    return h;
}

float Voronoi(vec2 p) {
    vec2 n = floor(p);
    vec2 f = fract(p);
    float md = 5.0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = Hash2(n + g);
            vec2 r = g + o - f;
            float d = dot(r, r);
            md = min(md, d);
        }
    }
    return 1.0 - md;
}

float ComputeShadow(vec3 N, vec3 V, vec3 L)
{
    float dotNV = max(1e-6, dot(N, V));
    float dotNL = max(1e-6, dot(N, L));
    float shadow = dotNL / (dotNL + dotNV);
    return shadow;
}

vec3 CalculateMoonBloom(vec3 dir, vec3 moonDir, vec3 moonColor)
{
    const float moonSolidAngle = acos(0.9999);
    const float minMoonCosTheta = cos(moonSolidAngle);
    
    float cosTheta = dot(dir, moonDir);
    if (cosTheta >= minMoonCosTheta) {
        return vec3(0.0); // Inside moon disc
    }
    
    float offset = minMoonCosTheta - cosTheta;
    float gaussianBloom = exp(-offset * 5000.0) * 0.3;
    float invBloom = 1.0 / (0.001 + offset * 50.0) * 0.005;
    
    float bloomIntensity = clamp(uEnvironmentDataArray[0U].packedPhases.y, 0.1, 1.0);
    return moonColor * (gaussianBloom + invBloom) * bloomIntensity;
}

vec4 RenderMoon(vec3 dir, vec3 moonDir, vec3 moonColor)
{
    // Normalized position on the moon's disc using angular offset
    float cosTheta = dot(dir, moonDir);
    if (cosTheta < 0.9998) {
        return vec4(0.0);
    }
    
    float theta = sqrt(2.0 * (1.0 - cosTheta));
    const float moonAngularRadius = acos(0.9998);
    float normRadius = theta / moonAngularRadius;
    
    vec3 up = abs(moonDir.y) > 0.99 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, moonDir));
    up = normalize(cross(moonDir, right));
    
    // Project the offset direction
    vec3 offsetDir = normalize(dir - moonDir * cosTheta);
    vec2 uv;
    uv.x = dot(offsetDir, right) * normRadius;
    uv.y = dot(offsetDir, up) * normRadius;
    uv *= 0.5;
    
    float dist = length(uv);
    
    vec2 moonTexCoords = uv / 0.44;
    moonTexCoords = moonTexCoords * 2.0;
    
    // Surface textures
    float tex1 = Fbm(moonTexCoords * 2.0);
    float vor = Voronoi(moonTexCoords * 6.0);
    vor = smoothstep(0.0, 0.7, vor);
    float tex2 = Noise(moonTexCoords * 15.0);
    
    // Combine textures for surface
    vec3 surfaceColor = moonColor;
    surfaceColor *= 0.4 + 0.5 * tex1;  // Range: 0.4 to 0.9
    surfaceColor *= 0.7 + 0.2 * vor;   // Range: 0.7 to 0.9 (softer craters)
    surfaceColor *= 0.85 + 0.1 * tex2; // Range: 0.85 to 0.95
    
    float bloomIntensity = clamp(uEnvironmentDataArray[0U].packedPhases.y, 0.1, 1.0);
    vec3 surfaceBloom = moonColor * bloomIntensity; 
    
    surfaceColor += surfaceBloom;
    
    float discAlpha = smoothstep(0.46, 0.42, dist);
    // Edge fade for antialiasing
    discAlpha *= smoothstep(0.0, 0.05, 1.0 - normRadius);
    
    return vec4(surfaceColor, discAlpha);
}

vec4 RenderScene(
    highp vec3 pos, highp vec3 dir, in ScatteringLightSource light, in ScatteringParams params, vec3 viewPos, float planetIntersect)
{
    // the color to use, w is the scene depth
    vec4 color = vec4(0.0, 0.0, 0.0, 1e12);
    
    // Configurable angular threshold for sun disc
    const float celestialBodyAngularThreshold = 0.9998; // ~1.15 degrees angular radius
    bool isMoon = dot(dir, -light.dir) > celestialBodyAngularThreshold;
    vec4 moonColor = vec4(0.8f, 0.8f, 0.85, 1.0);
    vec3 bluishTint = vec3(0.1, 0.15, 0.3);

    vec3 upVector = normalize(viewPos - params.planetPosition);
    float sunElevation = dot(upVector, light.dir);
    bool isNight = sunElevation < 0.0;
    float brightness = clamp(uEnvironmentDataArray[0U].packedPhases.w, 0.1, 1.0);

    // Fade stars based on sun position
    float starsFadeFactor = smoothstep(0.1, -0.15, sunElevation);
    float glowFadeFactor = smoothstep(0.1, -0.3, sunElevation);
    float maxStarsIntensity = 0.4;
    float starsIntensity = starsFadeFactor * maxStarsIntensity;

    if (planetIntersect >= 0.0) {
        color.w = max(planetIntersect, 0.0);
        vec3 samplePos = pos + (dir * planetIntersect) - params.planetPosition;
        vec3 surfaceNormal = normalize(samplePos);
        vec3 moonLight = vec3(0.0);
        vec3 sunLight = vec3(0.0);
        vec3 nightSkyHorizonColor = vec3(0.0);

        if (isNight) {
            float moonIntensity = 0.4; // Moon should have less intensity than sun
            float moonFactor = max(0.0, dot(surfaceNormal, -light.dir));
            moonLight = moonColor.xyz * moonFactor * moonIntensity;
            moonLight *= ComputeShadow(surfaceNormal, -dir, -light.dir) * brightness;

            float glowIntensity = clamp(uEnvironmentDataArray[0U].packedPhases.z, 0.0, 0.1);
            nightSkyHorizonColor = glowIntensity * bluishTint;
        }
        else {
            sunLight *= ComputeShadow(surfaceNormal, -dir, light.dir) * brightness;
        }

        color.xyz += sunLight + moonLight + nightSkyHorizonColor;

    } else {
        vec3 moonBloom = CalculateMoonBloom(dir, -light.dir, moonColor.xyz);
        color.xyz += moonBloom;

        // Render moon disc on top of bloom
        if (isMoon) {
            vec4 moonResult = clamp(RenderMoon(dir, -light.dir, moonColor.xyz), vec4(0.0), vec4(1.0));
            color.xyz = color.xyz * (1.0 - moonResult.w) + moonResult.xyz * moonResult.w;
        }

        if (!isMoon) {
            color.xyz += CalculateStars(dir, sunElevation, uGeneralData.sceneTimingData.z) * starsIntensity;
        }

        if (isNight) {
            float horizonGlow = exp(2.0 * (1.0 - dir.y));
            float glowIntensity = clamp(uEnvironmentDataArray[0U].packedPhases.z, 0.0, 0.1);
            color.xyz += horizonGlow * glowIntensity * glowFadeFactor * bluishTint;
        }
    }
    return color;
}

// Function is from the image tab https://www.shadertoy.com/view/slSXRW
// The function introduces changes compared to the reference
vec3 GetValFromSkyLUT(vec3 rayDir, vec3 sunDir, vec3 viewPos)
{
    float height = length(viewPos);
    vec3 up = viewPos / height;

    float horizonAngle = safeacos(sqrt(height * height - GROUND_RADIUS_KM * GROUND_RADIUS_KM) / height);
    // Negated to match our coordinate system
    float altitudeAngle = -(horizonAngle - acos(dot(rayDir, up)));

    float azimuthAngle;
    if (abs(altitudeAngle) > (0.5 * PI - 0.0001)) {
        azimuthAngle = 0.0;
    } else {
        vec3 right = cross(sunDir, up);
        vec3 forward = cross(up, right);

        vec3 projectedDir = normalize(rayDir - up * (dot(rayDir, up)));
        float sinTheta = dot(projectedDir, right);
        float cosTheta = dot(projectedDir, forward);

        // Original used atan() + PI to shift range from [-PI,PI] to [0,2PI]
        // We use atan() directly for [-PI,PI] then normalize negative values
        azimuthAngle = atan(sinTheta, cosTheta);

        // Convert negative angles to positive equivalent for texture UV mapping
        // Ensures UV.x stays in valid [0,1] range when divided by 2PI
        if (azimuthAngle < 0.0) {
            azimuthAngle += 2.0 * PI;
        }
    }

    float v = 0.5 + 0.5 * sign(altitudeAngle) * sqrt(abs(altitudeAngle) * 2.0 / PI);
    vec2 uv = vec2(azimuthAngle / (2.0 * PI), v);
    vec3 color = texture(uImgSampler, uv).rgb;
    return color;
}

// Function is from the image tab https://www.shadertoy.com/view/slSXRW
vec3 SunWithBloom(vec3 rayDir, vec3 sunDir) {
    const float sunSolidAngle = 0.53 * PI / 180.0;
    const float minSunCosTheta = cos(sunSolidAngle);

    float cosTheta = dot(rayDir, sunDir);

    if (cosTheta >= minSunCosTheta) {
        return vec3(1.0);
    }
    
    float offset = minSunCosTheta - cosTheta;
    float gaussianBloom = exp(-offset * 50000.0) * 0.5;
    float invBloom = 1.0 / (0.02 + offset * 300.0) * 0.01;

    return vec3(gaussianBloom + invBloom);
}

void main(void)
{
    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);

    // uv already in -1 - 1
    CORE_RELAXEDP vec2 clip = inUv.xy;
    highp vec4 far = uCameras[cameraIdx].view * vec4(clip, 1.0, 1.0);
    highp vec4 near = uCameras[cameraIdx].view[2];

    vec3 actualCameraPos = uCameras[cameraIdx].viewInv[3].xyz;
    float actualHeight = length(actualCameraPos);
    float scaledHeight = GROUND_RADIUS_KM + (actualHeight * CAMERA_HEIGHT_SCALE);
    vec3 viewPos = vec3(0.0, scaledHeight, 0.0);

    // project from 4-dimensions to 3-dimensions
    // near.xyz /= near.w;
    // far.xyz /= far.w;

    Ray ray;
    ray = CreateRay(cameraIdx);

    bool isReflected = false;
    {
        vec3 right = uCameras[cameraIdx].view[0].xyz;
        vec3 up = uCameras[cameraIdx].view[1].xyz;
        vec3 forward = uCameras[cameraIdx].view[2].xyz;
        // If it is left handed coordinate system, it is a reflection camera
        isReflected = dot(cross(right, up), forward) < 0.0;
    }

    ray.dir = isReflected ? -ray.dir : ray.dir;
    vec3 light_dir = normalize(GetSunPos());
    ScatteringParams params = DefaultParameters();


    // Sample the sky view
    vec3 lum = GetValFromSkyLUT(ray.dir, light_dir, vec3(viewPos.x, -viewPos.y, viewPos.z));
    vec3 sunLum = SunWithBloom(ray.dir, light_dir);
    // Use smoothstep to limit the effect, so it drops off to actual zero
    sunLum = smoothstep(0.002, 1.0, sunLum);
    highp float planetIntersect = RayIntersectSphere(viewPos, ray.dir, GROUND_RADIUS_KM);
    
    if (length(sunLum) > 0.0) {
        if (planetIntersect >= 0.0) {
            sunLum *= 0.0;
        } else {
            // If the sun value is applied to this pixel, we need to calculate the transmittance to obscure it
            sunLum *= GetValFromTransmittanceLUT(viewPos, light_dir);
        }
    }
    lum += sunLum;

    vec4 scene =
        RenderScene(ray.o, ray.dir, ScatteringLightSource(light_dir, vec3(40)), params, viewPos, planetIntersect);

    lum += scene.xyz;
    outColor = vec4(lum, 1.0);

    gl_FragDepth = 1.0;

    // basic stuff

    vec2 fragUv;
    CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

    // specialization for post process
    if (CORE_POST_PROCESS_FLAGS > 0) {
        InplacePostProcess(fragUv, outColor);
    }

    // ray from camera to an infinity point (NDC) for the current frame
    const vec2 resolution = uGeneralData.viewportSizeInvViewportSize.xy;

    const vec4 currentClipSpacePos =
        vec4(fragUv.x / resolution.x * 2.0 - 1.0, (resolution.y - fragUv.y) / resolution.y * 2.0 - 1.0, 1.0, 1.0);
    vec4 currentWorldSpaceDir = uCameras[cameraIdx].viewProjInv * currentClipSpacePos;
    currentWorldSpaceDir /= currentWorldSpaceDir.w;

    // ray from camera to an infinity point (NDC) for the previous frame
    vec4 prevClipSpacePos = uCameras[cameraIdx].viewProjPrevFrame * currentWorldSpaceDir;
    prevClipSpacePos /= prevClipSpacePos.w;
    const vec2 prevScreenSpacePos =
        vec2((prevClipSpacePos.x + 1.0) * 0.5 * resolution.x, (1.0 - prevClipSpacePos.y) * 0.5 * resolution.y);

    const vec2 velocity = (fragUv.xy - prevScreenSpacePos) * vec2(1.0, -1.0);
    outVelocityNormal = GetPackVelocityAndNormal(velocity, vec3(0.0));
}