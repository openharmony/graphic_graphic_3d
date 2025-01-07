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

#ifndef API_CORE_PROPERTY_PROPERTY_H
#define API_CORE_PROPERTY_PROPERTY_H

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/compile_time_hashes.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** \addtogroup group_property_propertytypes
 *  @{
 */
/** Property flags */
enum class PropertyFlags : uint32_t {
    /** Min */
    HAS_MIN = 1,
    /** Max */
    HAS_MAX = 2,
    /** slider */
    IS_SLIDER = 4,
    /** Hide From UI */
    IS_HIDDEN = 8,
    /** Readonly */
    IS_READONLY = 16,
    /** NO_SERIALIZE */
    NO_SERIALIZE = 32,
    /** Property is a bit flag container */
    IS_BITFIELD = 64,
};

constexpr uint32_t operator|(PropertyFlags a, PropertyFlags b)
{
    return ((uint32_t)a) | ((uint32_t)b);
}

constexpr uint32_t operator|(uint32_t a, PropertyFlags b)
{
    return ((uint32_t)a) | ((uint32_t)b);
}

/** Property type declaration */
struct PropertyTypeDecl {
    /** Is this a c-style array. (size unknown) */
    bool isArray;
    /** Size of the type in bytes. For arrays this is the size of a single element. */
    uint32_t byteSize;
    /** Type hash */
    uint64_t typeHash;
    /** Compare hash */
    uint64_t compareHash;
    /** Type name */
    BASE_NS::string_view name;
    /** Return compare hash of this property type declaration */
    constexpr operator uint64_t() const
    {
        return compareHash;
    }
};

/** Equality operator, returns true if types are the same */
constexpr inline bool operator==(const PropertyTypeDecl& aA, const PropertyTypeDecl& aB)
{
    return aA.compareHash == aB.compareHash;
}

/** Inequality operator, returns true if types are not the same */
constexpr inline bool operator!=(const PropertyTypeDecl& aA, const PropertyTypeDecl& aB)
{
    return aA.compareHash != aB.compareHash;
}

/** @} */
/** Metadata of an enumeratio value (enumerator). */
struct EnumMetaData {
    BASE_NS::string_view name;
    BASE_NS::string_view displayName;
    int64_t value;
};

struct Property; // Forward declare the struct.

/** Metadata of a type. */
struct MetaData {
    BASE_NS::array_view<const Property> memberProperties; // properties of the actual "type"
    const struct ContainerApi* containerMethods;
    BASE_NS::array_view<const EnumMetaData> enumMetaData;
};

/** Metadata of a variable. */
struct Property {
    BASE_NS::string_view name;
    uint64_t hash;
    PropertyTypeDecl type;
    size_t count;     // Count of elements in array, 1 if not an array
    size_t size;      // Size of property in bytes.
    uintptr_t offset; // Offset to data blob
    BASE_NS::string_view displayName;
    uint32_t flags; // PropertyFlags combination.
    MetaData metaData;
};

/** Functions for accessing a container and metadata of the containers element type. */
struct ContainerApi {
    // return the size of container
    size_t (*size)(uintptr_t container);
    // resize the container.
    void (*resize)(uintptr_t container, size_t newSize);
    // remove element from container
    void (*erase)(uintptr_t container, size_t index);
    // insert element at location to container
    uintptr_t (*insert)(uintptr_t container, size_t index);
    // get value by indexed property from container (returns uintptr_t type pointer to the element)
    uintptr_t (*get)(uintptr_t container, size_t index);
    // property type for the elements in array or container
    Property property;
};

/** Fills a PropertyTypeDecl for the given type.
 * @param baseName String used as the type's name. typeHash and compareHash are based on this string.
 * @return PropertyTypeDecl for the given type.
 */
template<typename T, typename = BASE_NS::enable_if_t<!BASE_NS::is_array_v<T>>>
constexpr CORE_NS::PropertyTypeDecl MakePropertyTypeDecl(const char* baseName)
{
    return CORE_NS::PropertyTypeDecl { false, static_cast<uint32_t>(sizeof(T)),
        BASE_NS::CompileTime::FNV1aHash(baseName), BASE_NS::CompileTime::FNV1aHash(baseName), baseName };
}

/** Fills a PropertyTypeDecl for the given array type.
 * @param baseName String used as the type's name. typeHash is based on this string.
 * @param arrayTypeName compareHash is based on this string.
 * @return PropertyTypeDecl for the given type.
 */
template<typename T, typename = BASE_NS::enable_if_t<BASE_NS::is_array_v<T>>>
constexpr CORE_NS::PropertyTypeDecl MakePropertyTypeDecl(const char* baseName, const char* arrayTypeName)
{
    return CORE_NS::PropertyTypeDecl { true, static_cast<uint32_t>(sizeof(BASE_NS::remove_extent_t<T>)),
        BASE_NS::CompileTime::FNV1aHash(baseName), BASE_NS::CompileTime::FNV1aHash(arrayTypeName), baseName };
}

#define PROPERTYNAME(b) #b
#define ARRAY_PROPERTYNAME(b) (#b "_ARRAY")

/** Helper for filling a PropertyTypeDecl for the given type. */
#define PROPERTYTYPE(b) CORE_NS::MakePropertyTypeDecl<b>(PROPERTYNAME(b))

/** Helper for filling a PropertyTypeDecl for an array of the given type. */
#define PROPERTYTYPE_ARRAY(b) CORE_NS::MakePropertyTypeDecl<b[]>(PROPERTYNAME(b), ARRAY_PROPERTYNAME(b))
CORE_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()
template<typename T>
uint64_t hash(const T&);

template<>
inline uint64_t hash(const CORE_NS::PropertyTypeDecl& value)
{
    return value.compareHash;
}
BASE_END_NAMESPACE()
#endif // API_CORE_PROPERTY_PROPERTY_H
