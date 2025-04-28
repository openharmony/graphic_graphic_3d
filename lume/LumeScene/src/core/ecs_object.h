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

#ifndef SCENE_SRC_CORE_ECS_OBJECT_H
#define SCENE_SRC_CORE_ECS_OBJECT_H

#include <scene/base/types.h>
#include <scene/ext/intf_ecs_event_listener.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_internal_scene.h>
#include <shared_mutex>

#include <core/ecs/intf_ecs.h>

#include <meta/ext/object.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(EcsObject, "6f1d5812-ce80-4f1d-9a74-6815f3e111b0", META_NS::ObjectCategoryBits::NO_CATEGORY)

class EcsObject : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IEcsObject, IEcsEventListener> {
    META_OBJECT(EcsObject, SCENE_NS::ClassId::EcsObject, IntroduceInterfaces)

public:
    BASE_NS::string GetName() const override;
    IInternalScene::Ptr GetScene() const override;
    CORE_NS::Entity GetEntity() const override;
    META_NS::IEngineValueManager::Ptr GetEngineValueManager() override;

    Future<bool> AddAllEngineProperties(const META_NS::IMetadata::Ptr& object, BASE_NS::string_view component) override;
    Future<bool> AttachEngineProperty(const META_NS::IProperty::Ptr&, BASE_NS::string_view path) override;
    Future<META_NS::IProperty::Ptr> CreateEngineProperty(BASE_NS::string_view path) override;

public: // call in engine thread
    bool Build(const META_NS::IMetadata::Ptr& d) override;
    void SyncProperties() override;
    BASE_NS::string GetPath() const override;
    bool SetName(const BASE_NS::string& name) override;

protected:
    void OnEntityEvent(CORE_NS::IEcs::EntityListener::EventType type) override;
    void OnComponentEvent(
        CORE_NS::IEcs::ComponentListener::EventType type, const CORE_NS::IComponentManager& component) override;

    META_NS::EnginePropertyHandle GetPropertyHandle(BASE_NS::string_view component) const;
    bool AddEngineProperty(META_NS::IMetadata& object, const META_NS::IEngineValue::Ptr& v);
    void AddAllComponentProperties(META_NS::IMetadata& object);
    void AddAllProperties(META_NS::IMetadata& object, CORE_NS::IComponentManager*);
    bool AttachEnginePropertyImpl(
        META_NS::EnginePropertyHandle, const BASE_NS::string&, const META_NS::IProperty::Ptr&, BASE_NS::string_view);

private:
    IInternalScene::WeakPtr scene_;
    CORE_NS::Entity entity_;
    META_NS::IEngineValueManager::Ptr valueManager_;
    // locked access
    mutable std::shared_mutex mutex_;
    BASE_NS::string name_;
};

SCENE_END_NAMESPACE()

#endif