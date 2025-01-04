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

#ifndef META_INTERFACE_ICONTAINABLE_H
#define META_INTERFACE_ICONTAINABLE_H

#include <base/util/uid.h>

#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IContainable, "4bcc982b-4b40-4171-9b77-073dc86d8f63")

/**
 * @brief Interface indicating the object can be contained (for example in IContainer)
 *        and so it has a parent
 */
class IContainable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContainable)
public:
    /**
     * @brief Returns the parent object of this object if any
     */
    virtual IObject::Ptr GetParent() const = 0;
};

META_REGISTER_INTERFACE(IMutableContainable, "dd0b8f12-0f81-4dc3-869d-79c3457d4557")

/**
 * @brief Implies IContainable where the parent can be re-set
 */
class IMutableContainable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMutableContainable)
public:
    /**
     * @brief Sets the parent object for this object
     */
    virtual void SetParent(const IObject::Ptr& parent) = 0;
};

META_REGISTER_INTERFACE(IEnablePropertyTraversal, "32612e47-f755-404b-8bfb-365dac04c644")

/**
 * @brief Interface that enables Resolve traversal through property types
 */
class IEnablePropertyTraversal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEnablePropertyTraversal)
public:
    /**
     * @brief Set the property that currently owns this value.
     */
    virtual void SetProperty(const IProperty::ConstWeakPtr&) = 0;
    /**
     * @brief Get the currently owning property.
     */
    virtual IProperty::ConstWeakPtr GetProperty() const = 0;
};

META_END_NAMESPACE()

#endif