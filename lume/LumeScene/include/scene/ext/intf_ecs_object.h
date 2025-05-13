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

#ifndef SCENE_EXT_IECS_OBJECT_H
#define SCENE_EXT_IECS_OBJECT_H

#include <scene/ext/intf_internal_scene.h>

#include <meta/ext/object.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

SCENE_BEGIN_NAMESPACE()

class IEcsResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsResource, "4a605fe0-b0bf-4627-8f42-5e7d473e91ba")
public:
    virtual CORE_NS::Entity GetEntity() const = 0;
};

class IEcsObject : public IEcsResource {
    META_INTERFACE(IEcsResource, IEcsObject, "5576c6f2-364b-497f-b342-4fffeb022e53")
public:
    virtual BASE_NS::string GetPath() const = 0;
    virtual BASE_NS::string GetName() const = 0;
    virtual IInternalScene::Ptr GetScene() const = 0;
    virtual META_NS::IEngineValueManager::Ptr GetEngineValueManager() = 0;

    virtual bool SetName(const BASE_NS::string& name) = 0;

    /**
     * @brief Add all engine properties that the component defines. Construct new properties if they don't exist yet.
     * @param object Add properties to this object. Must not be a null pointer.
     * @param component The name of the component. If empty, add all properties from all components the object has.
     * @return True if all properties were created, false otherwise.
     */
    virtual Future<bool> AddAllProperties(const META_NS::IMetadata::Ptr& object, BASE_NS::string_view component) = 0;
    virtual Future<bool> AttachProperty(const META_NS::IProperty::Ptr&, BASE_NS::string_view path) = 0;
    virtual Future<META_NS::IProperty::Ptr> CreateProperty(BASE_NS::string_view path) = 0;
    virtual Future<META_NS::IProperty::Ptr> CreateProperty(const META_NS::IEngineValue::Ptr&) = 0;
    virtual Future<bool> AttachProperty(const META_NS::IProperty::Ptr&, const META_NS::IEngineValue::Ptr&) = 0;

    virtual Future<bool> SetActive(bool active) = 0;

    virtual void SyncProperties() = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IEcsObject)

#endif
