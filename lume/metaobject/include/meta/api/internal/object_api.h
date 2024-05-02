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

#ifndef META_API_INTERNAL_OBJECT_API_H
#define META_API_INTERNAL_OBJECT_API_H

#include <assert.h>

#include <base/containers/type_traits.h>
#include <base/util/uid_util.h>

#include <meta/api/property/binding.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property.h>

// Fix compiler error due to naming clash with windows headers
#if defined(GetClassName)
#undef GetClassName
#endif

META_BEGIN_NAMESPACE()

namespace Internal {

#define META_INTERFACE_API(ClassName)                                                                      \
private:                                                                                                   \
    using FinalClassType = FinalClass;                                                                     \
                                                                                                           \
public:                                                                                                    \
    using META_NS::Object::operator=;                                                                      \
    ClassName() noexcept = default;                                                                        \
    ~ClassName() override = default;                                                                       \
    ClassName(const ClassName& other) noexcept /* NOLINT(bugprone-copy-constructor-init)*/                 \
    {                                                                                                      \
        META_NS::Object::Initialize(other);                                                                \
    }                                                                                                      \
    ClassName(ClassName&& other) noexcept                                                                  \
    {                                                                                                      \
        META_NS::Object::Initialize(other);                                                                \
        other.ResetIObject();                                                                              \
    }                                                                                                      \
    explicit ClassName(const META_NS::IObject::Ptr& other)                                                 \
    {                                                                                                      \
        META_NS::Object::Initialize(other);                                                                \
    }                                                                                                      \
    ClassName& operator=(const ClassName& other) noexcept                                                  \
    {                                                                                                      \
        return static_cast<ClassName&>(META_NS::Object::operator=(other));                                 \
    }                                                                                                      \
    ClassName& operator=(ClassName&& other) noexcept                                                       \
    {                                                                                                      \
        return static_cast<ClassName&>(META_NS::Object::operator=(static_cast<META_NS::Object&&>(other))); \
    }

#define META_API(ClassName)                                                                                \
private:                                                                                                   \
    using FinalClassType = ClassName;                                                                      \
                                                                                                           \
public:                                                                                                    \
    using META_NS::Object::operator=;                                                                      \
    ClassName() noexcept = default;                                                                        \
    ~ClassName() override = default;                                                                       \
    ClassName(const ClassName& other) noexcept /* NOLINT(bugprone-copy-constructor-init)*/                 \
    {                                                                                                      \
        META_NS::Object::Initialize(other);                                                                \
    }                                                                                                      \
    ClassName(ClassName&& other) noexcept                                                                  \
    {                                                                                                      \
        META_NS::Object::Initialize(other);                                                                \
        other.ResetIObject();                                                                              \
    }                                                                                                      \
    explicit ClassName(const META_NS::IObject::Ptr& other)                                                 \
    {                                                                                                      \
        META_NS::Object::Initialize(other);                                                                \
    }                                                                                                      \
    ClassName& operator=(const ClassName& other) noexcept                                                  \
    {                                                                                                      \
        return static_cast<ClassName&>(META_NS::Object::operator=(other));                                 \
    }                                                                                                      \
    ClassName& operator=(ClassName&& other) noexcept                                                       \
    {                                                                                                      \
        return static_cast<ClassName&>(META_NS::Object::operator=(static_cast<META_NS::Object&&>(other))); \
    }

/** Caches an interface pointer to InterfaceType with a unique Name identifier */
#define META_API_CACHE_INTERFACE(InterfaceType, Name)            \
protected:                                                       \
    InterfaceType* Get##Name##Interface() const noexcept         \
    {                                                            \
        return interface_cast<InterfaceType>(this->ObjectRef()); \
    }

/** Returns the interface with given Name cached using META_API_CACHE_INTERFACE */
#define META_API_CACHED_INTERFACE(Name) Get##Name##Interface()

/** Defines an implicit conversion to desired interface pointer */
#define META_API_OBJECT_CONVERTIBLE(ConvertibleType)                       \
public:                                                                    \
    inline operator ConvertibleType::Ptr() const noexcept                  \
    {                                                                      \
        return interface_pointer_cast<ConvertibleType>(this->ObjectRef()); \
    }                                                                      \
    inline operator ConvertibleType::ConstPtr() const noexcept             \
    {                                                                      \
        return interface_pointer_cast<ConvertibleType>(this->ObjectRef()); \
    }                                                                      \
    inline operator ConvertibleType::ConstWeakPtr() const noexcept         \
    {                                                                      \
        return interface_pointer_cast<ConvertibleType>(this->ObjectRef()); \
    }                                                                      \
    inline operator ConvertibleType::WeakPtr() const noexcept              \
    {                                                                      \
        return interface_pointer_cast<ConvertibleType>(this->ObjectRef()); \
    }                                                                      \
                                                                           \
private:

} // namespace Internal

/**
 * @brief The ObjectBase class is the base class for all IObject-derived high level API objects.
 */
class Object {
    META_API_OBJECT_CONVERTIBLE(META_NS::IMetadata)
    META_API_CACHE_INTERFACE(META_NS::IMetadata, Metadata)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAttach)
    META_API_CACHE_INTERFACE(META_NS::IAttach, Attach)

public:
    Object() = default;
    virtual ~Object() = default;
    // Base object accepts any IObject
    Object(const Object& other) noexcept
    {
        SetObjectRef(other);
    }
    Object(Object&& other) noexcept
    {
        SetObjectRef(other);
        other.ResetIObject();
    }
    explicit Object(const IObject::Ptr& other) noexcept
    {
        SetObjectRef(other);
    }
    /**
     * @brief Initializes this object with a given IObject::Ptr.
     * @note Initialize can only be called once on an uninitialized object. Any
     *       subsequent initialize calls will fail.
     * @param ptr The IObject::Ptr to initialize from.
     * @return true if initiazation succeed (class id of ptr matches
     *         what this object expects), false otherwise.
     */
    bool Initialize(const META_NS::IObject::Ptr& ptr)
    {
        if (object_) {
            CORE_LOG_E("%s: Object already initialized. Initialize can only be called on an uninitialized object",
                GetClassName().data());
            return false;
        }
        if (IsCompatible(ptr)) {
            SetObjectRef(ptr);
            return true;
        }
        const BASE_NS::string_view className = ptr ? ptr->GetClassName() : "<null>";
        CORE_LOG_E("%s: Cannot initialize with an instance of '%s'", GetClassName().data(), className.data());
        return false;
    }
    /**
     * @brief Resets the underlying IObject::Ptr, effectively returning this object to its
     *        initial state.
     */
    void ResetIObject()
    {
        SetObjectRef({});
    }
    /**
     * @brief Meta::IObject::Ptr conversion operator.
     */
    operator const META_NS::IObject::Ptr&() const noexcept
    {
        return ObjectRef();
    }
    /**
     * @brief Meta::IObject::ConstPtr conversion operator.
     */
    operator META_NS::IObject::ConstPtr() const noexcept
    {
        return ObjectRef();
    }
    /**
     * @brief Meta::IObject::WeakPtr conversion operator.
     */
    operator META_NS::IObject::WeakPtr() const noexcept
    {
        return ObjectRef();
    }
    /**
     * @brief Meta::IObject::ConstWeakPtr conversion operator.
     */
    operator META_NS::IObject::ConstWeakPtr() const noexcept
    {
        return ObjectRef();
    }
    /**
     * @brief Copy assignment operator
     */
    Object& operator=(const Object& other) noexcept
    {
        if (IsCompatible(other)) {
            SetObjectRef(other.ObjectRef());
        } else {
            CORE_LOG_E(
                "%s: Cannot assign to with an instance of '%s'", GetClassName().data(), other.GetClassName().data());
        }
        return *this;
    }
    /**
     * @brief Move assignment operator
     */
    Object& operator=(Object&& other) noexcept
    {
        if (IsCompatible(other)) {
            SetObjectRef(other.ObjectRef());
        } else {
            ResetIObject();
            CORE_LOG_E(
                "%s: Cannot assign to with an instance of '%s'", GetClassName().data(), other.GetClassName().data());
        }
        other.ResetIObject();
        return *this;
    }
    /**
     * @brief Boolean operator, returns true if the contained object is valid.
     */
    explicit operator bool() const noexcept
    {
        return object_.operator bool();
    }
    /**
     * @brief Equality operator.
     * @note Only the underlying object references are compared.
     */
    bool operator==(const Object& other) const noexcept
    {
        return object_ == other.object_;
    }
    /**
     * @brief Equality operator.
     * @note Only the underlying object references are compared.
     */
    bool operator!=(const Object& other) const noexcept
    {
        return !operator==(other);
    }
    /**
     * @brief Return class id of the underlying object
     */
    virtual ObjectId GetClassId() const noexcept
    {
        return object_ ? object_->GetClassId() : ClassId::Object;
    }
    /**
     * @brief Return class id of the underlying object
     */
    virtual BASE_NS::string_view GetClassName() const noexcept
    {
        return object_ ? object_->GetClassName() : ClassId::Object.Name();
    }
    /**
     * @brief Returns the contained IObject::Ptr.
     */
    inline META_NS::IObject::Ptr GetIObject() const noexcept
    {
        return ObjectRef();
    }
    /**
     * @brief Returns true if given object is compatible with this object.
     * @param ptr The IObject to compare against.
     */
    virtual bool IsCompatible(const META_NS::IObject::ConstPtr& ptr) const noexcept
    {
        // Base Object is compatible with any IObject
        return true;
    }
    /**
     * @brief Returns true if given object is compatible with this object.
     * @param ptr The Object to compare against.
     */
    virtual bool IsCompatible(const Object& object) const noexcept
    {
        return true;
    }
    /**
     * @brief A convenience method for getting an typed interface pointer from a high level API object.
     * @return
     */
    template<class T>
    constexpr const typename T::Ptr GetInterfacePtr() const noexcept
    {
        return interface_pointer_cast<T>(ObjectRef());
    }
    /**
     * @brief A convenience method for getting an typed interface pointer from a high level API object.
     * @return
     */
    template<class T>
    constexpr T* GetInterface() const noexcept
    {
        return interface_cast<T>(ObjectRef());
    }
    /**
     * @brief Adds a property to the metadata of this object, while enabling
     *        the builder pattern for object properties.
     * @param property The property to add.
     */
    Object& MetaProperty(const IProperty::Ptr& property)
    {
        AddProperty(property);
        return *this;
    }
    /**
     * @brief Adds a property to the metadata of this object.
     * @param property The property to add.
     */
    void AddProperty(const IProperty::Ptr& property)
    {
        META_API_CACHED_INTERFACE(Metadata)->AddProperty(property);
    }
    /**
     * @brief Removes a property from the metadata of this object.
     * @param property The property to remove.
     */
    Object& RemoveProperty(const META_NS::IProperty::Ptr& property)
    {
        META_API_CACHED_INTERFACE(Metadata)->RemoveProperty(property);
        return *this;
    }
    /**
     * @brief Helper template for adding a property of specific name and value.
     */
    template<typename PropertyValueType>
    Object& MetaProperty(BASE_NS::string_view propertyName, const PropertyValueType& propertyValue)
    {
        const auto meta = META_API_CACHED_INTERFACE(Metadata);
        if (const auto existing = meta->GetPropertyByName(propertyName)) {
            if (Property<PropertyValueType> typed { existing }) {
                typed->SetValue(propertyValue);
            } else {
                CORE_LOG_E("%s: Type mismatch on property '%s'", object_->GetClassName().data(), propertyName.data());
            }
        } else {
            meta->AddProperty(ConstructProperty<PropertyValueType>(propertyName, propertyValue));
        }
        return *this;
    }
    /**
     * @brief Helper template for adding an array property of specific name and value.
     */
    template<typename PropertyValueType>
    Object& ArrayMetaProperty(
        BASE_NS::string_view propertyName, BASE_NS::array_view<const PropertyValueType> arrayValue)
    {
        const auto meta = META_API_CACHED_INTERFACE(Metadata);
        if (const auto existing = meta->GetPropertyByName(propertyName)) {
            if (auto typed = ArrayProperty<PropertyValueType>(existing)) {
                typed->SetValue(arrayValue);
            } else {
                CORE_LOG_E(
                    "%s: Type mismatch on array property '%s'", object_->GetClassName().data(), propertyName.data());
            }
        } else {
            meta->AddProperty(META_NS::ConstructArrayProperty<PropertyValueType>(propertyName, arrayValue));
        }
        return *this;
    }
    /**
     * @brief Returns a reference to the meta data interface of this object.
     */
    META_NS::IMetadata& Metadata()
    {
        return *META_API_CACHED_INTERFACE(Metadata);
    }
    /**
     * @brief Attach an attachment to this object.
     */
    auto& Attach(const META_NS::IObject::Ptr& attachment, const META_NS::IObject::Ptr& dataContext = {})
    {
        META_API_CACHED_INTERFACE(Attach)->Attach(attachment, dataContext);
        return *this;
    }
    template<class T, class U>
    auto& Attach(const T& attachment, const U& dataContext = {})
    {
        return Attach(interface_pointer_cast<IObject>(attachment), interface_pointer_cast<IObject>(dataContext));
    }
    template<class T>
    auto& Attach(const T& attachment)
    {
        return Attach(interface_pointer_cast<IObject>(attachment), {});
    }
    /**
     * @brief Detach an attachment from this object.
     */
    auto& Detach(const META_NS::IObject::Ptr& attachment)
    {
        META_API_CACHED_INTERFACE(Attach)->Detach(attachment);
        return *this;
    }
    template<class T>
    auto& Detach(const T& attachment)
    {
        return Detach(interface_pointer_cast<IObject>(attachment));
    }
    /**
     * @brief Return all attachments matching the constraints.
     */
    BASE_NS::vector<META_NS::IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids = {}, bool strict = false)
    {
        return META_API_CACHED_INTERFACE(Attach)->GetAttachments(uids, strict);
    }
    /**
     * @brief Return a list of attachments which implement T::UID.
     */
    template<class T>
    BASE_NS::vector<typename T::Ptr> GetAttachments() const
    {
        return META_API_CACHED_INTERFACE(Attach)->GetAttachments<T>();
    }

protected:
    /** Returns a reference to the contained IObject::Ptr */
    const META_NS::IObject::Ptr& ObjectRef() const noexcept
    {
        if (!object_) {
            CreateObject();
        }
        return object_;
    }
    /** Sets the internal object reference */
    void SetObjectRef(const IObject::Ptr& to) const noexcept
    {
        if (object_ != to) {
            object_ = to;
        }
    }
    /** Creates the underlying object, object_ should be valid after this call */
    virtual void CreateObject() const noexcept
    {
        SetObjectRef(META_NS::GetObjectRegistry().Create(ClassId::Object));
        BASE_ASSERT(object_);
    }

private:
    mutable META_NS::IObject::Ptr object_;
};

namespace Internal {

template<class FinalClass, const META_NS::ClassInfo& Class>
class ObjectInterfaceAPI : public META_NS::Object {
    using FinalClassType = FinalClass;
    using Super = Object;

public:
    ObjectInterfaceAPI() = default;
    ~ObjectInterfaceAPI() override = default;
    /**
     * @brief Stores the contained object into a given API class.
     * @param ptr The target object. The target must be uninitialized.
     */
    constexpr FinalClassType& Store(FinalClassType& ptr)
    {
        auto& me = static_cast<FinalClassType&>(*this);
        ptr = me;
        return me;
    }
    /**
     * @brief Returns the class id of the contained object this class supports.
     */
    ObjectId GetClassId() const noexcept override
    {
        return Class.Id();
    }
    /**
     * @brief Returns the class name of the contained object
     */
    BASE_NS::string_view GetClassName() const noexcept override
    {
        return Class.Name();
    }
    /**
     * @brief Returns true if this class is compatible with the given class id.
     * @param id The class id to check against.
     */
    bool IsCompatible(const ObjectId& id) const noexcept
    {
        return GetClassId() == id;
    }
    /**
     * @brief See Object::IsCompatible.
     */
    bool IsCompatible(const META_NS::IObject::ConstPtr& ptr) const noexcept override
    {
        if (!ptr) {
            return true;
        }
        return GetClassId() == ptr->GetClassId();
    }
    /**
     * @brief See Object::IsCompatible.
     */
    bool IsCompatible(const Object& object) const noexcept override
    {
        if (!object) {
            return true;
        }
        return object.GetClassId() == Class.Id();
    }

    FinalClassType& Attach(const META_NS::IAttachment::Ptr& attachment, const META_NS::IObject::Ptr& dataContext = {})
    {
        Object::Attach(attachment, dataContext);
        return static_cast<FinalClassType&>(*this);
    }
    FinalClassType& Detach(const META_NS::IAttachment::Ptr& attachment)
    {
        Object::Detach(attachment);
        return static_cast<FinalClassType&>(*this);
    }

protected:
    /** Creates the contained object */
    static META_NS::IObject::Ptr Create()
    {
        return META_NS::GetObjectRegistry().Create(Class);
    }

private:
    void CreateObject() const noexcept override
    {
        // Make sure that we call the Create method of the derived type,
        // not necessarily the base implementation.
        if (const auto object = FinalClassType::Create()) {
            SetObjectRef(object);
        } else {
            CORE_LOG_E("%s: Cannot create instance", Class.Name().data());
        }
    }
};

/** Creates a property forwarder using the interface cached with META_API_CACHE_INTERFACE */
#define META_API_INTERFACE_PROPERTY_CACHED(InterfaceCachedName, PropertyName, PropertyType)    \
    inline META_NS::Property<PropertyType> PropertyName() noexcept                             \
    {                                                                                          \
        return META_API_CACHED_INTERFACE(InterfaceCachedName)->PropertyName();                 \
    }                                                                                          \
    inline FinalClassType& PropertyName(const PropertyType& value)                             \
    {                                                                                          \
        PropertyName()->SetValue(value);                                                       \
        return static_cast<FinalClassType&>(*this);                                            \
    }                                                                                          \
    inline FinalClassType& PropertyName(const META_NS::IProperty::Ptr& property)               \
    {                                                                                          \
        PropertyName()->SetBind(property);                                                     \
        return static_cast<FinalClassType&>(*this);                                            \
    }                                                                                          \
    inline FinalClassType& PropertyName(const META_NS::Property<const PropertyType>& property) \
    {                                                                                          \
        PropertyName()->SetBind(property);                                                     \
        return static_cast<FinalClassType&>(*this);                                            \
    }                                                                                          \
    inline FinalClassType& PropertyName(const META_NS::Property<PropertyType>& property)       \
    {                                                                                          \
        PropertyName()->SetBind(property);                                                     \
        return static_cast<FinalClassType&>(*this);                                            \
    }                                                                                          \
    inline FinalClassType& PropertyName(const META_NS::IFunction::ConstPtr& function)          \
    {                                                                                          \
        PropertyName()->SetBind(function);                                                     \
        return static_cast<FinalClassType&>(*this);                                            \
    }                                                                                          \
    inline FinalClassType& PropertyName(META_NS::Binding&& binding)                            \
    {                                                                                          \
        binding.MakeBind(PropertyName());                                                      \
        return static_cast<FinalClassType&>(*this);                                            \
    }

/** Creates a read-only property forwarder using the interface cached with META_API_CACHE_INTERFACE */
#define META_API_INTERFACE_READONLY_PROPERTY_CACHED(InterfaceCachedName, PropertyName, PropertyType) \
    META_NS::Property<const PropertyType> PropertyName() const                                       \
    {                                                                                                \
        return META_API_CACHED_INTERFACE(InterfaceCachedName)->PropertyName();                       \
    }

/** Creates a property forwarder using the interface cached with META_API_CACHE_INTERFACE */
#define META_API_INTERFACE_ARRAY_PROPERTY_CACHED(InterfaceCachedName, PropertyName, PropertyType) \
    inline META_NS::ArrayProperty<PropertyType> PropertyName() noexcept                           \
    {                                                                                             \
        return META_API_CACHED_INTERFACE(InterfaceCachedName)->PropertyName();                    \
    }                                                                                             \
    inline FinalClassType& PropertyName(                                                          \
        META_NS::ArrayProperty<PropertyType>::IndexType index, const PropertyType& value)         \
    {                                                                                             \
        PropertyName()->SetValueAt(index, value);                                                 \
        return static_cast<FinalClassType&>(*this);                                               \
    }                                                                                             \
    inline FinalClassType& PropertyName(const BASE_NS::vector<PropertyType>& value)               \
    {                                                                                             \
        PropertyName()->SetValue(value);                                                          \
        return static_cast<FinalClassType&>(*this);                                               \
    }                                                                                             \
    inline FinalClassType& PropertyName(const META_NS::ArrayProperty<PropertyType>& property)     \
    {                                                                                             \
        PropertyName()->SetBind(property);                                                        \
        return static_cast<FinalClassType&>(*this);                                               \
    }                                                                                             \
    inline FinalClassType& PropertyName(const META_NS::IFunction::ConstPtr& function)             \
    {                                                                                             \
        PropertyName()->SetBind(function);                                                        \
        return static_cast<FinalClassType&>(*this);                                               \
    }                                                                                             \
    inline FinalClassType& PropertyName(META_NS::Binding&& binding)                               \
    {                                                                                             \
        binding.MakeBind(PropertyName().GetProperty());                                           \
        return static_cast<FinalClassType&>(*this);                                               \
    }

/** Creates a property forwarder using the interface cached with META_API_CACHE_INTERFACE */
#define META_API_INTERFACE_READONLY_ARRAY_PROPERTY_CACHED(InterfaceCachedName, PropertyName, PropertyType) \
    META_NS::ArrayProperty<const PropertyType> PropertyName() const                                        \
    {                                                                                                      \
        return META_API_CACHED_INTERFACE(InterfaceCachedName)->PropertyName();                             \
    }

/** Creates a forwarder for a property defined in a specific interface */
#define META_API_INTERFACE_PROPERTY(Interface, PropertyName, PropertyType)             \
    META_NS::Property<PropertyType> PropertyName()                                     \
    {                                                                                  \
        auto o = interface_cast<Interface>(ObjectRef());                               \
        assert(o);                                                                     \
        return o->PropertyName();                                                      \
    }                                                                                  \
    FinalClassType& PropertyName(const PropertyType& value)                            \
    {                                                                                  \
        if (auto o = interface_cast<Interface>(ObjectRef())) {                         \
            o->PropertyName()->SetValue(value);                                        \
        }                                                                              \
        return static_cast<FinalClassType&>(*this);                                    \
    }                                                                                  \
    FinalClassType& PropertyName(const META_NS::Property<const PropertyType>& binding) \
    {                                                                                  \
        if (auto o = interface_cast<Interface>(ObjectRef())) {                         \
            o->PropertyName()->SetBind(binding);                                       \
        }                                                                              \
        return static_cast<FinalClassType&>(*this);                                    \
    }

/** Creates a forwarder for a read-only property defined in a specific interface */
#define META_API_INTERFACE_READONLY_PROPERTY(Interface, PropertyName, PropertyType) \
    const META_NS::Property<const PropertyType> PropertyName() const                \
    {                                                                               \
        auto o = interface_cast<Interface>(ObjectRef());                            \
        assert(o);                                                                  \
        return o->PropertyName();                                                   \
    }

} // namespace Internal

// NOLINTBEGIN(readability-identifier-naming) to keep std like syntax

using ::interface_cast;
using ::interface_pointer_cast;

/**
 * @brief interface_pointer_cast implementation for high level objects
 */
template<class T>
BASE_NS::shared_ptr<T> interface_pointer_cast(const META_NS::Object& object)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    return object.GetInterfacePtr<T>();
}

/**
 * @brief interface_cast implementation for high level objects
 */
template<class T>
T* interface_cast(const META_NS::Object& object)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    return object.GetInterface<T>();
}

// NOLINTEND(readability-identifier-naming) to keep std like syntax

META_END_NAMESPACE()

#endif
