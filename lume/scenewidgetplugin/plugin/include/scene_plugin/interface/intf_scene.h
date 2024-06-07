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
#ifndef SCENEPLUGIN_INTF_SCENE_H
#define SCENEPLUGIN_INTF_SCENE_H

#include <scene_plugin/interface/intf_camera.h>
#include <scene_plugin/interface/intf_ecs_animation.h>
#include <scene_plugin/interface/intf_ecs_object.h>
#include <scene_plugin/interface/intf_material.h>
#include <scene_plugin/interface/intf_mesh.h>
#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/intf_render_configuration.h>
#include <scene_plugin/interface/mesh_arrays.h>

#include <core/ecs/intf_ecs.h>

#include <meta/api/animation/animation.h>
#include <meta/base/types.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Scene interface, implemented by SCENE_NS::ClassId::Scene
 */
REGISTER_INTERFACE(IScene, "49df31df-eea6-4d61-a8f1-46aa1f516a18")
class IScene : public META_NS::INamed {
    META_INTERFACE(META_NS::INamed, IScene, InterfaceId::IScene)
public:
    /**
     * @brief The SCENE_STATE enum
     */
    enum SCENE_STATE {
        SCENE_STATUS_UNINITIALIZED = 0,
        SCENE_STATUS_LOADING = 1,
        SCENE_STATUS_READY = 2,
        SCENE_STATUS_LOADING_FAILED = 3,
    };

    /**
     * @brief Status of the scene. Reflects to main scene file, not prefabs and such.
     * @return Pointer to the property.
     */
    META_READONLY_PROPERTY(uint32_t, Status)

    /**
     * @brief Root node of 3D scene. Populated asynchronously when scene loads.
     * @return Pointer to the property.
     */
    META_READONLY_PROPERTY(INode::Ptr, RootNode)

    /**
     * @brief Default camera of the scene. Populated asynchronously when scene loads.
     * @return Pointer to the property.
     */
    META_PROPERTY(ICamera::Ptr, DefaultCamera)

    /**
     * @brief Scene Uri, refers to the main scene file.
     * @return Pointer to the property.
     */
    META_PROPERTY(BASE_NS::string, Uri)

    /**
     * @brief Render configuration.
     * @return
     */
    META_PROPERTY(IRenderConfiguration::Ptr, RenderConfiguration)

    /**
     * @brief Operation mode. If set true, all communication towards scene takes place through task queues.
     * In practice this means that when ever the new elements are introduced on the scene, yield of some sort
     * needs to take place in order to scene set up the bindings. Multithreading requires this to be on.
     * Changing the operation mode is possible only before the main scene is loaded, otherwise the behavior is undefined
     * @return Pointer to the property.
     */
    META_PROPERTY(bool, Asynchronous)

    /**
     * @brief Access to all user-created materials in the scene.
     * @return Pointer to the property.
     */
    META_ARRAY_PROPERTY(IMaterial::Ptr, Materials)

    /**
     * @brief Event which is invoked once the scene has been successfully loaded.
     * @return Event that is used to add handlers for the scene loading.
     */
    META_EVENT(META_NS::IOnChanged, OnLoaded)
            

    /**
     * @brief Create an empty scene.
     */
    virtual void CreateEmpty() = 0;

    /**
     * @brief Load the scene from a file. It is possible to extend the scene after it has been loaded.
     * @param file The file defining the scene.
     * @return Synchronous reply if the loading was started. Use OnLoaded event and Status property for better control.
     */
    virtual bool Load(const BASE_NS::string_view file) = 0;

    /**
     * @brief Returns a node from the scene with a given path (e.g. something like "view/camera_1"). Uses flat cache,
     * but creates a new node if the previous instance does not exist.
     * @param path Path defining the node on 3d scene.
     * @param classId Optional, if the creation type of the node instance is known, it should be provided. Otherwise the
     * system tries to guess it.
     * @return Returns always a node. If the respective 3d element does not exists yet, holds the values but does not
     * post OnLoaded event.
     */
    virtual INode::Ptr GetNode(const BASE_NS::string_view path, BASE_NS::Uid classId = META_NS::IObject::UID,
        INode::BuildBehavior buildBehavior = INode::BuildBehavior::NODE_BUILD_CHILDREN_GRADUAL) = 0;

    /**
     * @brief Add material to scene, this makes scene to take ownership of the material.
     * @param material Material instance
     */
    virtual void AddMaterial(IMaterial::Ptr) = 0;

    /**
     * @brief Remove material from scene, after this scene no longer owns the material.
     * @param material Material instance
     */
    virtual void RemoveMaterial(IMaterial::Ptr) = 0;

    /**
     * @brief Returns all materials from the scene.
     * @return Returns a vector of material objects.
     */
    virtual BASE_NS::vector<IMaterial::Ptr> GetMaterials() const = 0;

    /**
     * @brief Returns a material from the scene with a given name. Materials are typically nodeless elements, and thus
     * the path is flat. Uses flat cache, but creates a new material if the previous instance does not exist.
     * @param name Name of the material entity.
     * @return Returns always an instance. If the respective material does not exists yet, holds the set values but does
     * not complete OnLoaded.
     */
    virtual IMaterial::Ptr GetMaterial(const BASE_NS::string_view name) = 0;

    /**
     * @brief Loads a material from uri.
     * @param uri Uri to the material file.
     * @return Deserializes material from URI and returns the material instance if successful.
     */
    virtual IMaterial::Ptr LoadMaterial(const BASE_NS::string_view uri) = 0;

    /**
     * @brief Returns all meshes from the scene.
     * @return Returns a vector of mesh objects.
     */
    virtual BASE_NS::vector<IMesh::Ptr> GetMeshes() const = 0;

    /**
     * @brief Returns a mesh from the scene with a given name. Meshes are typically nodeless elements, and thus
     * the path is flat. Uses flat cache, but creates a new mesh if the previous instance does not exist.
     * @param name Name of the mesh entity.
     * @return Returns always an instance. If the respective mesh does not exists yet, holds the set values but does
     * not complete OnLoaded.
     */
    virtual IMesh::Ptr GetMesh(const BASE_NS::string_view name) = 0;

    /**
     * @brief Returns a new mesh constructed from the arrays. 16 bit indices.
     * @param name of the mesh instance to be created. Should be unique.
     * @param arrays defining the mesh, see MeshGeometryArray.
     * @return Returns always an instance. if the creation fails, the engine side may not be accessible.
     */
    virtual IMesh::Ptr CreateMeshFromArraysI16(
        const BASE_NS::string_view name, MeshGeometryArrayPtr<uint16_t> arrays) = 0;

    /**
     * @brief Returns a new mesh constructed from the arrays. 32 bit indices.
     * @param name of the mesh instance to be created. Should be unique.
     * @param arrays defining the mesh, see MeshGeometryArray.
     * @return Returns always an instance. if the creation fails, the engine side may not be accessible.
     */
    virtual IMesh::Ptr CreateMeshFromArraysI32(
        const BASE_NS::string_view name, MeshGeometryArrayPtr<uint32_t> arrays) = 0;

    /**
     * @brief Create an empty node.
     * @param path Path to which add or find the engine component.
     * @param createEngineObject The boolean defining whether the 3d element should exist or be created by us.
     * @param classId The type of the node implementation, should be provided if known.
     * @return Newly created instance.
     */
    virtual INode::Ptr CreateNode(const BASE_NS::string_view path, bool createEngineObject = true,
        META_NS::ObjectId classId = META_NS::IObject::UID,
        INode::BuildBehavior buildBehavior = INode::BuildBehavior::NODE_BUILD_CHILDREN_GRADUAL) = 0;

    /**
     * @brief Release INode reference from flat cache. It is possible that someone else will hold a reference and the
     * resource is not freed.
     * @param name or path of the node.
     * @return The strong pointer to the resource that was removed from cache.
     */
    virtual INode::Ptr ReleaseNode(const BASE_NS::string_view name) = 0;

    /**
     * @brief Release INode reference from flat cache. caller will still own the reference.
     * @param node to be released.
     */
    virtual void ReleaseNode(const INode::Ptr& node) = 0;

    /**
     * @brief Release IMaterial reference from flat cache. It is possible that someone else will hold a reference and
     * the resource is not freed.
     * @param name or path of the material.
     * @return The strong pointer to the resource that was removed from cache.
     */
    virtual IMaterial::Ptr ReleaseMaterial(const BASE_NS::string_view name) = 0;

    /**
     * @brief Release IMesh reference from flat cache. It is possible that someone else will hold a reference and
     * the resource is not freed.
     * @param name or path of the mesh.
     * @return The strong pointer to the resource that was removed from cache.
     */
    virtual IMesh::Ptr ReleaseMesh(const BASE_NS::string_view name) = 0;

    /**
     * @brief Release IAnimation reference from flat cache. It is possible that someone else will hold a reference and
     * the resource is not freed.
     * @param name or path of the animation.
     * @return The strong pointer to the resource that was removed from cache.
     */
    virtual META_NS::IAnimation::Ptr ReleaseAnimation(const BASE_NS::string_view name) = 0;

    /**
     * @brief Returns all animations from the scene.
     * @return Returns a vector of animation objects.
     */
    virtual BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations() = 0;

    /**
     * @brief Returns the ecs animation from the scene with the given path.
     * @param name The name of the animation.
     * @return The strong pointer to an animation.
     */
    virtual META_NS::IAnimation::Ptr GetAnimation(const BASE_NS::string_view name) = 0;

    /**
     * @brief Controls the 3d scene System Graph Uri. Changing the value resets the loaded scene completely.
     * @return Pointer to the property.
     */
    META_PROPERTY(BASE_NS::string, SystemGraphUri)

    static constexpr BASE_NS::string_view MATERIALS_PREFIX { "materials" };
    static constexpr BASE_NS::string_view MESHES_PREFIX { "meshes" };
    static constexpr BASE_NS::string_view ANIMATIONS_PREFIX { "animations" };

    /**
     * @brief Update INode cache reference when node is moved to another parent or renamed.
     * @param node that has changed.
     */
    virtual void UpdateCachedReference(const INode::Ptr& node) = 0;

    virtual void UpdateCachedNodePath(const SCENE_NS::INode::Ptr& node) = 0;
    virtual void SetCacheEnabled(const SCENE_NS::INode::Ptr& node, bool enabled) = 0;

    /**
     * @brief Helper for returning a node with a given type.
     * @param path The path of the 3D element.
     * @return Returns always an instance. Use INode::Status and INode::OnLoaded to check the status of the node before
     * reading the 3d values.
     */
    template<class T>
    typename T::Ptr GetNode(const BASE_NS::string_view path)
    {
        const auto uid = T::UID;
        return interface_pointer_cast<T>(GetNode(path, uid));
    }

    /**
     * @brief Create an empty node.
     * @param path Path to which add or find the engine component.
     * @param createEngineObject The boolean defining whether the 3d element should exist or be created by us.
     * @return Newly created instance.
     */
    template<class T>
    typename T::Ptr CreateNode(const BASE_NS::string_view path, bool createEngineObject = true)
    {
        const auto uid = T::UID;
        return interface_pointer_cast<T>(CreateNode(path, createEngineObject, uid));
    }

    /**
     * @brief Create an empty material instance.
     * @param name Name of the new material.
     * @return Newly created instance.
     */
    IMaterial::Ptr CreateMaterial(const BASE_NS::string_view name)
    {
        return CreateNode<IMaterial>(name);
    }

    // Internals, to be forked to some other interface

    /** This can be used to access the scene default camera without known handle*/
    static constexpr uint64_t DEFAULT_CAMERA { 0 };

    /**
     * @brief Get the latest bitmap from the engine. If the bitmap for the camera handle does not exist, it will be
     * created.
     * @param notifyFrameDrawn Tell engine that a frame was consumed.
     * @param cameraHandle Specify if we are interested on some specific camera.
     * @return Pointer to bitmap
     */
    virtual SCENE_NS::IBitmap::Ptr GetBitmap(bool notifyFrameDrawn, const ICamera::Ptr& camera) = 0;

    /**
     * @brief Set bitmap for the engine updates.
     * @param bitmap
     * @param cameraHandle Specify if we are interested on some specific camera.
     */
    virtual void SetBitmap(const SCENE_NS::IBitmap::Ptr& bitmap, const ICamera::Ptr& camera) = 0;

    /**
     * @brief Set Render Size for target bitmap in pixels. Subject of redesign to use the property in ICamera instead.
     * @param width The width of the image.
     * @param height The height of the image.
     * @param cameraHandle Specify if we are interested on some specific camera, 0 is system default.
     */
    virtual void SetRenderSize(uint32_t width, uint32_t height, const ICamera::Ptr& camera) = 0;

    /**
     *  Load all material proxies on the current scene.
     */
    virtual void InstantiateMaterialProxies() = 0;

    /**
     *  Load all material proxies on the current scene.
     */
    virtual void InstantiateMeshProxies() = 0;

    /** Calculates a world space AABB from local min max values. */
    virtual IPickingResult::Ptr GetWorldAABB(
        const BASE_NS::Math::Mat4X4& world, const BASE_NS::Math::Vec3& aabbMin, const BASE_NS::Math::Vec3& aabbMax) = 0;

    /**
     * Get all nodes hit by ray.
     * @param start Starting point of the ray.
     * @param direction Direction of the ray.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual IRayCastResult::Ptr RayCast(const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction) = 0;

    /**
     * Get nodes hit by ray. Only entities included in the given layer mask are in the result. Entities without
     * LayerComponent default to LayerConstants::DEFAULT_LAYER_MASK.
     * @param start Starting point of the ray.
     * @param direction Direction of the ray.
     * @param layerMask Layer mask for limiting the returned result.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual IRayCastResult::Ptr RayCast(
        const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction, uint64_t layerMask) = 0;

    /**
     * Activate rendering of camera in scene.
     * @param camera The camera to be used in rendering
     */
    virtual void ActivateCamera(const ICamera::Ptr& camera) = 0;

    /**
     * Deactivates rendering of camera in scene.
     * @param camera The camera to be deactivated.
     */
    virtual void DeactivateCamera(const ICamera::Ptr& camera) = 0;

    virtual bool IsCameraActive(const ICamera::Ptr& camera) = 0;

    /**
     * @brief Returns all cameras from the scene.
     * @return Returns a vector of camera objects.
     */
    virtual BASE_NS::vector<ICamera::Ptr> GetCameras() const = 0;

    // returns a list of camera entities that were updated.
    virtual BASE_NS::vector<CORE_NS::Entity> RenderCameras() = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IScene::WeakPtr);
META_TYPE(SCENE_NS::IScene::Ptr);

#endif // SCENEPLUGIN_INTF_SCENE_H
