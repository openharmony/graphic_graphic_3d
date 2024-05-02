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

#include <base/math/quaternion.h>
#include <base/math/matrix.h>

#include <PropertyTools/property_data.h>
#include <scene_plugin/interface/intf_ecs_object.h>
#include <scene_plugin/namespace.h>

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <core/ecs/intf_ecs.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

#include <meta/api/make_callback.h>
#include <meta/api/engine/util.h>
#include <meta/ext/object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

#include "ecs_listener.h"

#include <inttypes.h>

SCENE_BEGIN_NAMESPACE()

class EcsObject
    : public META_NS::ObjectFwd<EcsObject, ClassId::EcsObject, META_NS::ClassId::Object, IEcsObject, IEcsProxyObject> {
    using ObjectBase =
        META_NS::ObjectFwd<EcsObject, ClassId::EcsObject, META_NS::ClassId::Object, IEcsObject, IEcsProxyObject>;

    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(
        SCENE_NS::IEcsObject, uint8_t, ConnectionStatus, SCENE_NS::IEcsObject::ECS_OBJ_STATUS_DISCONNECTED)

    bool Build(const IMetadata::Ptr& /*data*/) override
    {
        return true;
    }

    CORE_NS::IEcs::Ptr GetEcs() const override
    {
        return ecsInstance_;
    }

    CORE_NS::Entity GetEntity() const override
    {
        return entity_;
    }

    void SetEntity(CORE_NS::IEcs::Ptr ecs, CORE_NS::Entity entity) override
    {
        BindObject(nullptr, entity_);
        // Should we allow setting a valid ecs with invalid entity?
        if (CORE_NS::EntityUtil::IsValid(entity)) {
            BindObject(ecs, entity);
        }
    }

    void SetCommonListener(BASE_NS::shared_ptr<SCENE_NS::EcsListener> listener) override
    {
        if (auto commonListener = listener_.lock()) {
            for (auto& componentManager : componentManagers_) {
                commonListener->RemoveEntity(entity_, GetSelf<SCENE_NS::IEcsProxyObject>(), *componentManager->manager);
            }
            componentManagers_.clear();
        }
        listener_ = listener;
    }

    void BindObject(CORE_NS::IEcs::Ptr ecsInstance, CORE_NS::Entity entity) override
    {
        if (ecsInstance && CORE_NS::EntityUtil::IsValid(entity)) {
            ecsInstance_ = ecsInstance;
            entity_ = ecsInstance->GetEntityManager().GetReferenceCounted(entity);
            assert(componentManagers_.size() == 0);
            UpdateMetaCache();
            if (auto commonListener = listener_.lock()) {
                for (auto& componentManager : componentManagers_) {
                    commonListener->AddEntity(entity, GetSelf<SCENE_NS::IEcsProxyObject>(), *componentManager->manager);
                }
            }
            // Nb. we may reach connected state even we don't have listener or single valid component to listen
            META_ACCESS_PROPERTY(ConnectionStatus)->SetValue(SCENE_NS::IEcsObject::ECS_OBJ_STATUS_CONNECTED);
        } else if (ecsInstance_) {
            if (auto commonListener = listener_.lock()) {
                for (auto& componentManager : componentManagers_) {
                    commonListener->RemoveEntity(
                        entity, GetSelf<SCENE_NS::IEcsProxyObject>(), *componentManager->manager);
                }
            }
            for (auto& componentManager : componentManagers_) {
                componentManager->valueManager->RemoveAll();
                for (auto& prop : componentManager->properties) {
                    RemoveProperty(prop);
                }
            }

            attachments_.clear();
            componentManagers_.clear();
            entity_ = {};
            ecsInstance_.reset();
            META_ACCESS_PROPERTY(ConnectionStatus)->SetValue(SCENE_NS::IEcsObject::ECS_OBJ_STATUS_DISCONNECTED);
        }
    }

    ~EcsObject() override
    {
        BindObject(nullptr, entity_);
    }

    void DoEntityEvent(CORE_NS::IEcs::EntityListener::EventType type, const CORE_NS::Entity& /*entity*/) override
    {
        switch (type) {
            case CORE_NS::IEcs::EntityListener::EventType::CREATED: {
                break;
            }
            case CORE_NS::IEcs::EntityListener::EventType::ACTIVATED: {
                break;
            }
            case CORE_NS::IEcs::EntityListener::EventType::DEACTIVATED: {
                break;
            }
            case CORE_NS::IEcs::EntityListener::EventType::DESTROYED: {
                // detach from the entity. (stop listening for events, and detach the properties)
                BindObject(nullptr, entity_);
                break;
            }
        };
    }

    void DoComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
        const CORE_NS::IComponentManager& componentManager, const CORE_NS::Entity& entity) override
    {
        if (entity!=entity_) {
            CORE_LOG_F("Invalid event for %s entity %" PRIx64 " handler object entity %" PRIx64,BASE_NS::string(componentManager.GetName()).c_str(),entity.id,CORE_NS::Entity(entity_).id);
            return;
        }

        if (type == CORE_NS::IEcs::ComponentListener::EventType::CREATED) {
            // new component added. add it's properties
        }
        if (type == CORE_NS::IEcs::ComponentListener::EventType::DESTROYED) {
            // component removed. remove the properties.
        }
        if (type == CORE_NS::IEcs::ComponentListener::EventType::MODIFIED) {
            // possibly re-evaluate because of change to one of the components..
            //CORE_LOG_F("[%p] Modified event for %s entity %llx handler object entity %llx",this, BASE_NS::string(componentManager.GetName()).c_str(),entity.id,CORE_NS::Entity(entity_).id);
            UpdateProperties(componentManager);
        }
    }

    void UpdateMetaCache() const
    {
        auto obj = const_cast<EcsObject*>(this);
        obj->UpdateMetaCache();
    }

    void DefineTargetProperties(
        BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>> names) override
    {
        CORE_LOG_E("DefineTargetProperties");
        if (ecsInstance_) {
            UpdateMetaCache();
        }
    }

    BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>> names_;

    void UpdateMetaCache()
    {
        CORE_LOG_E("UpdateMetaCache");
        BASE_NS::vector<CORE_NS::IComponentManager*> managers;
        ecsInstance_->GetComponents(entity_, managers);
        bool addAll = names_.empty();
        for (auto cm : managers) {
            auto handle = cm->GetData(entity_);
            if (!handle) {
                break;
            }
            if (auto* papi = handle->Owner()) {
                BASE_NS::shared_ptr<ComponentManager> m;
                for(auto&& v : componentManagers_) {
                    if(v->manager == cm) {
                        m = v;
                        break;
                    }
                }
                if(!m) {
                    m = ComponentManager::Create(*cm);
                    componentManagers_.push_back(m);
                }

                // Add a special "Name" property containing the name from a NameComponent
                if (cm->GetName() == "NameComponent") {
                    for (int i = 0; i < papi->PropertyCount(); i++) {
                        const auto md = papi->MetaData(i);
                        if (md->type == CORE_NS::PropertyType::STRING_T && md->name == "name") {
                            m->valueManager->ConstructValue(META_NS::EnginePropertyParams{handle, *md}, {});
                            auto prop = GetPropertyByName("Name");
                            if (!prop) {
                                prop = META_NS::ConstructProperty<BASE_NS::string>("Name");
                                AddProperty(prop);
                            }
                            SetEngineValueToProperty(prop, m->valueManager->GetEngineValue("name"));
                            m->properties.push_back(prop);
                        }
                    }
                }
                BASE_NS::string name { cm->GetName() };
                BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
                AddEngineValuesRecursively(m->valueManager, handle, META_NS::EngineValueOptions{name, &values, true});
                for(auto&& v : values) {
                    if(auto prop = GetPropertyByName(v->GetName())) {
                        SetEngineValueToProperty(prop, v);
                    } else {
                        AddProperty(m->valueManager->ConstructProperty(v->GetName()));
                    }
                }
            }
        }
    }

    void UpdateProperties(const CORE_NS::IComponentManager& cm)
    {
        auto* papi = cm.GetData(entity_)->Owner();
        if (!papi) {
            return;
        }

        for (auto& active_mgr : componentManagers_) {
            if (active_mgr->manager == &cm) {
                active_mgr->valueManager->Sync(META_NS::EngineSyncDirection::FROM_ENGINE);
                return;
            }
        }
    }

    virtual BASE_NS::vector<CORE_NS::Entity> GetAttachments() override
    {
        return attachments_;
    }

    void AddAttachment(CORE_NS::Entity entity) override
    {
        attachments_.push_back(entity);
    }

    void RemoveAttachment(CORE_NS::Entity entity) override
    {
        // assuming that entities can be there only once
        for (auto attachment = attachments_.cbegin(); attachment != attachments_.cend(); attachment++) {
            if (attachment->id == entity.id) {
                attachments_.erase(attachment);
                break;
            }
        }
    }

    void Activate() override {}

    void Deactivate() override {}

public:
    EcsObject(const META_NS::IMetadata::Ptr& data) {}
    EcsObject() {}

private:
    CORE_NS::IEcs::Ptr ecsInstance_ {};
    CORE_NS::EntityReference entity_;
    BASE_NS::weak_ptr<SCENE_NS::EcsListener> listener_ {};
    uint64_t firstOffset_ { 0 };
    struct ComponentManager {
        static BASE_NS::shared_ptr<ComponentManager> Create(CORE_NS::IComponentManager& manager)
        {
            return BASE_NS::shared_ptr<ComponentManager> { new ComponentManager { &manager,
                META_NS::GetObjectRegistry().Create<META_NS::IEngineValueManager>(
                    META_NS::ClassId::EngineValueManager) } };
        }
        CORE_NS::IComponentManager* manager;
        META_NS::IEngineValueManager::Ptr valueManager;
        BASE_NS::vector<META_NS::IProperty::Ptr> properties;
    };

    BASE_NS::vector<BASE_NS::shared_ptr<ComponentManager>> componentManagers_;
    BASE_NS::vector<CORE_NS::Entity> attachments_;
};

void RegisterEcsObject()
{
    META_NS::GetObjectRegistry().RegisterObjectType<EcsObject>();
}

void UnregisterEcsObject()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<EcsObject>();
}

SCENE_END_NAMESPACE()
