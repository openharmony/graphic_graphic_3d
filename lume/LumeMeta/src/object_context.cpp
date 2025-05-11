/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Object registry implementation
 * Author: Lauri Jaaskela
 * Create: 2021-09-07
 */

#include "object_context.h"

META_BEGIN_NAMESPACE()

namespace Internal {

void ObjectContext::SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super)
{
    ObjectFwd::SetSuperInstance(aggr, super);
    proxy_ = interface_cast<IProxyObject>(super);
    assert(proxy_);
    metadata_ = interface_cast<IMetadata>(super);
    assert(metadata_);
}

IObjectRegistry& ObjectContext::GetObjectRegistry()
{
    // If we are shadowing another context, get object registry from the shadowed object
    if (auto target = GetTarget()) {
        return interface_cast<IObjectContext>(target)->GetObjectRegistry();
    }
    // By default return the global one.
    return META_NS::GetObjectRegistry();
}

const IObject::Ptr ObjectContext::GetTarget() const
{
    return proxy_ ? proxy_->GetTarget() : nullptr;
}

bool ObjectContext::SetTarget(const IObject::Ptr& target)
{
    if (target && !interface_cast<META_NS::IObjectContext>(target)) {
        CORE_LOG_E("ObjectContext shadow targets must implement IObjectContext");
        return false;
    }
    return proxy_ && proxy_->SetTarget(target);
}

BASE_NS::vector<IProperty::ConstPtr> ObjectContext::GetOverrides() const
{
    return proxy_ ? proxy_->GetOverrides() : BASE_NS::vector<IProperty::ConstPtr> {};
}

IProperty::ConstPtr ObjectContext::GetOverride(BASE_NS::string_view name) const
{
    return proxy_ ? proxy_->GetOverride(name) : nullptr;
}

IProperty::Ptr ObjectContext::SetPropertyTarget(const IProperty::Ptr& property)
{
    return proxy_ ? proxy_->SetPropertyTarget(property) : nullptr;
}

IProperty::ConstPtr ObjectContext::GetProxyProperty(BASE_NS::string_view name) const
{
    return proxy_ ? proxy_->GetProxyProperty(name) : nullptr;
}

IProperty::Ptr ObjectContext::GetProperty(BASE_NS::string_view name, MetadataQuery q)
{
    return metadata_ ? metadata_->GetProperty(name) : nullptr;
}

IProperty::ConstPtr ObjectContext::GetProperty(BASE_NS::string_view name, MetadataQuery q) const
{
    return metadata_ ? metadata_->GetProperty(name) : nullptr;
}

BASE_NS::vector<IProperty::Ptr> ObjectContext::GetProperties()
{
    return metadata_ ? metadata_->GetProperties() : BASE_NS::vector<IProperty::Ptr> {};
}

BASE_NS::vector<IProperty::ConstPtr> ObjectContext::GetProperties() const
{
    const IMetadata* p = metadata_;
    return p ? p->GetProperties() : BASE_NS::vector<IProperty::ConstPtr> {};
}

void ObjectContext::AddInternalProxy(BASE_NS::string_view propertyPropertyName, BASE_NS::string_view sourcePropertyName)
{
    if (proxy_) {
        proxy_->AddInternalProxy(propertyPropertyName, sourcePropertyName);
    }
}

} // namespace Internal

META_END_NAMESPACE()
