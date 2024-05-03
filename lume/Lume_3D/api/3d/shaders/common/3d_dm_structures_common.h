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

#ifndef SHADERS__COMMON__3D_DEFAULT_MATERIAL_STRUCTURES_COMMON_H
#define SHADERS__COMMON__3D_DEFAULT_MATERIAL_STRUCTURES_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_packing_common.h"

#ifdef VULKAN

// aligned to 256 with indcies uvec4
#define CORE_MATERIAL_FACTOR_UNIFORM_VEC4_COUNT 15
#define CORE_MATERIAL_PACKED_UNIFORM_UVEC4_COUNT 15

#define CORE_MATERIAL_DEFAULT_REFLECTANCE 0.04

#define CORE3D_HDR_FLOAT_CLAMP_MAX_VALUE 64512.0

// base color is separated
#define CORE_MATERIAL_TEX_NORMAL_IDX 0
#define CORE_MATERIAL_TEX_MATERIAL_IDX 1
#define CORE_MATERIAL_TEX_EMISSIVE_IDX 2
#define CORE_MATERIAL_TEX_AO_IDX 3
#define CORE_MATERIAL_TEX_CLEARCOAT_IDX 4
#define CORE_MATERIAL_TEX_CLEARCOAT_ROUGHNESS_IDX 5
#define CORE_MATERIAL_TEX_CLEARCOAT_NORMAL_IDX 6
#define CORE_MATERIAL_TEX_SHEEN_IDX 7
#define CORE_MATERIAL_TEX_TRANSMISSION_IDX 8
#define CORE_MATERIAL_TEX_SPECULAR_IDX 9
#define CORE_MATERIAL_SAMPTEX_COUNT 10

#define CORE_MATERIAL_TEXCOORD_INFO_SHIFT 16

#define CORE_MATERIAL_TEXCOORD_INFO_BASE_BIT (1 << 0)
#define CORE_MATERIAL_TEXCOORD_INFO_NORMAL_BIT (1 << 1)
#define CORE_MATERIAL_TEXCOORD_INFO_MATERIAL_BIT (1 << 2)
#define CORE_MATERIAL_TEXCOORD_INFO_EMISSIVE_BIT (1 << 3)
#define CORE_MATERIAL_TEXCOORD_INFO_AO_BIT (1 << 4)
#define CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_BIT (1 << 5)
#define CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_ROUGHNESS_BIT (1 << 6)
#define CORE_MATERIAL_TEXCOORD_INFO_CLEARCOAT_NORMAL_BIT (1 << 7)
#define CORE_MATERIAL_TEXCOORD_INFO_SHEEN_BIT (1 << 8)
#define CORE_MATERIAL_TEXCOORD_INFO_TRANSMISSION_BIT (1 << 9)
#define CORE_MATERIAL_TEXCOORD_INFO_SPECULAR_BIT (1 << 10)

// factor indices
#define CORE_MATERIAL_FACTOR_BASE_IDX 0
#define CORE_MATERIAL_FACTOR_NORMAL_IDX 1
#define CORE_MATERIAL_FACTOR_MATERIAL_IDX 2
#define CORE_MATERIAL_FACTOR_EMISSIVE_IDX 3
#define CORE_MATERIAL_FACTOR_AO_IDX 4
#define CORE_MATERIAL_FACTOR_CLEARCOAT_IDX 5
#define CORE_MATERIAL_FACTOR_CLEARCOAT_ROUGHNESS_IDX 6
#define CORE_MATERIAL_FACTOR_CLEARCOAT_NORMAL_IDX 7
#define CORE_MATERIAL_FACTOR_SHEEN_IDX 8
#define CORE_MATERIAL_FACTOR_TRANSMISSION_IDX 9
#define CORE_MATERIAL_FACTOR_SPECULAR_IDX 10
#define CORE_MATERIAL_FACTOR_ADDITIONAL_IDX 11

// packed texture transforms
#define CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX 0
#define CORE_MATERIAL_PACK_TEX_NORMAL_UV_IDX 1
#define CORE_MATERIAL_PACK_TEX_MATERIAL_UV_IDX 2
#define CORE_MATERIAL_PACK_TEX_EMISSIVE_UV_IDX 3
#define CORE_MATERIAL_PACK_TEX_AO_UV_IDX 4
#define CORE_MATERIAL_PACK_TEX_CLEARCOAT_UV_IDX 5
#define CORE_MATERIAL_PACK_TEX_CLEARCOAT_ROUGHNESS_UV_IDX 6
#define CORE_MATERIAL_PACK_TEX_CLEARCOAT_NORMAL_UV_IDX 7
#define CORE_MATERIAL_PACK_TEX_SHEEN_UV_IDX 8
#define CORE_MATERIAL_PACK_TEX_TRANSMISSION_UV_IDX 9
#define CORE_MATERIAL_PACK_TEX_SPECULAR_UV_IDX 10
#define CORE_MATERIAL_PACK_ADDITIONAL_IDX 11

// needs to match api/core/material/material_desc.h MaterialAlphaMode
#define CORE_MATERIAL_METALLIC_ROUGHNESS 0
#define CORE_MATERIAL_SPECULAR_GLOSSINESS 1
#define CORE_MATERIAL_UNLIT 2
#define CORE_MATERIAL_UNLIT_SHADOW_ALPHA 3
#define CORE_MATERIAL_CUSTOM 4
#define CORE_MATERIAL_CUSTOM_COMPLEX 5

// needs to match LumeEngine/3D/api/3d/render/render_data_defines_3d.h RenderMaterialFlagBits
#define CORE_MATERIAL_SHADOW_RECEIVER_BIT (1 << 0)
#define CORE_MATERIAL_SHADOW_CASTER_BIT (1 << 1)
#define CORE_MATERIAL_NORMAL_MAP_BIT (1 << 2)
#define CORE_MATERIAL_TEXTURE_TRANSFORM_BIT (1 << 3)
#define CORE_MATERIAL_CLEARCOAT_BIT (1 << 4)
#define CORE_MATERIAL_TRANSMISSION_BIT (1 << 5)
#define CORE_MATERIAL_SHEEN_BIT (1 << 6)
#define CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT (1 << 7)
#define CORE_MATERIAL_OPAQUE_BIT (1 << 8)
#define CORE_MATERIAL_SPECULAR_BIT (1 << 9)
#define CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT (1 << 10)
#define CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT (1 << 11)
// basic (1 << 12)
// complex (1 << 13)
#define CORE_MATERIAL_GPU_INSTANCING_BIT (1 << 14)

// needs to match api/core/render/intf_render_data_store_default_light LightingFlagBits
#define CORE_LIGHTING_SHADOW_TYPE_VSM_BIT (1 << 0)
#define CORE_LIGHTING_POINT_ENABLED_BIT (1 << 1)
#define CORE_LIGHTING_SPOT_ENABLED_BIT (1 << 2)

// needs to match LumeEngine/3D/api/3d/render/render_data_defines_3d.h RenderSubmeshFlagBits
#define CORE_SUBMESH_TANGENTS_BIT (1 << 0)
#define CORE_SUBMESH_VERTEX_COLORS_BIT (1 << 1)
#define CORE_SUBMESH_SKIN_BIT (1 << 2)
#define CORE_SUBMESH_SECOND_TEXCOORD_BIT (1 << 3)
#define CORE_SUBMESH_INVERSE_WINDING_BIT (1 << 4) // usually not controlled in shader
#define CORE_SUBMESH_VELOCITY_BIT (1 << 5)        // defines that one should calculate correct velocity

// needs to match render_data_defines.h LightUsageFlagBits
#define CORE_LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT (1 << 0)
#define CORE_LIGHT_USAGE_POINT_LIGHT_BIT (1 << 1)
#define CORE_LIGHT_USAGE_SPOT_LIGHT_BIT (1 << 2)
#define CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT (1 << 3)

// CORE_CAMERA_FLAGS related to render_data_defines_3d.h CameraShaderFlags
#define CORE_CAMERA_FOG_BIT (1 << 0)

// needs to match api/core/render/render_data_defines.h BackgroundType
#define CORE_BACKGROUND_TYPE_IMAGE 1
#define CORE_BACKGROUND_TYPE_CUBEMAP 2
#define CORE_BACKGROUND_TYPE_EQUIRECTANGULAR 3

#define CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT 256
// 16kB min limit
#define CORE_MAX_MESH_MATRIX_UBO_ELEMENT_COUNT 64
#define CORE_MAX_MESH_MATRIX_UBO_USER_DATA_COUNT 2
#define CORE_MAX_MATERIAL_UBO_ELEMENT_COUNT 64

// 16 * sizeof(uvec4) -> 256 (CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT)
#define CORE_MAX_MATERIAL_UBO_USER_DATA_COUNT 16u

#define CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT 256u
#define CORE_DEFAULT_MATERIAL_PREV_JOINT_OFFSET 128u

#define CORE_DEFAULT_MATERIAL_MAX_LIGHT_COUNT 64
#define CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT 16
#define CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT 8

#define CORE_DEFAULT_MATERIAL_MAX_SH_VEC3_VALUE_COUNT 9

#define CORE_DEFAULT_MATERIAL_TEXTURE_TRANSFORM_COUNT 5
#define CORE_DEFAULT_MATERIAL_USER_DATA_VEC4_COUNT 8

#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_LEFT 0
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_RIGHT 1
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_BOTTOM 2
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_TOP 3
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_NEAR 4
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_FAR 5
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT 6

#else

// aligned for 256 with indices uvec4 bytes
constexpr uint32_t CORE_MATERIAL_FACTOR_UNIFORM_VEC4_COUNT { 15u };
constexpr uint32_t CORE_MATERIAL_PACKED_UNIFORM_UVEC4_COUNT { 15u };

// factor indices
constexpr uint32_t CORE_MATERIAL_FACTOR_BASE_IDX { 0u };
constexpr uint32_t CORE_MATERIAL_FACTOR_NORMAL_IDX { 1 };
constexpr uint32_t CORE_MATERIAL_FACTOR_MATERIAL_IDX { 2 };
constexpr uint32_t CORE_MATERIAL_FACTOR_EMISSIVE_IDX { 3 };
constexpr uint32_t CORE_MATERIAL_FACTOR_AO_IDX { 4 };
constexpr uint32_t CORE_MATERIAL_FACTOR_CLEARCOAT_IDX { 5 };
constexpr uint32_t CORE_MATERIAL_FACTOR_CLEARCOAT_ROUGHNESS_IDX { 6 };
constexpr uint32_t CORE_MATERIAL_FACTOR_CLEARCOAT_NORMAL_IDX { 7 };
constexpr uint32_t CORE_MATERIAL_FACTOR_SHEEN_IDX { 8 };
constexpr uint32_t CORE_MATERIAL_FACTOR_TRANSMISSION_IDX { 9 };
constexpr uint32_t CORE_MATERIAL_FACTOR_SPECULAR_IDX { 10 };

// packed texture transforms
constexpr uint32_t CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX { 0u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_NORMAL_UV_IDX { 1u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_MATERIAL_UV_IDX { 2u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_EMISSIVE_UV_IDX { 3u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_AO_UV_IDX { 4u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_CLEARCOAT_UV_IDX { 5u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_CLEARCOAT_ROUGHNESS_UV_IDX { 6u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_CLEARCOAT_NORMAL_UV_IDX { 7u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_SHEEN_UV_IDX { 8u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_TRANSMISSION_UV_IDX { 9u };
constexpr uint32_t CORE_MATERIAL_PACK_TEX_SPECULAR_UV_IDX { 10u };
constexpr uint32_t CORE_MATERIAL_PACK_ADDITIONAL_IDX { 11u };

// matches PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE
constexpr uint32_t CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT { 256u };
constexpr uint32_t CORE_UNIFORM_BUFFER_MAX_BIND_SIZE { 1024u * 16u };
// 16kB min limit
constexpr uint32_t CORE_MAX_MESH_MATRIX_UBO_ELEMENT_COUNT { 64u };
constexpr uint32_t CORE_MAX_MESH_MATRIX_UBO_USER_DATA_COUNT { 2u };
constexpr uint32_t CORE_MAX_MATERIAL_UBO_ELEMENT_COUNT { 64u };

// 16 * sizeof(uvec4) -> 256 (CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT)
constexpr uint32_t CORE_MAX_MATERIAL_UBO_USER_DATA_COUNT { 16u };

constexpr uint32_t CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT { 256u };
constexpr uint32_t CORE_DEFAULT_MATERIAL_PREV_JOINT_OFFSET { 128u };

constexpr uint32_t CORE_DEFAULT_MATERIAL_MAX_LIGHT_COUNT { 64u };
constexpr uint32_t CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT { 16u };
constexpr uint32_t CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT { 8u };

constexpr uint32_t CORE_DEFAULT_MATERIAL_MAX_SH_VEC3_VALUE_COUNT { 9u };

constexpr uint32_t CORE_DEFAULT_MATERIAL_TEXTURE_TRANSFORM_COUNT { 5u };
constexpr uint32_t CORE_DEFAULT_MATERIAL_USER_DATA_VEC4_COUNT { 8u };

constexpr uint32_t CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_LEFT { 0u };
constexpr uint32_t CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_RIGHT { 1 };
constexpr uint32_t CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_BOTTOM { 2 };
constexpr uint32_t CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_TOP { 3 };
constexpr uint32_t CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_NEAR { 4 };
constexpr uint32_t CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_FAR { 5 };
constexpr uint32_t CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT { 6 };

#endif

struct DefaultMaterialSingleMeshStruct {
    // currently using offsets in binding CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT

    mat4 world;
    mat4 normalWorld; // NOTE: not implemented yet
    mat4 prevWorld;

    // .xy render mesh id, .zw mesh id (64 bit id, with 32 bit info usually meaningfull)
    uvec4 indices;
    // layers, .xy layer mask
    uvec4 layers;
    // custom data
    uvec4 customData[CORE_MAX_MESH_MATRIX_UBO_USER_DATA_COUNT];
};
// by default GPU instancing enablers are active
struct DefaultMaterialMeshStruct {
    DefaultMaterialSingleMeshStruct mesh[CORE_MAX_MESH_MATRIX_UBO_ELEMENT_COUNT];
};

struct DefaultMaterialSingleMaterialStruct {
    vec4 factors[CORE_MATERIAL_FACTOR_UNIFORM_VEC4_COUNT];
    uvec4 indices;
};
// by default GPU instancing enablers are active
struct DefaultMaterialMaterialStruct {
    DefaultMaterialSingleMaterialStruct material[CORE_MAX_MATERIAL_UBO_ELEMENT_COUNT];
};

struct DefaultMaterialTransformSingleMaterialStruct {
    uvec4 packed[CORE_MATERIAL_FACTOR_UNIFORM_VEC4_COUNT];
    uvec4 indices;
};
// by default GPU instancing enablers are active
struct DefaultMaterialTransformMaterialStruct {
    DefaultMaterialTransformSingleMaterialStruct material[CORE_MAX_MATERIAL_UBO_ELEMENT_COUNT];
};

struct DefaultMaterialSingleUserMaterialStruct {
    uvec4 userData[CORE_MAX_MATERIAL_UBO_USER_DATA_COUNT];
};
// by default GPU instancing enablers are active
struct DefaultMaterialUserMaterialStruct {
    DefaultMaterialSingleUserMaterialStruct material[CORE_MAX_MATERIAL_UBO_ELEMENT_COUNT];
};

struct DefaultMaterialSingleLightStruct {
    vec4 pos;
    // .w is range
    vec4 dir;
    // intensity baked into rgb, .w is intensity (if needs to be inverse multiplied)
    vec4 color;
    // .x is inner cone angle, .y is outer cone angle, .z is light angle scale, .w is light angle offset
    vec4 spotLightParams;
    // .x = shadow strength, .y = depth bias, .z = normal bias, .w = shadow uv step size
    vec4 shadowFactors;
    // additional factors for customization
    vec4 additionalFactor;

    // .x is light flags, .y is shadow camera idx, .z is shadow idx in atlas, .w is shadow count
    uvec4 flags;
    // .xy = unique id (64-bit), .zw = layer mask (64 bit)
    uvec4 indices;
};

struct DefaultMaterialLightStruct {
    uint directionalLightBeginIndex;
    uint directionalLightCount;

    uint pointLightBeginIndex;
    uint pointLightCount;

    uint spotLightBeginIndex;
    uint spotLightCount;

    uint pad0;
    uint pad1;

    uvec4 clusterSizes;
    vec4 clusterFactors;

    vec4 atlasSizeInvSize;
    vec4 additionalFactors;

    DefaultMaterialSingleLightStruct lights[CORE_DEFAULT_MATERIAL_MAX_LIGHT_COUNT];
};

struct DefaultMaterialSkinStruct {
    // previous frame matrices in offset CORE_DEFAULT_MATERIAL_PREV_JOINT_OFFSET
    mat4 jointMatrices[CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT];
};

// can change per camera
struct DefaultMaterialGeneralDataStruct {
    // .x is render camera index
    uvec4 indices;
    // .xy is viewport size 0 - size, .zw is inv viewport size
    vec4 viewportSizeInvViewportSize;
    // .x = scene delta time (ms), .y = tick delta time (ms), .z = tick total time (s), .w = frame index (asuint)
    vec4 sceneTimingData;
};

// can change per camera
struct DefaultMaterialEnvironmentStruct {
    // .rgb is tint * intensity (intensity baked into rgb), .a is intensity
    vec4 indirectSpecularColorFactor;
    // .rgb = tint * intensity (intensity baked into rgb), .a is intensity
    vec4 indirectDiffuseColorFactor;
    // .rgb = tint * intensity (intensity baked into rgb), .a is intensity
    vec4 envMapColorFactor;
    // .r = radiance cubemap mip count, .g = specular cubemap lod level
    vec4 values;
    // blender factor for multi-env case
    vec4 blendFactor;

    // Environment rotation matrix.
    mat4 envRotation;

    // .xy = unique id (64-bit), .zw = layer mask (64 bit)
    uvec4 indices;

    // spherical harmonics for indirect diffuse environment lighting (3 bands)
    vec4 shIndirectCoefficients[CORE_DEFAULT_MATERIAL_MAX_SH_VEC3_VALUE_COUNT];

    // padding
    vec4 pad0;
};

struct DefaultMaterialFogStruct {
    // .xy = unique id (64 bit), .zw = layer mask (64 bit)
    uvec4 indices;

    // .x = density, .y = heightFalloff, .z = heightFogOffset
    vec4 firstLayer;
    // .x = density, .y = heightFalloff, .z = heightFogOffset
    vec4 secondLayer;

    // .x = startDistance, .y = cutoffDistance, .z = maxOpacity
    vec4 baseFactors;
    // primary color for the fog (.rgb = tint, .a = intensity)
    vec4 inscatteringColor;
    // env map color factor (.rgb = tint, .a = intensity)
    vec4 envMapFactor;

    // Additional custom factor
    vec4 additionalFactor;
};

// camera data
struct DefaultCameraMatrixStruct {
    mat4 view;
    mat4 proj;
    mat4 viewProj;

    mat4 viewInv;
    mat4 projInv;
    mat4 viewProjInv;

    mat4 viewPrevFrame;
    mat4 projPrevFrame;
    mat4 viewProjPrevFrame;

    mat4 shadowViewProj;
    mat4 shadowViewProjInv;

    // .xy = jitter offset, .zw = jitter offset with baked screen size
    vec4 jitter;
    vec4 jitterPrevFrame;

    // .xy = unique id (64-bit), .zw = layer mask (64 bit)
    uvec4 indices;
    // .x multi-view camera additional layer count, .yzw 3 multi-view camera indices
    uvec4 multiViewIndices;

    vec4 frustumPlanes[CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT];

    // .x environment count
    uvec4 counts;
    // padding to 256
    uvec4 pad0;
    mat4 matPad0;
    mat4 matPad1;
};

#ifdef VULKAN

// General data unpacking

struct DefaultMaterialUnpackedSceneTimingStruct {
    // scene delta time (ms)
    float sceneDeltaTime;
    // tick delta time (ms)
    float tickDeltaTime;
    // tick total time (s)
    float tickTotalTime;
    // frame index of the current ECS scene render system
    uint frameIndex;
};

struct DefaultMaterialUnpackedPostProcessStruct {
    float tonemapExposure;
    float vignetteCoeff;
    float vignettePower;
};

uint GetPackFlatIndices(const uint cameraIdx, const uint instanceIdx)
{
    return ((instanceIdx << 16) | (cameraIdx & 0xffff)); // 16 : bit size
}

void GetUnpackFlatIndices(in uint indices, out uint cameraIdx, out uint instanceIdx)
{
    cameraIdx = indices & 0xffff;
    instanceIdx = indices >> 16;
}

uint GetUnpackFlatIndicesInstanceIdx(in uint indices)
{
    return (indices >> 16);
}

uint GetUnpackFlatIndicesCameraIdx(in uint indices)
{
    return (indices & 0xffff);
}

uint GetUnpackCameraIndex(const DefaultMaterialGeneralDataStruct dmgds)
{
    return dmgds.indices.x;
}

DefaultMaterialUnpackedSceneTimingStruct GetUnpackSceneTiming(const DefaultMaterialGeneralDataStruct dmgds)
{
    DefaultMaterialUnpackedSceneTimingStruct dm;
    dm.sceneDeltaTime = dmgds.sceneTimingData.x;
    dm.tickDeltaTime = dmgds.sceneTimingData.y;
    dm.tickTotalTime = dmgds.sceneTimingData.z;
    dm.frameIndex = floatBitsToUint(dmgds.sceneTimingData.w);
    return dm;
}

vec4 GetUnpackViewport(const DefaultMaterialGeneralDataStruct dmgds)
{
    return dmgds.viewportSizeInvViewportSize;
}

// Default material unpacking

struct DefaultMaterialUnpackedTexTransformStruct {
    vec4 rotateScale;
    vec2 translate;
};

DefaultMaterialUnpackedTexTransformStruct GetUnpackTextureTransform(const uvec4 packedTexTransform)
{
    DefaultMaterialUnpackedTexTransformStruct dm;
    dm.rotateScale = UnpackVec4Half2x16(packedTexTransform.xy);
    dm.translate = UnpackVec4Half2x16(packedTexTransform.zw).xy;
    return dm;
}

// DEPRECATED base methods with struct inputs
#ifdef CORE3D_USE_DEPRECATED_UNPACK_METHODS

vec4 GetUnpackBaseColor(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_BASE_IDX];
}
vec4 GetUnpackBaseColor(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_BASE_IDX];
}

vec4 GetUnpackMaterial(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_MATERIAL_IDX];
}
vec4 GetUnpackMaterial(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_MATERIAL_IDX];
}

float GetUnpackAO(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_AO_IDX].x;
}
float GetUnpackAO(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_AO_IDX].x;
}

float GetUnpackClearcoat(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_IDX].x;
}
float GetUnpackClearcoat(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_IDX].x;
}

// NOTE: sampling from .y
float GetUnpackClearcoatRoughness(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_ROUGHNESS_IDX].y;
}
float GetUnpackClearcoatRoughness(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_ROUGHNESS_IDX].y;
}

// .xyz = sheen factor, .w = sheen roughness
vec4 GetUnpackSheen(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_SHEEN_IDX];
}
vec4 GetUnpackSheen(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_SHEEN_IDX];
}

float GetUnpackTransmission(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_TRANSMISSION_IDX].x;
}
float GetUnpackTransmission(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_TRANSMISSION_IDX].x;
}

// .xyz = specular color, .w = specular strength
vec4 GetUnpackSpecular(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_SPECULAR_IDX];
}
vec4 GetUnpackSpecular(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_SPECULAR_IDX];
}

vec3 GetUnpackEmissiveColor(const DefaultMaterialMaterialStruct dmms)
{
    const vec4 emissive = dmms.material[0].factors[CORE_MATERIAL_FACTOR_EMISSIVE_IDX];
    return emissive.rgb * emissive.a;
}
vec3 GetUnpackEmissiveColor(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    const vec4 emissive = dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_EMISSIVE_IDX];
    return emissive.rgb * emissive.a;
}

float GetUnpackNormalScale(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_NORMAL_IDX].x;
}
float GetUnpackNormalScale(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_NORMAL_IDX].x;
}

float GetUnpackAlphaCutoff(const DefaultMaterialMaterialStruct dmms)
{
    return dmms.material[0].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].x;
}
float GetUnpackAlphaCutoff(const DefaultMaterialMaterialStruct dmms, const uint instanceIdx)
{
    return dmms.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].x;
}

vec4 GetUnpackMaterialTextureInfoSlotFactor(const DefaultMaterialMaterialStruct dmms, const uint materialIndexSlot)
{
    const uint maxIndex = min(materialIndexSlot, CORE_MATERIAL_FACTOR_UNIFORM_VEC4_COUNT - 1);
    return dmms.material[0].factors[maxIndex].xyzw;
}
vec4 GetUnpackMaterialTextureInfoSlotFactor(
    const DefaultMaterialMaterialStruct dmms, const uint materialIndexSlot, const uint instanceIdx)
{
    const uint maxIndex = min(materialIndexSlot, CORE_MATERIAL_FACTOR_UNIFORM_VEC4_COUNT - 1);
    return dmms.material[instanceIdx].factors[maxIndex].xyzw;
}

// transform = high bits (16)
// uv set bit = low bits (0)
uint GetUnpackTexCoordInfo(const DefaultMaterialMaterialStruct dmms)
{
    return floatBitsToUint(dmms.material[0].factors[CORE_MATERIAL_FACTOR_ADDITIONAL_IDX].y);
}

// transform = high bits (16)
// uv set bit = low bits (0)
uint GetUnpackTexCoordInfo(const DefaultMaterialTransformMaterialStruct dmms)
{
    return dmms.material[0].packed[CORE_MATERIAL_PACK_ADDITIONAL_IDX].y;
}

#endif // CORE3D_USE_DEPRECATED_UNPACK_METHODS

#endif

#endif // SHADERS__COMMON__3D_DEFAULT_MATERIAL_STRUCTURES_COMMON_H
