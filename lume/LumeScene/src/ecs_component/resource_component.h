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

#if !defined(SCENE_SRC_ECS_COMPONENT_RESOURCE_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define SCENE_SRC_ECS_COMPONENT_RESOURCE_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <scene/base/namespace.h>

#include <base/containers/string_view.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/resources/intf_resource.h>

SCENE_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IResourceComponentManager, ResourceComponent)
    DEFINE_PROPERTY(CORE_NS::ResourceId, resourceId, "ResourceId", 0,)
END_COMPONENT_EXT(IResourceComponentManager, ResourceComponent, "8356ec65-f43f-4dc6-82f5-d65a51d19c11",
                  /**
                   * @brief Get Entity from the resource id
                   */
                  virtual CORE_NS::Entity GetEntityWithResource(const CORE_NS::ResourceId& rid) const = 0;
                  /**
                   * @brief True if any resource has the given group
                   */
                  virtual bool HasGroup(BASE_NS::string_view group) const = 0;)
#if !defined(IMPLEMENT_MANAGER)
SCENE_END_NAMESPACE()
#endif

#endif
