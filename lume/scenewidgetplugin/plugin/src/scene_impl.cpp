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
#include <algorithm>
#include <functional>
#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/api/environment_uid.h>
#include <scene_plugin/api/light_uid.h>
#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/api/mesh_uid.h>
#include <scene_plugin/api/node_uid.h>
#include <scene_plugin/api/scene_uid.h>
#include <scene_plugin/api/view_node_uid.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_environment.h>

#include <3d/ecs/systems/intf_node_system.h>
#include <core/io/intf_file_manager.h>

#include <meta/api/event_handler.h>
#include <meta/ext/object_container.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/intf_object_hierarchy_observer.h>
#include <meta/interface/serialization/intf_importer.h>

#include "intf_node_private.h"
#include "intf_resource_private.h"
#include "scene_holder.h"
#include "task_utils.h"

// "synchronously initialize the nodes from main scene file when the scene is loaded

// Save the project .scene file when scene object is being serialized. In practice this may override the changes on
// scene file from other sources. To be rectified further

// Name prefab instances with IObject::GetUid(). The other option is to use fixed "prefab_instance_NN" with running
// instance number. The latter may cause trouble with .scene-files if it contains previous instances

using SCENE_NS::MakeTask;
namespace {
class NotifyOnExit {
public:
    explicit NotifyOnExit(bool notify, std::function<void()> callback) : notify_(notify), callback_(callback) {}
    virtual ~NotifyOnExit()
    {
        if (notify_) {
            callback_();
        }
    }
    bool notify_;
    std::function<void()> callback_;
};

class SceneImpl final : public META_NS::ObjectContainerFwd<SceneImpl, SCENE_NS::ClassId::Scene, SCENE_NS::IScene,
                            SCENE_NS::IEcsScene, META_NS::IContent, META_NS::IAnimationController> {
    /// Add the animation controller stuff here. (as scene should be the controller of it's animations)
    META_FORWARD_READONLY_PROPERTY(uint32_t, Count, animationController_->Count())
    META_FORWARD_READONLY_PROPERTY(uint32_t, RunningCount, animationController_->RunningCount())

    BASE_NS::vector<BASE_NS::weak_ptr<META_NS::IAnimation>> GetAnimations() const override;
    BASE_NS::vector<BASE_NS::weak_ptr<META_NS::IAnimation>> GetRunning() const override;
    bool AddAnimation(const BASE_NS::shared_ptr<META_NS::IAnimation>& animation) override;
    bool RemoveAnimation(const BASE_NS::shared_ptr<META_NS::IAnimation>& animation) override;
    void Clear() override;
    StepInfo Step(const META_NS::IClock::ConstPtr& clock) override;
    // now back to normal scene impl

    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IScene, BASE_NS::string, SystemGraphUri, "project://assets/config/system_graph.json")
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(
        SCENE_NS::IScene, uint32_t, Status, SCENE_STATUS_UNINITIALIZED, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::IScene, SCENE_NS::INode::Ptr, RootNode, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IScene, SCENE_NS::ICamera::Ptr, DefaultCamera, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IScene, BASE_NS::string, Uri, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IScene, bool, Asynchronous, false)
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(SCENE_NS::IScene, SCENE_NS::IMaterial::Ptr, Materials, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IScene, SCENE_NS::IRenderConfiguration::Ptr, RenderConfiguration, {})

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnLoaded)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IEcsScene, uint8_t, RenderMode, SCENE_NS::IEcsScene::RENDER_IF_DIRTY)

    META_FORWARD_READONLY_PROPERTY(IObject::Ptr, Content, (contentImpl_ ? contentImpl_->Content() : nullptr))
    META_FORWARD_PROPERTY(bool, ContentSearchable, (contentImpl_ ? contentImpl_->ContentSearchable() : nullptr))
    META_FORWARD_PROPERTY(
        META_NS::IContentLoader::Ptr, ContentLoader, (contentImpl_ ? contentImpl_->ContentLoader() : nullptr))

    void SetRenderMode()
    {
        AddEngineTask(MakeTask(
                          [renderMode = META_NS::GetValue(RenderMode())](auto sceneHolder) {
                              sceneHolder->SetRenderMode(renderMode);
                              return false;
                          },
                          sceneHolder_),
            false);
    }

    uint64_t GetCameraHandle(const SCENE_NS::ICamera::Ptr& camera)
    {
        if (camera) {
            auto ecsObject = interface_pointer_cast<SCENE_NS::IEcsObject>(camera);
            if (ecsObject) {
                return ecsObject->GetEntity().id;
            }
        }

        return SCENE_NS::IScene::DEFAULT_CAMERA;
    }

    struct BitmapInfo {
        BitmapInfo() = default;
        BitmapInfo(SCENE_NS::IBitmap::Ptr bmp) : bitmap(bmp) {}
        BASE_NS::Math::UVec2 size {};
        SCENE_NS::IBitmap::Ptr bitmap {};
        META_NS::EventHandler bitmapChanged {};
    };
    BASE_NS::unordered_map<uint64_t, BitmapInfo> bitmaps_;
    BitmapInfo& GetData(const SCENE_NS::ICamera::Ptr& camera)
    {
        auto cameraHandle = GetCameraHandle(camera);
        if (cameraHandle == SCENE_NS::IScene::DEFAULT_CAMERA) {
            cameraHandle = defaultCameraHandle_;
        }
        auto handle = cameraHandle;
        if (cameraHandle == SCENE_NS::IScene::DEFAULT_CAMERA && !bitmaps_.empty()) {
            handle = defaultCameraHandle_;
        }
        return bitmaps_[handle];
    }

    void SetBitmap(const SCENE_NS::IBitmap::Ptr& bitmap, const SCENE_NS::ICamera::Ptr& camera) override
    {
        BitmapInfo& data = GetData(camera);
        auto uiBitmap = interface_pointer_cast<SCENE_NS::IBitmap>(bitmap);
        if (!uiBitmap) {
            // disable bitmap override.
            data.bitmapChanged.Unsubscribe();
            data.bitmap = {};
            sceneHolder_->SetCameraTarget(camera, data.size, {});
            return;
        }

        data.bitmap = uiBitmap;
        data.size = data.bitmap->Size()->GetValue();
        const auto rh = data.bitmap->GetRenderHandle();

        data.bitmapChanged.Subscribe(
            uiBitmap->ResourceChanged(), META_NS::MakeCallback<META_NS::IOnChanged>([this, camera]() {
                BitmapInfo& data = GetData(camera);
                data.size = data.bitmap->Size()->GetValue();
                const auto rh = data.bitmap->GetRenderHandle();
                sceneHolder_->SetCameraTarget(camera, data.size, rh);
            }));

        META_NS::Invoke<META_NS::IOnChanged>(uiBitmap->ResourceChanged());
    }

    SCENE_NS::IBitmap::Ptr GetBitmap(bool notifyFrameDrawn, const SCENE_NS::ICamera::Ptr& camera) override
    {
        BitmapInfo& data = GetData(camera);
        if (!data.bitmap) {
            // there is no bitmap for this.
            //  create it?
        }
        return data.bitmap;
    }

#define CREATE_META_INSTANCE(type, name) GetObjectRegistry().Create<META_NS::type>(META_NS::ClassId::name)

    // META_NS::IContent
    bool SetContent(const META_NS::IObject::Ptr& content) override
    {
        // Should be no-op because the Root Node of a scene (content in this case) can be assigned only if the scene was
        // reloaded.
        return false;
    }

    bool Build(const META_NS::IMetadata::Ptr& data) override
    {
        auto& registry = GetObjectRegistry();

        animationController_ = registry.Create<META_NS::IAnimationController>(META_NS::ClassId::AnimationController);

        using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc;
        META_NS::ITaskQueue::Ptr appQueue;
        META_NS::ITaskQueue::Ptr engineQueue;

        if (data) {
            if (auto prp = data->GetPropertyByName<IntfPtr>("RenderContext")) {
                rc = interface_pointer_cast<RENDER_NS::IRenderContext>(prp->GetValue());
            }
            if (auto prp = data->GetPropertyByName<IntfPtr>("EngineQueue")) {
                engineQueue = interface_pointer_cast<META_NS::ITaskQueue>(prp->GetValue());
            }
            if (auto prp = data->GetPropertyByName<IntfPtr>("AppQueue")) {
                appQueue = interface_pointer_cast<META_NS::ITaskQueue>(prp->GetValue());
            }
        }
        if ((!rc) || (!engineQueue) || (!appQueue)) {
            return false;
        }

        hierarchyController_ =
            registry.Create<META_NS::IObjectHierarchyObserver>(SCENE_NS::ClassId::NodeHierarchyController);

        contentImpl_ = registry.Create<META_NS::IContent>(META_NS::ClassId::ContentObject);
        if (const auto req = interface_cast<META_NS::IRequiredInterfaces>(contentImpl_)) {
            req->SetRequiredInterfaces({ SCENE_NS::INode::UID });
        }

        sceneHolder_.reset(new SceneHolder(GetInstanceId(), registry, rc, appQueue, engineQueue));
        sceneHolder_->SetOperationMode(Asynchronous()->GetValue());

        asyncChangedToken_ = Asynchronous()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(
            [](const auto& sceneHolder, const auto& async) {
                if (sceneHolder && async) {
                    sceneHolder->SetOperationMode(async->GetValue());
                }
            },
            sceneHolder_, Asynchronous()));

        renderModeChangedToken_ = RenderMode()->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([weak = BASE_NS::weak_ptr(GetSelf())]() {
                if (auto self = static_pointer_cast<SceneImpl>(weak.lock())) {
                    self->SetRenderMode();
                }
            }));

        sceneHolder_->SetInitializeCallback(
            META_NS::MakeCallback<SceneHolder::ISceneInitialized>(
                [me = BASE_NS::weak_ptr(GetSelf())](const BASE_NS::string& rootId, const BASE_NS::string& cameraId) {
                    if (auto self = me.lock().get())
                        static_cast<SceneImpl*>(self)->onSceneInitialized(rootId, cameraId);
                }),
            sceneHolder_);
        sceneHolder_->SetSceneLoadedCallback(
            META_NS::MakeCallback<SceneHolder::ISceneLoaded>([me = BASE_NS::weak_ptr(GetSelf())](uint32_t status) {
                if (auto self = me.lock().get())
                    static_cast<SceneImpl*>(self)->OnSceneLoaded(status);
            }),
            sceneHolder_);

        sceneHolder_->Initialize(sceneHolder_);
        sceneHolder_->SetSystemGraphUri(META_NS::GetValue(SystemGraphUri()));
        SubscribeToPropertyChanges();

        return true;
    }

    // this is not usually needed as explicit action, but the scene will pickup changes from
    // ScenePresenters on the fly
    void OnCameraChanged()
    {
        // Async, sceneholder takes over
        if (sceneHolder_) {
            sceneHolder_->ChangeCamera(META_ACCESS_PROPERTY(DefaultCamera)->GetValue());
        }
    }
    
    // Async, sceneholder takes over
    BASE_NS::vector<META_NS::IAnimation::Ptr> allAnims_;
    BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations() override
    {
        // adds/removes animations to cache..
        auto tmp = allAnims_;
        allAnims_.clear();
        for (auto anim : sceneHolder_->GetAnimations()) {
            if (auto i = interface_cast<META_NS::IObject>(anim)) {
                auto name = i->GetName();

                size_t ix = 0;
                if (!RollOverPrefix(ix, name, ANIMATIONS_PREFIX)) {
                    continue;
                }

                // check cache
                META_NS::IAnimation::Ptr rn;
                auto subname = name.substr(ix);
                for (auto& a : tmp) {
                    if (interface_cast<META_NS::IObject>(a)->GetName() == subname) {
                        rn = a;
                        break;
                    }
                }
                if (!rn) {
                    rn = interface_pointer_cast<META_NS::IAnimation>(
                        CreateNode(name.substr(ix), false, SCENE_NS::ClassId::Animation.Id(),
                            SCENE_NS::INode::BuildBehavior::NODE_BUILD_CHILDREN_NO_BUILD));
                }

                allAnims_.push_back(rn);
            }
        }
        return allAnims_;
    }

    META_NS::IAnimation::Ptr GetAnimation(const BASE_NS::string_view name) override
    {
#ifndef USE_DIRECT_ECS_ANIMATION

        size_t ix = 0;
        if (!RollOverPrefix(ix, name, ANIMATIONS_PREFIX)) {
            return { nullptr };
        }

        // check cache
        if (auto it = animations_.find(name.substr(ix)); it != animations_.end()) {
            if (auto animation = it->second.lock()) {
                return animation;
            }
        }
        // Create EcsObject. When running asynchronously, we have no way of knowing if we can rely that existing
        // animation will be found
        auto anim = interface_pointer_cast<META_NS::IAnimation>(CreateNode(name.substr(ix), false,
            SCENE_NS::ClassId::Animation.Id(), SCENE_NS::INode::BuildBehavior::NODE_BUILD_CHILDREN_NO_BUILD));

        return anim;
#else
        auto ecsAnimation = sceneHolder_->GetAnimation(BASE_NS::string(name.data(), name.size()));

        if (!ecsAnimation) {
            ecsAnimation = GetObjectRegistry().Create<SCENE_NS::IEcsAnimation>(SCENE_NS::ClassId::EcsAnimation);
            AddEngineTask(
                META_NS::MakeCallable<META_NS::ITaskQueueTask>(
                    [ecsAnimation, nameString = BASE_NS::string(name.data(), name.size()),
                        weak = BASE_NS::weak_ptr(sceneHolder_)]() {
                        if (auto sceneHolder = weak.lock()) {
                            CORE_NS::Entity entity;
                            if (sceneHolder->FindAnimation(nameString, entity)) {
                                if (auto ecsProxyIf = interface_pointer_cast<SCENE_NS::IEcsProxyObject>(ecsAnimation)) {
                                    ecsProxyIf->SetCommonListener(sceneHolder->GetCommonEcsListener());
                                }
                                ecsAnimation->SetEntity(*sceneHolder->GetEcs(), entity);
                            }
                        }
                        return false;
                    }),
                false);
        }
        META_ACCESS_PROPERTY(Animations)->Get()->Add(ecsAnimation);

        return interface_pointer_cast<META_NS::IAnimation>(ecsAnimation);
#endif
    }

    void CreateEmpty() override
    {
        Load("scene://empty");
    }

    bool Load(const BASE_NS::string_view uri) override
    {
        if (!Name()->IsValueSet()) {
            SetValue(Name(), "Scene");
        }

        if (!uri.empty()) {
            META_ACCESS_PROPERTY(Status)->SetValue(SCENE_STATUS_LOADING);
            sceneHolder_->Load(BASE_NS::string(uri.data(), uri.size()));
            return true;
        }
        return false;
    }

    void onSystemGraphUriChanged()
    {
        META_ACCESS_PROPERTY(Status)->SetValue(SCENE_STATUS_LOADING);
        sceneHolder_->SetSystemGraphUri(META_NS::GetValue(SystemGraphUri()));
    }

    void SetRenderSize(uint32_t width, uint32_t height, const SCENE_NS::ICamera::Ptr& camera) override
    {
        if (camera) {
            camera->SetDefaultRenderTargetSize(width, height);
        } else if (auto defaultCamera = META_NS::GetValue(DefaultCamera())) {
            defaultCamera->SetDefaultRenderTargetSize(width, height);
        }
    }

    SCENE_NS::INode::Ptr GetNode(const BASE_NS::string_view path, const BASE_NS::Uid classId,
        SCENE_NS::INode::BuildBehavior buildBehavior) override
    {
        return GetNodeRecursive(path, classId, true, buildBehavior);
    }

    META_NS::ObjectId ResolveNodeTypeFromPath(const BASE_NS::string_view patchedPath, bool isNodeType)
    {
        // This is best effort
        // We cannot determine the type unless ECS has probed the component

        // This kind of introspection may cause materials and meshes to be treated as nodes
        // which kind of contradicts with their normal use through API
        auto ecs = GetEcs();
        CORE3D_NS::INodeSystem& nodeSystem = *CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*ecs);
        const auto& root = nodeSystem.GetRootNode();

        const auto& ecsNode = root.LookupNodeByPath(patchedPath);
        CORE_NS::Entity entity {};
        if (ecsNode) {
            entity = ecsNode->GetEntity();
        } else {
            CORE_LOG_W("%s:No entity for %s, type info not available", __func__, BASE_NS::string(patchedPath).c_str());
            CORE_LOG_W("If you know the expected type, consider using a template or providing the class id");
        }

        // shadow camera could provide false positive, so order matters
        if (auto lightManager = ecs->GetComponentManager(CORE3D_NS::ILightComponentManager::UID)) {
            if (lightManager->HasComponent(entity)) {
                return SCENE_NS::ClassId::Light;
            }
        }

        if (auto cameraManager = ecs->GetComponentManager(CORE3D_NS::ICameraComponentManager::UID)) {
            if (cameraManager->HasComponent(entity)) {
                return SCENE_NS::ClassId::Camera;
            }
        }

        if (auto envManager = ecs->GetComponentManager(CORE3D_NS::IEnvironmentComponentManager::UID)) {
            if (envManager->HasComponent(entity)) {
                return SCENE_NS::ClassId::Environment;
            }
        }

        if (!isNodeType) {
            if (auto nodeManager = ecs->GetComponentManager(CORE3D_NS::INodeComponentManager::UID)) {
                // quirk, prefer nodes with node component treated as node)
                if (!nodeManager->HasComponent(entity)) {
                    if (auto meshManager = ecs->GetComponentManager(CORE3D_NS::IMeshComponentManager::UID)) {
                        if (meshManager->HasComponent(entity)) {
                            return SCENE_NS::ClassId::Mesh;
                        }
                    }
                    if (auto materialManager = ecs->GetComponentManager(CORE3D_NS::IMaterialComponentManager::UID)) {
                        if (materialManager->HasComponent(entity)) {
                            return SCENE_NS::ClassId::Material;
                        }
                    }
                }
            }
        }

        return SCENE_NS::ClassId::Node;
    }

    SCENE_NS::INode::Ptr GetNodeRecursive(const BASE_NS::string_view path, const META_NS::ObjectId classId,
        bool recurse, SCENE_NS::INode::BuildBehavior buildBehavior)
    {
        if (path.empty() || path == "/") {
            return SCENE_NS::INode::Ptr {};
        }

        BASE_NS::string patchedPath = recurse ? NormalizePath(path) : BASE_NS::string(path.data(), path.size());
        if (auto ite = nodes_.find(patchedPath) != nodes_.cend()) {
            return nodes_[patchedPath];
        }

        // ensure parent objects exist
        if (recurse) {
            size_t ix = patchedPath.find('/', 1);
            while (BASE_NS::string_view::npos != ix) {
                auto substr = patchedPath.substr(0, ix);
                // When we traverse up the tree, we must ensure that the object is a node.
                currentParent_ = GetNodeRecursive(substr, SCENE_NS::INode::UID, false, buildBehavior);
                ++ix;
                ix = patchedPath.find('/', ix);
            }
        }

        auto ecs = GetEcs();
        META_NS::ObjectId implementationId = SCENE_NS::ClassId::Node;

        bool isNodeType = (classId == SCENE_NS::INode::UID);
        if ((classId == META_NS::IObject::UID || isNodeType) && ecs) {
            implementationId = ResolveNodeTypeFromPath(patchedPath, isNodeType);

        } else {
            implementationId = classId;
        }

        auto node = CreateNode(patchedPath, false, implementationId, buildBehavior);

        currentParent_ = {};
        return node; // finalInterface;
    }

    void RemoveNodeFromCurrentContainer(SCENE_NS::INode::Ptr& node)
    {
        if (auto containable = interface_cast<META_NS::IContainable>(node)) {
            if (auto parent = interface_pointer_cast<META_NS::IContainer>(containable->GetParent())) {
                parent->Remove(node);
            }
        }
    }

    bool RollOverPrefix(size_t& ix, const BASE_NS::string_view& name, const BASE_NS::string_view& prefix)
    {
        while (ix < name.length() && name[ix] == '/') {
            ix++;
        }

        if (name.substr(ix).find(prefix) == 0) {
            ix += prefix.length();
        }

        while (ix < name.length() && name[ix] == '/') {
            ix++;
        }

        return ix < name.length();
    }

    void AddMaterial(SCENE_NS::IMaterial::Ptr material) override
    {
        auto materials = Materials()->GetValue();
        auto it = std::find(materials.begin(), materials.end(), material);
        if (it != materials.end()) {
            // Already exists.
            CORE_LOG_D("Trying to add same material to scene multiple times.");
            return;
        }

        Materials()->AddValue(material);
        UpdateCachedReference(interface_pointer_cast<SCENE_NS::INode>(material));
    }

    void RemoveMaterial(SCENE_NS::IMaterial::Ptr material) override
    {
        auto lock = Materials().GetLockedAccess();
        auto vec = lock->GetValue();
        for (size_t index = 0; index != vec.size(); ++index) {
            if (vec[index] == material) {
                lock->RemoveAt(index);
                break;
            }
        }

        for (auto&& ite : materials_) {
            if (ite.second.lock() == material) {
                ReleaseMaterial(ite.first);
                break;
            }
        }
    }

    BASE_NS::vector<SCENE_NS::ICamera::Ptr> GetCameras() const override
    {
        BASE_NS::vector<SCENE_NS::ICamera::Ptr> result;
        for (auto c : cameras_) {
            if (auto cam = c.second.lock()) {
                result.push_back(cam);
            }
        }
        return result;
    }

    BASE_NS::vector<SCENE_NS::IMaterial::Ptr> GetMaterials() const override
    {
        BASE_NS::vector<SCENE_NS::IMaterial::Ptr> result;
        for (auto& material : materials_) {
            auto ptr = material.second.lock();
            if (ptr) {
                result.push_back(ptr);
            }
        }

        return result;
    }

    // Returns a material from the scene with a given path
    SCENE_NS::IMaterial::Ptr GetMaterial(const BASE_NS::string_view name) override
    {
        // The material file, aka uri-path is somewhat parallel due ownership is different for uri-materials
        // however, one can claim traditional handle and have them preserved
        // through flat cache. Should be consolidated someday.
        if (name.find("://") != BASE_NS::string_view::npos) {
            return GetOrLoadMaterial(name);
        }

        size_t ix = 0;
        if (!RollOverPrefix(ix, name, MATERIALS_PREFIX)) {
            return { nullptr };
        }

        // check cache (first with name:entityid)
        if (auto it = materials_.find(name.substr(ix)); it != materials_.end()) {
            if (auto material = it->second.lock()) {
                return material;
            }
        }

        // check cache (direct material name)
        for (auto entry : materials_) {
            auto material = entry.second.lock();
            if (auto node = interface_pointer_cast<SCENE_NS::INode>(material)) {
                if (node->Name()->GetValue() == name) {
                    return material;
                }
            }
        }

        if (auto it = materials_.find(name.substr(ix)); it != materials_.end()) {
            if (auto material = it->second.lock()) {
                return material;
            }
        }

        // Create EcsObject. When running asynchronously, we have no way of knowing if we should create
        // new node or just rely that existing will be found
        auto mat = interface_pointer_cast<SCENE_NS::IMaterial>(CreateNode(name.substr(ix), false,
            SCENE_NS::ClassId::Material.Id(), SCENE_NS::INode::BuildBehavior::NODE_BUILD_CHILDREN_NO_BUILD));

        return mat;
    }

    BASE_NS::vector<SCENE_NS::IMesh::Ptr> GetMeshes() const override
    {
        BASE_NS::vector<SCENE_NS::IMesh::Ptr> result;
        for (auto mesh : meshes_) {
            auto ptr = mesh.second.lock();
            if (ptr) {
                result.push_back(ptr);
            }
        }

        return result;
    }

    // Returns a material from the scene with a given name
    SCENE_NS::IMesh::Ptr GetMesh(const BASE_NS::string_view name) override
    {
        size_t ix = 0;
        if (!RollOverPrefix(ix, name, MESHES_PREFIX)) {
            return { nullptr };
        }

        // check cache
        if (auto it = meshes_.find(name.substr(ix)); it != meshes_.end()) {
            if (auto mesh = it->second.lock()) {
                return mesh;
            }
        }
        // Create EcsObject. When running asynchronously, we have no way of knowing if we should create
        // new node or just rely that existing will be found
        auto mesh = interface_pointer_cast<SCENE_NS::IMesh>(CreateNode(name.substr(ix), false,
            SCENE_NS::ClassId::Mesh.Id(), SCENE_NS::INode::BuildBehavior::NODE_BUILD_CHILDREN_NO_BUILD));

        return mesh;
    }

    BASE_NS::string constructPath(CORE_NS::Entity ent) const
    {
        auto ecs = GetEcs();
        auto* nodeManager = CORE_NS::GetManager<CORE3D_NS::INodeComponentManager>(*ecs);
        auto* nameManager = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
        BASE_NS::string path;
        if (nodeManager && nameManager) {
            auto curent = ent;
            for (;;) {
                if (!CORE_NS::EntityUtil::IsValid(curent)) {
                    // Reached root.
                    break;
                }
                if (!nodeManager->HasComponent(curent)) {
                    // not a node?
                    return "";
                }
                if (!nameManager->HasComponent(curent)) {
                    // no name in hierarchy. "fail"? or generate "name" for path..
                    return "";
                }
                auto namecomp = nameManager->Get(curent);
                if (!path.empty()) {
                    path.insert(0, "/");
                }
                path.insert(0, namecomp.name.c_str());
                const auto& node = nodeManager->Get(curent);
                curent = node.parent;
            }
        }
        if (!path.empty()) {
            path.insert(0, "/");
        }
        return path;
    }
    // Implementation for scene change callbacks.
    // The tricky bit here is that we may have placeholders on containers that may have received serialized data
    // during construction, so we just cannot replace those, but need to rebind them instead
    void onSceneInitialized(const BASE_NS::string& rootId, const BASE_NS::string& cameraId)
    {
        // Set root node
        rootNodeId_ = rootId;
        auto rootIdNormalized = NormalizePath(rootNodeId_);

        if (rootNodePtr_) {
            // check if someone assigned us a root node while we are preserving the previous one
            if (!META_ACCESS_PROPERTY(RootNode)->GetValue()) {
                META_ACCESS_PROPERTY(RootNode)->SetValue(rootNodePtr_);
            }
            rootNodePtr_.reset();
        }

        if (cameraNodePtr_) {
            // check if someone assigned us a root node while we are preserving the previous one
            if (!META_ACCESS_PROPERTY(DefaultCamera)->GetValue()) {
                META_ACCESS_PROPERTY(DefaultCamera)->SetValue(cameraNodePtr_);
            }
            cameraNodePtr_.reset();
        }

        if (auto renderConfiguration = META_NS::GetValue(RenderConfiguration())) {
            // Ensure the render configuration is bound to scene.
            auto resource = interface_pointer_cast<IResourcePrivate>(renderConfiguration);
            if (resource) {
                resource->Connect(sceneHolder_);
            }
        }

        // setup subscription for toolkit changes
        RenderConfiguration()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
            auto resource = interface_pointer_cast<IResourcePrivate>(GetValue(RenderConfiguration()));
            if (resource) {
                resource->Connect(sceneHolder_);
            }
        }),
            reinterpret_cast<uint64_t>(this));

        if (auto rootNode = META_ACCESS_PROPERTY(RootNode)->GetValue()) {
            // switch new ecsObject
            BindNodeToEcs(rootNode, rootIdNormalized, false);
            nodes_[rootIdNormalized] = rootNode;
        } else {
            META_ACCESS_PROPERTY(RootNode)->SetValue(GetSelf<SCENE_NS::IScene>()->GetNode<SCENE_NS::INode>(rootId));
        }

        auto defaultCamera = META_ACCESS_PROPERTY(DefaultCamera)->GetValue();
        if (!defaultCamera) {
            defaultCamera = GetSelf<SCENE_NS::IScene>()->GetNode<SCENE_NS::ICamera>(cameraId);
            META_ACCESS_PROPERTY(DefaultCamera)->SetValue(defaultCamera);
        }

        if (defaultCamera) {
            ActivateCamera(defaultCamera);
        }

        // under normal conditions, this would take place asyncronously
        // however, if we are running on synchronous mode we need to restore the ecs bindings here
        for (auto& prevNode : nodes_) {
            if (prevNode.first != rootIdNormalized && prevNode.first != cameraId) {
                BindNodeToEcs(prevNode.second, prevNode.first, false);
            }
        }

        // This is information only unless we instantiate all the scene nodes immediately
        // Should be flagged more effectively on production builds
        CORE3D_NS::INodeSystem& nodeSystem = *CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*GetEcs());
        const auto& root = nodeSystem.GetRootNode();

        instantiateNodes(root, "/");

        // notify observers about the change
        META_ACCESS_PROPERTY(Status)->SetValue(SCENE_NS::IScene::SCENE_STATUS_READY);
        META_NS::Invoke<META_NS::IOnChanged>(OnLoaded());

        sceneHolder_->SetUninitializeCallback(
            META_NS::MakeCallback<SceneHolder::ISceneUninitialized>([me = BASE_NS::weak_ptr(GetSelf())]() {
                if (auto self = me.lock().get())
                    static_cast<SceneImpl*>(self)->DetachScene();
            }),
            sceneHolder_);
    }

    void OnSceneLoaded(uint32_t status)
    {
        META_ACCESS_PROPERTY(Status)->SetValue(status);
        if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
            META_NS::Invoke<META_NS::IOnChanged>(OnLoaded());
        }
    }

    // Implementation for scene processing.

    void DetachScene(bool setDirty = false)
    {
        if (META_ACCESS_PROPERTY(Status)) {
            META_ACCESS_PROPERTY(Status)->SetValue(SCENE_STATUS_UNINITIALIZED);
        }

        cameras_.clear();
        materials_.clear();
        meshes_.clear();
        animations_.clear();
        nodes_.clear();

        // This effectively means that we keep the scene user objects forcibly live
        // until we get fresh ones to replace them, not sure if that is intended
        rootNodePtr_ = META_ACCESS_PROPERTY(RootNode)->GetValue();
        cameraNodePtr_ = META_ACCESS_PROPERTY(DefaultCamera)->GetValue();
        META_ACCESS_PROPERTY(RootNode)->SetValue({});
        META_ACCESS_PROPERTY(DefaultCamera)->SetValue({});

        if (setDirty) {
            for (const auto& bitmap : bitmaps_) {
                /*auto externalBitmap = interface_pointer_cast<UI_NS::ILume2DExternalBitmap>(bitmap.second.bitmap);
                if (externalBitmap) {
                    externalBitmap->SetCoreBitmap(nullptr);
                }*/
                bitmap.second.bitmap->SetRenderHandle({}, { 0, 0 });
            }
        }
    }

    void instantiateNodes(const CORE3D_NS::ISceneNode& node, BASE_NS::string path)
    {
        if (!path.empty() && path.back() != '/') {
            path.append("/");
        }
        if (!node.GetName().empty()) {
            path.append(node.GetName());
        } else {
#ifndef INSTANTIATE_NODES_ON_INITIALIZE
            path.append("[");
            path.append(BASE_NS::to_string(node.GetEntity().id));
            path.append("]");
#endif
        }
        SCENE_PLUGIN_VERBOSE_LOG("%s", path.c_str());
#ifdef INSTANTIATE_NODES_ON_INITIALIZE
        GetNode(path);
#endif

        for (const auto child : node.GetChildren())
            instantiateNodes(*child, path);
    }

    BASE_NS::string NormalizePath(const BASE_NS::string_view& path)
    {
        BASE_NS::string patchedPath;
        if (path == rootNodeId_) {
            patchedPath = path;
            patchedPath.insert(0, "/");
        } else {
            bool hasSlash = (path[0] == '/');
            if (path.compare(hasSlash ? 1 : 0, rootNodeId_.size(), rootNodeId_.data())) {
                SCENE_PLUGIN_VERBOSE_LOG("%s: the path does not contain root node, patching the path", __func__);
                patchedPath.append("/");
                patchedPath.append(rootNodeId_);
                if (!hasSlash) {
                    patchedPath.append("/");
                }
                patchedPath.append(path.data(), path.size());
            } else {
                patchedPath = path;
                if (!hasSlash) {
                    patchedPath = "/" + patchedPath;
                }
            }
        }
        return patchedPath;
    }

    SCENE_NS::IEcsObject::Ptr FindEcsObject(const BASE_NS::string_view& path, BASE_NS::string& patchedPath)
    {
        patchedPath = NormalizePath(path);

        if (auto it = nodes_.find(patchedPath); it != nodes_.end()) {
            return interface_pointer_cast<SCENE_NS::IEcsObject>(it->second);
        }
        return SCENE_NS::IEcsObject::Ptr {};
    }

    SCENE_NS::IEcsObject::Ptr CreateNewEcsObject(const BASE_NS::string& /*path*/)
    {
        // Create new helper object
        // Not much left here, could be presumably removed
        auto ret = SCENE_NS::IEcsObject::Ptr {};

// Allow construction to progress even ecs was not available,
// nodes will perform initialization asynchronously anyway
        if (auto object = META_NS::GetObjectRegistry().Create(SCENE_NS::ClassId::EcsObject)) {
            ret = interface_pointer_cast<SCENE_NS::IEcsObject>(object);
        }
        return ret;
    }

    SCENE_NS::IEcsObject::Ptr GetEcsObject(const BASE_NS::string_view& path) override
    {
        return interface_pointer_cast<SCENE_NS::IEcsObject>(
            GetNode(path, META_NS::IObject::UID, SCENE_NS::INode::BuildBehavior::NODE_BUILD_CHILDREN_NO_BUILD));
    }

    CORE_NS::IEcs::Ptr GetEcs() override
    {
        return sceneHolder_->GetEcs();
    }

    CORE_NS::IEcs::Ptr GetEcs() const
    {
        return sceneHolder_->GetEcs();
    }

    META_NS::ITaskQueue::Token AddEngineTask(
        const META_NS::ITaskQueue::CallableType::Ptr& task, bool runDeferred) override
    {
        if (sceneHolder_) {
            return sceneHolder_->QueueEngineTask(task, runDeferred);
        }
        return META_NS::ITaskQueue::Token {};
    }

    META_NS::ITaskQueue::Token AddApplicationTask(
        const META_NS::ITaskQueue::CallableType::Ptr& task, bool runDeferred) override
    {
        if (sceneHolder_) {
            return sceneHolder_->QueueApplicationTask(task, runDeferred);
        }
        return META_NS::ITaskQueue::Token {};
    }

    void CancelEngineTask(META_NS::ITaskQueue::Token token) override
    {
        if (sceneHolder_) {
            return sceneHolder_->CancelEngineTask(token);
        }
    }

    void CancelAppTask(META_NS::ITaskQueue::Token token) override
    {
        if (sceneHolder_) {
            return sceneHolder_->CancelAppTask(token);
        }
    }

    SCENE_NS::IEntityCollection* GetEntityCollection() override
    {
        if (sceneHolder_) {
            return sceneHolder_->GetEntityCollection();
        }

        return {};
    }

    SCENE_NS::IAssetManager* GetAssetManager() override
    {
        if (sceneHolder_) {
            return sceneHolder_->GetAssetManager();
        }

        return {};
    }

    SCENE_NS::IMaterial::Ptr GetOrLoadMaterial(BASE_NS::string_view uri)
    {
        // check cache
        if (auto it = uriMaterials_.find(uri); it != uriMaterials_.end()) {
            if (auto material = it->second.lock()) {
                return material;
            }
        }

        auto ret = LoadMaterial(uri);
        uriMaterials_[uri] = ret;
        return ret;
    }

    SCENE_NS::IMaterial::Ptr LoadMaterial(BASE_NS::string_view uri) override
    {
        bool isGltfUri = (uri.find(".glb/materials/") != BASE_NS::string_view::npos) ||
                         (uri.find(".gltf/materials/") != BASE_NS::string_view::npos);
        if (isGltfUri) {
            auto resource = CreateResourceFromUri(SCENE_NS::ClassId::Material, uri);
            if (resource) {
                return interface_pointer_cast<SCENE_NS::IMaterial>(resource);
            }

            CORE_LOG_W("Could not resolve material URI: %s", BASE_NS::string(uri.data(), uri.size()).c_str());
            // we could return the control to CreateMaterial and let the engine resolve the materials with uri
            // componenta as they seem to be in place when loaded from glb or gltf
        }

        SCENE_NS::IMaterial::Ptr ret;

        return ret;
    }

    SCENE_NS::INode::Ptr CreateNode(const META_NS::ObjectId classId)
    {
        SCENE_NS::INode::Ptr node;

        if (classId == SCENE_NS::ClassId::Light.Id() || classId == SCENE_NS::ILight::UID) {
            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::Light);
        } else if (classId == SCENE_NS::ClassId::Camera.Id() || classId == SCENE_NS::ICamera::UID) {
            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::Camera);
        } /*else if (classId == SCENE_NS::ClassId::ViewNode.Id() || classId == SCENE_NS::IViewNode::UID) {
            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::ViewNode);
        } */
        else if (classId == SCENE_NS::ClassId::Material.Id() || classId == SCENE_NS::IMaterial::UID) {
            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::Material);
        } else if (classId == SCENE_NS::ClassId::Mesh.Id() || classId == SCENE_NS::IMesh::UID) {
            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::Mesh);
        } else if (classId == SCENE_NS::ClassId::Animation.Id() || classId == META_NS::IAnimation::UID) {
            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::Animation);
        } else if (classId == SCENE_NS::ClassId::Environment.Id() || classId == SCENE_NS::IEnvironment::UID) {
            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::Environment);
        } else {
            if (classId != META_NS::IObject::UID && classId != SCENE_NS::ClassId::Node.Id() &&
                classId != SCENE_NS::INode::UID) {
                CORE_LOG_W("%s: uid not known, returning INode instance", __func__);
            }

            node = GetObjectRegistry().Create<SCENE_NS::INode>(SCENE_NS::ClassId::Node);
        }

        return node;
    }

    // Quirk / constraints:
    // 1) has name -> cached and asynchronously initialized
    // 2) name empty, just an object, not sure if needed at the end
    SCENE_NS::INode::Ptr CreateNode(const BASE_NS::string_view name, bool createEngineObject,
        const META_NS::ObjectId classId, SCENE_NS::INode::BuildBehavior buildBehavior) override
    {
        if (const auto& ite = nodes_.find(name); ite != nodes_.cend() && createEngineObject) {
            CORE_LOG_W("Refusing to create new duplicate node: %s", BASE_NS::string(name.data(), name.size()).c_str());
            return ite->second;
        }

        SCENE_NS::INode::Ptr node = CreateNode(classId);

        if (node) {
            node->BuildChildren(buildBehavior);
            BindNodeToEcs(node, name, createEngineObject);
        }
        return node;
    }

    SCENE_NS::INode::Ptr CreateResourceFromUri(const META_NS::ObjectId classId, BASE_NS::string_view uri)
    {
        SCENE_NS::INode::Ptr node;

        auto entity = sceneHolder_->GetEntityByUri(uri);

        if (CORE_NS::EntityUtil::IsValid(entity)) {
            BASE_NS::string name;
            if (sceneHolder_->GetEntityName(entity, name)) {
                node = CreateNode(classId);
                if (node) {
                    auto ecsScene = GetSelf<SCENE_NS::IEcsScene>();
                    auto ecsObject = CreateNewEcsObject({});
                    auto nodeInterface = interface_pointer_cast<INodeEcsInterfacePrivate>(node);
                    nodeInterface->Initialize(ecsScene, ecsObject, {}, "", name, sceneHolder_, entity);
                }
            }
        }

        return node;
    }

    void BindNodeToEcs(
        SCENE_NS::INode::Ptr& node, const BASE_NS::string_view fullPath, bool createEngineObject) override
    {
        SCENE_PLUGIN_VERBOSE_LOG("Scene::BindNodeToEcs called for %s", BASE_NS::string(fullPath).c_str());

        CORE_NS::Entity entity;
        auto ecsScene = GetSelf<SCENE_NS::IEcsScene>();

        bool addToRootContainer { false };
        BASE_NS::string nodePath;
        BASE_NS::string nodeName;

        auto classUid = interface_cast<META_NS::IObject>(node)->GetClassId();
        bool isResourceClassType = (classUid == SCENE_NS::ClassId::Material) || // Material
                                   (classUid == SCENE_NS::ClassId::Mesh) ||     // Mesh
                                   (classUid == SCENE_NS::ClassId::Animation);  // Animation

        auto isRealUri = fullPath.find("://") != BASE_NS::string_view::npos;

        if (!isRealUri) {
            auto cutIx = fullPath.find_last_of('/');
            if (cutIx != BASE_NS::string_view::npos && cutIx < fullPath.size()) {
                ++cutIx;
                nodePath = BASE_NS::string(fullPath.data(), cutIx);
                nodeName = BASE_NS::string(fullPath.data() + cutIx, fullPath.size() - cutIx);
            } else if (!fullPath.empty()) {
                nodeName = BASE_NS::string(fullPath.data(), fullPath.size());
            }
        }

        if (nodeName.empty()) {
            if (createEngineObject) {
                nodeName = interface_cast<META_NS::IObjectInstance>(node)->GetInstanceId().ToString();
            } else {
                if (isRealUri) {
                    auto ecsObject = CreateNewEcsObject({});
                    auto nodeInterface = interface_cast<INodeEcsInterfacePrivate>(node);
                    nodeInterface->Initialize(ecsScene, ecsObject, {}, "", BASE_NS::string(fullPath), sceneHolder_, {});
                } else {
                    CORE_LOG_W("%s: refusing to create proxy object without valid target, name is empty.", __func__);
                }
                return;
            }
        }

        auto nodeInterface = interface_cast<INodeEcsInterfacePrivate>(node);

        if (nodeInterface->EcsObject() && CORE_NS::EntityUtil::IsValid(nodeInterface->EcsObject()->GetEntity())) {
            CORE_LOG_W("%s: refusing to recreate proxy object while current one is valid: %s", __func__,
                BASE_NS::string(fullPath).c_str());
            return;
        }

        auto ecsObject = CreateNewEcsObject({});

            // NEW NODE
        if (createEngineObject) {
            auto classUid = interface_pointer_cast<META_NS::IObject>(node)->GetClassId();
            auto constructNode = [this, nodePath, nodeName, classUid](
                                         const auto& sceneHolder) -> CORE_NS::Entity {
                    CORE_NS::Entity entity;
                if (sceneHolder) {
                    // Should not need to type this deep here
                    if (classUid == SCENE_NS::ClassId::Material) {
                        entity = sceneHolder->CreateMaterial(nodeName);
                    } else if (classUid == SCENE_NS::ClassId::Camera) {
                        entity = sceneHolder->CreateCamera(nodePath, nodeName, 0);
                    } else if (auto node = sceneHolder->CreateNode(nodePath, nodeName)) {
                        entity = node->GetEntity();

                        sceneHolder->EnableLayerComponent(entity);
                        if (classUid == SCENE_NS::ClassId::Light) {
                            sceneHolder->EnableLightComponent(entity);
                        }
                        if (classUid == SCENE_NS::ClassId::Environment) {
                            sceneHolder->EnableEnvironmentComponent(entity);
                        }
                    }
                }

                return entity;
            };
            if (GetValue(Asynchronous())) {
                AddEngineTask(MakeTask(
                                  [constructNode](const auto& sceneHolder) {
                                      constructNode(sceneHolder);
                                      return false;
                                  },
                                  sceneHolder_),
                    false);
            } else {
                entity = constructNode(sceneHolder_);
            }

            addToRootContainer = true;
            // If we don't have parent .. then attach to root.
            if (nodePath.empty() && !isResourceClassType) {
                nodePath = "/" + rootNodeId_ + "/";
            }

            if (META_NS::Property<uint32_t> creationPolicy = nodeInterface->GetLifecycleInfo()) {
                creationPolicy->SetValue(NODE_LC_CREATED);
                nodeInterface->ClaimOwnershipOfEntity(true);
            }
        } else {
            // We expect to find the node from ecs (sooner or later)
            // If we are running synchronously, we could check it and even
            // tell the calling code if the node is there.

            // We'd need to know some additional info about the node parent etc
            if (META_NS::Property<uint32_t> creationPolicy = nodeInterface->GetLifecycleInfo()) {
                creationPolicy->SetValue(NODE_LC_MIRROR_EXISTING);
            }
        }

        SCENE_NS::INode::Ptr parent;

        if (!isResourceClassType) {
            auto containable = interface_cast<META_NS::IContainable>(node);
            if (containable->GetParent()) {
                parent = interface_pointer_cast<SCENE_NS::INode>(containable->GetParent());
            }

            if (!parent) {
                parent = addToRootContainer ? RootNode()->GetValue() : currentParent_;
            }
        }

        nodeInterface->Initialize(ecsScene, ecsObject, parent, nodePath, nodeName, sceneHolder_, entity);
    }

    void UpdateCachedNodePath(const SCENE_NS::INode::Ptr& node) override
    {
        if (node) {
            if (interface_cast<META_NS::IObject>(node)->GetClassId() != SCENE_NS::ClassId::Node) {
                UpdateCachedReference(node);
                return;
            }

            bool found = false;

            for (auto&& ite : nodes_) {
                if (ite.second == node) {
                    nodes_.erase(ite.first);
                    break;
                }
            }

            BASE_NS::string pathString = node->Path()->GetValue();
            pathString.append(node->Name()->GetValue());
            nodes_[pathString] = node;
        }
    }

    void SetCacheEnabled(const SCENE_NS::INode::Ptr& node, bool enabled) override
    {
        if (!node) {
            return;
        }

        if (enabled) {
            BASE_NS::string pathString = node->Path()->GetValue();
            pathString.append(node->Name()->GetValue());
            nodes_[pathString] = node;
        } else {
            for (auto&& ite : nodes_) {
                if (ite.second == node) {
                    nodes_.erase(ite.first);
                    break;
                }
            }
        }
    }

    typedef BASE_NS::shared_ptr<CORE_NS::IInterface> (*fun)(SceneImpl* me, const BASE_NS::string_view);

    static BASE_NS::shared_ptr<CORE_NS::IInterface> relmat(SceneImpl* me, const BASE_NS::string_view p)
    {
        return me->ReleaseMaterial(p);
    }
    static BASE_NS::shared_ptr<CORE_NS::IInterface> relmesh(SceneImpl* me, const BASE_NS::string_view p)
    {
        return me->ReleaseMesh(p);
    }
    static BASE_NS::shared_ptr<CORE_NS::IInterface> relanim(SceneImpl* me, const BASE_NS::string_view p)
    {
        return me->ReleaseAnimation(p);
    }
    static BASE_NS::shared_ptr<CORE_NS::IInterface> relnode(SceneImpl* me, const BASE_NS::string_view p)
    {
        return me->ReleaseNode(p);
    }
    template<typename Type>
    bool CacheNode(const char* const tname, const SCENE_NS::INode::Ptr& node,
        BASE_NS::unordered_map<BASE_NS::string, typename Type::WeakPtr>& cache, fun func)
    {
        if (auto typed = interface_pointer_cast<Type>(node)) {
            BASE_NS::string uri;
            bool found = false;

            if (node->GetInterface(SCENE_NS::ICamera::UID)) {
                // handle camera naming..
                uri = node->Path()->GetValue();
                uri.append(node->Name()->GetValue());

            } else {
                auto ecsObject = interface_pointer_cast<SCENE_NS::IEcsObject>(typed);
                uri = sceneHolder_->GetResourceId(ecsObject->GetEntity());
                if (uri.empty()) {
                    uri = node->Name()->GetValue() + ":" + BASE_NS::to_hex(ecsObject->GetEntity().id);
                }
            }

            for (auto&& ite : cache) {
                if (ite.second.lock() == typed) {
                    if (uri != ite.first) {
                        cache[uri] = interface_pointer_cast<Type>(func(this, ite.first));
                        SCENE_PLUGIN_VERBOSE_LOG("Updating cached reference of %s: %s", tname, uri.c_str());
                    }
                    found = true;
                    break; // reference is valid so return true regardless if the node was moved
                }
            }

            if (!found) {
                SCENE_PLUGIN_VERBOSE_LOG("Caching reference of %s: %s", tname, uri.c_str());
                cache[uri] = typed;
            }
            return true;
        }
        return false;
    }
    void UpdateCachedReference(const SCENE_NS::INode::Ptr& node) override
    {
        if (node) {
            if (CacheNode<SCENE_NS::IMaterial>("material", node, materials_, relmat) ||
                CacheNode<SCENE_NS::IMesh>("mesh", node, meshes_, relmesh) ||
                CacheNode<META_NS::IAnimation>("animation", node, animations_, relanim)) {
                // completed. (meshes, materials and animations are not added to node list)
                return;
            }
            if (CacheNode<SCENE_NS::ICamera>("camera", node, cameras_, relnode)) {
                // camera cache updated.
                auto* obj = interface_cast<SCENE_NS::IEcsObject>(node);
                if (obj) {
                    auto entity = obj->GetEntity();
                    if (CORE_NS::EntityUtil::IsValid(entity)) {
                        sceneHolder_->AddCamera(entity);
                    } else {
                        CORE_LOG_V("camera has no entity id yet");
                    }
                } else {
                    CORE_LOG_V("camera has no IEcsObject");
                }
            }
            // update node cache.

            bool found = false;
            BASE_NS::string uri = node->Path()->GetValue();
            uri.append(node->Name()->GetValue());

            for (auto&& ite : nodes_) {
                if (ite.second == node) {
                    if (uri != ite.first) {
                        nodes_[uri] = ReleaseNode(ite.first);
                    }
                    found = true;
                    break; // reference is valid so return true regardless if the node was moved
                }
            }

            if (!found) {
                nodes_[uri] = node;
            }
        }
    }

    // Release Node Reference from cache
    SCENE_NS::INode::Ptr ReleaseNode(const BASE_NS::string_view name) override
    {
        SCENE_NS::INode::Ptr ret {};
        if (auto ite = nodes_.extract(BASE_NS::string(name)); !ite.empty()) {
            ret = BASE_NS::move(ite.mapped());
            if (auto privateIntf = interface_cast<INodeEcsInterfacePrivate>(ret)) {
                privateIntf->ClaimOwnershipOfEntity(true);
            }
            RemoveNodeFromCurrentContainer(ret);
            if (auto cam = interface_pointer_cast<SCENE_NS::ICamera>(ret)) {
                ReleaseCamera(name);
            }
        }
        return ret;
    }
    void ReleaseNode(const SCENE_NS::INode::Ptr& node) override
    {
        if (node) {
            ReleaseNode(node->Path()->GetValue() + node->Name()->GetValue());
        }
    }

    SCENE_NS::IMaterial::Ptr ReleaseMaterial(const BASE_NS::string_view name) override
    {
        SCENE_NS::IMaterial::Ptr ret {};
        if (auto ite = materials_.find(name); ite != materials_.end()) {
            ret = ite->second.lock();
            materials_.erase(ite);
        }
        return ret;
    }

    SCENE_NS::IMesh::Ptr ReleaseMesh(const BASE_NS::string_view name) override
    {
        SCENE_NS::IMesh::Ptr ret {};
        if (auto ite = meshes_.find(name); ite != meshes_.end()) {
            ret = ite->second.lock();
            meshes_.erase(ite);
        }

        return ret;
    }

    META_NS::IAnimation::Ptr ReleaseAnimation(const BASE_NS::string_view name) override
    {
        META_NS::IAnimation::Ptr ret {};
        if (auto ite = animations_.find(name); ite != animations_.end()) {
            ret = ite->second.lock();
            animations_.erase(ite);
        }
        return ret;
    }

    SCENE_NS::ICamera::Ptr ReleaseCamera(const BASE_NS::string_view name)
    {
        SCENE_NS::ICamera::Ptr ret {};
        if (auto ite = cameras_.find(name); ite != cameras_.end()) {
            ret = ite->second.lock();
            // make sure the rendertarget/bitmap is also released.
            SetBitmap(nullptr, ret);
            cameras_.erase(ite);
        }
        return ret;
    }

    SCENE_NS::IMesh::Ptr CreateMeshFromArraysI16(
        const BASE_NS::string_view name, SCENE_NS::MeshGeometryArrayPtr<uint16_t> arrays) override
    {
        return CreateMeshFromArrays<uint16_t>(name, arrays, RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT16);
    }

    SCENE_NS::IMesh::Ptr CreateMeshFromArraysI32(
        const BASE_NS::string_view name, SCENE_NS::MeshGeometryArrayPtr<uint32_t> arrays) override
    {
        return CreateMeshFromArrays<uint32_t>(name, arrays, RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT32);
    }

    template<typename IndicesType>
    SCENE_NS::IMesh::Ptr CreateMeshFromArrays(const BASE_NS::string_view name,
        SCENE_NS::MeshGeometryArrayPtr<IndicesType> arrays, RENDER_NS::IndexType indexType)
    {
        auto nameString = BASE_NS::shared_ptr(new BASE_NS::string(name.data(), name.size()));

        AddEngineTask(MakeTask(
                          [arrays, nameString, indexType](auto sceneHolder) {
                              auto meshEntity = sceneHolder->template CreateMeshFromArrays<IndicesType>(
                                  *nameString, arrays, indexType);
                              if (!sceneHolder->IsAsync()) {
                                  *nameString = sceneHolder->GetResourceId(meshEntity);
                              }

                              return false;
                          },
                          sceneHolder_),
            false);
        return GetMesh(*nameString);
    }

    void InstantiateMaterialProxies() override
    {
        AddEngineTask(MakeTask(
                          [me = BASE_NS::weak_ptr(GetSelf<SCENE_NS::IScene>())](auto sceneHolder) {
                              auto ids = sceneHolder->ListMaterialNames();
                              sceneHolder->QueueApplicationTask(MakeTask(
                                                                    [ids](auto self) {
                                                                        for (auto& id : *ids) {
                                                                            self->GetMaterial(id);
                                                                        }
                                                                        return false;
                                                                    },
                                                                    me),
                                  false);
                              return false;
                          },
                          sceneHolder_),
            false);
    }

    void InstantiateMeshProxies() override
    {
        AddEngineTask(MakeTask(
                          [me = BASE_NS::weak_ptr(GetSelf<SCENE_NS::IScene>())](auto sceneHolder) {
                              auto ids = sceneHolder->ListMeshNames();
                              sceneHolder->QueueApplicationTask(MakeTask(
                                                                    [ids](auto self) {
                                                                        for (auto& id : *ids) {
                                                                            self->GetMesh(id);
                                                                        }

                                                                        return false;
                                                                    },
                                                                    me),
                                  false);

                              return false;
                          },
                          sceneHolder_),
            false);
    }

public:
    ~SceneImpl() override
    {
        allAnims_.clear();
        if (sceneHolder_) {
            DetachScene();
        }

        cameraNodePtr_.reset();
        rootNodePtr_.reset();

        UnsubscribeFromPropertyChanges();

        if (sceneHolder_) {
            sceneHolder_->Uninitialize();
            sceneHolder_.reset();
        }
    }

    void SetEcsInitializationCallback(IPrepareSceneForInitialization::WeakPtr callback) override
    {
        if (sceneHolder_) {
            sceneHolder_->SetEcsInitializationCallback(callback);
        } else {
            CORE_LOG_W("%s: sceneholder does not exist, cannot set callback", __func__);
        }
    }

    SCENE_NS::IPickingResult::Ptr GetWorldAABB(const BASE_NS::Math::Mat4X4& world, const BASE_NS::Math::Vec3& aabbMin,
        const BASE_NS::Math::Vec3& aabbMax) override
    {
        auto ret = META_NS::GetObjectRegistry().Create<SCENE_NS::IPickingResult>(SCENE_NS::ClassId::PendingVec3Request);
        if (ret) {
            AddEngineTask(
                META_NS::MakeCallback<META_NS::ITaskQueueTask>([w = world, min = aabbMin, max = aabbMax,
                                                                   weakRet = BASE_NS::weak_ptr(ret),
                                                                   weakSh = BASE_NS::weak_ptr(sceneHolder_)]() {
                    if (auto sh = weakSh.lock()) {
                        if (auto ret = weakRet.lock()) {
                            if (sh->GetWorldAABB(ret, w, min, max)) {
                                sh->QueueApplicationTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet]() {
                                    if (auto writable =
                                            interface_pointer_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(
                                                weakRet)) {
                                        writable->MarkReady();
                                    }
                                    return false;
                                }),
                                    false);
                            }
                        }
                    }
                    return false;
                }),
                false);
            return ret;
        }
        return SCENE_NS::IPickingResult::Ptr();
    }

    SCENE_NS::IRayCastResult::Ptr RayCast(
        const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction) override
    {
        auto ret =
            META_NS::GetObjectRegistry().Create<SCENE_NS::IRayCastResult>(SCENE_NS::ClassId::PendingDistanceRequest);
        if (ret) {
            AddEngineTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([origin = start, dir = direction,
                                                                             weakRet = BASE_NS::weak_ptr(ret),
                                                                             weakSh = BASE_NS::weak_ptr(sceneHolder_),
                                                                             weakSelf = BASE_NS::weak_ptr(
                                                                                 GetSelf<SCENE_NS::IScene>())]() {
                if (auto sh = weakSh.lock()) {
                    if (auto ret = weakRet.lock()) {
                        if (sh->RayCast(ret, origin, dir)) {
                            sh->QueueApplicationTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet,
                                                                                                        weakSelf]() {
                                if (auto writable =
                                        interface_pointer_cast<SCENE_NS::IPendingRequestData<SCENE_NS::NodeDistance>>(
                                            weakRet)) {
                                    if (auto self = weakSelf.lock()) {
                                        // resolve proxy nodes
                                        for (size_t ii = writable->MetaData().size(); ii > 0;) {
                                            --ii;
                                            writable->MutableData().at(ii).node =
                                                self->GetNode(writable->MetaData().at(ii));
                                        }
                                        writable->MarkReady();
                                    }
                                }
                                return false;
                            }),
                                false);
                        }
                    }
                }
                return false;
            }),
                false);
            return ret;
        }
        return SCENE_NS::IRayCastResult::Ptr();
    }

    SCENE_NS::IRayCastResult::Ptr RayCast(
        const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction, uint64_t layerMask) override
    {
        auto ret =
            META_NS::GetObjectRegistry().Create<SCENE_NS::IRayCastResult>(SCENE_NS::ClassId::PendingDistanceRequest);
        if (ret) {
            AddEngineTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([origin = start, dir = direction, layerMask,
                                                                             weakRet = BASE_NS::weak_ptr(ret),
                                                                             weakSh = BASE_NS::weak_ptr(sceneHolder_),
                                                                             weakSelf = BASE_NS::weak_ptr(
                                                                                 GetSelf<SCENE_NS::IScene>())]() {
                if (auto sh = weakSh.lock()) {
                    if (auto ret = weakRet.lock()) {
                        if (sh->RayCast(ret, origin, dir, layerMask)) {
                            sh->QueueApplicationTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet,
                                                                                                        weakSelf]() {
                                if (auto writable =
                                        interface_pointer_cast<SCENE_NS::IPendingRequestData<SCENE_NS::NodeDistance>>(
                                            weakRet)) {
                                    if (auto self = weakSelf.lock()) {
                                        // resolve proxy nodes
                                        for (size_t ii = writable->MetaData().size(); ii > 0;) {
                                            --ii;
                                            writable->MutableData().at(ii).node =
                                                self->GetNode(writable->MetaData().at(ii));
                                        }
                                        writable->MarkReady();
                                    }
                                }
                                return false;
                            }),
                                false);
                        }
                    }
                }
                return false;
            }),
                false);
            return ret;
        }
        return SCENE_NS::IRayCastResult::Ptr();
    }
    BASE_NS::vector<CORE_NS::Entity> RenderCameras() override
    {
        if (sceneHolder_) {
            return sceneHolder_->RenderCameras();
        }
        return {};
    }
    void ActivateCamera(const SCENE_NS::ICamera::Ptr& camera) override
    {
        if (auto e = interface_pointer_cast<SCENE_NS::IEcsObject>(camera)) {
            auto ent = e->GetEntity();
            sceneHolder_->ActivateCamera(ent);
        }
    }

    void DeactivateCamera(const SCENE_NS::ICamera::Ptr& camera) override
    {
        if (!camera) {
            return;
        }

        if (auto e = interface_pointer_cast<SCENE_NS::IEcsObject>(camera)) {
            sceneHolder_->DeactivateCamera(e->GetEntity());
        }
    }
    bool IsCameraActive(const SCENE_NS::ICamera::Ptr& camera) override
    {
        if (!camera) {
            return false;
        }

        if (auto e = interface_pointer_cast<SCENE_NS::IEcsObject>(camera)) {
            return sceneHolder_->IsCameraActive(e->GetEntity());
        }
        return false;
    }

private:
    void SubscribeToPropertyChanges()
    {
        if (RootNode()) {
            rootNodeChangedToken_ = RootNode()->OnChanged()->AddHandler(
                META_NS::MakeCallback<META_NS::IOnChanged>(this, &SceneImpl::OnRootNodeChanged));
        }

        if (Uri()) {
            uriHandlerToken_ = Uri()->OnChanged()->AddHandler(
                META_NS::MakeCallback<META_NS::IOnChanged>([this]() { this->Load(Uri()->GetValue()); }));
        }

        // Start listening changes of the scene controller properties. These may go to different place some day
        if (SystemGraphUri()) {
            systemGraphUriHandlerToken_ = SystemGraphUri()->OnChanged()->AddHandler(
                META_NS::MakeCallback<META_NS::IOnChanged>(this, &SceneImpl::onSystemGraphUriChanged));
        }
    }

    void UnsubscribeFromPropertyChanges()
    {
        if (Uri()) {
            Uri()->OnChanged()->RemoveHandler(uriHandlerToken_);
            uriHandlerToken_ = {};
        }

        if (RootNode()) {
            RootNode()->OnChanged()->RemoveHandler(rootNodeChangedToken_);
            rootNodeChangedToken_ = {};
        }

        if (SystemGraphUri()) {
            SystemGraphUri()->OnChanged()->RemoveHandler(systemGraphUriHandlerToken_);
            systemGraphUriHandlerToken_ = {};
        }

        if (Asynchronous()) {
            Asynchronous()->OnChanged()->RemoveHandler(asyncChangedToken_);
            asyncChangedToken_ = {};
        }
    }

    void OnRootNodeChanged()
    {
        auto contentObject = interface_pointer_cast<META_NS::IObject>(RootNode()->GetValue());
        contentImpl_->SetContent(contentObject);

        META_NS::HierarchyChangeModeValue changeMode;
        changeMode.Set(META_NS::HierarchyChangeMode::NOTIFY_CONTAINER);
        hierarchyController_->SetTarget(contentObject, changeMode);
    }

    META_NS::IEvent::Token systemGraphUriHandlerToken_ {};
    META_NS::IEvent::Token renderSizeHandlerToken_ {};
    META_NS::IEvent::Token cameraHandlerToken_ {};
    META_NS::IEvent::Token uriHandlerToken_ {};
    META_NS::IEvent::Token rootNodeChangedToken_ {};
    META_NS::IEvent::Token renderModeChangedToken_ {};
    META_NS::IEvent::Token asyncChangedToken_ {};

    SceneHolder::Ptr sceneHolder_;

    BASE_NS::string rootNodeId_;
    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::INode::Ptr> nodes_;
    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::ICamera::WeakPtr> cameras_;
    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::IMesh::WeakPtr> meshes_;
    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::IMaterial::WeakPtr> materials_;
    BASE_NS::unordered_map<BASE_NS::string, META_NS::IAnimation::WeakPtr> animations_;

    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::IMaterial::WeakPtr> uriMaterials_;

    uint64_t instanceNumber_ { 0 };

    // Preserve property instances while scene / ecs is invalid
    SCENE_NS::INode::Ptr rootNodePtr_ {};
    SCENE_NS::ICamera::Ptr cameraNodePtr_ {};
    META_NS::IContent::Ptr contentImpl_ {};

    uint64_t defaultCameraHandle_ { 0 };

    // We need to add a node onto a container in bit awkward position (without exposing it to a public api)
    // Store it here.
    SCENE_NS::INode::Ptr currentParent_ {};
    META_NS::IObjectHierarchyObserver::Ptr hierarchyController_;

    META_NS::IAnimationController::Ptr animationController_;
};

BASE_NS::vector<BASE_NS::weak_ptr<META_NS::IAnimation>> SceneImpl::GetAnimations() const
{
    return animationController_->GetAnimations();
}
BASE_NS::vector<BASE_NS::weak_ptr<META_NS::IAnimation>> SceneImpl::GetRunning() const
{
    return animationController_->GetRunning();
}
bool SceneImpl::AddAnimation(const BASE_NS::shared_ptr<META_NS::IAnimation>& animation)
{
    return animationController_->AddAnimation(animation);
}
bool SceneImpl::RemoveAnimation(const BASE_NS::shared_ptr<META_NS::IAnimation>& animation)
{
    return animationController_->RemoveAnimation(animation);
}
void SceneImpl::Clear()
{
    animationController_->Clear();
}
META_NS::IAnimationController::StepInfo SceneImpl::Step(const META_NS::IClock::ConstPtr& clock)
{
    return animationController_->Step(clock);
}

} // namespace
SCENE_BEGIN_NAMESPACE()
void RegisterSceneImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<SceneImpl>();
}
void UnregisterSceneImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<SceneImpl>();
}
SCENE_END_NAMESPACE()
