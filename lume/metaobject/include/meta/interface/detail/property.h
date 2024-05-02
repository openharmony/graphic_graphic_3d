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

#ifndef META_INTERFACE_DETAIL_PROPERTY_H
#define META_INTERFACE_DETAIL_PROPERTY_H

#include <base/containers/type_traits.h>

#include <meta/interface/detail/any.h>
#include <meta/interface/intf_lockable.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_value.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/property/intf_property_internal.h>
#include <meta/interface/property/intf_property_register.h>
#include <meta/interface/property/intf_stack_property.h>

META_BEGIN_NAMESPACE()

class ConstTypelessPropertyInterface {
public:
    using PropertyType = const IProperty*;

    explicit ConstTypelessPropertyInterface(PropertyType p) : p_(p) {}

    BASE_NS::string GetName() const
    {
        return p_->GetName();
    }
    IObject::WeakPtr GetOwner() const
    {
        return p_->GetOwner();
    }

    const IAny& GetValueAny() const
    {
        return p_->GetValue();
    }

    const IAny& GetDefaultValueAny() const
    {
        IAny::ConstPtr ret;
        if (auto i = interface_cast<IStackProperty>(p_)) {
            return i->GetDefaultValue();
        }
        return META_NS::GetObjectRegistry().GetPropertyRegister().InvalidAny();
    }

    bool IsDefaultValue() const
    {
        return p_->IsDefaultValue();
    }
    bool IsValueSet() const
    {
        return !p_->IsDefaultValue();
    }

    TypeId GetTypeId() const
    {
        return p_->GetTypeId();
    }

    bool IsCompatible(const TypeId& id) const
    {
        return p_->IsCompatible(id);
    }

    auto OnChanged() const
    {
        return p_->OnChanged();
    }

    void NotifyChange() const
    {
        p_->NotifyChange();
    }

    PropertyType GetProperty() const
    {
        return p_;
    }

    const IStackProperty* GetStackProperty() const
    {
        return interface_cast<IStackProperty>(p_);
    }

    template<typename Interface>
    BASE_NS::vector<typename Interface::Ptr> GetModifiers() const
    {
        BASE_NS::vector<typename Interface::Ptr> res;
        if (auto i = interface_cast<IStackProperty>(p_)) {
            const TypeId view[] = { Interface::UID };
            for (auto& v : i->GetModifiers(view, true)) {
                res.push_back(interface_pointer_cast<Interface>(v));
            }
        }
        return res;
    }

    IFunction::ConstPtr GetBind() const
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            const TypeId binds[] = { IBind::UID };
            auto vec = i->GetValues(binds, false);
            if (!vec.empty()) {
                if (auto bind = interface_cast<IBind>(vec.back())) {
                    return bind->GetTarget();
                }
            }
        }
        return nullptr;
    }

protected:
    PropertyType p_;
};

class TypelessPropertyInterface : public ConstTypelessPropertyInterface {
public:
    using PropertyType = IProperty*;

    TypelessPropertyInterface(PropertyType p) : ConstTypelessPropertyInterface(p), p_(p) {}

    AnyReturnValue SetValueAny(const IAny& any)
    {
        return p_->SetValue(any);
    }

    AnyReturnValue SetDefaultValueAny(const IAny& value)
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            return i->SetDefaultValue(value);
        }
        return AnyReturn::FAIL;
    }

    template<typename Intf>
    ReturnError PushValue(const BASE_NS::shared_ptr<Intf>& value)
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            if (auto v = interface_pointer_cast<IValue>(value)) {
                return i->PushValue(v);
            }
        }
        return GenericError::FAIL;
    }

    ReturnError PopValue()
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            return i->PopValue();
        }
        return GenericError::FAIL;
    }

    ReturnError AddModifier(const IModifier::Ptr& mod)
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            return i->AddModifier(mod);
        }
        return GenericError::FAIL;
    }

    bool SetBind(const IFunction::ConstPtr& func, const BASE_NS::array_view<const INotifyOnChange::ConstPtr>& deps = {})
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            auto b = CreateBind(*i);
            if (!b->SetTarget(func, deps.empty(), p_)) {
                return false;
            }
            for (auto& d : deps) {
                b->AddDependency(d);
            }
            return i->PushValue(interface_pointer_cast<IValue>(b));
        }
        return false;
    }

    bool SetBind(const IProperty::ConstPtr& prop, const BASE_NS::array_view<const INotifyOnChange::ConstPtr>& deps = {})
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            auto b = CreateBind(*i);
            if (!b->SetTarget(prop, deps.empty(), p_)) {
                return false;
            }
            for (auto& d : deps) {
                b->AddDependency(d);
            }
            return i->PushValue(interface_pointer_cast<IValue>(b));
        }
        return false;
    }

    void ResetBind()
    {
        if (auto i = interface_cast<IStackProperty>(p_)) {
            const TypeId binds[] = { IBind::UID };
            auto vec = i->GetValues(binds, false);
            if (!vec.empty()) {
                i->RemoveValue(vec.back());
                NotifyChange();
            }
        }
    }

    void ResetValue()
    {
        p_->ResetValue();
    }

    void Reset()
    {
        ResetValue();
    }

    PropertyType GetProperty()
    {
        return p_;
    }

    IStackProperty* GetStackProperty()
    {
        return interface_cast<IStackProperty>(p_);
    }

protected:
    IBind::Ptr CreateBind(IStackProperty& prop)
    {
        if (interface_cast<IBind>(prop.TopValue())) {
            prop.PopValue();
        }
        return META_NS::GetObjectRegistry().GetPropertyRegister().CreateBind();
    }

protected:
    PropertyType p_;
};

template<typename Type>
using PropertyBaseType =
    BASE_NS::conditional_t<BASE_NS::is_const_v<Type>, ConstTypelessPropertyInterface, TypelessPropertyInterface>;

template<typename Type>
class PropertyInterface : public PropertyBaseType<Type> {
    using Super = PropertyBaseType<Type>;
    using Super::p_;

public:
    using ValueType = BASE_NS::remove_const_t<Type>;
    using PropertyType = typename Super::PropertyType;

    explicit PropertyInterface(PropertyType p) : Super(p) {}

    ValueType GetDefaultValue() const
    {
        ValueType v {};
        this->GetDefaultValueAny().GetValue(v);
        return v;
    }

    AnyReturnValue SetDefaultValue(ValueType value, bool resetToDefault)
    {
        auto ret = this->SetDefaultValueAny(Any<ValueType>(value));
        if (resetToDefault && ret) {
            this->ResetValue();
        }
        return ret;
    }

    AnyReturnValue SetDefaultValue(ValueType value)
    {
        return SetDefaultValue(BASE_NS::move(value), false);
    }

    ValueType GetValue() const
    {
        ValueType v {};
        this->GetValueAny().GetValue(v);
        return v;
    }

    AnyReturnValue SetValue(ValueType value)
    {
        return this->SetValueAny(Any<ValueType>(value));
    }
};

template<typename Type>
class TypedPropertyLock final : public PropertyInterface<Type> {
    using PropertyType = typename PropertyInterface<Type>::PropertyType;
    using IT = PropertyInterface<Type>;
    using InterfaceType = BASE_NS::conditional_t<BASE_NS::is_const_v<Type>, const IT*, IT*>;

    META_NO_COPY_MOVE(TypedPropertyLock)

public:
    explicit TypedPropertyLock(PropertyType p) : PropertyInterface<Type>(p)
    {
        if (auto i = interface_cast<ILockable>(p)) {
            i->Lock();
        }
    }
    ~TypedPropertyLock()
    {
        if (auto i = interface_cast<ILockable>(this->GetProperty())) {
            i->Unlock();
        }
    }

    InterfaceType operator->() const
    {
        return const_cast<TypedPropertyLock*>(this);
    }
};

template<typename Property>
class PropertyLock final : public PropertyBaseType<Property> {
    using InterfaceType = PropertyBaseType<Property>*;

    META_NO_COPY_MOVE(PropertyLock)

public:
    explicit PropertyLock(BASE_NS::shared_ptr<Property> p) : PropertyBaseType<Property>(p.get())
    {
        if (auto i = interface_cast<ILockable>(p)) {
            i->Lock();
        }
    }
    ~PropertyLock()
    {
        if (auto i = interface_cast<ILockable>(this->GetProperty())) {
            i->Unlock();
        }
    }

    InterfaceType operator->() const
    {
        return const_cast<PropertyLock*>(this);
    }
};

META_END_NAMESPACE()

#endif
