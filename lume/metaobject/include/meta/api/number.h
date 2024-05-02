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

#ifndef META_API_NUMBER_H
#define META_API_NUMBER_H

#include <meta/base/interface_traits.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

template<typename T>
using EnableIfBasicType =
    BASE_NS::enable_if_t<!IsSharedOrWeakPtr_v<T> && !BASE_NS::is_convertible_v<BASE_NS::remove_const_t<T&>, IAny&>>;

REGISTER_CLASS(Number, "751c6067-4fb8-404b-a240-4fa6f32c725a", ObjectCategoryBits::NO_CATEGORY)

class Number {
public:
    Number() : number_(META_NS::GetObjectRegistry().Create<IAny>(META_NS::ClassId::Number)) {}

    template<typename T, typename = EnableIfBasicType<T>>
    Number(T t) : Number()
    {
        if constexpr (is_enum_v<T>) {
            number_->SetValue(static_cast<BASE_NS::underlying_type_t<T>>(t));
        } else {
            number_->SetValue(t);
        }
    }

    Number(IAny::ConstPtr any) : Number()
    {
        number_->CopyFrom(*any);
    }

    Number(const IAny& any) : Number()
    {
        number_->CopyFrom(any);
    }

    template<typename T, typename = EnableIfBasicType<T>>
    Number& operator=(T t)
    {
        if constexpr (is_enum_v<T>) {
            number_->SetValue(static_cast<BASE_NS::underlying_type_t<T>>(t));
        } else {
            number_->SetValue(t);
        }
        return *this;
    }

    Number& operator=(IAny::ConstPtr any)
    {
        number_->CopyFrom(*any);
        return *this;
    }

    Number& operator=(const IAny& any)
    {
        number_->CopyFrom(any);
        return *this;
    }

    template<typename T>
    T Get() const
    {
        T t;
        if constexpr (is_enum_v<T>) {
            BASE_NS::underlying_type_t<T> ut;
            number_->GetValue(ut);
            t = static_cast<T>(ut);
        } else {
            number_->GetValue(t);
        }
        return t;
    }

private:
    IAny::Ptr number_;
};

META_END_NAMESPACE()

#endif
