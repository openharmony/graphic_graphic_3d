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

#ifndef META_INTERFACE_IMETADATA_H
#define META_INTERFACE_IMETADATA_H

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/plugin/intf_interface.h>

#include <meta/api/util.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_callable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IMetadata, "5ad7917a-e744-48bb-b789-9446d3712cf9")

class IMetadata;

/**
 * @brief The IMetadata interface defines a queryable metadata interface for retrieving object properties and functions.
 */
class IMetadata : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMetadata)
public:
    virtual IContainer::Ptr GetPropertyContainer() = 0;
    virtual IContainer::ConstPtr GetPropertyContainer() const = 0;
    /**
     * @brief Returns all available properties.
     */
    virtual BASE_NS::vector<IProperty::Ptr> GetAllProperties() = 0;
    virtual BASE_NS::vector<IProperty::ConstPtr> GetAllProperties() const = 0;
    /**
     * @brief Returns all available functions.
     */
    virtual BASE_NS::vector<IFunction::Ptr> GetAllFunctions() = 0;
    virtual BASE_NS::vector<IFunction::ConstPtr> GetAllFunctions() const = 0;
    /**
     * @brief Returns all available events.
     */
    virtual BASE_NS::vector<IEvent::Ptr> GetAllEvents() = 0;
    virtual BASE_NS::vector<IEvent::ConstPtr> GetAllEvents() const = 0;

    /**
     * @brief Returns the property with a given name.
     * @param name Name of the property to retrieve.
     * @return The property or nullptr if such property does not exist.
     */
    virtual IProperty::Ptr GetPropertyByName(BASE_NS::string_view name) = 0;
    virtual IProperty::ConstPtr GetPropertyByName(BASE_NS::string_view name) const = 0;
    /**
     * @brief Returns the function with a given name
     * @param name Name of the function to retrieve.
     * @return The function or nullptr if such function does not exist.
     */
    virtual IFunction::Ptr GetFunctionByName(BASE_NS::string_view name) = 0;
    virtual IFunction::ConstPtr GetFunctionByName(BASE_NS::string_view name) const = 0;
    /**
     * @brief Returns the event with a given name
     * @param name Name of the event to retrieve.
     * @return The event or nullptr if such event does not exist.
     */
    virtual IEvent::ConstPtr GetEventByName(BASE_NS::string_view name) const = 0;
    virtual IEvent::Ptr GetEventByName(BASE_NS::string_view name) = 0;

    /*
     * @brief Make Deep copy of the metadata
     * Notice that the IFunctions cannot be used without re-attaching to object
     */
    virtual IMetadata::Ptr CloneMetadata() const = 0;

    /**
     * @brief Add function to metadata
     */
    virtual void AddFunction(const IFunction::Ptr&) = 0;
    /**
     * @brief Remove function from metadata
     */
    virtual void RemoveFunction(const IFunction::Ptr&) = 0;

    /**
     * @brief Add property to metadata
     */
    virtual void AddProperty(const IProperty::Ptr&) = 0;
    /**
     * @brief Remove property from metadata
     */
    virtual void RemoveProperty(const IProperty::Ptr&) = 0;

    /**
     * @brief Add event to metadata
     */
    virtual void AddEvent(const IEvent::Ptr&) = 0;
    /**
     * @brief Remove event from metadata
     */
    virtual void RemoveEvent(const IEvent::Ptr&) = 0;

    /**
     * @brief Set all properties (removes previous properties)
     */
    virtual void SetProperties(const BASE_NS::vector<IProperty::Ptr>&) = 0;

    /**
     * @brief Merge given metadata to current one
     */
    virtual void Merge(const IMetadata::Ptr&) = 0;

    // templated helper for named property fetching. handles typing etc.
    // returns null if not found or type is not matching.
    template<typename ValueType>
    Property<ValueType> GetPropertyByName(BASE_NS::string_view name)
    {
        return GetPropertyByName(name);
    }
    template<typename ValueType>
    Property<const ValueType> GetPropertyByName(BASE_NS::string_view name) const
    {
        return GetPropertyByName(name);
    }
    template<typename ValueType>
    ArrayProperty<ValueType> GetArrayPropertyByName(BASE_NS::string_view name)
    {
        return GetPropertyByName(name);
    }
    template<typename ValueType>
    ArrayProperty<const ValueType> GetArrayPropertyByName(BASE_NS::string_view name) const
    {
        return GetPropertyByName(name);
    }
};

template<typename ValueType>
constexpr auto GetValue(const META_NS::IMetadata* meta, BASE_NS::string_view name, ValueType defaultValue = {}) noexcept
{
    if (meta) {
        if (auto property = meta->GetPropertyByName<ValueType>(name)) {
            return GetValue(property, defaultValue);
        }
    }
    return BASE_NS::move(defaultValue);
}

template<typename ValueType, typename Interface,
    typename = BASE_NS::enable_if_t<IsKindOfIInterface_v<BASE_NS::remove_const_t<Interface>*>>>
constexpr auto GetValue(
    const BASE_NS::shared_ptr<Interface>& intf, BASE_NS::string_view name, ValueType defaultValue = {}) noexcept
{
    return GetValue<ValueType>(interface_cast<IMetadata>(intf), name, defaultValue);
}

template<typename ValueType, typename Interface,
    typename = BASE_NS::enable_if_t<IsKindOfIInterface_v<BASE_NS::remove_const_t<Interface>*>>>
constexpr auto GetValue(
    const BASE_NS::weak_ptr<Interface>& intf, BASE_NS::string_view name, ValueType defaultValue = {}) noexcept
{
    return GetValue<ValueType>(interface_pointer_cast<IMetadata>(intf), name, defaultValue);
}

template<typename ValueType>
constexpr bool SetValue(META_NS::IMetadata* meta, BASE_NS::string_view name, const ValueType& value) noexcept
{
    if (meta) {
        if (auto property = meta->GetPropertyByName<ValueType>(name)) {
            return SetValue(property, value);
        }
    }
    return false;
}

template<typename ValueType, typename Interface, typename = BASE_NS::enable_if_t<IsKindOfIInterface_v<Interface*>>>
constexpr auto SetValue(
    const BASE_NS::shared_ptr<Interface>& intf, BASE_NS::string_view name, const ValueType& value) noexcept
{
    return SetValue(interface_cast<IMetadata>(intf), name, value);
}

template<typename ValueType, typename Interface, typename = BASE_NS::enable_if_t<IsKindOfIInterface_v<Interface*>>>
constexpr bool SetValue(
    const BASE_NS::weak_ptr<Interface>& intf, BASE_NS::string_view name, const ValueType& value) noexcept
{
    return SetValue(interface_pointer_cast<IMetadata>(intf), name, value);
}

/**
 * @brief Internal interface to set metadata for objects
 */
class IMetadataInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMetadataInternal, "3e748485-88bd-4098-ae8d-cf5ada17461d")
public:
    virtual IMetadata::Ptr GetMetadata() const = 0;
    virtual void SetMetadata(const IMetadata::Ptr& meta) = 0;
    virtual const StaticObjectMetadata& GetStaticMetadata() const = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IMetadata);

#endif
