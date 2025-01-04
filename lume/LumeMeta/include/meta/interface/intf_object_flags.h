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

#ifndef META_INTERFACE_IOBJECT_FLAGS_H
#define META_INTERFACE_IOBJECT_FLAGS_H

#include <core/plugin/intf_interface.h>

#include <meta/base/bit_field.h>
#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IObjectFlags, "18f6d873-174e-406b-a252-3cb11a9f45c0")

/**
 * @brief Flags for all object types.
 * @note  Depending on the interfaces a particular class implements, some of the flags
 *        may not have any effect.
 */
enum class ObjectFlagBits : uint8_t {
    /** No bits set */
    NONE = 0,
    /** Should this object be serialized.*/
    SERIALIZE = 1,
    /** If the object contains a sub-hierarchy (e.g. through implementing IContainer or IContent),
     *  this flag controls whether the hierarchy is serialized.
     *  If SERIALIZE flag is not set, this flag has no effect. */
    SERIALIZE_HIERARCHY = 2,
    /** If the object has attachments, this flag controls whether they are serialized.
     *  If SERIALIZE flag is not set, this flag has no effect. */
    SERIALIZE_ATTACHMENTS = 4,
    /** The object is an internal object for the implementation.
     * E.g. a user interface editor should not show the object in the UI. */
    INTERNAL = 8,
    /** The object is native to the system, e.g. property predefined in the implementing class, BorderWidth
     * is a native property of the Rectangle. This flag is not set when object was created by the user. */
    NATIVE = 16,
    /** The default object flags for for objects. The default flags for a class can be
        queried by calling IObjectFlags::GetObjectDefaultFlags. */
    DEFAULT_FLAGS = SERIALIZE | SERIALIZE_HIERARCHY | SERIALIZE_ATTACHMENTS,
};

inline ObjectFlagBits operator|(ObjectFlagBits l, ObjectFlagBits r)
{
    return ObjectFlagBits(uint8_t(l) | uint8_t(r));
}

/**
 * @brief All flags for object, first 8 bits are generic object flags and rest of the bits are
 * for specific objects to define as their extension.
 */
using ObjectFlagBitsValue = EnumBitField<ObjectFlagBits, uint64_t>;

/**
 * @brief Offset where user defined extension bits start and how many there are.
 */
constexpr size_t USER_FLAG_BASE = 8;
constexpr size_t USER_FLAG_SIZE = 56;

/**
 * @brief The IObjectFlags interface can be implemented by classes which
 *        allow setting flags on the objects.
 */
class IObjectFlags : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectFlags)
public:
    /**
     * @brief Get Flags associated with this object
     */
    virtual ObjectFlagBitsValue GetObjectFlags() const = 0;
    /**
     * @brief Set Flags associated with this object
     */
    virtual void SetObjectFlags(const ObjectFlagBitsValue& value) = 0;
    /**
     * @brief Returns the default object flags for the class which implements IObjectFlags.
     */
    virtual ObjectFlagBitsValue GetObjectDefaultFlags() const = 0;
};

/**
 * @brief A helper function for setting the state of an object flag.
 * @param object The object to set flags on.
 * @param value The ObjectFlagBitsValue to set.
 * @param isSet If true, the bits should be set. If false, bits should be unset.
 */
template<typename T, typename = BASE_NS::enable_if<IsKindOfPointer_v<T>>>
void SetObjectFlags(const T& object, ObjectFlagBitsValue value, bool isSet)
{
    if (auto objectFlagsPtr = interface_cast<IObjectFlags>(object)) {
        const auto flags = objectFlagsPtr->GetObjectFlags();
        objectFlagsPtr->SetObjectFlags(isSet ? flags | value : flags & ~value);
    }
}

template<typename T, typename = BASE_NS::enable_if<IsKindOfPointer_v<T>>>
ObjectFlagBitsValue GetObjectFlags(const T& object)
{
    const auto objectFlagsPtr = interface_cast<IObjectFlags>(object);
    return objectFlagsPtr ? objectFlagsPtr->GetObjectFlags() : ObjectFlagBitsValue(ObjectFlagBits::NONE);
}

template<typename T, typename = BASE_NS::enable_if<IsKindOfPointer_v<T>>>
bool IsFlagSet(const T& object, ObjectFlagBitsValue value)
{
    if (auto objectFlagsPtr = interface_cast<IObjectFlags>(object)) {
        return objectFlagsPtr->GetObjectFlags().IsSet(value);
    }
    return false;
}

META_END_NAMESPACE()

META_TYPE(META_NS::IObjectFlags::Ptr);
META_TYPE(META_NS::IObjectFlags::WeakPtr);
META_TYPE(META_NS::IObjectFlags::ConstPtr);
META_TYPE(META_NS::IObjectFlags::ConstWeakPtr);

#endif // META_INTERFACE_IOBJECT_FLAGS_H
