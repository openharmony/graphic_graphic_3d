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

#ifndef META_INTERFACE_ICLASS_REGISTRY_H
#define META_INTERFACE_ICLASS_REGISTRY_H

#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_object_factory.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IOnClassRegistrationChanged, "3a91e751-551d-44a8-b68d-7c5638173efe")
META_REGISTER_INTERFACE(IOnClassRegistrationChangedCallable, "3285cc3e-f417-42d8-888d-3ed25faabfd1")

struct IOnClassRegistrationChangedInfo {
    constexpr static BASE_NS::Uid UID { META_NS::InterfaceId::IOnClassRegistrationChanged };
    constexpr static char const* NAME { "OnClassRegistrationChanged" };
};

/**
 * @brief The ClassRegistrationInfo struct defines the parameters for an event which is invoked when
 *        an object is registered to or unregistered from the class registry.
 */
struct ClassRegistrationInfo {
    /** ClassInfo of the class whose registration status changed */
    IClassInfo::ConstPtr classInfo;
};

using IOnClassRegistrationChanged =
    META_NS::SimpleEvent<IOnClassRegistrationChangedInfo, void(const ClassRegistrationInfo&)>;

/**
 * @brief The IClassRegistry interface provides the functionality for class management via the object registry.
 */
class IClassRegistry : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IClassRegistry, "cde99105-ea78-451d-a52d-06056ba13fd0")
public:
    /**
     * @brief Returns a list of all classes which implement the given interface requirements.
     * @param interfaceUids List of interfaces a qualifying class shall implement.
     * @param strict If true, the class must implement all listed interfaces. If false, it is enough if
     *               the class implements any of the given interfaces.
     */
    virtual BASE_NS::vector<IClassInfo::ConstPtr> GetAllTypes(
        const BASE_NS::vector<BASE_NS::Uid>& interfaceUids, bool strict, bool excludeDeprecated) const = 0;
    /**
     * @brief A helper for GetAllTypes(interfaceUids, strict) which always defines strict=true.
     */
    BASE_NS::vector<IClassInfo::ConstPtr> GetAllTypes(const BASE_NS::vector<BASE_NS::Uid>& interfaceUids) const
    {
        return GetAllTypes(interfaceUids, true, true);
    }
    /**
     * @brief Invoked when a class is registered.
     */
    META_EVENT(IOnClassRegistrationChanged, OnClassRegistered)
    /**
     * @brief Invoked when a class is unregistered.
     */
    META_EVENT(IOnClassRegistrationChanged, OnClassUnregistered)
};

META_END_NAMESPACE()

META_TYPE(META_NS::ClassRegistrationInfo)

#endif
