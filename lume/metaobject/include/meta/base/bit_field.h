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

#ifndef META_BASE_BIT_FIELD_H
#define META_BASE_BIT_FIELD_H

#include <stdint.h>

#include <base/containers/type_traits.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The BitField class encapsulates a bit field value.
 */
class BitField {
public:
    using BaseValueType = uint64_t;

    constexpr BitField() noexcept = default;
    constexpr BitField(const BaseValueType& value) noexcept : value_(value) {}
    constexpr bool operator==(const BitField& other) const noexcept
    {
        return value_ == other.value_;
    }
    constexpr bool operator!=(const BitField& other) const noexcept
    {
        return !operator==(other);
    }
    constexpr BitField& operator|=(BaseValueType value) noexcept
    {
        value_ |= value;
        return *this;
    }
    constexpr BitField operator|(BaseValueType value) const noexcept
    {
        return BitField(value_ | value);
    }
    constexpr BitField& operator&=(BaseValueType value) noexcept
    {
        value_ &= value;
        return *this;
    }
    constexpr BitField operator&(BaseValueType value) const noexcept
    {
        return BitField(value_ & value);
    }
    constexpr BitField& operator^=(BaseValueType value) noexcept
    {
        value_ ^= value;
        return *this;
    }
    constexpr BitField operator^(BaseValueType value) const noexcept
    {
        return BitField(value_ ^ value);
    }
    constexpr BitField operator~() const noexcept
    {
        return BitField(~value_);
    }
    explicit constexpr operator bool() const noexcept
    {
        return value_ != 0;
    }
    constexpr BaseValueType GetValue() const noexcept
    {
        return value_;
    }

protected:
    BaseValueType value_ {};
};

/**
 * @brief The EnumBitField class is a wrapper for a bit flag enum type, allowing
 *        the usage of binary operations with the enum flags as if they were regular integers.
 */
template<class EnumType, class ValueType = BASE_NS::underlying_type_t<EnumType>, size_t BitBeginOffset = 0,
    size_t BitCount = sizeof(ValueType) * 8>
class EnumBitField : public BitField {
    static_assert(BASE_NS::is_integral_v<ValueType>, "EnumBitField value type must be integral");
    static_assert(BitBeginOffset < sizeof(ValueType) * 8, "too big begin offset");

public:
    constexpr static size_t BEGIN_OFFSET = BitBeginOffset;

    using BaseValueType = BitField::BaseValueType;

    ~EnumBitField() = default;
    constexpr EnumBitField() noexcept = default;
    constexpr EnumBitField(EnumBitField&& other) noexcept = default;
    constexpr EnumBitField(const EnumBitField& other) noexcept = default;
    constexpr EnumBitField(const EnumType& value) noexcept
    {
        SetValueFromEnum(value);
    }
    constexpr EnumBitField(const BaseValueType& value) noexcept
    {
        SetValueFromBase(value);
    }
    constexpr EnumBitField& operator=(const EnumBitField& other) noexcept = default;
    constexpr EnumBitField& operator=(EnumBitField&& other) noexcept = default;
    constexpr EnumBitField& operator=(EnumType value) noexcept
    {
        SetValueFromEnum(value);
        return *this;
    }
    constexpr EnumBitField& operator=(ValueType value) noexcept
    {
        value_ = value;
        return *this;
    }
    constexpr bool IsSet(const EnumType& bits) const noexcept
    {
        return (*this & bits).GetValue() != 0;
    }
    constexpr void Set(const EnumType& bits)
    {
        *this |= bits;
    }
    constexpr void Clear(const EnumType& bits)
    {
        value_ &= ~static_cast<BaseValueType>(EnumBitField(bits).GetValue());
    }
    constexpr void Clear()
    {
        value_ = {};
    }
    constexpr operator EnumType() const noexcept
    {
        return static_cast<EnumType>(GetEnumValue());
    }
    constexpr bool operator==(EnumType value) const noexcept
    {
        return this->BitField::operator==(EnumBitField(value));
    }
    constexpr bool operator!=(EnumType value) const noexcept
    {
        return !operator==(value);
    }
    /** | operator */
    constexpr EnumBitField& operator|=(const EnumBitField& other) noexcept
    {
        BitField::operator|=(other.value_);
        return *this;
    }
    constexpr EnumBitField& operator|=(EnumType value) noexcept
    {
        BitField::operator|=(EnumBitField(value).GetValue());
        return *this;
    }
    constexpr EnumBitField operator|(const EnumBitField& other) const noexcept
    {
        return EnumBitField(*this).operator|=(other);
    }
    constexpr EnumBitField operator|(EnumType value) const noexcept
    {
        return EnumBitField(*this).operator|=(value);
    }
    /** & operator */
    constexpr EnumBitField& operator&=(const EnumBitField& other) noexcept
    {
        BitField::operator&=(other.value_);
        return *this;
    }
    constexpr EnumBitField& operator&=(EnumType value) noexcept
    {
        BitField::operator&=(EnumBitField(value).GetValue());
        return *this;
    }
    constexpr EnumBitField operator&(const EnumBitField& other) const noexcept
    {
        return EnumBitField(*this).operator&=(other);
    }
    constexpr EnumBitField operator&(EnumType value) const noexcept
    {
        return EnumBitField(*this).operator&=(value);
    }
    /** ^ operator */
    constexpr EnumBitField& operator^=(const EnumBitField& other) noexcept
    {
        BitField::operator^=(other.value_);
        return *this;
    }
    constexpr EnumBitField& operator^=(EnumType value) noexcept
    {
        BitField::operator^=(EnumBitField(value).GetValue());
        return *this;
    }
    constexpr EnumBitField operator^(const EnumBitField& other) const noexcept
    {
        return EnumBitField(*this).operator^=(other);
    }
    constexpr EnumBitField operator^(EnumType value) const noexcept
    {
        return EnumBitField(*this).operator^=(value);
    }
    /** ~ operator */
    constexpr EnumBitField operator~() const noexcept
    {
        return EnumBitField(static_cast<EnumType>(~GetEnumValue()));
    }

    template<class SubEnumType, size_t SubBitBeginOffset, size_t SubBitCount>
    EnumBitField& SetSubBits(EnumBitField<SubEnumType, ValueType, SubBitBeginOffset, SubBitCount> sub)
    {
        EnumBitField<SubEnumType, ValueType, SubBitBeginOffset, SubBitCount> empty;
        auto bits = ~empty;
        EnumBitField mask = bits.GetValue();
        value_ &= ~mask.value_;
        value_ |= sub.GetValue();
        return *this;
    }

private:
    constexpr static BaseValueType BITMASK = BaseValueType(-1) >> (sizeof(ValueType) * 8 - BitCount);

    constexpr void SetValueFromBase(BaseValueType t)
    {
        value_ = (static_cast<BaseValueType>(t >> BitBeginOffset) & BITMASK) << BitBeginOffset;
    }

    constexpr void SetValueFromEnum(EnumType t)
    {
        value_ = (static_cast<BaseValueType>(t) & BITMASK) << BitBeginOffset;
    }

    constexpr BaseValueType GetEnumValue() const
    {
        return (value_ >> BitBeginOffset) & BITMASK;
    }
};

/**
 * @brief Converts enum values to start from bit offset to be used as sub-EnumBitField
 */
template<class EnumType, class TopEnumBitField, size_t Offset, size_t Count>
class SubEnumBitField : public EnumBitField<EnumType, typename TopEnumBitField::BaseValueType, Offset, Count> {
    using Super = EnumBitField<EnumType, typename TopEnumBitField::BaseValueType, Offset, Count>;
    static_assert(
        Offset + Count < sizeof(typename TopEnumBitField::BaseValueType) * 8, // 8 : size
        "invalid bit offset and/or count");

public:
    using Super::Super;
    constexpr SubEnumBitField(Super s) : Super(s) {}
    constexpr SubEnumBitField(TopEnumBitField e) : Super(e.GetValue()) {}

    constexpr SubEnumBitField(const SubEnumBitField&) = default;
    constexpr SubEnumBitField(SubEnumBitField&&) = default;
    constexpr SubEnumBitField& operator=(const SubEnumBitField&) = default;
    constexpr SubEnumBitField& operator=(SubEnumBitField&&) = default;
    ~SubEnumBitField() = default;

    constexpr operator TopEnumBitField() const
    {
        return TopEnumBitField(this->GetValue());
    }
};

META_END_NAMESPACE()

#endif // META_BASE_BIT_FIELD_H
