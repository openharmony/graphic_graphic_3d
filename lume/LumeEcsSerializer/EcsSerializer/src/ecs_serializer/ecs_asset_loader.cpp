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

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <base/containers/fixed_string.h>
#include <base/containers/vector.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <ecs_serializer/ecs_animation_util.h>
#include <ecs_serializer/intf_ecs_asset_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <util/io_util.h>
#include <util/path_util.h>

#include "ecs_serializer/asset_migration.h"
#include "ecs_serializer/intf_ecs_asset_loader.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using namespace UTIL_NS;

ECS_SERIALIZER_BEGIN_NAMESPACE()

namespace {
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
class EcsAssetLoader final : public IEcsAssetLoader,
                             private IEcsAssetLoader::IListener,
                             private IGLTF2Importer::Listener {
public:
    EcsAssetLoader(IEcsAssetManager& assetManager, IGraphicsContext& graphicsContext, IEntityCollection& ec,
        string_view src, string_view contextUri)
        : assetManager_(assetManager), renderContext_(graphicsContext.GetRenderContext()),
          graphicsContext_(graphicsContext), ecs_(ec.GetEcs()), ec_(ec), src_(src), contextUri_(contextUri)
    {}

    ~EcsAssetLoader() override
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

    void AddListener(IEcsAssetLoader::IListener& listener) override
    {
        CORE_ASSERT(&listener);
        listeners_.emplace_back(&listener);
    }

    void RemoveListener(IEcsAssetLoader::IListener& listener) override
    {
        CORE_ASSERT(&listener);
        for (size_t i = 0; i < listeners_.size(); ++i) {
            if (&listener == listeners_[i]) {
                listeners_.erase(listeners_.begin() + static_cast<int64_t>(i));
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

    IEcsAssetLoader* GetNextDependency() const
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
    // From IEcsAssetLoader::Listener
    //
    void OnLoadFinished(const IEcsAssetLoader& loader) override
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
        if (cancelled_) {
            return;
        }

        GltfImportFinished();
        if (async_) {
            ContinueLoading();
        }
    }
    void OnImportProgressed(size_t taskIndex, size_t taskCount) override {}

    bool Execute(uint32_t timeBudget) override
    {
        if (!async_) {
            return true;
        }
        if (done_) {
            return true;
        }

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

        if (done) {
            for (auto* listener : listeners_) {
                listener->OnLoadFinished(*this);
            }
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
            case IEcsAssetManager::ExtensionType::COLLECTION: {
                ec_.SetType("entitycollection");
                auto result = LoadJsonEntityCollection();
                if (result.compatibilityInfo.versionMajor == 0 && result.compatibilityInfo.versionMinor == 0) {
                    MigrateAnimation(ec_);
                }
                break;
            }

            case IEcsAssetManager::ExtensionType::SCENE: {
                ec_.SetType("scene");
                auto result = LoadJsonEntityCollection();
                if (result.compatibilityInfo.versionMajor == 0 && result.compatibilityInfo.versionMinor == 0) {
                    MigrateAnimation(ec_);
                }
                break;
            }

            case IEcsAssetManager::ExtensionType::PREFAB: {
                ec_.SetType("prefab");
                auto result = LoadJsonEntityCollection();
                if (result.compatibilityInfo.versionMajor == 0 && result.compatibilityInfo.versionMinor == 0) {
                    MigrateAnimation(ec_);
                }
                break;
            }

            case IEcsAssetManager::ExtensionType::ANIMATION: {
                ec_.SetType("animation");
                auto result = LoadJsonEntityCollection();
                if (result.compatibilityInfo.versionMajor == 0 && result.compatibilityInfo.versionMinor == 0) {
                    MigrateAnimation(ec_);
                }
                break;
            }

            case IEcsAssetManager::ExtensionType::MATERIAL: {
                ec_.SetType("material");
                LoadJsonEntityCollection();
                break;
            }

            case IEcsAssetManager::ExtensionType::GLTF:
            case IEcsAssetManager::ExtensionType::GLB: {
                ec_.SetType("gltf");
                LoadGltfEntityCollection();
                break;
            }

            case IEcsAssetManager::ExtensionType::JPG:
            case IEcsAssetManager::ExtensionType::PNG:
            case IEcsAssetManager::ExtensionType::KTX: {
                ec_.SetType("image");
                LoadImageEntityCollection();
                break;
            }

            case IEcsAssetManager::ExtensionType::NOT_SUPPORTED:
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

            if (!async_) {
                for (auto* listener : listeners_) {
                    listener->OnLoadFinished(*this);
                }
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

    IoUtil::SerializationResult LoadJsonEntityCollection()
    {
        const auto resolvedUri = GetUri();
        const auto resolvedFile = PathUtil::ResolveUri(contextUri_, src_, false);

        string textIn;
        auto& fileManager = renderContext_.GetEngine().GetFileManager();
        if (IoUtil::LoadTextFile(fileManager, resolvedFile, textIn)) {
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
                    if (!assetManager_.IsCachedCollection(dep.src, dep.contextUri)) {
                        auto* cacheEc = assetManager_.CreateCachedCollection(ec_.GetEcs(), dep.src, dep.contextUri);
                        dependencies_.emplace_back(
                            assetManager_.CreateEcsAssetLoader(*cacheEc, dep.src, dep.contextUri));
                        dependencies_.back()->AddListener(*this);
                    }
                }

                if (GetNextDependency() == nullptr) {
                    // No dependencies to load. Just load the entity collection itself and this loading is done.
                    auto result = assetManager_.GetEcsSerializer().ReadEntityCollection(ec_, json, resolvedUri);
                    done_ = true;
                    return result;
                }

                // There are dependencies that need to be parsed in a next step.
                return {};
            }
        }

        CreateDummyEntity();
        result_ = { false, "collection loading failed." };
        done_ = true;

        IoUtil::SerializationResult serializationResult;
        serializationResult.status = IoUtil::Status::ERROR_GENERAL;
        serializationResult.error = result_.error;
        return serializationResult;
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

        // Link animation tracks to targets
        if (!resourceData.animations.empty()) {
            INodeSystem* nodeSystem = GetSystem<INodeSystem>(ecs_);
            if (auto animationRootNode = nodeSystem->GetNode(importedSceneEntity); animationRootNode) {
                for (const auto& animationEntity : resourceData.animations) {
                    ECS_SERIALIZER_NS::UpdateAnimationTrackTargets(
                        ecs_, animationEntity, animationRootNode->GetEntity());
                }
            }
        }

        return importedSceneEntity;
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

        if (importer_) {
            const auto& gltfImportResult = importer_->GetResult();
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
            const auto originalRootEntity = ImportSceneFromGltf({});
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

                loadNode->SetParent(nodeSystem->GetRootNode());
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
                    for (auto& current : importResult.data.animations) {
                        if (auto handle = acm->Read(current); handle) {
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
                ncm->Set(entity, nc);
            }

            {
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
    IEcsAssetManager& assetManager_;
    RENDER_NS::IRenderContext& renderContext_;
    CORE3D_NS::IGraphicsContext& graphicsContext_;

    CORE_NS::IEcs& ecs_;
    IEntityCollection& ec_;
    BASE_NS::string src_;
    BASE_NS::string contextUri_;

    IEcsAssetLoader::Result result_ {};

    vector<EcsAssetLoader::Ptr> dependencies_;

    bool done_ { false };
    bool cancelled_ { false };
    bool async_ { false };

    GLTFLoadResult loadResult_ {};
    IGLTF2Importer::Ptr importer_ {};

    vector<IEcsAssetLoader::IListener*> listeners_;
};

IEcsAssetLoader::Ptr CreateEcsAssetLoader(IEcsAssetManager& assetManager, IGraphicsContext& graphicsContext,
    IEntityCollection& ec, string_view src, string_view contextUri)
{
    return IEcsAssetLoader::Ptr { new EcsAssetLoader(assetManager, graphicsContext, ec, src, contextUri) };
}

ECS_SERIALIZER_END_NAMESPACE()
