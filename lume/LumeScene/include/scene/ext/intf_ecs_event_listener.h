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

#ifndef SCENE_EXT_IECS_EVENT_LISTENER_H
#define SCENE_EXT_IECS_EVENT_LISTENER_H

#include <scene/interface/intf_node.h>

#include <core/ecs/intf_ecs.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class IEcsEventListener : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsEventListener, "e451db64-135a-46ef-9e18-c6e036fc3cb8")
public:
    virtual void OnEntityEvent(CORE_NS::IEcs::EntityListener::EventType type) = 0;
    virtual void OnComponentEvent(
        CORE_NS::IEcs::ComponentListener::EventType type, const CORE_NS::IComponentManager& component) = 0;
};

SCENE_END_NAMESPACE()

#endif