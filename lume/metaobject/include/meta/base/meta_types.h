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
#ifndef META_BASE_META_TYPES_H
#define META_BASE_META_TYPES_H

#include <stdint.h>

#include <base/namespace.h>
#include <base/util/compile_time_hashes.h>
#include <base/util/uid.h>
#include <core/property/property_types.h>

#include <meta/base/type_traits.h>

META_BEGIN_NAMESPACE()

template<typename t>
struct MetaType;

template<class T, class B>
inline constexpr auto CorePropertyTypeDeclFromType()
{
    if constexpr (CORE_NS::PropertySystem::is_defined<T>().value) {
        return CORE_NS::PropertySystem::PropertyTypeDeclFromType<T, B>();
    }
    return CORE_NS::PropertyTypeDecl {};
}

inline constexpr BASE_NS::Uid MakeUidImpl(uint64_t hash, const char (&type)[9])
{
    uint8_t data[16u] { static_cast<uint8_t>((hash >> 56) & 0xFF), static_cast<uint8_t>((hash >> 48) & 0xFF),
        static_cast<uint8_t>((hash >> 40) & 0xFF), static_cast<uint8_t>((hash >> 32) & 0xFF),
        static_cast<uint8_t>((hash >> 24) & 0xFF), static_cast<uint8_t>((hash >> 16) & 0xFF),
        static_cast<uint8_t>((hash >> 8) & 0xFF), static_cast<uint8_t>(hash & 0xFF), static_cast<uint8_t>(type[0]),
        static_cast<uint8_t>(type[1]), static_cast<uint8_t>(type[2]), static_cast<uint8_t>(type[3]),
        static_cast<uint8_t>(type[4]), static_cast<uint8_t>(type[5]), static_cast<uint8_t>(type[6]),
        static_cast<uint8_t>(type[7]) };
    return BASE_NS::Uid(data);
}

/**
 * @brief Generate UID from name and type string.
 */
inline constexpr BASE_NS::Uid MakeUid(const char* const name, const char (&type)[9])
{
    return MakeUidImpl(BASE_NS::CompileTime::FNV1aHash(name), type);
}

template<typename Type>
using EnableIfDefined = decltype(sizeof(Type));

template<typename Type>
constexpr bool IsDefined_v = META_NS::IsDetected_v<EnableIfDefined, Type>; // NOLINT(readability-identifier-naming)

template<typename Type>
constexpr bool HasUid_v = IsDefined_v<MetaType<Type>>; // NOLINT(readability-identifier-naming)

/**
 * @brief Generate UID from registered type and type string.
 */
template<typename Type>
inline constexpr BASE_NS::Uid MakeUid(const char (&type)[9])
{
    static_assert(HasUid_v<Type>, "META_TYPE missing for given type");
    return MakeUidImpl(BASE_NS::CompileTime::FNV1aHash(MetaType<Type>::name), type);
}

template<typename Type>
inline constexpr uint64_t CombineHash(uint64_t hash)
{
    static_assert(HasUid_v<Type>, "META_TYPE missing for given type");
    return hash ^
           (BASE_NS::CompileTime::FNV1aHash(MetaType<Type>::name) + 0x9e3779b97f4a7c15LLU + 
           (hash << 12) + (hash >> 4)); // 12 4 shift multiple
}

/**
 * @brief Generate UID from multiple registered types and type string.
 */
template<typename... Types>
inline constexpr BASE_NS::Uid MakeUidFromTypes(const char (&type)[9])
{
    uint64_t hash {};
    if constexpr (sizeof...(Types) != 0) {
        hash = (CombineHash<Types>(hash), ...);
    }
    return MakeUidImpl(hash, type);
}

/**
 * @brief Check if UID is valid.
 */
inline constexpr bool IsValidUid(const BASE_NS::Uid& uid)
{
    for (const auto& v : uid.data) {
        if (v != 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Generate UID for read-only property type.
 */
template<typename Type>
inline constexpr BASE_NS::Uid UidFromReadOnlyType()
{
// At least visual c++ 2017 has a bug that causes this uid variable to be uninitialised at runtime and so crashing
// We know at least visual c++ 2022 works...
#if defined(_MSC_VER) && _MSC_VER < 1930
    return MakeUid<Type>("ReadOnly");
#else
    constexpr auto uid = MakeUid<Type>("ReadOnly");
    static_assert(IsValidUid(uid));
    return uid;
#endif
}
/**
 * @brief Generate UID for property type.
 */

template<typename Type>
inline constexpr BASE_NS::Uid UidFromType()
{
// At least visual c++ 2017 has a bug that causes this uid variable to be uninitialised at runtime and so crashing
// We know at least visual c++ 2022 works...
#if defined(_MSC_VER) && _MSC_VER < 1930
    return MakeUid<Type>("Property");
#else
    constexpr auto uid = MakeUid<Type>("Property");
    static_assert(IsValidUid(uid));
    return uid;
#endif
}

template<typename Type>
inline constexpr BASE_NS::Uid ArrayUidFromType()
{
    if constexpr (BASE_NS::is_array_v<Type>) {
        return UidFromType<Type>();
    }
    return UidFromType<Type[]>();
}

template<typename Type>
inline constexpr BASE_NS::Uid ItemUidFromType()
{
    return UidFromType<BASE_NS::remove_extent_t<Type>>();
}

template<>
struct MetaType<void> {
    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr char name[] = "void";
    static constexpr CORE_NS::PropertyTypeDecl coreType = CorePropertyTypeDeclFromType<void, BASE_NS::false_type>();
    // NOLINTEND(readability-identifier-naming)
};

META_END_NAMESPACE()

/**
 * @brief To introduce new types to the property system, call META_TYPE(<type>) in global namespace.
 */
#define META_TYPE_IMPL(a, n)                                                                    \
    template<>                                                                                  \
    struct ::META_NS::MetaType<a> {                                                             \
        static constexpr char name[] = n;                                                       \
        static constexpr CORE_NS::PropertyTypeDecl coreType =                                   \
            ::META_NS::CorePropertyTypeDeclFromType<a, BASE_NS::false_type>();                  \
    };                                                                                          \
    template<>                                                                                  \
    struct ::META_NS::MetaType<a[]> {                                                           \
        static constexpr char name[] = n "[]";                                                  \
        static constexpr CORE_NS::PropertyTypeDecl coreType =                                   \
            ::META_NS::CorePropertyTypeDeclFromType<a, BASE_NS::true_type>();                   \
    };                                                                                          \
    template<>                                                                                  \
    struct ::META_NS::MetaType<BASE_NS::vector<a>> {                                            \
        static constexpr char name[] = "BASE_NS::vector<" n ">";                                \
        static constexpr CORE_NS::PropertyTypeDecl coreType =                                   \
            ::META_NS::CorePropertyTypeDeclFromType<BASE_NS::vector<a>, BASE_NS::false_type>(); \
    };

#define META_TYPE(a) META_TYPE_IMPL(a, #a)
#define META_INTERFACE_TYPE(a)                   \
    META_TYPE_IMPL(a::Ptr, #a "::Ptr")           \
    META_TYPE_IMPL(a::ConstPtr, #a "::ConstPtr") \
    META_TYPE_IMPL(a::WeakPtr, #a "::WeakPtr")   \
    META_TYPE_IMPL(a::ConstWeakPtr, #a "::ConstWeakPtr")

META_TYPE(float);
META_TYPE(double);
META_TYPE(bool);
META_TYPE(uint8_t);
META_TYPE(uint16_t);
META_TYPE(uint32_t);
META_TYPE(uint64_t);
META_TYPE(int8_t);
META_TYPE(int16_t);
META_TYPE(int32_t);
META_TYPE(int64_t);
#ifdef __APPLE__
META_TYPE(size_t);
#endif
META_TYPE(BASE_NS::Uid);
META_TYPE(BASE_NS::string);
META_TYPE(BASE_NS::string_view);
META_TYPE(BASE_NS::Math::Vec2);
META_TYPE(BASE_NS::Math::UVec2);
META_TYPE(BASE_NS::Math::IVec2);
META_TYPE(BASE_NS::Math::Vec3);
META_TYPE(BASE_NS::Math::UVec3);
META_TYPE(BASE_NS::Math::IVec3);
META_TYPE(BASE_NS::Math::Vec4);
META_TYPE(BASE_NS::Math::UVec4);
META_TYPE(BASE_NS::Math::IVec4);
META_TYPE(BASE_NS::Math::Quat);
META_TYPE(BASE_NS::Math::Mat3X3);
META_TYPE(BASE_NS::Math::Mat4X4);
META_TYPE(CORE_NS::Entity);
META_TYPE(CORE_NS::EntityReference);

CORE_BEGIN_NAMESPACE()
class IPropertyHandle;
DECLARE_PROPERTY_TYPE(CORE_NS::IPropertyHandle*);
CORE_END_NAMESPACE()

META_TYPE(CORE_NS::IPropertyHandle*);

#endif
