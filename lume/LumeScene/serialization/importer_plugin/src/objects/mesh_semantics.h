/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_OBJECTS_MESH_SEMANTICS_H
#define SCENE_IMP_SRC_OBJECTS_MESH_SEMANTICS_H

#include <scene/interface/mesh_descriptor.h>
#include <scene_importer/base/namespace.h>

SCENE_IMP_BEGIN_NAMESPACE()

/// JSON-side: every mesh-attribute semantic name accepted by the schema-1.0
/// importer. Mirrors the `semantic` enum in mesh.schema.json — keep the two
/// lists in lockstep. New SCENE_NS::MeshSemantic entries must be added here
/// too.
constexpr BASE_NS::string_view ALL_SEMANTICS[] = {
    SCENE_NS::MeshSemantic::POSITION,
    SCENE_NS::MeshSemantic::NORMAL,
    SCENE_NS::MeshSemantic::UV0,
    SCENE_NS::MeshSemantic::UV1,
    SCENE_NS::MeshSemantic::TANGENT,
    SCENE_NS::MeshSemantic::JOINTS,
    SCENE_NS::MeshSemantic::WEIGHTS,
    SCENE_NS::MeshSemantic::COLOR,
};

/// JSON-side names for the SCENE_NS::SubmeshFlag values, used by the importer
/// to map `flags: [...]` array entries to bits.
namespace MeshFlag {
constexpr BASE_NS::string_view TANGENTS = "tangents";
constexpr BASE_NS::string_view VERTEX_COLORS = "vertexColors";
constexpr BASE_NS::string_view SKIN = "skin";
}  // namespace MeshFlag

/// Returns true when `s` is one of the semantic names accepted by the
/// schema-1.0 importer (matches one of ALL_SEMANTICS).
inline bool IsKnownSemantic(BASE_NS::string_view s)
{
    for (auto known : ALL_SEMANTICS) {
        if (known == s) {
            return true;
        }
    }
    return false;
}

SCENE_IMP_END_NAMESPACE()

#endif
