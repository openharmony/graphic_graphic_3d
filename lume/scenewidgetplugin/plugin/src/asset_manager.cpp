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

#include "asset_manager.h"

#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <render/intf_render_context.h>

#include "asset_loader.h"
#include "entity_collection.h"
#include "json.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

SCENE_BEGIN_NAMESPACE()

AssetManager::AssetManager(IRenderContext& renderContext, IGraphicsContext& graphicsContext)
    : renderContext_(renderContext), graphicsContext_(graphicsContext), ecsSerializer_(renderContext_)
{
    ecsSerializer_.SetDefaultSerializers();
    ecsSerializer_.SetListener(this);
}

AssetManager::~AssetManager() {}

AssetManager::ExtensionType AssetManager::GetExtensionType(string_view ext) const
{
    // Ignore case.
    // Better type recognition
    if (ext == "collection") {
        return ExtensionType::COLLECTION;
    } else if (ext == "scene") {
        return ExtensionType::SCENE;
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

IAssetLoader::Ptr AssetManager::CreateAssetLoader(IEntityCollection& ec, string_view src, string_view contextUri)
{
    return SCENE_NS::CreateAssetLoader(*this, renderContext_, graphicsContext_, ec, src, contextUri);
}

bool AssetManager::LoadAsset(IEntityCollection& ec, string_view uri, string_view contextUri)
{
    auto assetLoader = CreateAssetLoader(ec, uri, contextUri);
    if (assetLoader) {
        //  Use syncronous asset loading.
        assetLoader->LoadAsset();
        auto result = assetLoader->GetResult();
        return result.success;
    }
    return false;
}

bool SaveTextFile(string_view fileUri, string_view fileContents, CORE_NS::IFileManager& fileManager)
{
    // TODO: the safest way to save files would be to save to a temp file and rename.
    SCENE_PLUGIN_VERBOSE_LOG("Save file: '%s'", string(fileUri).c_str());
    auto file = fileManager.CreateFile(fileUri);
    if (file) {
        file->Write(fileContents.data(), fileContents.length());
        file->Close();
        return true;
    }

    return false;
}

bool AssetManager::SaveJsonEntityCollection(const IEntityCollection& ec, string_view uri, string_view contextUri) const
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    SCENE_PLUGIN_VERBOSE_LOG("SaveJsonEntityCollection: '%s'", resolvedUri.c_str());

    json::standalone_value jsonOut;
    auto result = ecsSerializer_.WriteEntityCollection(ec, jsonOut);
    if (result) {
        const string jsonString = CORE_NS::to_formatted_string(jsonOut, 4);
        if (SaveTextFile(
                resolvedUri, { jsonString.data(), jsonString.size() }, renderContext_.GetEngine().GetFileManager())) {
            return true;
        }
    }
    return false;
}

IEntityCollection* AssetManager::LoadAssetToCache(
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

    // Need to first remove the possible cached collection because otherwise gltf importer will not reload the resources
    // like meshes if they have the same names.
    cacheCollections_.erase(cacheId);

    // Load the entity collection.

    auto ec = EntityCollection::Ptr { new EntityCollection(ecs, uri, contextUri) };
    if (LoadAsset(*ec, uri, contextUri)) {
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

bool AssetManager::IsCachedCollection(string_view uri, string_view contextUri) const
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    return cacheCollections_.find(cacheId) != cacheCollections_.cend();
}

IEntityCollection* AssetManager::CreateCachedCollection(
    IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri)
{
    auto ec = EntityCollection::Ptr { new EntityCollection(ecs, uri, contextUri) };
    ec->SetActive(false);

    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    // Add the created collection to the cache map.
    return (cacheCollections_[cacheId] = move(ec)).get();
}

IEntityCollection* AssetManager::GetCachedCollection(string_view uri, string_view contextUri) const
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    auto collection = cacheCollections_.find(cacheId);
    return collection != cacheCollections_.cend() ? collection->second.get() : nullptr;
}

void AssetManager::RemoveFromCache(string_view uri, string_view contextUri)
{
    const auto resolvedUri = PathUtil::ResolveUri(contextUri, uri);
    const auto& cacheId = resolvedUri;

    cacheCollections_.erase(cacheId);
}

void AssetManager::ClearCache()
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

void AssetManager::RefreshAsset(IEntityCollection& ec, bool active)
{
    json::standalone_value json;
    if (!ecsSerializer_.WriteEntityCollection(ec, json)) {
        return;
    }

    if (json) {
        ec.Clear();
        // Set the active state when the collection empty to not iterate all the contents.
        ec.SetActive(active);
        // NOTE: This does conversion from standalone json to read only json.
        ecsSerializer_.ReadEntityCollection(ec, json, ec.GetContextUri());
    }
}

IEntityCollection* AssetManager::InstantiateCollection(IEntityCollection& ec, string_view uri, string_view contextUri)
{
#ifdef VERBOSE_LOGGING
    SCENE_PLUGIN_VERBOSE_LOG(
        "InstantiateCollection: uri='%s' context='%s'", string(uri).c_str(), string(contextUri).c_str());
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

IEcsSerializer& AssetManager::GetEcsSerializer()
{
    return ecsSerializer_;
}

IEntityCollection* AssetManager::GetExternalCollection(IEcs& ecs, string_view uri, string_view contextUri)
{
    // NOTE: Does not automatically try to load external dependencies.
    return GetCachedCollection(uri, contextUri);
}

void AssetManager::Destroy()
{
    delete this;
}

SCENE_END_NAMESPACE()
