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
#ifndef SCENEPLUGIN_INTF_MULTIMESHINITIALIZATION_H
#define SCENEPLUGIN_INTF_MULTIMESHINITIALIZATION_H

#include <scene_plugin/namespace.h>

#include <core/ecs/entity.h>

#include <meta/base/types.h>

#include "scene_holder.h"

REGISTER_INTERFACE(IMultimeshInitilization, "383f57c1-c7fe-424e-b037-edaf0447a7dd")
class IMultimeshInitilization : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMultimeshInitilization, InterfaceId::IMultimeshInitilization)
public:
    virtual void Initialize(SceneHolder::Ptr sceneHolder, size_t instanceCount, CORE_NS::Entity baseComponent) = 0;
};

#endif