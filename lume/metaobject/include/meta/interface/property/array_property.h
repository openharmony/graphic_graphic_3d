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

#ifndef META_INTERFACE_TYPED_ARRAY_PROPERTY_H
#define META_INTERFACE_TYPED_ARRAY_PROPERTY_H

#include <meta/base/types.h>
#include <meta/interface/detail/array_property.h>

META_BEGIN_NAMESPACE()

template<typename Type>
class ArrayProperty final {
public:
    using ValueType = BASE_NS::remove_const_t<Type>;
    using PropertyInterfaceType = BASE_NS::conditional_t<BASE_NS::is_const_v<Type>, const IProperty, IProperty>;
    using PropertyType = BASE_NS::shared_ptr<PropertyInterfaceType>;
    using IndexType = typename ArrayPropertyInterface<Type>::IndexType;

    ArrayProperty() = default;
    ArrayProperty(nullptr_t) {}

    template<typename Prop, typename = BASE_NS::enable_if_t<BASE_NS::is_convertible_v<Prop*, PropertyInterfaceType*>>>
    ArrayProperty(BASE_NS::shared_ptr<Prop> p) : p_(BASE_NS::move(p))
    {
        if (p_ && !p_->IsCompatible(ArrayUidFromType<ValueType>())) {
            CORE_LOG_W("Not compatible any for given array property type [%s]", p_->GetName().c_str());
            p_ = nullptr;
        }
    }
    template<typename Prop, typename = BASE_NS::enable_if_t<BASE_NS::is_convertible_v<Prop*, PropertyInterfaceType*>>>
    ArrayProperty(NoCheckT, BASE_NS::shared_ptr<Prop> p) : p_(p)
    {}

    bool IsValid() const
    {
        return p_ != nullptr;
    }

    explicit operator bool() const
    {
        return IsValid();
    }

    TypedArrayPropertyLock<Type> operator->() const
    {
        return GetLockedAccess();
    }

    TypedArrayPropertyLock<Type> GetLockedAccess() const
    {
        return TypedArrayPropertyLock<Type>(p_.get());
    }

    ArrayPropertyInterface<Type> GetUnlockedAccess() const
    {
        return ArrayPropertyInterface<Type>(p_.get());
    }

    operator IProperty::ConstPtr() const
    {
        return p_;
    }

    operator IProperty::Ptr()
    {
        return p_;
    }

    operator IProperty::ConstWeakPtr() const
    {
        return p_;
    }

    operator IProperty::WeakPtr()
    {
        return p_;
    }

    PropertyType GetProperty() const
    {
        return p_;
    }

private:
    PropertyType p_;
};

template<typename T>
using ConstArrayProperty = ArrayProperty<const T>;

template<typename T>
inline bool operator==(const ArrayProperty<T>& l, const ArrayProperty<T>& r)
{
    return l.GetProperty() == r.GetProperty();
}

template<typename T>
inline bool operator!=(const ArrayProperty<T>& l, const ArrayProperty<T>& r)
{
    return !(l == r);
}

META_END_NAMESPACE()

#endif
