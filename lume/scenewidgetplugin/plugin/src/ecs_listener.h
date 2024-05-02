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

#ifndef SCENEPLUGIN_ECS_LISTENER
#define SCENEPLUGIN_ECS_LISTENER

#include <PropertyTools/property_data.h>
#include <scene_plugin/interface/intf_ecs_object.h>
#include <scene_plugin/namespace.h>

#include <3d/ecs/components/name_component.h>
#include <core/ecs/intf_ecs.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

SCENE_BEGIN_NAMESPACE()

class EcsListener;

REGISTER_INTERFACE(IEcsProxyObject, "74ad9154-5f61-4551-95db-249bae110d6d")
class IEcsProxyObject : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsProxyObject, InterfaceId::IEcsProxyObject)

public:
    virtual void SetCommonListener(BASE_NS::shared_ptr<SCENE_NS::EcsListener> listener) = 0;
    virtual void DoEntityEvent(CORE_NS::IEcs::EntityListener::EventType type, const CORE_NS::Entity& entity) = 0;
    virtual void DoComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
        const CORE_NS::IComponentManager& componentManager, const CORE_NS::Entity& entity) = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IEcsProxyObject::WeakPtr);
META_TYPE(SCENE_NS::IEcsProxyObject::Ptr);

SCENE_BEGIN_NAMESPACE()

class EcsListener : public CORE_NS::IEcs::EntityListener, CORE_NS::IEcs::ComponentListener {
public:
    bool IsCallbackActive()
    {
        return callbackActive_;
    }

    void SetEcs(const CORE_NS::IEcs::Ptr& ecs)
    {
        if (auto ecsRaw = ecs.get(); ecs_ != ecsRaw) {
            Reset();
            ecs_ = ecsRaw;
            ecs_->AddListener((CORE_NS::IEcs::EntityListener&)*this);
        }
    }

    void Reset(CORE_NS::IComponentManager* manager = nullptr)
    {
        entities_.clear();
        if (ecs_) {
            ecs_->RemoveListener((CORE_NS::IEcs::EntityListener&)*this);
            for (auto& manager : componentManagers_) {
                ecs_->RemoveListener(*manager.first, (CORE_NS::IEcs::ComponentListener&)*this);
            }
            componentManagers_.clear();
            ecs_ = nullptr;
        }
    }

    void AddEntity(const CORE_NS::Entity& entity, IEcsProxyObject::WeakPtr object, CORE_NS::IComponentManager& manager)
    {
        if (const auto& ite = entities_.find(entity.id); ite == entities_.cend()) {
            entities_[entity.id].push_back(ComponentProxy::Create(manager, object));
        } else {
            // ToDo: should we add ref count or array to multiple instances?
            // In principle there should not be more than one EcsObject per entity
            // but it is possible to have several instances pointing to same entity
            auto& vec = entities_[entity.id];
            bool found = false;
            for (auto& oldEntity : vec) {
                if (object.Compare(oldEntity->entity) && &manager == oldEntity->cm) {
                    found = true;
                    oldEntity->refcount++;
                    SCENE_PLUGIN_VERBOSE_LOG("%s: multiple, count %zu", __func__, vec.size());
                    break;
                }
            }
            if (!found) {
                entities_[entity.id].push_back(ComponentProxy::Create(manager, object));
            }
        }

        // we increase refcount for manager always regardless the duplicate entity subscriptions so that tear down will
        // work
        if (auto managerEntry = componentManagers_.find(&manager); managerEntry != componentManagers_.end()) {
            managerEntry->second++;
        } else {
            // ToDo: could queue entity references instead reference counts so that we could
            // traverse minimal amount of entities when dispatching the events
            componentManagers_[&manager] = 1;
            ecs_->AddListener(manager, (CORE_NS::IEcs::ComponentListener&)*this);
        }
    }

    void RemoveEntity(
        const CORE_NS::Entity& entity, IEcsProxyObject::WeakPtr object, CORE_NS::IComponentManager& manager)
    {
        if (const auto& ite = entities_.find(entity.id); ite != entities_.cend()) {
            size_t ix = 0;
            for (auto& obj : ite->second) {
                if (obj->entity.Compare(object) && obj->cm == &manager) {
                    --obj->refcount;
                    if (!obj->refcount) {
                        ite->second.erase(ite->second.cbegin() + ix);
                    }
                    break;
                }
                ++ix;
            }
            if (ite->second.empty()) {
                entities_.erase(entity.id);
            }
        }
        if (--componentManagers_[&manager] == 0) {
            ecs_->RemoveListener(manager, (CORE_NS::IEcs::ComponentListener&)*this);
            componentManagers_.erase(&manager);
        }
    }

    void OnEntityEvent(
        CORE_NS::IEcs::EntityListener::EventType type, BASE_NS::array_view<const CORE_NS::Entity> entities) override
    {
        callbackActive_ = true;
        for (const auto& e : entities) {
            if (const auto& entity = entities_.find(e.id); entity != entities_.cend()) {
                // hack to remove duplicates here! PLZ FIX
                BASE_NS::vector<IEcsProxyObject::Ptr> tmp;
                auto keepAlive = entity->second;
                for (int ii = 0; ii < keepAlive.size(); ii++) {
                    const auto& object = keepAlive[ii];
                    if (auto proxy = object->entity.lock()) {
                        bool dupe=false;
                        for (auto t:tmp) {
                            if (t==proxy) {
                                dupe=true;
                            }
                        }
                        if (!dupe) {
                            tmp.push_back(proxy);
                        }
                    }
                }
                for (auto t:tmp) {
                    t->DoEntityEvent(type, e);
                }
            }
        }
        callbackActive_ = false;
    }

    void OnComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
        const CORE_NS::IComponentManager& componentManager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override
    {
        callbackActive_ = true;
        for (const auto& e : entities) {
            // SCENE_PLUGIN_VERBOSE_LOG("%s: %i", __func__, e.id);
            if (const auto& entity = entities_.find(e.id); entity != entities_.cend()) {
                // hack to remove duplicates here! PLZ FIX
                BASE_NS::vector<IEcsProxyObject::Ptr> tmp;
                auto keepAlive = entity->second;
                for (int ii = 0; ii < keepAlive.size(); ii++) {
                    const auto& object = keepAlive[ii];
                    if (auto proxy = object->entity.lock()) {
                        bool dupe=false;
                        for (auto t:tmp) {
                            if (t==proxy) {
                                dupe=true;
                            }
                        }
                        if (!dupe) {
                            tmp.push_back(proxy);
                        }
                    }
                }
                for (auto t:tmp) {
                    t->DoComponentEvent(type, componentManager, e);
                }
            }
        }
        callbackActive_ = false;
    }
    struct ComponentProxy {
        static BASE_NS::shared_ptr<ComponentProxy> Create(
            CORE_NS::IComponentManager& cm, IEcsProxyObject::WeakPtr entity)
        {
            return BASE_NS::shared_ptr<ComponentProxy> { new ComponentProxy { &cm, entity, 1 } };
        }
        CORE_NS::IComponentManager* cm;
        IEcsProxyObject::WeakPtr entity;
        size_t refcount { 0 };
    };

    BASE_NS::unordered_map<uint64_t, BASE_NS::vector<BASE_NS::shared_ptr<ComponentProxy>>> entities_;
    CORE_NS::IEcs* ecs_ {};
    BASE_NS::unordered_map<CORE_NS::IComponentManager*, size_t> componentManagers_;
    bool callbackActive_ { false };
};

SCENE_END_NAMESPACE()

#endif // !SCENEPLUGIN_ECS_LISTENER
