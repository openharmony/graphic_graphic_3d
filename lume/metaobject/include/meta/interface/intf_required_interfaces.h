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

#ifndef META_INTERFACE_IREQUIRED_INTERFACES_H
#define META_INTERFACE_IREQUIRED_INTERFACES_H

#include <base/util/uid.h>

#include <meta/base/interface_utils.h>
#include <meta/base/object_traits.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IRequiredInterfaces, "d59ca1ee-e82d-4eb7-b323-728c0f5fdde3")

/**
 * @brief The IRequiredInterfaces interface can be implemented by objects which require objects handled by the
 *        implementer to implement a set of interfaces.
 *        TYpically implementers also implement IContainer or IContent.
 */
class IRequiredInterfaces : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRequiredInterfaces)
public:
    /**
     * @brief Sets a set of Uids which all objects handled by the implementer must implement. If the list is empty
     *        no interface check is made.
     * @param interfaces List of interfaces an object handled by the implementer must to implement.
     * @note Calling this function may alter the implementing object in case some existing items do not fulfill the new
     *       interface requirements.
     * @note This function may only affect the direct children of the implementer. If the implementer e.g. implements
     *       an object hierarchy, the hierarchy may contain  objects that do not fulfill the requirement.
     * @note IObject is an implicit requirement of any objects handled by the implementer, meaning that IObject::UID
     *       does not need to be included in the interface list.
     * @note The implementer may also opt to not allow changes to the required interfaces, in such cases
     *       SetRequiredInterfaces should return false.
     * @return True if the interface list was successfully applied, false otherwise.
     */
    virtual bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) = 0;
    /**
     * @brief Returns the list of interfaces required by the implementer.
     */
    virtual BASE_NS::vector<TypeId> GetRequiredInterfaces() const = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_IREQUIRED_INTERFACES_H
