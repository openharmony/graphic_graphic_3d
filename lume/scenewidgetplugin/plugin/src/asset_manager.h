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

#ifndef SCENEPLUGIN_ASSETMANAGER_H
#define SCENEPLUGIN_ASSETMANAGER_H

#include <scene_plugin/interface/intf_asset_loader.h>
#include <scene_plugin/interface/intf_asset_manager.h>
#include <scene_plugin/interface/intf_entity_collection.h>

#include "ecs_serializer.h"

SCENE_BEGIN_NAMESPACE()

class AssetManager : public IAssetManager {
public:
    AssetManager(RENDER_NS::IRenderContext& renderContext, CORE3D_NS::IGraphicsContext& graphicsContext);
    virtual ~AssetManager();

    //
    // From IAssetManager
    //
    ExtensionType GetExtensionType(BASE_NS::string_view ext) const override;

    IAssetLoader::Ptr CreateAssetLoader(
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

    //
    // From IEcsSerializer::IListener
    //
    IEntityCollection* GetExternalCollection(
        CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;

protected:
    void Destroy() override;

private:
    RENDER_NS::IRenderContext& renderContext_;
    CORE3D_NS::IGraphicsContext& graphicsContext_;
    EcsSerializer ecsSerializer_;

    // Mapping from uri to a collection of entities. The entities live in the ECS but we need to keep track of some kind
    // of internal ownership of each cached entity.
    BASE_NS::unordered_map<BASE_NS::string, IEntityCollection::Ptr> cacheCollections_;
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_ASSETMANAGER_H
