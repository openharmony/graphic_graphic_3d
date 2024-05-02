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

#ifndef SCENEPLUGINAPI_MESH_H
#define SCENEPLUGINAPI_MESH_H

#include <scene_plugin/api/material.h>
#include <scene_plugin/api/mesh_uid.h>
#include <scene_plugin/interface/intf_scene.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

class SubMesh final : public META_NS::Internal::ObjectInterfaceAPI<SubMesh, ClassId::SubMesh> {
    META_API(SubMesh)
    META_API_OBJECT_CONVERTIBLE(ISubMesh)
    META_API_CACHE_INTERFACE(ISubMesh, SubMesh)
public:
    // ToDo: The properties mirror the values from entity system, but setting the values
    // needs to happen using setter methods below (even the properties are not marked
    // ReadOnly
    META_API_INTERFACE_PROPERTY_CACHED(SubMesh, Material, SCENE_NS::IMaterial::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(SubMesh, AABBMin, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(SubMesh, AABBMax, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(SubMesh, RenderSortLayerOrder, uint8_t)

    /**
     * @brief Within a render slot, a layer can define a sort layer order for a submesh.
     * There are 0-63 values available. Default id value is 32.
     * 0 first, 63 last
     * 1. Typical use case is to set render sort layer to objects which render with depth test without depth write.
     * 2. Typical use case is to always render character and/or camera object first to cull large parts of the view.
     * 3. Sort e.g. plane layers.
     * @param index The selected submesh index.
     * @param value The layer order number.
     */
    void SetRenderSortLayerOrder(uint8_t order)
    {
        if (auto submesh = META_API_CACHED_INTERFACE(SubMesh)) {
            submesh->SetRenderSortLayerOrder(order);
        }
    }

    /**
     * @brief Set axis aligned bounding box min to the submesh.
     */
    void SetAABBMin(const BASE_NS::Math::Vec3& min)
    {
        if (auto submesh = META_API_CACHED_INTERFACE(SubMesh)) {
            submesh->SetAABBMin(min);
        }
    }

    /**
     * @brief Set axis aligned bounding box max to the submesh.
     */
    void SetAABBMax(const BASE_NS::Math::Vec3& max)
    {
        if (auto submesh = META_API_CACHED_INTERFACE(SubMesh)) {
            submesh->SetAABBMax(max);
        }
    }

    /**
     * @brief Set material to the submesh.
     */
    void SetMaterial(SCENE_NS::IMaterial::Ptr material)
    {
        if (auto submesh = META_API_CACHED_INTERFACE(SubMesh)) {
            submesh->SetMaterial(material);
        }
    }
};

/**
 * @brief Mesh class wraps IMesh interface. It keeps the referenced object alive using strong ref.
 *        The construction of the object is asynchronous, the properties of the engine may not be available
 *        right after the object instantiation, but OnLoaded() event can be used to observe the state changes.
 *        Mesh object may not implement Node interfaces on Engine side, so container operations and
 *        transform operations are not applicable as such.
 */
class Mesh final : public META_NS::Internal::ObjectInterfaceAPI<Mesh, ClassId::Mesh> {
    META_API(Mesh)
    META_API_OBJECT_CONVERTIBLE(IMesh)
    META_API_CACHE_INTERFACE(IMesh, Mesh)
    META_API_OBJECT_CONVERTIBLE(INode)
    META_API_CACHE_INTERFACE(INode, Node)
public:
    // From Node
    META_API_INTERFACE_PROPERTY_CACHED(Node, Name, BASE_NS::string)
    META_API_INTERFACE_READONLY_ARRAY_PROPERTY_CACHED(Mesh, SubMeshes, SCENE_NS::ISubMesh::Ptr)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Mesh, AABBMin, BASE_NS::Math::Vec3)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Mesh, AABBMax, BASE_NS::Math::Vec3)

public:
    /**
     * @brief Construct Mesh instance from INode strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Mesh(const INode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Construct Mesh instance from IMesh strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    Mesh(const IMesh::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Gets OnLoaded event from INode-interface
     * @return INode::OnLoaded
     */
    auto OnLoaded()
    {
        return META_API_CACHED_INTERFACE(Node)->OnLoaded();
    }

    /**
     * @brief Runs a callback once the mesh is loaded. If mesh is already initialized, callback will not run.
     * @param callback Code to run, if strong reference is passed, it will keep the instance alive
     *                 causing engine to report memory leak on application exit.
     * @return reference to this instance of Mesh.
     */
    template<class Callback>
    auto& OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }

    /**
     * @brief Get the material from the selected submesh. The returned object is constructed asynchronously.
     * @param index The selected submesh index.
     * @return If the given index is valid, an object referencing the selected material.
     */
    Material GetMaterial(size_t index)
    {
        IMaterial::Ptr ret {};
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            ret = mesh->GetMaterial(index);
        }
        return Material(ret);
    }

    /**
     * @brief Set given material for all the submeshes.
     * @material The material to be set.
     */
    void SetMaterial(const IMaterial::Ptr material)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->SetMaterial(-1, material);
        }
    }

    /**
     * @brief Set material for the spesific submesh.
     * @material The material to be set.
     * @param index The selected submesh index.
     */
    void SetMaterial(size_t index, const IMaterial::Ptr material)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->SetMaterial(index, material);
        }
    }

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
    void SetRenderSortLayerOrder(size_t index, uint8_t value)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->SetRenderSortLayerOrder(index, value);
        }
    }

    /**
     * @brief Get render sort layer order for selected submesh
     * @return The layer order for selected index, if the index is not present, returns 0u.
     */
    uint8_t GetRenderSortLayerOrder(size_t index) const
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            return mesh->GetRenderSortLayerOrder(index);
        }
        return 0u;
    }

    /**
     * @brief Update mesh data from the arrays. 16 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    void UpdateMeshFromArraysI16(MeshGeometryArrayPtr<uint16_t> arrays)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->UpdateMeshFromArraysI16(arrays);
        }
    }

    /**
     * @brief Update mesh data from the arrays. 32 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    void UpdateMeshFromArraysI32(MeshGeometryArrayPtr<uint32_t> arrays)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->UpdateMeshFromArraysI32(arrays);
        }
    }

    /**
     * @brief Add submesh data from the arrays. 16 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    void AddSubmeshesFromArrayI16(MeshGeometryArrayPtr<uint16_t> arrays)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->AddSubmeshesFromArrayI16(arrays);
        }
    }

    /**
     * @brief Add submesh data from the arrays. 32 bit indices.
     * @param arrays defining the mesh, see MeshGeometryArray.
     */
    void AddSubmeshesFromArraysI32(MeshGeometryArrayPtr<uint32_t> arrays)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->AddSubmeshesFromArraysI32(arrays);
        }
    }

    /**
     * @brief Copy the contents of the submesh into this mesh instance.
     */
    void CloneSubmesh(ISubMesh::Ptr submesh)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->CloneSubmesh(submesh);
        }
    }

    /**
     * @brief Remove the submesh from this mesh instance.
     */
    void RemoveSubMesh(size_t index)
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->RemoveSubMesh(index);
        }
    }

    /**
     * @brief Remove all the submeshes from this mesh instance.
     */
    void RemoveAllSubmeshes()
    {
        if (auto mesh = META_API_CACHED_INTERFACE(Mesh)) {
            mesh->RemoveAllSubmeshes();
        }
    }
};

class MultiMeshProxy final : public META_NS::Internal::ObjectInterfaceAPI<MultiMeshProxy, ClassId::MultiMeshProxy> {
    META_API(MultiMeshProxy)
    META_API_OBJECT_CONVERTIBLE(IMultiMeshProxy)
    META_API_CACHE_INTERFACE(IMultiMeshProxy, MultiMeshProxy)
public:
    META_API_INTERFACE_PROPERTY_CACHED(MultiMeshProxy, MaterialOverride, SCENE_NS::IMaterial::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(MultiMeshProxy, Mesh, SCENE_NS::IMesh::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(MultiMeshProxy, VisibleInstanceCount, size_t)
    META_API_INTERFACE_ARRAY_PROPERTY_CACHED(MultiMeshProxy, CustomData, BASE_NS::Math::Vec4)
    META_API_INTERFACE_ARRAY_PROPERTY_CACHED(MultiMeshProxy, Transforms, BASE_NS::Math::Mat4X4)

    /**
     * @brief Set the capacity of the proxy and reset properties to the whole batch
     */
    void SetInstanceCount(size_t count)
    {
        if (auto mmesh = META_API_CACHED_INTERFACE(MultiMeshProxy)) {
            mmesh->SetInstanceCount(count);
        }
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_MESH_H
