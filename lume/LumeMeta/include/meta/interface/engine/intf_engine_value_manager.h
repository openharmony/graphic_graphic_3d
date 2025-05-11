/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: meta engine value manager
 * Author: Mikael Kilpeläinen
 * Create: 2024-02-19
 */

#ifndef META_ENGINE_INTERFACE_ENGINE_VALUE_MANAGER_H
#define META_ENGINE_INTERFACE_ENGINE_VALUE_MANAGER_H

#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IEngineValueManager, "542b7e13-21bd-4c9c-8d3f-a9527732216c")

/**
 * @brief Options for constructing engine values
 */
struct EngineValueOptions {
    /// prefix that is added before the name (plus '.')
    BASE_NS::string namePrefix;
    /// optional output vector to put all matching values
    BASE_NS::vector<IEngineValue::Ptr>* values {};
    /**
     * @brief Push value directly to engine when set, if this is enabled,
     *        the values can only be set in the engine task queue thread
     **/
    bool pushValuesDirectlyToEngine {};
    /**
     * @brief If true, also recursively handle known struct types. E.g. Vec4 would be expanded into x, y, z, and w
     * properties.
     */
    bool recurseKnownStructs {};
};

/**
 * @brief Manager that contains engine values, handles their synchronisation and notification of changes
 */
class IEngineValueManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineValueManager)
public:
    /**
     * @brief Set task queue where the engine value change notifications are posted to.
     *        This must be set to be able to get notifications.
     */
    virtual void SetNotificationQueue(const ITaskQueue::WeakPtr&) = 0;

    /**
     * @brief Synchronise engine values with core engine
     *        Allows to specify direction to synchronise to.
     * @note  This must be called in correct thread because it touches the core engine
     */
    virtual bool Sync(EngineSyncDirection) = 0;

    /// Add engine values from the handle
    virtual bool ConstructValues(EnginePropertyHandle handle, EngineValueOptions) = 0;
    // Add engine values from value (e.g. member properties or follow EnginePropertyHandle)
    virtual bool ConstructValues(IValue::Ptr value, EngineValueOptions) = 0;
    // Add single engine value from EnginePropertyParams
    virtual bool ConstructValue(EnginePropertyParams property, EngineValueOptions) = 0;
    /// Add single engine value starting from handle and following path
    virtual bool ConstructValue(EnginePropertyHandle handle, BASE_NS::string_view path, EngineValueOptions) = 0;

    /// Remove engine value with given name
    virtual bool RemoveValue(BASE_NS::string_view name) = 0;
    /// Remove all engine values
    virtual void RemoveAll() = 0;

    /// Construct property from engine value with given name
    virtual IProperty::Ptr ConstructProperty(BASE_NS::string_view name) const = 0;
    /// Get engine value of given name
    virtual IEngineValue::Ptr GetEngineValue(BASE_NS::string_view name) const = 0;
    /// Construct property from all contained engine values
    virtual BASE_NS::vector<IProperty::Ptr> ConstructAllProperties() const = 0;
    /// Get all engine values
    virtual BASE_NS::vector<IEngineValue::Ptr> GetAllEngineValues() const = 0;
    /// Construct property from engine value with given name
    template<typename Type>
    Property<Type> ConstructProperty(BASE_NS::string_view name)
    {
        return Property<Type>(ConstructProperty(name));
    }
    /// Construct array property from engine value with given name
    template<typename Type>
    ArrayProperty<Type> ConstructArrayProperty(BASE_NS::string_view name)
    {
        return ArrayProperty<Type>(ConstructProperty(name));
    }
    /// Add engine values from the handle with default options
    bool ConstructValues(EnginePropertyHandle handle)
    {
        return ConstructValues(handle, {});
    }
};

META_INTERFACE_TYPE(META_NS::IEngineValueManager)
META_END_NAMESPACE()

#endif
