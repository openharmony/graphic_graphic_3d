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
#ifndef META_SRC_METADATA_H
#define META_SRC_METADATA_H

#include <base/containers/array_view.h>
#include <base/containers/vector.h>

#include <meta/base/types.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_metadata.h>

#include "container/flat_container.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class Metadata : public IntroduceInterfaces<IMetadata> {
public:
    Metadata();
    Metadata(const IMetadata& data);
    ~Metadata() override = default;

    IMetadata::Ptr CloneMetadata() const override;

protected:
    IContainer::Ptr GetPropertyContainer() override;
    IContainer::ConstPtr GetPropertyContainer() const override;

    void AddFunction(const IFunction::Ptr&) override;
    void RemoveFunction(const IFunction::Ptr&) override;

    void AddProperty(const IProperty::Ptr&) override;
    void RemoveProperty(const IProperty::Ptr&) override;

    void AddEvent(const IEvent::Ptr&) override;
    void RemoveEvent(const IEvent::Ptr&) override;

    void SetProperties(const BASE_NS::vector<IProperty::Ptr>&) override;

    void Merge(const IMetadata::Ptr&) override;

    BASE_NS::vector<IProperty::Ptr> GetAllProperties() override;
    BASE_NS::vector<IProperty::ConstPtr> GetAllProperties() const override;
    BASE_NS::vector<IFunction::Ptr> GetAllFunctions() override;
    BASE_NS::vector<IFunction::ConstPtr> GetAllFunctions() const override;
    BASE_NS::vector<IEvent::Ptr> GetAllEvents() override;
    BASE_NS::vector<IEvent::ConstPtr> GetAllEvents() const override;

    IProperty::Ptr GetPropertyByName(BASE_NS::string_view name) override;
    IProperty::ConstPtr GetPropertyByName(BASE_NS::string_view name) const override;
    IFunction::Ptr GetFunctionByName(BASE_NS::string_view name) override;
    IFunction::ConstPtr GetFunctionByName(BASE_NS::string_view name) const override;
    IEvent::ConstPtr GetEventByName(BASE_NS::string_view name) const override;
    IEvent::Ptr GetEventByName(BASE_NS::string_view name) override;

private:
    BASE_NS::shared_ptr<class MetadataPropertyContainer> propertyContainer_;
    IContainer::Ptr properties_;
    BASE_NS::vector<IFunction::Ptr> functionMetadata_;
    BASE_NS::vector<IEvent::Ptr> eventMetadata_;
};

class MetadataPropertyContainer
    : public IntroduceInterfaces<IObject, IContainer, IRequiredInterfaces, IContainerPreTransaction, IIterable> {
public:
    MetadataPropertyContainer();
    ~MetadataPropertyContainer() override = default;
    META_NO_COPY_MOVE(MetadataPropertyContainer);

    BASE_NS::vector<IObject::Ptr> GetAll() const override;
    IObject::Ptr GetAt(SizeType index) const override;
    SizeType GetSize() const override;
    BASE_NS::vector<IObject::Ptr> FindAll(const FindOptions& options) const override;
    IObject::Ptr FindAny(const FindOptions& options) const override;
    IObject::Ptr FindByName(BASE_NS::string_view name) const override;
    bool Add(const IObject::Ptr& object) override;
    bool Insert(SizeType index, const IObject::Ptr& object) override;
    bool Remove(SizeType index) override;
    bool Remove(const IObject::Ptr& child) override;
    bool Move(SizeType fromIndex, SizeType toIndex) override;
    bool Move(const IObject::Ptr& child, SizeType toIndex) override;
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override;
    void RemoveAll() override;
    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) override;
    bool IsAncestorOf(const IObject::ConstPtr& object) const override;
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const override;
    BASE_NS::shared_ptr<IEvent> EventOnAdded() const override;
    BASE_NS::shared_ptr<IEvent> EventOnRemoved() const override;
    BASE_NS::shared_ptr<IEvent> EventOnMoved() const override;
    BASE_NS::shared_ptr<IEvent> EventOnAdding() const override;
    BASE_NS::shared_ptr<IEvent> EventOnRemoving() const override;
    IterationResult Iterate(const IterationParameters& params) override;
    IterationResult Iterate(const IterationParameters& params) const override;

    ObjectId GetClassId() const override
    {
        return {};
    }
    BASE_NS::string_view GetClassName() const override
    {
        return "MetadataPropertyContainer";
    }
    BASE_NS::string GetName() const override
    {
        return "";
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return GetInterfacesVector();
    }

private:
    FlatContainer impl_;
    mutable BASE_NS::shared_ptr<EventImpl<IOnChildChanged>> onAdded_ { CreateShared<EventImpl<IOnChildChanged>>(
        "OnAdded") };
    mutable BASE_NS::shared_ptr<EventImpl<IOnChildChanged>> onRemoved_ { CreateShared<EventImpl<IOnChildChanged>>(
        "OnRemoved") };
    mutable BASE_NS::shared_ptr<EventImpl<IOnChildMoved>> onMoved_ { CreateShared<EventImpl<IOnChildMoved>>(
        "OnMoved") };
    mutable BASE_NS::shared_ptr<EventImpl<IOnChildChanging>> onAdding_ { CreateShared<EventImpl<IOnChildChanging>>(
        "OnAdding") };
    mutable BASE_NS::shared_ptr<EventImpl<IOnChildChanging>> onRemoving_ { CreateShared<EventImpl<IOnChildChanging>>(
        "OnRemoving") };
};

} // namespace Internal

META_END_NAMESPACE()
#endif
