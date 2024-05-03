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

#ifndef API_3D_RENDER_RENDER_DATA_DEFINES_H
#define API_3D_RENDER_RENDER_DATA_DEFINES_H

#include <cstdint>

#include <3d/ecs/components/mesh_component.h>
#include <3d/render/default_material_constants.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/namespace.h>
#include <render/render_data_structures.h>

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_render_renderdatadefines
 *  @{
 */
/** Render data constants */
namespace RenderSceneDataConstants {
/** Max morph target count */
static constexpr uint32_t MAX_MORPH_TARGET_COUNT { 64u };

/** Mesh indices in index */
static constexpr uint32_t MESH_INDEX_INDEX { 5u };
/** Mesh weights in index */
static constexpr uint32_t MESH_WEIGHT_INDEX { 6u };

/** Max vec3 count for 3 bands */
static constexpr uint32_t MAX_SH_VEC3_COUNT { 9u };

/** Max camera target buffer count */
static constexpr uint32_t MAX_CAMERA_COLOR_TARGET_COUNT { 8u };

/** Max custom push constant data size */
static constexpr uint32_t MAX_CUSTOM_PUSH_CONSTANT_DATA_SIZE {
    RENDER_NS::PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE
};

/** Max default material env custom resources */
static constexpr uint32_t MAX_ENV_CUSTOM_RESOURCE_COUNT { 4u };

/** Additional custom data size which is bind with render mesh structure to shader
 * Should match api/shaders/common/3d_dm_structures_common.h DefaultMaterialSingleMeshStruct userData
 */
static constexpr uint32_t MESH_CUSTOM_DATA_VEC4_COUNT { 2u };

/** Max multi-view layer camera count. Max layers is 4 -> additional cameras 3 */
static constexpr uint32_t MAX_MULTI_VIEW_LAYER_CAMERA_COUNT { 3u };

/** Invalid index with default material indices */
static constexpr uint32_t INVALID_INDEX { ~0u };

/** Invalid 64 bit id */
static constexpr uint64_t INVALID_ID { 0xFFFFFFFFffffffff };

/** Default render sort layer id */
static constexpr uint8_t DEFAULT_RENDER_SORT_LAYER_ID { 32u };

/** Default layer mask */
static constexpr uint64_t DEFAULT_LAYER_MASK { 0x1 };
} // namespace RenderSceneDataConstants

/** Render draw command */
struct RenderDrawCommand {
    /** Vertex count */
    uint32_t vertexCount { 0 };
    /** Index count */
    uint32_t indexCount { 0 };
    /** Instance count */
    uint32_t instanceCount { 1 };
};

/** Render vertex buffer */
struct RenderVertexBuffer {
    /** Buffer handle */
    RENDER_NS::RenderHandleReference bufferHandle {};
    /** Buffer offset */
    uint32_t bufferOffset { 0 };
    /** Byte size */
    uint32_t byteSize { RENDER_NS::PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
};

/** Render index buffer */
struct RenderIndexBuffer {
    /** Buffer handle */
    RENDER_NS::RenderHandleReference bufferHandle {};
    /** Buffer offset */
    uint32_t bufferOffset { 0 };
    /** Byte size */
    uint32_t byteSize { 0 };
    /** Index type */
    RENDER_NS::IndexType indexType { RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT32 };
};

/** Convert RenderVertexBuffer to Renderer VertexBuffer */
inline RENDER_NS::VertexBuffer ConvertVertexBuffer(const RenderVertexBuffer& rvb)
{
    return RENDER_NS::VertexBuffer { rvb.bufferHandle.GetHandle(), rvb.bufferOffset, rvb.byteSize };
}

/** Convert RenderIndexBuffer to Renderer IndexBuffer */
inline RENDER_NS::IndexBuffer ConvertIndexBuffer(const RenderIndexBuffer& rib)
{
    return RENDER_NS::IndexBuffer { rib.bufferHandle.GetHandle(), rib.bufferOffset, rib.byteSize, rib.indexType };
}

/** Render mesh data
 * In default material pipeline created by:
 * RenderMeshComponent creates RenderMeshData for every mesh.
 */
struct RenderMeshData {
    /** Regular world matrix. */
    BASE_NS::Math::Mat4X4 world;
    /** Normal world matrix. */
    BASE_NS::Math::Mat4X4 normalWorld;
    /** Regular previous frame world matrix. */
    BASE_NS::Math::Mat4X4 prevWorld;

    /** 64 bit id for render instance. Can be used for rendering time identification. RenderMeshComponent entity. */
    uint64_t id { RenderSceneDataConstants::INVALID_ID };
    /** 64 bit id for mesh instance. MeshComponent entity. */
    uint64_t meshId { RenderSceneDataConstants::INVALID_ID };

    /** layer mask. */
    uint64_t layerMask { 1 };
    /** Unused offset. */
    uint64_t additional { 0 };

    /** Custom data. */
    BASE_NS::Math::UVec4 customData[RenderSceneDataConstants::MESH_CUSTOM_DATA_VEC4_COUNT] {};
};

/** The rendering material specialization flag bits
 */
enum RenderMaterialFlagBits : uint32_t {
    /** Defines whether this material receives shadow */
    RENDER_MATERIAL_SHADOW_RECEIVER_BIT = (1 << 0),
    /** Defines whether this material is a shadow caster */
    RENDER_MATERIAL_SHADOW_CASTER_BIT = (1 << 1),
    /** Defines whether this material has a normal map enabled */
    RENDER_MATERIAL_NORMAL_MAP_BIT = (1 << 2),
    /** Defines whether this material as one or many texture transforms */
    RENDER_MATERIAL_TEXTURE_TRANSFORM_BIT = (1 << 3),
    /** Defines whether to use clear-coat parameters to simulate multi-layer material */
    RENDER_MATERIAL_CLEAR_COAT_BIT = (1 << 4),
    /** Defines whether to use transmission parameters to simulate optically transparent materials. */
    RENDER_MATERIAL_TRANSMISSION_BIT = (1 << 5),
    /** Defines whether to use sheen parameters to simulate e.g. cloth and fabric materials. */
    RENDER_MATERIAL_SHEEN_BIT = (1 << 6),
    /** Defines and additional shader discard bit. e.g. alpha mask discard */
    RENDER_MATERIAL_SHADER_DISCARD_BIT = (1 << 7),
    /** Defines if object is opaque and alpha is ignored */
    RENDER_MATERIAL_OPAQUE_BIT = (1 << 8),
    /** Defines whether to use specular color and strength parameters. */
    RENDER_MATERIAL_SPECULAR_BIT = (1 << 9),
    /** Defines whether this material will receive light from punctual lights (points, spots, directional) */
    RENDER_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT = (1 << 10),
    /** Defines whether this material will receive indirect light from SH and cubemaps */
    RENDER_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT = (1 << 11),
    /** Defines if this material is "basic" in default material pipeline
     * NOTE: used in render node graph to discard materials
     */
    RENDER_MATERIAL_BASIC_BIT = (1 << 12),
    /** Defines if this material is "complex" in default material pipeline
     * NOTE: used in render node graph to discard materials
     */
    RENDER_MATERIAL_COMPLEX_BIT = (1 << 13),
    /** Defines whether this material uses GPU instancing and needs dynamic UBO indices.
     * Spesializes the shader, and therefore needs to be setup
     */
    RENDER_MATERIAL_GPU_INSTANCING_BIT = (1 << 14),
};
/** Container for material flag bits */
using RenderMaterialFlags = uint32_t;

/** Render material type enumeration */
enum class RenderMaterialType : uint8_t {
    /** Enumeration for Metallic roughness workflow */
    METALLIC_ROUGHNESS = 0,
    /** Enumumeration for Specular glossiness workflow */
    SPECULAR_GLOSSINESS = 1,
    /** Enumumeration for KHR materials unlit workflow */
    UNLIT = 2,
    /** Enumumeration for special unlit shadow receiver */
    UNLIT_SHADOW_ALPHA = 3,
    /** Custom material. Could be used with custom material model e.g. with shader graph.
     * Disables automatic factor based modifications for flags.
     * Note: that base color is always automatically pre-multiplied in all cases
     */
    CUSTOM = 4,
    /** Custom complex material. Could be used with custom material model e.g. with shader graph.
     * Disables automatic factor based modifications for flags.
     * Does not use deferred rendering path in any case due to complex material model.
     * Note: that base color is always automatically pre-multiplied in all cases
     */
    CUSTOM_COMPLEX = 5,
};

/** The rendering submesh specialization flag bits
 */
enum RenderSubmeshFlagBits : uint32_t {
    /** Defines whether to use tangents with this submesh. */
    RENDER_SUBMESH_TANGENTS_BIT = (1 << 0),
    /** Defines whether to use vertex colors with this submesh. */
    RENDER_SUBMESH_VERTEX_COLORS_BIT = (1 << 1),
    /** Defines whether to use skinning with this submesh. */
    RENDER_SUBMESH_SKIN_BIT = (1 << 2),
    /** Defines whether the submesh has second texcoord set available. */
    RENDER_SUBMESH_SECOND_TEXCOORD_BIT = (1 << 3),
    /** Defines whether to use inverse winding with this submesh. */
    RENDER_SUBMESH_INVERSE_WINDING_BIT = (1 << 4),
    /** Defines whether to calculate correct velocity in shader. */
    RENDER_SUBMESH_VELOCITY_BIT = (1 << 5),
};
/** Container for submesh flag bits */
using RenderSubmeshFlags = uint32_t;

/** Rendering flags (specialized rendering flags) */
enum RenderExtraRenderingFlagBits : uint32_t {
    /** Is an additional flag which can be used to discard some materials from rendering from render node graph */
    RENDER_EXTRA_RENDERING_DISCARD_BIT = (1 << 0),
};
/** Container for extra material rendering flag bits */
using RenderExtraRenderingFlags = uint32_t;

/** Render submesh */
struct RenderSubmesh {
    /** 64 bit id for render instance. Can be used for rendering time identification. RenderMeshComponent entity. */
    uint64_t id { RenderSceneDataConstants::INVALID_ID };
    /** 64 bit id for mesh instance. MeshComponent entity. */
    uint64_t meshId { RenderSceneDataConstants::INVALID_ID };
    /** Submesh index. */
    uint32_t subMeshIndex { 0 };

    /** Layer mask. */
    uint64_t layerMask { RenderSceneDataConstants::DEFAULT_LAYER_MASK };

    /** Render sort layer id. Valid values are 0 - 63 */
    uint8_t renderSortLayer { RenderSceneDataConstants::DEFAULT_RENDER_SORT_LAYER_ID };
    /** Render sort layer id. Valid values are 0 - 255 */
    uint8_t renderSortLayerOrder { 0 };

    /** A valid index to material. Get from AddMaterial() */
    uint32_t materialIndex { RenderSceneDataConstants::INVALID_INDEX };
    /** A valid index to mesh (matrix). Get from AddMeshData() */
    uint32_t meshIndex { RenderSceneDataConstants::INVALID_INDEX };
    /** A valid index to skin joint matrices if has skin. Get from AddSkinJointMatrices() */
    uint32_t skinJointIndex { RenderSceneDataConstants::INVALID_INDEX };
    /** A valid index to material custom resources if any. Get from AddMaterialCustomResources() */
    uint32_t customResourcesIndex { RenderSceneDataConstants::INVALID_INDEX };

    /** World center vector */
    BASE_NS::Math::Vec3 worldCenter { 0.0f, 0.0f, 0.0f };
    /** World radius */
    float worldRadius { 0.0f };

    /** Index buffer */
    RenderIndexBuffer indexBuffer;
    /** Vertex buffers */
    RenderVertexBuffer vertexBuffers[RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
    /** Vertex buffer count */
    uint32_t vertexBufferCount { 0 };

    /* Optional indirect args buffer for indirect draw. */
    RenderVertexBuffer indirectArgsBuffer;

    /** Draw command */
    RenderDrawCommand drawCommand;

    /** Submesh flags */
    RenderSubmeshFlags submeshFlags { 0 };
};

/** Render light */
struct RenderLight {
    /** Light usage flag bits */
    enum LightUsageFlagBits {
        /** Directional light bit */
        LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT = (1 << 0),
        /** Point light bit */
        LIGHT_USAGE_POINT_LIGHT_BIT = (1 << 1),
        /** Spot light bit */
        LIGHT_USAGE_SPOT_LIGHT_BIT = (1 << 2),
        /** Shadow light bit */
        LIGHT_USAGE_SHADOW_LIGHT_BIT = (1 << 3),
    };
    /** Light usage flags */
    using LightUsageFlags = uint32_t;

    /** Unique id. */
    uint64_t id { RenderSceneDataConstants::INVALID_ID };

    /** Layer mask (light affect mask). All enabled by default */
    uint64_t layerMask { RenderSceneDataConstants::INVALID_ID };

    /** Position */
    BASE_NS::Math::Vec4 pos { 0.0f, 0.0f, 0.0f, 0.0f };
    /** Direction */
    BASE_NS::Math::Vec4 dir { 0.0f, 0.0f, 0.0f, 0.0f };
    /** Color, w is intensity */
    BASE_NS::Math::Vec4 color { 0.0f, 0.0f, 0.0f, 0.0f };

    /* Spot light params. .x = angleScale, .y = angleOffset, .z = innerAngle, .w = outerAngle */
    BASE_NS::Math::Vec4 spotLightParams { 0.0f, 0.0f, 0.0f, 0.0f };
    /* Point and spot params. */
    float range { 0.0f };
    // .x = shadow factor, .y = depth bias, .z = normal bias, .w = empty (filled later with step size)
    BASE_NS::Math::Vec4 shadowFactors { 1.0f, 0.005f, 0.02f, 0.0f };

    /** Object ID */
    uint32_t objectId { ~0u };
    /** Light usage flags */
    LightUsageFlags lightUsageFlags { 0u };

    /** Shadow camera index in render lights */
    uint32_t shadowCameraIndex { ~0u };
    /** Filled by the data store */
    uint32_t shadowIndex { ~0u };
};

/** Render camera */
struct RenderCamera {
    enum CameraFlagBits : uint32_t {
        /** Clear depth. Overrides camera RNG loadOp with clear. */
        CAMERA_FLAG_CLEAR_DEPTH_BIT = (1 << 0),
        /** Clear color. Overrides camera RNG loadOp with clear. */
        CAMERA_FLAG_CLEAR_COLOR_BIT = (1 << 1),
        /** Shadow camera */
        CAMERA_FLAG_SHADOW_BIT = (1 << 2),
        /** MSAA enabled */
        CAMERA_FLAG_MSAA_BIT = (1 << 3),
        /** Reflection camera */
        CAMERA_FLAG_REFLECTION_BIT = (1 << 4),
        /** Main scene camera. I.e. the final main camera which might try to render to swapchain */
        CAMERA_FLAG_MAIN_BIT = (1 << 5),
        /** This is a pre-pass color camera. */
        CAMERA_FLAG_COLOR_PRE_PASS_BIT = (1 << 6),
        /** Render only opaque. */
        CAMERA_FLAG_OPAQUE_BIT = (1 << 7),
        /** Use and flip history */
        CAMERA_FLAG_HISTORY_BIT = (1 << 8),
        /** Jitter camera. */
        CAMERA_FLAG_JITTER_BIT = (1 << 9),
        /** Output samplable velocity normal */
        CAMERA_FLAG_OUTPUT_VELOCITY_NORMAL_BIT = (1 << 10),
        /** Disable FW velocity */
        CAMERA_FLAG_INVERSE_WINDING_BIT = (1 << 11),
        /** Samplable depth target */
        CAMERA_FLAG_OUTPUT_DEPTH_BIT = (1 << 12),
        /** Custom targets */
        CAMERA_FLAG_CUSTOM_TARGETS_BIT = (1 << 13),
        /** Multi-view camera */
        CAMERA_FLAG_MULTI_VIEW_ONLY_BIT = (1 << 14),
        /** Dynamic cubemap */
        CAMERA_FLAG_DYNAMIC_CUBEMAP_BIT = (1 << 15),
        /** Allow reflection */
        CAMERA_FLAG_ALLOW_REFLECTION_BIT = (1 << 16),
    };
    using Flags = uint32_t;

    enum ShaderFlagBits : uint32_t {
        /** Fog enabled in the shader. Changed based on render slots and rendering types. */
        CAMERA_SHADER_FOG_BIT = (1 << 0),
    };
    using ShaderFlags = uint32_t;

    enum class RenderPipelineType : uint32_t {
        /** Forward */
        LIGHT_FORWARD = 0,
        /** Forward */
        FORWARD = 1,
        /** Deferred */
        DEFERRED = 2,
        /** Custom */
        CUSTOM = 3,
    };

    enum class CameraCullType : uint8_t {
        /** None */
        CAMERA_CULL_NONE = 0,
        /** Front to back */
        CAMERA_CULL_VIEW_FRUSTUM = 1,
    };

    /** Matrices */
    struct Matrices {
        /** View matrix */
        BASE_NS::Math::Mat4X4 view;
        /** Projection matrix */
        BASE_NS::Math::Mat4X4 proj;

        /** Previous view matrix */
        BASE_NS::Math::Mat4X4 viewPrevFrame;
        /** Previous projection matrix */
        BASE_NS::Math::Mat4X4 projPrevFrame;
    };

    /** Environment setup */
    struct Environment {
        /** Background type for env node */
        enum BackgroundType {
            /** None */
            BG_TYPE_NONE = 0,
            /** Image (2d) */
            BG_TYPE_IMAGE = 1,
            /** Cubemap */
            BG_TYPE_CUBEMAP = 2,
            /** Equirectangular */
            BG_TYPE_EQUIRECTANGULAR = 3,
        };

        /** Unique id. */
        uint64_t id { RenderSceneDataConstants::INVALID_ID };

        /** Layer mask (camera render mask). All enabled by default */
        uint64_t layerMask { RenderSceneDataConstants::INVALID_ID };

        /** Radiance cubemap resource handle */
        RENDER_NS::RenderHandleReference radianceCubemap;
        /** Radiance cubemap mip count. Zero indicates that full mipchain is available */
        uint32_t radianceCubemapMipCount { 0u };

        /** Environment map resource handle */
        RENDER_NS::RenderHandleReference envMap;
        /** Environment map lod level for sampling */
        float envMapLodLevel { 0.0f };

        /** Spherical harmonic coefficients for indirect diffuse (irradiance)
         * Prescaled with 1.0 / PI. */
        BASE_NS::Math::Vec4 shIndirectCoefficients[RenderSceneDataConstants::MAX_SH_VEC3_COUNT];

        /** Indirect diffuse color factor (.rgb = tint, .a = intensity) */
        BASE_NS::Math::Vec4 indirectDiffuseFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        /** Indirect specular color factor (.rgb = tint, .a = intensity) */
        BASE_NS::Math::Vec4 indirectSpecularFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        /** Env map color factor (.rgb = tint, .a = intensity) */
        BASE_NS::Math::Vec4 envMapFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        /** Additional blend factor */
        BASE_NS::Math::Vec4 blendFactor { 0.0f, 0.0f, 0.0f, 0.0f };

        /** Environment rotation */
        BASE_NS::Math::Quat rotation { 0.0f, 0.0f, 0.0f, 1.0f };

        /** Background type */
        BackgroundType backgroundType { BackgroundType::BG_TYPE_NONE };

        /** Custom shader handle */
        RENDER_NS::RenderHandleReference shader;
        /** invalid handles if not given */
        RENDER_NS::RenderHandleReference customResourceHandles[RenderSceneDataConstants::MAX_ENV_CUSTOM_RESOURCE_COUNT];
    };

    /** Fog setup */
    struct Fog {
        /** Unique id. */
        uint64_t id { RenderSceneDataConstants::INVALID_ID };

        /** Layer mask (camera render mask). All enabled by default */
        uint64_t layerMask { RenderSceneDataConstants::INVALID_ID };

        /** .x = density, .y = heightFalloff, .z = heightFogOffset */
        BASE_NS::Math::Vec4 firstLayer { 1.0f, 1.0f, 1.0f, 0.0f };
        /** .x = density, .y = heightFalloff, .z = heightFogOffset */
        BASE_NS::Math::Vec4 secondLayer { 1.0f, 1.0f, 1.0f, 0.0f };

        /** .x = startDistance, .y = cutoffDistance, .z = maxOpacity */
        BASE_NS::Math::Vec4 baseFactors { 1.0f, -1.0f, 1.0f, 0.0f };

        /** Primary color for the fog (.rgb = tint, .a = intensity) */
        BASE_NS::Math::Vec4 inscatteringColor { 1.0f, -1.0f, 1.0f, 0.0f };
        /** Env map color factor (.rgb = tint, .a = intensity) */
        BASE_NS::Math::Vec4 envMapFactor { 1.0f, -1.0f, 1.0f, 0.0f };

        /** Additional factor */
        BASE_NS::Math::Vec4 additionalFactor { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    /** Unique id. */
    uint64_t id { RenderSceneDataConstants::INVALID_ID };
    /** Shadow id. (Can be invalid) */
    uint64_t shadowId { RenderSceneDataConstants::INVALID_ID };

    /** Layer mask (camera render mask). All enabled by default */
    uint64_t layerMask { RenderSceneDataConstants::INVALID_ID };

    /** Main camera to this camera id. (e.g. reflection camera has info of the main camera) */
    uint64_t mainCameraId { RenderSceneDataConstants::INVALID_ID };

    /** Matrices (Contains view, projection, view of previous frame and projection of previous frame) */
    Matrices matrices;

    /** Viewport (relative to render resolution) */
    BASE_NS::Math::Vec4 viewport { 0.0f, 0.0f, 1.0f, 1.0f };
    /** Scissor (relative to render resolution) */
    BASE_NS::Math::Vec4 scissor { 0.0f, 0.0f, 1.0f, 1.0f };
    /** Render resolution */
    BASE_NS::Math::UVec2 renderResolution { 0u, 0u };

    /** Z near value */
    float zNear { 0.0f };
    /** Z far value */
    float zFar { 0.0f };

    /** Custom depth target */
    RENDER_NS::RenderHandleReference depthTarget {};
    /** Custom color targets */
    RENDER_NS::RenderHandleReference colorTargets[RenderSceneDataConstants::MAX_CAMERA_COLOR_TARGET_COUNT];

    /** Custom pre-pass color target. Can be tried to fetch with a name if handle is not given. */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> prePassColorTargetName;

    /** Flags for camera */
    Flags flags { 0u };
    /** Shader flags for camera */
    ShaderFlags shaderFlags { 0u };

    /** Flags for camera render pipeline */
    RenderPipelineType renderPipelineType { RenderPipelineType::FORWARD };

    /** Clear depth / stencil values. Clear enabled if flags set. */
    RENDER_NS::ClearDepthStencilValue clearDepthStencil { 1.0f, 0u };
    /** Clear color values. Clear enabled if flags set */
    RENDER_NS::ClearColorValue clearColorValues { 0.0f, 0.0f, 0.0f, 0.0f };

    /** Camera cull type */
    CameraCullType cullType { CameraCullType::CAMERA_CULL_VIEW_FRUSTUM };

    /** Default environment setup for camera */
    Environment environment;

    /** Fog setup for camera */
    Fog fog;

    /** Custom render node graph from camera component (WILL BE DEPRECATED) */
    RENDER_NS::RenderHandleReference customRenderNodeGraph;

    /** Custom render node graph file from camera component */
    BASE_NS::string customRenderNodeGraphFile;

    /** Custom render node graph file from post process configuration component */
    BASE_NS::string customPostProcessRenderNodeGraphFile;

    /** Target customization */
    struct TargetUsage {
        /** Target format */
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
        /** Usage flags hints for optimizing resource creation */
        RENDER_NS::ImageUsageFlags usageFlags { 0 };
    };

    /** Depth target customization */
    TargetUsage depthTargetCustomization;
    /** Color target customization */
    TargetUsage colorTargetCustomization[RenderSceneDataConstants::MAX_CAMERA_COLOR_TARGET_COUNT];

    /** Unique camera name for getting named camera. */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> name;

    /** Camera post process name. Can be empty */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> postProcessName;

    /** Multi-view extra camera count. */
    uint32_t multiViewCameraCount { 0U };
    /** 64bit camera id of multi-view layer cameras. */
    uint64_t multiViewCameraIds[RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT] {
        RenderSceneDataConstants::INVALID_ID, RenderSceneDataConstants::INVALID_ID, RenderSceneDataConstants::INVALID_ID
    };
    uint64_t multiViewParentCameraId { RenderSceneDataConstants::INVALID_ID };

    /** Environment count. */
    uint32_t environmentCount { 0U };
    /** 64bit environment id of environments. */
    uint64_t environmentIds[DefaultMaterialCameraConstants::MAX_ENVIRONMENT_COUNT] {
        RenderSceneDataConstants::INVALID_ID, RenderSceneDataConstants::INVALID_ID,
        RenderSceneDataConstants::INVALID_ID, RenderSceneDataConstants::INVALID_ID,
        RenderSceneDataConstants::INVALID_ID, RenderSceneDataConstants::INVALID_ID,
        RenderSceneDataConstants::INVALID_ID, RenderSceneDataConstants::INVALID_ID
    };
};

/** Render scene */
struct RenderScene {
    /** Unique id. */
    uint64_t id { RenderSceneDataConstants::INVALID_ID };
    /** Unique scene name (If name is empty a default id/name is created with index) */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> name;

    /** Scene render data store name needs to be known */

    /** Camera render data store name */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameCamera;
    /** Light render data store name */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameLight;
    /** Material data store name */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameMaterial;
    /** Morphing data store name */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameMorph;
    /** Data store name prefix */
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNamePrefix;

    /** World scene center */
    BASE_NS::Math::Vec3 worldSceneCenter { 0.0f, 0.0f, 0.0f };
    /** World scene bounding sphere radius */
    float worldSceneBoundingSphereRadius { 0.0f };

    /** Index of scene camera in rendering cameras */
    uint32_t cameraIndex { RenderSceneDataConstants::INVALID_INDEX };

    /** Scene delta time in milliseconds */
    float sceneDeltaTime { 0 };
    /** Real total tick time in seconds */
    float totalTime { 0u };
    /** Real delta tick time in milliseconds */
    float deltaTime { 0u };
    /** Render scene frame index */
    uint32_t frameIndex { 0u };

    /** Custom render node graph file from scene component */
    BASE_NS::string customRenderNodeGraphFile;
    /** Custom post scene render node graph file from scene component */
    BASE_NS::string customPostSceneRenderNodeGraphFile;
};

struct SlotSubmeshIndex {
    uint32_t submeshIndex { 0 };

    uint32_t sortLayerKey { 0 };
    uint64_t sortKey { 0 };
    uint32_t renderMaterialSortHash { 0 };
    RENDER_NS::RenderHandle shaderHandle;
    RENDER_NS::RenderHandle gfxStateHandle;
};

enum RenderSceneFlagBits : uint32_t {
    RENDER_SCENE_DIRECT_POST_PROCESS_BIT = (1 << 0),
    RENDER_SCENE_FLIP_WINDING_BIT = (1 << 1),
    RENDER_SCENE_DISCARD_MATERIAL_BIT = (1 << 2),
    RENDER_SCENE_ENABLE_FOG_BIT = (1 << 3),
    RENDER_SCENE_DISABLE_FOG_BIT = (1 << 4),
};
using RenderSceneFlags = uint32_t;
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_RENDER_DATA_DEFINES_H
