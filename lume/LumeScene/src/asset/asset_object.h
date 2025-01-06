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

#ifndef SCENE_SRC_ASSET_ASSET_OBJECT_H
#define SCENE_SRC_ASSET_ASSET_OBJECT_H

#include <ecs_serializer/intf_ecs_asset_manager.h>
#include <scene/interface/intf_scene.h>

#include <meta/ext/object.h>


SCENE_BEGIN_NAMESPACE()

class IAssetObject : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAssetObject, "70c77b11-bd85-490a-880f-d8a80d2addb0")
public:
    virtual bool Load(const IScene::Ptr&, BASE_NS::string_view uri) = 0;

};

META_REGISTER_CLASS(AssetObject, "a8b4c9e9-9c28-49b7-85b2-2eb6ac812b7c", META_NS::ObjectCategoryBits::NO_CATEGORY)

class AssetObject : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IAssetObject> {
    META_OBJECT(AssetObject, SCENE_NS::ClassId::AssetObject, IntroduceInterfaces)

public:
    bool Load(const IScene::Ptr&, BASE_NS::string_view uri) override;

private:
    ECS_SERIALIZER_NS::IEntityCollection::Ptr entities_;
    ECS_SERIALIZER_NS::IEcsAssetManager::Ptr manager_;
    ECS_SERIALIZER_NS::IEcsAssetLoader::Ptr loader_;
};

SCENE_END_NAMESPACE()

#endif
