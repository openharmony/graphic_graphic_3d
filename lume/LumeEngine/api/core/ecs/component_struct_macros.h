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
#include <cstddef>

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/scoped_handle.h>

// clang-format off
// Component struct defines
#define DECLARE_GENERIC_INTERFACE1(NAME, TYPE, TYPE_UID)                                      \
    /** @ingroup group_ecs_componentmanagers_##NAME */                                        \
    class NAME : public CORE_NS::IComponentManager {                                          \
    public:                                                                                   \
        using ComponentId = CORE_NS::IComponentManager::ComponentId;                          \
        /** UID of TYPE */                                                                    \
        static constexpr BASE_NS::Uid UID{ TYPE_UID };                                        \
        /** Sets TYPE */                                                                      \
        virtual void Set(ComponentId index, const TYPE& data) = 0;                            \
        /** Sets TYPE */                                                                      \
        virtual void Set(CORE_NS::Entity entity, const TYPE& data) = 0;                       \
        /** Gets TYPE */                                                                      \
        virtual TYPE Get(ComponentId index) const = 0;                                        \
        /** Gets TYPE */                                                                      \
        virtual TYPE Get(CORE_NS::Entity entity) const = 0;                                   \
        /** Scoped read access to component data. */                                          \
        virtual CORE_NS::ScopedHandle<const TYPE> Read(ComponentId index) const = 0;          \
        /** Scoped read access to component data. */                                          \
        virtual CORE_NS::ScopedHandle<const TYPE> Read(CORE_NS::Entity entity) const = 0;     \
        /** Scoped write access to component data. */                                         \
        virtual CORE_NS::ScopedHandle<TYPE> Write(ComponentId index) = 0;                     \
        /** Scoped write access to component data. */                                         \
        virtual CORE_NS::ScopedHandle<TYPE> Write(CORE_NS::Entity entity) = 0;

#define DECLARE_GENERIC_INTERFACE2(NAME, TYPE)                                                \
    protected:                                                                                \
        NAME() = default;                                                                     \
        virtual ~NAME() = default;                                                            \
        NAME(const NAME&) = delete;                                                           \
        NAME(NAME&&) = delete;                                                                \
        NAME& operator=(const NAME&) = delete;                                                \
        NAME& operator=(NAME&& data) = delete;                                                \
    };                                                                                        \
    inline constexpr BASE_NS::string_view GetName(const NAME*)                                \
    {                                                                                         \
        return #TYPE;                                                                         \
    }                                                                                         \
    inline constexpr BASE_NS::string_view GetName(const TYPE*)                                \
    {                                                                                         \
        return #TYPE;                                                                         \
    }                                                                                         \
    inline constexpr BASE_NS::Uid GetUid(const NAME*)                                         \
    {                                                                                         \
        return NAME::UID;                                                                     \
    }                                                                                         \
    inline constexpr BASE_NS::Uid GetUid(const TYPE*)                                         \
    {                                                                                         \
        return NAME::UID;                                                                     \
    }

#define DECLARE_GENERIC_INTERFACE(NAME, TYPE, TYPE_UID)                                       \
    DECLARE_GENERIC_INTERFACE1(NAME, TYPE, TYPE_UID)                                          \
    DECLARE_GENERIC_INTERFACE2(NAME, TYPE)

#if !defined(IMPLEMENT_MANAGER)
#undef BEGIN_COMPONENT
#undef VALUE
#undef ARRAY_VALUE
#undef DEFINE_PROPERTY
#undef DEFINE_BITFIELD_PROPERTY
#undef DEFINE_ARRAY_PROPERTY
#undef BEGIN_COMPONENT_MANAGER
#undef END_COMPONENT_MANAGER
#undef END_COMPONENT
#undef END_COMPONENT_EXT
#define BEGIN_COMPONENT(NAME, COMPONENT_NAME) /** @ingroup group_ecs_components_##COMPONENT_NAME */ \
    struct COMPONENT_NAME {
#define VALUE(val) val
#define ARRAY_VALUE(...) __VA_ARGS__
#define DEFINE_PROPERTY(type, name, displayname, flags, value) type name{ value };
#define DEFINE_BITFIELD_PROPERTY(type, name, displayname, flags, value, enumeration) type name{ value };
#define DEFINE_ARRAY_PROPERTY(type, count, name, displayname, flags, value) type name[count]{ value };

#define BEGIN_COMPONENT_MANAGER(NAME, COMPONENT_NAME, TYPE_UID) \
    DECLARE_GENERIC_INTERFACE1(NAME, COMPONENT_NAME, TYPE_UID)
#define END_COMPONENT_MANAGER(NAME, COMPONENT_NAME) \
    DECLARE_GENERIC_INTERFACE2(NAME, COMPONENT_NAME)

#define END_COMPONENT(NAME, COMPONENT_NAME, TYPE_UID) };      \
    BEGIN_COMPONENT_MANAGER(NAME, COMPONENT_NAME, TYPE_UID) \
    END_COMPONENT_MANAGER(NAME, COMPONENT_NAME)

#define END_COMPONENT_EXT(NAME, COMPONENT_NAME, TYPE_UID, METHODS) }; \
    BEGIN_COMPONENT_MANAGER(NAME, COMPONENT_NAME, TYPE_UID) \
    METHODS \
    END_COMPONENT_MANAGER(NAME, COMPONENT_NAME)
#endif
// clang-format on
