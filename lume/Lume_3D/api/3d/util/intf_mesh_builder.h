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

#ifndef API_3D_UTIL_IMESH_BUILDER_H
#define API_3D_UTIL_IMESH_BUILDER_H

#include <cstddef>

#include <3d/ecs/components/mesh_component.h>
#include <base/containers/refcnt_ptr.h>
#include <base/util/formats.h>
#include <core/ecs/entity.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>
#include <render/device/pipeline_state_desc.h>
#include <render/resource_handle.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_util_imeshbuilder */
/** Mesh builder interface for building meshes */
class IMeshBuilder : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "8d2892a4-77e5-4304-a5aa-38f866b7c788" };

    using Ptr = BASE_NS::refcnt_ptr<IMeshBuilder>;

    /** Submesh attributes */
    struct Submesh {
        /** Vertex count */
        uint32_t vertexCount { 0 };
        /** Index count */
        uint32_t indexCount { 0 };
        /** Instance count */
        uint32_t instanceCount { 1 };
        /** Morph target count */
        uint32_t morphTargetCount { 0 };
        /** Index type */
        RENDER_NS::IndexType indexType { RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT32 };

        /** Material */
        CORE_NS::Entity material {};
        /** Tangents */
        bool tangents { false };
        /** Colors */
        bool colors { false };
        /** Joints */
        bool joints { false };
    };

    /** GPU buffer create info */
    struct GpuBufferCreateInfo {
        /** Vertex buffer additional usage flags */
        RENDER_NS::BufferUsageFlags vertexBufferFlags { 0U };
        /** Index buffer additional usage flags */
        RENDER_NS::BufferUsageFlags indexBufferFlags { 0U };
        /** Morph buffer additional usage flags */
        RENDER_NS::BufferUsageFlags morphBufferFlags { 0U };
    };

    /** Setup vertex declaration and submesh count for the builder. Also resets the builder for re-use.
     */
    virtual void Initialize(
        const RENDER_NS::VertexInputDeclarationView& vertexInputDeclaration, size_t submeshCount) = 0;

    /** Add a submesh and related import info to this mesh.
     * @param submesh Submesh import information that is used to determine memory requirements etc.
     */
    virtual void AddSubmesh(const Submesh& submesh) = 0;

    /** Returns Import info for given submesh.
     * @param index Index of the submesh.
     * @return Import information for the given submesh.
     */
    virtual const Submesh& GetSubmesh(size_t index) const = 0;

    /** Allocates memory for this mesh, should be called after all submeshes have been added to this mesh and before
     * data is being fed to submeshes. */
    virtual void Allocate() = 0;

    /** Struct for passing data to the mesh builder.*/
    struct DataBuffer {
        /** Format of each element in buffer. e.g. three float values per element would be R32G32B32_SFLOAT. */
        BASE_NS::Format format;
        /** Offset between elements. This should match the size of one element for tightly packed values. */
        uint32_t stride;
        /** Byte arrays which will be interpreted based on format and stride. */
        BASE_NS::array_view<const uint8_t> buffer;
    };

    /** Set geometry data for a given submesh.
     * @param submeshIndex Index of the submesh.
     * @param positions Position data (3 * vertexCount values), this parameter is required.
     * @param normals Normal data (3 * vertexCount values), this parameter is required.
     * @param texcoords0 Texture coordinate 0 data (2 * vertexCount values), this parameter is required.
     * @param texcoords1 Texture coordinate 1 data (2 * vertexCount values), this parameter is optional.
     * @param tangents Tangent data (4 * vertexCount values), this parameter is optional.
     * @param colors Vertex color data  (4 * vertexCount values), this parameter is optional.
     */
    virtual void SetVertexData(size_t submeshIndex, const DataBuffer& positions, const DataBuffer& normals,
        const DataBuffer& texcoords0, const DataBuffer& texcoords1, const DataBuffer& tangents,
        const DataBuffer& colors) = 0;

    /** Set Axis-aligned bounding box for a submesh.
     * @param submeshIndex Index of the submesh.
     * @param min Minimum corner.
     * @param max Minimum corner.
     */
    virtual void SetAABB(size_t submeshIndex, const BASE_NS::Math::Vec3& min, const BASE_NS::Math::Vec3& max) = 0;

    /** Calculate Axis-aligned bounding box for a submesh.
     * @param submeshIndex Index of the submesh.
     * @param positions Array of vertex positions in submesh.
     */
    virtual void CalculateAABB(size_t submeshIndex, const DataBuffer& positions) = 0;

    /** Set triangle indices for a submesh.
     * @param submeshIndex Index of the submesh.
     * @param indices Index data.
     */
    virtual void SetIndexData(size_t submeshIndex, const DataBuffer& indices) = 0;

    /** Set Joint data for a submesh.
     * @param submeshIndex Index of the submesh.
     * @param jointData Joint indices per vertex, (4 indices per bone).
     * @param weightData Joint weights per vertex , (4 weights per bone).
     * @param vertexPositions Position data that is used for skin bounds calculations (3 * vertexCount
     * values), this data should match the data set with SetVertexData().
     */
    virtual void SetJointData(size_t submeshIndex, const DataBuffer& jointData, const DataBuffer& weightData,
        const DataBuffer& vertexPositions) = 0;

    /** Set Morph targets for a submesh.
     * @param submeshIndex Index of the submesh.
     * @param basePositions Initial vertex positions (base pose).
     * @param baseNormals Initial vertex normals (base pose).
     * @param baseTangents Initial vertex tangents (base pose).
     * @param targetPositions Morph target vertex positions (delta offsets from base vertex, total size of array is
     * target_count * vertex_count).
     * @param targetNormals Morph target vertex normals (delta offsets from base vertex, total size of array is
     * target_count * vertex_count).
     * @param targetTangents Morph target vertex tangents (delta offsets from base vertex, total size of array is
     * target_count * vertex_count).
     */
    virtual void SetMorphTargetData(size_t submeshIndex, const DataBuffer& basePositions, const DataBuffer& baseNormals,
        const DataBuffer& baseTangents, const DataBuffer& targetPositions, const DataBuffer& targetNormals,
        const DataBuffer& targetTangents) = 0;

    /** Returns all vertex data of this mesh. */
    virtual BASE_NS::array_view<const uint8_t> GetVertexData() const = 0;

    /** Returns all index data of this mesh. */
    virtual BASE_NS::array_view<const uint8_t> GetIndexData() const = 0;

    /** Returns all joint data of this mesh. */
    virtual BASE_NS::array_view<const uint8_t> GetJointData() const = 0;

    /** Returns all morph target data of this mesh. */
    virtual BASE_NS::array_view<const uint8_t> GetMorphTargetData() const = 0;

    /**
     * Returns an array of joint bounds data. Each joint that is referenced by the vertex joint data will
     * be represented by 6 values defining the min and max bounds defining the bounds for that joint.
     */
    virtual BASE_NS::array_view<const float> GetJointBoundsData() const = 0;

    /** Returns all submeshes of this mesh. */
    virtual BASE_NS::array_view<const MeshComponent::Submesh> GetSubmeshes() const = 0;

    /** Returns total vertex count in this mesh, note that this function should be called after Allocate() has been
     * called. */
    virtual uint32_t GetVertexCount() const = 0;

    /** Returns total index count in this mesh, note that 16bit indices are packed (two per one 32-bit slot) and there
     * might be padding involved. */
    virtual uint32_t GetIndexCount() const = 0;

    /** Creates GPU buffers and copies the data passed in by the Set*Data calls.
     */
    virtual void CreateGpuResources() = 0;

    /** Creates GPU buffers and copies the data passed in by the Set*Data calls.
     * @param Additional create info with additional flags for buffers.
     */
    virtual void CreateGpuResources(const GpuBufferCreateInfo& createInfo) = 0;

    /** Helper for creating an entity with a mesh component. The mesh component is filled using the previously given
     * data (submeshes, vertex data, index data, etc.). NOTE: the mesh component owns the attached GPU resources.
     */
    virtual CORE_NS::Entity CreateMesh(CORE_NS::IEcs& ecs) const = 0;

protected:
    IMeshBuilder() = default;
    virtual ~IMeshBuilder() = default;
};

inline constexpr BASE_NS::string_view GetName(const IMeshBuilder*)
{
    return "IMeshBuilder";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_UTIL_IMESH_BUILDER_H
