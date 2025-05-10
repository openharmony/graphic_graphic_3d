/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Proxy Object implementation
 * Author: Lauri Jaaskela/Mikael Kilpeläinen
 * Create: 2022-10-17
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
    if (meta_) {
        ResetTargetListener();
        for (auto&& p : meta_->GetProperties()) {
            p->OnChanged()->RemoveHandler(uintptr_t(this));
        }
    }
}

bool ProxyObject::Build(const IMetadata::Ptr& data)
{
    bool ret = Super::Build(data);
    if (ret) {
        meta_ = interface_cast<META_NS::IMetadata>(Super::GetAttachmentContainer(true));
        if (!meta_) {
            return false;
        }

        Dynamic()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
            if (Dynamic()->GetValue()) {
                ListenTargetChanges();
                RefreshProperties();
            } else {
                ResetTargetListener();
            }
        }));

        for (auto&& p : meta_->GetProperties()) {
            p->OnChanged()->AddHandler(MakeCallback<IOnChanged>(
                                           [this](auto p) {
                                               if (p) {
                                                   OnPropertyChanged(p);
                                               }
                                           },
                                           p),
                uintptr_t(this));
        }

        if (auto meta = GetSelf<IAttach>()) {
            metaAdded_.Subscribe<IOnChildChanged>(
                meta->GetAttachmentContainer(true)->OnContainerChanged(), [this](const auto& i) {
                    if (interface_cast<IProperty>(i.object)) {
                        if (i.type == ContainerChangeType::ADDED) {
                            OnPropertyAdded(i);
                        } else if (i.type == ContainerChangeType::REMOVED) {
                            OnPropertyRemoved(i);
                        }
                    }
                });
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
    if (auto target = interface_pointer_cast<IAttach>(GetTarget())) {
        targetAddedListener_.Subscribe<IOnChildChanged>(
            target->GetAttachmentContainer(true)->OnContainerChanged(), [&](auto i) {
                if (interface_cast<IProperty>(i.object)) {
                    RefreshProperties();
                }
            });
    }
}

void ProxyObject::ResetTargetListener()
{
    targetAddedListener_.Unsubscribe();
    targetRemovedListener_.Unsubscribe();
}

void ProxyObject::RefreshProperties()
{
    if (!GetTarget()) {
        RemoveAllProxyProperties();
        return;
    }

    updating_ = true;

    auto propertiesToRemove = BASE_NS::vector<BASE_NS::string>();
    auto propertiesToAdd = BASE_NS::vector<BASE_NS::string>();

    for (auto&& property : proxyProperties_) {
        const auto& name = property.first;
        META_NS::IProperty::Ptr sourceProperty;

        if (auto internalSourceName = internalBindings_.find(name); internalSourceName != internalBindings_.end()) {
            auto targetSourceProperty = interface_cast<IMetadata>(GetTarget())->GetProperty(internalSourceName->second);

            if (targetSourceProperty) {
                propertiesToAdd.push_back(name);
                continue;
            }
            // internal binding source is bind to target property, so target property needs to exist
            // if not then we need to find property with same name as internal bind property
            internalBindings_.erase(internalSourceName);
            sourceProperty = interface_cast<IMetadata>(GetTarget())->GetProperty(name);
            if (!sourceProperty) {
                propertiesToRemove.push_back(name);
                propertiesToAdd.push_back(name);
                continue;
            }
        } else {
            sourceProperty = interface_cast<IMetadata>(GetTarget())->GetProperty(name);
            if (!sourceProperty) {
                propertiesToRemove.push_back(name);
                continue;
            }
        }
        auto& valueBind = proxyProperties_.find(name)->second;
        const auto bindingResult = valueBind.Bind(sourceProperty);
        // it is possible that property was removed, but property with same name (but different type) was added
        if (!bindingResult) {
            propertiesToRemove.push_back(name);
            propertiesToAdd.push_back(name);
        }
    }

    for (const auto& remove : propertiesToRemove) {
        RemoveProxyProperty(remove);
    }
    for (const auto& name : propertiesToAdd) {
        AddProxyProperty(name);
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
    for (auto&& p : meta_->GetProperties()) {
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
    if (auto p = m->GetProperty(name)) {
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
        if (auto p = meta_->GetProperty(property->GetName())) {
            proxyProperties_.insert_or_assign(p->GetName(), DefaultValueBind(p, property));
            res = p;
        } else {
            res = AddProxyProperty(property->GetName(), property);
        }
    }
    return res;
}

IProperty::Ptr ProxyObject::AddProxyProperty(BASE_NS::string_view name, const IProperty::ConstPtr& tp)
{
    auto p = meta_->GetProperty(name);
    if (!p) {
        p = DuplicatePropertyType(META_NS::GetObjectRegistry(), tp, name);
    }
    if (p) {
        proxyProperties_.insert_or_assign(p->GetName(), DefaultValueBind(p, tp));
        AddProperty(p);
    }
    return p;
}

IProperty::Ptr ProxyObject::AddProxyProperty(BASE_NS::string_view name)
{
    IProperty::Ptr p;
    if (internalBindings_.contains(name)) {
        auto source = GetProperty(internalBindings_[name]);
        if (!source) {
            return {};
        }
        p = AddProxyProperty(name, source);
    } else if (auto target = interface_pointer_cast<IMetadata>(GetTarget())) {
        if (auto tp = target->GetProperty(name)) {
            p = AddProxyProperty(name, tp);
        }
    }
    return p;
}

void ProxyObject::PopulateAllProperties()
{
    if (auto target = interface_pointer_cast<IMetadata>(GetTarget())) {
        for (auto&& p : target->GetProperties()) {
            auto it = proxyProperties_.find(p->GetName());
            if (it == proxyProperties_.end()) {
                AddProxyProperty(p->GetName(), p);
            }
        }
    }
}

IProperty::Ptr ProxyObject::GetProperty(BASE_NS::string_view name, MetadataQuery q)
{
    auto p = meta_->GetProperty(name, q);
    if (!p) {
        p = AddProxyProperty(name);
    }
    return p;
}

IProperty::ConstPtr ProxyObject::GetProperty(BASE_NS::string_view name, MetadataQuery q) const
{
    return const_cast<ProxyObject*>(this)->GetProperty(name, q);
}

bool ProxyObject::RemoveProperty(const IProperty::Ptr& p)
{
    bool res = meta_->RemoveProperty(p);
    proxyProperties_.erase(p->GetName());
    return res;
}

BASE_NS::vector<IProperty::Ptr> ProxyObject::GetProperties()
{
    PopulateAllProperties();
    return meta_->GetProperties();
}

BASE_NS::vector<IProperty::ConstPtr> ProxyObject::GetProperties() const
{
    const_cast<ProxyObject*>(this)->PopulateAllProperties();
    return const_cast<const IMetadata*>(meta_)->GetProperties();
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
            for (auto&& p : meta_->GetProperties()) {
                serialise = ShouldSerialise(p);
                if (serialise) {
                    break;
                }
            }
        }
        META_NS::SetObjectFlags(GetSelf(), ObjectFlagBits::SERIALIZE, serialise);
    }
}

void ProxyObject::RemoveProxyProperty(const BASE_NS::string& name)
{
    meta_->RemoveProperty(GetProperty(name));
    proxyProperties_.erase(name);
    internalBindings_.erase(name);
}

void ProxyObject::RemoveAllProxyProperties()
{
    for (const auto& p : meta_->GetProperties()) {
        RemoveProxyProperty(p->GetName());
    }
}

void ProxyObject::AddInternalProxy(BASE_NS::string_view propertyPropertyName, BASE_NS::string_view sourcePropertyName)
{
    auto internalSourceProperty = GetProperty(sourcePropertyName);
    if (!internalSourceProperty) {
        return;
    }

    if (internalBindings_.contains(propertyPropertyName)) {
        if (internalBindings_[propertyPropertyName] == sourcePropertyName) {
            return;
        }
        internalBindings_.erase(propertyPropertyName);
    }

    internalBindings_.insert({ BASE_NS::string(propertyPropertyName), BASE_NS::string(sourcePropertyName) });

    if (proxyProperties_.contains(propertyPropertyName)) {
        proxyProperties_.erase(propertyPropertyName);
    }
    auto bindProperty = meta_->GetProperty(propertyPropertyName);
    if (!bindProperty) {
        // if given property does not exist we create it when user ask for proxy property
        return;
    }
    proxyProperties_.insert({ propertyPropertyName, DefaultValueBind(bindProperty, internalSourceProperty) });
}

} // namespace Internal

META_END_NAMESPACE()
