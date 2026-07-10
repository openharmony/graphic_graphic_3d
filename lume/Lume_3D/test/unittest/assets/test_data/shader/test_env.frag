#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets

// in 3d_dm_env_frag_layout set 3 has three combined iamge samplers while the app tries to use separate images and a 
// sampler. with GL(ES) the duplicate set + bindings cause confusion so the contents of the include are copied here 
// except for the set 3 uniforms.
// #include "3d/shaders/common/3d_dm_env_frag_layout_common.h"

#define CORE3D_ENVIRONMENT_FRAG_LAYOUT
#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"
layout(set = 0, binding = 2, std140) uniform uEnvironmentStructDataArray
{
    DefaultMaterialEnvironmentStruct uEnvironmentDataArray[CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT];
};
layout(constant_id = CORE_DM_CONSTANT_ID_ENV_TYPE) const uint CORE_DEFAULT_ENV_TYPE = 0;

// override bindings (there are bindings coming from 3d_dm_env_frag_layout_common.h
// -> we cannot use only reflection,
// we need a separate pipeline layout defined to have the correct pipeline layout in use
layout(set = 3, binding = 0) uniform textureCube uMyCustomCubeTex;
layout(set = 3, binding = 1) uniform texture2D uMyCustomTex;
layout(set = 3, binding = 2) uniform sampler uMyCustomSampler;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

#include "3d/shaders/common/3d_dm_inplace_post_process.h"

#define MY_OWN_DEFAULT_ENV_PI 3.1415926535897932384626433832795

/*
fragment shader for environment sampling
*/
void main(void)
{
    vec3 color = vec3(0.0);
    CORE_RELAXEDP const float lodLevel = uEnvironmentData.values.y;
    if ((CORE_DEFAULT_ENV_TYPE == CORE_BACKGROUND_TYPE_CUBEMAP) ||
        (CORE_DEFAULT_ENV_TYPE == CORE_BACKGROUND_TYPE_EQUIRECTANGULAR)) {
        // NOTE: would be nicer to calculate in the vertex shader

        // remove translation from view
        const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
        mat4 viewProjInv = uCameras[cameraIdx].viewInv;
        viewProjInv[3] = vec4(0.0, 0.0, 0.0, 1.0);
        viewProjInv = viewProjInv * uCameras[cameraIdx].projInv;

        vec4 farPlane = viewProjInv * vec4(inUv.x, inUv.y, 1.0, 1.0);
        farPlane.xyz = farPlane.xyz / farPlane.w;

        vec4 nearPlane = viewProjInv * vec4(inUv.x, inUv.y, 0.0, 1.0);
        nearPlane.xyz = nearPlane.xyz / nearPlane.w;

        const vec3 worldView = mat3(uEnvironmentData.envRotation) * normalize(farPlane.xyz - nearPlane.xyz);

        if (CORE_DEFAULT_ENV_TYPE == CORE_BACKGROUND_TYPE_CUBEMAP) {
            color = unpackEnvMap(textureLod(samplerCube(uMyCustomCubeTex, uMyCustomSampler), worldView, lodLevel));
        } else {
            const vec2 texCoord = vec2(atan(worldView.z, worldView.x) + MY_OWN_DEFAULT_ENV_PI, acos(worldView.y)) /
                                  vec2(2.0 * MY_OWN_DEFAULT_ENV_PI, MY_OWN_DEFAULT_ENV_PI);
            color = textureLod(sampler2D(uMyCustomTex, uMyCustomSampler), texCoord, lodLevel).xyz;
        }
    } else if (CORE_DEFAULT_ENV_TYPE == CORE_BACKGROUND_TYPE_IMAGE) {
        const vec2 texCoord = (inUv + vec2(1.0)) * 0.5;
        color = unpackEnvMap(textureLod(sampler2D(uMyCustomTex, uMyCustomSampler), texCoord, lodLevel));
    }

    color *= uEnvironmentData.envMapColorFactor.xyz;

    vec2 fragUv;
    CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

    if (CORE_POST_PROCESS_FLAGS > 0) {
        InplacePostProcess(fragUv, outColor);
    }

    // special
    {
        DefaultMaterialUnpackedSceneTimingStruct timing = GetUnpackSceneTiming(uGeneralData);
        const float lerpValue = sin(fract(timing.tickTotalTime) * MY_OWN_DEFAULT_ENV_PI);
        const vec4 newColor = texture(sampler2D(uMyCustomTex, uMyCustomSampler), inUv.xy);
        color = mix(color, newColor.rgb, lerpValue * 0.25);
    }

    outColor = vec4(color.xyz, 1.0);
    outColor.r += 0.5;
}
