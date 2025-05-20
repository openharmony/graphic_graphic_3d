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

#include <core/log.h>

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
    NOT_SUPPORTED = -6
};

/**
 * @brief The class template Expected provides a way to represent either of two values:
 *        an expected value of type Type, or an unexpected value of type Error.
 * @Notice Expected is never valueless.
 */
template<typename Type, typename Error>
class Expected {
public:
    constexpr Expected(Type t) : hasValue_(true), value_(BASE_NS::move(t)) {}
    constexpr Expected(Error e) : error_(BASE_NS::move(e)) {}

    ~Expected()
    {
        if (hasValue_) {
            value_.~Type();
        } else {
            error_.~Error();
        }
    }

    constexpr Expected(const Expected& e) : hasValue_(e.hasValue_)
    {
        if (hasValue_) {
            new (&value_) Type(e.GetValue());
        } else {
            new (&error_) Error(e.GetError());
        }
    }

    Expected(Expected&&) = delete;
    Expected& operator=(const Expected&) = delete;
    Expected& operator=(Expected&&) = delete;

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

/// Expected specialisation for GenericError to present success or error value
template<>
class Expected<void, GenericError> {
public:
    constexpr Expected() = default;
    constexpr Expected(GenericError e) : error_(e) {}

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

/// Helper template to map enum return codes to success when positive
template<typename Enum>
class ReturnValue {
public:
    constexpr ReturnValue(Enum code) : code_(code) {}

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
