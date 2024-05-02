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

#include "asset_migration.h"

#include <3d/ecs/components/animation_output_component.h>
#include <core/property/property_types.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

SCENE_BEGIN_NAMESPACE()

namespace {
struct Conversion {
    PropertyTypeDecl oldVersion;
    PropertyTypeDecl newVersion;
};

// Add all datatype conversions here.
constexpr Conversion conversions[] = {

    { PROPERTYTYPE(Math::IVec2), PropertyType::IVEC2_T }, // Math::IVec2 to BASE_NS::Math::IVec2
    { PROPERTYTYPE(Math::IVec3), PropertyType::IVEC3_T }, // Math::IVec3 to BASE_NS::Math::IVec3
    { PROPERTYTYPE(Math::IVec4), PropertyType::IVEC4_T }, // Math::IVec4 to BASE_NS::Math::IVec4

    { PROPERTYTYPE(Math::UVec2), PropertyType::UVEC2_T }, // Math::UVec2 to BASE_NS::Math::UVec2
    { PROPERTYTYPE(Math::UVec3), PropertyType::UVEC3_T }, // Math::UVec3 to BASE_NS::Math::UVec3
    { PROPERTYTYPE(Math::UVec4), PropertyType::UVEC4_T }, // Math::UVec4 to BASE_NS::Math::UVec4

    { PROPERTYTYPE(Math::Vec2), PropertyType::VEC2_T }, // Math::Vec2 to BASE_NS::Math::Vec2
    { PROPERTYTYPE(Math::Vec3), PropertyType::VEC3_T }, // Math::Vec3 to BASE_NS::Math::Vec3
    { PROPERTYTYPE(Math::Vec4), PropertyType::VEC4_T }, // Math::Vec4 to BASE_NS::Math::Vec4

    { PROPERTYTYPE(Math::Quat), PropertyType::QUAT_T },     // Math::Quat to BASE_NS::Math::Quat
    { PROPERTYTYPE(Math::Mat3X3), PropertyType::MAT3X3_T }, // Math::Mat3X3 to BASE_NS::Math::Mat3X3
    { PROPERTYTYPE(Math::Mat4X4), PropertyType::MAT4X4_T }, // Math::Mat4X4 to BASE_NS::Math::Mat4X4

    { PROPERTYTYPE(Uid), PropertyType::UID_T },       // Uid to BASE_NS::Uid
    { PROPERTYTYPE(string), PropertyType::STRING_T }, // string to BASE_NS::string

    // Array types.
    { PROPERTYTYPE_ARRAY(Math::IVec2), PropertyType::IVEC2_ARRAY_T }, // Math::IVec2 to BASE_NS::Math::IVec2
    { PROPERTYTYPE_ARRAY(Math::IVec3), PropertyType::IVEC3_ARRAY_T }, // Math::IVec3 to BASE_NS::Math::IVec3
    { PROPERTYTYPE_ARRAY(Math::IVec4), PropertyType::IVEC4_ARRAY_T }, // Math::IVec4 to BASE_NS::Math::IVec4

    { PROPERTYTYPE_ARRAY(Math::UVec2), PropertyType::UVEC2_ARRAY_T }, // Math::UVec2 to BASE_NS::Math::UVec2
    { PROPERTYTYPE_ARRAY(Math::UVec3), PropertyType::UVEC3_ARRAY_T }, // Math::UVec3 to BASE_NS::Math::UVec3
    { PROPERTYTYPE_ARRAY(Math::UVec4), PropertyType::UVEC4_ARRAY_T }, // Math::UVec4 to BASE_NS::Math::UVec4

    { PROPERTYTYPE_ARRAY(Math::Vec2), PropertyType::VEC2_ARRAY_T }, // Math::Vec2 to BASE_NS::Math::Vec2
    { PROPERTYTYPE_ARRAY(Math::Vec3), PropertyType::VEC3_ARRAY_T }, // Math::Vec3 to BASE_NS::Math::Vec3
    { PROPERTYTYPE_ARRAY(Math::Vec4), PropertyType::VEC4_ARRAY_T }, // Math::Vec4 to BASE_NS::Math::Vec4

    { PROPERTYTYPE_ARRAY(Math::Quat), PropertyType::QUAT_ARRAY_T },     // Math::Quat to BASE_NS::Math::Quat
    { PROPERTYTYPE_ARRAY(Math::Mat3X3), PropertyType::MAT3X3_ARRAY_T }, // Math::Mat3X3 to BASE_NS::Math::Mat3X3
    { PROPERTYTYPE_ARRAY(Math::Mat4X4), PropertyType::MAT4X4_ARRAY_T }, // Math::Mat4X4 to BASE_NS::Math::Mat4X4

    { PROPERTYTYPE_ARRAY(Uid), PropertyType::UID_ARRAY_T }, // Uid to BASE_NS::Uid

    { PROPERTYTYPE(vector<float>), PropertyType::FLOAT_VECTOR_T },
    { PROPERTYTYPE(vector<Math::Mat4X4>), PropertyType::MAT4X4_VECTOR_T },
    { PROPERTYTYPE(vector<EntityReference>), PropertyType::ENTITY_REFERENCE_VECTOR_T }

};

size_t NUMBER_OF_CONVERSIONS = sizeof(conversions) / sizeof(conversions[0]);
} // namespace

void MigrateAnimation(IEntityCollection& collection)
{
    auto* aocm = GetManager<IAnimationOutputComponentManager>(collection.GetEcs());
    if (aocm) {
        vector<EntityReference> entities;
        collection.GetEntitiesRecursive(false, entities);
        for (const auto& entity : entities) {
            if (aocm->HasComponent(entity)) {
                if (auto handle = aocm->Write(entity); handle) {
                    for (auto i = 0; i < NUMBER_OF_CONVERSIONS; ++i) {
                        if (handle->type == conversions[i].oldVersion.typeHash) {
                            handle->type = conversions[i].newVersion.typeHash;
                            break;
                        }
                    }
                }
            }
        }
    }
}

SCENE_END_NAMESPACE()
