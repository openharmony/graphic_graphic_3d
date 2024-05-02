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

#ifndef META_INTERFACE_IOBJECT_REGISTRY_H
#define META_INTERFACE_IOBJECT_REGISTRY_H

#include <base/containers/vector.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include <meta/base/ids.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/base/object_traits.h>
#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/object_type_info.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/serialization/intf_global_serialization_data.h>

META_BEGIN_NAMESPACE()
/**
 * @brief Information for an object category.
 */
struct ObjectCategoryItem {
    ObjectCategoryBits category;
    BASE_NS::string_view name;
};

class IInterpolator;
class IObjectRegistry;
class ICallContext;
class IObjectContext;
class IObjectFactory;
class IObject;
class IClassRegistry;
class IPropertyRegister;
class IMetadata;
class IEngineData;

META_REGISTER_INTERFACE(IObjectRegistry, "1b2081c8-031f-4962-bb97-de2209573e96")
META_REGISTER_INTERFACE(IObjectRegistryExporter, "79e4e8ec-7da5-4757-a819-a1817e4bdd4f")

/**
 * @brief The IObjectRegistryExporter defines an interface for exporting an
 *        object registry instance to string. Used for debugging purposes.
 */
class IObjectRegistryExporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectRegistryExporter)
public:
    /**
     * @brief Return a string representation of all objects alive within an object
     *        registry instance.
     * @param registry The object registry instance to export.
     */
    virtual BASE_NS::string ExportRegistry(const IObjectRegistry* registry) = 0;
};

/**
 * @brief The IObjectRegistry interface can be used to create widgets of any type registered with the registry.
 */
class IObjectRegistry : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectRegistry)
public:
    /**
     * @brief The CreateInfo struct defines set of object creation parameters that the user can give
     *        as a parameter when creating an object. In most cases the create information does not
     *        need to be filled.
     */
    struct CreateInfo {
        constexpr CreateInfo() noexcept : instanceId(), isGloballyAvailable(false) {};
        constexpr CreateInfo(InstanceId id) noexcept : instanceId(id), isGloballyAvailable(false) {};
        constexpr CreateInfo(InstanceId id, bool global) noexcept : instanceId(id), isGloballyAvailable(global) {};
        /** Specify instance id of the object to create. This should usually be left empty. */
        InstanceId instanceId;
        /** If true, the instance should be considered as a global object, This will affect
            serialization, as global objects are only serialized as a reference. The references
            are assumed to be always available even when de-serializing the object in some
            other toolkit instance. */
        bool isGloballyAvailable;
    };

    /**
     * @brief Registers a class to the registry. Only registered objects can be constructed through Create().
     * @param classInfo Class info for the class. Class info is assumed to not change while the class is
     *                  registered.
     * @return True if class was successfully registered, false otherwise.
     */
    virtual bool RegisterObjectType(const IClassInfo::Ptr& classInfo) = 0;

    /**
     * @brief Unregisters a class from the registry. Must be called before the plugin that called RegisterObjectType
     *        is unloaded.
     * @param classInfo The info for the class to unregister.
     * @return True if class was successfully unregistered, false otherwise.
     */
    virtual bool UnregisterObjectType(const IClassInfo::Ptr& classInfo) = 0;
    /**
     * @brief A helper template for calling RegisterObjectType on a type which defines IObjectFactory::Ptr GetFactory().
     */
    template<class T>
    constexpr void RegisterObjectType()
    {
        RegisterObjectType(T::GetFactory());
    }
    /**
     * @brief A helper template for calling UnregisterObjectType on a type which defines IObjectFactory::Ptr
     * GetFactory().
     */
    template<class T>
    constexpr void UnregisterObjectType()
    {
        UnregisterObjectType(T::GetFactory());
    }
    /**
     * @brief Creates a new instance of an object of a given type.
     * @param uid Uid of the Object to create.
     * @return The Object of correct type or nullptr if creation was not successful.
     */
    BASE_NS::shared_ptr<IObject> Create(ObjectId id) const
    {
        return Create(BASE_NS::move(id), CreateInfo {});
    }

    virtual BASE_NS::shared_ptr<IObject> Create(
        ObjectId id, const CreateInfo& createInfo, const BASE_NS::shared_ptr<IMetadata>& data) const = 0;

    /**
     * @brief Creates a new instance of an object of a given type and specific creation info
     * @param uid Uid of the Object to create.
     * @param createInfo Force some parameters for the created object.
     * @return The Object of correct type or nullptr if creation was not successful.
     */
    virtual BASE_NS::shared_ptr<IObject> Create(ObjectId id, const CreateInfo& createInfo) const = 0;

    /**
     * @brief Creates a new instance of an object of a given type.
     *        If the singleton flag is true, Object registry will store a weak reference to
     *        the created object, so as long as any caller has a strong reference to the
     *        object any further calls to Create will return this same object.
     * @param info ClassInfo of the Object to create.
     * @return The Object of correct type or nullptr if creation was not successful.
     */
    BASE_NS::shared_ptr<IObject> Create(const META_NS::ClassInfo& info) const
    {
        return Create(info, CreateInfo {});
    }
    BASE_NS::shared_ptr<IObject> Create(
        const META_NS::ClassInfo& info, const BASE_NS::shared_ptr<IMetadata>& data) const
    {
        return Create(info, CreateInfo {}, data);
    }

    /**
     * @brief Creates a new instance of an object of a given type specific creation info.
     *        If the singleton flag is true, Object registry will store a weak reference to
     *        the created object, so as long as any caller has a strong reference to the
     *        object any further calls to Create will return this same object.
     * @param info ClassInfo of the Object to create.
     * @param createInfo Force some parameters for the created object.
     * @return The Object of correct type or nullptr if creation was not successful.
     */
    virtual BASE_NS::shared_ptr<IObject> Create(const META_NS::ClassInfo& info, const CreateInfo& createInfo) const = 0;

    /**
     * @brief Returns all available object categories. Use GetAllTypes() for a list of
     *        objects that can be created with Create() for a specific category.
     */
    virtual BASE_NS::vector<ObjectCategoryItem> GetAllCategories() const = 0;
    /**
     * @brief Returns a type information list of all registered object types for given categories.
     * @param category The category to list. If category is ObjectCategoryBits::ANY, all
     *        registered object type infos are returned, regardless of category.
     */
    BASE_NS::vector<IClassInfo::ConstPtr> GetAllTypes(ObjectCategoryBits category) const
    {
        return GetAllTypes(category, true, true);
    }

    virtual BASE_NS::shared_ptr<const IObjectFactory> GetObjectFactory(const ObjectId& uid) const = 0;

    /**
     * @brief Returns a type information list of all registered object types for a given category.
     * @param category The category to list. If category is ObjectCategoryBits::ANY, all
     *        registered object type infos are returned, regardless of category.
     * @param strict If true, all of the given category bits much be set for a type to be returned
     *        in the list. If false, if any of the category bits are set for a type it will be added
     *        to the list.
     */
    virtual BASE_NS::vector<IClassInfo::ConstPtr> GetAllTypes(
        ObjectCategoryBits category, bool strict, bool excludeDeprecated) const = 0;
    /**
     * @brief Returns an object instance with the given instance id.
     * @param uid Instance id of the object to return.
     * @return Object or nullptr if uid is does not point to a valid object.
     */
    virtual BASE_NS::shared_ptr<IObject> GetObjectInstanceByInstanceId(InstanceId uid) const = 0;
    /**
     * @brief Returns a list of all objects which are currently alive.
     * @note  The list includes both regular and singleton objects.
     */
    virtual BASE_NS::vector<BASE_NS::shared_ptr<IObject>> GetAllObjectInstances() const = 0;
    /**
     * @brief Returns a list of all singleton objects which are currently alive.
     */
    virtual BASE_NS::vector<BASE_NS::shared_ptr<IObject>> GetAllSingletonObjectInstances() const = 0;

    /**
     * @brief Remove any references to dead instances
     */
    virtual void Purge() = 0;

    /**
     * @brief Tell object registry that object with given uid is not used any more and so references can be removed
     */
    virtual void DisposeObject(const InstanceId&) const = 0;

    /**
     * @brief Construct empty default metadata structure
     */
    virtual BASE_NS::shared_ptr<IMetadata> ConstructMetadata() const = 0;

    /**
     * @brief Construct empty default call context structure
     */
    virtual BASE_NS::shared_ptr<ICallContext> ConstructDefaultCallContext() const = 0;

    /**
     * @brief Returns a list of all objects which are currently alive and belong to a all specified object
     *        categories.
     * @param category The category to return.
     */
    BASE_NS::vector<BASE_NS::shared_ptr<IObject>> GetObjectInstancesByCategory(ObjectCategoryBits category) const
    {
        return GetObjectInstancesByCategory(category, true);
    }
    /**
     * @brief Returns a list of all objects which are currently alive and belong to a specified object
     *        category.
     * @param category The category to return.
     * @param strict If true, all of the given category bits much be set for a type to be returned
     *        in the list. If false, if any of the category bits are set for a type it will be added
     *        to the list.
     */
    virtual BASE_NS::vector<BASE_NS::shared_ptr<IObject>> GetObjectInstancesByCategory(
        ObjectCategoryBits category, bool strict) const = 0;
    /**
     * @brief Returns a string dump of all objects which are currently alive and created through
     *        this object registry instance.
     * @param exporter The exporter to use.
     */
    virtual BASE_NS::string ExportToString(const IObjectRegistryExporter::Ptr& exporter) const = 0;
    /**
     * @brief Returns the default object context.
     */
    virtual BASE_NS::shared_ptr<IObjectContext> GetDefaultObjectContext() const = 0;

    // Templated helper method, Creates object by UID and returns the requested Interface (if available)
    template<typename Interface>
    typename Interface::Ptr Create(ObjectId uid) const
    {
        auto p = Create(uid);
        if (p) {
            return interface_pointer_cast<Interface>(p);
        }
        return nullptr;
    }
    template<typename Interface>
    typename Interface::Ptr Create(ObjectId uid, BASE_NS::shared_ptr<IMetadata> data) const
    {
        auto p = Create(META_NS::ClassInfo { uid }, data);
        if (p) {
            return interface_pointer_cast<Interface>(p);
        }
        return nullptr;
    }
    template<typename Interface>
    typename Interface::Ptr Create(ObjectId uid, InstanceId instanceid) const
    {
        auto p = Create(uid, instanceid);
        if (p) {
            return interface_pointer_cast<Interface>(p);
        }
        return nullptr;
    }
    template<typename Interface>
    typename Interface::Ptr Create(ObjectId uid, const CreateInfo& createInfo) const
    {
        auto p = Create(uid, createInfo);
        if (p) {
            return interface_pointer_cast<Interface>(p);
        }
        return nullptr;
    }

    /**
     * @brief Registers an interpolator class for a property type. Any previously registered interpolators
     *        for the given property type are unregistered.
     *        After registration, an interpolator for a property type can be created by calling one of the
     *        CreateInterpolator methods.
     * @param propertyTypeUid Property type.
     * @param interpolatorClassUid Interpolator class id.
     */
    virtual void RegisterInterpolator(TypeId propertyTypeUid, BASE_NS::Uid interpolatorClassUid) = 0;
    /**
     * @brief Unregisters an interpolator for a given property type.
     * @param propertyTypeUid The property type to unregister.
     */
    virtual void UnregisterInterpolator(TypeId propertyTypeUid) = 0;
    /**
     * @brief Returns true if an interpolator has been registered for the given property type.
     * @note  This function checks if a type-specific interpolator has been registered, and ignores
     *        the default interpolator.
     * @param propertyTypeUid Property type to check.
     */
    virtual bool HasInterpolator(TypeId propertyTypeUid) const = 0;
    /**
     * @brief Creates an interpolator object for given property type.
     * @param propertyTypeUid The property type.
     * @note  If an interpolator for the given property type has not been registered, a default
     *        interpolator is returned. This default interpolator just steps the property value
     *        without interpolation.
     * @return Interpolator object or the default interpolator
     */
    virtual BASE_NS::shared_ptr<IInterpolator> CreateInterpolator(TypeId propertyTypeUid) = 0;
    /**
     * @brief Creates an interpolator object for given property, based on its type.
     * @param property The property to create interpolator for.
     * @return Interpolator object or null if an interpolator could not be created.
     */
    BASE_NS::shared_ptr<IInterpolator> CreateInterpolator(const IProperty::ConstPtr& property)
    {
        if (!property) {
            return {};
        }
        return CreateInterpolator(property->GetTypeId());
    }
    /**
     * @brief See HasInterpolator(BASE_NS::Uid propertyTypeUid)
     */
    bool HasInterpolator(const IProperty::ConstPtr& property) const
    {
        if (!property) {
            return false;
        }
        return HasInterpolator(property->GetTypeId());
    }
    /**
     * @brief Returns the class registry instance used by the object registry instance.
     */
    virtual IClassRegistry& GetClassRegistry() = 0;
    virtual IPropertyRegister& GetPropertyRegister() = 0;
    virtual IGlobalSerializationData& GetGlobalSerializationData() = 0;

    virtual IEngineData& GetEngineData() = 0;
};

/** Returns a reference to the IWidgetRegistry instance. InitializeToolkit or InitializeToolKitAndEngine must be called
 *  before calling this function. */
inline META_NS::IObjectRegistry& GetObjectRegistry()
{
    return GetMetaObjectLib().GetObjectRegistry();
}

/**
 * @brief Retuns the global class registry instance.
 */
inline META_NS::IClassRegistry& GetClassRegistry()
{
    return GetObjectRegistry().GetClassRegistry();
}

/**
 * @brief Returns the default context object from the global object registry.
 */
inline BASE_NS::shared_ptr<IObjectContext> GetDefaultObjectContext()
{
    return META_NS::GetObjectRegistry().GetDefaultObjectContext();
}

/**
 * @brief Registers Type to the default object registry.
 */
template<typename Type>
constexpr void RegisterObjectType()
{
    META_NS::GetObjectRegistry().RegisterObjectType<Type>();
}

/**
 * @brief Unregisters Type from default object registry.
 */
template<typename Type>
constexpr void UnregisterObjectType()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<Type>();
}

META_END_NAMESPACE()

#endif
