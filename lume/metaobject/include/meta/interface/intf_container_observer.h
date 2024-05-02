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
    META_EVENT(IOnChildChanged, OnDescendantAdded)
    /**
     * @brief Invoked when a child is removed from any container in the hierarchy.
     */
    META_EVENT(IOnChildChanged, OnDescendantRemoved)
    /**
     * @brief Invoked when a child is moved within any container in the hierarchy.
     */
    META_EVENT(IOnChildMoved, OnDescendantMoved)
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ICONTAINER_OBSERVER_H
