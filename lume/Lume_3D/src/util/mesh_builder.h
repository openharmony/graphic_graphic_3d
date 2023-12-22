/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

CORE3D_BEGIN_NAMESPACE()
class MeshBuilder final : public IMeshBuilder {
public:
    MeshBuilder() = default;

    ~MeshBuilder() override = default;

    void Initialize(const RENDER_NS::VertexInputDeclarationView& vertexInputDeclaration, size_t submeshCount) override;

    void AddSubmesh(const MeshBuilder::Submesh& submesh) override;

    const Submesh& GetSubmesh(size_t index) const override;

    void Allocate() override;

    void SetVertexData(size_t submeshIndex, const BASE_NS::array_view<const BASE_NS::Math::Vec3>& positions,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& normals,
        const BASE_NS::array_view<const BASE_NS::Math::Vec2>& texcoords0,
        const BASE_NS::array_view<const BASE_NS::Math::Vec2>& texcoords1,
        const BASE_NS::array_view<const BASE_NS::Math::Vec4>& tangents,
        const BASE_NS::array_view<const BASE_NS::Math::Vec4>& colors) override;

    void SetVertexData(size_t submeshIndex, const BASE_NS::array_view<const float>& positions,
        const BASE_NS::array_view<const float>& normals, const BASE_NS::array_view<const float>& texcoords0,
        const BASE_NS::array_view<const float>& texcoords1, const BASE_NS::array_view<const float>& tangents,
        const BASE_NS::array_view<const float>& colors) override;

    void SetIndexData(size_t submeshIndex, const BASE_NS::array_view<const uint8_t>& indices) override;
    void SetJointData(size_t submeshIndex, const BASE_NS::array_view<const uint8_t>& jointData,
        const BASE_NS::array_view<const BASE_NS::Math::Vec4>& weightData,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& vertexPositions) override;

    void SetMorphTargetData(size_t submeshIndex, const BASE_NS::array_view<const BASE_NS::Math::Vec3>& basePositions,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& baseNormals,
        const BASE_NS::array_view<const BASE_NS::Math::Vec4>& baseTangents,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& targetPositions,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& targetNormals,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& targetTangents) override;

    void SetAABB(size_t submeshIndex, const BASE_NS::Math::Vec3& min, const BASE_NS::Math::Vec3& max) override;
    void CalculateAABB(size_t submeshIndex, const BASE_NS::array_view<const BASE_NS::Math::Vec3>& positions) override;

    BASE_NS::array_view<const uint8_t> GetVertexData() const override;
    BASE_NS::array_view<const uint8_t> GetIndexData() const override;
    BASE_NS::array_view<const uint8_t> GetJointData() const override;
    BASE_NS::array_view<const uint8_t> GetMorphTargetData() const override;

    BASE_NS::array_view<const float> GetJointBoundsData() const override;

    const BASE_NS::array_view<const MeshComponent::Submesh> GetSubmeshes() const override;

    uint32_t GetVertexCount() const override;
    uint32_t GetIndexCount() const override;

    CORE_NS::Entity CreateMesh(CORE_NS::IEcs& ecs) const override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;

    struct BufferEntities {
        CORE_NS::EntityReference vertexBuffer;
        CORE_NS::EntityReference jointBuffer;
        CORE_NS::EntityReference indexBuffer;
        CORE_NS::EntityReference morphBuffer;
    };

private:
    BufferEntities CreateBuffers(CORE_NS::IEcs& ecs) const;

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
        uint32_t vertexIndex = 0;
        BASE_NS::vector<uint32_t> vertexBindingByteSize;
        BASE_NS::vector<uint32_t> vertexBindingOffset;
        uint32_t jointBufferOffset = 0;
        uint32_t indexBufferOffset = 0;
        uint32_t morphTargetBufferOffset = 0;
        uint32_t morphTargetBufferSize = 0;

        BASE_NS::vector<MorphTargetDesc> morphTargets;
    };
    struct BufferSizesInBytes {
        size_t indexBuffer;
        size_t jointBuffer;
        size_t morphVertexData;
    };
    BufferSizesInBytes CalculateSizes();
    void GatherDeltas(SubmeshExt& submesh, uint32_t baseOffset, uint32_t indexOffset, uint32_t targetSize,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3> targetPositions,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3> targetNormals,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3> targetTangents);

    void CalculateJointBounds(const BASE_NS::array_view<const BASE_NS::Math::Vec4>& weightData,
        const BASE_NS::array_view<const uint8_t>& jointData,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& vertexPositions);

    bool WriteData(const BASE_NS::array_view<const float>& data, uint32_t componentCount, const SubmeshExt& submesh,
        uint32_t attributeLocation, uint32_t& byteOffset, uint32_t& byteSize);

    void WriteFloatAttributeData(const void* destination, const void* source, size_t bufferOffset, size_t stride,
        size_t componentCount, size_t componentByteSize, size_t elementCount, BASE_NS::Format targetFormat);

    void WriteByteAttributeData(const void* destination, const void* source, size_t bufferOffset, size_t stride,
        size_t componentCount, size_t componentByteSize, size_t elementCount, BASE_NS::Format targetFormat);

    RENDER_NS::VertexInputDeclarationView vertexInputDeclaration_;

    BASE_NS::vector<SubmeshExt> submeshInfos_;
    BASE_NS::vector<MeshComponent::Submesh> submeshes_;

    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;

    BASE_NS::vector<uint8_t> vertexData_;
    BASE_NS::vector<uint8_t> jointData_;
    BASE_NS::vector<uint8_t> indexData_;
    BASE_NS::vector<uint8_t> targetData_;

    struct Bounds {
        BASE_NS::Math::Vec3 min;
        BASE_NS::Math::Vec3 max;
    };
    // Bounds for each joint is 6 floats (3 min & 3 max).
    static_assert(sizeof(Bounds) == (sizeof(float) * 6));
    BASE_NS::vector<Bounds> jointBoundsData_;
    uint32_t refCount_ = 0;
};

CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_MESH_BUILDER_H
