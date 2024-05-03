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

#ifndef API_3D_RENDER_DEFAULT_MATERIAL_CONSTANTS_H
#define API_3D_RENDER_DEFAULT_MATERIAL_CONSTANTS_H

#include <cstdint>

#include <3d/namespace.h>
#include <3d/shaders/common/3d_dm_structures_common.h>

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_render_defaultmaterialconstants
 *  @{
 */
/** Render node created resources will add the name of the render graph to the name presented here.
 */
/** Default material lighting constants
 * Scene constants -> prefix with unique scene render data store name when using
 * Default render data store for scene can be found:
 * renderNodeGraphData.renderNodeGraphDataStoreName.empty() ?
        "RenderDataStoreDefaultScene" :
        renderNodeGraphData.renderNodeGraphDataStoreName;
 */
struct DefaultMaterialLightingConstants {
    /** Max directional light count */
    static constexpr uint32_t MAX_LIGHT_COUNT { 64 };
    /** Max shadow count */
    static constexpr uint32_t MAX_SHADOW_COUNT { 8u };

    /** Light data buffer name. Default material pipelines uses per camera ligh buffers */
    static constexpr const BASE_NS::string_view LIGHT_DATA_BUFFER_NAME { "CORE3D_DM_LIGHT_DATA_BUFFER" };
    /** Light cluster data buffer name. Default material pipelines uses per camera ligh buffers */
    static constexpr const BASE_NS::string_view LIGHT_CLUSTER_DATA_BUFFER_NAME {
        "CORE3D_DM_LIGHT_CLUSTER_DATA_BUFFER"
    };

    /** Shadow atlas depth buffer name */
    static constexpr const BASE_NS::string_view SHADOW_DEPTH_BUFFER_NAME { "CORE3D_DM_SHADOW_DEPTH_BUFFER" };
    /** VSM shadow atlas color buffer name (VSM) */
    static constexpr const BASE_NS::string_view SHADOW_VSM_COLOR_BUFFER_NAME { "CORE3D_DM_VSM_SHADOW_COLOR_BUFFER" };
};

/** Default material constants
 * Scene constants -> prefix with unique scene render data store name when using
 */
struct DefaultMaterialMaterialConstants {
    /** Material buffer name */
    static constexpr const BASE_NS::string_view MATERIAL_DATA_BUFFER_NAME { "CORE3D_DM_DATA_BUFFER" };
    /** Material transform buffer name */
    static constexpr const BASE_NS::string_view MATERIAL_TRANSFORM_DATA_BUFFER_NAME {
        "CORE3D_DM_TRANSFORM_DATA_BUFFER"
    };
    /** Material user data buffer name */
    static constexpr const BASE_NS::string_view MATERIAL_USER_DATA_BUFFER_NAME { "CORE3D_DM_USER_DATA_BUFFER" };
    /** Mesh buffer name */
    static constexpr const BASE_NS::string_view MESH_DATA_BUFFER_NAME { "CORE3D_DM_MESH_DATA_BUFFER" };
    /** Skin buffer name */
    static constexpr const BASE_NS::string_view SKIN_DATA_BUFFER_NAME { "CORE3D_DM_SKIN_DATA_BUFFER" };
};

/** Default material shader related constants. (Must be matched in relevant json and c-code files) */
struct DefaultMaterialShaderConstants {
    /** Default opaque render slots. Forward opaque. */
    static constexpr const BASE_NS::string_view RENDER_SLOT_FORWARD_OPAQUE { "CORE3D_RS_DM_FW_OPAQUE" };
    /** Default translucent render slot. Forward translucent. */
    static constexpr const BASE_NS::string_view RENDER_SLOT_FORWARD_TRANSLUCENT { "CORE3D_RS_DM_FW_TRANSLUCENT" };
    /** Default deferred opaque render slot */
    static constexpr const BASE_NS::string_view RENDER_SLOT_DEFERRED_OPAQUE { "CORE3D_RS_DM_DF_OPAQUE" };
    /** Default render slot for depth-only rendering. Handles PCF shadows and states */
    static constexpr const BASE_NS::string_view RENDER_SLOT_DEPTH { "CORE3D_RS_DM_DEPTH" };
    /** VSM shadow render slot with color buffer target. Handles VSM shadows and states */
    static constexpr const BASE_NS::string_view RENDER_SLOT_DEPTH_VSM { "CORE3D_RS_DM_DEPTH_VSM" };

    /** Default forward vertex input declaration */
    static constexpr const BASE_NS::string_view VERTEX_INPUT_DECLARATION_FORWARD {
        "3dvertexinputdeclarations://core3d_dm_fw.shadervid"
    };
    /** Default forward pipeline layout */
    static constexpr const BASE_NS::string_view PIPELINE_LAYOUT_FORWARD { "3dpipelinelayouts://core3d_dm_fw.shaderpl" };

    /** Default depth vertex input declaration */
    static constexpr const BASE_NS::string_view VERTEX_INPUT_DECLARATION_DEPTH {
        "3dvertexinputdeclarations://core3d_dm_depth.shadervid"
    };
    /** Default depth pipeline layout */
    static constexpr const BASE_NS::string_view PIPELINE_LAYOUT_DEPTH {
        "3dpipelinelayouts://core3d_dm_depth.shaderpl"
    };

    /** Default environment pipeline layout */
    static constexpr const BASE_NS::string_view PIPELINE_LAYOUT_ENV { "3dpipelinelayouts://core3d_dm_env.shaderpl" };

    /** Default material forward shader with automatic render slot and graphics state setup */
    static constexpr const BASE_NS::string_view OPAQUE_SHADER_NAME { "3dshaders://shader/core3d_dm_fw.shader" };
    /** Default material forward shader graphics state with automatic render slot and graphics state setup */
    static constexpr const BASE_NS::string_view SHADER_STATE_NAME { "3dshaderstates://core3d_dm.shadergs" };
    /** Default material depth shader */
    static constexpr const BASE_NS::string_view DEPTH_SHADER_NAME { "3dshaders://shader/core3d_dm_depth.shader" };
    /** Default material forward env shader */
    static constexpr const BASE_NS::string_view ENV_SHADER_NAME {
        "3dshaders://shader/core3d_dm_fullscreen_env.shader"
    };
};

/** Default camera constants
 * Scene constants -> prefix with unique scene render data store name when using
 */
struct DefaultMaterialCameraConstants {
    /** Max camera count. This includes scene and shadow cameras.  */
    static constexpr const uint32_t MAX_CAMERA_COUNT { CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT };
    /** Max environment count.  */
    static constexpr const uint32_t MAX_ENVIRONMENT_COUNT { CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT };

    /** Camera data buffer name (this is usable in the whole pipeline / scene) */
    static constexpr const BASE_NS::string_view CAMERA_DATA_BUFFER_NAME { "CORE3D_DM_CAM_DATA_BUF" };
    /** Camera prefix for per camera data buffers */
    static constexpr const BASE_NS::string_view CAMERA_GENERAL_BUFFER_PREFIX_NAME { "CORE3D_DM_CAM_GENERAL_BUF_" };
    /** Camera prefix for per camera data buffers */
    static constexpr const BASE_NS::string_view CAMERA_ENVIRONMENT_BUFFER_PREFIX_NAME { "CORE3D_DM_CAM_ENV_BUF_" };
    /** Camera prefix for per camera data buffers */
    static constexpr const BASE_NS::string_view CAMERA_FOG_BUFFER_PREFIX_NAME { "CORE3D_DM_CAM_FOG_BUF_" };
    /** Camera prefix for per camera data buffers */
    static constexpr const BASE_NS::string_view CAMERA_POST_PROCESS_BUFFER_PREFIX_NAME {
        "CORE3D_DM_CAM_POST_PROCESS_BUF_"
    };
    /** Camera prefix for per light buffers */
    static constexpr const BASE_NS::string_view CAMERA_LIGHT_BUFFER_PREFIX_NAME { "CORE3D_DM_CAM_LIGHT_BUF_" };
    /** Camera prefix for per light cluster buffers */
    static constexpr const BASE_NS::string_view CAMERA_LIGHT_CLUSTER_BUFFER_PREFIX_NAME {
        "CORE3D_DM_CAM_LIGHT_CLUSTER_BUF_"
    };

    /** Camera color buffer prefix (often combined with scene name + prefix name + to_hex(camera id))
     * For example: RenderDataStoreDefaultSceneCORE3D_CAMERA_DEPTH_40000000D
     */
    static constexpr const BASE_NS::string_view CAMERA_COLOR_PREFIX_NAME { "CORE3D_CAMERA_COLOR_" };
    /** Camera depth buffer prefix  (often combined with scene name + to_hex(camera id)) */
    static constexpr const BASE_NS::string_view CAMERA_DEPTH_PREFIX_NAME { "CORE3D_CAMERA_DEPTH_" };

    /** Default camera post process name (often combined with default name + post process id) */
    static constexpr const BASE_NS::string_view CAMERA_POST_PROCESS_PREFIX_NAME { "CORE3D_POST_PROCESS_CAM" };
    /** Default pre-pass camera post process name (often combined with default name + post process id) */
    static constexpr const BASE_NS::string_view CAMERA_PRE_PASS_POST_PROCESS_PREFIX_NAME {
        "CORE3D_POST_PROCESS_PRE_PASS_CAM"
    };
    /** Default reflection camera post process name (often combined with default name + post process id) */
    static constexpr const BASE_NS::string_view CAMERA_REFLECTION_POST_PROCESS_PREFIX_NAME {
        "CORE3D_POST_PROCESS_REFLECTION_CAM"
    };

    /** Default mip count for reflection plane */
    static constexpr const uint32_t REFLECTION_PLANE_MIP_COUNT { 6u };
};

/** Default material GPU resource constants */
struct DefaultMaterialGpuResourceConstants {
    /** Default material base color, white srgb */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_BASE_COLOR { "CORE_DEFAULT_GPU_IMAGE_WHITE" };
    /** Default material emissive, white srgb */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_EMISSIVE { "CORE_DEFAULT_GPU_IMAGE_WHITE" };
    /** Default material normal */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_NORMAL { "CORE3D_DM_NORMAL" };
    /** Default material material map, white linear */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_MATERIAL { "CORE_DEFAULT_GPU_IMAGE_WHITE" };
    /** Default material ambient occlusion, white */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_AO { "CORE3D_DM_AO" };

    /** Default material clearcoat intensity, white */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_CLEARCOAT { "CORE_DEFAULT_GPU_IMAGE_WHITE" };
    /** Default material clearcoat roughness, white */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_CLEARCOAT_ROUGHNESS {
        "CORE_DEFAULT_GPU_IMAGE_WHITE"
    };
    /** Default material clearcoat normal */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_CLEARCOAT_NORMAL { "CORE3D_DM_NORMAL" };

    /** Default material clearcoat intensity, white */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_SHEEN { "CORE_DEFAULT_GPU_IMAGE_WHITE" };

    /** Default material specular, white srgb */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_SPECULAR { "CORE_DEFAULT_GPU_IMAGE_WHITE" };

    /** Default material transmission, white */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_MATERIAL_TRANSMISSION { "CORE3D_DM_AO" };

    /** Default material radiance cubemap */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_RADIANCE_CUBEMAP { "CORE3D_DM_RADIANCE_CUBEMAP" };

    /** Default skybox */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SKYBOX_CUBEMAP { "CORE3D_DM_SKYBOX_CUBEMAP" };

    /** Default radiance cubemap sampler */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER {
        "CORE3D_DM_RADIANCE_CUBEMAP_SAMPLER"
    };
    /** Default VSM shadow sampler */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_VSM_SHADOW_SAMPLER { "CORE3D_DM_VSM_SHADOW_SAMPLER" };
    /** Default PCF shadow sampler */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_PCF_SHADOW_SAMPLER { "CORE3D_DM_PCF_SHADOW_SAMPLER" };
};

/** Default material render node share constants */
struct DefaultMaterialRenderNodeConstants {
    /** Render node default camera controller output name for depth */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_OUTPUT { "output" };

    /** Render node default camera controller output name for depth */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_DEPTH { "depth" };
    /** Render node default camera controller output name for color */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_COLOR { "color" };
    /** Render node default camera controller output name for depth msaa */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_DEPTH_MSAA { "depth_msaa" };
    /** Render node default camera controller output name for color msaa */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_COLOR_MSAA { "color_msaa" };
    /** Render node default camera controller output name for history */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_HISTORY { "history" };
    /** Render node default camera controller output name for history to be outputted for next frame */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_HISTORY_NEXT { "history_next" };

    /** Render node default camera controller output name for deferred packed velocity and normal */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_VELOCITY_NORMAL { "velocity_normal" };
    /** Render node default camera controller output name for deferred base color */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_BASE_COLOR { "base_color" };
    /** Render node default camera controller output name for deferred material */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_MATERIAL { "material" };
    /** Render node default camera controller output name for camera dynamic radiance cubemap */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_RADIANCE_CUBEMAP { "radiance_cubemap" };

    /** Render node default camera controller output name for deferred custom buffers
     * Is a prefix for the final name.
     * With deferred targets the name is deferred_0 and so on.
     */
    static constexpr const BASE_NS::string_view CORE_DM_CAMERA_DEFERRED_PREFIX { "deferred_" };
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_DEFAULT_MATERIAL_CONSTANTS_H
