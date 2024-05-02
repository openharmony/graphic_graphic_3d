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

#include "mesh_util.h"

#include <algorithm>

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <base/containers/vector.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_factory.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "util/uri_lookup.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr Math::Vec3 PLANE_NORM[6u] = {
    Math::Vec3(0.0f, 1.0f, 0.0f),
    Math::Vec3(0.0f, 1.0f, 0.0f),
    Math::Vec3(0.0f, 1.0f, 0.0f),

    Math::Vec3(0.0f, 1.0f, 0.0f),
    Math::Vec3(0.0f, 1.0f, 0.0f),
    Math::Vec3(0.0f, 1.0f, 0.0f),
};

constexpr Math::Vec2 PLANE_UV[6u] = {
    Math::Vec2(1.0f, 1.0f),
    Math::Vec2(1.0f, 0.0f),
    Math::Vec2(0.0f, 1.0f),

    Math::Vec2(1.0f, 0.0f),
    Math::Vec2(0.0f, 0.0f),
    Math::Vec2(0.0f, 1.0f),
};

constexpr uint16_t PLANE_IND[] = { 0u, 1u, 2u, 3u, 4u, 5u };

constexpr Math::Vec3 CUBE_POS[8u] = {
    Math::Vec3(-0.5f, -0.5f, -0.5f), //
    Math::Vec3(0.5f, -0.5f, -0.5f),  //
    Math::Vec3(0.5f, 0.5f, -0.5f),   //
    Math::Vec3(-0.5f, 0.5f, -0.5f),  //
    Math::Vec3(-0.5f, -0.5f, 0.5f),  //
    Math::Vec3(0.5f, -0.5f, 0.5f),   //
    Math::Vec3(0.5f, 0.5f, 0.5f),    //
    Math::Vec3(-0.5f, 0.5f, 0.5f)    //
};

constexpr Math::Vec2 CUBE_UV[4u] = { Math::Vec2(1.0f, 1.0f), Math::Vec2(0.0f, 1.0f), Math::Vec2(0.0f, 0.0f),
    Math::Vec2(1.0f, 0.0f) };

constexpr uint16_t CUBE_INDICES[6u * 6u] = {
    0, 3, 1, 3, 2, 1, //
    1, 2, 5, 2, 6, 5, //
    5, 6, 4, 6, 7, 4, //
    4, 7, 0, 7, 3, 0, //
    3, 7, 2, 7, 6, 2, //
    4, 0, 5, 0, 1, 5  //
};

constexpr uint32_t CUBE_UV_INDICES[6u] = { 0, 3, 1, 3, 2, 1 };

constexpr float TWO_PI = Math::PI * 2.0f;

template<typename IndexType>
struct Geometry {
    vector<Math::Vec3>& vertices;
    vector<Math::Vec3>& normals;
    vector<Math::Vec2>& uvs;
    vector<IndexType>& indices;
};

void GenerateCubeGeometry(float width, float height, float depth, Geometry<uint16_t> geometry)
{
    vector<Math::Vec3>& vertices = geometry.vertices;
    vector<Math::Vec3>& normals = geometry.normals;
    vector<Math::Vec2>& uvs = geometry.uvs;
    vector<uint16_t>& indices = geometry.indices;

    vertices.reserve(countof(CUBE_INDICES));
    normals.reserve(countof(CUBE_INDICES));
    uvs.reserve(countof(CUBE_INDICES));
    indices.reserve(countof(CUBE_INDICES));

    constexpr size_t triangleCount = countof(CUBE_INDICES) / 3u;
    for (size_t i = 0; i < triangleCount; ++i) {
        const size_t vertexIndex = i * 3u;

        const Math::Vec3 v0 = CUBE_POS[CUBE_INDICES[vertexIndex + 0u]];
        const Math::Vec3 v1 = CUBE_POS[CUBE_INDICES[vertexIndex + 1u]];
        const Math::Vec3 v2 = CUBE_POS[CUBE_INDICES[vertexIndex + 2u]];

        vertices.emplace_back(v0.x * width, v0.y * height, v0.z * depth);
        vertices.emplace_back(v1.x * width, v1.y * height, v1.z * depth);
        vertices.emplace_back(v2.x * width, v2.y * height, v2.z * depth);

        const Math::Vec3 normal = Math::Normalize(Math::Cross((v1 - v0), (v2 - v0)));
        normals.insert(normals.end(), 3u, normal);

        uvs.emplace_back(
            CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 0u) % 6u]].x, CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 0u) % 6u]].y);
        uvs.emplace_back(
            CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 1u) % 6u]].x, CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 1u) % 6u]].y);
        uvs.emplace_back(
            CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 2u) % 6u]].x, CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 2u) % 6u]].y);
    }

    for (uint16_t i = 0u; i < countof(CUBE_INDICES); ++i) {
        indices.push_back(i);
    }
}

void GenerateSphereGeometry(float radius, uint32_t rings, uint32_t sectors, Geometry<uint32_t> geometry)
{
    vector<Math::Vec3>& vertices = geometry.vertices;
    vector<Math::Vec3>& normals = geometry.normals;
    vector<Math::Vec2>& uvs = geometry.uvs;
    vector<uint32_t>& indices = geometry.indices;

    const size_t maxVertexCount = rings * sectors;
    const size_t maxIndexCount = (rings - 1) * sectors * 6u;

    vertices.reserve(maxVertexCount);
    normals.reserve(maxVertexCount);
    uvs.reserve(maxVertexCount);
    indices.reserve(maxIndexCount);

    const float r = 1.0f / static_cast<float>(rings - 1);
    const float s = 1.0f / static_cast<float>(sectors - 1);

    constexpr float pi = Math::PI;
    constexpr float halfPi = Math::PI * 0.5f;

    for (uint32_t ring = 0; ring < rings; ++ring) {
        const auto ringF = static_cast<float>(ring);
        for (uint32_t sector = 0; sector < sectors; ++sector) {
            const auto sectorF = static_cast<float>(sector);
            const float y = Math::sin(-halfPi + pi * ringF * r);
            const float x = Math::cos(TWO_PI * sectorF * s) * Math::sin(pi * ringF * r);
            const float z = Math::sin(TWO_PI * sectorF * s) * Math::sin(pi * ringF * r);

            vertices.emplace_back(x * radius, y * radius, z * radius);
            normals.emplace_back(x, y, z);
            uvs.emplace_back(sectorF * s, ringF * r);

            if (ring < rings - 1) {
                const uint32_t curRow = ring * sectors;
                const uint32_t nextRow = (ring + 1) * sectors;
                const uint32_t nextS = (sector + 1) % sectors;

                indices.push_back(curRow + sector);
                indices.push_back(nextRow + sector);
                indices.push_back(nextRow + nextS);

                indices.push_back(curRow + sector);
                indices.push_back(nextRow + nextS);
                indices.push_back(curRow + nextS);
            }
        }
    }
}

void GenerateConeCap(
    float radius, float length, uint32_t sectors, Geometry<uint32_t> geometry, const vector<Math::Vec2>& unitCoords)
{
    vector<Math::Vec3>& vertices = geometry.vertices;
    vector<Math::Vec3>& normals = geometry.normals;
    vector<Math::Vec2>& uvs = geometry.uvs;
    vector<uint32_t>& indices = geometry.indices;

    // Already generated vertices: tip + sectors
    uint32_t startVertex = 1U + sectors;

    // Cap center vert.
    const uint32_t bottomIndex = startVertex;
    vertices.emplace_back(0.0f, 0.0f, length);
    normals.emplace_back(0.0f, 0.0f, 1.0f);
    uvs.emplace_back(0.5f, 0.5f);

    ++startVertex;

    // Cap ring and triangles.
    for (uint32_t idx = 0; idx < sectors; ++idx) {
        const uint32_t vertexIndex = startVertex + idx;

        const Math::Vec2& coords = unitCoords[idx];

        vertices.emplace_back(coords.x * radius, coords.y * radius, length);
        normals.emplace_back(0.0f, 0.0f, 1.0f);

        float uvx = (coords.x + 1.0f) * 0.5f;
        float uvy = 1.0f - (coords.y + 1.0f) * 0.5f;
        uvs.emplace_back(uvx, uvy);

        const uint32_t nextVertexIndex = startVertex + ((idx + 1) % sectors);

        indices.push_back(vertexIndex);
        indices.push_back(nextVertexIndex);
        indices.push_back(bottomIndex);
    }
}

void GenerateConeGeometry(float radius, float length, uint32_t sectors, Geometry<uint32_t> geometry)
{
    vector<Math::Vec3>& vertices = geometry.vertices;
    vector<Math::Vec3>& normals = geometry.normals;
    vector<Math::Vec2>& uvs = geometry.uvs;
    vector<uint32_t>& indices = geometry.indices;

    const float s = (sectors > 0U) ? (1.0f / static_cast<float>(sectors)) : 1.0f;

    const size_t maxVertexCount = (2 * static_cast<size_t>(sectors)) + 2u;
    const size_t maxIndexCount = static_cast<size_t>(sectors) * 6u;

    vertices.reserve(maxVertexCount);
    normals.reserve(maxVertexCount);
    uvs.reserve(maxVertexCount);
    indices.reserve(maxIndexCount);

    vector<Math::Vec2> unitCoords;
    unitCoords.reserve(sectors);

    vertices.emplace_back(0.0f, 0.0f, 0.0f);
    normals.emplace_back(0.0f, 0.0f, -1.0f);
    uvs.emplace_back(0.5f, 0.5f);

    // Bottom ring vertices and side triangles, with given radius
    const uint32_t startVertex = 1U;
    for (uint32_t idx = 0; idx < sectors; ++idx) {
        const auto idxF = static_cast<float>(idx);
        const float x = Math::cos(idxF * s * TWO_PI);
        const float y = Math::sin(idxF * s * TWO_PI);
        unitCoords.emplace_back(x, y);

        vertices.emplace_back(x * radius, y * radius, length);
        normals.emplace_back(x, y, 0.f);

        float uvx = (x + 1.0f) * 0.5f;
        float uvy = 1.0f - (y + 1.0f) * 0.5f;
        uvs.emplace_back(uvx, uvy);

        const uint32_t v0 = 0;
        const uint32_t v1 = startVertex + idx;
        if (sectors == 0) {
            return;
        }
        const uint32_t v2 = startVertex + ((idx + 1) % sectors);

        indices.push_back(v0);
        indices.push_back(v2);
        indices.push_back(v1);
    }

    constexpr bool generateCapping = true;
    if constexpr (generateCapping) {
        GenerateConeCap(radius, length, sectors, geometry, unitCoords);
    }
}

vector<Math::Vec3> GenerateTorusSlices(float minorRadius, uint32_t minorSectors, float minorStep)
{
    vector<Math::Vec3> tubeSlice;
    tubeSlice.reserve(minorSectors);
    for (uint32_t tube = 0; tube < minorSectors; tube++) {
        const float minorRadians = static_cast<float>(tube) * minorStep;
        const float x = 0.0f;
        const float y = Math::cos(minorRadians) * minorRadius;
        const float z = Math::sin(minorRadians) * minorRadius;
        tubeSlice.emplace_back(x, y, z);
    }
    return tubeSlice;
}

void GenerateTorusGeometry(
    float majorRadius, float minorRadius, uint32_t majorSectors, uint32_t minorSectors, Geometry<uint32_t> geometry)
{
    vector<Math::Vec3>& vertices = geometry.vertices;
    vector<Math::Vec3>& normals = geometry.normals;
    vector<Math::Vec2>& uvs = geometry.uvs;
    vector<uint32_t>& indices = geometry.indices;

    const float majorStep = TWO_PI / static_cast<float>(majorSectors);
    const float minorStep = TWO_PI / static_cast<float>(minorSectors);

    const size_t maxVertexCount = static_cast<size_t>(majorSectors) * static_cast<size_t>(minorSectors);
    const size_t maxIndexCount = maxVertexCount * 6u;

    vertices.reserve(maxVertexCount);
    normals.reserve(maxVertexCount);
    uvs.reserve(maxVertexCount);
    indices.reserve(maxIndexCount);

    const vector<Math::Vec3> tubeSlice = GenerateTorusSlices(minorRadius, minorSectors, minorStep);

    uint32_t currentVertexIndex = 0;
    for (uint32_t ring = 0; ring < majorSectors; ring++) {
        const float majorRadians = static_cast<float>(ring) * majorStep;
        const auto rotation = Math::AngleAxis(majorRadians, { 0.0f, 1.0f, 0.0f });
        const auto translation = Math::Vec3(0.0f, 0.0f, 1.0f) * majorRadius;

        for (uint32_t vertexIndex = 0; vertexIndex < minorSectors; vertexIndex++) {
            const auto& ringVertex = tubeSlice[vertexIndex];

            const auto tubeCenter = rotation * translation;

            vertices.push_back(rotation * ringVertex + tubeCenter);

            normals.push_back(Math::Normalize(rotation * ringVertex));

            const float minorRadians = static_cast<float>(vertexIndex) * minorStep;
            const float tx = 1.0f - Math::abs(majorRadians / TWO_PI * 2.0f - 1.0f);
            const float ty = 1.0f - Math::abs(minorRadians / TWO_PI * 2.0f - 1.0f);
            uvs.emplace_back(tx, ty);

            const uint32_t i0 = currentVertexIndex;
            const uint32_t i1 = (i0 + 1) % maxVertexCount;
            const uint32_t i2 = (i0 + minorSectors) % maxVertexCount;
            const uint32_t i3 = (i2 + 1) % maxVertexCount;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);

            currentVertexIndex++;
        }
    }
}
} // namespace

template<typename IndexType>
void CalculateTangentImpl(const array_view<const IndexType>& indices, const array_view<const Math::Vec3>& positions,
    const array_view<const Math::Vec3>& normals, const array_view<const Math::Vec2>& uvs,
    array_view<Math::Vec4>& outTangents)
{
    // http://www.terathon.com/code/tangent.html
    vector<Math::Vec3> tan(positions.size(), { 0, 0, 0 });
    vector<Math::Vec3> bitan(positions.size(), { 0, 0, 0 });

    for (size_t i = 0; i < indices.size(); i += 3u) {
        const IndexType aa = indices[i + 0u];
        const IndexType bb = indices[i + 1u];
        const IndexType cc = indices[i + 2u];

        const Math::Vec2& uv1 = uvs[aa];
        const Math::Vec2& uv2 = uvs[bb];
        const Math::Vec2& uv3 = uvs[cc];

        const auto st1 = uv2 - uv1;
        const auto st2 = uv3 - uv1;

        auto d = Math::Cross(st1, st2);
        if (Math::abs(d) < Math::EPSILON) {
            d = Math::EPSILON;
        }
        const float r = 1.0f / d;

        const Math::Vec3& v1 = positions[aa];
        const Math::Vec3& v2 = positions[bb];
        const Math::Vec3& v3 = positions[cc];

        const auto e1 = v2 - v1;
        const auto e2 = v3 - v1;

        const Math::Vec3 sdir { (e1 * st2.y - e2 * st1.y) * r };
        tan[aa] += sdir;
        tan[bb] += sdir;
        tan[cc] += sdir;

        const Math::Vec3 tdir { (e2 * st1.x - e1 * st2.x) * r };

        bitan[aa] += tdir;
        bitan[bb] += tdir;
        bitan[cc] += tdir;
    }

    for (size_t i = 0; i < positions.size(); i++) {
        const Math::Vec3& n = normals[i];
        const Math::Vec3& t = tan[i];

        // Gram-Schmidt orthogonalize
        const Math::Vec3 tmp = Math::Normalize(t - n * Math::Dot(n, t));

        // Calculate handedness
        const float w = (Math::Dot(Math::Cross(n, t), bitan[i]) < 0.0F) ? 1.0F : -1.0F;

        outTangents[i] = Math::Vec4(tmp.x, tmp.y, tmp.z, w);
    }
}

void MeshUtil::CalculateTangents(const array_view<const uint32_t>& indices,
    const array_view<const Math::Vec3>& positions, const array_view<const Math::Vec3>& normals,
    const array_view<const Math::Vec2>& uvs, array_view<Math::Vec4> outTangents)
{
    CalculateTangentImpl<uint32_t>(indices, positions, normals, uvs, outTangents);
}

void MeshUtil::CalculateTangents(const array_view<const uint16_t>& indices,
    const array_view<const Math::Vec3>& positions, const array_view<const Math::Vec3>& normals,
    const array_view<const Math::Vec2>& uvs, array_view<Math::Vec4> outTangents)
{
    CalculateTangentImpl<uint16_t>(indices, positions, normals, uvs, outTangents);
}

void MeshUtil::CalculateTangents(const array_view<const uint8_t>& indices,
    const array_view<const Math::Vec3>& positions, const array_view<const Math::Vec3>& normals,
    const array_view<const Math::Vec2>& uvs, array_view<Math::Vec4> outTangents)
{
    CalculateTangentImpl<uint8_t>(indices, positions, normals, uvs, outTangents);
}

template<typename T>
constexpr inline IMeshBuilder::DataBuffer FillData(array_view<const T> c) noexcept
{
    Format format = BASE_FORMAT_UNDEFINED;
    if constexpr (is_same_v<T, Math::Vec2>) {
        format = BASE_FORMAT_R32G32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec3>) {
        format = BASE_FORMAT_R32G32B32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec4>) {
        format = BASE_FORMAT_R32G32B32A32_SFLOAT;
    } else if constexpr (is_same_v<T, uint16_t>) {
        format = BASE_FORMAT_R16_UINT;
    } else if constexpr (is_same_v<T, uint32_t>) {
        format = BASE_FORMAT_R32_UINT;
    }
    return IMeshBuilder::DataBuffer { format, sizeof(T),
        { reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(T) } };
}

template<typename T, size_t N>
constexpr inline IMeshBuilder::DataBuffer FillData(const T (&c)[N]) noexcept
{
    return FillData(array_view(c, N));
}

template<typename T>
constexpr inline IMeshBuilder::DataBuffer FillData(const vector<T>& c) noexcept
{
    return FillData(array_view<const T> { c });
}

Entity MeshUtil::GeneratePlaneMesh(const IEcs& ecs, const string_view name, Entity material, float width, float depth)
{
    const float extentX = width * 0.5f;
    const float extentZ = depth * 0.5f;

    const Math::Vec3 pos[6u] = {
        Math::Vec3(-extentX, 0.0f, -extentZ),
        Math::Vec3(-extentX, 0.0f, extentZ),
        Math::Vec3(extentX, 0.0f, -extentZ),

        Math::Vec3(-extentX, 0.0f, extentZ),
        Math::Vec3(extentX, 0.0f, extentZ),
        Math::Vec3(extentX, 0.0f, -extentZ),
    };
    vector<Math::Vec4> tangents(countof(pos));
    {
        constexpr auto indicesView = array_view(PLANE_IND);
        const auto positionsView = array_view(pos);
        constexpr auto normalsView = array_view(PLANE_NORM);
        constexpr auto uvsView = array_view(PLANE_UV);

        CalculateTangents(indicesView, positionsView, normalsView, uvsView, tangents);
    }

    IMeshBuilder::Submesh submesh;
    submesh.material = material;
    submesh.vertexCount = 6u;
    submesh.indexCount = 6u;
    submesh.indexType = CORE_INDEX_TYPE_UINT16;
    submesh.tangents = true;

    auto builder = InitializeBuilder(submesh);

    auto positionData = FillData(pos);
    auto normalData = FillData(PLANE_NORM);
    auto uvData = FillData(PLANE_UV);
    auto tangentData = FillData(tangents);
    IMeshBuilder::DataBuffer dummy {};
    builder->SetVertexData(0, positionData, normalData, uvData, dummy, tangentData, dummy);

    builder->CalculateAABB(0, positionData);

    auto indices = FillData(PLANE_IND);
    builder->SetIndexData(0, indices);

    return CreateMesh(ecs, *builder, name);
}

Entity MeshUtil::GenerateSphereMesh(
    const IEcs& ecs, const string_view name, Entity material, float radius, uint32_t rings, uint32_t sectors)
{
    vector<Math::Vec3> vertices;
    vector<Math::Vec3> normals;
    vector<Math::Vec2> uvs;
    vector<uint32_t> indices;
    GenerateSphereGeometry(radius, rings, sectors, { vertices, normals, uvs, indices });

    vector<Math::Vec4> tangents(vertices.size());
    CalculateTangents(indices, vertices, normals, uvs, tangents);

    IMeshBuilder::Submesh submesh;
    submesh.material = material;
    submesh.vertexCount = static_cast<uint32_t>(vertices.size());
    submesh.indexCount = static_cast<uint32_t>(indices.size());
    submesh.indexType = submesh.vertexCount <= UINT16_MAX ? CORE_INDEX_TYPE_UINT16 : CORE_INDEX_TYPE_UINT32;
    submesh.tangents = true;

    auto builder = InitializeBuilder(submesh);

    auto positionData = FillData(vertices);
    auto normalData = FillData(normals);
    auto uvData = FillData(uvs);
    auto tangentData = FillData(tangents);
    IMeshBuilder::DataBuffer dummy {};
    builder->SetVertexData(0, positionData, normalData, uvData, dummy, tangentData, dummy);

    builder->CalculateAABB(0, positionData);

    auto indexData = FillData(indices);
    builder->SetIndexData(0, indexData);

    return CreateMesh(ecs, *builder, name);
}

Entity MeshUtil::GenerateConeMesh(
    const IEcs& ecs, const string_view name, Entity material, float radius, float length, uint32_t sectors)
{
    vector<Math::Vec3> vertices;
    vector<Math::Vec3> normals;
    vector<Math::Vec2> uvs;
    vector<uint32_t> indices;
    GenerateConeGeometry(radius, length, sectors, { vertices, normals, uvs, indices });

    IMeshBuilder::Submesh submesh;
    submesh.material = material;
    submesh.vertexCount = static_cast<uint32_t>(vertices.size());
    submesh.indexCount = static_cast<uint32_t>(indices.size());
    submesh.indexType = submesh.vertexCount <= UINT16_MAX ? CORE_INDEX_TYPE_UINT16 : CORE_INDEX_TYPE_UINT32;
    submesh.tangents = true;

    vector<Math::Vec4> tangents(vertices.size());
    CalculateTangents(indices, vertices, normals, uvs, tangents);

    auto builder = InitializeBuilder(submesh);

    auto positionData = FillData(vertices);
    auto normalData = FillData(normals);
    auto uvData = FillData(uvs);
    auto tangentData = FillData(tangents);
    IMeshBuilder::DataBuffer dummy {};
    builder->SetVertexData(0, positionData, normalData, uvData, dummy, tangentData, dummy);

    builder->CalculateAABB(0, positionData);

    auto indexData = FillData(indices);
    builder->SetIndexData(0, indexData);

    return CreateMesh(ecs, *builder, name);
}

Entity MeshUtil::GenerateTorusMesh(const IEcs& ecs, const string_view name, Entity material, float majorRadius,
    float minorRadius, uint32_t majorSectors, uint32_t minorSectors)
{
    vector<Math::Vec3> vertices;
    vector<Math::Vec3> normals;
    vector<Math::Vec2> uvs;
    vector<uint32_t> indices;
    GenerateTorusGeometry(majorRadius, minorRadius, majorSectors, minorSectors, { vertices, normals, uvs, indices });

    vector<Math::Vec4> tangents(vertices.size());
    CalculateTangents(indices, vertices, normals, uvs, tangents);

    IMeshBuilder::Submesh submesh;
    submesh.material = material;
    submesh.vertexCount = static_cast<uint32_t>(vertices.size());
    submesh.indexCount = static_cast<uint32_t>(indices.size());
    submesh.indexType = submesh.vertexCount <= UINT16_MAX ? CORE_INDEX_TYPE_UINT16 : CORE_INDEX_TYPE_UINT32;
    submesh.tangents = true;

    auto builder = InitializeBuilder(submesh);

    auto positionData = FillData(vertices);
    auto normalData = FillData(normals);
    auto uvData = FillData(uvs);
    auto tangentData = FillData(tangents);
    IMeshBuilder::DataBuffer dummy {};
    builder->SetVertexData(0, positionData, normalData, uvData, dummy, tangentData, dummy);

    builder->CalculateAABB(0, positionData);

    auto indexData = FillData(indices);
    builder->SetIndexData(0, indexData);

    return CreateMesh(ecs, *builder, name);
}

Entity MeshUtil::GenerateCubeMesh(
    const IEcs& ecs, const string_view name, Entity material, float width, float height, float depth)
{
    vector<Math::Vec3> positions;
    vector<Math::Vec3> normals;
    vector<Math::Vec2> uvs;
    vector<uint16_t> indices;
    GenerateCubeGeometry(width, height, depth, { positions, normals, uvs, indices });

    vector<Math::Vec4> tangents(positions.size());
    CalculateTangents(indices, positions, normals, uvs, tangents);

    IMeshBuilder::Submesh submesh;
    submesh.material = material;
    submesh.vertexCount = static_cast<uint32_t>(countof(CUBE_INDICES));
    submesh.indexCount = static_cast<uint32_t>(countof(CUBE_INDICES));
    submesh.indexType = CORE_INDEX_TYPE_UINT16;
    submesh.tangents = true;

    auto builder = InitializeBuilder(submesh);

    auto positionData = FillData(positions);
    auto normalData = FillData(normals);
    auto uvData = FillData(uvs);
    auto tangentData = FillData(tangents);
    IMeshBuilder::DataBuffer dummy {};
    builder->SetVertexData(0, positionData, normalData, uvData, dummy, tangentData, dummy);

    builder->CalculateAABB(0, positionData);

    auto indexData = FillData(indices);
    builder->SetIndexData(0, indexData);

    return CreateMesh(ecs, *builder, name);
}

Entity MeshUtil::GenerateEntity(const IEcs& ecs, const string_view name, Entity meshHandle)
{
    INodeSystem* nodesystem = GetSystem<INodeSystem>(ecs);
    CORE_ASSERT(nodesystem);

    // Create node to scene.
    ISceneNode* node = nodesystem->CreateNode();
    if (!node) {
        return Entity {};
    }

    node->SetName(name);

    // Add render mesh component.
    IRenderMeshComponentManager* renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
    CORE_ASSERT(renderMeshManager);

    RenderMeshComponent component = CreateComponent(*renderMeshManager, node->GetEntity());
    component.mesh = meshHandle;
    renderMeshManager->Set(node->GetEntity(), component);

    return node->GetEntity();
}

Entity MeshUtil::GenerateCube(
    const IEcs& ecs, const string_view name, Entity material, float width, float height, float depth)
{
    const Entity meshHandle = GenerateCubeMesh(ecs, name, material, width, height, depth);
    return GenerateEntity(ecs, name, meshHandle);
}

Entity MeshUtil::GeneratePlane(const IEcs& ecs, const string_view name, Entity material, float width, float depth)
{
    const Entity meshHandle = GeneratePlaneMesh(ecs, name, material, width, depth);
    return GenerateEntity(ecs, name, meshHandle);
}

Entity MeshUtil::GenerateSphere(
    const IEcs& ecs, const string_view name, Entity material, float radius, uint32_t rings, uint32_t sectors)
{
    const Entity meshHandle = GenerateSphereMesh(ecs, name, material, radius, rings, sectors);
    return GenerateEntity(ecs, name, meshHandle);
}

Entity MeshUtil::GenerateCone(
    const IEcs& ecs, const string_view name, Entity material, float radius, float length, uint32_t sectors)
{
    const Entity meshHandle = GenerateConeMesh(ecs, name, material, radius, length, sectors);
    return GenerateEntity(ecs, name, meshHandle);
}

Entity MeshUtil::GenerateTorus(const IEcs& ecs, const string_view name, Entity material, float majorRadius,
    float minorRadius, uint32_t majorSectors, uint32_t minorSectors)
{
    const Entity meshHandle =
        GenerateTorusMesh(ecs, name, material, majorRadius, minorRadius, majorSectors, minorSectors);
    return GenerateEntity(ecs, name, meshHandle);
}

IMeshBuilder::Ptr MeshUtil::InitializeBuilder(const IMeshBuilder::Submesh& submesh) const
{
    IMeshBuilder::Ptr builder;
    if (IClassRegister* classRegister = factory_.GetInterface<IClassRegister>(); classRegister) {
        auto renderContext = GetInstance<IRenderContext>(*classRegister, UID_RENDER_CONTEXT);
        IShaderManager& shaderManager = renderContext->GetDevice().GetShaderManager();
        const VertexInputDeclarationView vertexInputDeclaration =
            shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
                DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));
        builder = CreateInstance<IMeshBuilder>(*renderContext, UID_MESH_BUILDER);
        builder->Initialize(vertexInputDeclaration, 1);

        builder->AddSubmesh(submesh);
        builder->Allocate();
    }

    return builder;
}

Entity MeshUtil::CreateMesh(const IEcs& ecs, const IMeshBuilder& builder, const string_view name) const
{
    auto meshEntity = builder.CreateMesh(const_cast<IEcs&>(ecs));
    if (!name.empty()) {
        GetManager<IUriComponentManager>(ecs)->Set(meshEntity, { string(name) });
        GetManager<INameComponentManager>(ecs)->Set(meshEntity, { string(name) });
    }
    return meshEntity;
}

MeshUtil::MeshUtil(IClassFactory& factory) : factory_(factory) {}
CORE3D_END_NAMESPACE()
