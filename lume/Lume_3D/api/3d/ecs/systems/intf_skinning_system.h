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

#ifndef API_3D_ECS_SYSTEMS_ISKINNING_SYSTEM_H
#define API_3D_ECS_SYSTEMS_ISKINNING_SYSTEM_H

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <core/ecs/intf_system.h>

CORE_BEGIN_NAMESPACE()
struct Entity;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_ecs_systems_iskinning */
/** ISkinning system */
class ISkinningSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "06931e7e-cd68-43b4-8f89-40652dade9fa" };

    /** Creates a skin instance for the given entity.
     *  @param skinIbmEntity Entity where we get skin ibm matrices.
     *  @param joints List of entities which are the joints of the skin. The order should match the order of the skin
     * IBM entity's matrices.
     *  @param entity Entity where we get node which we use in instance creation.
     *  @param skeleton Entity pointing to the node that is the common root of the joint entity hierarchy (optional).
     */
    virtual void CreateInstance(CORE_NS::Entity const& skinIbmEntity,
        BASE_NS::array_view<const CORE_NS::Entity> const& joints, CORE_NS::Entity const& entity,
        CORE_NS::Entity const& skeleton) = 0;

    /** Creates a skin instance for the given entity.
     *  @param skinIbmEntity Entity where we get skin ibm matrices and joint entities.
     *  @param entity Entity where we get node which we use in instance creation.
     *  @param skeleton Entity pointing to the node that is the common root of the joint entity hierarchy (optional).
     */
    virtual void CreateInstance(
        CORE_NS::Entity const& skinIbmEntity, CORE_NS::Entity const& entity, CORE_NS::Entity const& skeleton) = 0;

    /** Destroys Skin Instance.
     *  @param entity Entity whose skin instance is destroyed.
     */
    virtual void DestroyInstance(CORE_NS::Entity const& entity) = 0;

protected:
    ISkinningSystem() = default;
    ISkinningSystem(const ISkinningSystem&) = delete;
    ISkinningSystem(ISkinningSystem&&) = delete;
    ISkinningSystem& operator=(const ISkinningSystem&) = delete;
    ISkinningSystem& operator=(ISkinningSystem&&) = delete;
};

/** @ingroup group_ecs_systems_iskinning */
/** Return name of this system
 */
inline constexpr BASE_NS::string_view GetName(const ISkinningSystem*)
{
    return "SkinningSystem";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_SYSTEMS_ISKINNING_SYSTEM_H
