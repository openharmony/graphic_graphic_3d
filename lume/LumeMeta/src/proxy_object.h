/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Proxy Object implementation
 * Author: Lauri Jaaskela
 * Create: 2022-10-17
 */

#ifndef META_SRC_PROXY_OBJECT_H
#define META_SRC_PROXY_OBJECT_H

#include <base/containers/unordered_map.h>

#include <meta/api/event_handler.h>
#include <meta/api/property/default_value_bind.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_proxy_object.h>

#include "object.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class ProxyObject final : public IntroduceInterfaces<MetaObject, IProxyObject> {
    META_OBJECT(ProxyObject, META_NS::ClassId::ProxyObject, IntroduceInterfaces)
public:
    ProxyObject() = default;
    ~ProxyObject() override;
    META_NO_COPY_MOVE(ProxyObject)

public: // ILifecycle
    bool Build(const IMetadata::Ptr& data) override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IProxyObject, ProxyModeBitsValue, Mode, ProxyMode::REFLECT_PROXY_HIERARCHY)
    META_STATIC_PROPERTY_DATA(IProxyObject, bool, Dynamic, true)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(ProxyModeBitsValue, Mode)
    META_IMPLEMENT_PROPERTY(bool, Dynamic)

public: // IMetadata
    using IMetadata::GetProperty;
    IProperty::Ptr GetProperty(BASE_NS::string_view name, MetadataQuery) override;
    IProperty::ConstPtr GetProperty(BASE_NS::string_view name, MetadataQuery) const override;
    bool RemoveProperty(const IProperty::Ptr&) override;

    BASE_NS::vector<IProperty::Ptr> GetProperties() override;
    BASE_NS::vector<IProperty::ConstPtr> GetProperties() const override;

public: // IProxyObject
    const IObject::Ptr GetTarget() const override;
    bool SetTarget(const IObject::Ptr& target) override;
    BASE_NS::vector<IProperty::ConstPtr> GetOverrides() const override;
    IProperty::ConstPtr GetOverride(BASE_NS::string_view name) const override;
    IProperty::Ptr SetPropertyTarget(const IProperty::Ptr& property) override;
    IProperty::ConstPtr GetProxyProperty(BASE_NS::string_view name) const override;
    void AddInternalProxy(BASE_NS::string_view propertyPropertyName, BASE_NS::string_view sourcePropertyName) override;

private:
    void ListenTargetChanges();
    void ResetTargetListener();
    void RefreshProperties();
    IProperty::Ptr AddProxyProperty(BASE_NS::string_view name, const IProperty::ConstPtr& tp);
    IProperty::Ptr AddProxyProperty(BASE_NS::string_view name);
    void RemoveProxyProperty(const BASE_NS::string& name);
    void RemoveAllProxyProperties();
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
    mutable BASE_NS::unordered_map<BASE_NS::string, BASE_NS::string> internalBindings_;
    EventHandler targetAddedListener_;
    EventHandler targetRemovedListener_;
    EventHandler metaAdded_;
    EventHandler metaRemoved_;
    bool updating_ {};
    META_NS::IMetadata* meta_ {};
};

} // namespace Internal

META_END_NAMESPACE()

#endif
