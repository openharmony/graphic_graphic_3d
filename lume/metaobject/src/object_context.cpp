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
    return proxy_->GetTarget();
}

bool ObjectContext::SetTarget(const IObject::Ptr& target)
{
    if (target && !interface_cast<META_NS::IObjectContext>(target)) {
        CORE_LOG_E("ObjectContext shadow targets must implement IObjectContext");
        return false;
    }
    return proxy_->SetTarget(target);
}

BASE_NS::vector<IProperty::ConstPtr> ObjectContext::GetOverrides() const
{
    return proxy_->GetOverrides();
}

IProperty::ConstPtr ObjectContext::GetOverride(BASE_NS::string_view name) const
{
    return proxy_->GetOverride(name);
}

IProperty::Ptr ObjectContext::SetPropertyTarget(const IProperty::Ptr& property)
{
    return proxy_->SetPropertyTarget(property);
}

IProperty::ConstPtr ObjectContext::GetProxyProperty(BASE_NS::string_view name) const
{
    return proxy_->GetProxyProperty(name);
}

IProperty::Ptr ObjectContext::GetPropertyByName(BASE_NS::string_view name)
{
    return metadata_->GetPropertyByName(name);
}

IProperty::ConstPtr ObjectContext::GetPropertyByName(BASE_NS::string_view name) const
{
    return metadata_->GetPropertyByName(name);
}

BASE_NS::vector<IProperty::Ptr> ObjectContext::GetAllProperties()
{
    return metadata_->GetAllProperties();
}

BASE_NS::vector<IProperty::ConstPtr> ObjectContext::GetAllProperties() const
{
    const IMetadata* p = metadata_;
    return p->GetAllProperties();
}

} // namespace Internal

META_END_NAMESPACE()
