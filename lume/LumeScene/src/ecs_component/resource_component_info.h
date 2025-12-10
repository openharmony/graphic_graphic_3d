/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_ECS_COMPONENT_RESOURCE_COMPONENT_INFO_H
#define SCENE_SRC_ECS_COMPONENT_RESOURCE_COMPONENT_INFO_H

#include <core/plugin/intf_plugin.h>

#include "resource_component.h"

SCENE_BEGIN_NAMESPACE()

CORE_NS::IComponentManager* IResourceComponentManagerInstance(CORE_NS::IEcs&);
void IResourceComponentManagerDestroy(CORE_NS::IComponentManager*);

constexpr CORE_NS::ComponentManagerTypeInfo RESOURCE_COMPONENT_TYPE_INFO { { CORE_NS::ComponentManagerTypeInfo::UID },
    IResourceComponentManager::UID, CORE_NS::GetName<IResourceComponentManager>().data(),
    IResourceComponentManagerInstance, IResourceComponentManagerDestroy };

SCENE_END_NAMESPACE()

#endif
