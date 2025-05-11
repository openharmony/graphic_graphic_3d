#ifndef META_INTERFACE_DETAIL_ENUM_H
#define META_INTERFACE_DETAIL_ENUM_H

#include <meta/interface/detail/multi_type_any.h>
#include <meta/interface/enum_macros.h>

META_BEGIN_NAMESPACE()
namespace Internal {

template<typename Enum, size_t... Index>
BASE_NS::array_view<const META_NS::EnumValue> GetValues(const EnumValue<Enum>* const values, IndexSequence<Index...>)
{
    static const META_NS::EnumValue arr[] { values[Index].info... };
    return BASE_NS::array_view<const META_NS::EnumValue> { arr };
}

} // namespace Internal

template<typename T>
using EnumCompatType = BASE_NS::conditional_t<BASE_NS::is_unsigned_v<BASE_NS::underlying_type_t<T>>, uint64_t, int64_t>;

template<typename Type>
class EnumBase : public MultiTypeAny<Type, StaticCastConv, EnumCompatType<Type>> {
public:
    explicit EnumBase(Type v = {})
    {
        InternalSetValue(v);
    }

    using IAny::Clone;
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;

private:
    AnyReturnValue InternalSetValue(const Type& value) override
    {
        if (value != value_) {
            value_ = value;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }
    const Type& InternalGetValue() const override
    {
        return value_;
    }

    Type value_ {};
};

template<typename Type>
class ArrayEnumBase : public ArrayMultiTypeAnyBase<Type> {
    using Super = ArrayMultiTypeAnyBase<Type>;

public:
    using Super::Super;

    const BASE_NS::array_view<const TypeId> GetItemCompatibleTypes(CompatibilityDirection dir) const override
    {
        return EnumBase<Type>::StaticGetCompatibleTypes(dir);
    }

    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new EnumBase<Type>);
        }
        return IAny::Ptr(new ArrayEnumBase {
            options.value == CloneValueType::COPY_VALUE ? this->value_ : typename Super::ArrayType {} });
    }
};

template<typename Type>
IAny::Ptr EnumBase<Type>::Clone(const AnyCloneOptions& options) const
{
    if (options.role == TypeIdRole::ARRAY) {
        return IAny::Ptr(new ArrayEnumBase<Type>());
    }
    return IAny::Ptr(new EnumBase { options.value == CloneValueType::COPY_VALUE ? this->value_ : Type {} });
}

template<typename EnumType>
class Enum : public IntroduceInterfaces<EnumBase<typename EnumType::Type>, IEnum> {
public:
    using Super = IntroduceInterfaces<EnumBase<typename EnumType::Type>, IEnum>;
    using Type = typename EnumType::Type;
    using RealType = BASE_NS::underlying_type_t<Type>;

    explicit Enum(Type v = EnumType::DEFAULT_VALUE)
    {
        InternalSetValue(v);
    }

    BASE_NS::string_view GetName() const override
    {
        return EnumType::NAME;
    }
    BASE_NS::string_view GetDescription() const override
    {
        return EnumType::DESC;
    }
    META_NS::EnumType GetEnumType() const override
    {
        return EnumType::ENUM_TYPE;
    }
    BASE_NS::array_view<const EnumValue> GetValues() const override
    {
        return Internal::GetValues(EnumType::VALUES, MakeIndexSequence<EnumType::VALUES_SIZE>());
    }
    size_t GetValueIndex(const Type& v) const
    {
        for (size_t i = 0; i != EnumType::VALUES_SIZE; ++i) {
            if (v == EnumType::VALUES[i].value) {
                return i;
            }
        }
        return -1;
    }
    size_t GetValueIndex() const override
    {
        return GetValueIndex(value_);
    }
    bool SetValueIndex(size_t index) override
    {
        if (index < EnumType::VALUES_SIZE) {
            value_ = EnumType::VALUES[index].value;
            return true;
        }
        return false;
    }
    bool IsValueSet(size_t index) const override
    {
        if (index < EnumType::VALUES_SIZE) {
            return RealType(value_) & RealType(EnumType::VALUES[index].value);
        }
        return false;
    }
    bool FlipValue(size_t index, bool isSet) override
    {
        if (EnumType::ENUM_TYPE == META_NS::EnumType::BIT_FIELD && index < EnumType::VALUES_SIZE) {
            if (isSet) {
                value_ = Type(RealType(value_) | RealType(EnumType::VALUES[index].value));
            } else {
                value_ = Type(RealType(value_) & ~RealType(EnumType::VALUES[index].value));
            }
            return true;
        }
        return false;
    }

    AnyReturnValue ResetValue() override
    {
        return InternalSetValue(EnumType::DEFAULT_VALUE);
    }

    using IAny::Clone;
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;

private:
    AnyReturnValue InternalSetValue(const Type& value) override
    {
        if (EnumType::ENUM_TYPE == META_NS::EnumType::SINGLE_VALUE && GetValueIndex(value) == -1) {
            return AnyReturn::INVALID_ARGUMENT;
        }
        if (value != value_) {
            value_ = value;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }
    const Type& InternalGetValue() const override
    {
        return value_;
    }

    Type value_ { EnumType::DEFAULT_VALUE };
};

template<typename EnumType>
class ArrayEnum : public ArrayEnumBase<typename EnumType::Type> {
    using Type = typename EnumType::Type;
    using Super = ArrayEnumBase<Type>;

public:
    using Super::Super;

    const BASE_NS::array_view<const TypeId> GetItemCompatibleTypes(CompatibilityDirection dir) const override
    {
        return Enum<EnumType>::StaticGetCompatibleTypes(dir);
    }

    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new Enum<EnumType>);
        }
        return IAny::Ptr(new ArrayEnum {
            options.value == CloneValueType::COPY_VALUE ? this->value_ : typename Super::ArrayType {} });
    }
};

template<typename EnumType>
IAny::Ptr Enum<EnumType>::Clone(const AnyCloneOptions& options) const
{
    if (options.role == TypeIdRole::ARRAY) {
        return IAny::Ptr(new ArrayEnum<EnumType>());
    }
    return IAny::Ptr(new Enum { options.value == CloneValueType::COPY_VALUE ? this->value_ : EnumType::DEFAULT_VALUE });
}

META_END_NAMESPACE()

#endif
