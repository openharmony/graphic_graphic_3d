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

#include <core/intf_engine.h>
#include <render/intf_render_context.h>
#include <util/io_util.h>
#include <util/json.h>
#include <util/path_util.h>

#include <ecs_serializer/api.h>
#include <ecs_serializer/intf_ecs_asset_manager.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using namespace UTIL_NS;

ECS_SERIALIZER_BEGIN_NAMESPACE()

class EcsAssetManager : public IEcsAssetManager {
public:
    explicit EcsAssetManager(CORE3D_NS::IGraphicsContext& graphicsContext);
    virtual ~EcsAssetManager();
    // From IEcsAssetManager
    ExtensionType GetExtensionType(BASE_NS::string_view ext) const override;

    IEcsAssetLoader::Ptr CreateEcsAssetLoader(
        IEntityCollection& ec, BASE_NS::string_view src, BASE_NS::string_view contextUri) override;

    bool LoadAsset(IEntityCollection& ec, BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;
    bool SaveJsonEntityCollection(
        const IEntityCollection& ec, BASE_NS::string_view uri, BASE_NS::string_view contextUri) const override;

    IEntityCollection* LoadAssetToCache(CORE_NS::IEcs& ecs, BASE_NS::string_view cacheUri, BASE_NS::string_view uri,
        BASE_NS::string_view contextUri, bool active, bool forceReload) override;

    bool IsCachedCollection(BASE_NS::string_view uri, BASE_NS::string_view contextUri) const override;
    IEntityCollection* CreateCachedCollection(
        CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;
    IEntityCollection* GetCachedCollection(BASE_NS::string_view uri, BASE_NS::string_view contextUri) const override;
    void RemoveFromCache(BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;
    void ClearCache() override;

    void RefreshAsset(IEntityCollection& ec, bool active) override;

    IEntityCollection* InstantiateCollection(
        IEntityCollection& ec, BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;

    IEcsSerializer& GetEcsSerializer() override;
    const IEcsSerializer& GetEcsSerializer() const override;
    // From IEcsSerializer::IListener
    IEntityCollection* GetExternalCollection(
        CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;

protected:
    void Destroy() override;

private:
    RENDER_NS::IRenderContext& renderContext_;
    CORE3D_NS::IGraphicsContext& graphicsContext_;
    IEcsSerializer::Ptr ecsSerializer_;

    // Mapping from uri to a collection of entities. The entities live in the ECS but we need to keep track of some kind
    // of internal ownership of each cached entity.
    BASE_NS::unordered_map<BASE_NS::string, IEntityCollection::Ptr> cacheCollections_;
};

EcsAssetManager::EcsAssetManager(IGraphicsContext& graphicsContext)
    : renderContext_(graphicsContext.GetRenderContext()), graphicsContext_(graphicsContext),
      ecsSerializer_(CreateEcsSerializer(renderContext_))
{
    ecsSerializer_->SetDefaultSerializers();
    ecsSerializer_->SetListener(this);
}

EcsAssetManager::~EcsAssetManager() {}

EcsAssetManager::ExtensionType EcsAssetManager::GetExtensionType(string_view ext) const
{
    // Ignore case.
    // Better type recognition
    if (ext == "collection") {
        return ExtensionType::COLLECTION;
    } else if (ext == "scene") {
        return ExtensionType::SCENE;
    } else if (ext == "prefab") {
        return ExtensionType::PREFAB;
    } else if (ext == "animation") {
        return ExtensionType::ANIMATION;
    } else if (ext == "material") {
        return ExtensionType::MATERIAL;
    } else if (ext == "gltf") {
        return ExtensionType::GLTF;
    } else if (ext == "glb") {
        return ExtensionType::GLB;
    } else if (ext == "png") {
        return ExtensionType::PNG;
    } else if (ext == "jpg" || ext == "jpeg") {
        return ExtensionType::JPG;
    } else if (ext == "ktx") {
        return ExtensionType::KTX;
    } else {
        return ExtensionType::NOT_SUPPORTED;
    }
}

IEcsAssetLoader::Ptr EcsAssetManager::CreateEcsAssetLoader(
    IEntityCollection& ec, string_view src, string_view contextUri)
{
    return ::ECS_SERIALIZER_NS::CreateEcsAssetLoader(*this, graphicsContext_, ec, src, contextUri);
}

bool EcsAssetManager::LoadAsset(IEntityCollection& ec, string_view uri, string_view contextUri)
{
    auto assetLoader = CreateEcsAssetLoader(ec, uri, contextUri);
    if (assetLoader) {
        //  Use syncronous asset loading.
        assetLoader->LoadAsset();
        auto result = assetLoader->GetResult();
        return result.success;
    }
    return false;
}

bool EcsAssetManager::SaveJsonEntityCollection(
    const IEntityCollection& ec, string_view uri, string_view contextUri) const
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    CORE_LOG_D("SaveJsonEntityCollection: '%s'", resolvedUri.c_str());

    json::standalone_value jsonOut;
    auto result = GetEcsSerializer().WriteEntityCollection(ec, jsonOut);
    if (result) {
        const string jsonString = to_formatted_string(jsonOut, 4);
        auto& fileManager = renderContext_.GetEngine().GetFileManager();
        if (IoUtil::SaveTextFile(fileManager, resolvedUri, { jsonString.data(), jsonString.size() })) {
            return true;
        }
    }
    return false;
}

IEntityCollection* EcsAssetManager::LoadAssetToCache(
    IEcs& ecs, string_view cacheUri, string_view uri, string_view contextUri, bool active, bool forceReload)
{
    // Check if this collection was already loaded.
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = cacheUri;

    if (!forceReload) {
        auto it = cacheCollections_.find(cacheId);
        if (it != cacheCollections_.cend()) {
            return it->second.get();
        }
    }

    // Not yet loaded (or forcing reload) -> load the entity collection.
    // Need to first remove the possible cached collection because otherwise gltf importer will not reload the resources
    // like meshes if they have the same names.
    cacheCollections_.erase(cacheId);

    auto ec = CreateEntityCollection(ecs, cacheUri, contextUri);
    if (LoadAsset(*ec, resolvedUri, contextUri)) {
        // Something was loaded. Set all loaded entities as inactive if requested.
        if (!active) {
            ec->SetActive(false);
        }
        // Add loaded collection to cache map.
        return (cacheCollections_[cacheId] = move(ec)).get();
    } else {
        return nullptr;
    }
}

bool EcsAssetManager::IsCachedCollection(string_view uri, string_view contextUri) const
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    return cacheCollections_.find(cacheId) != cacheCollections_.cend();
}

IEntityCollection* EcsAssetManager::CreateCachedCollection(
    IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri)
{
    auto ec = CreateEntityCollection(ecs, uri, contextUri);
    ec->SetActive(false);

    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    // Add the created collection to the cache map.
    return (cacheCollections_[cacheId] = move(ec)).get();
}

IEntityCollection* EcsAssetManager::GetCachedCollection(string_view uri, string_view contextUri) const
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    auto collection = cacheCollections_.find(cacheId);
    return collection != cacheCollections_.cend() ? collection->second.get() : nullptr;
}

void EcsAssetManager::RemoveFromCache(string_view uri, string_view contextUri)
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    cacheCollections_.erase(cacheId);
}

void EcsAssetManager::ClearCache()
{
    // NOTE: not removing any active collections.
    for (auto it = cacheCollections_.begin(); it != cacheCollections_.end();) {
        if (!it->second->IsActive()) {
            it = cacheCollections_.erase(it);
        } else {
            ++it;
        }
    }
}

void EcsAssetManager::RefreshAsset(IEntityCollection& ec, bool active)
{
    json::standalone_value json;
    if (!GetEcsSerializer().WriteEntityCollection(ec, json)) {
        return;
    }

    if (json) {
        ec.Clear();
        // Set the active state when the collection empty to not iterate all the contents.
        ec.SetActive(active);
        // NOTE: This does conversion from standalone json to read only json.
        GetEcsSerializer().ReadEntityCollection(ec, json, ec.GetContextUri());
    }
}

IEntityCollection* EcsAssetManager::InstantiateCollection(
    IEntityCollection& ec, string_view uri, string_view contextUri)
{
#ifdef VERBOSE_LOGGING
    CORE_LOG_D("InstantiateCollection: uri='%s' context='%s'", string(uri).c_str(), string(contextUri).c_str());
#endif
    auto& ecs = ec.GetEcs();
    auto externalCollection = LoadAssetToCache(ecs, uri, uri, contextUri, false, false);
    if (externalCollection) {
        auto& newCollection = ec.AddSubCollectionClone(*externalCollection, {});
        newCollection.SetSrc(uri);
        newCollection.MarkModified(false);
        return &newCollection;
    }

    CORE_LOG_E("Loading collection failed: '%s'", string(uri).c_str());
    return nullptr;
}

IEcsSerializer& EcsAssetManager::GetEcsSerializer()
{
    return *ecsSerializer_;
}

const IEcsSerializer& EcsAssetManager::GetEcsSerializer() const
{
    return *ecsSerializer_;
}

IEntityCollection* EcsAssetManager::GetExternalCollection(IEcs& ecs, string_view uri, string_view contextUri)
{
    // NOTE: Does not automatically try to load external dependencies.
    return GetCachedCollection(uri, contextUri);
}

void EcsAssetManager::Destroy()
{
    delete this;
}

IEcsAssetManager::Ptr CreateEcsAssetManager(CORE3D_NS::IGraphicsContext& graphicsContext)
{
    return IEcsAssetManager::Ptr { new EcsAssetManager(graphicsContext) };
}

ECS_SERIALIZER_END_NAMESPACE()
