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

#include "proxy_object.h"

#include <algorithm>

#include <meta/api/make_callback.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/api/util.h>

META_BEGIN_NAMESPACE()

namespace Internal {

ProxyObject::~ProxyObject()
{
    if (GetMetadata()) {
        ResetTargetListener();
        for (auto&& p : Super::GetAllProperties()) {
            p->OnChanged()->RemoveHandler(uintptr_t(this));
        }
    }
}

bool ProxyObject::Build(const IMetadata::Ptr& data)
{
    bool ret = Super::Build(data);
    if (ret) {
        Dynamic()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
            if (Dynamic()->GetValue()) {
                ListenTargetChanges();
                RefreshProperties();
            } else {
                ResetTargetListener();
            }
        }));

        for (auto&& p : Super::GetAllProperties()) {
            p->OnChanged()->AddHandler(MakeCallback<IOnChanged>(
                                           [this](auto p) {
                                               if (p) {
                                                   OnPropertyChanged(p);
                                               }
                                           },
                                           p),
                uintptr_t(this));
        }

        if (auto meta = GetSelf<IMetadata>()) {
            metaAdded_.Subscribe<IOnChildChanged>(
                meta->GetPropertyContainer()->OnAdded(), [this](const auto& i) { OnPropertyAdded(i); });
            metaRemoved_.Subscribe<IOnChildChanged>(
                meta->GetPropertyContainer()->OnRemoved(), [this](const auto& i) { OnPropertyRemoved(i); });
        }
        UpdateSerializeState();
    }
    return ret;
}

static bool SerializeEmbeddedProxy(IProperty::Ptr p)
{
    // Check for embedded proxy objects
    if (auto proxy = GetPointer<IProxyObject>(p)) {
        if (auto f = interface_cast<IObjectFlags>(proxy)) {
            return static_cast<bool>(f->GetObjectFlags() & ObjectFlagBits::SERIALIZE);
        }
    }
    return false;
}

void ProxyObject::OnPropertyAdded(const ChildChangedInfo& info)
{
    if (auto p = interface_pointer_cast<IProperty>(info.object)) {
        auto f = GetSelf<IObjectFlags>();
        if (!updating_ && f && !(f->GetObjectFlags() & ObjectFlagBits::SERIALIZE)) {
            META_NS::SetObjectFlags(f, ObjectFlagBits::SERIALIZE, ShouldSerialise(p));
        }
        p->OnChanged()->AddHandler(MakeCallback<IOnChanged>(
                                       [this](auto p) {
                                           if (p) {
                                               OnPropertyChanged(p);
                                           }
                                       },
                                       p),
            uintptr_t(this));
    }
}

void ProxyObject::OnPropertyRemoved(const ChildChangedInfo& info)
{
    if (auto p = interface_cast<IProperty>(info.object)) {
        p->OnChanged()->RemoveHandler(uintptr_t(this));
    }
    auto f = GetSelf<IObjectFlags>();
    if (!updating_ && f && f->GetObjectFlags() & ObjectFlagBits::SERIALIZE) {
        UpdateSerializeState();
    }
}

void ProxyObject::OnPropertyChanged(const IProperty::Ptr& p)
{
    auto f = GetSelf<IObjectFlags>();
    if (!updating_ && f) {
        if (f->GetObjectFlags() & ObjectFlagBits::SERIALIZE) {
            UpdateSerializeState();
        } else {
            META_NS::SetObjectFlags(f, ObjectFlagBits::SERIALIZE, ShouldSerialise(p));
        }
    }
}

void ProxyObject::ListenTargetChanges()
{
    if (auto target = interface_pointer_cast<IMetadata>(GetTarget())) {
        targetAddedListener_.Subscribe<IOnChildChanged>(
            target->GetPropertyContainer()->OnAdded(), [&](auto) { RefreshProperties(); });
        targetRemovedListener_.Subscribe<IOnChildChanged>(
            target->GetPropertyContainer()->OnRemoved(), [&](auto) { RefreshProperties(); });
    }
}

void ProxyObject::ResetTargetListener()
{
    targetAddedListener_.Unsubscribe();
    targetRemovedListener_.Unsubscribe();
}

void ProxyObject::RefreshProperties()
{
    updating_ = true;
    if (auto target = interface_pointer_cast<IMetadata>(GetTarget())) {
        auto props = BASE_NS::move(proxyProperties_);
        for (auto&& p : props) {
            if (auto tp = target->GetPropertyByName(p.first)) {
                auto res = proxyProperties_.insert({ p.first, p.second });
                res.first->second.Bind(tp);
            } else {
                Super::RemoveProperty(p.second.GetProperty());
            }
        }
    } else {
        // remove all we added
        for (auto&& p : proxyProperties_) {
            Super::RemoveProperty(p.second.GetProperty());
        }
        proxyProperties_.clear();
    }
    updating_ = false;
    UpdateSerializeState();
}

const IObject::Ptr ProxyObject::GetTarget() const
{
    return target_.lock();
}

bool ProxyObject::SetTarget(const IObject::Ptr& target)
{
    ResetTargetListener();
    target_ = target;
    if (Dynamic()->GetValue()) {
        ListenTargetChanges();
    }
    RefreshProperties();
    if (META_ACCESS_PROPERTY_VALUE(Mode) & ProxyMode::REFLECT_PROXY_HIERARCHY) {
        ReflectHierarchy(target);
    }
    return true;
}

void ProxyObject::ReflectHierarchy(const IObject::Ptr& target)
{
    auto m = interface_pointer_cast<IMetadata>(target);
    if (!m) {
        return;
    }
    for (auto&& p : Super::GetAllProperties()) {
        // reflect only non-proxyable properties
        if (!proxyProperties_.count(p->GetName())) {
            if (auto proxy = GetPointer<IProxyObject>(p)) {
                ReflectTargetForProperty(m, p->GetName(), proxy);
            }
        }
    }
}

void ProxyObject::ReflectTargetForProperty(
    const IMetadata::Ptr& m, BASE_NS::string_view name, const IProxyObject::Ptr& proxy)
{
    if (auto p = m->GetPropertyByName(name)) {
        if (auto tp = GetPointer<IObject>(p)) {
            proxy->SetTarget(tp);
        }
    }
}

BASE_NS::vector<IProperty::ConstPtr> ProxyObject::GetOverrides() const
{
    BASE_NS::vector<IProperty::ConstPtr> res;
    for (auto&& p : proxyProperties_) {
        if (!p.second.IsDefaultValue()) {
            res.push_back(p.second.GetProperty());
        }
    }
    return res;
}

IProperty::ConstPtr ProxyObject::GetOverride(BASE_NS::string_view name) const
{
    auto it = proxyProperties_.find(name);
    return it != proxyProperties_.end() && !it->second.IsDefaultValue() ? it->second.GetProperty() : nullptr;
}

IProperty::ConstPtr ProxyObject::GetProxyProperty(BASE_NS::string_view name) const
{
    auto it = proxyProperties_.find(name);
    return it != proxyProperties_.end() ? it->second.GetProperty() : nullptr;
}

IProperty::Ptr ProxyObject::SetPropertyTarget(const IProperty::Ptr& property)
{
    if (!property) {
        return nullptr;
    }
    IProperty::Ptr res;
    bool add = true;
    auto it = proxyProperties_.find(property->GetName());
    if (it != proxyProperties_.end()) {
        auto bprop = it->second.GetProperty();
        add = !bprop || bprop->GetTypeId() != property->GetTypeId();
        if (add) {
            proxyProperties_.erase(it);
        } else {
            it->second.Bind(property);
            res = it->second.GetProperty();
        }
    }
    if (add && property) {
        if (auto p = Super::GetPropertyByName(property->GetName())) {
            auto r = proxyProperties_.insert_or_assign(p->GetName(), DefaultValueBind(p));
            r.first->second.Bind(property);
            res = p;
        } else {
            res = AddProxyProperty(property);
        }
    }
    return res;
}

IProperty::Ptr ProxyObject::AddProxyProperty(const IProperty::ConstPtr& tp)
{
    auto p = DuplicatePropertyType(META_NS::GetObjectRegistry(), tp);
    if (p) {
        auto res = proxyProperties_.insert_or_assign(p->GetName(), DefaultValueBind(p));
        res.first->second.Bind(tp);
        AddProperty(p);
    }
    return p;
}

IProperty::Ptr ProxyObject::AddProxyProperty(BASE_NS::string_view name)
{
    IProperty::Ptr p;
    if (auto target = interface_pointer_cast<IMetadata>(GetTarget())) {
        if (auto tp = target->GetPropertyByName(name)) {
            p = AddProxyProperty(tp);
        }
    }
    return p;
}

void ProxyObject::PopulateAllProperties()
{
    if (auto target = interface_pointer_cast<IMetadata>(GetTarget())) {
        for (auto&& p : target->GetAllProperties()) {
            auto it = proxyProperties_.find(p->GetName());
            if (it == proxyProperties_.end()) {
                AddProxyProperty(p);
            }
        }
    }
}

IProperty::Ptr ProxyObject::GetPropertyByName(BASE_NS::string_view name)
{
    auto p = Super::GetPropertyByName(name);
    if (!p) {
        p = AddProxyProperty(name);
    }
    return p;
}

IProperty::ConstPtr ProxyObject::GetPropertyByName(BASE_NS::string_view name) const
{
    return const_cast<ProxyObject*>(this)->GetPropertyByName(name);
}

void ProxyObject::RemoveProperty(const IProperty::Ptr& p)
{
    Super::RemoveProperty(p);
    proxyProperties_.erase(p->GetName());
}

BASE_NS::vector<IProperty::Ptr> ProxyObject::GetAllProperties()
{
    PopulateAllProperties();
    return Super::GetAllProperties();
}

BASE_NS::vector<IProperty::ConstPtr> ProxyObject::GetAllProperties() const
{
    const_cast<ProxyObject*>(this)->PopulateAllProperties();
    return Super::GetAllProperties();
}

bool ProxyObject::ShouldSerialise(const IProperty::Ptr& p) const
{
    auto s = interface_cast<IStackProperty>(p);
    return (s && !s->GetValues({}, false).empty()) ||
           (!IsFlagSet(p, ObjectFlagBits::NATIVE) && !proxyProperties_.count(p->GetName())) ||
           SerializeEmbeddedProxy(p);
}

void ProxyObject::UpdateSerializeState()
{
    if (!updating_) {
        bool serialise = !GetOverrides().empty();
        if (!serialise) {
            for (auto&& p : Super::GetAllProperties()) {
                serialise = ShouldSerialise(p);
                if (serialise) {
                    break;
                }
            }
        }
        META_NS::SetObjectFlags(GetSelf(), ObjectFlagBits::SERIALIZE, serialise);
    }
}

} // namespace Internal

META_END_NAMESPACE()
