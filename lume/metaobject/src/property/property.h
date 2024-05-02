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
#include <meta/ext/minimal_object.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_lockable.h>
#include <meta/interface/property/intf_modifier.h>
#include <meta/interface/property/intf_property_internal.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()
namespace Internal {

META_REGISTER_CLASS(Property, "1801d9a9-3557-4e0d-b90e-54791acd6768", ObjectCategoryBits::NO_CATEGORY)

class PropertyBase : public MinimalObject<ClassId::Property, IProperty, IPropertyInternal, ILockable, IValue> {
public:
    using OnChangedEvent = EventImpl<IOnChanged>;

    PropertyBase(BASE_NS::string name) : name_(BASE_NS::move(name)) {}

    BASE_NS::string GetName() const override;

    IObject::WeakPtr GetOwner() const override;
    void SetOwner(IObject::Ptr owner) override;
    void SetSelf(IProperty::Ptr self) override;

    AnyReturnValue SetValue(const IAny& value) override;
    const IAny& GetValue() const override;
    bool IsCompatible(const TypeId& id) const override;

    TypeId GetTypeId() const override;
    void NotifyChange() const override;

    IEvent::Ptr EventOnChanged() const override;

    void Lock() const override;
    void Unlock() const override;
    void LockShared() const override;
    void UnlockShared() const override;

protected:
    virtual IAny::Ptr& GetData() const = 0;
    AnyReturnValue SetInternalValue(const IAny& value);
    void CallOnChanged() const;

protected:
    // this has to be recursive because of the usage pattern
    mutable std::recursive_mutex mutex_;
    mutable size_t locked_ {};
    mutable bool pendingInvoke_ {};
    const BASE_NS::string name_;
    IObject::WeakPtr owner_;
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