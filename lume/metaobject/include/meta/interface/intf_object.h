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

#ifndef META_INTERFACE_IOBJECT_H
#define META_INTERFACE_IOBJECT_H

#include <core/plugin/intf_interface.h>

#include <meta/base/bit_field.h>
#include <meta/base/ids.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

class IObjectRegistry;
class RefUri;
META_REGISTER_INTERFACE(IObject, "50c06aa7-0db7-435b-9c6f-c15cac8ef5c7")

/**
 * @brief The IObject interface defines an object that can contain bindable properties and functions
 *        within meta object library scope. Such objects also provide a queryable metadata which can
 *        be used for enumerating all the properties and functions exposed by a given object.
 *
 * @note  To implement IObject interface in a custom class, use (Base/Meta)ObjectFwd,
 *        found in meta/ext/object.h.
 */
class IObject : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObject)
public:
    /**
     * @brief Returns the UId of the class which implements this interface.
     */
    virtual ObjectId GetClassId() const = 0;
    /**
     * @brief Returns the name of the class which implements this interface.
     */
    virtual BASE_NS::string_view GetClassName() const = 0;
    /**
     * @brief Returns the name of this object, as default this is the instance id.
     */
    virtual BASE_NS::string GetName() const = 0;
    /**
     * @brief Get uids of interfaces this object implements
     */
    virtual BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const = 0;
    /**
     * @brief Tries to find the last object the uri points to using this object as base.
     */
    IObject::Ptr Resolve(const RefUri& uri) const;
    /**
     * @brief Get shared ptr to top most self, notice that this might be different object than "this".
     * Note: Works by default only for objects constructed via ObjectRegistry
     */
    IObject::Ptr GetSelf() const;
};

class IObjectInstance : public IObject {
    META_INTERFACE(IObject, IObjectInstance, "c64b029b-1617-449f-8489-d1dd1a3c3227")
public:
    /**
     * @brief Return the framework assigned instance UID for the object instance.
     */
    virtual InstanceId GetInstanceId() const = 0;
    /**
     * @brief Tries to find the last object the uri points to using this object as base.
     */
    virtual IObject::Ptr Resolve(const RefUri& uri) const = 0;

    template<typename Interface>
    typename Interface::Ptr Resolve(const RefUri& uri) const
    {
        return interface_pointer_cast<Interface>(Resolve(uri));
    }
    /**
     * @brief Get shared ptr to top most self, notice that this might be different object than "this".
     * Note: Works by default only for objects constructed via ObjectRegistry
     */
    virtual IObject::Ptr GetSelf() const = 0;

    template<typename Interface>
    typename Interface::Ptr GetSelf() const
    {
        return interface_pointer_cast<Interface>(GetSelf());
    }
};

inline IObject::Ptr IObject::Resolve(const RefUri& uri) const
{
    if (auto oi = GetInterface<IObjectInstance>()) {
        return oi->Resolve(uri);
    }
    CORE_LOG_E("Called Resolve for non-IObjectInstance");
    return nullptr;
}

inline IObject::Ptr IObject::GetSelf() const
{
    if (auto oi = GetInterface<IObjectInstance>()) {
        return oi->GetSelf();
    }
    CORE_LOG_E("Called GetSelf for non-IObjectInstance");
    return nullptr;
}

template<typename T, typename = BASE_NS::enable_if<IsInterfacePtr_v<T>>>
IObject::Ptr GetSelf(const T& object)
{
    if (auto o = interface_cast<IObjectInstance>(object)) {
        return o->GetSelf();
    }
    return nullptr;
}

template<typename Intf, typename T, typename = BASE_NS::enable_if<IsInterfacePtr_v<T>>>
typename Intf::Ptr GetSelf(const T& object)
{
    if (auto o = interface_cast<IObjectInstance>(object)) {
        return o->template GetSelf<Intf>();
    }
    return nullptr;
}

template<typename T, typename = BASE_NS::enable_if<IsInterfacePtr_v<T>>>
IObject::Ptr Resolve(const T& object, const RefUri& uri)
{
    if (auto o = interface_cast<IObjectInstance>(object)) {
        return o->Resolve(uri);
    }
    return nullptr;
}

template<typename Intf, typename T, typename = BASE_NS::enable_if<IsInterfacePtr_v<T>>>
typename Intf::Ptr Resolve(const T& object, const RefUri& uri)
{
    if (auto o = interface_cast<IObjectInstance>(object)) {
        return o->template Resolve<Intf>(uri);
    }
    return nullptr;
}

META_INTERFACE_TYPE(IObject);
META_INTERFACE_TYPE(IObjectInstance);

META_END_NAMESPACE()

#endif
