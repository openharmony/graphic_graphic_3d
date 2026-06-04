/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_MESH_DESCRIPTOR_H
#define SCENE_INTERFACE_MESH_DESCRIPTOR_H

#include <scene/base/types.h>
#include <scene/interface/intf_material.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/formats.h>
#include <core/resources/intf_resource.h>
#include <render/device/intf_shader_pipeline_binder.h>
#include <render/device/pipeline_state_desc.h>

SCENE_BEGIN_NAMESPACE()

/// Engine-side canonical names for vertex-attribute semantics. Each name is
/// stored on the matching `VertexAttribute::semantic`; the mesh builder uses
/// the name to assign each attribute to the corresponding pipeline slot.
namespace MeshSemantic {
constexpr BASE_NS::string_view POSITION = "position";
constexpr BASE_NS::string_view NORMAL = "normal";
constexpr BASE_NS::string_view UV0 = "uv0";
constexpr BASE_NS::string_view UV1 = "uv1";
constexpr BASE_NS::string_view TANGENT = "tangent";
constexpr BASE_NS::string_view JOINTS = "joints";
constexpr BASE_NS::string_view WEIGHTS = "weights";
constexpr BASE_NS::string_view COLOR = "color";
}  // namespace MeshSemantic

/// Submesh feature flags.
enum class SubmeshFlag : uint32_t {
    NONE = 0,
    TANGENTS_BIT = 1u << 0,
    VERTEX_COLORS_BIT = 1u << 1,
    SKIN_BIT = 1u << 2,
};

inline SubmeshFlag operator|(SubmeshFlag a, SubmeshFlag b)
{
    return static_cast<SubmeshFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline SubmeshFlag operator&(SubmeshFlag a, SubmeshFlag b)
{
    return static_cast<SubmeshFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool Any(SubmeshFlag a)
{
    return static_cast<uint32_t>(a) != 0u;
}

/// A byte range within the binary buffer. Used directly for index regions and
/// extended (with a `semantic`) for vertex-attribute regions.
struct BufferRegion {
    uint64_t offset{0};
    uint64_t size{0};
};

/// Vertex-attribute buffer region within the binary data. `semantic` matches
/// one of the mesh's `VertexAttribute::semantic` values to identify which
/// attribute the region holds data for.
struct VertexBufferRegion : BufferRegion {
    BASE_NS::string semantic;
};

/// Vertex attribute description.
struct VertexAttribute {
    BASE_NS::string semantic;
    BASE_NS::Format format{BASE_NS::BASE_FORMAT_UNDEFINED};
    uint32_t stride{0};
};

/// Axis-aligned bounding box (object space).
struct Aabb {
    BASE_NS::Math::Vec3 min{0.f, 0.f, 0.f};
    BASE_NS::Math::Vec3 max{0.f, 0.f, 0.f};
};

/// Submesh descriptor.
struct SubmeshDesc {
    uint32_t vertexCount{0};
    uint32_t indexCount{0};
    RENDER_NS::IndexType indexType{RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT32};
    /// Primitive topology. `MAX_ENUM` (the default) leaves the choice to the
    /// engine mesh builder, which uses `TRIANGLE_LIST`.
    RENDER_NS::PrimitiveTopology topology{RENDER_NS::PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_MAX_ENUM};
    SubmeshFlag flags{SubmeshFlag::NONE};

    BASE_NS::vector<VertexBufferRegion> buffers;
    BufferRegion indexBuffer;

    Aabb aabb;

    /// Material resource ID for this submesh.
    CORE_NS::ResourceId materialId;

    /// Per-submesh render-sort override. Currently parsed by the importer but
    /// not yet applied to the live submesh — ISubMesh has no RenderSort
    /// property in the current engine surface. Wired up so the data path is
    /// ready once that surface exists.
    RenderSort renderSort;
};

SCENE_END_NAMESPACE()

#endif
