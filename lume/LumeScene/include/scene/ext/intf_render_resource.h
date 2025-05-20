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

#ifndef SCENE_EXT_IRENDER_RESOURCE_H
#define SCENE_EXT_IRENDER_RESOURCE_H

#include <scene/base/namespace.h>

#include <base/math/vector.h>
#include <core/ecs/entity_reference.h>
#include <render/resource_handle.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/property/property_events.h>

SCENE_BEGIN_NAMESPACE()

class IInternalScene;

class IRenderResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRenderResource, "23bcea3d-de86-4c35-b3c5-c9425f27f6e7")
public:
    virtual bool SetRenderHandle(RENDER_NS::RenderHandleReference) = 0;
    virtual RENDER_NS::RenderHandleReference GetRenderHandle() const = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IRenderResource)

#endif
