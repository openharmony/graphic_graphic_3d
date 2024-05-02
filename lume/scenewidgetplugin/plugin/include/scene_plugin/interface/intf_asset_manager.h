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

#ifndef SCENEPLUGIN_INTF_ASSETMANAGER_H
#define SCENEPLUGIN_INTF_ASSETMANAGER_H

#include <scene_plugin/interface/intf_asset_loader.h>
#include <scene_plugin/interface/intf_ecs_serializer.h>
#include <scene_plugin/interface/intf_entity_collection.h>

#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>

SCENE_BEGIN_NAMESPACE()

class IAssetManager : public IEcsSerializer::IListener {
public:
    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IAssetManager* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IAssetManager, Deleter>;

    // For now detecting content by extension in the uri.
    enum ExtensionType { NOT_SUPPORTED, COLLECTION, SCENE, ANIMATION, MATERIAL, GLTF, GLB, PNG, JPG, KTX };

    virtual ExtensionType GetExtensionType(BASE_NS::string_view ext) const = 0;

    /** Create an asset file loader for loading 3D scene assets.
     *  @param ecs ECS instance that will be used for loading and resource creation.
     *  @return asset loader instance.
     */
    virtual IAssetLoader::Ptr CreateAssetLoader(
        IEntityCollection& ec, BASE_NS::string_view src, BASE_NS::string_view contextUri) = 0;

    virtual bool LoadAsset(IEntityCollection& ec, BASE_NS::string_view uri, BASE_NS::string_view contextUri) = 0;

    virtual bool SaveJsonEntityCollection(
        const IEntityCollection& ec, BASE_NS::string_view uri, BASE_NS::string_view contextUri) const = 0;

    virtual IEntityCollection* LoadAssetToCache(CORE_NS::IEcs& ecs, BASE_NS::string_view cacheUri,
        BASE_NS::string_view uri, BASE_NS::string_view contextUri, bool active, bool forceReload) = 0;

    virtual bool IsCachedCollection(BASE_NS::string_view uri, BASE_NS::string_view contextUri) const = 0;
    virtual IEntityCollection* CreateCachedCollection(
        CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri) = 0;
    virtual IEntityCollection* GetCachedCollection(BASE_NS::string_view uri, BASE_NS::string_view contextUri) const = 0;
    virtual void RemoveFromCache(BASE_NS::string_view uri, BASE_NS::string_view contextUri) = 0;
    virtual void ClearCache() = 0;

    virtual void RefreshAsset(IEntityCollection& ec, bool active) = 0;

    virtual IEntityCollection* InstantiateCollection(
        IEntityCollection& ec, BASE_NS::string_view uri, BASE_NS::string_view contextUri) = 0;

    virtual IEcsSerializer& GetEcsSerializer() = 0;

protected:
    IAssetManager() = default;
    virtual ~IAssetManager() = default;
    virtual void Destroy() = 0;
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_INTF_ASSETMANAGER_H
