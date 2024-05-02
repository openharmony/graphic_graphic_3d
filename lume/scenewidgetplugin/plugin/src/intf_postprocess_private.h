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
#ifndef INTF_POSTPROCESS_PRIVATE_H
#define INTF_POSTPROCESS_PRIVATE_H

#include <scene_plugin/namespace.h>

#include <meta/ext/concrete_base_object.h>

#include "scene_holder.h"

REGISTER_INTERFACE(IPostProcessPrivate, "a4754697-2a2d-44e2-8faf-7b9a3077ed40")
class IPostProcessPrivate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPostProcessPrivate, InterfaceId::IPostProcessPrivate)
public:
    virtual void SetEntity(CORE_NS::Entity entity, SceneHolder::Ptr sh, bool preferEcsValues) = 0;
    virtual CORE_NS::Entity GetEntity() = 0;
};

REGISTER_INTERFACE(IPostProcessEffectPrivate, "dd6ae27b-ad67-432a-857e-df1037186245")
class IPostProcessEffectPrivate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPostProcessEffectPrivate, InterfaceId::IPostProcessEffectPrivate)
public:
    virtual void Bind(SCENE_NS::IEcsObject::Ptr, SceneHolder::Ptr sh, bool preferEcsValues) = 0;
};

#endif
