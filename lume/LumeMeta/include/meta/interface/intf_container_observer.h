/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: IContainer observer interface
 * Author: Lauri Jaaskela
 * Create: 2023-03-21
 */

#ifndef META_INTERFACE_ICONTAINER_OBSERVER_H
#define META_INTERFACE_ICONTAINER_OBSERVER_H

#include <meta/interface/intf_container.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IContainerObserver, "19ddad68-4743-4902-a816-a5f4d5812c7e")

/**
 * @brief The IContainerObserver interface can be implemented by classes which
 *        can be used to monitor the container hierarchy of an IContainer.
 *
 *        The difference between IContainerObserver events and IContainer events
 *        is that IContainer defines events only for the immediate children of
 *        the IContainer, while IContainerObserver events are fired as a response
 *        to changes in the entire child hierarchy.
 */
class IContainerObserver : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContainerObserver)
public:
    /**
     * @brief Sets the container to observe.
     * @param container The target container.
     */
    virtual void SetContainer(const IContainer::Ptr& container) = 0;
    /**
     * @brief Invoked when a child is added to any container in the hierarchy.
     */
    META_EVENT(IOnChildChanged, OnDescendantChanged)
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ICONTAINER_OBSERVER_H
