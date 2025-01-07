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

#ifndef API_ECS_SERIALIZER_API_H
#define API_ECS_SERIALIZER_API_H

#include <ecs_serializer/intf_ecs_asset_loader.h>
#include <ecs_serializer/intf_ecs_asset_manager.h>
#include <ecs_serializer/intf_ecs_serializer.h>
#include <ecs_serializer/intf_entity_collection.h>

ECS_SERIALIZER_BEGIN_NAMESPACE()

IEntityCollection::Ptr CreateEntityCollection(
    CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri);

IEcsSerializer::Ptr CreateEcsSerializer(RENDER_NS::IRenderContext& renderContext);

IEcsAssetManager::Ptr CreateEcsAssetManager(CORE3D_NS::IGraphicsContext& graphicsContext);

IEcsAssetLoader::Ptr CreateEcsAssetLoader(IEcsAssetManager& assetManager, CORE3D_NS::IGraphicsContext& graphicsContext,
    IEntityCollection& ec, BASE_NS::string_view src, BASE_NS::string_view contextUri);

ECS_SERIALIZER_END_NAMESPACE()

#endif // API_ECS_SERIALIZER_API_H
