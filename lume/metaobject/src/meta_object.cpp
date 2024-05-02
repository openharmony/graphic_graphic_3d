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
#include "meta_object.h"

#include <base/util/uid_util.h>
#include <core/plugin/intf_class_factory.h>

#include <meta/api/iteration.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_proxy_object.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/intf_property_internal.h>

#include "ref_uri_util.h"

META_BEGIN_NAMESPACE()
namespace Internal {

// IObject
IObject::Ptr MetaObject::Resolve(const RefUri& uri) const
{
    if (uri == RefUri::ContextUri()) {
        return interface_pointer_cast<IObjectInstance>(ObjectContext()->GetValue());
    }
    return Super::Resolve(uri);
}

// ILifecycle
bool MetaObject::Build(const IMetadata::Ptr& data)
{
    bool ret = Super::Build(data);
    if (ret) {
        // update owners in case the top most object was not set when we added some of the properties
        // notice that we are building the object, so no locking is needed.
        auto me = GetSelf();
        META_NS::Iterate(
            GetPropertyContainer(),
            [&](const IObject::Ptr& p) {
                if (auto pp = interface_cast<IPropertyInternal>(p)) {
                    pp->SetOwner(me);
                }
                return true;
            },
            IterateStrategy { TraversalType::NO_HIERARCHY, LockType::NO_LOCK });
    }
    return ret;
}

void MetaObject::Destroy()
{
    if (auto c = GetMetadata()) {
        c->GetPropertyContainer()->RemoveAll();
    }
    Super::Destroy();
}

// IObjectContextProvider
IProperty::Ptr MetaObject::PropertyObjectContext()
{
    if (!objectContext_) {
        // By default use the global object context
        auto context = META_NS::GetDefaultObjectContext();
        CORE_ASSERT(context);
        objectContext_ = ConstructProperty<IObjectContext::Ptr>(context->GetObjectRegistry(), "ObjectContext", context);
        CORE_ASSERT(objectContext_);
        if (auto internal = interface_cast<IPropertyInternal>(objectContext_.GetProperty())) {
            internal->SetOwner(GetSelf());
        }
    }
    return objectContext_;
}

IProperty::ConstPtr MetaObject::PropertyObjectContext() const
{
    if (!objectContext_) {
        // By default use the global object context
        auto context = META_NS::GetDefaultObjectContext();
        CORE_ASSERT(context);
        objectContext_ = ConstructProperty<IObjectContext::Ptr>(context->GetObjectRegistry(), "ObjectContext", context);
        CORE_ASSERT(objectContext_);
        if (auto internal = interface_cast<IPropertyInternal>(objectContext_.GetProperty())) {
            internal->SetOwner(GetSelf());
        }
    }
    return objectContext_;
}

void MetaObject::ResetObjectContext()
{
    if (objectContext_) {
        objectContext_->SetValue(META_NS::GetDefaultObjectContext());
    }
}

IObjectRegistry& MetaObject::GetObjectRegistry() const
{
    if (objectContext_) {
        if (auto ctx = objectContext_->GetValue()) {
            return ctx->GetObjectRegistry();
        }
    }
    // No context set
    return Super::GetObjectRegistry();
}

IMetadata::Ptr MetaObject::CloneMetadata() const
{
    return meta_->CloneMetadata();
}

IContainer::Ptr MetaObject::GetPropertyContainer()
{
    return meta_->GetPropertyContainer();
}

IContainer::ConstPtr MetaObject::GetPropertyContainer() const
{
    return meta_->GetPropertyContainer();
}

void MetaObject::AddFunction(const IFunction::Ptr& p)
{
    meta_->AddFunction(p);
}
void MetaObject::RemoveFunction(const IFunction::Ptr& p)
{
    meta_->RemoveFunction(p);
}
void MetaObject::AddProperty(const IProperty::Ptr& p)
{
    if (auto pp = interface_pointer_cast<IPropertyInternal>(p)) {
        pp->SetOwner(GetSelf());
    }
    meta_->AddProperty(p);
}
void MetaObject::RemoveProperty(const IProperty::Ptr& p)
{
    meta_->RemoveProperty(p);
}
void MetaObject::AddEvent(const IEvent::Ptr& p)
{
    meta_->AddEvent(p);
}
void MetaObject::RemoveEvent(const IEvent::Ptr& p)
{
    meta_->RemoveEvent(p);
}
void MetaObject::SetProperties(const BASE_NS::vector<IProperty::Ptr>& vec)
{
    meta_->SetProperties(vec);
}
void MetaObject::Merge(const IMetadata::Ptr& data)
{
    meta_->Merge(data);
}
BASE_NS::vector<IProperty::Ptr> MetaObject::GetAllProperties()
{
    return meta_->GetAllProperties();
}
BASE_NS::vector<IProperty::ConstPtr> MetaObject::GetAllProperties() const
{
    return static_cast<const IMetadata*>(meta_.get())->GetAllProperties();
}
BASE_NS::vector<IFunction::Ptr> MetaObject::GetAllFunctions()
{
    return meta_->GetAllFunctions();
}
BASE_NS::vector<IFunction::ConstPtr> MetaObject::GetAllFunctions() const
{
    return static_cast<const IMetadata*>(meta_.get())->GetAllFunctions();
}
BASE_NS::vector<IEvent::Ptr> MetaObject::GetAllEvents()
{
    return meta_->GetAllEvents();
}
BASE_NS::vector<IEvent::ConstPtr> MetaObject::GetAllEvents() const
{
    return static_cast<const IMetadata*>(meta_.get())->GetAllEvents();
}
IProperty::Ptr MetaObject::GetPropertyByName(BASE_NS::string_view name)
{
    return meta_->GetPropertyByName(name);
}
IProperty::ConstPtr MetaObject::GetPropertyByName(BASE_NS::string_view name) const
{
    return meta_->GetPropertyByName(name);
}
IFunction::Ptr MetaObject::GetFunctionByName(BASE_NS::string_view name)
{
    return meta_->GetFunctionByName(name);
}
IFunction::ConstPtr MetaObject::GetFunctionByName(BASE_NS::string_view name) const
{
    return meta_->GetFunctionByName(name);
}
IEvent::ConstPtr MetaObject::GetEventByName(BASE_NS::string_view name) const
{
    return meta_->GetEventByName(name);
}
IEvent::Ptr MetaObject::GetEventByName(BASE_NS::string_view name)
{
    return meta_->GetEventByName(name);
}
IMetadata::Ptr MetaObject::GetMetadata() const
{
    return meta_;
}
void MetaObject::SetMetadata(const IMetadata::Ptr& meta)
{
    meta_ = meta;
}

const StaticObjectMetadata& MetaObject::GetStaticMetadata() const
{
    return GetStaticObjectMetadata();
}

} // namespace Internal

META_END_NAMESPACE()
