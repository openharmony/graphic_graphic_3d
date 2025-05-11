/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Object containable interface, something that can be container in IContainer
 * Author: Mikael Kilpeläinen
 * Create: 2022-11-22
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