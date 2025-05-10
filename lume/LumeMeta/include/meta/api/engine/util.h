/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: Engine value helpers
 * Author: Mikael Kilpeläinen
 * Create: 2024-02-28
 */
#ifndef META_API_ENGINE_UTIL_H
#define META_API_ENGINE_UTIL_H

#include <meta/api/util.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

META_BEGIN_NAMESPACE()

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
            PropertyLock lock { p };
            TypeId ids[] = { IEngineValue::UID };
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
        CORE_LOG_W("Trying to attach incompatible engine value to property (%s)", p->GetName().c_str());
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
