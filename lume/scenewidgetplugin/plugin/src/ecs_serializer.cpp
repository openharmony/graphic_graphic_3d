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

// This is bit unfortunate construct, clean up once there is one agreed implementation of JSON somewhere
#include <PropertyTools/property_data.h>
#define JSON_IMPL // have template specialisation defined once
#include <algorithm>
#include <charconv>
#include <cinttypes>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <base/util/base64_decode.h>
#include <base/util/base64_encode.h>
#include <base/util/uid_util.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <core/property/intf_property_handle.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/util/intf_render_util.h>

#include "asset_loader.h"
#include "ecs_serializer.h"
#include "ecs_util.h"
#include "entity_collection.h"
#include "json.h"
#include "json_util.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

// #define VERBOSE_LOGGING
SCENE_BEGIN_NAMESPACE()

namespace {

// Helper function that makes sure that any dynamic arrays referenced in a property path are large enough to contain the
// referenced indices.
void EnsureDynamicArraySize(IPropertyHandle* propertyHandle, string_view propertyPath)
{
    const auto separatorPosition = propertyPath.find('[');
    if (separatorPosition == BASE_NS::string::npos) {
        return;
    }
    const auto separatorEndPosition = propertyPath.find(']', separatorPosition);
    if (separatorEndPosition == BASE_NS::string::npos) {
        return;
    }

    string arrayPath, arrayIndex;
    arrayPath = propertyPath.substr(0, separatorPosition);
    arrayIndex = propertyPath.substr(separatorPosition + 1, separatorEndPosition - separatorPosition - 1);

    char* end = nullptr;
    const unsigned long index = std::strtoul(arrayIndex.c_str(), &end, 10); // 10: base
    // Check that conversion stopped at the end of the string.
    if (!end || *end != '\0') {
        return;
    }

    PropertyData propertyData;
    PropertyData::PropertyOffset propertyOffset = propertyData.WLock(*propertyHandle, arrayPath);
    if (propertyOffset) {
        auto* containerMethods = propertyOffset.property->metaData.containerMethods;
        if (containerMethods && containerMethods->resize) {
            if (containerMethods->size(propertyOffset.offset) <= index) {
                containerMethods->resize(propertyOffset.offset, index + 1);
            }
        }
    }

    const auto restOfThePath = propertyPath.substr(separatorEndPosition);
    EnsureDynamicArraySize(&propertyData, restOfThePath);
}

template<typename Type>
Type& GetPropertyValue(uintptr_t ptr)
{
    return *reinterpret_cast<Type*>(ptr);
}

struct IoUtil {
    struct CompatibilityInfo {
        uint32_t versionMajor { 0 };
        uint32_t versionMinor { 0 };
        BASE_NS::string type;
    };

    struct CompatibilityRange {
        static const uint32_t IGNORE_VERSION = { ~0u };

        uint32_t versionMajorMin { IGNORE_VERSION };
        uint32_t versionMajorMax { IGNORE_VERSION };
        uint32_t versionMinorMin { IGNORE_VERSION };
        uint32_t versionMinorMax { IGNORE_VERSION };
        BASE_NS::string type {};
    };

    static bool WriteCompatibilityInfo(json::standalone_value& jsonOut, const CompatibilityInfo& info)
    {
        jsonOut["compatibility_info"] = json::standalone_value::object();
        jsonOut["compatibility_info"]["version"] =
            string(to_string(info.versionMajor) + "." + to_string(info.versionMinor));
        jsonOut["compatibility_info"]["type"] = string(info.type);
        return true;
    }

    static bool CheckCompatibility(const json::value& json, array_view<CompatibilityRange const> validVersions)
    {
        string type;
        string version;
        if (const json::value* iter = json.find("compatibility_info"); iter) {
            string parseError;
            SafeGetJsonValue(*iter, "type", parseError, type);
            SafeGetJsonValue(*iter, "version", parseError, version);

            uint32_t versionMajor { 0 };
            uint32_t versionMinor { 0 };
            if (const auto delim = version.find('.'); delim != string::npos) {
                std::from_chars(version.data(), version.data() + delim, versionMajor);
                const size_t minorStart = delim + 1;
                std::from_chars(version.data() + minorStart, version.data() + version.size(), versionMinor);
            } else {
                std::from_chars(version.data(), version.data() + version.size(), versionMajor);
            }

            for (const auto& range : validVersions) {
                if (type != range.type) {
                    continue;
                }
                if ((range.versionMajorMin != CompatibilityRange::IGNORE_VERSION) &&
                    (versionMajor < range.versionMajorMin)) {
                    continue;
                }
                if ((range.versionMajorMax != CompatibilityRange::IGNORE_VERSION) &&
                    (versionMajor > range.versionMajorMax)) {
                    continue;
                }
                if ((range.versionMinorMin != CompatibilityRange::IGNORE_VERSION) &&
                    (versionMinor < range.versionMinorMin)) {
                    continue;
                }
                if ((range.versionMinorMax != CompatibilityRange::IGNORE_VERSION) &&
                    (versionMinor > range.versionMinorMax)) {
                    continue;
                }

                // A compatible version was found from the list of valid versions.
                return true;
            }
        }

        // Not a compatible version.
        return false;
    }
};
} // namespace

//
// EcsSerializer::SimpleJsonSerializer
//
bool EcsSerializer::SimpleJsonSerializer::ToJson(
    const IEntityCollection& ec, const Property& property, uintptr_t offset, json::standalone_value& jsonOut) const
{
    return PropertyToJson(ec, property, offset, jsonOut);
}

bool EcsSerializer::SimpleJsonSerializer::FromJson(
    const IEntityCollection& ec, const json::value& jsonIn, const Property& property, uintptr_t offset) const
{
    return PropertyFromJson(ec, jsonIn, property, offset);
}

EcsSerializer::SimpleJsonSerializer::SimpleJsonSerializer(PropertyToJsonFunc toJson, PropertyFromJsonFunc fromJson)
    : PropertyToJson(toJson), PropertyFromJson(fromJson)
{}

//
// EcsSerializer
//
EcsSerializer::EcsSerializer(RENDER_NS::IRenderContext& renderContext) : renderContext_(renderContext) {}

EcsSerializer::IPropertySerializer& EcsSerializer::Add(PropertyToJsonFunc toJson, PropertyFromJsonFunc fromJson)
{
    auto serializer = new SimpleJsonSerializer(toJson, fromJson);
    auto ptr = unique_ptr<SimpleJsonSerializer> { serializer };
    ownedSerializers_.emplace_back(move(ptr));
    return *ownedSerializers_.back();
}

namespace {
template<class Type>
bool PropertyToJson(
    const IEntityCollection& /*ec*/, const Property& /*property*/, uintptr_t offset, json::standalone_value& jsonOut)
{
    jsonOut = ToJson(GetPropertyValue<Type>(offset));
    return true;
}

template<class Type>
bool PropertyFromJson(
    const IEntityCollection& /*ec*/, const json::value& jsonIn, const Property& /*property*/, uintptr_t offset)
{
    auto& value = GetPropertyValue<Type>(offset);
    return FromJson(jsonIn, value);
}

bool RenderHandleReferenceFromJson(const IEcsSerializer& ecsSerializer, IRenderContext& renderContext,
    const IEntityCollection& ec, const json::value& jsonIn, RenderHandleReference& handleRefOut)
{
    CORE_UNUSED(ec);

    if (!jsonIn.is_object()) {
        return false;
    }

    string error;
    RenderHandleDesc desc;

    // Casting to enum directly from numeric value is a bit fishy.
    uint32_t type = 0;
    if (!SafeGetJsonValue(jsonIn, "type", error, type)) {
        return false;
    }
    desc.type = static_cast<RenderHandleType>(type);

    if (!SafeGetJsonValue(jsonIn, "name", error, desc.name)) {
        return false;
    }

    SafeGetJsonValue(jsonIn, "additionalName", error, desc.additionalName);

    auto& renderUtil = renderContext.GetRenderUtil();
    handleRefOut = renderUtil.GetRenderHandle(desc);

    if (!handleRefOut && desc.type == RenderHandleType::GPU_IMAGE && !desc.name.empty()) {
        // Special handling for images: Load the image if it was not already loaded.
        // Note: assuming that the name is the image uri.
        handleRefOut = ecsSerializer.LoadImageResource(desc.name);
    }

    return true;
}

bool RenderHandleReferenceToJson(IRenderContext& renderContext, const IEntityCollection& ec,
    const RenderHandleReference& handleRefIn, json::standalone_value& jsonOut)
{
    CORE_UNUSED(ec);

    auto& renderUtil = renderContext.GetRenderUtil();
    const auto desc = renderUtil.GetRenderHandleDesc(handleRefIn);

    jsonOut = json::standalone_value::object {};
    jsonOut["type"] = static_cast<unsigned int>(desc.type);
    jsonOut["name"] = string(desc.name);
    if (!desc.additionalName.empty()) {
        jsonOut["additionalName"] = string(desc.additionalName);
    }

    return true;
}

bool EntityFromJson(const IEntityCollection& ec, const json::value& jsonIn, Entity& entityOut)
{
    // Figure out to what entity id a piece of json refers to and handle error cases.
    Entity entity {};
    if (jsonIn.is_unsigned_int()) {
        const auto entityReference = static_cast<uint32_t>(jsonIn.unsigned_);
        entity = ec.GetEntity(entityReference);
        if (entity == Entity {}) {
            CORE_LOG_W("Component entity not found for index: %u", entityReference);
            return false;
        }
    } else if (jsonIn.is_string()) {
        string entityReferenceString;
        if (FromJson(jsonIn, entityReferenceString)) {
            entity = ec.GetEntity(entityReferenceString);
        }
        if (entity == Entity {}) {
            CORE_LOG_W("Component entity not found for id: '%s'", entityReferenceString.c_str());
            return false;
        }
    } else if (jsonIn.is_object()) {
        // Empty object means "null ref".
        if (jsonIn.empty()) {
            entityOut = Entity {};
            return true;
        }

        // Figure out the correct collection (Recursive).
        const IEntityCollection* collection { nullptr };
        const auto* collectionJson = jsonIn.find("collection");
        if (collectionJson) {
            if (collectionJson->is_string()) {
                string collectionName;
                if (FromJson(*collectionJson, collectionName)) {
                    if (auto index = ec.GetSubCollectionIndex(collectionName); index >= 0) {
                        collection = ec.GetSubCollection(static_cast<size_t>(index));
                    }
                }
                if (!collection) {
                    CORE_LOG_W("Collection not found: '%s'", collectionName.c_str());
                    return false;
                }

            } else if (collectionJson->is_unsigned_int()) {
                const auto collectionIndex = collectionJson->unsigned_;
                if (collectionIndex < ec.GetSubCollectionCount()) {
                    collection = ec.GetSubCollection(collectionIndex);
                } else {
                    CORE_LOG_W("Collection not found: %" PRIu64, collectionIndex);
                    return false;
                }

            } else {
                CORE_LOG_W("Invalid collection for a component.");
                return false;
            }

            if (collection) {
                const auto* entityJson = jsonIn.find("entity");
                if (entityJson) {
                    return EntityFromJson(*collection, *entityJson, entityOut);
                }
            }
            return false;
        }

    } else {
        CORE_LOG_W("Component entity property must be an index to the entities array, an string id, or an object");
        return false;
    }

    entityOut = entity;
    return true;
}

bool EntityToJson(const IEntityCollection& ec, const Entity& entityIn, json::standalone_value& jsonOut)
{
    // Write entity index/name if part of this collection.
    const auto entityCount = ec.GetEntityCount();
    for (size_t i = 0; i < entityCount; ++i) {
        if (entityIn == ec.GetEntity(i)) {
            auto id = ec.GetId(entityIn);
            if (!id.empty()) {
                jsonOut = string(id);
            } else {
                jsonOut = i;
            }
            return true;
        }
    }

    // Otherwise check sub-collections recursively.
    const auto collectionCount = ec.GetSubCollectionCount();
    size_t collectionId = 0;
    for (size_t i = 0; i < collectionCount; ++i) {
        auto* collection = ec.GetSubCollection(i);
        BASE_ASSERT(collection);
        // NOTE: Skipping over destroyed collections (same needs to be done when writing the actual collections).
        if (collection->IsMarkedDestroyed() || !collection->IsSerialized()) {
            continue;
        }
        json::standalone_value entityJson;
        if (EntityToJson(*collection, entityIn, entityJson)) {
            jsonOut = json::standalone_value::object {};
            jsonOut[string_view { "collection" }] = collectionId;
            jsonOut[string_view { "entity" }] = move(entityJson);
            return true;
        }
        collectionId++;
    }

    return false;
}

bool EntityReferenceToJson(IRenderContext& renderContext, const IEntityCollection& ec, const EntityReference& entityIn,
    json::standalone_value& jsonOut)
{
    if (EntityToJson(ec, entityIn, jsonOut)) {
        return true;
    }

    // Write render handle reference as render handle desc.
    auto* rhm = GetManager<IRenderHandleComponentManager>(ec.GetEcs());
    if (rhm) {
        if (auto handle = rhm->Read(entityIn); handle) {
            json::standalone_value renderHandleJson = json::standalone_value::object {};
            if (RenderHandleReferenceToJson(renderContext, ec, handle->reference, renderHandleJson["renderHandle"])) {
                jsonOut = move(renderHandleJson);
                return true;
            }
        }
    }

    return false;
}

bool EntityReferenceFromJson(const IEcsSerializer& ecsSerializer, IRenderContext& renderContext,
    const IEntityCollection& ec, const json::value& jsonIn, EntityReference& entityOut)
{
    // A generic handler for any uri to a render handle.
    const auto* renderHandleJson = jsonIn.find("renderHandle");
    if (renderHandleJson) {
        // Find a shader render handle by desc.
        RenderHandleReference renderHandle;
        if (RenderHandleReferenceFromJson(ecsSerializer, renderContext, ec, *renderHandleJson, renderHandle)) {
            auto* rhm = GetManager<IRenderHandleComponentManager>(ec.GetEcs());
            if (rhm && renderHandle) {
                entityOut = GetOrCreateEntityReference(ec.GetEcs().GetEntityManager(), *rhm, renderHandle);
                return true;
            }
        }
    }

    Entity temp;
    if (!EntityFromJson(ec, jsonIn, temp)) {
        return false;
    }
    entityOut = ec.GetEcs().GetEntityManager().GetReferenceCounted(temp);
    return true;
}
} // namespace

void EcsSerializer::SetDefaultSerializers()
{
    //
    // Basic types (for types that nlohman knows how to serialize).
    //

    SetSerializer(PropertyType::FLOAT_T, Add(PropertyToJson<float>, PropertyFromJson<float>));
    SetSerializer(PropertyType::DOUBLE_T, Add(PropertyToJson<double>, PropertyFromJson<double>));

    SetSerializer(PropertyType::UINT8_T, Add(PropertyToJson<uint8_t>, PropertyFromJson<uint8_t>));
    SetSerializer(PropertyType::UINT16_T, Add(PropertyToJson<uint16_t>, PropertyFromJson<uint16_t>));
    SetSerializer(PropertyType::UINT32_T, Add(PropertyToJson<uint32_t>, PropertyFromJson<uint32_t>));
    SetSerializer(PropertyType::UINT64_T, Add(PropertyToJson<uint64_t>, PropertyFromJson<uint64_t>));

    SetSerializer(PropertyType::INT8_T, Add(PropertyToJson<int8_t>, PropertyFromJson<int8_t>));
    SetSerializer(PropertyType::INT16_T, Add(PropertyToJson<int16_t>, PropertyFromJson<int16_t>));
    SetSerializer(PropertyType::INT32_T, Add(PropertyToJson<int32_t>, PropertyFromJson<int32_t>));
    SetSerializer(PropertyType::INT64_T, Add(PropertyToJson<int64_t>, PropertyFromJson<int64_t>));

    SetSerializer(PropertyType::VEC2_T, Add(PropertyToJson<Math::Vec2>, PropertyFromJson<Math::Vec2>));
    SetSerializer(PropertyType::VEC3_T, Add(PropertyToJson<Math::Vec3>, PropertyFromJson<Math::Vec3>));
    SetSerializer(PropertyType::VEC4_T, Add(PropertyToJson<Math::Vec4>, PropertyFromJson<Math::Vec4>));

    SetSerializer(PropertyType::UVEC2_T, Add(PropertyToJson<Math::UVec2>, PropertyFromJson<Math::UVec2>));
    SetSerializer(PropertyType::UVEC3_T, Add(PropertyToJson<Math::UVec3>, PropertyFromJson<Math::UVec3>));
    SetSerializer(PropertyType::UVEC4_T, Add(PropertyToJson<Math::UVec4>, PropertyFromJson<Math::UVec4>));

    SetSerializer(PropertyType::BOOL_T, Add(PropertyToJson<bool>, PropertyFromJson<bool>));
    SetSerializer(PropertyType::QUAT_T, Add(PropertyToJson<Math::Quat>, PropertyFromJson<Math::Quat>));
    SetSerializer(PropertyType::UID_T, Add(PropertyToJson<Uid>, PropertyFromJson<Uid>));

    SetSerializer(PropertyType::STRING_T, Add(PropertyToJson<string>, PropertyFromJson<string>));

    //
    // Others
    //

    SetSerializer(PropertyType::CHAR_ARRAY_T,
        Add(
            [](const IEntityCollection& /*ec*/, const Property& property, uintptr_t offset,
                json::standalone_value& jsonOut) {
                const auto* value = &GetPropertyValue<char>(offset);
                // NOTE: a hacky way to calculate cstring size.
                string_view view(value);
                const size_t size = view.size() < property.size ? view.size() : property.size;
                jsonOut = ToJson(string(value, size));
                return true;
            },
            [](const IEntityCollection& /*ec*/, const json::value& jsonIn, const Property& property, uintptr_t offset) {
                string value;
                if (FromJson(jsonIn, value)) {
                    char* charArray = &GetPropertyValue<char>(offset);
                    charArray[value.copy(charArray, property.size - 1)] = '\0';
                    return true;
                }
                return false;
            }));

    SetSerializer(PropertyType::ENTITY_T,
        Add(
            [](const IEntityCollection& ec, const Property& /*property*/, uintptr_t offset,
                json::standalone_value& jsonOut) {
                return EntityToJson(ec, GetPropertyValue<const Entity>(offset), jsonOut);
            },
            [](const IEntityCollection& ec, const json::value& jsonIn, const Property& /*property*/, uintptr_t offset) {
                return EntityFromJson(ec, jsonIn, GetPropertyValue<Entity>(offset));
            }));

    SetSerializer(PropertyType::ENTITY_REFERENCE_T,
        Add(
            [this](const IEntityCollection& ec, const Property& /*property*/, uintptr_t offset,
                json::standalone_value& jsonOut) {
                return EntityReferenceToJson(
                    renderContext_, ec, GetPropertyValue<const EntityReference>(offset), jsonOut);
            },
            [this](const IEntityCollection& ec, const json::value& jsonIn, const Property& /*property*/,
                uintptr_t offset) {
                return EntityReferenceFromJson(
                    *this, renderContext_, ec, jsonIn, GetPropertyValue<EntityReference>(offset));
            }));

    SetSerializer(PROPERTYTYPE(RenderHandleReference),
        Add(
            [this](const IEntityCollection& ec, const Property& /*property*/, uintptr_t offset,
                json::standalone_value& jsonOut) {
                return RenderHandleReferenceToJson(
                    renderContext_, ec, GetPropertyValue<const RenderHandleReference>(offset), jsonOut);
            },
            [this](const IEntityCollection& ec, const json::value& jsonIn, const Property& /*property*/,
                uintptr_t offset) {
                return RenderHandleReferenceFromJson(
                    *this, renderContext_, ec, jsonIn, GetPropertyValue<RenderHandleReference>(offset));
            }));
}

void EcsSerializer::SetListener(IListener* listener)
{
    listener_ = listener;
}

void EcsSerializer::SetSerializer(const PropertyTypeDecl& type, IPropertySerializer& serializer)
{
    typetoSerializerMap_[type] = &serializer;
}

bool EcsSerializer::WriteEntityCollection(const IEntityCollection& ec, json::standalone_value& jsonOut) const
{
    // NOTE: We make sure collections and entities are written in the output before entity-components to make it
    // possible to parse the file in one go.

    jsonOut = json::standalone_value::object();

    string type = ec.GetType();
    if (type.empty()) {
        type = "entitycollection";
    }

    const IoUtil::CompatibilityInfo info { VERSION_MAJOR, VERSION_MINOR, type };
    if (!IoUtil::WriteCompatibilityInfo(jsonOut, info)) {
        return false;
    }

    if (string rootSrc = ec.GetSrc(); !rootSrc.empty()) {
        jsonOut["src"] = move(rootSrc);
    }

    // Write external collections instanced in this collection.
    const auto collectionCount = ec.GetSubCollectionCount();
    if (collectionCount > 0) {
        json::standalone_value collectionsJson = json::standalone_value::array();
        collectionsJson.array_.reserve(collectionCount);

        for (size_t i = 0; i < collectionCount; ++i) {
            json::standalone_value collectionJson = json::standalone_value::object();
            auto* collection = ec.GetSubCollection(i);

            if (!collection || collection->IsMarkedDestroyed() || !collection->IsSerialized()) {
                // NOTE: Skipping over destroyed collections (same needs to be done when writing entity references).
                continue;
            }

            if (string src = collection->GetSrc(); !src.empty()) {
                collectionJson["src"] = move(src);
            }
            if (string id = collection->GetUri(); !id.empty()) {
                collectionJson["id"] = move(id);
            }

            collectionsJson.array_.emplace_back(move(collectionJson));
        }

        if (!collectionsJson.array_.empty()) {
            jsonOut["collections"] = move(collectionsJson);
        }
    }

    // Write bare entities without components.
    const auto entityCount = ec.GetEntityCount();
    if (entityCount > 0) {
        auto& entitiesJson = (jsonOut["entities"] = json::standalone_value::array());
        entitiesJson.array_.reserve(entityCount);
        for (size_t i = 0; i < entityCount; ++i) {
            json::standalone_value entityJson = json::standalone_value::object();

            // Write id if one is defined.
            auto entity = ec.GetEntity(i);
            const auto& id = ec.GetId(entity);
            if (!id.empty()) {
                entityJson["id"] = string(id);
            }

            entitiesJson.array_.emplace_back(move(entityJson));
        }
    }

    vector<EntityReference> allEntities;
    ec.GetEntitiesRecursive(false, allEntities, false);
    if (allEntities.size() > 0) {
        auto& entityComponentsJson = (jsonOut["entity-components"] = json::standalone_value::array());
        for (Entity entity : allEntities) {
            json::standalone_value componentJson = json::standalone_value::object();
            if (WriteComponents(ec, entity, componentJson["components"])) {
                json::standalone_value entityRefJson;
                if (EntityToJson(ec, entity, entityRefJson)) {
                    componentJson["entity"] = move(entityRefJson);
                }
                entityComponentsJson.array_.emplace_back(move(componentJson));
            }
        }
    }

    // NOTE: Always returns true even if parts of the writing failed.
    return true;
}

bool EcsSerializer::WriteComponents(const IEntityCollection& ec, Entity entity, json::standalone_value& jsonOut) const
{
    // Write all entity components to json (Sorting by uid to keep the order more stable).
    vector<string> managerUids;
    jsonOut = json::standalone_value::object();
    auto cms = ec.GetEcs().GetComponentManagers();
    for (auto cm : cms) {
        const auto componentId = cm->GetComponentId(entity);
        if (componentId != IComponentManager::INVALID_COMPONENT_ID) {
            auto uidString = to_string(cm->GetUid());
            managerUids.emplace_back(string(uidString));
        }
    }
    std::sort(managerUids.begin(), managerUids.end());

    for (auto& uidString : managerUids) {
        const auto* cm = ec.GetEcs().GetComponentManager(StringToUid(uidString));
        const auto componentId = cm->GetComponentId(entity);

        json::standalone_value componentJson = json::standalone_value::object();
        if (WriteComponent(ec, entity, *cm, componentId, componentJson)) {
            jsonOut[uidString] = move(componentJson);
        }
    }

    return (!jsonOut.empty());
}

bool EcsSerializer::WriteComponent(const IEntityCollection& ec, Entity entity, const IComponentManager& cm,
    IComponentManager::ComponentId id, json::standalone_value& jsonOut) const
{
    // Write all properties to json.
    json::standalone_value propertiesJson = json::standalone_value::object();
    const auto* props = ec.GetSerializedProperties(entity, cm.GetUid());
    if (props) {
        const auto* propertyHandle = cm.GetData(id);
        if (!propertyHandle) {
            return false;
        }
        for (auto& propertyPath : *props) {
            const IPropertyHandle* handle = propertyHandle;

            PropertyData propertyData;
            PropertyData::PropertyOffset propertyOffset;

            // Check if this is property container.
            string path, name;
            auto containerHandle = ResolveContainerProperty(*handle, propertyPath, path, name);
            if (containerHandle) {
                propertyOffset = propertyData.RLock(*containerHandle, name);
            } else {
                propertyOffset = propertyData.RLock(*propertyHandle, propertyPath);
            }

            if (propertyOffset) {
                if ((propertyOffset.property->flags & static_cast<uint32_t>(PropertyFlags::NO_SERIALIZE)) != 0) {
                    continue;
                }

                json::standalone_value propertyJson;
                if (WriteProperty(ec, *propertyOffset.property, propertyOffset.offset, propertyJson)) {
                    if (!propertyJson.is_null()) {
                        propertiesJson[propertyPath] = move(propertyJson);
                    }
                }
            }
        }

        // NOTE: optional name to make reading files easier.
        jsonOut["name"] = string(cm.GetName());

        if (!propertiesJson.empty()) {
            jsonOut["properties"] = move(propertiesJson);
        }

        // NOTE: Maybe return false if any property write fails?
        return true;
    }
    return false;
}

bool EcsSerializer::WriteProperty(
    const IEntityCollection& ec, const Property& property, uintptr_t offset, json::standalone_value& jsonOut) const
{
    auto serializer = typetoSerializerMap_.find(property.type);
    if (serializer != typetoSerializerMap_.end()) {
        return serializer->second->ToJson(ec, property, offset, jsonOut);

    } else if (!property.metaData.enumMetaData.empty()) {
        // Enum type property.
        switch (property.size) {
            case sizeof(uint8_t):
                jsonOut = GetPropertyValue<uint8_t>(offset);
                return true;
            case sizeof(uint16_t):
                jsonOut = GetPropertyValue<uint16_t>(offset);
                return true;
            case sizeof(uint32_t):
                jsonOut = GetPropertyValue<uint32_t>(offset);
                return true;
            case sizeof(uint64_t):
                jsonOut = GetPropertyValue<uint64_t>(offset);
                return true;
        }

    } else if (property.metaData.containerMethods) {
        const auto& container = *property.metaData.containerMethods;

        // Container type property.
        if (property.type.isArray) {
            // Special handling for byte arrays. Save as a base64 encoded string.
            if (property.type == PropertyType::UINT8_ARRAY_T) {
                array_view<const uint8_t> bytes { (const uint8_t*)offset, property.size };
                jsonOut = BASE_NS::Base64Encode(bytes);
                return true;
            }

            // C style array.
            json::standalone_value array = json::standalone_value::array();
            array.array_.reserve(property.count);
            for (size_t i = 0; i < property.count; i++) {
                uintptr_t ptr = offset + i * container.property.size;
                // TODO: return false if any recurseive call fails?
                json::standalone_value elementJson;
                WriteProperty(ec, container.property, ptr, elementJson);
                array.array_.push_back(move(elementJson));
            }
            jsonOut = move(array);
            return true;

        } else {
            // This is a "non trivial container".
            const auto count = container.size(offset);

            // Special handling for byte arrays. Save as a base64 encoded string.
            // NOTE: Only specifically vector so we can assume that the memory usage is linear.
            if (property.type == PROPERTYTYPE(vector<uint8_t>)) {
                if (count == 0) {
                    jsonOut = "";
                } else {
                    auto data = container.get(offset, 0);
                    array_view<const uint8_t> bytes { reinterpret_cast<const uint8_t*>(data), count };
                    jsonOut = BASE_NS::Base64Encode(bytes);
                }
                return true;
            }

            json::standalone_value array = json::standalone_value::array();
            array.array_.reserve(count);

            for (size_t i = 0; i < count; i++) {
                uintptr_t ptr = container.get(offset, i);
                // TODO: return false if any recurseive call fails?
                json::standalone_value elementJson;
                WriteProperty(ec, container.property, ptr, elementJson);
                array.array_.push_back(move(elementJson));
            }
            jsonOut = move(array);
            return true;
        }

    } else if (!property.metaData.memberProperties.empty()) {
        // Struct type property (ie. has sub properties).
        json::standalone_value object = json::standalone_value::object();
        for (const auto& subProperty : property.metaData.memberProperties) {
            json::standalone_value subPropertyJson;
            if (WriteProperty(ec, subProperty, offset + subProperty.offset, subPropertyJson)) {
                object[subProperty.name] = move(subPropertyJson);
            }
        }
        // TODO: return false if any recurseive call fails?
        jsonOut = move(object);
        return true;

    } else {
        CORE_LOG_V("Ecs serializer not found for '%s'", string(property.type.name).c_str());
    }
    return false;
}

bool EcsSerializer::GatherExternalCollections(
    const json::value& jsonIn, string_view contextUri, vector<ExternalCollection>& externalCollectionsOut) const
{
    const auto* srcJson = jsonIn.find("src");
    string srcUri;
    if (srcJson && FromJson(*srcJson, srcUri)) {
#ifdef VERBOSE_LOGGING
        SCENE_PLUGIN_VERBOSE_LOG(
            "External Collection: uri='%s' context='%s'", srcUri.c_str(), string(contextUri).c_str());
#endif
        externalCollectionsOut.emplace_back(ExternalCollection { srcUri, string { contextUri } });
    }

    const auto* collectionsJson = jsonIn.find("collections");
    if (collectionsJson && collectionsJson->is_array()) {
        for (const auto& collectionJson : collectionsJson->array_) {
            if (collectionJson.is_object()) {
                const auto* collectionSrcJson = collectionJson.find("src");
                if (collectionSrcJson && collectionSrcJson->is_string()) {
                    string collectionSrcUri;
                    FromJson(*collectionSrcJson, collectionSrcUri);
                    externalCollectionsOut.emplace_back(ExternalCollection { collectionSrcUri, string { contextUri } });
                }
            }
        }
    }
    return true;
}

/*IIoUtil::SerializationResult*/ int EcsSerializer::ReadEntityCollection(
    IEntityCollection& ec, const json::value& jsonIn, string_view contextUri) const
{
    // TODO: Move version check to be separately so it can be done before gathering the dependencies.
    // NOTE: Only comparing the major version.
    const auto minor = IoUtil::CompatibilityRange::IGNORE_VERSION;
    // TODO: Type name was changed to be in line with engine naming. Allow the old type name for a while.
    const IoUtil::CompatibilityRange validVersions[] {
        { VERSION_MAJOR, VERSION_MAJOR, minor, minor, ec.GetType() },
        { VERSION_MAJOR, VERSION_MAJOR, minor, minor, "entity_collection" },
    };

    auto result = IoUtil::CheckCompatibility(jsonIn, validVersions);
    if (!result) {
        CORE_LOG_W("Deprecated compatibility info found \"entity_collection\": '%s'", string(contextUri).c_str());
    }

    const auto* srcJson = jsonIn.find("src");
    string srcUri;
    if (srcJson && FromJson(*srcJson, srcUri)) {
#ifdef VERBOSE_LOGGING
        SCENE_PLUGIN_VERBOSE_LOG(
            "External Collection: uri='%s' context='%s'", srcUri.c_str(), string(contextUri).c_str());
#endif
        auto* externalCollection = listener_->GetExternalCollection(ec.GetEcs(), srcUri, contextUri);
        if (externalCollection) {
            ec.CopyContents(*externalCollection);
        }
    }

    const auto* collectionsJson = jsonIn.find("collections");
    if (collectionsJson && collectionsJson->is_array()) {
        for (const auto& collectionJson : collectionsJson->array_) {
            if (collectionJson.is_object()) {
                string id;
                const auto* idJson = collectionJson.find("id");
                if (idJson) {
                    FromJson(*idJson, id);
                }

                const auto* collectionSrcJson = collectionJson.find("src");
                if (collectionSrcJson && collectionSrcJson->is_string()) {
                    string collectionSrcUri;
                    FromJson(*collectionSrcJson, collectionSrcUri);

                    // Instantiate the collection pointed by src.
#ifdef VERBOSE_LOGGING
                    SCENE_PLUGIN_VERBOSE_LOG("External Collection: uri='%s' context='%s'", collectionSrcUri.c_str(),
                        string(contextUri).c_str());
#endif
                    auto* externalCollection =
                        listener_->GetExternalCollection(ec.GetEcs(), collectionSrcUri, contextUri);
                    if (externalCollection) {
                        ec.AddSubCollectionClone(*externalCollection, id).SetSrc(collectionSrcUri);
                        ;
                    } else {
                        CORE_LOG_E("Loading collection failed: '%s'", collectionSrcUri.c_str());
                        // Just adding an empty collection as a placeholder.
                        ec.AddSubCollection(id, collectionSrcUri).SetSrc(collectionSrcUri);
                    }
                } else {
                    ec.AddSubCollection(id, {});
                }
            }
        }
    }

    const auto* entitiesJson = jsonIn.find("entities");
    bool idSet { false };
    if (entitiesJson && entitiesJson->is_array()) {
        auto& em = ec.GetEcs().GetEntityManager();

        // First create all entities so they can be referenced by components.
        for (const auto& entityJson : entitiesJson->array_) {
            // Create a new entity.
            EntityReference entity = em.CreateReferenceCounted();
            ec.AddEntity(entity);

            // Add with an id if one is defined.
            const auto* idJson = entityJson.find("id");
            string id;
            if (idJson && FromJson(*idJson, id)) {
                ec.SetId(id, entity);
                idSet = true;
            }
        }
    }

    const auto* ecJson = jsonIn.find("entity-components");
    if (ecJson && ecJson->is_array()) {
        // Then load entity contents (i.e. components).
        for (size_t i = 0; i < ecJson->array_.size(); ++i) {
            ReadComponents(ec, ecJson->array_.at(i), !idSet);
        }
    }

    if (!ec.IsActive()) {
        // If the ec is not active also make the newly loaded entities not active.
        ec.SetActive(false);
    }

    // NOTE: Always returns success, even if parts of the load failed.
    return 1; // result;
}

bool EcsSerializer::ReadComponents(IEntityCollection& ec, const json::value& jsonIn, bool setId) const
{
    // Figure out to which entity these components belong to.
    const auto* entityJson = jsonIn.find("entity");
    if (!entityJson) {
        CORE_LOG_W("No entity defined for a component.");
        return false;
    }

    Entity entity {};

    if (!EntityFromJson(ec, *entityJson, entity)) {
        return false;
    }

    auto& ecs = ec.GetEcs();
    const auto* componentsJson = jsonIn.find("components");
    if (componentsJson) {
        // Read all entity components from json.
        for (auto& component : componentsJson->object_) {
            auto& key = component.key;
            const auto componentUid = StringToUid(key);

            auto& componentJson = component.value;
            auto* cm = ecs.GetComponentManager(componentUid);
            if (cm) {
                ReadComponent(ec, componentJson, entity, *cm);
                if (setId && cm->GetName() == "NameComponent") {
                    if (auto handle = reinterpret_cast<CORE3D_NS::INameComponentManager*>(cm)->Read(entity)) {
                        for (auto& ref : ec.GetEntities()) {
                            if (ref == entity) {
                                BASE_NS::string compound = handle->name;
                                compound.append(":");
                                compound.append(BASE_NS::to_hex(entity.id));

                                ec.SetUniqueIdentifier(compound, ref);
                                break;
                            }
                        }
                    }
                }
            } else {
                // TODO: Maybe we should try to find a matching component by name as a fallback
                CORE_LOG_W("Unrecognized component found: '%s'", string(key).c_str());
            }
        }
    }

    return true;
}

bool EcsSerializer::ReadComponent(
    IEntityCollection& ec, const json::value& jsonIn, Entity entity, IComponentManager& component) const
{
    // Create the component if it does not exist yet.
    auto componentId = component.GetComponentId(entity);
    if (componentId == IComponentManager::INVALID_COMPONENT_ID) {
        component.Create(entity);
        componentId = component.GetComponentId(entity);
    }

    ec.MarkComponentSerialized(entity, component.GetUid(), true);

    const auto* propertiesJson = jsonIn.find("properties");
    if (!propertiesJson || propertiesJson->type != json::type::object) {
        // No properties.
        return true;
    }

    auto* propertyHandle = component.GetData(componentId);
    if (!propertyHandle) {
        return false;
    }

    for (auto& propertyJson : propertiesJson->object_) {
        const auto& propertyPath = propertyJson.key;
        const auto& propertyValueJson = propertyJson.value;
        auto pathView = string_view(propertyPath.data(), propertyPath.size());

        // Find the property using the propertyName
        {
            const IPropertyHandle* handle = propertyHandle;
            PropertyData propertyData;
            PropertyData::PropertyOffset propertyOffset;

            // Check if this is property container.
            string path, name;
            auto containerHandle = ResolveContainerProperty(*handle, string(pathView), path, name);
            if (containerHandle) {
                propertyOffset = propertyData.WLock(*containerHandle, name);
            } else {
                // We can only ask for the property if we first make sure it exists (we may be referencing a dynamic
                // array).
                EnsureDynamicArraySize(propertyHandle, pathView);

                propertyOffset = propertyData.WLock(*propertyHandle, pathView);
            }

            if (propertyOffset) {
                if (ReadProperty(ec, propertyValueJson, *propertyOffset.property, propertyOffset.offset)) {
                    // Mark this property value as serialized (instead of being a cloned from a prototype entity).
                    ec.MarkPropertySerialized(entity, component.GetUid(), pathView, true);
                } else {
                    CORE_LOG_W("Unrecognized property: Component: '%s' Property: '%s'", component.GetName().data(),
                        string(pathView).c_str());
                }
            }
        }
    }
    return true;
}

bool EcsSerializer::ReadProperty(
    IEntityCollection& ec, const json::value& jsonIn, const Property& property, uintptr_t offset) const
{
    // See if there is a serializer for this type. Otherwise recurse further.
    auto serializer = typetoSerializerMap_.find(property.type);
    if (serializer != typetoSerializerMap_.end()) {
        return serializer->second->FromJson(ec, jsonIn, property, offset);

    } else if (!property.metaData.enumMetaData.empty()) {
        // Enum type property.
        if (jsonIn.is_unsigned_int()) {
            switch (property.size) {
                case sizeof(uint8_t):
                    GetPropertyValue<uint8_t>(offset) = static_cast<uint8_t>(jsonIn.unsigned_);
                    return true;
                case sizeof(uint16_t):
                    GetPropertyValue<uint16_t>(offset) = static_cast<uint16_t>(jsonIn.unsigned_);
                    return true;
                case sizeof(uint32_t):
                    GetPropertyValue<uint32_t>(offset) = static_cast<uint32_t>(jsonIn.unsigned_);
                    return true;
                case sizeof(uint64_t):
                    GetPropertyValue<uint64_t>(offset) = static_cast<uint64_t>(jsonIn.unsigned_);
                    return true;
            }
        }

    } else if (property.metaData.containerMethods) {
        // Special handling for byte data encoded as base64.
        if (jsonIn.is_string()) {
            // A base64 encoded string containing raw array data
            auto bytes = BASE_NS::Base64Decode(jsonIn.string_);

            // Only valid for byte data.
            if (property.type == PropertyType::UINT8_ARRAY_T) {
                if (property.size != bytes.size()) {
                    CORE_LOG_W("Invalid base64 data size in: %s", string(property.name).c_str());
                    return false;
                }
                CloneData(GetPropertyValue<uint8_t*>(offset), property.size, &bytes[0], bytes.size());
                return true;

            } else if (property.type == PROPERTYTYPE(vector<uint8_t>)) {
                GetPropertyValue<vector<uint8_t>>(offset).swap(bytes);
                return true;
            }
            return false;
        }

        // Container type property.
        if (property.type.isArray) {
            // C style array.
            if (jsonIn.is_array()) {
                if (jsonIn.array_.size() != property.count) {
                    CORE_LOG_W("Expecting a json array of size %zu", property.count);
                    return false;
                }
                for (size_t i = 0; i < property.count; i++) {
                    uintptr_t ptr = offset + i * property.metaData.containerMethods->property.size;
                    // TODO: return false if any recurseive call fails?
                    ReadProperty(ec, jsonIn.array_.at(i), property.metaData.containerMethods->property, ptr);
                }
                return true;

            } else if (jsonIn.is_object()) {
                // Allow "sparse arrays" by using objects with the array index as the key.
                for (auto& element : jsonIn.object_) {
                    const auto& key = element.key;
                    const auto index = static_cast<size_t>(strtol(string(key).c_str(), nullptr, 10));
                    if ((index == 0 && key != "0") || index >= property.count) {
                        CORE_LOG_W("Invalid array Index: %s", string(key).c_str());
                        continue;
                    }
                    uintptr_t ptr = offset + index * property.metaData.containerMethods->property.size;
                    ReadProperty(ec, element.value, property.metaData.containerMethods->property, ptr);
                }
                return true;
            }
            return false;

        } else {
            // This is a "non trivial container".
            if (jsonIn.is_array()) {
                const auto count = jsonIn.array_.size();
                property.metaData.containerMethods->resize(offset, count);
                for (size_t i = 0; i < count; i++) {
                    uintptr_t ptr = property.metaData.containerMethods->get(offset, i);
                    ReadProperty(ec, jsonIn.array_.at(i), property.metaData.containerMethods->property, ptr);
                }
                return true;

            } else if (jsonIn.is_object()) {
                // Allow "sparse arrays" by using objects with the array index as the key.
                for (auto& element : jsonIn.object_) {
                    const auto& key = element.key;
                    const auto index = static_cast<size_t>(strtol(string(key).c_str(), nullptr, 10));
                    if ((index == 0 && key != "0")) {
                        CORE_LOG_W("Invalid array Index: %s", string(key).c_str());
                        continue;
                    }

                    const auto count = property.metaData.containerMethods->size(offset);
                    if (count <= index) {
                        property.metaData.containerMethods->resize(offset, index + 1);
                    }
                    uintptr_t ptr = property.metaData.containerMethods->get(offset, index);
                    ReadProperty(ec, element.value, property.metaData.containerMethods->property, ptr);
                }
                return true;
            }
            return false;
        }

    } else if (!property.metaData.memberProperties.empty()) {
        // Struct type property (ie. has sub properties).
        if (jsonIn.is_object()) {
            for (const auto& subProperty : property.metaData.memberProperties) {
                // TODO: is there a way to not create the string
                const string name(subProperty.name);
                const auto* subJson = jsonIn.find(name);
                if (subJson) {
                    ReadProperty(ec, *subJson, subProperty, offset + subProperty.offset);
                }
            }
            // TODO: return false if any recurseive call fails?
            return true;
        }
    }

    return false;
}

RENDER_NS::RenderHandleReference EcsSerializer::LoadImageResource(BASE_NS::string_view uri) const
{
    // Get rid of a possible query string in the uri.
    const auto fileUri = PathUtil::ResolvePath({}, uri, false);

    auto params = PathUtil::GetUriParameters(uri);

    // Image loading flags can be passed in the uri query string.
    uint64_t imageLoaderFlags {};
    auto loaderFlags = params.find("loaderFlags");
    if (loaderFlags != params.end()) {
        const char* start = loaderFlags->second.data();
        const char* end = start + loaderFlags->second.size();
        std::from_chars(start, end, imageLoaderFlags);
    }

    auto imageLoadResult = renderContext_.GetEngine().GetImageLoaderManager().LoadImage(fileUri, imageLoaderFlags);

    RenderHandleReference imageHandle {};
    if (!imageLoadResult.success) {
        CORE_LOG_E("Could not load image asset: %s", imageLoadResult.error);

    } else {
        auto& gpuResourceMgr = renderContext_.GetDevice().GetGpuResourceManager();
        GpuImageDesc gpuDesc = gpuResourceMgr.CreateGpuImageDesc(imageLoadResult.image->GetImageDesc());
        gpuDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (gpuDesc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS) {
            gpuDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        gpuDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        imageHandle = gpuResourceMgr.Create(uri, gpuDesc, std::move(imageLoadResult.image));
    }

    return imageHandle;
}

void EcsSerializer::Destroy()
{
    delete this;
}

SCENE_END_NAMESPACE()
