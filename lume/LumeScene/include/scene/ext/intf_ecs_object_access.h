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

#ifndef SCENE_EXT_IECS_OBJECT_ACCESS_H
#define SCENE_EXT_IECS_OBJECT_ACCESS_H

#include <scene/base/types.h>
#include <scene/ext/intf_ecs_object.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief The IEcsObjectAccess interface is used by the framework internally to set/get the underlying ECS object of a
 *        wrapper object.
 */
class IEcsObjectAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsObjectAccess, "b4c430bd-29ad-41cf-96c6-16d4df74396a")
public:
    /**
     * @brief Sets the underlying ecs object of an object.
     * @return True if successful, false otherwise.
     */
    virtual bool SetEcsObject(const IEcsObject::Ptr&) = 0;
    /**
     * @brief Returns the underlying ecs object of an object.
     */
    virtual IEcsObject::Ptr GetEcsObject() const = 0;
};

/**
 * @brief Helper for getting the underlying entity from IEcsObjectAccess*.
 * @param ecsoa IEcsObjectAccess* instance.
 * @return Underlying entity or invalid entity in case of error.
 */
inline CORE_NS::Entity GetEcsObjectEntity(const IEcsObjectAccess* ecsoa)
{
    CORE_NS::Entity entity;
    if (ecsoa) {
        auto ecso = ecsoa->GetEcsObject();
        if (ecso) {
            entity = ecso->GetEntity();
        }
    }
    return entity;
}

/// @see GetEcsObjectEntity(const IEcsObjectAccess* ecsoa)
inline CORE_NS::Entity GetEcsObjectEntity(const IEcsObjectAccess::Ptr& ecsoa)
{
    return GetEcsObjectEntity(ecsoa.get());
}

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IEcsObjectAccess)

#endif
