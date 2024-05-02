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

#ifndef META_BASE_TIMESPAN_H
#define META_BASE_TIMESPAN_H

#include <stdint.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Microsecond precision time span type.
 * The int64_t min and max values are used to mark infinities.
 */
class TimeSpan final {
public:
    constexpr TimeSpan() : value_(0) {}

    ~TimeSpan() = default;

    constexpr TimeSpan(const TimeSpan& rhs) noexcept = default;
    constexpr TimeSpan(TimeSpan&& rhs) noexcept = default;

    constexpr TimeSpan& operator=(const TimeSpan& rhs) noexcept = default;
    constexpr TimeSpan& operator=(TimeSpan&& rhs) noexcept = default;

    constexpr int64_t ToSeconds() const noexcept
    {
        return value_ / 1000000;
    }

    constexpr float ToSecondsFloat() const noexcept
    {
        return static_cast<float>(value_) / 1000000.0f;
    }

    constexpr int64_t ToMilliseconds() const noexcept
    {
        return Round(static_cast<float>(value_) / 1000.f);
    }

    constexpr int64_t ToMicroseconds() const noexcept
    {
        return value_;
    }

    constexpr void SetSeconds(float seconds) noexcept
    {
        value_ = ToMicroseconds(seconds);
    }

    constexpr void SetMilliseconds(int64_t milliseconds) noexcept
    {
        value_ = milliseconds * 1000;
    }

    constexpr void SetMicroseconds(int64_t microseconds) noexcept
    {
        value_ = microseconds;
    }

    constexpr bool IsFinite() const
    {
        return value_ != INT64_MIN && value_ != INT64_MAX;
    }

    static constexpr TimeSpan Seconds(float seconds) noexcept
    {
        return TimeSpan(ToMicroseconds(seconds));
    }

    static constexpr TimeSpan Milliseconds(int64_t milliseconds) noexcept
    {
        return TimeSpan(milliseconds * 1000);
    }

    static constexpr TimeSpan Microseconds(int64_t microseconds) noexcept
    {
        return TimeSpan(microseconds);
    }

    static constexpr TimeSpan Max() noexcept
    {
        return TimeSpan(INT64_MAX);
    }

    static constexpr TimeSpan Zero() noexcept
    {
        return TimeSpan(0);
    }

    static constexpr TimeSpan Infinite() noexcept
    {
        return TimeSpan(INT64_MAX);
    }

    constexpr bool operator==(const TimeSpan& rhs) const noexcept
    {
        return value_ == rhs.value_;
    }

    constexpr bool operator!=(const TimeSpan& rhs) const noexcept
    {
        return value_ != rhs.value_;
    }

    constexpr bool operator<(const TimeSpan& rhs) const noexcept
    {
        return value_ < rhs.value_;
    }

    constexpr bool operator>(const TimeSpan& rhs) const noexcept
    {
        return value_ > rhs.value_;
    }

    constexpr bool operator>=(const TimeSpan& rhs) const noexcept
    {
        return value_ >= rhs.value_;
    }

    constexpr bool operator<=(const TimeSpan& rhs) const noexcept
    {
        return value_ <= rhs.value_;
    }

    constexpr TimeSpan operator-() const noexcept
    {
        TimeSpan result;

        if (*this == Infinite()) {
            result = TimeSpan(INT64_MIN);
        } else if (value_ == INT64_MIN) {
            result = TimeSpan::Infinite();
        } else {
            result = TimeSpan(-value_);
        }

        return result;
    }

    constexpr TimeSpan operator+(const TimeSpan& rhs) const noexcept
    {
        TimeSpan result;

        if (value_ == INT64_MAX || rhs.value_ == INT64_MAX) {
            result = Infinite();
        } else if (value_ == INT64_MIN || rhs.value_ == INT64_MIN) {
            result = -Infinite();
        } else {
            result = TimeSpan(value_ + rhs.value_);
        }

        return result;
    }

    constexpr TimeSpan operator*(const TimeSpan& rhs) const noexcept
    {
        TimeSpan result;

        if (value_ == INT64_MAX || rhs.value_ == INT64_MAX) {
            result = Infinite();
        } else if (value_ == INT64_MIN || rhs.value_ == INT64_MIN) {
            result = -Infinite();
        } else {
            result = TimeSpan(value_ * rhs.value_);
        }

        return result;
    }

    constexpr TimeSpan operator/(const TimeSpan& rhs) const noexcept
    {
        TimeSpan result;

        if (value_ == INT64_MAX || rhs.value_ == INT64_MAX || rhs.value_ == 0) {
            result = Infinite();
        } else if (value_ == INT64_MIN || rhs.value_ == INT64_MIN) {
            result = -Infinite();
        } else {
            result = TimeSpan(value_ / rhs.value_);
        }

        return result;
    }

    constexpr TimeSpan operator-(const TimeSpan& rhs) const noexcept
    {
        return *this + -rhs;
    }

    constexpr TimeSpan& operator+=(const TimeSpan& rhs) noexcept
    {
        *this = *this + rhs;
        return *this;
    }

    constexpr TimeSpan& operator-=(const TimeSpan& rhs) noexcept
    {
        *this = *this - rhs;
        return *this;
    }

    constexpr TimeSpan& operator*=(const TimeSpan& rhs) noexcept
    {
        *this = *this * rhs;
        return *this;
    }

    constexpr TimeSpan& operator/=(const TimeSpan& rhs) noexcept
    {
        *this = *this / rhs;
        return *this;
    }

    constexpr TimeSpan operator*(int n) const noexcept
    {
        TimeSpan result;

        if (IsFinite()) {
            result = TimeSpan(n * value_);
        } else if (n == 0) {
            result = Zero();
        } else if (n > 0) {
            result = *this;
        } else {
            result = -*this;
        }

        return result;
    }

    constexpr TimeSpan operator*(float n) const noexcept
    {
        TimeSpan result;

        if (IsFinite()) {
            result = TimeSpan(Round(n * static_cast<float>(value_)));
        } else if (n == 0) {
            result = Zero();
        } else if (n > 0) {
            result = *this;
        } else {
            result = -*this;
        }

        return result;
    }

    constexpr TimeSpan operator/(float n) const noexcept
    {
        TimeSpan result;

        if (n == 0) {
            result = Infinite();
        } else if (IsFinite()) {
            result = TimeSpan(Round(static_cast<float>(value_) / n));
        } else if (n > 0) {
            result = *this;
        } else {
            result = -*this;
        }

        return result;
    }

    friend constexpr TimeSpan operator*(int n, const TimeSpan& rhs)
    {
        return rhs * n;
    }
    friend constexpr TimeSpan operator*(float n, const TimeSpan& rhs)
    {
        return rhs * n;
    }
    friend constexpr TimeSpan operator/(float n, const TimeSpan& rhs)
    {
        return rhs / n;
    }

private:
    explicit constexpr TimeSpan(int64_t microseconds) : value_(microseconds) {}

    static constexpr int64_t Round(float in) noexcept
    {
        in = in < 0 ? in - 0.5f : in + 0.5f;
        return static_cast<int64_t>(in);
    }

    static constexpr int64_t ToMicroseconds(float seconds) noexcept
    {
        return Round(seconds * 1000000.0f);
    }

private:
    // Value stored in microseconds
    int64_t value_ = 0;
};

namespace TimeSpanLiterals {
constexpr TimeSpan operator"" _s(unsigned long long seconds)
{
    return TimeSpan::Seconds(static_cast<float>(seconds));
}

constexpr TimeSpan operator"" _ms(unsigned long long milliseconds)
{
    return TimeSpan::Milliseconds(static_cast<int64_t>(milliseconds));
}

constexpr TimeSpan operator"" _us(unsigned long long microseconds)
{
    return TimeSpan::Microseconds(static_cast<int64_t>(microseconds));
}
} // namespace TimeSpanLiterals

META_END_NAMESPACE()

META_TYPE(META_NS::TimeSpan)

#endif
