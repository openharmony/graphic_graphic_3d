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

#ifndef META_BASE_TYPES_H
#define META_BASE_TYPES_H

#include <base/containers/string_view.h>
#include <base/util/uid.h>

#include <meta/base/ids.h>
#include <meta/base/namespace.h>

#include "macros.h"

META_BEGIN_NAMESPACE()

using nullptr_t = decltype(nullptr);

struct NoCheckT {};
constexpr NoCheckT NOCHECK;

/**
 * @brief The InterfaceInfo struct is used to register a new interface to the toolkit.
 *        Call META_REGISTER_INTERFACE(<InterfaceName>, <InterfaceId>) to register the interface.
 */
struct InterfaceInfo {
    /** Returns the unique id of the interface. */
    constexpr const TypeId& Id() const noexcept
    {
        return id;
    }
    /** Returns the name of the interface. */
    constexpr const BASE_NS::string_view& Name() const noexcept
    {
        return name;
    }
    /** Returns the readable name of the interface. */
    constexpr const BASE_NS::string_view& ReadableName() const noexcept
    {
        return readableName;
    }
    constexpr operator TypeId() const noexcept
    {
        return id;
    }
    /** Implicit conversion operator InterfaceInfo->Uid. */
    constexpr operator BASE_NS::Uid() const noexcept
    {
        return id.ToUid();
    }
    constexpr bool IsValid() const noexcept
    {
        return id.IsValid();
    }
    /** The Uid of the interface. Must be unique. */
    TypeId id;
    /** The name of the interface. */
    BASE_NS::string_view name;
    /** The readable name of the interface. */
    BASE_NS::string_view readableName;
};

inline bool operator==(const InterfaceInfo& lhs, const InterfaceInfo& rhs) noexcept
{
    return lhs.id == rhs.id;
}

inline bool operator!=(const InterfaceInfo& lhs, const InterfaceInfo& rhs) noexcept
{
    return lhs.id != rhs.id;
}

inline bool operator<(const InterfaceInfo& lhs, const InterfaceInfo& rhs) noexcept
{
    return lhs.id < rhs.id;
}

/**
 * @brief List of supported object categories.
 */
namespace ObjectCategory {
enum Bits : uint64_t {
    /** Bitmask indicating any category except DEPRECATED */
    ANY = 0,
    /** The object does not belong to any specific category */
    NO_CATEGORY = 0x1,
    /** The object is a UI control */
    WIDGET = 0x2,
    /** The object is an animation */
    ANIMATION = 0x4,
    /** The object is a layout object */
    LAYOUT = 0x8,
    /** The object is a container object */
    CONTAINER = 0x10,
    /** The object is a curve */
    CURVE = 0x20,
    /** The object is a shape */
    SHAPE = 0x40,
    /** The object is for implementation */
    INTERNAL = 0x80,
    /** The object is an application-specific object. Objects registered by
        applications should usually use this category. */
    APPLICATION = 0x100,
    /** The object is an animation modifier */
    ANIMATION_MODIFIER = 0x200,
    /** The object is a gesture */
    GESTURE = 0x400,
    /** The object is deprecated */
    DEPRECATED = 0x800,
};
}

using ObjectCategoryBits = ObjectCategory::Bits;

/**
 * @brief The ClassInfo struct contains the basic type information for a given class.
 */
struct ClassInfo {
    /** Returns the unique id of the class. */
    constexpr const ObjectId& Id() const noexcept
    {
        return id;
    }
    /** Returns the name of the class. */
    constexpr const BASE_NS::string_view& Name() const noexcept
    {
        return name;
    }
    /** Returns the readable name of the class. */
    constexpr const BASE_NS::string_view& ReadableName() const noexcept
    {
        return readableName;
    }
    /** Should the class be created as singleton? */
    constexpr bool IsSingleton() const noexcept
    {
        return singleton;
    }
    /** Implicit conversion operator ClassInfo->Uid. */
    constexpr operator ObjectId() const noexcept
    {
        return id;
    }
    /** The Uid of the class. Must be unique. */
    ObjectId id;
    /** The name of the class. */
    BASE_NS::string_view name;
    /** Category of the class */
    ObjectCategoryBits category;
    /** A flag indicating if the class can be created as singleton. */
    bool singleton;
    /** The readable name of the class. */
    BASE_NS::string_view readableName;
};

inline bool operator==(const ClassInfo& lhs, const ClassInfo& rhs) noexcept
{
    return lhs.id == rhs.id;
}

inline bool operator!=(const ClassInfo& lhs, const ClassInfo& rhs) noexcept
{
    return lhs.id != rhs.id;
}

inline bool operator<(const ClassInfo& lhs, const ClassInfo& rhs) noexcept
{
    return lhs.id < rhs.id;
}

/**
 * @brief Register UID and category for class name. This tells there exists a concrete object type with this UID which
 * can be constructed via ObjectRegistry. Example: META_REGISTER_CLASS(AnimationController,
 * "32ccf293-e684-46e3-b733-85f9bbcc703c", ObjectCategoryBits::NO_CATEGORY)
 */
#define META_REGISTER_IMPL_CLASS4(singleton, name, id, cat, readable)                                                  \
    namespace ClassId {                                                                                                \
    [[maybe_unused]] inline constexpr ::META_NS::ClassInfo name { BASE_NS::Uid(id), #name, cat, singleton, readable }; \
    }

#define META_REGISTER_IMPL_CLASS3(singleton, name, id, cat) META_REGISTER_IMPL_CLASS4(singleton, name, id, cat, #name)

#define META_REGISTER_IMPL_CLASS(singleton, ...)                                                         \
    META_EXPAND(META_GET_MACRO4_IMPL(__VA_ARGS__, META_REGISTER_IMPL_CLASS4, META_REGISTER_IMPL_CLASS3)( \
        singleton, __VA_ARGS__))

#define META_REGISTER_CLASS(...) META_REGISTER_IMPL_CLASS(false, __VA_ARGS__)
#define REGISTER_CLASS(...) META_REGISTER_CLASS(__VA_ARGS__)

/**
 * @brief Register UID and category for class name. This tells there exists a singleton object with this UID which can
 * be accessed via ObjectRegistry.
 */
#define META_REGISTER_SINGLETON_CLASS(...) META_REGISTER_IMPL_CLASS(true, __VA_ARGS__)
#define REGISTER_SINGLETON_CLASS(...) META_REGISTER_SINGLETON_CLASS(__VA_ARGS__)

/**
 * @brief Register UID for interface type. This will associate interface name to UID.
 * Note: One can use this with two parameter version of META_INTERFACE macro, or
 * alternatively there is no need for this if using the three parameter version of META_INTERFACE.
 * Example:
 *     META_REGISTER_INTERFACE(IAttach, "2cc0d7b9-ccec-474e-acdd-d06fb5aeb714")
 *     class IAttach : public CORE_NS::IInterface {
 *         META_INTERFACE(CORE_NS::IInterface, IAttach)
 *   Here the META_INTERFACE macro will find the UID using the name "IAttach".
 */
#define META_REGISTER_INTERFACE3(name, id, readable)                                                          \
    namespace InterfaceId {                                                                                   \
    [[maybe_unused]] inline constexpr ::META_NS::InterfaceInfo name { META_NS::TypeId(id), #name, readable }; \
    }

#define META_REGISTER_INTERFACE2(name, id) META_REGISTER_INTERFACE3(name, id, #name)

#define META_REGISTER_INTERFACE(...) \
    META_EXPAND(META_GET_MACRO3_IMPL(__VA_ARGS__, META_REGISTER_INTERFACE3, META_REGISTER_INTERFACE2)(__VA_ARGS__))

#define REGISTER_INTERFACE(...) META_REGISTER_INTERFACE(__VA_ARGS__)

META_END_NAMESPACE()

#endif
