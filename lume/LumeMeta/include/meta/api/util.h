/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Utilities
 * Author: Mikael Kilpeläinen
 * Create: 2023-11-16
 */

#ifndef META_API_UTIL_H
#define META_API_UTIL_H

#include <meta/api/locking.h>
#include <meta/base/ids.h>
#include <meta/interface/animation/intf_interpolator.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_value.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/intf_modifier.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

/// Get shared_ptr of IInterface from an any, or nullptr if not compatible
inline BASE_NS::shared_ptr<CORE_NS::IInterface> GetPointer(const IAny& any)
{
    BASE_NS::shared_ptr<CORE_NS::IInterface> ret;
    any.GetValue(ret);
    return ret;
}

/// Get shared_ptr of IInterface from an any ptr, or nullptr if not compatible
inline BASE_NS::shared_ptr<CORE_NS::IInterface> GetPointer(const IAny::Ptr& any)
{
    return any ? GetPointer(*any) : nullptr;
}

/// Get shared_ptr of IInterface from an any, or nullptr if not compatible
inline BASE_NS::shared_ptr<const CORE_NS::IInterface> GetConstPointer(const IAny& any)
{
    BASE_NS::shared_ptr<const CORE_NS::IInterface> ret;
    any.GetValue(ret);
    return ret;
}

/// Get shared_ptr of IInterface from an any, or nullptr if not compatible
inline BASE_NS::shared_ptr<const CORE_NS::IInterface> GetConstPointer(const IAny::ConstPtr& any)
{
    return any ? GetConstPointer(*any) : nullptr;
}

/// Get shared_ptr of IInterface from an any ptr, or nullptr if not compatible
inline BASE_NS::shared_ptr<const CORE_NS::IInterface> GetPointer(const IAny::ConstPtr& any)
{
    return any ? GetConstPointer(*any) : nullptr;
}

/// Get interface pointer from an any, or nullptr if not compatible
template<typename Interface>
inline BASE_NS::shared_ptr<Interface> GetPointer(const IAny& any)
{
    return interface_pointer_cast<Interface>(GetPointer(any));
}

/// Returns IIinterface pointer if property contains a pointer that can be converted to it.
inline BASE_NS::shared_ptr<CORE_NS::IInterface> GetPointer(const IProperty::ConstPtr& p)
{
    InterfaceSharedLock lock { p };
    return lock ? GetPointer(p->GetValue()) : nullptr;
}

/// Returns IIinterface pointer if property contains a pointer that can be converted to it.
inline BASE_NS::shared_ptr<const CORE_NS::IInterface> GetConstPointer(const IProperty::ConstPtr& p)
{
    InterfaceSharedLock lock { p };
    return lock ? GetConstPointer(p->GetValue()) : nullptr;
}

/// Returns interface pointer if property contains a pointer that can be converted to it.
template<typename Interface>
inline BASE_NS::shared_ptr<Interface> GetPointer(const IProperty::ConstPtr& p)
{
    return interface_pointer_cast<Interface>(GetPointer(p));
}

/// Returns interface pointer if property contains a pointer that can be converted to it.
template<typename Interface>
inline BASE_NS::shared_ptr<const Interface> GetConstPointer(const IProperty::ConstPtr& p)
{
    return interface_pointer_cast<Interface>(GetConstPointer(p));
}

/// Returns the internal any from property
inline IAny::ConstPtr GetInternalAny(const IProperty::ConstPtr& p)
{
    auto i = interface_cast<IPropertyInternalAny>(p);
    return i ? i->GetInternalAny() : nullptr;
}

/// Returns the internal any from property
inline IAny::ConstPtr GetInternalAny(const IProperty& p)
{
    auto i = interface_cast<IPropertyInternalAny>(&p);
    return i ? i->GetInternalAny() : nullptr;
}

/// Set IIinterface pointer for any if it is compatible
inline bool SetPointer(IAny& any, const BASE_NS::shared_ptr<CORE_NS::IInterface>& p)
{
    return any.SetValue(p);
}

/// Set IIinterface pointer for any if it is compatible
inline bool SetPointer(IAny& any, const BASE_NS::shared_ptr<const CORE_NS::IInterface>& p)
{
    return any.SetValue(p);
}

/// Set IIinterface pointer for property if it is compatible
inline bool SetPointer(const IProperty::Ptr& p, const BASE_NS::shared_ptr<CORE_NS::IInterface>& ptr)
{
    if (InterfaceSharedLock lock { p }) {
        if (auto inany = GetInternalAny(p)) {
            if (auto any = inany->Clone(false)) {
                if (any->SetValue(ptr)) {
                    return p->SetValue(*any);
                }
            }
        }
    }
    return false;
}

/// Set IIinterface pointer for property if it is compatible
inline bool SetPointer(const IProperty::Ptr& p, const BASE_NS::shared_ptr<const CORE_NS::IInterface>& ptr)
{
    if (InterfaceSharedLock lock { p }) {
        if (auto inany = GetInternalAny(p)) {
            if (auto any = inany->Clone(false)) {
                if (any->SetValue(ptr)) {
                    return p->SetValue(*any);
                }
            }
        }
    }
    return false;
}

/// Set IIinterface pointer for property if it is compatible
template<typename Interface>
inline bool SetPointer(const IProperty::Ptr& p, const BASE_NS::shared_ptr<Interface>& ptr)
{
    return SetPointer(p, interface_pointer_cast<CORE_NS::IInterface>(ptr));
}

/**
 * @brief Duplicate property type without values or modifiers
 * @param obr Object registry to use
 * @param p Property to duplicate
 * @param name Name for the new property, name of p is used if empty
 * @return New property with same type as p
 */
inline IProperty::Ptr DuplicatePropertyType(IObjectRegistry& obr, IProperty::ConstPtr p, BASE_NS::string_view name = {})
{
    IProperty::Ptr dup;
    PropertyLock lock { p };
    if (auto obj = interface_cast<IObject>(p)) {
        dup = obr.GetPropertyRegister().Create(obj->GetClassId(), name.empty() ? p->GetName() : name);
        if (auto dupi = interface_cast<IPropertyInternalAny>(dup)) {
            if (auto any = GetInternalAny(p)) {
                dupi->SetInternalAny(any->Clone(false));
            }
        }
    }
    return dup;
}

/// Get value of property or given default value if property not set or incompatible
template<typename Type>
Type GetValue(const Property<Type>& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return p ? p->GetValue() : BASE_NS::move(defaultValue);
}
/// Get value of property or given default value if property not set or incompatible
template<typename Type>
Type GetValue(const IProperty::ConstPtr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue(Property<Type>(p), BASE_NS::move(defaultValue));
}
/// Get value of property or given default value if property not set or incompatible
template<typename Type>
Type GetValue(const IProperty::ConstWeakPtr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue<Type>(p.lock(), BASE_NS::move(defaultValue));
}
// to disambiguate
/// Get value of property or given default value if property not set or incompatible
template<typename Type>
Type GetValue(const IProperty::Ptr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue(Property<Type>(p), BASE_NS::move(defaultValue));
}
// to disambiguate
/// Get value of property or given default value if property not set or incompatible
template<typename Type>
Type GetValue(const IProperty::WeakPtr& p, NonDeduced_t<BASE_NS::remove_const_t<Type>> defaultValue = {}) noexcept
{
    return GetValue<Type>(p.lock(), BASE_NS::move(defaultValue));
}
/// Set value for property, true on success
template<typename Type>
bool SetValue(Property<Type> property, const NonDeduced_t<Type>& value) noexcept
{
    return property && property->SetValue(value);
}
/// Set value for property, true on success
template<typename Type>
bool SetValue(IProperty::Ptr p, const NonDeduced_t<Type>& value) noexcept
{
    return SetValue(Property<Type>(p), value);
}
/// Set value for property, true on success
template<typename Type>
bool SetValue(IProperty::WeakPtr p, const NonDeduced_t<Type>& value) noexcept
{
    return SetValue<Type>(p.lock(), value);
}
/// Copy value from src to dst property
inline bool Copy(const IProperty::ConstPtr& src, const IProperty::Ptr& dst)
{
    PropertyLock source(src);
    PropertyLock dest(dst);
    return dest->SetValueAny(source->GetValueAny());
}
/// Check if property is compatible with given type id and direction
inline bool IsCompatible(
    const IProperty& prop, const TypeId& id, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    bool res = false;
    if (auto iany = GetInternalAny(prop)) {
        res = IsCompatible(*iany, id, dir);
    }
    return res;
}
/// Check if property is compatible with given type id and direction
inline bool IsCompatible(
    const IProperty::ConstPtr& prop, const TypeId& id, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    return prop && IsCompatible(*prop, id, dir);
}
/// Check if property is compatible with type and direction
template<typename T>
inline bool IsCompatibleWith(const IProperty::ConstPtr& prop, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    return IsCompatible(prop, GetTypeId<BASE_NS::remove_const_t<BASE_NS::remove_reference_t<T>>>(), dir);
}
/// Check if property is compatible to set value with given type id
inline bool IsSetCompatible(const IProperty::ConstPtr& prop, const TypeId& id)
{
    return IsCompatible(prop, id, CompatibilityDirection::SET);
}
/// Check if property is compatible to get value with given type id
inline bool IsGetCompatible(const IProperty::ConstPtr& prop, const TypeId& id)
{
    return IsCompatible(prop, id, CompatibilityDirection::GET);
}
/// Check if property is compatible to set value with type
template<typename T>
inline bool IsSetCompatibleWith(const IProperty::ConstPtr& prop)
{
    return IsCompatibleWith<T>(prop, CompatibilityDirection::SET);
}
/// Check if property is compatible to get value with type
template<typename T>
inline bool IsGetCompatibleWith(const IProperty::ConstPtr& prop)
{
    return IsCompatibleWith<T>(prop, CompatibilityDirection::GET);
}
/**
 * @brief Typeless interpolate between two anys using property as result
 * @param inter Interpolator to use
 * @param output PRoperty to use as result
 * @param from Start of the range
 * @param to End of the range
 * @param t The interpolation position in range [0,1]
 */
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
/// Check if any is get-compatible with the given IValue
inline bool IsValueGetCompatible(const IAny& any, const IValue& value)
{
    for (auto&& t : any.GetCompatibleTypes(CompatibilityDirection::GET)) {
        if (value.IsCompatible(t)) {
            return true;
        }
    }
    return false;
}
/// Check if any is get-compatible with the given IModifier
inline bool IsModifierGetCompatible(const IAny& any, const IModifier& value)
{
    for (auto&& t : any.GetCompatibleTypes(CompatibilityDirection::GET)) {
        if (value.IsCompatible(t)) {
            return true;
        }
    }
    return false;
}
/// Returns the top most value interface from property that implements Interface or nullptr if no such
template<typename Interface>
typename Interface::Ptr GetFirstValueFromProperty(const IProperty::ConstPtr& p)
{
    if (auto i = interface_cast<IStackProperty>(p)) {
        PropertyLock lock { p };
        TypeId ids[] = { Interface::UID };
        auto values = i->GetValues(ids, false);
        if (!values.empty()) {
            return interface_pointer_cast<Interface>(values.back());
        }
    }
    return nullptr;
}

/// Check if property contains pointer that can be extracted as constant IInterface
inline bool IsGetPointer(const IProperty::ConstPtr& p)
{
    return IsGetCompatibleWith<SharedPtrConstIInterface>(p);
}

/// Check if property contains pointer that can be set as IInterface
inline bool IsSetPointer(const IProperty::ConstPtr& p)
{
    return IsSetCompatibleWith<SharedPtrIInterface>(p);
}

META_END_NAMESPACE()

#endif
