
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

#ifndef CORE_CORE_PROPERTY_SCOPED_HANDLE_H
#define CORE_CORE_PROPERTY_SCOPED_HANDLE_H

#include <base/containers/type_traits.h>
#include <base/namespace.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/property/intf_property_handle.h>

BASE_BEGIN_NAMESPACE()
template<typename CharT>
class basic_string_view;
using string_view = basic_string_view<char>;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
struct PropertyTypeDecl;
template<typename Type, bool readOnly_ = BASE_NS::is_const_v<Type>>
class ScopedHandle {
public:
    ScopedHandle() = default;

    explicit ScopedHandle(IPropertyHandle* handle) : handle_(handle)
    {
        if (handle) {
            if constexpr (readOnly_) {
                data_ = static_cast<Type*>(handle->RLock());
            } else {
                data_ = static_cast<Type*>(handle->WLock());
            }
        }
    }

    explicit ScopedHandle(const IPropertyHandle* handle)
    {
        static_assert(readOnly_, "const IPropertyHandle can be used with <const Type>.");
        if (handle) {
            if constexpr (readOnly_) {
                handle_ = handle;
                data_ = static_cast<Type*>(handle_->RLock());
            }
        }
    }

    ScopedHandle(ScopedHandle&& other)
        : handle_(BASE_NS::exchange(other.handle_, nullptr)), data_(BASE_NS::exchange(other.data_, nullptr))
    {}

    ScopedHandle& operator=(ScopedHandle&& other) noexcept
    {
        if (&other != this) {
            if (handle_) {
                if constexpr (readOnly_) {
                    handle_->RUnlock();
                } else {
                    handle_->WUnlock();
                }
            }
            handle_ = BASE_NS::exchange(other.handle_, nullptr);
            data_ = BASE_NS::exchange(other.data_, nullptr);
        }
        return *this;
    }

    ~ScopedHandle()
    {
        if (handle_) {
            if constexpr (readOnly_) {
                handle_->RUnlock();
            } else {
                handle_->WUnlock();
            }
        }
    }

    Type* operator->() const
    {
        return data_;
    }

    Type& operator*() const
    {
        CORE_ASSERT(data_);
        return *data_;
    }

    explicit operator bool() const
    {
        return data_;
    }

private:
    template<typename T>
    friend ScopedHandle<T> MakeScopedHandle(
        IPropertyHandle& handle, BASE_NS::string_view propertyName, const PropertyTypeDecl& propertyType);
    BASE_NS::conditional_t<readOnly_, const IPropertyHandle*, IPropertyHandle*> handle_ { nullptr };
    Type* data_ { nullptr };
};
CORE_END_NAMESPACE()

#endif // CORE_CORE_PROPERTY_SCOPED_HANDLE_H
