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

#include "system_graph_loader.h"

#include <charconv>
#include <cinttypes>
#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_system.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property.h>
#include <core/property/property_types.h>

#define JSON_IMPL
#include <core/json/json.h>

#include "json_util.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::Math::Quat;
using BASE_NS::Math::Vec2;
using BASE_NS::Math::Vec3;
using BASE_NS::Math::Vec4;

namespace {
constexpr size_t VERSION_SIZE { 5u };
constexpr uint32_t VERSION_MAJOR { 22u };

template<class TYPEINFO>
const TYPEINFO* FindTypeInfo(const string_view name, const array_view<const ITypeInfo* const>& container)
{
    for (const auto& info : container) {
        if (static_cast<const TYPEINFO*>(info)->typeName == name) {
            return static_cast<const TYPEINFO*>(info);
        }
    }

    return nullptr;
}

template<class TYPEINFO>
const TYPEINFO* FindTypeInfo(const Uid& uid, const array_view<const ITypeInfo* const>& container)
{
    for (const auto& info : container) {
        if (static_cast<const TYPEINFO* const>(info)->uid == uid) {
            return static_cast<const TYPEINFO*>(info);
        }
    }

    return nullptr;
}

struct PropertyValue {
    const IPropertyHandle* handle;
    void* offset;
    const Property* info;
    template<typename Type>
    Type& Get() const
    {
        return *static_cast<Type*>(offset);
    }
};

// For primitive types..
template<typename ElementType>
void ReadArrayPropertyValue(const json::value& jsonData, PropertyValue& propertyData, string& error)
{
    ElementType* output = &propertyData.Get<ElementType>();
    if (propertyData.info->type.isArray) {
        // expecting array data.
        if (auto const array = jsonData.find(propertyData.info->name); array) {
            auto outputView = array_view<ElementType>(output, propertyData.info->count);
            from_json(*array, outputView);
        }
    } else {
        // expecting single value
        ElementType result;
        if (SafeGetJsonValue(jsonData, propertyData.info->name, error, result)) {
            *output = result;
        }
    }
}

// Specialization for char types (strings).. (null terminate the arrays)
template<>
void ReadArrayPropertyValue<char>(const json::value& jsonData, PropertyValue& propertyData, string& error)
{
    char* output = &propertyData.Get<char>();
    string_view result;
    if (SafeGetJsonValue(jsonData, propertyData.info->name, error, result)) {
        if (propertyData.info->type.isArray) {
            const auto end = result.copy(output, propertyData.info->count - 1, 0);
            output[end] = 0;
        } else {
            *output = result[0];
        }
    }
}

template<class HandleType>
void ReadHandlePropertyValue(const json::value& jsonData, PropertyValue& propertyData, string& error)
{
    decltype(HandleType::id) result;
    if (SafeGetJsonValue(jsonData, propertyData.info->name, error, result)) {
        HandleType& handle = propertyData.Get<HandleType>();
        handle.id = result;
    }
}

template<class VecType, size_t n>
void ReadVecPropertyValue(const json::value& jsonData, PropertyValue& propertyData, string& /* error */)
{
    if (auto const array = jsonData.find(propertyData.info->name); array) {
        VecType& result = propertyData.Get<VecType>();
        from_json(*array, result.data);
    }
}

bool ResolveComponentDependencies(
    IEcs& ecs, array_view<const Uid> dependencies, const array_view<const ITypeInfo* const> componentMetadata)
{
    for (const Uid& dependencyUid : dependencies) {
        // default constructed UID is used as a wildcard for dependecies not known at compile time.
        if (dependencyUid != Uid {}) {
            // See if this component type exists in ecs
            const IComponentManager* componentManager = ecs.GetComponentManager(dependencyUid);
            if (!componentManager) {
                // Find component type and create such manager.
                const auto* componentTypeInfo =
                    FindTypeInfo<ComponentManagerTypeInfo>(dependencyUid, componentMetadata);
                if (componentTypeInfo) {
                    ecs.CreateComponentManager(*componentTypeInfo);
                } else {
                    // Could not find this component manager, unable to continue.
                    return false;
                }
            }
        }
    }

    return true;
}

PropertyValue GetProperty(IPropertyHandle* handle, size_t i, void* offset)
{
    auto* api = handle->Owner();
    if (i < api->PropertyCount()) {
        auto prop = api->MetaData(i);
        return { handle, static_cast<uint8_t*>(offset) + prop->offset, prop };
    }
    return { nullptr, nullptr, nullptr };
}

void ParseProperties(const json::value& jsonData, ISystem& system, string& error)
{
    if (const json::value* propertiesIt = jsonData.find("properties"); propertiesIt) {
        if (auto systemPropertyHandle = system.GetProperties(); systemPropertyHandle) {
            const IPropertyApi* propertyApi = systemPropertyHandle->Owner();
            if (auto offset = systemPropertyHandle->WLock(); offset) {
                for (size_t i = 0; i < propertyApi->PropertyCount(); ++i) {
                    PropertyValue property = GetProperty(systemPropertyHandle, i, offset);
                    if (property.info) {
                        switch (property.info->type) {
                            case PropertyType::BOOL_T:
                            case PropertyType::BOOL_ARRAY_T:
                                ReadArrayPropertyValue<bool>(*propertiesIt, property, error);
                                break;

                            case PropertyType::CHAR_T:
                            case PropertyType::CHAR_ARRAY_T:
                                ReadArrayPropertyValue<char>(*propertiesIt, property, error);
                                break;

                            case PropertyType::INT8_T:
                            case PropertyType::INT8_ARRAY_T:
                                ReadArrayPropertyValue<int8_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::INT16_T:
                            case PropertyType::INT16_ARRAY_T:
                                ReadArrayPropertyValue<int16_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::INT32_T:
                            case PropertyType::INT32_ARRAY_T:
                                ReadArrayPropertyValue<int32_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::INT64_T:
                            case PropertyType::INT64_ARRAY_T:
                                ReadArrayPropertyValue<int64_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::UINT8_T:
                            case PropertyType::UINT8_ARRAY_T:
                                ReadArrayPropertyValue<uint8_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::UINT16_T:
                            case PropertyType::UINT16_ARRAY_T:
                                ReadArrayPropertyValue<uint16_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::UINT32_T:
                            case PropertyType::UINT32_ARRAY_T:
                                ReadArrayPropertyValue<uint32_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::UINT64_T:
                            case PropertyType::UINT64_ARRAY_T:
                                ReadArrayPropertyValue<uint64_t>(*propertiesIt, property, error);
                                break;

                            case PropertyType::DOUBLE_T:
                            case PropertyType::DOUBLE_ARRAY_T:
                                ReadArrayPropertyValue<double>(*propertiesIt, property, error);
                                break;

                            case PropertyType::ENTITY_T:
                                ReadHandlePropertyValue<Entity>(*propertiesIt, property, error);
                                break;

                            case PropertyType::VEC2_T:
                                ReadVecPropertyValue<Vec2, 2>(*propertiesIt, property, error); // 2: Vec2
                                break;
                            case PropertyType::VEC3_T:
                                ReadVecPropertyValue<Vec3, 3>(*propertiesIt, property, error); // 3: Vec3
                                break;
                            case PropertyType::VEC4_T:
                                ReadVecPropertyValue<Vec4, 4>(*propertiesIt, property, error); // 4: Vec4
                                break;
                            case PropertyType::QUAT_T:
                                ReadVecPropertyValue<Quat, 4>(*propertiesIt, property, error); // 4: Vec4
                                break;
                            case PropertyType::MAT4X4_T:
                                // NOTE: Implement.
                                break;

                            case PropertyType::FLOAT_T:
                            case PropertyType::FLOAT_ARRAY_T:
                                ReadArrayPropertyValue<float>(*propertiesIt, property, error);
                                break;

                            case PropertyType::STRING_T:
                                ReadArrayPropertyValue<string>(*propertiesIt, property, error);
                                break;

                            case PropertyType::ENTITY_ARRAY_T:
                            case PropertyType::VEC2_ARRAY_T:
                            case PropertyType::VEC3_ARRAY_T:
                            case PropertyType::VEC4_ARRAY_T:
                            case PropertyType::QUAT_ARRAY_T:
                            case PropertyType::MAT4X4_ARRAY_T:
                            case PropertyType::INVALID:
                                // NOTE: Implement.
                                break;
                        }
                    }
                }
            }
            systemPropertyHandle->WUnlock();
            system.SetProperties(*systemPropertyHandle); // notify that the properties HAVE changed.
        }
    }
}

bool ParseSystem(const json::value& jsonData, const array_view<const ITypeInfo* const>& componentMetadata,
    const array_view<const ITypeInfo* const>& systemMetadata, IEcs& ecs, string& error)
{
    string typeName;
    SafeGetJsonValue(jsonData, "typeName", error, typeName);

    bool optional = false;
    SafeGetJsonValue(jsonData, "optional", error, optional);

    // Look up this system type from metadata.
    const auto* typeInfo = FindTypeInfo<SystemTypeInfo>(typeName, systemMetadata);
    if (typeInfo) {
        // Ensure we have all components required by this system.
        if (!ResolveComponentDependencies(ecs, typeInfo->componentDependencies, componentMetadata)) {
            error += "Failed to resolve component dependencies: (" + typeName + ".\n";
            return false;
        }
        if (!ResolveComponentDependencies(ecs, typeInfo->readOnlyComponentDependencies, componentMetadata)) {
            error += "Failed to resolve read-only component dependencies: (" + typeName + ".\n";
            return false;
        }

        // Create system and read properties.
        ISystem* system = ecs.CreateSystem(*typeInfo);
        ParseProperties(jsonData, *system, error);

        return true;
    }
    CORE_LOG_W("Cannot find system: %s (optional: %s)", typeName.c_str(), (optional ? "true" : "false"));
    if (!optional) {
        error += "Cannot find system: " + typeName + ".\n";
    }

    return optional;
}
} // namespace

SystemGraphLoader::LoadResult SystemGraphLoader::Load(const string_view uri, IEcs& ecs)
{
    IFile::Ptr file = fileManager_.OpenFile(uri);
    if (!file) {
        CORE_LOG_D("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to open file.");
    }

    const uint64_t byteLength = file->GetLength();

    string raw(static_cast<size_t>(byteLength), string::value_type());
    if (file->Read(raw.data(), byteLength) != byteLength) {
        CORE_LOG_D("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to read file.");
    }

    return LoadString(raw, ecs);
}

SystemGraphLoader::LoadResult SystemGraphLoader::LoadString(const string_view jsonString, IEcs& ecs)
{
    auto const json = json::parse(jsonString.data());
    if (json) {
        LoadResult finalResult;
        {
            string ver;
            string type;
            uint32_t verMajor { ~0u };
            uint32_t verMinor { ~0u };
            if (const json::value* iter = json.find("compatibility_info"); iter) {
                SafeGetJsonValue(*iter, "type", finalResult.error, type);
                SafeGetJsonValue(*iter, "version", finalResult.error, ver);
                if (ver.size() == VERSION_SIZE) {
                    if (const auto delim = ver.find('.'); delim != string::npos) {
                        std::from_chars(ver.data(), ver.data() + delim, verMajor);
                        std::from_chars(ver.data() + delim + 1, ver.data() + ver.size(), verMinor);
                    }
                }
            }
            if ((type != "systemgraph") || (verMajor != VERSION_MAJOR)) {
                // NOTE: we're still loading the system graph
                CORE_LOG_W("DEPRECATED SYSTEM GRAPH: invalid system graph type (%s) and / or invalid version (%s).",
                    type.c_str(), ver.c_str());
            }
        }
        auto& pluginRegister = GetPluginRegister();
        auto componentMetadata = pluginRegister.GetTypeInfos(ComponentManagerTypeInfo::UID);
        auto systemMetadata = pluginRegister.GetTypeInfos(SystemTypeInfo::UID);
        const auto& systemsArrayIt = json.find("systems");
        if (systemsArrayIt && systemsArrayIt->is_array()) {
            for (const auto& systemJson : systemsArrayIt->array_) {
                if (!ParseSystem(systemJson, componentMetadata, systemMetadata, ecs, finalResult.error)) {
                    break;
                }
            }
        }

        finalResult.success = finalResult.error.empty();

        return finalResult;
    }
    return LoadResult("Invalid json file.");
}

SystemGraphLoader::SystemGraphLoader(IFileManager& fileManager) : fileManager_ { fileManager } {}

void SystemGraphLoader::Destroy()
{
    delete this;
}

// SystemGraphLoader factory
ISystemGraphLoader::Ptr SystemGraphLoaderFactory::Create(IFileManager& fileManager)
{
    return ISystemGraphLoader::Ptr { new SystemGraphLoader(fileManager) };
}

const IInterface* SystemGraphLoaderFactory::GetInterface(const Uid& uid) const
{
    if ((uid == ISystemGraphLoaderFactory::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* SystemGraphLoaderFactory::GetInterface(const Uid& uid)
{
    if ((uid == ISystemGraphLoaderFactory::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void SystemGraphLoaderFactory::Ref() {}
void SystemGraphLoaderFactory::Unref() {}

CORE_END_NAMESPACE()
