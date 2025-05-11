/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: interface macros
 * Author: Mikael Kilpeläinen
 * Create: 2024-10-15
 */

#ifndef META_INTERFACE_ENUM_MACROS_H
#define META_INTERFACE_ENUM_MACROS_H

#include <meta/base/meta_types.h>
#include <meta/interface/intf_enum.h>

META_BEGIN_NAMESPACE()
namespace Internal {
template<typename Type>
struct EnumValue {
    Type value {};
    META_NS::EnumValue info;
};
} // namespace Internal

template<typename, typename>
struct MapAnyType;
template<typename EnumType>
class Enum;
template<typename EnumType>
class ArrayEnum;

META_END_NAMESPACE()

#define META_BEGIN_ENUM_IMPL(Enum, Name, Desc, DefaultValue, EType) \
    struct Enum##Metadata {                                         \
        using Type = Enum;                                          \
        static constexpr const Type DEFAULT_VALUE = DefaultValue;   \
        static constexpr const META_NS::EnumType ENUM_TYPE = EType; \
        static constexpr const char* const NAME = Name;             \
        static constexpr const char* const DESC = Desc;             \
        inline static constexpr const META_NS::Internal::EnumValue<Type> VALUES[] = {
#define META_BEGIN_ENUM(Enum, Desc, DefaultValue) \
    META_BEGIN_ENUM_IMPL(Enum, #Enum, Desc, DefaultValue, META_NS::EnumType::SINGLE_VALUE)

#define META_BEGIN_BITFIELD_ENUM(Enum, Desc, DefaultValue) \
    META_BEGIN_ENUM_IMPL(Enum, #Enum, Desc, DefaultValue, META_NS::EnumType::BIT_FIELD)

#define META_END_ENUM()                                                           \
    }                                                                             \
    ;                                                                             \
    static constexpr const size_t VALUES_SIZE = sizeof(VALUES) / sizeof(*VALUES); \
    }                                                                             \
    ;

#define META_ENUM_VALUE(Value, Name, Desc) { Value, { Name, Desc } },

#define META_ENUM_TYPE(MyEnum)                                                  \
    META_TYPE(MyEnum)                                                           \
    template<>                                                                  \
    struct ::META_NS::MapAnyType<MyEnum, ::META_NS::EnableSpecialisationType> { \
        using AnyType = ::META_NS::Enum<MyEnum##Metadata>;                      \
        using ArrayAnyType = ::META_NS::ArrayEnum<MyEnum##Metadata>;            \
    };

#endif