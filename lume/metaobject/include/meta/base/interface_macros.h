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

#ifndef META_BASE_INTERFACE_MACROS_H
#define META_BASE_INTERFACE_MACROS_H

#include "types.h"

/**
 * @brief Implement plain reference counting (see IInterface).
 */
#define META_IMPLEMENT_REF_COUNT_NO_ONDESTROY()        \
    int32_t refcnt_ { 0 };                             \
    void Ref() override                                \
    {                                                  \
        CORE_NS::AtomicIncrement(&refcnt_);            \
    }                                                  \
    void Unref() override                              \
    {                                                  \
        if (CORE_NS::AtomicDecrement(&refcnt_) == 0) { \
            delete this;                               \
        }                                              \
    }

#define META_DEFAULT_COPY(className)                \
    className(const className&) noexcept = default; \
    className& operator=(const className&) noexcept = default;

#define META_DEFAULT_MOVE(className)           \
    className(className&&) noexcept = default; \
    className& operator=(className&&) noexcept = default;

#define META_DEFAULT_COPY_MOVE(className) \
    META_DEFAULT_COPY(className)          \
    META_DEFAULT_MOVE(className)

/**
 * @brief Make class non-copyable.
 */
#define META_NO_COPY(className)           \
    className(const className&) = delete; \
    className& operator=(const className&) = delete;

/**
 * @brief Make class non-movable.
 */
#define META_NO_MOVE(className)      \
    className(className&&) = delete; \
    className& operator=(className&&) = delete;

/**
 * @brief Make class non-copyable and non-movable.
 */
#define META_NO_COPY_MOVE(className) \
    META_NO_COPY(className)          \
    META_NO_MOVE(className)

/**
 * @brief Add compiler generated default constructor and virtual destructor to a class.
 */
#define META_INTERFACE_CTOR_DTOR(className) \
    className() noexcept = default;         \
    virtual ~className() = default;

/**
 * @brief Make class non-copyable and non-movable with compiler generated default constructor and virtual destructor.
 */
#define META_NO_COPY_MOVE_INTERFACE(className) \
    META_INTERFACE_CTOR_DTOR(className)        \
    META_NO_COPY_MOVE(className)

/**
 * @see NO_COPY_MOVE_INTERFACE
 */
#define NO_COPY_MOVE_BASE(className) NO_COPY_MOVE_INTERFACE(className)

META_BEGIN_NAMESPACE()
namespace Internal {
constexpr const InterfaceInfo ToInterfaceInfoImpl(const InterfaceInfo& info, BASE_NS::string_view)
{
    return info;
}
constexpr const InterfaceInfo ToInterfaceInfoImpl(TypeId uid, BASE_NS::string_view name)
{
    return InterfaceInfo { uid, name };
}
constexpr const InterfaceInfo ToInterfaceInfoImpl(const char (&str)[37], BASE_NS::string_view name)
{
    return InterfaceInfo { TypeId { str }, name };
}
} // namespace Internal
META_END_NAMESPACE()

#define META_INTERFACE3(basename, name, intf_name)                                                         \
public:                                                                                                    \
    using base = basename;                                                                                 \
    using base::GetInterface;                                                                              \
    /* NOLINTNEXTLINE(readability-identifier-naming) */                                                    \
    constexpr static const META_NS::InterfaceInfo INTERFACE_INFO { META_NS::Internal::ToInterfaceInfoImpl( \
        intf_name, #name) };                                                                               \
    constexpr static const BASE_NS::Uid UID { intf_name };                                                 \
    static_assert(META_NS::IsValidUid(UID), "invalid UID");                                                \
    using Ptr = BASE_NS::shared_ptr<name>;                                                                 \
    using ConstPtr = BASE_NS::shared_ptr<const name>;                                                      \
    using WeakPtr = BASE_NS::weak_ptr<name>;                                                               \
    using ConstWeakPtr = BASE_NS::weak_ptr<const name>;                                                    \
                                                                                                           \
protected:                                                                                                 \
    name() noexcept = default;                                                                             \
    ~name() override = default;                                                                            \
    META_NO_COPY_MOVE(name)                                                                                \
private:

#define META_INTERFACE2(basename, name) META_INTERFACE3(basename, name, InterfaceId::name)

/**
 * @brief Implement "interface"-interface for class. Supports two and three parameter versions.
 * Example:
 *      META_INTERFACE(CORE_NS::IInterface, ICallContext, "2e9cac45-0e61-4152-8b2a-bc1c65fded3d")
 *      META_INTERFACE(CORE_NS::IInterface, ICallContext)
 *
 *   The two parameter version expects to find InterfaceId::ICallContext for the UID (see META_REGISTER_INTERFACE)
 */
#define META_INTERFACE(...) \
    META_EXPAND(META_GET_MACRO3_IMPL(__VA_ARGS__, META_INTERFACE3, META_INTERFACE2)(__VA_ARGS__))

#include <meta/base/shared_ptr.h>

#endif
