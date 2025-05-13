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
#ifndef META_SRC_PROPERTY_H
#define META_SRC_PROPERTY_H

#include <atomic>
#include <memory>
#include <mutex>

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/event_impl.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_lockable.h>
#include <meta/interface/property/intf_modifier.h>
#include <meta/interface/property/intf_property_internal.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()
namespace Internal {

META_REGISTER_CLASS(Property, "1801d9a9-3557-4e0d-b90e-54791acd6768", ObjectCategoryBits::NO_CATEGORY)

class PropertyBase
    : public IntroduceInterfaces<MinimalObject, IProperty, IPropertyInternal, ILockable, IValue, IAttachable> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::Property)
public:
    using OnChangedEvent = EventImpl<IOnChanged>;

    PropertyBase(BASE_NS::string name) : name_(BASE_NS::move(name)) {}

    BASE_NS::string GetName() const override;

    IOwner::WeakPtr GetOwner() const override;
    void SetOwner(IOwner::Ptr owner) override;
    void SetSelf(IProperty::Ptr self) override;

    AnyReturnValue SetValue(const IAny& value) override;
    const IAny& GetValue() const override;
    bool IsCompatible(const TypeId& id) const override;

    TypeId GetTypeId() const override;
    void NotifyChange() const override;

    IEvent::Ptr EventOnChanged(MetadataQuery) const override;

    void Lock() const override;
    void Unlock() const override;
    void LockShared() const override;
    void UnlockShared() const override;

    bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override;
    bool Detaching(const IAttach::Ptr& target) override;

protected:
    virtual IAny::Ptr& GetData() const = 0;
    AnyReturnValue SetInternalValue(const IAny& value);
    void CallOnChanged(bool forcePending) const;
    void InvokeOnChanged(const BASE_NS::shared_ptr<OnChangedEvent>& onChanged, const IOwner::WeakPtr& owner) const;

    bool HasPendingInvoke() const
    {
        return states_ & 0x01;
    }
    void SetPendingInvoke(bool v) const
    {
        states_ = (states_ & ~0x01) | uint32_t(v);
    }

    bool HasDefaultValueFlag() const
    {
        return states_ & 0x02;
    }
    void SetDefaultValueFlag(bool v)
    {
        states_ = (states_ & ~0x02) | (uint32_t(v) << 1);
    }

protected:
    // this has to be recursive because of the usage pattern
    mutable std::recursive_mutex mutex_;
    mutable uint32_t locked_ {};
    mutable uint32_t states_ { 0b10 };
    const BASE_NS::string name_;
    IOwner::WeakPtr owner_;
    IProperty::WeakPtr self_;
    // lazy event, constructed when needed
    mutable BASE_NS::shared_ptr<OnChangedEvent> onChanged_;
    mutable std::atomic<OnChangedEvent*> onChangedAtomic_ {};
};

class GenericProperty : public IntroduceInterfaces<PropertyBase, IPropertyInternalAny> {
    using Super = IntroduceInterfaces<PropertyBase, IPropertyInternalAny>;

public:
    using Super::Super;

    AnyReturnValue SetInternalAny(IAny::Ptr any) override;
    IAny::Ptr GetInternalAny() const override;

protected:
    IAny::Ptr& GetData() const override
    {
        return data_;
    }

private:
    mutable IAny::Ptr data_;
};

} // namespace Internal
META_END_NAMESPACE()

#endif