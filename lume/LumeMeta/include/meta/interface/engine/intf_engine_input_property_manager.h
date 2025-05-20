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
