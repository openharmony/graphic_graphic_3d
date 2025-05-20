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
#include "asset_object.h"

#include <ecs_serializer/api.h>
#include <ecs_serializer/intf_ecs_asset_loader.h>
#include <ecs_serializer/intf_ecs_asset_manager.h>
#include <ecs_serializer/intf_entity_collection.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>

SCENE_BEGIN_NAMESPACE()

bool AssetObject::Load(const IScene::Ptr& sc, BASE_NS::string_view uri)
{
    using ECS_SERIALIZER_NS::CreateEcsAssetLoader;
    using ECS_SERIALIZER_NS::CreateEcsAssetManager;
    using ECS_SERIALIZER_NS::CreateEntityCollection;

    auto scene = sc->GetInternalScene();
    auto& ecs = scene->GetEcsContext();

    if (!entities_) {
        entities_ = CreateEntityCollection(*ecs.GetNativeEcs(), "scene", {});
        if (!entities_) {
            CORE_LOG_E("Failed to create entity collection");
            return false;
        }
    }
    auto manager = CreateEcsAssetManager(scene->GetGraphicsContext());
    if (!manager) {
        CORE_LOG_E("Failed to create ecs asset manager");
        return false;
    }
    auto loader = CreateEcsAssetLoader(*manager, scene->GetGraphicsContext(), *entities_, uri, {});
    if (!loader) {
        CORE_LOG_E("Failed to create ecs asset loader");
        return false;
    }

    loader->LoadAsset();

    if (entities_->GetEntityCount() == 0) {
        CORE_LOG_E("Entity count is zero in loaded scene");
        return false;
    }

    if (!ecs.CreateUnnamedRootNode()) {
        CORE_LOG_E("Failed to create root node");
        return false;
    }
    return true;
}

SCENE_END_NAMESPACE()
