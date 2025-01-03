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

#ifndef SCENE_INTERFACE_IMESH_H
#define SCENE_INTERFACE_IMESH_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>
#include <scene/interface/intf_material.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class ISubMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISubMesh, "89060424-5034-43e2-af6c-a7554b90c107")
public:
    META_PROPERTY(IMaterial::Ptr, Material)
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
};

class IMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMesh, "aab724a6-312d-4872-9ff2-b7bed8571b44")
public:
    /**
     * @brief Allows to override all submesh materials with a single material.
     * @return Material that is used to override all submesh materials, if set.
     */
    META_PROPERTY(SCENE_NS::IMaterial::Ptr, MaterialOverride)
    /**
     * @brief Axis aligned bounding box min. Calculated using all submeshes.
     * @return vector defining the min point.
     */
    META_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    /**
     * @brief Axis aligned bounding box max. Calculated using all submeshes.
     * @return vector defining the max point.
     */
    META_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMax)

    virtual Future<BASE_NS::vector<ISubMesh::Ptr>> GetSubmeshes() const = 0;
    virtual Future<bool> SetSubmeshes(const BASE_NS::vector<ISubMesh::Ptr>&) = 0;
};

class IMeshAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMeshAccess, "63703b44-c1f5-437e-9d57-4248d1b69311")
public:
    virtual Future<bool> SetMesh(const IMesh::Ptr&) = 0;
    virtual Future<IMesh::Ptr> GetMesh() const = 0;
};
META_REGISTER_CLASS(MeshNode, "e0ad59e3-ef66-4b74-8a40-59b1584f14dd", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(SubMesh, "7f48b697-9fba-43da-8b15-f61fbbf2a925", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(Mesh, "8478fc2b-13fe-4b59-963a-370c04a94d15", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ISubMesh)
META_INTERFACE_TYPE(SCENE_NS::IMesh)

#endif
