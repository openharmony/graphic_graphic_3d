/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of IMetadata interface
 * Author: Jani Kattelus
 * Create: 2021-08-26
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
#include <meta/interface/intf_static_metadata.h>
#include <meta/interface/metadata_query.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IMetadata, "5ad7917a-e744-48bb-b789-9446d3712cf9")

struct MetadataInfo {
    MetadataType type {};
    BASE_NS::string name;
    TypeId interfaceId;
    bool readOnly {};
    TypeId propertyType;
    const StaticMetadata* data {};

    bool IsValid() const
    {
        return !name.empty();
    }
};

inline bool operator==(const MetadataInfo& l, const MetadataInfo& r)
{
    return l.type == r.type && l.name == r.name && l.interfaceId == r.interfaceId && l.readOnly == r.readOnly &&
           l.propertyType == r.propertyType;
}
inline bool operator!=(const MetadataInfo& l, const MetadataInfo& r)
{
    return !(l == r);
}

/**
 * @brief The IMetadata interface defines a queryable metadata interface for retrieving object properties and functions.
 */
class IMetadata : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMetadata)
public:
    /**
     * @brief Returns all properties explicitly added or instantiated from static metadata
     */
    virtual BASE_NS::vector<IProperty::Ptr> GetProperties() = 0;
    virtual BASE_NS::vector<IProperty::ConstPtr> GetProperties() const = 0;
    /**
     * @brief Returns all functions explicitly added or instantiated from static metadata
     */
    virtual BASE_NS::vector<IFunction::Ptr> GetFunctions() = 0;
    virtual BASE_NS::vector<IFunction::ConstPtr> GetFunctions() const = 0;
    /**
     * @brief Returns all events explicitly added or instantiated from static metadata
     */
    virtual BASE_NS::vector<IEvent::Ptr> GetEvents() = 0;
    virtual BASE_NS::vector<IEvent::ConstPtr> GetEvents() const = 0;

    /**
     * @brief Get all metadata for requested types
     * @Note  This combines static metadata and existing runtime data
     */
    virtual BASE_NS::vector<MetadataInfo> GetAllMetadatas(MetadataType types) const = 0;

    /**
     * @brief Get metadata for requested entity, the first matching name is returned if type matches
     */
    virtual MetadataInfo GetMetadata(MetadataType type, BASE_NS::string_view name) const = 0;

    /**
     * @brief Returns the property with a given name.
     * @param name Name of the property to retrieve.
     * @return The property or nullptr if such property does not exist.
     */
    virtual IProperty::Ptr GetProperty(BASE_NS::string_view name, MetadataQuery) = 0;
    IProperty::Ptr GetProperty(BASE_NS::string_view name)
    {
        return GetProperty(name, MetadataQuery::CONSTRUCT_ON_REQUEST);
    }
    virtual IProperty::ConstPtr GetProperty(BASE_NS::string_view name, MetadataQuery) const = 0;
    IProperty::ConstPtr GetProperty(BASE_NS::string_view name) const
    {
        return GetProperty(name, MetadataQuery::CONSTRUCT_ON_REQUEST);
    }
    /**
     * @brief Returns the function with a given name
     * @param name Name of the function to retrieve.
     * @return The function or nullptr if such function does not exist.
     */
    virtual IFunction::Ptr GetFunction(BASE_NS::string_view name, MetadataQuery) = 0;
    IFunction::Ptr GetFunction(BASE_NS::string_view name)
    {
        return GetFunction(name, MetadataQuery::CONSTRUCT_ON_REQUEST);
    }
    virtual IFunction::ConstPtr GetFunction(BASE_NS::string_view name, MetadataQuery) const = 0;
    IFunction::ConstPtr GetFunction(BASE_NS::string_view name) const
    {
        return GetFunction(name, MetadataQuery::CONSTRUCT_ON_REQUEST);
    }

    /**
     * @brief Returns the event with a given name
     * @param name Name of the event to retrieve.
     * @return The event or nullptr if such event does not exist.
     */
    virtual IEvent::Ptr GetEvent(BASE_NS::string_view name, MetadataQuery) = 0;
    IEvent::Ptr GetEvent(BASE_NS::string_view name)
    {
        return GetEvent(name, MetadataQuery::CONSTRUCT_ON_REQUEST);
    }
    virtual IEvent::ConstPtr GetEvent(BASE_NS::string_view name, MetadataQuery) const = 0;
    IEvent::ConstPtr GetEvent(BASE_NS::string_view name) const
    {
        return GetEvent(name, MetadataQuery::CONSTRUCT_ON_REQUEST);
    }

    /**
     * @brief Add function to metadata
     */
    virtual bool AddFunction(const IFunction::Ptr&) = 0;
    /**
     * @brief Remove function from metadata
     */
    virtual bool RemoveFunction(const IFunction::Ptr&) = 0;

    /**
     * @brief Add property to metadata
     */
    virtual bool AddProperty(const IProperty::Ptr&) = 0;
    /**
     * @brief Remove property from metadata
     */
    virtual bool RemoveProperty(const IProperty::Ptr&) = 0;

    /**
     * @brief Add event to metadata
     */
    virtual bool AddEvent(const IEvent::Ptr&) = 0;
    /**
     * @brief Remove event from metadata
     */
    virtual bool RemoveEvent(const IEvent::Ptr&) = 0;

    // templated helper for named property fetching. handles typing etc.
    // returns null if not found or type is not matching.
    template<typename ValueType>
    Property<ValueType> GetProperty(BASE_NS::string_view name, MetadataQuery req = MetadataQuery::CONSTRUCT_ON_REQUEST)
    {
        return GetProperty(name, req);
    }
    template<typename ValueType>
    Property<const ValueType> GetProperty(
        BASE_NS::string_view name, MetadataQuery req = MetadataQuery::CONSTRUCT_ON_REQUEST) const
    {
        return GetProperty(name, req);
    }
    template<typename ValueType>
    ArrayProperty<ValueType> GetArrayProperty(
        BASE_NS::string_view name, MetadataQuery req = MetadataQuery::CONSTRUCT_ON_REQUEST)
    {
        return GetProperty(name, req);
    }
    template<typename ValueType>
    ArrayProperty<const ValueType> GetArrayProperty(
        BASE_NS::string_view name, MetadataQuery req = MetadataQuery::CONSTRUCT_ON_REQUEST) const
    {
        return GetProperty(name, req);
    }
};

template<typename ValueType>
constexpr auto GetValue(const META_NS::IMetadata* meta, BASE_NS::string_view name, ValueType defaultValue = {}) noexcept
{
    if (meta) {
        if (auto property = meta->GetProperty<ValueType>(name)) {
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
        if (auto property = meta->GetProperty<ValueType>(name)) {
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

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IMetadata)

#endif
