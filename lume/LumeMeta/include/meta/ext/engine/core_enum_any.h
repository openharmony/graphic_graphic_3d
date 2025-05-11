#ifndef META_EXT_ENGINE_CORE_ENUM_ANY_H
#define META_EXT_ENGINE_CORE_ENUM_ANY_H

#include <meta/base/type_traits.h>
#include <meta/interface/detail/multi_type_any.h>
#include <meta/interface/engine/intf_engine_type.h>
#include <meta/interface/intf_enum.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Any type for Core enum property
 *
 * Supports compatibility with the primary type and additional types via conversion traits.
 * Implements the IEnum and IEngineType interfaces to get information about the Core enum type.
 */
template<typename Type, typename Conv, typename... CoreType>
class CoreEnumAny : public IntroduceInterfaces<MultiTypeAny<Type, Conv, CoreType..., int64_t>, IEnum, IEngineType> {
public:
    using Super = IntroduceInterfaces<MultiTypeAny<Type, Conv, CoreType..., int64_t>, IEnum, IEngineType>;
    using RealType = RealType_t<Type>;

    explicit CoreEnumAny(const CORE_NS::Property& p, const Type& v = {})
        : value_(v), prop_(p),
          type_(p.flags & uint32_t(CORE_NS::PropertyFlags::IS_BITFIELD) ? META_NS::EnumType::BIT_FIELD
                                                                        : META_NS::EnumType::SINGLE_VALUE)
    {}

    CORE_NS::PropertyTypeDecl GetTypeDecl() const override
    {
        return prop_.type;
    }

    BASE_NS::string_view GetName() const override
    {
        return prop_.name;
    }
    BASE_NS::string_view GetDescription() const override
    {
        return prop_.displayName;
    }
    META_NS::EnumType GetEnumType() const override
    {
        return type_;
    }
    BASE_NS::vector<EnumValue> ConstructValues() const
    {
        BASE_NS::vector<EnumValue> values;
        for (auto&& v : prop_.metaData.enumMetaData) {
            values.push_back(EnumValue { v.name, v.displayName });
        }
        return values;
    }
    BASE_NS::array_view<const EnumValue> GetValues() const override
    {
        static BASE_NS::vector<EnumValue> values = ConstructValues();
        return BASE_NS::array_view<const EnumValue>(values);
    }
    size_t GetValueIndex(const Type& v) const
    {
        for (size_t i = 0; i != prop_.metaData.enumMetaData.size(); ++i) {
            if (DefaultCompare<Type>::Equal(v, Type(prop_.metaData.enumMetaData[i].value))) {
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
        if (index < prop_.metaData.enumMetaData.size()) {
            value_ = Type(prop_.metaData.enumMetaData[index].value);
            return true;
        }
        return false;
    }
    bool IsValueSet(size_t index) const override
    {
        if constexpr (BASE_NS::is_integral_v<RealType>) {
            if (index < prop_.metaData.enumMetaData.size()) {
                return RealType(value_) & RealType(prop_.metaData.enumMetaData[index].value);
            }
        }
        return false;
    }
    bool FlipValue(size_t index, bool isSet) override
    {
        if constexpr (BASE_NS::is_integral_v<RealType>) {
            if (type_ == META_NS::EnumType::BIT_FIELD && index < prop_.metaData.enumMetaData.size()) {
                if (isSet) {
                    value_ = Type(RealType(value_) | RealType(prop_.metaData.enumMetaData[index].value));
                } else {
                    value_ = Type(RealType(value_) & ~RealType(prop_.metaData.enumMetaData[index].value));
                }
                return true;
            }
        }
        return false;
    }

    AnyReturnValue ResetValue() override
    {
        return InternalSetValue({});
    }

    using IAny::Clone;
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;

private:
    AnyReturnValue InternalSetValue(const Type& value) override
    {
        if (type_ == META_NS::EnumType::SINGLE_VALUE && GetValueIndex(value) == -1) {
            return AnyReturn::INVALID_ARGUMENT;
        }
        if (!DefaultCompare<Type>::Equal(value, value_)) {
            value_ = value;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }
    const Type& InternalGetValue() const override
    {
        return value_;
    }

private:
    Type value_ {};
    const CORE_NS::Property prop_;
    const META_NS::EnumType type_;
};

template<typename Type, typename Conv, typename... CoreType>
class ArrayCoreEnumAny : public ArrayMultiTypeAnyBase<Type> {
    using Super = ArrayMultiTypeAnyBase<Type>;
    using typename Super::ArrayType;

public:
    explicit ArrayCoreEnumAny(const CORE_NS::Property& p, ArrayType v = {}) : Super(BASE_NS::move(v)), prop_(p) {}

    const BASE_NS::array_view<const TypeId> GetItemCompatibleTypes(CompatibilityDirection dir) const override
    {
        return CoreEnumAny<Type, Conv, CoreType...>::StaticGetCompatibleTypes(dir);
    }

    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new CoreEnumAny<Type, Conv, CoreType...>(prop_));
        }
        return IAny::Ptr(new ArrayCoreEnumAny {
            prop_, options.value == CloneValueType::COPY_VALUE ? this->value_ : typename Super::ArrayType {} });
    }

private:
    const CORE_NS::Property prop_;
};

template<typename Type, typename Conv, typename... CoreType>
IAny::Ptr CoreEnumAny<Type, Conv, CoreType...>::Clone(const AnyCloneOptions& options) const
{
    if (options.role == TypeIdRole::ARRAY) {
        return IAny::Ptr(new ArrayCoreEnumAny<Type, Conv, CoreType...>(prop_));
    }
    return IAny::Ptr(new CoreEnumAny { prop_, options.value == CloneValueType::COPY_VALUE ? this->value_ : Type {} });
}

META_END_NAMESPACE()

#endif