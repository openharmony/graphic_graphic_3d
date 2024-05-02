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
#ifndef SCENEPLUGIN_INTF_MESH_H
#define SCENEPLUGIN_INTF_MESH_H

#include <scene_plugin/interface/intf_material.h>
#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/mesh_arrays.h>

#include <base/containers/vector.h>

#include <meta/api/animation/animation.h>
#include <meta/base/types.h>
#include <meta/interface/intf_container.h>

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::SubMesh
REGISTER_INTERFACE(ISubMesh, "b203cb19-923a-44a1-aa9b-17854721f90e")
class ISubMesh : public META_NS::INamed {
    META_INTERFACE(META_NS::INamed, ISubMesh, InterfaceId::ISubMesh)
public:
    META_PROPERTY(SCENE_NS::IMaterial::Ptr, Material)
    META_PROPERTY(uint64_t, Handle)
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
    META_PROPERTY(uint8_t, RenderSortLayerOrder)
    META_READONLY_PROPERTY(BASE_NS::string, MaterialUri)

    virtual void SetRenderSortLayerOrder(uint8_t order) = 0;
    virtual void SetAABBMin(BASE_NS::Math::Vec3) = 0;
    virtual void SetAABBMax(BASE_NS::Math::Vec3) = 0;
    virtual void SetMaterial(SCENE_NS::IMaterial::Ptr) = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ISubMesh::WeakPtr);
META_TYPE(SCENE_NS::ISubMesh::Ptr);

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::Mesh
REGISTER_INTERFACE(IMesh, "d66a1419-a79f-4e80-a133-31aee86da8bd")
class IMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMesh, InterfaceId::IMesh)
public:
    /**
     * @brief Allows to override all submesh materials with a single material.
     * @return Material that is used to override all submesh materials, if set.
     */
    META_PROPERTY(SCENE_NS::IMaterial::Ptr, MaterialOverride)

    /**
     * @brief Allows to read the uri of the override material, if set.
     * @return Uri of the override material, if iset.
     */
    META_READONLY_PROPERTY(BASE_NS::string, MaterialOverrideUri)

    /**
     * @brief Get list of all submeshes attached to this mesh.
     * @return Array of submeshes.
     */
    META_READONLY_ARRAY_PROPERTY(ISubMesh::Ptr, SubMeshes)

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

    /**
     * @brief Get material handle from submesh, async
     * @param index The selected submesh index.
     * @return pointer to material.
     */
    virtual IMaterial::Ptr GetMaterial(size_t index) = 0;

    /**
     * @brief Set given material for all the submeshes.
     */
    virtual void SetMaterial(const IMaterial::Ptr material) = 0;

    /**
     * @brief Set material for the selected submesh.
     * @param index The selected submesh index.
     * @param material The material to set.
     */
    virtual void SetMaterial(size_t index, const IMaterial::Ptr& material) = 0;

    /**
     * @brief Get render sort layer order for selected submesh
     * @return The layer order for selected index, if the index is not present, returns 0u.
     */
    virtual uint8_t GetRenderSortLayerOrder(size_t index) const = 0;

    /**
     * @brief Within a render slot, a layer can define a sort layer order for a submesh.
     * There are 0-63 values available. Default id value is 32.
     * 0 first, 63 last
     * 1. Typical use case is to set render sort layer to objects which render with depth test without depth write.
     * 2. Typical use case is to always render character and/or camera object first to cull large parts of the view.
     * 3. Sort e.g. plane layers.     * @param index The selected submesh index.
     * @param index The selected submesh index.
     * @param value The layer order number.
     */
    virtual void SetRenderSortLayerOrder(size_t index, uint8_t value) = 0;

    /**
     * @brief Update mesh data from the arrays. 16 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    virtual void UpdateMeshFromArraysI16(MeshGeometryArrayPtr<uint16_t> arrays) = 0;

    /**
     * @brief Update mesh data from the arrays. 32 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    virtual void UpdateMeshFromArraysI32(MeshGeometryArrayPtr<uint32_t> arrays) = 0;

    /**
     * @brief Add submesh data from the arrays. 16 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    virtual void AddSubmeshesFromArrayI16(MeshGeometryArrayPtr<uint16_t> arrays) = 0;

    /**
     * @brief Add submesh data from the arrays. 32 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    virtual void AddSubmeshesFromArraysI32(MeshGeometryArrayPtr<uint32_t> arrays) = 0;

    virtual void CloneSubmesh(ISubMesh::Ptr submesh) = 0;
    virtual void RemoveSubMesh(size_t index) = 0;
    virtual void RemoveAllSubmeshes() = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IMesh::WeakPtr);
META_TYPE(SCENE_NS::IMesh::Ptr);

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::MultiMeshProxy
REGISTER_INTERFACE(IMultiMeshProxy, "3a2f5b77-0778-4a67-a41f-4a54d9a4261d")
class IMultiMeshProxy : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMultiMeshProxy, InterfaceId::IMultiMeshProxy)
public:
    /**
     * @brief Allows to override all submesh materials with a single material.
     * @return Material that is used to override all submesh materials, if set.
     */
    META_PROPERTY(SCENE_NS::IMaterial::Ptr, MaterialOverride)

    /**
     * @brief Allows to read the uri of the override material, if set.
     * @return Uri of the override material, if iset.
     */
    // META_READONLY_PROPERTY(BASE_NS::string, MaterialOverrideUri)

    /**
     * @brief The mesh attached to the batch
     * @return Pointer to property
     */
    META_PROPERTY(IMesh::Ptr, Mesh)

    /**
     * @brief The amount of visible instances
     * @return Pointer to property
     */
    META_PROPERTY(size_t, VisibleInstanceCount)

    /**
     * @brief The custom data for the batch
     * @return Pointer to property
     */
    META_ARRAY_PROPERTY(BASE_NS::Math::Vec4, CustomData)

    /**
     * @brief The transformations for batch
     * @return Pointer to property
     */
    META_ARRAY_PROPERTY(BASE_NS::Math::Mat4X4, Transforms)

    virtual void SetInstanceCount(size_t count) = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IMultiMeshProxy::WeakPtr);
META_TYPE(SCENE_NS::IMultiMeshProxy::Ptr);

#endif // SCENEPLUGIN_INTF_MESH_H
