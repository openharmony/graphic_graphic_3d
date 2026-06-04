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
#include <meta/interface/intf_static_metadata.h>

SCENE_BEGIN_NAMESPACE()

namespace Internal {

// Look up static metadata to find the property name that maps to an engine path.
// E.g. "CameraComponent.zFar" -> "FarPlane". Returns empty if no mapping exists.
BASE_NS::string_view FindStaticNameForEnginePath(META_NS::IStaticMetadata* staticMeta, BASE_NS::string_view enginePath)
{
    if (!staticMeta) {
        return {};
    }
    for (auto* smd = staticMeta->GetStaticMetadata(); smd; smd = smd->baseclass) {
        for (size_t i = 0; i < smd->size; ++i) {
            auto& entry = smd->metadata[i];
            if (entry.type != META_NS::MetadataType::PROPERTY || !entry.data) {
                continue;
            }
            if (enginePath == static_cast<const char*>(entry.data)) {
                return entry.name;
            }
        }
    }
    return {};
}

META_NS::IProperty::Ptr FindPropertyFromMetadata(META_NS::IMetadata& meta, BASE_NS::string_view name)
{
    for (auto&& p : meta.GetProperties()) {
        if (auto ev = META_NS::GetEngineValueFromProperty(p)) {
            if (ev->GetName() == name) {
                return p;
            }
        }
    }
    return {};
}

}  // namespace Internal

bool EcsObject::Build(const META_NS::IMetadata::Ptr& d)
{
    if (Super::Build(d)) {
        auto s = GetInterfaceBuildArg<IInternalScene>(d, "Scene");
        auto ent = GetBuildArg<CORE_NS::Entity>(d, "Entity");

        if (s && CORE_NS::EntityUtil::IsValid(ent)) {
            valueManager_ =
                META_NS::GetObjectRegistry().Create<META_NS::IEngineValueManager>(META_NS::ClassId::EngineValueManager);
            valueManager_->SetNotificationQueue(s->GetContext()->GetApplicationQueue());
            valueManager_->SetDirtyNotifier(META_NS::MakeCallback<META_NS::IOnChanged>(
                [self = BASE_NS::weak_ptr(GetSelf<META_NS::IEnginePropertySync>()),
                    scene = IInternalScene::WeakPtr(s)] {
                    if (auto me = self.lock()) {
                        if (auto sc = scene.lock()) {
                            sc->SchedulePropertyUpdate(me);
                        }
                    }
                }));
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
        return SyncPropertyDirect(s, Name()).GetResult();
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

bool EcsObject::AddEngineProperty(META_NS::IMetadata& object, const META_NS::IEngineValue::Ptr& v)
{
    if (!v) {
        return false;
    }
    const auto name = v->GetName();
    auto prop = object.GetProperty(name);
    if (prop) {
        if (!META_NS::GetEngineValueFromProperty(prop)) {
            return META_NS::SetEngineValueToProperty(prop, v);
        }
    }
    // A static property (e.g. "FarPlane") may already cover this engine path
    // ("CameraComponent.zFar"). Check for existing engine-value bindings first,
    // then check static metadata for unbound properties that map to this path.
    for (auto&& p : object.GetProperties()) {
        if (auto ev = META_NS::GetEngineValueFromProperty(p)) {
            if (ev->GetName() == name) {
                return true;  // already has engine value + hook from static init
            }
        }
    }
    // Check static metadata: a property like "FarPlane" may map to "CameraComponent.zFar"
    // but hasn't had its engine value bound yet. Bind it now to avoid creating a duplicate.
    auto staticName = Internal::FindStaticNameForEnginePath(interface_cast<META_NS::IStaticMetadata>(&object), name);
    if (!staticName.empty()) {
        prop = object.GetProperty(staticName);
        if (prop) {
            return META_NS::SetEngineValueToProperty(prop, v);
        }
    }
    prop = valueManager_->ConstructProperty(name);
    if (prop) {
        object.AddProperty(prop);
        return true;
    }
    CORE_LOG_E("Failed to add %s", name.c_str());
    return false;
}

void EcsObject::AddAllProperties(META_NS::IMetadata& object, CORE_NS::IComponentManager* m)
{
    BASE_NS::string name{m->GetName()};
    BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
    META_NS::AddEngineValuesRecursively(
        valueManager_, META_NS::EnginePropertyHandle{m, entity_}, META_NS::EngineValueOptions{name, &values});
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

Future<bool> EcsObject::AddAllProperties(const META_NS::IMetadata::Ptr& object, BASE_NS::string_view cv)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTaskOrRunDirectly(
            [=, me = BASE_NS::weak_ptr(GetSelf()), component = BASE_NS::string(cv)]() mutable {
                if (!me.expired()) {
                    return HandleAddAllProperties(object, component);
                }
                return false;
            });
    }
    return {};
}

bool EcsObject::HandleAddAllProperties(const META_NS::IMetadata::Ptr& object, const BASE_NS::string& component)
{
    if (component.empty()) {
        AddAllComponentProperties(*object);
        return true;
    }
    return HandleComponentProperties(object, component);
}

bool EcsObject::HandleComponentProperties(const META_NS::IMetadata::Ptr& object, const BASE_NS::string& component)
{
    if (auto s = GetScene()) {
        auto name = ComponentName(component);
        if (auto m = s->GetEcsContext().FindComponent(name, entity_)) {
            AddAllProperties(*object, m);
            return true;
        }
    }
    return false;
}

bool EcsObject::AttachEnginePropertyImpl(META_NS::EnginePropertyHandle handle, const BASE_NS::string& component,
    const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
    if (valueManager_->ConstructValue(
            handle, FullPropertyName(path), META_NS::EngineValueOptions{component, &values})) {
        if (!values.empty()) {
            return META_NS::SetEngineValueToProperty(p, values.front());
        }
        // this most likely means we don't have registered access type for the property, lets continue anyhow for now
        return true;
    }
    return false;
}

META_NS::EnginePropertyHandle EcsObject::GetPropertyHandle(BASE_NS::string_view component) const
{
    if (auto s = GetScene()) {
        if (auto m = s->GetEcsContext().FindComponent(component, entity_)) {
            return {m, entity_};
        }
    }
    return {};
}

Future<bool> EcsObject::AttachProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view pv)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTaskOrRunDirectly([=, me = BASE_NS::weak_ptr(GetSelf()), path = BASE_NS::string(pv)] {
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
        return scene->AddTaskOrRunDirectly([=, me = BASE_NS::weak_ptr(GetSelf()), path = BASE_NS::string(pv)] {
            if (auto self = me.lock()) {
                auto component = ComponentName(path);
                if (auto handle = GetPropertyHandle(component)) {
                    auto name = FullPropertyName(path);
                    if (valueManager_->ConstructValue(
                            handle, name, META_NS::EngineValueOptions{BASE_NS::string(component)})) {
                        return valueManager_->ConstructProperty(path);
                    }
                }
            }
            return META_NS::IProperty::Ptr{};
        });
    }
    return {};
}

Future<META_NS::IProperty::Ptr> EcsObject::CreateProperty(const META_NS::IEngineValue::Ptr& value)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTaskOrRunDirectly([=, me = BASE_NS::weak_ptr(GetSelf())] {
            if (auto self = me.lock()) {
                BASE_NS::string pname(SCENE_NS::PropertyName(value->GetName()));
                return META_NS::PropertyFromEngineValue(pname, value);
            }
            return META_NS::IProperty::Ptr{};
        });
    }
    return {};
}

Future<bool> EcsObject::AttachProperty(const META_NS::IProperty::Ptr& prop, const META_NS::IEngineValue::Ptr& value)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTaskOrRunDirectly([=, me = BASE_NS::weak_ptr(GetSelf())] {
            if (auto self = me.lock()) {
                return META_NS::SetEngineValueToProperty(prop, value);
            }
            return false;
        });
    }
    return {};
}

Future<META_NS::IProperty::Ptr> EcsObject::ResolveProperty(
    const META_NS::IMetadata::Ptr& meta, BASE_NS::string_view name, META_NS::MetadataQuery q)
{
    auto scene = scene_.lock();
    return scene ? scene->AddTaskOrRunDirectly(
                       [this, meta, name = BASE_NS::string(name), q, me = BASE_NS::weak_ptr(GetSelf())] {
                           auto self = me.lock();
                           if (!(self && meta)) {
                               return META_NS::IProperty::Ptr{};
                           }
                           META_NS::IProperty::Ptr property;
                           // Check for static metadata->component property mapping
                           auto staticName = Internal::FindStaticNameForEnginePath(
                               interface_cast<META_NS::IStaticMetadata>(meta), name);
                           if (!staticName.empty()) {
                               property = meta->GetProperty(staticName, q);
                           }
                           if (!property) {
                               // Fallback, go through all existing properties and for matching value in engine
                               // values
                               property = Internal::FindPropertyFromMetadata(*meta, name);
                           }
                           if (!property && q == META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST) {
                               property = CreateProperty(name).GetResult();
                               if (property) {
                                   meta->AddProperty(property);
                               }
                           }
                           return property;
                       })
                 : Future<META_NS::IProperty::Ptr>{};
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

static void RemoveComponentPointers(
    const META_NS::IEngineValueManager::Ptr& m, const CORE_NS::IComponentManager* manager)
{
    for (auto&& v : m->GetAllEngineValues()) {
        if (auto acc = interface_cast<META_NS::IEngineValueInternal>(v)) {
            META_NS::InterfaceUniqueLock lock{v};
            auto param = acc->GetPropertyParams();
            if (param.handle.manager == manager) {
                param.handle.manager = nullptr;
                acc->SetPropertyParams(param);
            }
        }
    }
}

void EcsObject::OnComponentEvent(
    CORE_NS::IEcs::ComponentListener::EventType type, const CORE_NS::IComponentManager& component)
{
    if (type == CORE_NS::IEcs::ComponentListener::EventType::MODIFIED) {
        if (valueManager_->HasValues()) {
            valueManager_->Sync(META_NS::EngineSyncDirection::AUTO, &component);
        }
    } else if (type == CORE_NS::IEcs::ComponentListener::EventType::DESTROYED) {
        RemoveComponentPointers(valueManager_, &component);
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
            CORE_LOG_D("Failed to attach '%s' to '%s', falling back to normal property",
                p->GetName().c_str(),
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
        return s->AddTaskOrRunDirectly([=, me = BASE_NS::weak_ptr(GetSelf<IEcsObject>())] {
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
