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
#ifndef META_API_ENGINE_UTIL_H
#define META_API_ENGINE_UTIL_H

#include <core/property/intf_property_api.h>

#include <meta/api/util.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Info about an engine property without creating engine values or bridges
 */
struct EnginePropertyInfo {
    BASE_NS::string name;            ///< Property name
    CORE_NS::PropertyTypeDecl type;  ///< Property type
};

/** @brief EnumerateProperties configuration */
struct EnginePropertyInfoConfig {
    bool traverseMemberProperties{};  ///< If true, traverse also memberProperties of each property
    bool includeStructs{true};        ///< If true, include struct type properties in the output
};

namespace Internal {

/**
 * @brief Returns true if a property should be added to RecurseEngineProperties output based on config.
 */
inline bool ShouldAdd(const CORE_NS::Property& property, const EnginePropertyInfoConfig& config)
{
    bool isLeaf = property.metaData.memberProperties.empty();
    isLeaf |= !property.metaData.enumMetaData.empty();  // enums are considered a leaf even if it has a member
    return config.includeStructs || isLeaf;
}

/**
 * @brief Returns true if a property should be traversed into by RecurseEngineProperties
 */
inline bool ShouldTraverse(const CORE_NS::Property& property, const EnginePropertyInfoConfig& config)
{
    auto& meta = property.metaData;
    // Do not traverse into enum values
    return config.traverseMemberProperties && meta.enumMetaData.empty() && !meta.memberProperties.empty();
}

/**
 * @brief Recurses into member properties of a CORE_NS::Property
 * @param members Members to recuree into.
 * @param namePrefix Prefix to apply to output names.
 * @param out Vector to append the the info to.
 */
inline void RecurseEngineProperties(BASE_NS::array_view<const CORE_NS::Property> members,
    BASE_NS::string_view namePrefix, BASE_NS::vector<META_NS::EnginePropertyInfo>& out,
    const EnginePropertyInfoConfig& config)
{
    for (auto&& p : members) {
        META_NS::EnginePropertyInfo info;
        info.name = namePrefix.empty() ? p.name : namePrefix + "." + p.name;
        info.type = p.type;
        // Do not include if the property has members and includeStructs=False
        auto& ref = ShouldAdd(p, config) ? out.emplace_back(BASE_NS::move(info)) : info;
        if (ShouldTraverse(p, config)) {
            RecurseEngineProperties(p.metaData.memberProperties, ref.name, out, config);
        }
    }
}

}  // namespace Internal

/**
 * @brief Enumerate property names and types from a component manager without creating engine values.
 * @param manager The component manager to enumerate properties from
 * @param namePrefix Property name prefix
 * @param out Vector to append the the info to.
 * @param recursive. If true, add also memberProperties of each top-level property.
 */
inline void EnumerateEngineProperties(const CORE_NS::IComponentManager& manager, BASE_NS::string_view namePrefix,
    BASE_NS::vector<EnginePropertyInfo>& out, const EnginePropertyInfoConfig& config = {})
{
    auto d = manager.GetPropertyApi().MetaData();
    Internal::RecurseEngineProperties(d, namePrefix, out, config);
}

/**
 * @brief Populate engine values recursively from IEngineValue
 * @param m Engine value manager to populate
 * @param value Start populating from this engine value
 * @param options Options for engine values and optional output vector for populated values
 */
inline void AddEngineValuesRecursively(
    const IEngineValueManager::Ptr& m, const IEngineValue::Ptr& value, EngineValueOptions options = {})
{
    BASE_NS::vector<IEngineValue::Ptr> vec;
    auto evo = options;
    evo.values = &vec;
    if (m->ConstructValues(value, evo)) {
        if (options.values) {
            options.values->insert(options.values->end(), vec.begin(), vec.end());
        }
        if (evo.recurseKnownStructs) {
            for (auto&& v : vec) {
                AddEngineValuesRecursively(m, v, options);
            }
        }
    }
}

/**
 * @brief Populate engine values recursively from EnginePropertyHandle
 * @param m Engine value manager to populate
 * @param handle Start populating from this engine property handle
 * @param options Options for engine values and optional output vector for populated values
 */
inline void AddEngineValuesRecursively(
    const IEngineValueManager::Ptr& m, EnginePropertyHandle handle, EngineValueOptions options = {})
{
    BASE_NS::vector<IEngineValue::Ptr> vec;
    auto evo = options;
    evo.values = &vec;
    if (m->ConstructValues(handle, evo)) {
        if (options.values) {
            options.values->insert(options.values->end(), vec.begin(), vec.end());
        }
        if (evo.recurseKnownStructs) {
            options.namePrefix = "";
            for (auto&& v : vec) {
                AddEngineValuesRecursively(m, v, options);
            }
        }
    }
}

/**
 * @brief Add engine value to stack property
 * @param p Property where the value is added
 * @param value The engine value to be added
 * @return True on success
 */
inline bool SetEngineValueToProperty(const IProperty::Ptr& p, const IEngineValue::Ptr& value, bool setAsDefault = true)
{
    if (p && value && value->IsCompatible(p->GetTypeId())) {
        if (auto i = interface_cast<IStackProperty>(p)) {
            PropertyLock lock{p};
            TypeId ids[] = {IEngineValue::UID};
            for (auto&& v : i->GetValues(ids, false)) {
                i->RemoveValue(v);
            }
            i->PushValue(value);
            if (setAsDefault) {
                i->SetDefaultValue(value->GetValue());
            }
            return true;
        }
    } else {
        CORE_LOG_W("Trying to attach incompatible engine value to property (%s)", p ? p->GetName().c_str() : "unknown");
    }
    return false;
}

/// Get Engine value from property if it contains one, otherwise nullptr
inline IEngineValue::Ptr GetEngineValueFromProperty(const IProperty::ConstPtr& p)
{
    return GetFirstValueFromProperty<IEngineValue>(p);
}

/**
 * @brief Create property from engine value with matching type
 * @param name Name of the property
 * @param value Engine value where the type is derived and what is added to the property
 * @return Property on success, otherwise nullptr
 */
inline IProperty::Ptr PropertyFromEngineValue(BASE_NS::string_view name, const IEngineValue::Ptr& value)
{
    IProperty::Ptr ret;
    if (auto internal = interface_cast<IEngineValueInternal>(value)) {
        ret = GetObjectRegistry().GetPropertyRegister().Create(META_NS::ClassId::StackProperty, name);
        if (auto i = interface_cast<IPropertyInternalAny>(ret)) {
            i->SetInternalAny(internal->CreateAny());
        }
        if (auto i = interface_cast<IStackProperty>(ret)) {
            i->PushValue(value);
            i->SetDefaultValue(value->GetValue());
        }
    }
    return ret;
}

META_END_NAMESPACE()

#endif
