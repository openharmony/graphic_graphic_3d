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
#ifndef META_BASE_EXPECTED_H
#define META_BASE_EXPECTED_H

#include <meta/base/interface_macros.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

enum GenericError : int16_t {
    SUCCESS = 0,
    FAIL = -1,
    INVALID_ARGUMENT = -2,
    INCOMPATIBLE_TYPES = -3,
    NOT_FOUND = -4,
    RECURSIVE_CALL = -5,
};

template<typename Type, typename Error>
class Expected {
public:
    /* NOLINTNEXTLINE(google-explicit-constructor) */
    constexpr Expected(Type t) : hasValue_(true), value_(BASE_NS::move(t)) {}
    /* NOLINTNEXTLINE(google-explicit-constructor) */
    constexpr Expected(Error e) : error_(BASE_NS::move(e)) {}
    ~Expected()
    {
        if (hasValue_) {
            value_.~Type();
        }
    }
    META_DEFAULT_COPY_MOVE(Expected)

    /* NOLINTNEXTLINE(google-explicit-constructor) */
    constexpr operator bool() const
    {
        return hasValue_;
    }

    constexpr const Type& operator*() const
    {
        return GetValue();
    }
    constexpr Type& operator*()
    {
        return GetValue();
    }

    constexpr Error GetError() const
    {
        return !hasValue_ ? error_ : Error {};
    }

    constexpr const Type& GetValue() const
    {
        CORE_ASSERT(hasValue_);
        return value_;
    }
    constexpr Type& GetValue()
    {
        CORE_ASSERT(hasValue_);
        return value_;
    }

private:
    bool hasValue_ {};
    union {
        Error error_;
        Type value_;
    };
};

// we don't have is_enum so for now just specialising for GenericError
template<>
class Expected<void, GenericError> {
public:
    constexpr Expected() = default;
    /* NOLINTNEXTLINE(google-explicit-constructor) */
    constexpr Expected(GenericError e) : error_(e) {}

    /* NOLINTNEXTLINE(google-explicit-constructor) */
    constexpr operator bool() const
    {
        return error_ == 0;
    }

    constexpr GenericError GetError() const
    {
        return error_;
    }

private:
    GenericError error_ {};
};

using ReturnError = Expected<void, GenericError>;

template<typename Enum>
class ReturnValue {
public:
    /* NOLINTNEXTLINE(google-explicit-constructor) */
    constexpr ReturnValue(Enum code) : code_(code) {}

    /* NOLINTNEXTLINE(google-explicit-constructor) */
    constexpr operator bool() const
    {
        return BASE_NS::underlying_type_t<Enum>(code_) > 0;
    }

    constexpr Enum GetCode() const
    {
        return code_;
    }

    bool operator==(Enum v) const
    {
        return code_ == v;
    }

    bool operator!=(Enum v) const
    {
        return code_ != v;
    }

private:
    Enum code_ {};
};

META_END_NAMESPACE()

#endif
