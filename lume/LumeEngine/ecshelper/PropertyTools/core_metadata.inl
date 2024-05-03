/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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
DECLARE_PROPERTY_TYPE(Entity);
DECLARE_PROPERTY_TYPE(EntityReference);
DECLARE_PROPERTY_TYPE(BASE_NS::string);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<float>);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<Entity>);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<EntityReference>);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<BASE_NS::Math::Mat4X4>);

namespace PropertyType {
// Declare metadatas for some basic types.
template<class T>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const T*)
{
    return {};
}
// clang-format off
template<typename T>
inline constexpr CORE_NS::ContainerApi vectorArrayMeta{ nullptr, nullptr, nullptr, nullptr, nullptr,
    { "", 0, CORE_NS::PropertySystem::PropertyTypeDeclFromType<BASE_NS::remove_extent_t<T>, BASE_NS::is_array_t<T>>(),
        (sizeof(T) / sizeof(BASE_NS::remove_extent_t<T>)), sizeof(T), 0, "", 0,
        { MetaDataFromType((T*)0), nullptr, {} } } };
// clang-format on

template<class T>
inline constexpr const ContainerApi* ContainerApiFromType(const T*)
{
    if constexpr (BASE_NS::is_array_v<T>) {
        return &vectorArrayMeta<BASE_NS::remove_extent_t<T>>;
    }
    // non array/vector types
    return nullptr;
}

template<class T>
inline constexpr const ContainerApi* ContainerApiFromType(const BASE_NS::vector<T>*)
{
    // vector types..
    return nullptr;
}

template<class T>
inline constexpr CORE_NS::MetaData MetaDataFrom(const T* dummy)
{
    return { MetaDataFromType(dummy), ContainerApiFromType(dummy), {} };
}

// primitive types have no metadata, so just return empty.
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const bool*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const char*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const int8_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const int16_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const int32_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const int64_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const uint8_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const uint16_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const uint32_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const uint64_t*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const float*)
{
    return {};
}
template<>
inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const BASE_NS::string*)
{
    return {};
}
} // namespace PropertyType

// clang-format off
#define PROPERTYTYPE_GET(b) \
    CORE_NS::PropertySystem::PropertyTypeDeclFromType<BASE_NS::remove_extent_t<b>, BASE_NS::is_array_t<b>>()

#define BEGIN_PROPERTY(TYPE, NAME) \
            using COMPTYPE = TYPE; \
            static constexpr CORE_NS::Property NAME[] = {
#define DECL_NAMED_PROPERTY_METADATA(structName, prop_name, name, displayname, flags, metadata) \
    {                                                                                           \
        #prop_name,                                                                             \
        BASE_NS::CompileTime::FNV1aHash(#prop_name),                                            \
        PROPERTYTYPE_GET(BASE_NS::remove_reference_t<decltype(structName::name)>),              \
        BASE_NS::is_array_v<decltype(structName::name)> ?                                       \
            BASE_NS::extent_v<BASE_NS::remove_reference_t<decltype(structName::name)>> : 1U,    \
        sizeof(structName::name), offsetof(structName, name), displayname, (uint32_t)flags,     \
        metadata                                                                                \
    },
#define DECL_NAMED_PROPERTY(structName, prop_name, name, displayname, flags)                             \
    DECL_NAMED_PROPERTY_METADATA(                                                                        \
        structName, prop_name, name, displayname, flags,                                                 \
        CORE_NS::PropertyType::MetaDataFrom((BASE_NS::remove_reference_t<decltype(structName::name)>*)0) \
    )
#define DECL_BITFIELD_PROPERTY(structName, name, displayname, flags, enumeration)                     \
    DECL_NAMED_PROPERTY_METADATA(structName, name, name, displayname, flags,                          \
        CORE_NS::PropertyType::MetaDataFrom((const enumeration*)0) \
)
#define DECL_NAMED_PROPERTY2(prop_name, name, displayname, flags)                                     \
    DECL_NAMED_PROPERTY(COMPTYPE, prop_name, name, displayname, flags)
#define DECL_PROPERTY2(structName, name, displayname, flags)                                          \
    DECL_NAMED_PROPERTY(structName, name, name, displayname, flags)
#define DECL_PROPERTY3(name, displayname, flags) DECL_PROPERTY2(COMPTYPE, name, displayname, flags)
#define END_PROPERTY() }

#define BEGIN_METADATA(structName, dataType) \
    namespace PropertyType { namespace MetaData { inline constexpr CORE_NS::Property MagicData_##structName[] = {
#define END_METADATA(structName, dataType) }; };  template<>                                             \
    inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const dataType*)      \
    {                                                                                                    \
        return { MetaData::MagicData_##structName, BASE_NS::countof(MetaData::MagicData_##structName) }; \
    }                                                                                                    \
    template<>                                                                                           \
    inline constexpr const ContainerApi* ContainerApiFromType(const dataType*)                           \
    {                                                                                                    \
        if constexpr (BASE_NS::is_array_v<dataType>) {return &vectorArrayMeta<dataType>;}                \
        return nullptr;                                                                                  \
    }}

#define DECL_ENUM_VALUE(structName, flags)                                                                           \
    {                                                                                                                \
        "value", BASE_NS::CompileTime::FNV1aHash("value"), PROPERTYTYPE_GET(BASE_NS::underlying_type_t<structName>), \
            sizeof(structName), sizeof(structName), 0, "", (uint32_t)flags, {}                                       \
    }

#define BEGIN_ENUM(structName, dataType)                                                   \
    namespace PropertyType { namespace MetaData {                                          \
    inline constexpr Property MagicData_##structName[] = { DECL_ENUM_VALUE(dataType, 0) }; \
    inline constexpr EnumMetaData MagicEnum_##structName[] = {
#define DECL_ENUM(dataType, name, displayName) { #name, displayName, (int64_t)dataType::name },
#define END_ENUM(structName, dataType) }; }; template<>                                                   \
inline constexpr CORE_NS::MetaData MetaDataFrom(const dataType* /* dummy */)                              \
{                                                                                                         \
    return { { MetaData::MagicData_##structName, BASE_NS::countof(MetaData::MagicData_##structName) },    \
               nullptr,                                                                                   \
               { MetaData::MagicEnum_##structName, BASE_NS::countof(MetaData::MagicEnum_##structName) }}; \
}}

// clang-format on

// Declare metadata for these basic types..
BEGIN_METADATA(UVec2MetaData, BASE_NS::Math::UVec2)
DECL_PROPERTY2(BASE_NS::Math::UVec2, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::UVec2, y, "", 0)
END_METADATA(UVec2MetaData, BASE_NS::Math::UVec2)

BEGIN_METADATA(UVec3MetaData, BASE_NS::Math::UVec3)
DECL_PROPERTY2(BASE_NS::Math::UVec3, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::UVec3, y, "", 0)
DECL_PROPERTY2(BASE_NS::Math::UVec3, z, "", 0)
END_METADATA(UVec3MetaData, BASE_NS::Math::UVec3)

BEGIN_METADATA(UVec4MetaData, BASE_NS::Math::UVec4)
DECL_PROPERTY2(BASE_NS::Math::UVec4, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::UVec4, y, "", 0)
DECL_PROPERTY2(BASE_NS::Math::UVec4, z, "", 0)
DECL_PROPERTY2(BASE_NS::Math::UVec4, w, "", 0)
END_METADATA(UVec4MetaData, BASE_NS::Math::UVec4)

BEGIN_METADATA(IVec2MetaData, BASE_NS::Math::IVec2)
DECL_PROPERTY2(BASE_NS::Math::IVec2, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::IVec2, y, "", 0)
END_METADATA(IVec2MetaData, BASE_NS::Math::IVec2)

BEGIN_METADATA(IVec3MetaData, BASE_NS::Math::IVec3)
DECL_PROPERTY2(BASE_NS::Math::IVec3, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::IVec3, y, "", 0)
DECL_PROPERTY2(BASE_NS::Math::IVec3, z, "", 0)
END_METADATA(IVec3MetaData, BASE_NS::Math::IVec3)

BEGIN_METADATA(IVec4MetaData, BASE_NS::Math::IVec4)
DECL_PROPERTY2(BASE_NS::Math::IVec4, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::IVec4, y, "", 0)
DECL_PROPERTY2(BASE_NS::Math::IVec4, z, "", 0)
DECL_PROPERTY2(BASE_NS::Math::IVec4, w, "", 0)
END_METADATA(IVec4MetaData, BASE_NS::Math::IVec4)

BEGIN_METADATA(Vec2MetaData, BASE_NS::Math::Vec2)
DECL_PROPERTY2(BASE_NS::Math::Vec2, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Vec2, y, "", 0)
END_METADATA(Vec2MetaData, BASE_NS::Math::Vec2)

BEGIN_METADATA(Vec3MetaData, BASE_NS::Math::Vec3)
DECL_PROPERTY2(BASE_NS::Math::Vec3, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Vec3, y, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Vec3, z, "", 0)
END_METADATA(Vec3MetaData, BASE_NS::Math::Vec3)

BEGIN_METADATA(Vec4MetaData, BASE_NS::Math::Vec4)
DECL_PROPERTY2(BASE_NS::Math::Vec4, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Vec4, y, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Vec4, z, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Vec4, w, "", 0)
END_METADATA(Vec4MetaData, BASE_NS::Math::Vec4)

BEGIN_METADATA(QuatMetaData, BASE_NS::Math::Quat)
DECL_PROPERTY2(BASE_NS::Math::Quat, x, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Quat, y, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Quat, z, "", 0)
DECL_PROPERTY2(BASE_NS::Math::Quat, w, "", 0)
END_METADATA(QuatMetaData, BASE_NS::Math::Quat)

BEGIN_METADATA(Mat3x3MetaData, BASE_NS::Math::Mat3X3)
DECL_PROPERTY2(BASE_NS::Math::Mat3X3, data, "", 0)
END_METADATA(Mat3x3MetaData, BASE_NS::Math::Mat3X3)

BEGIN_METADATA(Mat4x4MetaData, BASE_NS::Math::Mat4X4)
DECL_PROPERTY2(BASE_NS::Math::Mat4X4, data, "", 0)
END_METADATA(Mat4x4MetaData, BASE_NS::Math::Mat4X4)

BEGIN_METADATA(EntityMetaData, Entity)
DECL_PROPERTY2(Entity, id, "", 0)
END_METADATA(EntityMetaData, Entity)

BEGIN_METADATA(UIDMetaData, BASE_NS::Uid)
DECL_PROPERTY2(BASE_NS::Uid, data, "", 0)
END_METADATA(UIDMetaData, BASE_NS::Uid)

// helpers for "vector" containers.
namespace PropertyType {
template<typename T>
size_t size(uintptr_t container)
{
    auto& vec = *(BASE_NS::vector<T>*)(container);
    return vec.size();
}
template<typename T>
void resize(uintptr_t container, size_t newsize)
{
    auto& vec = *(BASE_NS::vector<T>*)(container);
    vec.resize(newsize);
}

template<typename T>
void erase(uintptr_t container, size_t index)
{
    auto& vec = *(BASE_NS::vector<T>*)(container);
    vec.erase(vec.begin() + static_cast<typename BASE_NS::vector<T>::difference_type>(index));
}
template<typename T>
uintptr_t insert(uintptr_t container, size_t index)
{
    auto& vec = *(BASE_NS::vector<T>*)(container);
    vec.insert(vec.begin() + static_cast<typename BASE_NS::vector<T>::difference_type>(index), {});
    return (uintptr_t)&vec[index];
}

template<typename T>
uintptr_t get(uintptr_t container, size_t index)
{
    auto& vec = *(BASE_NS::vector<T>*)(container);
    return (uintptr_t)&vec[index];
}

template<class T>
inline constexpr CORE_NS::MetaData MetaDataFromT { MetaDataFromType((T*)nullptr), ContainerApiFromType((T*)nullptr),
    {} };

template<typename T>
inline constexpr CORE_NS::Property containerElementProperty { "", 0,
    CORE_NS::PropertySystem::PropertyTypeDeclFromType<BASE_NS::remove_extent_t<T>, BASE_NS::is_array_t<T>>(),
    (sizeof(T) / sizeof(BASE_NS::remove_extent_t<T>)), sizeof(T), 0, "", 0,
    MetaDataFromT<BASE_NS::remove_extent_t<T>> };

template<typename T>
inline constexpr CORE_NS::ContainerApi data { size<T>, resize<T>, erase<T>, insert<T>, get<T>,
    containerElementProperty<T> };
} // namespace PropertyType

#define DECLARE_CONTAINER_API(blob, type)                                                                        \
    namespace PropertyType {                                                                                     \
    template<>                                                                                                   \
    inline constexpr const ContainerApi* ContainerApiFromType(const BASE_NS::vector<type>*)                      \
    {                                                                                                            \
        return &data<type>;                                                                                      \
    }                                                                                                            \
    template<>                                                                                                   \
    inline constexpr BASE_NS::array_view<const CORE_NS::Property> MetaDataFromType(const BASE_NS::vector<type>*) \
    {                                                                                                            \
        return {};                                                                                               \
    }                                                                                                            \
    }

// Declare vector style container for these primitive types.
DECLARE_CONTAINER_API(uint8_t, uint8_t);
DECLARE_CONTAINER_API(uint16_t, uint16_t);
DECLARE_CONTAINER_API(uint32_t, uint32_t);
DECLARE_CONTAINER_API(MathVec4, BASE_NS::Math::Vec4);
DECLARE_CONTAINER_API(MathMath4X4, BASE_NS::Math::Mat4X4);
DECLARE_CONTAINER_API(float, float);
DECLARE_CONTAINER_API(string, BASE_NS::string);
DECLARE_CONTAINER_API(Entity, Entity);
DECLARE_CONTAINER_API(EntityReference, EntityReference);

CORE_END_NAMESPACE()
