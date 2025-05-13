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

#ifndef SCENE_EXT_ICOMPONENT_H
#define SCENE_EXT_ICOMPONENT_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Reflects component at engine side
 */
class IComponent : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IComponent, "11e73afb-2a49-459a-85f9-e07abc2dc28a")
public:
    virtual bool PopulateAllProperties() = 0;
};

/**
 * @brief Components not known to the LumeScene are mapped to IGenericComponent
 */
class IGenericComponent : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IGenericComponent, "d35462a8-3ba4-4e24-9f6a-aa2f635031c2")
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IComponent)
META_INTERFACE_TYPE(SCENE_NS::IGenericComponent)

#endif
