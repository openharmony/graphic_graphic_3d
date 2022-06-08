/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_PLUGIN_ICLASS_FACTORY_H
#define API_CORE_PLUGIN_ICLASS_FACTORY_H

#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>

CORE_BEGIN_NAMESPACE()
class IClassFactory : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "3a4cad5c-0e16-4708-bd83-626d136a7215" };

    /** Create a new interface instance by UID */
    virtual IInterface::Ptr CreateInstance(const BASE_NS::Uid& uid) = 0;
};

/** Create interface from specified interface registry */
template<class T>
auto CreateInstance(IClassFactory& factory, const BASE_NS::Uid& uid)
{
    // Create the instance
    if (auto res = factory.CreateInstance(uid)) {
        // Ask for the interface.
        return typename T::Ptr(res->GetInterface<T>());
    }
    return typename T::Ptr();
}

/** Creates a class instance by using a specific IClassFactory instance from global registry.
    and tries to get the requested interface from it.
    Returns empty T::Ptr (null) if the class does not support the interface (or the class could not be created).
*/
template<class T>
typename T::Ptr CreateInstance(const BASE_NS::Uid& factory_id, const BASE_NS::Uid& class_id)
{
    if (auto factory = GetInstance<IClassFactory>(factory_id)) {
        return CreateInstance<T>(*factory, class_id);
    }
    return typename T::Ptr();
}

/** Creates a class instance by using global registrys class factory.
    and tries to get the requested interface from it.
    Returns empty T::Ptr (null) if the class does not support the interface (or the class could not be created).
*/
template<class T>
typename T::Ptr CreateInstance(const BASE_NS::Uid& class_id)
{
    auto factory = GetPluginRegister().GetClassRegister().GetInterface<IClassFactory>();
    if (factory) {
        return CreateInstance<T>(*factory, class_id);
    }
    return typename T::Ptr();
}

/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_PLUGIN_ICLASS_FACTORY_H
