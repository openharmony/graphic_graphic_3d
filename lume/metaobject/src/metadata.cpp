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
#include "metadata.h"

#include <meta/api/iteration.h>
#include <meta/api/util.h>

META_BEGIN_NAMESPACE()
namespace Internal {

IMetadata::Ptr Metadata::CloneMetadata() const
{
    return BASE_NS::shared_ptr<IMetadata>(new Metadata(static_cast<const IMetadata&>(*this)));
}

Metadata::Metadata()
    : propertyContainer_(new MetadataPropertyContainer),
      properties_(interface_pointer_cast<IContainer>(propertyContainer_))
{}

// make deep copy of everything
Metadata::Metadata(const IMetadata& data)
    : propertyContainer_(new MetadataPropertyContainer),
      properties_(interface_pointer_cast<IContainer>(propertyContainer_))
{
    for (auto&& v : data.GetAllProperties()) {
        if (auto prop = DuplicatePropertyType(GetObjectRegistry(), v)) {
            prop->SetValue(v->GetValue());
            properties_->Add(prop);
        }
    }
    for (auto&& v : data.GetAllEvents()) {
        if (auto i = interface_cast<ICloneable>(v)) {
            AddEvent(interface_pointer_cast<IEvent>(i->GetClone()));
        }
    }
    for (auto&& v : data.GetAllFunctions()) {
        if (auto i = interface_cast<ICloneable>(v)) {
            AddFunction(interface_pointer_cast<IFunction>(i->GetClone()));
        }
    }
}

IContainer::Ptr Metadata::GetPropertyContainer()
{
    return properties_;
}

IContainer::ConstPtr Metadata::GetPropertyContainer() const
{
    return properties_;
}

void Metadata::AddProperty(const IProperty::Ptr& p)
{
    CORE_ASSERT_MSG(p, "Trying to add null property");
    if (p) {
        if (auto other = properties_->FindAny(IContainer::FindOptions { p->GetName(), TraversalType::NO_HIERARCHY })) {
            if (interface_pointer_cast<IProperty>(other) == p) {
                CORE_LOG_W("Property already in metadata: %s", p->GetName().c_str());
            } else {
                CORE_LOG_E("Property with same name already exists in metadata: %s", p->GetName().c_str());
            }
        } else {
            properties_->Add(p);
        }
    }
}

void Metadata::RemoveProperty(const IProperty::Ptr& p)
{
    if (properties_->Remove(p)) {
        if (auto pp = interface_pointer_cast<IPropertyInternal>(p)) {
            pp->SetOwner(nullptr);
        }
    }
}

template<typename Container, typename Type>
static void AddImpl(Container& cont, const Type& p)
{
    for (auto& pp : cont) {
        if (p == pp) {
            return;
        }
    }
    cont.push_back(p);
}

template<typename Container, typename Type>
static void RemoveImpl(Container& cont, const Type& p)
{
    for (auto it = cont.begin(); it != cont.end(); ++it) {
        if (*it == p) {
            cont.erase(it);
            return;
        }
    }
}

void Metadata::AddFunction(const IFunction::Ptr& p)
{
    AddImpl(functionMetadata_, p);
}

void Metadata::RemoveFunction(const IFunction::Ptr& p)
{
    RemoveImpl(functionMetadata_, p);
}
void Metadata::AddEvent(const IEvent::Ptr& p)
{
    AddImpl(eventMetadata_, p);
}
void Metadata::RemoveEvent(const IEvent::Ptr& p)
{
    RemoveImpl(eventMetadata_, p);
}

void Metadata::SetProperties(const BASE_NS::vector<IProperty::Ptr>& vec)
{
    properties_->RemoveAll();
    for (auto&& p : vec) {
        properties_->Add(p);
    }
}

void Metadata::Merge(const IMetadata::Ptr& data)
{
    if (auto cont = data->GetPropertyContainer()) {
        auto vec = cont->GetAll<IProperty>();
        for (auto&& v : vec) {
            AddProperty(v);
        }
        for (auto&& v : data->GetAllEvents()) {
            AddEvent(v);
        }
        for (auto&& v : data->GetAllFunctions()) {
            AddFunction(v);
        }
    }
}

template<typename NewContainer, typename Container>
static NewContainer GetAllImpl(const Container& cont)
{
    NewContainer res;
    res.reserve(cont.size());
    for (const auto& v : cont) {
        res.push_back(v);
    }
    return res;
}

BASE_NS::vector<IProperty::Ptr> Metadata::GetAllProperties()
{
    return properties_->GetAll<IProperty>();
}
BASE_NS::vector<IProperty::ConstPtr> Metadata::GetAllProperties() const
{
    return GetAllImpl<BASE_NS::vector<IProperty::ConstPtr>>(properties_->GetAll<IProperty>());
}

BASE_NS::vector<IFunction::Ptr> Metadata::GetAllFunctions()
{
    return functionMetadata_;
}

BASE_NS::vector<IFunction::ConstPtr> Metadata::GetAllFunctions() const
{
    return GetAllImpl<BASE_NS::vector<IFunction::ConstPtr>>(functionMetadata_);
}

BASE_NS::vector<IEvent::Ptr> Metadata::GetAllEvents()
{
    return eventMetadata_;
}
BASE_NS::vector<IEvent::ConstPtr> Metadata::GetAllEvents() const
{
    return GetAllImpl<BASE_NS::vector<IEvent::ConstPtr>>(eventMetadata_);
}

IProperty::Ptr Metadata::GetPropertyByName(BASE_NS::string_view name)
{
    return properties_->FindByName<IProperty>(name);
}
IProperty::ConstPtr Metadata::GetPropertyByName(BASE_NS::string_view name) const
{
    return properties_->FindByName<IProperty>(name);
}

template<typename Ret, typename Container>
static Ret GetByName(Container&& cont, BASE_NS::string_view name)
{
    for (auto&& t : cont) {
        if (t->GetName() == name) {
            return t;
        }
    }
    return nullptr;
}

IFunction::Ptr Metadata::GetFunctionByName(BASE_NS::string_view name)
{
    return GetByName<IFunction::Ptr>(functionMetadata_, name);
}
IFunction::ConstPtr Metadata::GetFunctionByName(BASE_NS::string_view name) const
{
    return GetByName<IFunction::ConstPtr>(functionMetadata_, name);
}
IEvent::Ptr Metadata::GetEventByName(BASE_NS::string_view name)
{
    return GetByName<IEvent::Ptr>(eventMetadata_, name);
}
IEvent::ConstPtr Metadata::GetEventByName(BASE_NS::string_view name) const
{
    return GetByName<IEvent::ConstPtr>(eventMetadata_, name);
}

MetadataPropertyContainer::MetadataPropertyContainer()
{
    impl_.SetImplementingIContainer(nullptr, this);
    SetRequiredInterfaces({ IProperty::UID });
}

BASE_NS::vector<IObject::Ptr> MetadataPropertyContainer::GetAll() const
{
    return impl_.GetAll();
}
IObject::Ptr MetadataPropertyContainer::GetAt(SizeType index) const
{
    return impl_.GetAt(index);
}
IContainer::SizeType MetadataPropertyContainer::GetSize() const
{
    return impl_.GetSize();
}
BASE_NS::vector<IObject::Ptr> MetadataPropertyContainer::FindAll(const FindOptions& options) const
{
    return impl_.FindAll(options);
}
IObject::Ptr MetadataPropertyContainer::FindAny(const FindOptions& options) const
{
    return impl_.FindAny(options);
}
IObject::Ptr MetadataPropertyContainer::FindByName(BASE_NS::string_view name) const
{
    return impl_.FindByName(name);
}
bool MetadataPropertyContainer::Add(const IObject::Ptr& object)
{
    return impl_.Add(object);
}
bool MetadataPropertyContainer::Insert(SizeType index, const IObject::Ptr& object)
{
    return impl_.Insert(index, object);
}
bool MetadataPropertyContainer::Remove(SizeType index)
{
    return impl_.Remove(index);
}
bool MetadataPropertyContainer::Remove(const IObject::Ptr& child)
{
    return impl_.Remove(child);
}
bool MetadataPropertyContainer::Move(SizeType fromIndex, SizeType toIndex)
{
    return impl_.Move(fromIndex, toIndex);
}
bool MetadataPropertyContainer::Move(const IObject::Ptr& child, SizeType toIndex)
{
    return impl_.Move(child, toIndex);
}
bool MetadataPropertyContainer::Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways)
{
    return impl_.Replace(child, replaceWith, addAlways);
}
void MetadataPropertyContainer::RemoveAll()
{
    return impl_.RemoveAll();
}
bool MetadataPropertyContainer::SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces)
{
    return false;
}
BASE_NS::vector<TypeId> MetadataPropertyContainer::GetRequiredInterfaces() const
{
    return impl_.GetRequiredInterfaces();
}
bool MetadataPropertyContainer::IsAncestorOf(const IObject::ConstPtr& object) const
{
    return impl_.IsAncestorOf(object);
}
BASE_NS::shared_ptr<IEvent> MetadataPropertyContainer::EventOnAdded() const
{
    return onAdded_;
}
BASE_NS::shared_ptr<IEvent> MetadataPropertyContainer::EventOnRemoved() const
{
    return onRemoved_;
}
BASE_NS::shared_ptr<IEvent> MetadataPropertyContainer::EventOnMoved() const
{
    return onMoved_;
}
BASE_NS::shared_ptr<IEvent> MetadataPropertyContainer::EventOnAdding() const
{
    return onAdding_;
}
BASE_NS::shared_ptr<IEvent> MetadataPropertyContainer::EventOnRemoving() const
{
    return onRemoving_;
}
IterationResult MetadataPropertyContainer::Iterate(const IterationParameters& params)
{
    return impl_.Iterate(params);
}
IterationResult MetadataPropertyContainer::Iterate(const IterationParameters& params) const
{
    return impl_.Iterate(params);
}

} // namespace Internal

META_END_NAMESPACE()
