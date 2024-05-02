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

#ifndef API_3D_ECS_SYSTEMS_IMORPHING_SYSTEM_H
#define API_3D_ECS_SYSTEMS_IMORPHING_SYSTEM_H

#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/ecs/intf_system.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderDataStoreManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class ISceneNode;

/** @ingroup group_ecs_systems_imorphing */
/** Morphing System.
 */
class IMorphingSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "96c75ea1-802c-426d-8188-af4ff0887e5e" };
    /** Properties */
    struct Properties {
        /** Data store manager */
        [[deprecated]] RENDER_NS::IRenderDataStoreManager* dataStoreManager { nullptr };
        /** Data store name */
        BASE_NS::string dataStoreName;
    };

protected:
    IMorphingSystem() = default;
    IMorphingSystem(const IMorphingSystem&) = delete;
    IMorphingSystem(IMorphingSystem&&) = delete;
    IMorphingSystem& operator=(const IMorphingSystem&) = delete;
    IMorphingSystem& operator=(IMorphingSystem&&) = delete;
};

/** @ingroup group_ecs_systems_imorphing */
/** Return name of this system
 */
inline constexpr BASE_NS::string_view GetName(const IMorphingSystem*)
{
    return "MorphingSystem";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_SYSTEMS_IMORPHING_SYSTEM_H
