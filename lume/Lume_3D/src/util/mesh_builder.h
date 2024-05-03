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

#ifndef CORE_UTIL_MESH_BUILDER_H
#define CORE_UTIL_MESH_BUILDER_H

#include <3d/util/intf_mesh_builder.h>
#include <base/util/formats.h>
#include <core/namespace.h>
#include <render/resource_handle.h>

#include "gltf/gltf2_util.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class MeshBuilder final : public IMeshBuilder {
public:
    MeshBuilder(RENDER_NS::IRenderContext& renderContext);

    ~MeshBuilder() override = default;

    void Initialize(const RENDER_NS::VertexInputDeclarationView& vertexInputDeclaration, size_t submeshCount) override;

    void AddSubmesh(const MeshBuilder::Submesh& submesh) override;

    const Submesh& GetSubmesh(size_t index) const override;

    void Allocate() override;

    void SetVertexData(size_t submeshIndex, const DataBuffer& positions, const DataBuffer& normals,
        const DataBuffer& texcoords0, const DataBuffer& texcoords1, const DataBuffer& tangents,
        const DataBuffer& colors) override;

    void SetIndexData(size_t submeshIndex, const DataBuffer& indices) override;

    void SetJointData(size_t submeshIndex, const DataBuffer& jointData, const DataBuffer& weightData,
        const DataBuffer& vertexPositions) override;

    void SetMorphTargetData(size_t submeshIndex, const DataBuffer& basePositions, const DataBuffer& baseNormals,
        const DataBuffer& baseTangents, const DataBuffer& targetPositions, const DataBuffer& targetNormals,
        const DataBuffer& targetTangents) override;

    void SetAABB(size_t submeshIndex, const BASE_NS::Math::Vec3& min, const BASE_NS::Math::Vec3& max) override;
    void CalculateAABB(size_t submeshIndex, const DataBuffer& positions) override;

    BASE_NS::array_view<const uint8_t> GetVertexData() const override;
    BASE_NS::array_view<const uint8_t> GetIndexData() const override;
    BASE_NS::array_view<const uint8_t> GetJointData() const override;
    BASE_NS::array_view<const uint8_t> GetMorphTargetData() const override;

    BASE_NS::array_view<const float> GetJointBoundsData() const override;

    BASE_NS::array_view<const MeshComponent::Submesh> GetSubmeshes() const override;

    uint32_t GetVertexCount() const override;
    uint32_t GetIndexCount() const override;

    void CreateGpuResources() override;
    void CreateGpuResources(const GpuBufferCreateInfo& createInfo) override;

    CORE_NS::Entity CreateMesh(CORE_NS::IEcs& ecs) const override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;

    struct BufferHandles {
        RENDER_NS::RenderHandleReference vertexBuffer;
        RENDER_NS::RenderHandleReference jointBuffer;
        RENDER_NS::RenderHandleReference indexBuffer;
        RENDER_NS::RenderHandleReference morphBuffer;
    };

    struct BufferEntities {
        CORE_NS::EntityReference vertexBuffer;
        CORE_NS::EntityReference jointBuffer;
        CORE_NS::EntityReference indexBuffer;
        CORE_NS::EntityReference morphBuffer;
    };

    // Morph target descriptor
    struct MorphTargetDesc {
        // Offset to morph target data from submesh's morphTargetOffset.
        uint32_t offset { 0 };
        // Byte size of morph target data.
        uint32_t byteSize { 0 };
    };

    // Extend submesh info with offset-data.
    struct SubmeshExt {
        MeshBuilder::Submesh info;

        // Automatically calculated by builder.
        BASE_NS::vector<uint32_t> vertexBindingByteSize;
        BASE_NS::vector<uint32_t> vertexBindingOffset;
        BASE_NS::vector<MorphTargetDesc> morphTargets;
        uint32_t jointBufferOffset = 0;
        uint32_t indexBufferOffset = 0;
        uint32_t morphTargetBufferOffset = 0;
        uint32_t morphTargetBufferSize = 0;
        bool hasNormals = false;
        bool hasUv0 = false;
        bool hasTangents = false;
        int32_t positionOffset = -1;
        uint32_t positionSize = 0;
        int32_t normalOffset = -1;
        uint32_t normalSize = 0;
        int32_t uvOffset = -1;
        uint32_t uvSize = 0;
        int32_t tangentsOffset = -1;
        uint32_t tangentSize = 0;
        int32_t indexOffset = -1;
        uint32_t indexSize = 0;
    };

private:
    BufferEntities CreateBuffers(CORE_NS::IEcs& ecs) const;
    void GenerateMissingAttributes() const;

    struct BufferSizesInBytes {
        size_t indexBuffer;
        size_t jointBuffer;
        size_t morphVertexData;
    };

    BufferSizesInBytes CalculateSizes();

    static void GatherDeltasP(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
        uint32_t targetSize, const DataBuffer& targetPositions);
    static void GatherDeltasPN(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
        uint32_t targetSize, const DataBuffer& targetPositions, const DataBuffer& targetNormals);
    static void GatherDeltasPT(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
        uint32_t targetSize, const DataBuffer& targetPositions, const MeshBuilder::DataBuffer& targetTangents);
    static void GatherDeltasPNT(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
        uint32_t targetSize, const DataBuffer& targetPositions, const DataBuffer& targetNormals,
        const DataBuffer& targetTangents);

    void CalculateJointBounds(
        const DataBuffer& jointData, const DataBuffer& weightData, const DataBuffer& vertexPositions);

    bool WriteData(const DataBuffer& data, const SubmeshExt& submesh, uint32_t attributeLocation, uint32_t& byteOffset,
        uint32_t& byteSize, uint8_t* dst) const;

    RENDER_NS::IRenderContext& renderContext_;
    RENDER_NS::VertexInputDeclarationView vertexInputDeclaration_;

    mutable BASE_NS::vector<SubmeshExt> submeshInfos_;
    mutable BASE_NS::vector<MeshComponent::Submesh> submeshes_;

    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;

    size_t vertexDataSize_ = 0;
    size_t indexDataSize_ = 0;
    size_t jointDataSize_ = 0;
    size_t targetDataSize_ = 0;
    BufferHandles bufferHandles_;
    RENDER_NS::RenderHandleReference stagingBuffer_;
    uint8_t* stagingPtr_ = nullptr;

    struct Bounds {
        BASE_NS::Math::Vec3 min;
        BASE_NS::Math::Vec3 max;
    };
    // Bounds for each joint is 6 floats (3 min & 3 max).
    static_assert(sizeof(Bounds) == (sizeof(float) * 6));
    BASE_NS::vector<Bounds> jointBoundsData_;

    mutable BASE_NS::vector<uint8_t> vertexData_;
    BASE_NS::vector<uint8_t> indexData_;
    uint32_t refCount_ = 0;
};
CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_MESH_BUILDER_H
