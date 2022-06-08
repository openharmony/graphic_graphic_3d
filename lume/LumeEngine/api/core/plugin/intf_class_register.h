/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_PLUGIN_ICLASS_REGISTER_H
#define API_CORE_PLUGIN_ICLASS_REGISTER_H

#include <base/containers/array_view.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

CORE_BEGIN_NAMESPACE()
class IClassRegister : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "fcdce31c-1208-4a4e-9ccf-292f87c9dbe0" };

    /** Register interface type
     * @param interfaceInfo InterfaceTypeInfo to be registered
     */
    virtual void RegisterInterfaceType(const InterfaceTypeInfo& interfaceInfo) = 0;

    /** Unregister interface type
     * @param interfaceInfo InterfaceTypeInfo to unregister.
     */
    virtual void UnregisterInterfaceType(const InterfaceTypeInfo& interfaceInfo) = 0;

    /** Returns interface metadata */
    virtual BASE_NS::array_view<const InterfaceTypeInfo* const> GetInterfaceMetadata() const = 0;

    /** Get interface info by UID */
    virtual const InterfaceTypeInfo& GetInterfaceMetadata(const BASE_NS::Uid& uid) const = 0;

    /** Get interface singleton instance by UID */
    virtual IInterface* GetInstance(const BASE_NS::Uid& uid) const = 0;

protected:
    IClassRegister() = default;
    virtual ~IClassRegister() = default;
};

/** Get class instance from specified class registry */
template<class T>
auto GetInstance(IClassRegister& registry, const BASE_NS::Uid& uid)
{
    IInterface* instance = static_cast<T*>(registry.GetInstance(uid));
    if (instance) {
        return static_cast<T*>(instance->GetInterface(T::UID));
    }
    return static_cast<T*>(nullptr);
}

/** Get class instance from specified class registry */
template<class T>
auto GetInstance(const IClassRegister& registry, const BASE_NS::Uid& uid)
{
    return GetInstance<T>(const_cast<IClassRegister&>(registry), uid);
}

/** Get interface from global registry */
template<class T>
auto GetInstance(const BASE_NS::Uid& uid)
{
    return GetInstance<T>(CORE_NS::GetPluginRegister().GetClassRegister(), uid);
}

/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_PLUGIN_ICLASS_REGISTER_H
