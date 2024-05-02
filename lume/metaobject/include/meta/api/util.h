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

#ifndef META_API_UTIL_H
#define META_API_UTIL_H

#include <meta/api/locking.h>
#include <meta/interface/animation/intf_interpolator.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_value.h>
#include <meta/interface/property/intf_modifier.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

inline BASE_NS::shared_ptr<CORE_NS::IInterface> GetPointer(const IAny& any)
{
    BASE_NS::shared_ptr<CORE_NS::IInterface> ret;
    any.GetValue(ret);
    return ret;
}

inline BASE_NS::shared_ptr<const CORE_NS::IInterface> GetConstPointer(const IAny& any)
{
    BASE_NS::shared_ptr<const CORE_NS::IInterface> ret;
    any.GetValue(ret);
    return ret;
}

template<typename Interface>
inline BASE_NS::shared_ptr<Interface> GetPointer(const IAny& any)
{
    return interface_pointer_cast<Interface>(GetPointer(any));
}

/// Returns IIinterface pointer if property contains a pointer that can be converted to it.
inline BASE_NS::shared_ptr<CORE_NS::IInterface> GetPointer(const IProperty::ConstPtr& p)
{
    InterfaceSharedLock lock { p };
    return GetPointer(p->GetValue());
}

template<typename Interface>
inline BASE_NS::shared_ptr<Interface> GetPointer(const IProperty::ConstPtr& p)
{
    return interface_pointer_cast<Interface>(GetPointer(p));
}

inline IProperty::Ptr DuplicatePropertyType(IObjectRegistry& obr, IProperty::ConstPtr p, BASE_NS::string_view name = {})
{
    IProperty::Ptr dup;
    PropertyLock lock { p };
    if (auto obj = interface_cast<IObject>(p)) {
        dup = obr.GetPropertyRegister().Create(obj->GetClassId(), name.empty() ? p->GetName() : name);
        if (auto dupi = interface_cast<IPropertyInternalAny>(dup)) {
            if (auto i = interface_cast<IPropertyInternalAny>(p)) {
                if (auto&& any = i->GetInternalAny()) {
                    dupi->SetInternalAny(any->Clone(false));
                }
            }
        }
    }
    return dup;
}

template<typename Type>
Type GetValue(const Property<Type>& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return p ? p->GetValue() : BASE_NS::move(defaultValue);
}
template<typename Type>
Type GetValue(const IProperty::ConstPtr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue(Property<Type>(p), BASE_NS::move(defaultValue));
}
template<typename Type>
Type GetValue(const IProperty::ConstWeakPtr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue<Type>(p.lock(), BASE_NS::move(defaultValue));
}
// to disambiguate
template<typename Type>
Type GetValue(const IProperty::Ptr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue(Property<Type>(p), BASE_NS::move(defaultValue));
}
// to disambiguate
template<typename Type>
Type GetValue(const IProperty::WeakPtr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue<Type>(p.lock(), BASE_NS::move(defaultValue));
}

template<typename Type>
bool SetValue(Property<Type> property, const NonDeduced_t<Type>& value) noexcept
{
    return property && property->SetValue(value);
}
template<typename Type>
bool SetValue(IProperty::Ptr p, const NonDeduced_t<Type>& value) noexcept
{
    return SetValue(Property<Type>(p), value);
}
template<typename Type>
bool SetValue(IProperty::WeakPtr p, const NonDeduced_t<Type>& value) noexcept
{
    return SetValue<Type>(p.lock(), value);
}

inline bool Copy(const IProperty::ConstPtr& src, const IProperty::Ptr& dst)
{
    PropertyLock source(src);
    PropertyLock dest(dst);
    return dest->SetValueAny(source->GetValueAny());
}

inline bool IsCompatible(
    const IProperty::ConstPtr& prop, const TypeId& id, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    bool res = false;
    if (auto i = interface_cast<IPropertyInternalAny>(prop)) {
        if (auto iany = i->GetInternalAny()) {
            res = IsCompatible(*iany, id, dir);
        }
    }
    return res;
}

template<typename T>
inline bool IsCompatibleWith(const IProperty::ConstPtr& prop, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    return IsCompatible(prop, UidFromType<BASE_NS::remove_const_t<BASE_NS::remove_reference_t<T>>>(), dir);
}

inline bool IsSetCompatible(const IProperty::ConstPtr& prop, const TypeId& id)
{
    return IsCompatible(prop, id, CompatibilityDirection::SET);
}

inline bool IsGetCompatible(const IProperty::ConstPtr& prop, const TypeId& id)
{
    return IsCompatible(prop, id, CompatibilityDirection::GET);
}

template<typename T>
inline bool IsSetCompatibleWith(const IProperty::ConstPtr& prop)
{
    return IsCompatibleWith<T>(prop, CompatibilityDirection::SET);
}

template<typename T>
inline bool IsGetCompatibleWith(const IProperty::ConstPtr& prop)
{
    return IsCompatibleWith<T>(prop, CompatibilityDirection::GET);
}

inline AnyReturnValue Interpolate(IInterpolator::ConstPtr inter, const IProperty::Ptr& output,
    const IAny::ConstPtr& from, const IAny::ConstPtr& to, float t)
{
    if (from && to) {
        if (auto i = interface_cast<IPropertyInternalAny>(output)) {
            PropertyLock lock { output };
            if (auto iany = i->GetInternalAny()) {
                auto ret = inter->Interpolate(*iany, *from, *to, t);
                lock.SetValueAny(*iany);
                return ret;
            }
        }
    }
    return AnyReturn::FAIL;
}

inline bool IsValueGetCompatible(const IAny& any, const IValue& value)
{
    for (auto&& t : any.GetCompatibleTypes(CompatibilityDirection::GET)) {
        if (value.IsCompatible(t)) {
            return true;
        }
    }
    return false;
}

inline bool IsModifierGetCompatible(const IAny& any, const IModifier& value)
{
    for (auto&& t : any.GetCompatibleTypes(CompatibilityDirection::GET)) {
        if (value.IsCompatible(t)) {
            return true;
        }
    }
    return false;
}

META_END_NAMESPACE()

#endif
