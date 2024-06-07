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

#ifndef SCENE_HOLDER_H
#define SCENE_HOLDER_H

#include <ComponentTools/component_query.h>
#include <scene_plugin/interface/intf_asset_manager.h>
#include <scene_plugin/interface/intf_bitmap.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_entity_collection.h>
#include <scene_plugin/interface/intf_nodes.h>
#include <scene_plugin/interface/intf_scene.h>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_picking.h>
#include <base/containers/string.h>
#include <base/util/uid.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <render/intf_render_context.h>

#include <meta/interface/intf_task_queue.h>

#include "ecs_listener.h"

class SceneHolder {
public:
    using Ptr = BASE_NS::shared_ptr<SceneHolder>;
    using WeakPtr = BASE_NS::weak_ptr<SceneHolder>;

    class ISceneInitialized : public META_NS::ICallable {
    public:
        constexpr static BASE_NS::Uid UID { "bd0405c7-bb97-4230-b893-1cd242909dd0" };
        using Ptr = BASE_NS::shared_ptr<ISceneInitialized>;
        virtual void Invoke(const BASE_NS::string& rootId, const BASE_NS::string& cameraId) = 0;
        using FunctionType = void(const BASE_NS::string& rootId, const BASE_NS::string& cameraId);

    protected:
        friend Ptr;
        META_NO_COPY_MOVE_INTERFACE(ISceneInitialized)
    };

    class ISceneLoaded : public META_NS::ICallable {
    public:
        constexpr static BASE_NS::Uid UID { "004304d0-63bf-4e63-928d-7349a337ec35" };
        using Ptr = BASE_NS::shared_ptr<ISceneLoaded>;
        virtual void Invoke(uint32_t loadingStatus) = 0;
        using FunctionType = void(uint32_t loadingStatus);

    protected:
        friend Ptr;
        META_NO_COPY_MOVE_INTERFACE(ISceneLoaded)
    };

    class ISceneUpdated : public META_NS::ICallable {
    public:
        constexpr static BASE_NS::Uid UID { "41cb396a-e628-43c3-a0eb-2fc7144674da" };
        using Ptr = BASE_NS::shared_ptr<ISceneUpdated>;
        virtual void Invoke(
            RENDER_NS::RenderHandleReference image, uint64_t handle, uint32_t width, uint32_t height) = 0;
        using FunctionType = void(
            RENDER_NS::RenderHandleReference image, uint64_t handle, uint32_t width, uint32_t height);

    protected:
        friend Ptr;
        META_NO_COPY_MOVE_INTERFACE(ISceneUpdated)
    };

    class ISceneUninitialized : public META_NS::ICallable {
    public:
        constexpr static BASE_NS::Uid UID { "99422a62-1963-4543-a17f-8539e5f58fb1" };
        using Ptr = BASE_NS::shared_ptr<ISceneUninitialized>;
        // return false from task to stop, otherwise it will be called again after
        // the defined delay.
        virtual void Invoke() = 0;
        using FunctionType = void();

    protected:
        friend Ptr;
        META_NO_COPY_MOVE_INTERFACE(ISceneUninitialized)
    };

    SceneHolder(META_NS::InstanceId uid, META_NS::IObjectRegistry& registry,
        const BASE_NS::shared_ptr<RENDER_NS::IRenderContext>& gc, META_NS::ITaskQueue::Ptr appQueue,
        META_NS::ITaskQueue::Ptr engineQueue);
    virtual ~SceneHolder();

    void Initialize(WeakPtr self);
    void Uninitialize();

    void Load(const BASE_NS::string& uri);

    void SetRenderSize(uint32_t width, uint32_t height, uint64_t cameraHandle);
    void SetSystemGraphUri(const BASE_NS::string& uri);

    void SetCameraTarget(
        const SCENE_NS::ICamera::Ptr& camera,  BASE_NS::Math::UVec2 size, RENDER_NS::RenderHandleReference ref);

    void SetInitializeCallback(ISceneInitialized::Ptr callback, WeakPtr self);
    void SetSceneLoadedCallback(ISceneLoaded::Ptr callback, WeakPtr self);
    void SetUninitializeCallback(ISceneUninitialized::Ptr callback, WeakPtr self);

    CORE_NS::IEcs::Ptr GetEcs();

    void ChangeCamera(SCENE_NS::ICamera::Ptr camera);

    void SaveScene(const BASE_NS::string& fileName);

    META_NS::ITaskQueue::Token QueueEngineTask(
        const META_NS::ITaskQueue::CallableType::Ptr& task, bool runDeferred = true)
    {
        if (!isAsync_ && !runDeferred) {
            if (task->Invoke()) {
                // task requested async retry, give it
                return engineTaskQueue_->AddTask(task);
            }
            return META_NS::ITaskQueue::Token {};
        } else {
            return engineTaskQueue_->AddTask(task);
        }
    }
    META_NS::ITaskQueue::Token QueueApplicationTask(
        const META_NS::ITaskQueue::CallableType::Ptr& task, bool runDeferred = true)
    {
        if (!isAsync_ && !runDeferred) {
            if (task->Invoke()) {
                return appTaskQueue_->AddTask(task);
            }
            return META_NS::ITaskQueue::Token {};
        } else {
            return appTaskQueue_->AddTask(task);
        }
    }

    void SetOperationMode(bool async)
    {
        isAsync_ = async;
    }

    bool IsAsync() const
    {
        return isAsync_;
    }

    void CancelEngineTask(META_NS::ITaskQueue::Token token)
    {
        engineTaskQueue_->CancelTask(token);
    }

    void CancelAppTask(META_NS::ITaskQueue::Token token)
    {
        appTaskQueue_->CancelTask(token);
    }

    SCENE_NS::IEntityCollection* GetEntityCollection()
    {
        return scene_.get();
    }

    SCENE_NS::IAssetManager* GetAssetManager()
    {
        return assetManager_.get();
    }

    // Set Uri to load scene
    void LoadScene(const BASE_NS::string& uri);

    // Actual loading code, can take place through several paths
    void LoadScene();

    // Run component queries for materials and meshes
    void IntrospectNodeless();

    // Find animation entity based on name
    bool FindAnimation(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, CORE_NS::Entity& entity);

    // Find material entity based on name
    bool FindMaterial(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, CORE_NS::Entity& entity);

    // Find mesh entity based on name
    bool FindMesh(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, CORE_NS::Entity& entity);

    // Find resource entity based on name using the scene-entity collection
    bool FindResource(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, CORE_NS::Entity& entity);

    BASE_NS::shared_ptr<BASE_NS::vector<BASE_NS::string_view>> ListMaterialNames();
    BASE_NS::shared_ptr<BASE_NS::vector<BASE_NS::string_view>> ListMeshNames();

    // Find resource identifier based on entity
    BASE_NS::string GetResourceId(CORE_NS::Entity entity);

    // Set given mesh to target entity
    void SetMesh(CORE_NS::Entity targetEntity, CORE_NS::Entity mesh);

    // Read the name component of the mesh in the given entity
    BASE_NS::string_view GetMeshName(CORE_NS::Entity referringEntity);

    // Get Mesh entity from submesh
    CORE_NS::Entity GetMaterial(CORE_NS::Entity meshEntity, int64_t submeshIndex);

    // Resolve material name from submesh
    BASE_NS::string_view GetMaterialName(CORE_NS::Entity meshEntity, int64_t submeshIndex);

    // Assign material to the given entity
    void SetMaterial(CORE_NS::Entity targetEntity, CORE_NS::Entity material, int64_t submeshIndex = -1);

    // Prepares a bitmap for 3D use
    CORE_NS::EntityReference BindUIBitmap(SCENE_NS::IBitmap::Ptr bitmap, bool createNew);

    // Set submesh render sort order
    void SetSubmeshRenderSortOrder(CORE_NS::Entity meshEntity, int64_t submeshIndex, uint8_t value);

    // Set submesh aabb min
    void SetSubmeshAABBMin(CORE_NS::Entity targetEntity, int64_t submeshIndex, const BASE_NS::Math::Vec3& vec);

    // Set submesh aabb max
    void SetSubmeshAABBMax(CORE_NS::Entity targetEntity, int64_t submeshIndex, const BASE_NS::Math::Vec3& vec);

    // Remove submesh(es)
    void RemoveSubmesh(CORE_NS::Entity targetEntity, int64_t submeshIndex);

    // Set texture to a given index on texture array
    void SetTexture(size_t index, CORE_NS::Entity targetEntity, CORE_NS::EntityReference imageEntity);

    enum UriHandleType {
        HANDLE_TYPE_DO_NOT_CARE = 0,
        HANDLE_TYPE_SHADER,
    };

    // Get handle for a shader
    BASE_NS::string GetHandleUri(
        RENDER_NS::RenderHandleReference renderHandleReference, UriHandleType type = HANDLE_TYPE_DO_NOT_CARE);

    // Set and get shader and graphics state to/from the given material
    enum ShaderType : uint8_t { MATERIAL_SHADER, DEPTH_SHADER };
    void SetShader(CORE_NS::Entity materialEntity, ShaderType type, SCENE_NS::IShader::Ptr shader);
    void SetGraphicsState(CORE_NS::Entity materialEntity, ShaderType type, SCENE_NS::IGraphicsState::Ptr state);
    SCENE_NS::IShader::Ptr GetShader(CORE_NS::Entity materialEntity, ShaderType type);
    SCENE_NS::IGraphicsState::Ptr GetGraphicsState(CORE_NS::Entity materialEntity, ShaderType type);

    // for state raw access
    void SetGraphicsState(CORE_NS::Entity materialEntity, ShaderType type, const RENDER_NS::GraphicsState& state);
    bool GetGraphicsState(
        CORE_NS::Entity materialEntity, ShaderType type, const SCENE_NS::IShaderGraphicsState::Ptr& ret);

    // Get handle for a sampler
    CORE_NS::Entity LoadSampler(BASE_NS::string_view uri);

    // Get handle for a image
    CORE_NS::EntityReference LoadImage(BASE_NS::string_view uri, RENDER_NS::RenderHandleReference rh = {});

    // Set (and create if needed) sampler to a given index on texture array
    void SetSampler(size_t index, CORE_NS::Entity targetEntity, SCENE_NS::ITextureInfo::SamplerId samplerId);

    // Enable evironment component on the entity.
    void EnableEnvironmentComponent(CORE_NS::Entity entity);

    // Enable layer component on the entity. This is called for every node that scene creates or binds
    void EnableLayerComponent(CORE_NS::Entity entity);

    // Enable light component on the entity.
    void EnableLightComponent(CORE_NS::Entity entity);

    // Enable canvas component on the entity.
    // Create new node
    CORE3D_NS::ISceneNode* CreateNode(const BASE_NS::string& name);
    CORE3D_NS::ISceneNode* CreateNode(const BASE_NS::string& path, const BASE_NS::string& name);

    // Create new material entity
    CORE_NS::Entity CreateMaterial(const BASE_NS::string& name);

    // Create new node with a camera component, by default no flag bits are set.
    CORE_NS::Entity CreateCamera(const BASE_NS::string& name, uint32_t flagBits);
    CORE_NS::Entity CreateCamera(const BASE_NS::string& path, const BASE_NS::string& name, uint32_t flagBits);

    CORE_NS::Entity CreatePostProcess();
    CORE_NS::Entity CreateRenderConfiguration();

    // Rename the entity name component
    void RenameEntity(CORE_NS::Entity entity, const BASE_NS::string& name);

    // Clone the entity, reference will be stored
    CORE_NS::Entity CloneEntity(CORE_NS::Entity entity, const BASE_NS::string& name, bool storeWithUniqueId);

    // Destroy entity
    void DestroyEntity(CORE_NS::Entity entity);

    void SetEntityActive(CORE_NS::Entity entity, bool active);

    // Find and re-parent node. Does not find nodeless entities
    const CORE3D_NS::ISceneNode* ReparentEntity(const BASE_NS::string& parentPath, const BASE_NS::string& name);

    bool ReparentEntity(CORE_NS::Entity entity, const BASE_NS::string& parentPath, size_t index);

    // Create mesh from raw data
    template<typename IndicesType>
    CORE_NS::Entity CreateMeshFromArrays(const BASE_NS::string& name,
        SCENE_NS::MeshGeometryArrayPtr<IndicesType> arrays, RENDER_NS::IndexType indexType,
        CORE_NS::Entity existingEntity = {}, bool append = false);

    void SetMultiviewCamera(CORE_NS::Entity target, CORE_NS::Entity source);
    void RemoveMultiviewCamera(CORE_NS::Entity target, CORE_NS::Entity source);

    // Copy submesh from another mesh entity
    void CopySubMesh(CORE_NS::Entity target, CORE_NS::Entity source, size_t index);

    // Create new multi-mesh batch
    CORE_NS::Entity CreateMultiMeshInstance(CORE_NS::Entity baseComponent);

    // Set mesh to multi-mesh
    void SetMeshMultimeshArray(CORE_NS::Entity target, CORE_NS::Entity mesh);

    // Set override material to multi-mesh
    BASE_NS::vector<CORE_NS::Entity> SetOverrideMaterialMultimeshArray(
        CORE_NS::Entity target, CORE_NS::Entity material);
    void ResetOverrideMaterialMultimeshArray(CORE_NS::Entity target, BASE_NS::vector<CORE_NS::Entity>& in);
    // Set instance count to multi-mesh
    void SetInstanceCountMultimeshArray(CORE_NS::Entity target, size_t count);

    // Set visible count to multi-mesh
    void SetVisibleCountMultimeshArray(CORE_NS::Entity target, size_t count);

    // Set custom data to multi-mesh index
    void SetCustomData(CORE_NS::Entity target, size_t index, const BASE_NS::Math::Vec4& data);

    // Set transformation to multi-mesh index
    void SetTransformation(CORE_NS::Entity target, size_t index, const BASE_NS::Math::Mat4X4& transform);

    // Reposition a node within its parent
    void ReindexEntity(CORE_NS::Entity target, size_t index);

    bool GetImageEntity(CORE_NS::Entity material, size_t index, CORE_NS::Entity& entity);
    bool SetEntityUri(const CORE_NS::Entity& entity, const BASE_NS::string& nameString);
    bool GetEntityUri(const CORE_NS::Entity& entity, BASE_NS::string& nameString);
    bool GetEntityName(const CORE_NS::Entity& entity, BASE_NS::string& nameString);
    bool GetImageHandle(
        const CORE_NS::Entity& entity, RENDER_NS::RenderHandleReference& handle, RENDER_NS::GpuImageDesc& desc);
    bool GetRenderHandleUri(const RENDER_NS::RenderHandle& handle, BASE_NS::string& uriString);
    void SetRenderHandle(const CORE_NS::Entity& target, const CORE_NS::Entity& source);

    CORE_NS::Entity GetEntityByUri(BASE_NS::string_view uriString);

    // Get common ECS listener to avoid overhead of all properties using their own
    BASE_NS::shared_ptr<SCENE_NS::EcsListener> GetCommonEcsListener()
    {
        return ecsListener_;
    }

    // Set a callback that fires when the ecs has been created, but not yet initialized.
    // Needs to be handled synchronously. Return value from callback is ignored currently.
    void SetEcsInitializationCallback(
        SCENE_NS::IEcsScene::IPrepareSceneForInitialization::WeakPtr ecsInitializationCallback)
    {
        ecsInitializationCallback_ = ecsInitializationCallback;
    }

    void ReleaseOwnership(CORE_NS::Entity entity);

    struct CameraData {
        using Ptr = BASE_NS::shared_ptr<CameraData>;
        static Ptr Create(const CORE_NS::Entity& entity)
        {
            return Ptr { new CameraData(entity) };
        }
        explicit CameraData(const CORE_NS::Entity& cameraEntity) : entity(cameraEntity) {}
        CORE_NS::Entity entity;
        bool ownsColorImage = true;
        RENDER_NS::RenderHandleReference colorImage;
        RENDER_NS::RenderHandleReference depthImage;
        uint32_t width = 128;
        uint32_t height = 128;
        bool resized = true;
        bool updateTargets = true;
    };

    void UpdateAttachments(SCENE_NS::IEcsObject::Ptr& ecsObject);
    void ResolveAnimations();

    // Not particulally happy with this
    META_NS::IObjectRegistry& GetObjectRegistry()
    {
        return objectRegistry_;
    }

    CORE_NS::IFileManager* GetFileManager()
    {
        if (graphicsContext3D_) {
            auto& engine = graphicsContext3D_->GetRenderContext().GetEngine();
            return &engine.GetFileManager();
        }
        return nullptr;
    }

    SCENE_NS::IEcsAnimation::Ptr GetAnimation(const CORE_NS::Entity& entity)
    {
        if (auto animation = animations_.find(entity.id); animation != animations_.cend()) {
            return animation->second;
        }
        return SCENE_NS::IEcsAnimation::Ptr {};
    }

    SCENE_NS::IEcsAnimation::Ptr GetAnimation(const BASE_NS::string& name)
    {
        for (auto& animation : animations_) {
            if (auto named = interface_pointer_cast<META_NS::INamed>(animation.second)) {
                if (META_NS::GetValue(named->Name()) == name) {
                    return animation.second;
                }
            }
        }
        return SCENE_NS::IEcsAnimation::Ptr {};
    }

    BASE_NS::vector<SCENE_NS::IEcsAnimation::Ptr> GetAnimations() const
    {
        BASE_NS::vector<SCENE_NS::IEcsAnimation::Ptr> res;
        for (auto& animation : animations_) {
            res.push_back(animation.second);
        }
        return res;
    }

    void SetRenderMode(uint8_t renderMode)
    {
        if (renderMode != renderMode_) {
            renderMode_ = (CORE_NS::IEcs::RenderMode)renderMode;
            if (ecs_) {
                ecs_->SetRenderMode(renderMode_);
            }
        }
    }

    bool GetWorldMatrixComponentAABB(SCENE_NS::IPickingResult::Ptr, CORE_NS::Entity entity, bool isRecursive);
    bool GetTransformComponentAABB(SCENE_NS::IPickingResult::Ptr, CORE_NS::Entity entity, bool isRecursive);
    bool GetWorldAABB(SCENE_NS::IPickingResult::Ptr, const BASE_NS::Math::Mat4X4& world,
        const BASE_NS::Math::Vec3& aabbMin, const BASE_NS::Math::Vec3& aabbMax);
    bool RayCast(SCENE_NS::IRayCastResult::Ptr, const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction);
    bool RayCast(SCENE_NS::IRayCastResult::Ptr, const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction,
        uint64_t layerMask);
    bool ScreenToWorld(SCENE_NS::IPickingResult::Ptr, CORE_NS::Entity camera, BASE_NS::Math::Vec3 screenCoordinate);

    bool WorldToScreen(SCENE_NS::IPickingResult::Ptr, CORE_NS::Entity camera, BASE_NS::Math::Vec3 worldCoordinate);

    bool RayCastFromCamera(SCENE_NS::IRayCastResult::Ptr, CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos);

    bool RayCastFromCamera(SCENE_NS::IRayCastResult::Ptr, CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos,
        uint64_t layerMask);

    bool RayFromCamera(SCENE_NS::IPickingResult::Ptr ret, CORE_NS::Entity camera, BASE_NS::Math::Vec2 screenCoordinate);

    void ProcessEvents();

    void AddCamera(const CORE_NS::Entity& entity);
    void RemoveCamera(const CORE_NS::Entity& entity);

    void SetMainCamera(const CORE_NS::Entity& entity);

    void ActivateCamera(const CORE_NS::Entity& entity) const;
    void DeactivateCamera(const CORE_NS::Entity& cameraEntity) const;
    bool IsCameraActive(const CORE_NS::Entity& cameraEntity) const;

    BASE_NS::vector<CORE_NS::Entity> RenderCameras();

private:
    void RemoveUriComponentsFromMeshes();

    bool InitializeScene();
    bool UninitializeScene();

    bool CreateDefaultEcs();
    void SetDefaultCamera();

    void UpdateViewportSize(uint32_t width, uint32_t height, uint64_t cameraHandle);

    void RecreateOutputTexture(CameraData::Ptr camera);
    bool UpdateCameraRenderTarget(CameraData::Ptr camera);

    void RequestReload();
    void ResetScene(bool initialize = false);

    void SetSceneSystemGraph(const BASE_NS::string& uri);

    BASE_NS::string GetUniqueName();

    bool IsMultiMeshChild(const CORE3D_NS::ISceneNode* child);

    CORE_NS::Entity FindCachedRelatedEntity(const CORE_NS::Entity& entity);

    BASE_NS::vector<CameraData::Ptr> cameras_;
    CameraData::Ptr mainCamera_ {};

    // Data for Engine thread.
    META_NS::ITaskQueue::Token updateTaskToken_ {};
    META_NS::ITaskQueue::Token redrawTaskToken_ {};

    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext3D_;
    CORE_NS::IEcs::Ptr ecs_;

    CORE_NS::Entity defaultCameraEntity_;

    CORE_NS::Entity sceneEntity_;

    CORE3D_NS::GLTFResourceData gltfResourceData_;

    CORE3D_NS::ISceneNode* rootNode_ { nullptr };

    BASE_NS::unordered_map<uint64_t, bool> isReadyForNewFrame_;

    META_NS::InstanceId instanceId_;

    ISceneInitialized::Ptr sceneInitializedCallback_;
    ISceneLoaded::Ptr sceneLoadedCallback_;
    ISceneUpdated::Ptr sceneUpdatedCallback_;
    ISceneUninitialized::Ptr sceneUninitializedCallback_;
    SCENE_NS::IEcsScene::IPrepareSceneForInitialization::WeakPtr ecsInitializationCallback_;

    META_NS::ITaskQueue::Ptr appTaskQueue_;
    META_NS::ITaskQueue::Ptr engineTaskQueue_;

    SCENE_NS::IAssetManager::Ptr assetManager_ {};
    SCENE_NS::IEntityCollection::Ptr scene_ {};
    uint64_t instanceNumber_ = 0;

    BASE_NS::string sceneSystemGraphUri_ { "project://assets/config/system_graph.json" };
    BASE_NS::string sceneUri_;
    META_NS::TimeSpan refreshInterval_ { META_NS::TimeSpan::Microseconds(16667) };
    bool scenePrepared_ { false };
    WeakPtr me_;

    BASE_NS::unique_ptr<CORE_NS::ComponentQuery> meshQuery_ {};
    BASE_NS::unique_ptr<CORE_NS::ComponentQuery> materialQuery_ {};
    BASE_NS::unique_ptr<CORE_NS::ComponentQuery> animationQuery_ {};

    CORE3D_NS::IAnimationComponentManager* animationComponentManager_ {};
    CORE3D_NS::ICameraComponentManager* cameraComponentManager_ {};

    CORE3D_NS::IEnvironmentComponentManager* envComponentManager_ {};
    CORE3D_NS::ILayerComponentManager* layerComponentManager_ {};
    CORE3D_NS::ILightComponentManager* lightComponentManager_ {};
    CORE3D_NS::IMaterialComponentManager* materialComponentManager_ {};
    CORE3D_NS::IMeshComponentManager* meshComponentManager_ {};
    CORE3D_NS::INameComponentManager* nameComponentManager_ {};
    CORE3D_NS::INodeComponentManager* nodeComponentManager_ {};
    CORE3D_NS::IRenderMeshComponentManager* renderMeshComponentManager_ {};
    CORE3D_NS::IRenderHandleComponentManager* rhComponentManager_ {};
    CORE3D_NS::ITransformComponentManager* transformComponentManager_ {};
    CORE3D_NS::IUriComponentManager* uriComponentManager_ {};

    // Not used regularly, perhaps we don't want to maintain the ptrs
    // CORE3D_NS::IRenderConfigurationComponentManager* rcComponentManager_{};

    CORE3D_NS::INodeSystem* nodeSystem_ {};
    BASE_NS::shared_ptr<SCENE_NS::EcsListener> ecsListener_;

    BASE_NS::unordered_map<uint64_t, SCENE_NS::IEcsAnimation::Ptr> animations_;

    META_NS::IObjectRegistry& objectRegistry_;

    bool isAsync_ { false };
    bool reloadPending_ { false };   // this may occur if switching between sync and async modes on fly
    bool loadSceneFailed_ { false }; // a corner case where request to reload a scene takes place before initialization
    CORE_NS::IEcs::RenderMode renderMode_ { CORE_NS::IEcs::RENDER_IF_DIRTY };

    CORE3D_NS::IPicking* picking_ { nullptr };

    bool isRunningFrame_ { false };
    bool requiresEventProcessing_ { false };

    uint64_t firstTime_ { ~0u };
    uint64_t previousFrameTime_ { ~0u };
    uint64_t deltaTime_ { 1 };
};

static constexpr BASE_NS::string_view MULTI_MESH_CHILD_PREFIX("multi_mesh_child");

META_TYPE(SceneHolder::Ptr);
META_TYPE(SceneHolder::WeakPtr);

#endif
