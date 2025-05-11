/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <base/util/color.h>
#include <base/util/compile_time_hashes.h>
#include <base/util/uid.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/property/property_types.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(bool);
DECLARE_PROPERTY_TYPE(char);
DECLARE_PROPERTY_TYPE(int8_t);
DECLARE_PROPERTY_TYPE(int16_t);
DECLARE_PROPERTY_TYPE(int32_t);
DECLARE_PROPERTY_TYPE(int64_t);
DECLARE_PROPERTY_TYPE(uint8_t);
DECLARE_PROPERTY_TYPE(uint16_t);
DECLARE_PROPERTY_TYPE(uint32_t);
DECLARE_PROPERTY_TYPE(uint64_t);
#ifdef __APPLE__
DECLARE_PROPERTY_TYPE(size_t);
#endif
DECLARE_PROPERTY_TYPE(float);
DECLARE_PROPERTY_TYPE(double);

// The "non-primitive" property declarations should be moved to their respective headers/implementations.
// (to remove un-necessary "leakage")

DECLARE_PROPERTY_TYPE(BASE_NS::Math::Vec2);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::Vec3);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::Vec4);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::UVec2);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::UVec3);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::UVec4);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::IVec2);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::IVec3);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::IVec4);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::Quat);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::Mat3X3);
DECLARE_PROPERTY_TYPE(BASE_NS::Math::Mat4X4);
DECLARE_PROPERTY_TYPE(BASE_NS::Uid);
DECLARE_PROPERTY_TYPE(BASE_NS::Color);
DECLARE_PROPERTY_TYPE(Entity);
DECLARE_PROPERTY_TYPE(EntityReference);
DECLARE_PROPERTY_TYPE(BASE_NS::string);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<float>);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<Entity>);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<EntityReference>);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<BASE_NS::Math::Mat4X4>);

namespace PropertyType {
// Helper for filling a Property struct for the given type.
template<typename T>
constexpr CORE_NS::Property MakeProperty(const BASE_NS::string_view propName, uintptr_t offset,
    BASE_NS::string_view displayName, uint32_t flags, const CORE_NS::MetaData& metaData)
{
    return CORE_NS::Property { propName, BASE_NS::CompileTime::FNV1aHash(propName.data()),
        CORE_NS::PropertySystem::PropertyTypeDeclFromType<BASE_NS::remove_reference_t<T>>(),
        BASE_NS::is_array_v<T> ? BASE_NS::extent_v<BASE_NS::remove_reference_t<T>> : 1U, sizeof(T), offset, displayName,
        flags, metaData };
}

// Struct for getting type specific list of Properties and ContainerApi.
template<typename T>
struct DataType {
    using BaseValueType = BASE_NS::remove_extent_t<T>;

    static constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType()
    {
        return {};
    }

    static constexpr const ContainerApi* ContainerApiFromType()
    {
        if constexpr (BASE_NS::is_array_v<T>) {
            return &arrayContainer;
        }
        // non array/vector types
        return nullptr;
    }

    static constexpr CORE_NS::Property containerElementProperty { MakeProperty<BaseValueType>("", 0U, "", 0U,
        { DataType<BaseValueType>::MetaDataFromType(), DataType<BaseValueType>::ContainerApiFromType(), {} }) };

    static constexpr CORE_NS::ContainerApi arrayContainer { nullptr, nullptr, nullptr, nullptr, nullptr,
        containerElementProperty };
};

// Struct for getting enumeration type specific MetaData.
template<typename T>
struct EnumType {
    static constexpr CORE_NS::MetaData MetaDataFrom()
    {
        return { {}, nullptr, {} };
    }
};

// Primitive types have no metadata, so just return empty.
#define NO_MEMBERS(typeName)                                                             \
    template<>                                                                           \
    struct DataType<typeName> {                                                          \
        static constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType() \
        {                                                                                \
            return {};                                                                   \
        }                                                                                \
        static constexpr const ContainerApi* ContainerApiFromType()                      \
        {                                                                                \
            return nullptr;                                                              \
        }                                                                                \
    }; // namespace PropertyType

NO_MEMBERS(bool);
NO_MEMBERS(char);
NO_MEMBERS(int8_t);
NO_MEMBERS(int16_t);
NO_MEMBERS(int32_t);
NO_MEMBERS(int64_t);
NO_MEMBERS(uint8_t);
NO_MEMBERS(uint16_t);
NO_MEMBERS(uint32_t);
NO_MEMBERS(uint64_t);
NO_MEMBERS(float);
NO_MEMBERS(BASE_NS::string);

// Returns MetaData for the given type.
template<typename T>
inline constexpr CORE_NS::MetaData MetaDataFrom()
{
    return { DataType<T>::MetaDataFromType(), DataType<T>::ContainerApiFromType(), {} };
}

// helpers for "vector" containers.
template<typename T>
struct DataType<BASE_NS::vector<T>> {
    static size_t Size(uintptr_t container)
    {
        auto& vec = *(BASE_NS::vector<T>*)(container);
        return vec.size();
    }

    static void Resize(uintptr_t container, size_t newsize)
    {
        auto& vec = *(BASE_NS::vector<T>*)(container);
        vec.resize(newsize);
    }

    static void Erase(uintptr_t container, size_t index)
    {
        auto& vec = *(BASE_NS::vector<T>*)(container);
        vec.erase(vec.begin() + static_cast<typename BASE_NS::vector<T>::difference_type>(index));
    }

    static uintptr_t Insert(uintptr_t container, size_t index)
    {
        auto& vec = *(BASE_NS::vector<T>*)(container);
        vec.insert(vec.begin() + static_cast<typename BASE_NS::vector<T>::difference_type>(index), {});
        return (uintptr_t)&vec[index];
    }

    static uintptr_t Get(uintptr_t container, size_t index)
    {
        auto& vec = *(BASE_NS::vector<T>*)(container);
        return (uintptr_t)&vec[index];
    }
    static constexpr CORE_NS::Property containerElementProperty { MakeProperty<T>(
        "", 0U, "", 0U, { DataType<T>::MetaDataFromType(), DataType<T>::ContainerApiFromType(), {} }) };

    static constexpr CORE_NS::ContainerApi data { Size, Resize, Erase, Insert, Get, containerElementProperty };

    static constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType()
    {
        return {};
    }

    static constexpr const ContainerApi* ContainerApiFromType()
    {
        return &data;
    }
};

} // namespace PropertyType

/** Initializes an EnumMetaData struct for an enum value.
 * @param enumType A valid enumeration type.
 * @param value A valid enumerator of enumType
 * @param displayName String used as the display name e.g. tooltip.
 */
// clang-format off
#define ENUM_VALUE(value, displayName)            \
    CORE_NS::EnumMetaData                         \
    {                                             \
        #value, displayName, (int64_t)Enum::value \
    }
// clang-format on

/** Defines a MetaDataFrom helper function for an enumeration type. Takes a list of ENUM_VALUE macros which define the
 * metadata for the values of the enum.
 * @param enumType A valid enumeration type.
 */
#define ENUM_TYPE_METADATA(enumType, ...)                                                                            \
    namespace PropertyType {                                                                                         \
    template<>                                                                                                       \
    struct EnumType<enumType> {                                                                                      \
        using Enum = enumType;                                                                                       \
        static constexpr CORE_NS::Property properties[] = { CORE_NS::Property { "value",                             \
            BASE_NS::CompileTime::FNV1aHash("value"),                                                                \
            CORE_NS::PropertySystem::PropertyTypeDeclFromType<BASE_NS::underlying_type_t<Enum>>(), 1U, sizeof(Enum), \
            0U, "", 0U, {} } };                                                                                      \
        static constexpr CORE_NS::EnumMetaData values[] = { __VA_ARGS__ };                                           \
        static constexpr CORE_NS::MetaData MetaDataFrom()                                                            \
        {                                                                                                            \
            return { { properties }, nullptr, { values } };                                                          \
        }                                                                                                            \
    };                                                                                                               \
    template<>                                                                                                       \
    inline constexpr CORE_NS::MetaData MetaDataFrom<enumType>()                                                      \
    {                                                                                                                \
        return EnumType<enumType>::MetaDataFrom();                                                                   \
    }                                                                                                                \
    } // namespace PropertyType

/** Helper for creating a Property struct for a member variable. Name of the property will match the name of the member
 * variable.
 * @param memberName Member of the struct data type.
 * @param displayName String which can be shown e.g. as a tooltip.
 * @param flags Combination of CORE_NS::PropertyFlags.
 * @return Property struct filled with metadata for the member variable.
 */
#define MEMBER_PROPERTY(memberName, displayName, flags)                                                              \
    CORE_NS::PropertyType::MakeProperty<decltype(COMPTYPE::memberName)>(#memberName, offsetof(COMPTYPE, memberName), \
        displayName, static_cast<uint32_t>(flags),                                                                   \
        CORE_NS::PropertyType::MetaDataFrom<BASE_NS::remove_reference_t<decltype(COMPTYPE::memberName)>>())

/** Helper for creating a Property struct for a member variable. Name of the property will be the given property name,
 * which can be different from the member variable name.
 * @param memberName Member of the struct data type.
 * @param propName Name of the property.
 * @param displayName String which can be shown e.g. as a tooltip.
 * @param flags Combination of CORE_NS::PropertyFlags.
 * @return Property struct filled with metadata for the member variable.
 */
#define RENAMED_MEMBER_PROPERTY(memberName, propName, displayName, flags)                                          \
    CORE_NS::PropertyType::MakeProperty<decltype(COMPTYPE::memberName)>(#propName, offsetof(COMPTYPE, memberName), \
        displayName, static_cast<uint32_t>(flags),                                                                 \
        CORE_NS::PropertyType::MetaDataFrom<BASE_NS::remove_reference_t<decltype(COMPTYPE::memberName)>>())

/** Helper for creating a Property struct for member variable which is a bitfield.
 * @param memberName Member of the struct data type.
 * @param displayName String which can be shown e.g. as a tooltip.
 * @param flags Combination of CORE_NS::PropertyFlags.
 * @param enumType Enumeration type defining the possible values of the bitfield.
 * @return Property struct filled with metadata for the member variable.
 */
#define BITFIELD_MEMBER_PROPERTY(memberName, displayName, flags, enumType)                                           \
    CORE_NS::PropertyType::MakeProperty<decltype(COMPTYPE::memberName)>(#memberName, offsetof(COMPTYPE, memberName), \
        displayName, static_cast<uint32_t>(flags), CORE_NS::PropertyType::MetaDataFrom<enumType>())

/** Defines MetaDataFromType and ContainerApiFromType helper function for a data type. Takes a list of *MEMBER_PROPERTY
 * macros which define the property metadata for the members of the data type.
 * @param dataType A valid data type.
 */
#define DATA_TYPE_METADATA(dataType, ...)                                                \
    namespace PropertyType {                                                             \
    template<>                                                                           \
    struct DataType<dataType> {                                                          \
        using COMPTYPE = dataType;                                                       \
        static constexpr CORE_NS::Property properties[] = { __VA_ARGS__ };               \
        static constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType() \
        {                                                                                \
            return { properties };                                                       \
        }                                                                                \
        static constexpr const ContainerApi* ContainerApiFromType()                      \
        {                                                                                \
            return nullptr;                                                              \
        }                                                                                \
        static constexpr CORE_NS::MetaData MetaDataFrom()                                \
        {                                                                                \
            return { properties, nullptr, {} };                                          \
        }                                                                                \
    };                                                                                   \
    } // namespace PropertyType

/** Starts a list on Property structs. This is expected to be followed be a varying number of MEMBER_PROPERTY,
 * RENAMED_MEMBER_PROPERTY, and BITFIELD_MEMBER_PROPERTY macros. This macro can be used inside
 * a namespace or class/struct declaration as dataType used in a type alias and would clash if declared multiple times
 * in the same scope.
 */
#define PROPERTY_LIST(dataType, storageName, ...) \
    using COMPTYPE = dataType;                    \
    static constexpr CORE_NS::Property storageName[] = { __VA_ARGS__ };

// Declare metadata for these basic types..
DATA_TYPE_METADATA(BASE_NS::Math::UVec2, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0))
DATA_TYPE_METADATA(
    BASE_NS::Math::UVec3, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0), MEMBER_PROPERTY(z, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::UVec4, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0),
    MEMBER_PROPERTY(z, "", 0), MEMBER_PROPERTY(w, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::IVec2, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0))
DATA_TYPE_METADATA(
    BASE_NS::Math::IVec3, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0), MEMBER_PROPERTY(z, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::IVec4, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0),
    MEMBER_PROPERTY(z, "", 0), MEMBER_PROPERTY(w, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::Vec2, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::Vec3, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0), MEMBER_PROPERTY(z, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::Vec4, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0), MEMBER_PROPERTY(z, "", 0),
    MEMBER_PROPERTY(w, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::Quat, MEMBER_PROPERTY(x, "", 0), MEMBER_PROPERTY(y, "", 0), MEMBER_PROPERTY(z, "", 0),
    MEMBER_PROPERTY(w, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::Mat3X3, MEMBER_PROPERTY(data, "", 0))
DATA_TYPE_METADATA(BASE_NS::Math::Mat4X4, MEMBER_PROPERTY(data, "", 0))
DATA_TYPE_METADATA(Entity, MEMBER_PROPERTY(id, "", 0))
DATA_TYPE_METADATA(BASE_NS::Uid, MEMBER_PROPERTY(data, "", 0))

CORE_END_NAMESPACE()
