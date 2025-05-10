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

#ifndef API_CORE_ECS_ENTITY_H
#define API_CORE_ECS_ENTITY_H

#include <cstdint>

#include <base/namespace.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** \addtogroup group_ecs_entity
 *  @{
 */
/** Invalid entity value */
constexpr const uint32_t INVALID_ENTITY = ~0u;

/** Entity struct which holds the id
    high 32 bits is generation.
    low 32 bits is id.
*/
struct Entity {
    /** ID */
    uint64_t id { INVALID_ENTITY };
};

constexpr bool operator==(const Entity& lhs, const Entity& rhs)
{
    return lhs.id == rhs.id;
}

constexpr bool operator!=(const Entity& lhs, const Entity& rhs)
{
    return lhs.id != rhs.id;
}

constexpr bool operator<(const Entity& lhs, const Entity& rhs)
{
    return lhs.id < rhs.id;
}

constexpr bool operator<=(const Entity& lhs, const Entity& rhs)
{
    return lhs.id <= rhs.id;
}

constexpr bool operator>(const Entity& lhs, const Entity& rhs)
{
    return lhs.id > rhs.id;
}

constexpr bool operator>=(const Entity& lhs, const Entity& rhs)
{
    return lhs.id >= rhs.id;
}

/** Entity util for checking entities validity */
namespace EntityUtil {
/** Returns true if entity is valid */
inline bool IsValid(const Entity entity)
{
    return entity != Entity {};
}
} // namespace EntityUtil
/** @} */

CORE_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()
template<typename T>
uint64_t hash(const T&);

template<>
inline uint64_t hash(const CORE_NS::Entity& value)
{
    return value.id;
}
BASE_END_NAMESPACE()

#endif // API_CORE_ECS_ENTITY_H
