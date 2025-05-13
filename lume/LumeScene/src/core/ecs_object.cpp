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
#include "ecs_object.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/util.h>

#include <3d/ecs/components/name_component.h>
#include <core/property_tools/property_data.h>

#include <meta/api/engine/util.h>
#include <meta/api/make_callback.h>

SCENE_BEGIN_NAMESPACE()

bool EcsObject::Build(const META_NS::IMetadata::Ptr& d)
{
    if (Super::Build(d)) {
        auto s = GetInterfaceBuildArg<IInternalScene>(d, "Scene");
        auto ent = GetBuildArg<CORE_NS::Entity>(d, "Entity");

        if (s && CORE_NS::EntityUtil::IsValid(ent)) {
            valueManager_ =
                META_NS::GetObjectRegistry().Create<META_NS::IEngineValueManager>(META_NS::ClassId::EngineValueManager);
            valueManager_->SetNotificationQueue(s->GetContext()->GetApplicationQueue());
            scene_ = s;
            entity_ = ent;
        }
    }
    return valueManager_ != nullptr && CORE_NS::EntityUtil::IsValid(entity_);
}

BASE_NS::string EcsObject::GetName() const
{
    return META_NS::GetValue(Name());
}

bool EcsObject::SetName(const BASE_NS::string& name)
{
    if (auto s = GetScene()) {
        META_NS::SetValue<BASE_NS::string>(Name(), name);
        return SyncProperty(s, Name()).GetResult();
    }
    return false;
}

BASE_NS::string EcsObject::GetPath() const
{
    if (auto s = GetScene()) {
        return s->GetEcsContext().GetPath(entity_);
    }
    return "";
}

IInternalScene::Ptr EcsObject::GetScene() const
{
    return scene_.lock();
}

CORE_NS::Entity EcsObject::GetEntity() const
{
    return entity_;
}

META_NS::IEngineValueManager::Ptr EcsObject::GetEngineValueManager()
{
    return valueManager_;
}

void EcsObject::AddPropertyUpdateHook(const META_NS::IProperty::Ptr& prop)
{
    if (prop) {
        prop->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([ecsobj = BASE_NS::weak_ptr(GetSelf<IEcsObject>())] {
                if (auto eobj = ecsobj.lock()) {
                    eobj->GetScene()->SchedulePropertyUpdate(eobj);
                }
            }));
    }
}

bool EcsObject::AddEngineProperty(META_NS::IMetadata& object, const META_NS::IEngineValue::Ptr& v)
{
    auto prop = object.GetProperty(v->GetName());
    if (prop) {
        META_NS::SetEngineValueToProperty(prop, v);
    } else {
        prop = valueManager_->ConstructProperty(v->GetName());
        if (prop) {
            object.AddProperty(prop);
        } else {
            CORE_LOG_E("Failed to add %s", v->GetName().c_str());
            return false;
        }
    }
    AddPropertyUpdateHook(prop);
    return true;
}

void EcsObject::AddAllProperties(META_NS::IMetadata& object, CORE_NS::IComponentManager* m)
{
    BASE_NS::string name { m->GetName() };
    BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
    META_NS::AddEngineValuesRecursively(
        valueManager_, META_NS::EnginePropertyHandle { m, entity_ }, META_NS::EngineValueOptions { name, &values });
    for (auto&& v : values) {
        AddEngineProperty(object, v);
    }
}

void EcsObject::AddAllComponentProperties(META_NS::IMetadata& object)
{
    BASE_NS::vector<CORE_NS::IComponentManager*> managers;
    if (auto s = GetScene()) {
        s->GetEcsContext().GetNativeEcs()->GetComponents(entity_, managers);
        for (auto m : managers) {
            AddAllProperties(object, m);
        }
    }
}

bool EcsObject::AddAllNamedComponentProperties(META_NS::IMetadata& object, BASE_NS::string_view cv)
{
    BASE_NS::vector<CORE_NS::IComponentManager*> managers;
    if (auto s = GetScene()) {
        s->GetEcsContext().GetNativeEcs()->GetComponents(entity_, managers);
        for (auto m : managers) {
            if (m->GetName() == cv) {
                AddAllProperties(object, m);
                return true;
            }
        }
    }
    return false;
}

Future<bool> EcsObject::AddAllProperties(const META_NS::IMetadata::Ptr& object, BASE_NS::string_view cv)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=, me = BASE_NS::weak_ptr(GetSelf()), component = BASE_NS::string(cv)] {
            if (auto self = me.lock()) {
                if (component.empty()) {
                    AddAllComponentProperties(*object);
                    return true;
                }
                if (auto s = GetScene()) {
                    auto name = ComponentName(component);
                    if (auto m = s->GetEcsContext().FindComponent(name)) {
                        AddAllProperties(*object, m);
                        return true;
                    }
                    return AddAllNamedComponentProperties(*object, name);
                }
            }
            return false;
        });
    }
    return {};
}

bool EcsObject::AttachEnginePropertyImpl(META_NS::EnginePropertyHandle handle, const BASE_NS::string& component,
    const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
    if (valueManager_->ConstructValue(
            handle, FullPropertyName(path), META_NS::EngineValueOptions { component, &values })) {
        if (!values.empty()) {
            auto ret = META_NS::SetEngineValueToProperty(p, values.front());
            if (ret) {
                AddPropertyUpdateHook(p);
            }
            return ret;
        }
        // this most likely means we don't have registered access type for the property, lets continue anyhow for now
        return true;
    }
    return false;
}

META_NS::EnginePropertyHandle EcsObject::GetPropertyHandle(BASE_NS::string_view component) const
{
    if (auto s = GetScene()) {
        if (auto m = s->GetEcsContext().FindComponent(component)) {
            return { m, entity_ };
        }
    }
    return {};
}

Future<bool> EcsObject::AttachProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view pv)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=, me = BASE_NS::weak_ptr(GetSelf()), path = BASE_NS::string(pv)] {
            if (auto self = me.lock()) {
                auto component = ComponentName(path);
                if (auto handle = GetPropertyHandle(component)) {
                    return AttachEnginePropertyImpl(handle, BASE_NS::string(component), p, path);
                }
            }
            return false;
        });
    }
    return {};
}

Future<META_NS::IProperty::Ptr> EcsObject::CreateProperty(BASE_NS::string_view pv)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=, me = BASE_NS::weak_ptr(GetSelf()), path = BASE_NS::string(pv)] {
            if (auto self = me.lock()) {
                auto component = ComponentName(path);
                if (auto handle = GetPropertyHandle(component)) {
                    auto name = FullPropertyName(path);
                    if (valueManager_->ConstructValue(
                            handle, name, META_NS::EngineValueOptions { BASE_NS::string(component) })) {
                        if (auto p = valueManager_->ConstructProperty(path)) {
                            AddPropertyUpdateHook(p);
                            return p;
                        }
                    }
                }
            }
            return META_NS::IProperty::Ptr {};
        });
    }
    return {};
}

Future<META_NS::IProperty::Ptr> EcsObject::CreateProperty(const META_NS::IEngineValue::Ptr& value)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=, me = BASE_NS::weak_ptr(GetSelf())] {
            if (auto self = me.lock()) {
                BASE_NS::string pname(SCENE_NS::PropertyName(value->GetName()));
                if (auto p = META_NS::PropertyFromEngineValue(pname, value)) {
                    AddPropertyUpdateHook(p);
                    return p;
                }
            }
            return META_NS::IProperty::Ptr {};
        });
    }
    return {};
}

Future<bool> EcsObject::AttachProperty(const META_NS::IProperty::Ptr& prop, const META_NS::IEngineValue::Ptr& value)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=, me = BASE_NS::weak_ptr(GetSelf())] {
            if (auto self = me.lock()) {
                if (META_NS::SetEngineValueToProperty(prop, value)) {
                    AddPropertyUpdateHook(prop);
                    return true;
                }
            }
            return false;
        });
    }
    return {};
}

void EcsObject::SyncProperties()
{
    valueManager_->Sync(META_NS::EngineSyncDirection::TO_ENGINE);
}

void EcsObject::OnEntityEvent(CORE_NS::IEcs::EntityListener::EventType type)
{
    if (type == CORE_NS::IEcs::EntityListener::EventType::CREATED) {
    }
    if (type == CORE_NS::IEcs::EntityListener::EventType::DESTROYED) {
        valueManager_->RemoveAll();
    }
}

void EcsObject::OnComponentEvent(
    CORE_NS::IEcs::ComponentListener::EventType type, const CORE_NS::IComponentManager& component)
{
    if (type == CORE_NS::IEcs::ComponentListener::EventType::MODIFIED) {
        valueManager_->Sync(META_NS::EngineSyncDirection::AUTO);
    }
}

bool EcsObject::AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    return AttachProperty(p, path).GetResult();
}

bool EcsObject::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "Name") {
        if (!AttachEngineProperty(p, path)) {
            CORE_LOG_D("Failed to attach '%s' to '%s', falling back to normal property", p->GetName().c_str(),
                BASE_NS::string(path).c_str());
            // no name component? Fallback to normal property
        }
        return true;
    }
    return AttachEngineProperty(p, path);
}

Future<bool> EcsObject::SetActive(bool active)
{
    if (auto s = GetScene()) {
        return s->AddTask([=, me = BASE_NS::weak_ptr(GetSelf<IEcsObject>())] {
            if (auto self = me.lock()) {
                s->SetEntityActive(self, active);
                return true;
            }
            return false;
        });
    }
    return {};
}

SCENE_END_NAMESPACE()
