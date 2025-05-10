/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of IProxyObject interface
 * Author: Lauri Jaaskela
 * Create: 2022-10-10
 */

#ifndef META_INTERFACE_IPROXY_OBJECT_H
#define META_INTERFACE_IPROXY_OBJECT_H

#include <meta/base/bit_field.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IProxyObject, "0bb4f08c-bdec-408e-b6f0-0fb19c2d2bf6")

enum class ProxyMode : uint8_t { REFLECT_PROXY_HIERARCHY = 1 };

using ProxyModeBitsValue = EnumBitField<ProxyMode>;

/**
 * @brief The IProxyObject interface defines an interface for a proxy object which
 *        exposes all of the properties and functions of its target object as its own
 *        properties/functions by default.
 *        Any changes to the property values are reflected in the target object as
 *        long as a property is not overriden.
 */
class IProxyObject : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IProxyObject)
public:
    META_PROPERTY(ProxyModeBitsValue, Mode)
    /**
     * @brief If true, the proxy object will dynamically reflect any metadata changes
     *        in the target object. If false, the target object's metadata is reflected
     *        in the proxy object only at the time of setting the target.
     *        The default value is true.
     */
    META_PROPERTY(bool, Dynamic)
    /**
     * @brief Returns the current target object.
     */
    virtual const IObject::Ptr GetTarget() const = 0;
    /**
     * @brief Sets the target object for this proxy.
     * @param target The target object for proxying.
     */
    virtual bool SetTarget(const IObject::Ptr& target) = 0;
    /**
     * @brief Returns a list of properties which have been locally overridden
     *        in the proxy object.
     */
    virtual BASE_NS::vector<IProperty::ConstPtr> GetOverrides() const = 0;
    /**
     * @brief Returns property if it has been locally overridden in the proxy object
     **/
    virtual IProperty::ConstPtr GetOverride(BASE_NS::string_view) const = 0;
    /**
     * @brief Sets target property to proxy for individual property. Returns the proxying property.
     */
    virtual IProperty::Ptr SetPropertyTarget(const IProperty::Ptr& property) = 0;
    /**
     * @brief returns the property if it is a proxy
     */
    virtual IProperty::ConstPtr GetProxyProperty(BASE_NS::string_view) const = 0;
    /**
     * @brief Default value binds object's property to another object's source property instead to target's property
     * with same name. If property does not exists yet default value binding is created when property is requested and
     * if source property exists in target object
     */
    virtual void AddInternalProxy(BASE_NS::string_view proxyPropertyName, BASE_NS::string_view sourcePropertyName) = 0;
};

META_END_NAMESPACE()

META_TYPE(META_NS::IProxyObject::Ptr);
META_TYPE(META_NS::IProxyObject::WeakPtr);
META_TYPE(META_NS::ProxyModeBitsValue)

#endif
