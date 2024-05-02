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

#ifndef SCENEPLUGIN_ECSSERIALIZER_H
#define SCENEPLUGIN_ECSSERIALIZER_H

#include <functional>

#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/property/property.h>
#include <render/intf_render_context.h>

#include <scene_plugin/interface/intf_ecs_serializer.h>

SCENE_BEGIN_NAMESPACE()

class EcsSerializer : public IEcsSerializer {
public:
    EcsSerializer(RENDER_NS::IRenderContext& renderContext);
    void Destroy() override;
    //
    // From IEcsSerializer
    //
    void SetListener(IListener* listener) override;

    void SetDefaultSerializers() override;
    void SetSerializer(const CORE_NS::PropertyTypeDecl& type, IPropertySerializer& serializer) override;

    bool WriteEntityCollection(const IEntityCollection& ec, CORE_NS::json::standalone_value& jsonOut) const override;
    bool WriteComponents(
        const IEntityCollection& ec, CORE_NS::Entity entity, CORE_NS::json::standalone_value& jsonOut) const override;
    bool WriteComponent(const IEntityCollection& ec, CORE_NS::Entity entity, const CORE_NS::IComponentManager& cm,
        CORE_NS::IComponentManager::ComponentId id, CORE_NS::json::standalone_value& jsonOut) const override;
    bool WriteProperty(const IEntityCollection& ec, const CORE_NS::Property& property, uintptr_t offset,
        CORE_NS::json::standalone_value& jsonOut) const override;

    bool GatherExternalCollections(const CORE_NS::json::value& jsonIn, BASE_NS::string_view contextUri,
        BASE_NS::vector<ExternalCollection>& externalCollectionsOut) const override;

    /*RUNTIME_NS::IIoUtil::SerializationResult*/ int ReadEntityCollection(
        IEntityCollection& ec, const CORE_NS::json::value& jsonIn, BASE_NS::string_view contextUri) const override;
    bool ReadComponents(IEntityCollection& ec, const CORE_NS::json::value& jsonIn, bool setId) const override;
    bool ReadComponent(IEntityCollection& ec, const CORE_NS::json::value& jsonIn, CORE_NS::Entity entity,
        CORE_NS::IComponentManager& component) const override;
    bool ReadProperty(IEntityCollection& ec, const CORE_NS::json::value& jsonIn, const CORE_NS::Property& property,
        uintptr_t offset) const override;

    RENDER_NS::RenderHandleReference LoadImageResource(BASE_NS::string_view uri) const override;

private:
    using PropertyToJsonFunc = std::function<bool(
        const IEntityCollection& ec, const CORE_NS::Property&, uintptr_t, CORE_NS::json::standalone_value&)>;
    using PropertyFromJsonFunc = std::function<bool(
        const IEntityCollection& ec, const CORE_NS::json::value&, const CORE_NS::Property&, uintptr_t)>;
    IPropertySerializer& Add(PropertyToJsonFunc toJson, PropertyFromJsonFunc fromJson);

    //
    // A little wrapper class to create a serializer with functions for writing and reading a value.
    //
    class SimpleJsonSerializer : public IPropertySerializer {
    public:
        bool ToJson(const IEntityCollection& ec, const CORE_NS::Property& property, uintptr_t offset,
            CORE_NS::json::standalone_value& jsonOut) const override;
        bool FromJson(const IEntityCollection& ec, const CORE_NS::json::value& jsonIn,
            const CORE_NS::Property& property, uintptr_t offset) const override;

        SimpleJsonSerializer(PropertyToJsonFunc toJson, PropertyFromJsonFunc fromJson);
        virtual ~SimpleJsonSerializer() = default;

    private:
        const PropertyToJsonFunc PropertyToJson;
        const PropertyFromJsonFunc PropertyFromJson;
    };
    BASE_NS::vector<BASE_NS::unique_ptr<SimpleJsonSerializer>> ownedSerializers_;

    RENDER_NS::IRenderContext& renderContext_;

    //
    // Mapping from each property type to a specific serializer.
    //
    using SerializerMap = BASE_NS::unordered_map<CORE_NS::PropertyTypeDecl, IPropertySerializer*>;
    SerializerMap typetoSerializerMap_;

    IListener* listener_ {};
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_ECSSERIALIZER_H
