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

#ifndef META_SRC_PROXY_OBJECT_H
#define META_SRC_PROXY_OBJECT_H

#include <base/containers/unordered_map.h>

#include <meta/api/event_handler.h>
#include <meta/api/property/default_value_bind.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/ext/object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_proxy_object.h>

#include "object.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class ProxyObject final : public Internal::ObjectFwd<ProxyObject, META_NS::ClassId::ProxyObject, IProxyObject> {
    using Super = Internal::ObjectFwd<ProxyObject, META_NS::ClassId::ProxyObject, IProxyObject>;

public:
    ProxyObject() = default;
    ~ProxyObject() override;
    META_NO_COPY_MOVE(ProxyObject)

public: // ILifecycle
    bool Build(const IMetadata::Ptr& data) override;

public: // IMetadata
    IProperty::Ptr GetPropertyByName(BASE_NS::string_view name) override;
    IProperty::ConstPtr GetPropertyByName(BASE_NS::string_view name) const override;
    void RemoveProperty(const IProperty::Ptr&) override;

    BASE_NS::vector<IProperty::Ptr> GetAllProperties() override;
    BASE_NS::vector<IProperty::ConstPtr> GetAllProperties() const override;

public: // IProxyObject
    META_IMPLEMENT_INTERFACE_PROPERTY(IProxyObject, ProxyModeBitsValue, Mode, ProxyMode::REFLECT_PROXY_HIERARCHY)
    META_IMPLEMENT_INTERFACE_PROPERTY(IProxyObject, bool, Dynamic, true);
    const IObject::Ptr GetTarget() const override;
    bool SetTarget(const IObject::Ptr& target) override;
    BASE_NS::vector<IProperty::ConstPtr> GetOverrides() const override;
    IProperty::ConstPtr GetOverride(BASE_NS::string_view name) const override;
    IProperty::Ptr SetPropertyTarget(const IProperty::Ptr& property) override;
    IProperty::ConstPtr GetProxyProperty(BASE_NS::string_view name) const override;

private:
    void ListenTargetChanges();
    void ResetTargetListener();
    void RefreshProperties();
    IProperty::Ptr AddProxyProperty(const IProperty::ConstPtr& tp);
    IProperty::Ptr AddProxyProperty(BASE_NS::string_view name);
    void PopulateAllProperties();
    bool ShouldSerialise(const IProperty::Ptr& p) const;
    void UpdateSerializeState();
    void ReflectHierarchy(const IObject::Ptr& target);
    void ReflectTargetForProperty(const IMetadata::Ptr& m, BASE_NS::string_view name, const IProxyObject::Ptr& proxy);

    void OnPropertyAdded(const ChildChangedInfo& info);
    void OnPropertyRemoved(const ChildChangedInfo& info);
    void OnPropertyChanged(const IProperty::Ptr& p);

private:
    IObject::WeakPtr target_;
    mutable BASE_NS::unordered_map<BASE_NS::string, DefaultValueBind> proxyProperties_;
    EventHandler targetAddedListener_;
    EventHandler targetRemovedListener_;
    EventHandler metaAdded_;
    EventHandler metaRemoved_;
    bool updating_ {};
};

} // namespace Internal

META_END_NAMESPACE()

#endif
