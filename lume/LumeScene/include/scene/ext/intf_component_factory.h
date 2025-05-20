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

#ifndef SCENE_EXT_ICOMPONENT_FACTORY_H
#define SCENE_EXT_ICOMPONENT_FACTORY_H

#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_object.h>

SCENE_BEGIN_NAMESPACE()

class IComponentFactory : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IComponentFactory, "512141cf-be70-40b2-9f37-e9a0670fff10")
public:
    virtual IComponent::Ptr CreateComponent(const IEcsObject::Ptr&) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IComponentFactory)

#endif
