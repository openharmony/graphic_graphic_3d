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

#ifndef SCENEPLUGIN_INTF_ECSSERIALIZER_H
#define SCENEPLUGIN_INTF_ECSSERIALIZER_H

#include <scene_plugin/interface/intf_entity_collection.h>
#include <scene_plugin/namespace.h>

#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/json/json.h>
#include <core/property/intf_property_api.h>
#include <core/property/scoped_handle.h>
#include <render/resource_handle.h>

SCENE_BEGIN_NAMESPACE()

class IEcsSerializer {
public:
    static const uint32_t VERSION_MAJOR { 22u };
    static const uint32_t VERSION_MINOR { 0u };

    struct EntityInfo {
        CORE_NS::Entity entity;
        BASE_NS::string srcUri;
        BASE_NS::string contextUri;
    };

    struct ExternalCollection {
        BASE_NS::string src;
        BASE_NS::string contextUri;
    };

    class IListener {
    public:
        virtual IEntityCollection* GetExternalCollection(
            CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri) = 0;

    protected:
        virtual ~IListener() = default;
    };

    class IPropertySerializer {
    public:
        virtual bool ToJson(const IEntityCollection& ec, const CORE_NS::Property& property, uintptr_t offset,
            CORE_NS::json::standalone_value& jsonOut) const = 0;
        virtual bool FromJson(const IEntityCollection& ec, const CORE_NS::json::value& jsonIn,
            const CORE_NS::Property& property, uintptr_t offset) const = 0;

    protected:
        virtual ~IPropertySerializer() = default;
    };

    virtual void SetListener(IListener* listener) = 0;

    virtual void SetDefaultSerializers() = 0;
    virtual void SetSerializer(const CORE_NS::PropertyTypeDecl& type, IPropertySerializer& serializer) = 0;

    virtual bool WriteEntityCollection(const IEntityCollection& ec, CORE_NS::json::standalone_value& jsonOut) const = 0;
    virtual bool WriteComponents(
        const IEntityCollection& ec, CORE_NS::Entity entity, CORE_NS::json::standalone_value& jsonOut) const = 0;
    virtual bool WriteComponent(const IEntityCollection& ec, CORE_NS::Entity entity,
        const CORE_NS::IComponentManager& cm, CORE_NS::IComponentManager::ComponentId id,
        CORE_NS::json::standalone_value& jsonOut) const = 0;
    virtual bool WriteProperty(const IEntityCollection& ec, const CORE_NS::Property& property, uintptr_t offset,
        CORE_NS::json::standalone_value& jsonOut) const = 0;

    virtual bool GatherExternalCollections(const CORE_NS::json::value& jsonIn, BASE_NS::string_view contextUri,
        BASE_NS::vector<ExternalCollection>& externalCollectionsOut) const = 0;

    virtual int /*IIoUtil::SerializationResult*/ ReadEntityCollection(
        IEntityCollection& ec, const CORE_NS::json::value& jsonIn, BASE_NS::string_view contextUri) const = 0;
    virtual bool ReadComponents(IEntityCollection& ec, const CORE_NS::json::value& jsonIn, bool setId) const = 0;
    virtual bool ReadComponent(IEntityCollection& ec, const CORE_NS::json::value& jsonIn, CORE_NS::Entity entity,
        CORE_NS::IComponentManager& component) const = 0;
    virtual bool ReadProperty(IEntityCollection& ec, const CORE_NS::json::value& jsonIn,
        const CORE_NS::Property& property, uintptr_t offset) const = 0;

    virtual RENDER_NS::RenderHandleReference LoadImageResource(BASE_NS::string_view uri) const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IEcsSerializer* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IEcsSerializer, Deleter>;

protected:
    IEcsSerializer() = default;
    virtual ~IEcsSerializer() = default;
    virtual void Destroy() = 0;
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_INTF_SCENESERIALIZER_H
