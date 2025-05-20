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
#include "common/cloud_structs.h"
#include "common/cloud_constants.h"
#include "common/cloud_math.h"
#include "common/cloud_scattering.h"

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

Ray CreateRay(uint cameraIdx)
{
    // avoid precision loss, by decomposing the matrix
    vec3 position = vec3(uCameras[cameraIdx].view[3].xyz);

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

/*
float GetHeightFractionForPoint(vec3 inPosition, vec3 inCloudMinMax)
{
    float a = (inPosition.z - inCloudMinMax.x);
    float b = (inCloudMinMax.y - inCloudMinMax.x);
    float c = a / b;
    return clamp(c, 0, 1);
}


vec2 cloud_min_max()
{
    return vec2(innerHeight, outerHeight);
}
*/

vec4 RenderScene(highp vec3 pos, highp vec3 dir, in ScatteringLightSource light, in ScatteringParams params)
{
    // the color to use, w is the scene depth
    vec4 color = vec4(0.0, 0.0, 0.0, 1e12);

    // add a sun, if the angle between the ray direction and the light direction is small enough, color the pixels
    // whitestar
    color.xyz = vec3(dot(dir, light.dir) > 0.9998 ? 3.0 : 0.0);
    color.xyz += CalculateStars(dir) * 0.04;
    highp vec3 planetIntersect =
        RaySphereIntersect(pos - params.planetPosition, dir, params.planetRadius - DEFAULT_PLANET_BIAS);

    if (0.0 < planetIntersect.y) {
        color.w = max(planetIntersect.x, 0.0);
        vec3 samplePos = pos + (dir * planetIntersect.x) - params.planetPosition;
        vec3 surfaceNormal = normalize(samplePos);
        vec3 groundAlbedo = vec3(0.3, 0.3, 0.2);
        float sunFactor = max(0.0, dot(surfaceNormal, light.dir));
        color.xyz = groundAlbedo * (0.2 + 0.8 * sunFactor);

        color.xyz *= ComputeShadow(surfaceNormal, -dir, light.dir);
        color.xyz += clamp(
            CalculateScatteringSkylight(samplePos, surfaceNormal, vec3(0.0), light, params) * vec3(0.0, 0.25, 0.05),
            0.0, 1.0);
    }

    return color;
}

vec3 GetSunPos()
{
//    const float ELEVATION = 0.1;
//    vec3 sunDir = uEnvironmentDataArray[0U].packedSun.xyz;
//    sunDir = normalize(sunDir);
//    float azimuth = atan(sunDir.x, sunDir.z);
//    
//    vec3 horizonSunDir = vec3(
//        cos(ELEVATION) * cos(azimuth),
//        sin(ELEVATION),
//        cos(ELEVATION) * sin(azimuth)
//    );
//    
//    return horizonSunDir;
    return uEnvironmentDataArray[0U].packedSun.xyz;
}

void main(void)
{
    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);

    // uv already in -1 - 1
    CORE_RELAXEDP vec2 clip = inUv.xy;
    highp vec4 far = uCameras[cameraIdx].view * vec4(clip, 1.0, 1.0);
    highp vec4 near = uCameras[cameraIdx].view[2];

    // project from 4-dimensions to 3-dimensions
    // near.xyz /= near.w;
    // far.xyz /= far.w;

    Ray ray;
    ray = CreateRay(cameraIdx);

    // get the camera vector
    vec3 camera_vector = ray.dir;
    vec3 camera_position = ray.o;
    vec3 light_dir = normalize(GetSunPos());

    // get the scene color and depth, color is in xyz, depth in w
    // replace this with something better if you are using this shader for something else
    ScatteringParams params = DefaultParameters();
    vec4 scene = RenderScene(camera_position, camera_vector, ScatteringLightSource(light_dir, vec3(40)), params);
    ScatteringResult result;

    CalculateScatteringComponents(Camera(camera_position, // the position of the camera
                                      camera_vector,      // the camera vector (ray direction of this pixel)
                                      scene.w),
        ScatteringLightSource(light_dir, // light direction
            vec3(40.0)),
        params, result);

    outColor = vec4(ScatterTonemapping(CalculateScatteringComposite(result, scene.xyz)), 1.0);

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
