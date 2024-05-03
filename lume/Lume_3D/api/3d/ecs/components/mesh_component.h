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

#if !defined(API_3D_ECS_COMPONENTS_MESH_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_MESH_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <base/util/formats.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <render/device/gpu_resource_desc.h>

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_mesh_meshdesc
 *  @{
 */
#endif

/** Mesh component represents a mesh which can be e.g. rendered by attaching the owning entity to RenderMeshComponents.
 * GPU buffers are attached via EntityReferences. These refer to entities which are expected to have a
 * RenderHandleComponent pointing to the actual resource.
 */
BEGIN_COMPONENT(IMeshComponentManager, MeshComponent)
#if !defined(IMPLEMENT_MANAGER)
    /** Submesh descriptor */
    struct Submesh {
        /** Submesh flag bits */
        enum FlagBits {
            /** Defines whether to use tangents with this submesh. */
            TANGENTS_BIT = (1 << 0),
            /** Defines whether to use vertex colors with this submesh. */
            VERTEX_COLORS_BIT = (1 << 1),
            /** Defines whether to use skinning with this submesh. */
            SKIN_BIT = (1 << 2),
            /** Defines whether the submesh has second texcoord set available. */
            SECOND_TEXCOORD_BIT = (1 << 3),
        };

        /** Container for submesh flag bits */
        using Flags = uint32_t;

        /** Buffer indices from which default material shaders fetch data. */
        enum DefaultMaterialBufferIndices {
            /** Vertex positions. */
            DM_VB_POS = 0,
            /** Vertex normals. */
            DM_VB_NOR,
            /** First UV coordinates. */
            DM_VB_UV0,
            /** Second UV coordinates. */
            DM_VB_UV1,
            /** Vertex tangents. */
            DM_VB_TAN,
            /** Joint indices. */
            DM_VB_JOI,
            /** Joint weights. */
            DM_VB_JOW,
            /** Vertex colors. */
            DM_VB_COL,
        };

        /** Maximum number of data buffers.
         * Special buffers (index and indirect args) are still separated for easier access.
         */
        static constexpr uint32_t BUFFER_COUNT = 8;

        /** Default value to access whole buffer byte size */
        static constexpr uint32_t MAX_BUFFER_ACCESS_BYTE_SIZE = 0xFFFFffff;

        /** Default render sort layer id */
        static constexpr uint32_t DEFAULT_RENDER_SORT_LAYER_ID = 32u;

        /** Buffer access */
        struct BufferAccess {
            /** The buffer */
            CORE_NS::EntityReference buffer;
            /** Byte offset to buffer data */
            uint32_t offset { 0 };
            /** Byte size to be used */
            uint32_t byteSize { MAX_BUFFER_ACCESS_BYTE_SIZE };
        };
        /** Index buffer access */
        struct IndexBufferAccess {
            /** The buffer */
            CORE_NS::EntityReference buffer;
            /** Byte offset to buffer data */
            uint32_t offset { 0 };
            /** Byte size to be used */
            uint32_t byteSize { MAX_BUFFER_ACCESS_BYTE_SIZE };
            /** Enumeration for index type (uint16, uint32) */
            RENDER_NS::IndexType indexType { RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT32 };
        };

        /** Instance count */
        uint32_t instanceCount { 1u };

        /** Vertex data buffers */
        BufferAccess bufferAccess[BUFFER_COUNT];
        /** Index buffer */
        IndexBufferAccess indexBuffer;
        /** Indirect args buffer */
        BufferAccess indirectArgsBuffer;
        /** Morph target buffer */
        BufferAccess morphTargetBuffer;

        /** Vertex count */
        uint32_t vertexCount { 0 };
        /** Index count */
        uint32_t indexCount { 0 };
        /** Morph target count */
        uint32_t morphTargetCount { 0 };

        /** AABB min */
        BASE_NS::Math::Vec3 aabbMin { 0.0f, 0.0f, 0.0f };
        /** AABB max */
        BASE_NS::Math::Vec3 aabbMax { 0.0f, 0.0f, 0.0f };

        /** Material to be used with this submesh. */
        CORE_NS::Entity material {};

        /** Material to be used with this submesh. */
        BASE_NS::vector<CORE_NS::Entity> additionalMaterials;

        /** Submesh flags that define the shader variant to be used with this submesh. */
        Flags flags { 0 };

        /** Render sort layer. Within a render slot a layer can define a sort layer order.
         * There are 0-63 values available. Default id value is 32.
         * 0 first, 63 last
         * 1. Typical use case is to set render sort layer to objects which render with depth test without depth write.
         * 2. Typical use case is to always render character and/or camera object first to cull large parts of the view.
         * 3. Sort e.g. plane layers.
         */
        /** Sort layer used sorting submeshes in rendering in render slots. Valid ID values 0 - 63. */
        uint8_t renderSortLayer { DEFAULT_RENDER_SORT_LAYER_ID };
        /** Sort layer order to describe fine order within sort layer. Valid order 0 - 255 */
        uint8_t renderSortLayerOrder { 0u };
    };
#endif

    /** Submeshes */
    DEFINE_PROPERTY(BASE_NS::vector<Submesh>, submeshes, "Submeshes", 0, )

    /** Joint bounds */
    DEFINE_PROPERTY(BASE_NS::vector<float>, jointBounds, "Joint Bounds", 0, )

    /** AABB min */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, aabbMin, "Min AABB", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))

    /** AABB max */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, aabbMax, "Max AABB", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))
END_COMPONENT(IMeshComponentManager, MeshComponent, "cd08dae1-d13b-4a4a-a317-56acbb332f76")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
