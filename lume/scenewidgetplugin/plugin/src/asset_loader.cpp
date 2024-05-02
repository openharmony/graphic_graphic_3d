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

#include "asset_loader.h"

#include <algorithm>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <base/containers/fixed_string.h>
#include <base/containers/vector.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#include "asset_manager.h"
#include "asset_migration.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

SCENE_BEGIN_NAMESPACE()

namespace {

struct IoUtil {
    static bool LoadTextFile(string_view fileUri, string& fileContentsOut, IFileManager& fileManager)
    {
        auto file = fileManager.OpenFile(fileUri);
        if (file) {
            const size_t length = file->GetLength();
            fileContentsOut.resize(length);
            return file->Read(fileContentsOut.data(), length) == length;
        }
        return false;
    }
};

// Add a node and all of its children recursively to a collection
void AddNodeToCollectionRecursive(IEntityCollection& ec, ISceneNode& node, string_view path)
{
    const auto entity = node.GetEntity();
    const auto currentPath = path + node.GetName();

    EntityReference ref = ec.GetEcs().GetEntityManager().GetReferenceCounted(entity);
    ec.AddEntity(ref);
    ec.SetId(currentPath, ref);

    const auto childBasePath = currentPath + "/";
    for (auto* child : node.GetChildren()) {
        AddNodeToCollectionRecursive(ec, *child, childBasePath);
    }
}
} // namespace

// A class that executes asset load operation over multiple frames.
class AssetLoader final : public IAssetLoader, private IAssetLoader::IListener, private IGLTF2Importer::Listener {
public:
    AssetLoader(AssetManager& assetManager, IRenderContext& renderContext, IGraphicsContext& graphicsContext,
        IEntityCollection& ec, string_view src, string_view contextUri)
        : assetManager_(assetManager), renderContext_(renderContext), graphicsContext_(graphicsContext),
          ecs_(ec.GetEcs()), ec_(ec), src_(src), contextUri_(contextUri),
          fileManager_(renderContext_.GetEngine().GetFileManager())
    {}

    ~AssetLoader() override
    {
        Cancel();
    }

    IEntityCollection& GetEntityCollection() const override
    {
        return ec_;
    }

    string GetSrc() const override
    {
        return src_;
    }

    string GetContextUri() const override
    {
        return contextUri_;
    }

    string GetUri() const override
    {
        return PathUtil::ResolveUri(contextUri_, src_, true);
    }

    void AddListener(IAssetLoader::IListener& listener) override
    {
        CORE_ASSERT(&listener);
        listeners_.emplace_back(&listener);
    }

    void RemoveListener(IAssetLoader::IListener& listener) override
    {
        CORE_ASSERT(&listener);
        for (size_t i = 0; i < listeners_.size(); ++i) {
            if (&listener == listeners_[i]) {
                listeners_.erase(listeners_.begin() + i);
                return;
            }
        }

        // trying to remove a non-existent listener.
        CORE_ASSERT(true);
    }

    void LoadAsset() override
    {
        async_ = false;
        StartLoading();
    }

    void LoadAssetAsync() override
    {
        async_ = true;
        StartLoading();
    }

    IAssetLoader* GetNextDependency() const
    {
        if (dependencies_.empty()) {
            return nullptr;
        }
        for (const auto& dep : dependencies_) {
            if (dep && !dep->IsCompleted()) {
                return dep.get();
            }
        }
        return nullptr;
    }

    //
    // From IAssetLoader::Listener
    //
    void OnLoadFinished(const IAssetLoader& loader) override
    {
        // This will be called when a dependency has finished loading.

        // Hide cached data.
        loader.GetEntityCollection().SetActive(false);

        auto* dep = GetNextDependency();
        if (dep) {
            // Load next dependency.
            ContinueLoading();
        } else {
            // There are no more dependencies. Try loading the main asset again but now with the dependencies loaded.
            StartLoading();
        }
    }

    //
    // From CORE_NS::IGLTF2Importer::Listener
    //
    void OnImportStarted() override {}
    void OnImportFinished() override
    {
        GltfImportFinished();
        if (async_) {
            ContinueLoading();
        }
    }
    void OnImportProgressed(size_t taskIndex, size_t taskCount) override {}

    bool Execute(uint32_t timeBudget) override
    {
        // NOTE: Currently actually only one import will be active at a time so we don't need to worry about the time
        // budget.
        bool done = true;

        for (auto& dep : dependencies_) {
            if (dep) {
                done &= dep->Execute(timeBudget);
            }
        }
        if (importer_) {
            done &= importer_->Execute(timeBudget);
        }
        return done;
    }

    void Cancel() override
    {
        cancelled_ = true;
        for (auto& dep : dependencies_) {
            if (dep) {
                dep->Cancel();
            }
        }

        if (importer_) {
            importer_->Cancel();
            importer_.reset();
        }
    }

    bool IsCompleted() const override
    {
        return done_;
    }

    Result GetResult() const override
    {
        return result_;
    }

    void Destroy() override
    {
        delete this;
    }

private:
    void StartLoading()
    {
        if (src_.empty()) {
            result_ = { false, "Ivalid uri" };
            done_ = true;
            return;
        }

        const auto resolvedUri = GetUri();

        const auto resolvedFile = PathUtil::ResolveUri(contextUri_, src_, false);
        const auto ext = PathUtil::GetExtension(resolvedFile);
        const auto type = assetManager_.GetExtensionType(ext);
        // TODO: Separate different loaders and make it possible to register more.
        switch (type) {
            case IAssetManager::ExtensionType::COLLECTION: {
                ec_.SetType("entitycollection");
                if (LoadJsonEntityCollection()) {
                    MigrateAnimation(ec_);
                }
                break;
            }

            case IAssetManager::ExtensionType::SCENE: {
                ec_.SetType("scene");
                if (LoadJsonEntityCollection()) {
                    MigrateAnimation(ec_);
                }
                break;
            }

            case IAssetManager::ExtensionType::ANIMATION: {
                ec_.SetType("animation");
                if (LoadJsonEntityCollection()) {
                    MigrateAnimation(ec_);
                }
                break;
            }

            case IAssetManager::ExtensionType::MATERIAL: {
                ec_.SetType("material");
                LoadJsonEntityCollection();
                break;
            }

            case IAssetManager::ExtensionType::GLTF:
            case IAssetManager::ExtensionType::GLB: {
                ec_.SetType("gltf");
                LoadGltfEntityCollection();
                break;
            }

            case IAssetManager::ExtensionType::JPG:
            case IAssetManager::ExtensionType::PNG:
            case IAssetManager::ExtensionType::KTX: {
                ec_.SetType("image");
                LoadImageEntityCollection();
                break;
            }

            case IAssetManager::ExtensionType::NOT_SUPPORTED:
            default: {
                CORE_LOG_E("Unsupported asset format: '%s'", src_.c_str());
                result_ = { false, "Unsupported asset format" };
                done_ = true;
                break;
            }
        }

        ContinueLoading();
    }

    void ContinueLoading()
    {
        if (done_) {
            ec_.MarkModified(false, true);

            for (auto* listener : listeners_) {
                listener->OnLoadFinished(*this);
            }
            return;
        }

        auto* dep = GetNextDependency();
        if (dep) {
            if (async_) {
                dep->LoadAssetAsync();
            } else {
                dep->LoadAsset();
            }
            return;
        }
    }

    void CreateDummyEntity()
    {
        // Create a dummy root entity as a placeholder for a missing asset.
        auto dummyEntity = ecs_.GetEntityManager().CreateReferenceCounted();
        // Note: adding a node component for now so it will be displayed in the hierarchy pane.
        // This is wrong as the failed asset might not be a 3D node in reality. this could be removed after combining
        // the Entity pane functionlity to the hierarchy pane and when there is better handling for missing references.
        auto* nodeComponentManager = GetManager<INodeComponentManager>(ecs_);
        CORE_ASSERT(nodeComponentManager);
        if (nodeComponentManager) {
            nodeComponentManager->Create(dummyEntity);
        }
        if (!GetUri().empty()) {
            auto* nameComponentManager = GetManager<INameComponentManager>(ecs_);
            CORE_ASSERT(nameComponentManager);
            if (nameComponentManager) {
                nameComponentManager->Create(dummyEntity);
                nameComponentManager->Write(dummyEntity)->name = GetUri();
            }
        }
        ec_.AddEntity(dummyEntity);
        ec_.SetId("/", dummyEntity);
    }

    bool LoadJsonEntityCollection()
    {
        const auto resolvedUri = GetUri();
        const auto resolvedFile = PathUtil::ResolveUri(contextUri_, src_, false);

        const auto params = PathUtil::GetUriParameters(src_);
        const auto targetParam = params.find("target");
        string entityTarget = (targetParam != params.cend()) ? targetParam->second : "";
        auto targetEntity = ec_.GetEntity(entityTarget);
        size_t subcollectionCount = ec_.GetSubCollectionCount();
        while (!targetEntity && subcollectionCount) {
            targetEntity = ec_.GetSubCollection(--subcollectionCount)->GetEntity(entityTarget);
        }

        string textIn;
        if (IoUtil::LoadTextFile(resolvedFile, textIn, fileManager_)) {
            // TODO: Check file version here.

            auto json = json::parse(textIn.data());
            if (!json) {
                CORE_LOG_E("Parsing json failed: '%s':\n%s", resolvedUri.c_str(), textIn.c_str());
            } else {
                if (!dependencies_.empty()) {
                    // There were dependencies loaded earlier and now we want to load the actual asset.
                    // No dependencies to load. Just load the entity collection itself and this loading is done.
                    auto result = assetManager_.GetEcsSerializer().ReadEntityCollection(ec_, json, resolvedUri);
                    done_ = true;
                    return result;
                }

                vector<IEcsSerializer::ExternalCollection> dependencies;
                assetManager_.GetEcsSerializer().GatherExternalCollections(json, resolvedUri, dependencies);

                for (const auto& dep : dependencies) {
                    BASE_NS::string patchedDepUri = dep.src;
                    if (!entityTarget.empty())
                        patchedDepUri.append("?target=").append(entityTarget);
                    if (!assetManager_.IsCachedCollection(patchedDepUri, dep.contextUri)) {
                        auto* cacheEc =
                            assetManager_.CreateCachedCollection(ec_.GetEcs(), patchedDepUri, dep.contextUri);
                        dependencies_.emplace_back(
                            assetManager_.CreateAssetLoader(*cacheEc, patchedDepUri, dep.contextUri));
                        auto& dep = *dependencies_.back();
                        dep.AddListener(*this);
                    }
                }

                if (GetNextDependency() == nullptr) {
                    // No dependencies to load. Just load the entity collection itself and this loading is done.
                    auto result = assetManager_.GetEcsSerializer().ReadEntityCollection(ec_, json, resolvedUri);
                    done_ = true;
                    return result;
                }

                // There are dependencies that need to be parsed in a next step.
                return true;
            }
        }

        CreateDummyEntity();
        result_ = { false, "collection loading failed." };
        done_ = true;
        return false;
    }

    void LoadGltfEntityCollection()
    {
        const auto resolvedFile = PathUtil::ResolveUri(contextUri_, src_, false);

        auto& gltf = graphicsContext_.GetGltf();

        loadResult_ = gltf.LoadGLTF(resolvedFile);
        if (!loadResult_.success) {
            CORE_LOG_E("Loaded '%s' with errors:\n%s", resolvedFile.c_str(), loadResult_.error.c_str());
        }
        if (!loadResult_.data) {
            CreateDummyEntity();
            result_ = { false, "glTF load failed." };
            done_ = true;
            return;
        }

        importer_ = gltf.CreateGLTF2Importer(ec_.GetEcs());

        if (async_) {
            importer_->ImportGLTFAsync(*loadResult_.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, this);
        } else {
            importer_->ImportGLTF(*loadResult_.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
            OnImportFinished();
        }
    }

    Entity ImportSceneFromGltf(EntityReference root)
    {
        auto& gltf = graphicsContext_.GetGltf();

        // Import the default scene, or first scene if there is no default scene set.
        size_t sceneIndex = loadResult_.data->GetDefaultSceneIndex();
        if (sceneIndex == CORE_GLTF_INVALID_INDEX && loadResult_.data->GetSceneCount() > 0) {
            // Use first scene.
            sceneIndex = 0;
        }

        const CORE3D_NS::GLTFResourceData& resourceData = importer_->GetResult().data;
        Entity importedSceneEntity {};
        if (sceneIndex != CORE_GLTF_INVALID_INDEX) {
            GltfSceneImportFlags importFlags = CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL;
            importedSceneEntity =
                gltf.ImportGltfScene(sceneIndex, *loadResult_.data, resourceData, ecs_, root, importFlags);
        }
        if (!EntityUtil::IsValid(importedSceneEntity)) {
            return {};
        }

        // make sure everyone has a name (rest of the system fails if there are unnamed nodes)
        auto* nodeComponentManager = GetManager<INodeComponentManager>(ecs_);
        for (auto i = 0; i < nodeComponentManager->GetComponentCount(); i++) {
            auto ent = nodeComponentManager->GetEntity(i);
            auto* ncm = GetManager<INameComponentManager>(ecs_);
            bool setName = true;
            if (ncm->HasComponent(ent)) {
                // check if empty
                auto name = ncm->Get(ent).name;
                if (!name.empty()) {
                    // it has a non empty name..
                    setName = false;
                }
            }
            if (setName) {
                // no name component, so create one create one.
                char buf[256];
                // possibly not unique enough. user created names could conflict.
                sprintf(buf, "Unnamed Node %d", i);
                ncm->Set(ent, { buf });
            }
        }

        // Link animation tracks to targets
        if (!resourceData.animations.empty()) {
            INodeSystem* nodeSystem = GetSystem<INodeSystem>(ecs_);
            if (auto animationRootNode = nodeSystem->GetNode(importedSceneEntity); animationRootNode) {
                for (const auto& animationEntity : resourceData.animations) {
                    UpdateTrackTargets(animationEntity, animationRootNode);
                }
            }
        }

        return importedSceneEntity;
    }

    void UpdateTrackTargets(Entity animationEntity, ISceneNode* node)
    {
        auto& nameManager_ = *GetManager<INameComponentManager>(ecs_);
        auto animationManager = GetManager<IAnimationComponentManager>(ecs_);
        auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(ecs_);
        auto& entityManager = ecs_.GetEntityManager();

        if (const ScopedHandle<const AnimationComponent> animationData = animationManager->Read(animationEntity);
            animationData) {
            vector<Entity> targetEntities;
            targetEntities.reserve(animationData->tracks.size());
            std::transform(animationData->tracks.begin(), animationData->tracks.end(),
                std::back_inserter(targetEntities), [&nameManager = nameManager_, &node](const Entity& trackEntity) {
                    if (auto nameHandle = nameManager.Read(trackEntity); nameHandle) {
                        if (auto targetNode = node->LookupNodeByPath(nameHandle->name); targetNode) {
                            return targetNode->GetEntity();
                        }
                    }
                    return Entity {};
                });
            if (animationData->tracks.size() == targetEntities.size()) {
                auto targetIt = targetEntities.begin();
                for (const auto& trackEntity : animationData->tracks) {
                    if (auto track = animationTrackManager->Write(trackEntity); track) {
                        if (track->target) {
                            SCENE_PLUGIN_VERBOSE_LOG("AnimationTrack %s already targetted",
                                to_hex(static_cast<const Entity&>(track->target).id).data());
                        }
                        track->target = entityManager.GetReferenceCounted(*targetIt);
                    }
                    ++targetIt;
                }
            }
        }
    }

    void GltfImportFinished()
    {
        auto* nodeSystem = GetSystem<INodeSystem>(ecs_);
        CORE_ASSERT(nodeSystem);
        auto* nodeComponentManager = GetManager<INodeComponentManager>(ecs_);
        CORE_ASSERT(nodeComponentManager);
        if (!nodeSystem || !nodeComponentManager) {
            result_ = { false, {} };
            done_ = true;
            return;
        }

        const auto params = PathUtil::GetUriParameters(src_);
        const auto rootParam = params.find("root");
        string entityPath = (rootParam != params.cend()) ? rootParam->second : "";

        const auto targetParam = params.find("target");
        string entityTarget = (targetParam != params.cend()) ? targetParam->second : "";
        auto targetEntity = ec_.GetEntity(entityTarget);
        size_t subcollectionCount = ec_.GetSubCollectionCount();
        while (!targetEntity && subcollectionCount) {
            targetEntity = ec_.GetSubCollection(--subcollectionCount)->GetEntity(entityTarget);
        }

        if (importer_) {
            auto const gltfImportResult = importer_->GetResult();
            if (!gltfImportResult.success) {
                CORE_LOG_E("Importing of '%s' failed: %s", GetUri().c_str(), gltfImportResult.error.c_str());
                CreateDummyEntity();
                result_ = { false, "glTF import failed." };
                done_ = true;
                return;

            } else if (cancelled_) {
                CORE_LOG_V("Importing of '%s' cancelled", GetUri().c_str());
                CreateDummyEntity();
                result_ = { false, "glTF import cancelled." };
                done_ = true;
                return;
            }

            // Loading and importing of glTF was done successfully. Fill the collection with all the gltf entities.
            const auto originalRootEntity = ImportSceneFromGltf(targetEntity);
            auto* originalRootNode = nodeSystem->GetNode(originalRootEntity);

            // It's possible to only add some specific node from the gltf.
            auto* loadNode = originalRootNode;
            if (!entityPath.empty()) {
                loadNode = originalRootNode->LookupNodeByPath(entityPath);
                if (!loadNode || loadNode->GetEntity() == Entity {}) {
                    CORE_LOG_E("Entity '%s' not found from '%s'", entityPath.c_str(), GetUri().c_str());
                }
            }

            if (!loadNode || loadNode->GetEntity() == Entity {}) {
                CreateDummyEntity();
                result_ = { false, "Ivalid uri" };
                done_ = true;
                return;
            }

            Entity entity = loadNode->GetEntity();
            if (entity != Entity {}) {
                EntityReference ref = ecs_.GetEntityManager().GetReferenceCounted(entity);
                ec_.AddEntity(ref);
                ec_.SetId("/", ref);

                if (!targetEntity) {
                    loadNode->SetParent(nodeSystem->GetRootNode());
                }
                for (auto* child : loadNode->GetChildren()) {
                    AddNodeToCollectionRecursive(ec_, *child, "/");
                }
            }

            // TODO: a little backwards to first create everything and then delete the extra.
            if (entity != originalRootEntity) {
                auto* oldRoot = nodeSystem->GetNode(originalRootEntity);
                CORE_ASSERT(oldRoot);
                if (oldRoot) {
                    nodeSystem->DestroyNode(*oldRoot);
                }
            }

            // Add all resources in separate sub-collections. Not just 3D nodes.
            {
                const auto& importResult = importer_->GetResult();
                ec_.AddSubCollection("images", {}).AddEntities(importResult.data.images);
                ec_.AddSubCollection("materials", {}).AddEntities(importResult.data.materials);
                ec_.AddSubCollection("meshes", {}).AddEntities(importResult.data.meshes);
                ec_.AddSubCollection("skins", {}).AddEntities(importResult.data.skins);
                ec_.AddSubCollection("animations", {}).AddEntities(importResult.data.animations);

                // TODO: don't list duplicates
                vector<EntityReference> animationTracks;
                auto* acm = GetManager<IAnimationComponentManager>(ecs_);
                if (acm) {
                    for (auto& entity : importResult.data.animations) {
                        if (auto handle = acm->Read(entity); handle) {
                            const auto& tracks = handle->tracks;
                            for (auto& entityRef : tracks) {
                                animationTracks.emplace_back(entityRef);
                            }
                        }
                    }
                }
                ec_.AddSubCollection("animationTracks", {}).AddEntities(animationTracks);
            }

            // Load finished successfully.
            done_ = true;
        }
    }

    bool LoadImageEntityCollection()
    {
        auto uri = GetUri();
        auto imageHandle = assetManager_.GetEcsSerializer().LoadImageResource(uri);

        // NOTE: Creating the entity even when the image load failed to load so we can detect a missing resource.
        EntityReference entity = ecs_.GetEntityManager().CreateReferenceCounted();
        ec_.AddEntity(entity);

        auto* ncm = GetManager<INameComponentManager>(ecs_);
        auto* ucm = GetManager<IUriComponentManager>(ecs_);
        auto* gcm = GetManager<IRenderHandleComponentManager>(ecs_);
        if (ncm && ucm && gcm) {
            {
                ncm->Create(entity);
                auto nc = ncm->Get(entity);
                nc.name = PathUtil::GetFilename(uri);
                ec_.SetUniqueIdentifier(nc.name, entity);
                ncm->Set(entity, nc);
            }

            {
                // TODO: maybe need to save uri + contextUri
                ucm->Create(entity);
                auto uc = ucm->Get(entity);
                uc.uri = uri;
                ucm->Set(entity, uc);
            }

            gcm->Create(entity);
            if (imageHandle) {
                auto ic = gcm->Get(entity);
                ic.reference = imageHandle;
                gcm->Set(entity, ic);

                done_ = true;
                return true;
            }
        }

        // NOTE: Always returning true as even when this fails a placeholder entity is created.
        CORE_LOG_E("Error creating image '%s'", uri.c_str());
        CreateDummyEntity();
        result_ = { false, "Error creating image" };
        done_ = true;
        return true;
    }

private:
    AssetManager& assetManager_;
    RENDER_NS::IRenderContext& renderContext_;
    CORE3D_NS::IGraphicsContext& graphicsContext_;

    CORE_NS::IEcs& ecs_;
    IEntityCollection& ec_;
    BASE_NS::string src_;
    BASE_NS::string contextUri_;

    IAssetLoader::Result result_ {};

    vector<AssetLoader::Ptr> dependencies_;

    bool done_ { false };
    bool cancelled_ { false };
    bool async_ { false };

    GLTFLoadResult loadResult_ {};
    IGLTF2Importer::Ptr importer_ {};

    vector<IAssetLoader::IListener*> listeners_;
    IFileManager& fileManager_;
};

IAssetLoader::Ptr CreateAssetLoader(AssetManager& assetManager, IRenderContext& renderContext,
    IGraphicsContext& graphicsContext, IEntityCollection& ec, string_view src, string_view contextUri)
{
    return IAssetLoader::Ptr { new AssetLoader(assetManager, renderContext, graphicsContext, ec, src, contextUri) };
}

SCENE_END_NAMESPACE()
