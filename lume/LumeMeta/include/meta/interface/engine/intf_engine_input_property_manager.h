/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: meta engine value manager
 * Author: Mikael Kilpeläinen
 * Create: 2024-02-19
 */

#ifndef META_ENGINE_INTERFACE_ENGINE_INPUT_PROPERTY_MANAGER_H
#define META_ENGINE_INTERFACE_ENGINE_INPUT_PROPERTY_MANAGER_H

#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IEngineInputPropertyManager, "db3adbf0-4caf-4b7b-8850-ee1f0936beab")

/**
 * @brief Collection of properties that are input for core engine
 */
class IEngineInputPropertyManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineInputPropertyManager)
public:
    /**
     * @brief Synchronise engine values to the core engine
     * @return True if synchronising all properties worked, otherwise false
     */
    virtual bool Sync() = 0;
    /**
     * @brief Construct (or return existing) property with the corresponding type from core engine
     *        and set the value from engine property as default value.
     */
    virtual IProperty::Ptr ConstructProperty(BASE_NS::string_view name) = 0;
    /**
     * @brief Ties the given property to engine value with 'valueName'
     * @return The property given if successful, otherwise nullptr
     */
    virtual IProperty::Ptr TieProperty(const IProperty::Ptr&, BASE_NS::string valueName) = 0;
    /**
     * @brief Get all properties in this collection
     */
    virtual BASE_NS::vector<IProperty::Ptr> GetAllProperties() const = 0;
    /**
     * @brief Construct property for all associated engine values and add to the given IMetadata
     * @return True on success
     */
    virtual bool PopulateProperties(IMetadata&) = 0;
    /**
     * @brief Remove property with given names
     */
    virtual void RemoveProperty(BASE_NS::string_view name) = 0;
    /**
     * @brief Get associated engine value manager
     */
    virtual IEngineValueManager::Ptr GetValueManager() const = 0;
    /**
     * @brief Construct (or return existing) property with the corresponding type from core engine
     *        and set the value from engine property as default value.
     */
    template<typename Type>
    Property<Type> ConstructProperty(BASE_NS::string_view name)
    {
        return Property<Type>(ConstructProperty(name));
    }
};

META_INTERFACE_TYPE(META_NS::IEngineInputPropertyManager)
META_END_NAMESPACE()

#endif
