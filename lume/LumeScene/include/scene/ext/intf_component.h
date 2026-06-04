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

#include <base/containers/string.h>
#include <base/containers/vector.h>

#include <meta/api/engine/util.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Reflects component at engine side
 */
class IComponent : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IComponent, "11e73afb-2a49-459a-85f9-e07abc2dc28a")
public:
    virtual BASE_NS::string GetName() const = 0;
    virtual bool PopulateAllProperties() = 0;

    /**
     * @brief Enumerate the property names for this component without creating engine value bridges.
     *        Returns full dotted names (e.g. "ComponentName.propertyName").
     */
    virtual BASE_NS::vector<META_NS::EnginePropertyInfo> EnumerateProperties(
        const META_NS::EnginePropertyInfoConfig& config) = 0;
    /**
     * @brief Default EnumerateProperties implementation only traverses the first level.
     */
    BASE_NS::vector<META_NS::EnginePropertyInfo> EnumerateProperties()
    {
        return EnumerateProperties({});
    }
};

/**
 * @brief Components not known to the LumeScene are mapped to IGenericComponent
 */
class IGenericComponent : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IGenericComponent, "d35462a8-3ba4-4e24-9f6a-aa2f635031c2")
public:
    virtual META_NS::ObjectId GetComponentId() const = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IComponent)
META_INTERFACE_TYPE(SCENE_NS::IGenericComponent)

#endif
