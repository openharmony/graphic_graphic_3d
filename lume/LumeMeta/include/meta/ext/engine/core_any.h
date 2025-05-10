#ifndef META_EXT_ENGINE_CORE_ANY_H
#define META_EXT_ENGINE_CORE_ANY_H

#include <meta/interface/detail/multi_type_any.h>
#include <meta/interface/engine/intf_engine_type.h>
#include <meta/interface/intf_info.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Any type for Core property
 *
 * Supports compatibility with the primary type and additional types via conversion traits.
 * Implements the IInfo and IEngineType interfaces to get information about the Core type.
 */
template<typename Type, typename Conv, typename... CoreType>
class CoreAny : public IntroduceInterfaces<MultiTypeAny<Type, Conv, CoreType...>, IInfo, IEngineType> {
public:
    using Super = IntroduceInterfaces<MultiTypeAny<Type, Conv, CoreType...>, IInfo>;

    explicit CoreAny(const CORE_NS::Property& p, const Type& v = {}) : value_(v), prop_(p) {}

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

    using IAny::Clone;
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;

private:
    AnyReturnValue InternalSetValue(const Type& value) override
    {
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
};

template<typename Type, typename Conv, typename... CoreType>
class ArrayCoreAny : public ArrayMultiTypeAnyBase<Type> {
    using Super = ArrayMultiTypeAnyBase<Type>;
    using typename Super::ArrayType;

public:
    explicit ArrayCoreAny(const CORE_NS::Property& p, ArrayType v = {}) : Super(BASE_NS::move(v)), prop_(p) {}

    const BASE_NS::array_view<const TypeId> GetItemCompatibleTypes(CompatibilityDirection dir) const override
    {
        return CoreAny<Type, Conv, CoreType...>::StaticGetCompatibleTypes(dir);
    }

    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new CoreAny<Type, Conv, CoreType...>(prop_));
        }
        return IAny::Ptr(new ArrayCoreAny {
            prop_, options.value == CloneValueType::COPY_VALUE ? this->value_ : typename Super::ArrayType {} });
    }

private:
    const CORE_NS::Property prop_;
};

template<typename Type, typename Conv, typename... CoreType>
IAny::Ptr CoreAny<Type, Conv, CoreType...>::Clone(const AnyCloneOptions& options) const
{
    if (options.role == TypeIdRole::ARRAY) {
        return IAny::Ptr(new ArrayCoreAny<Type, Conv, CoreType...>(prop_));
    }
    return IAny::Ptr(new CoreAny { prop_, options.value == CloneValueType::COPY_VALUE ? this->value_ : Type {} });
}

META_END_NAMESPACE()

#endif