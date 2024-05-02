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
#ifndef META_SRC_CONTAINER_OBSERVER_H
#define META_SRC_CONTAINER_OBSERVER_H

#include <base/containers/unordered_map.h>

#include <meta/ext/event_impl.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_container_observer.h>

#include "object.h"

META_BEGIN_NAMESPACE()

class ContainerChangeListener final {
public:
    META_NO_COPY(ContainerChangeListener)
    ~ContainerChangeListener();
    ContainerChangeListener() = delete;
    ContainerChangeListener(ContainerChangeListener&& other);
    ContainerChangeListener& operator=(ContainerChangeListener&& other);

    explicit ContainerChangeListener(const IContainer::Ptr& container);
    // Note the copy assignment implementation which is not directly applicable to all use cases
    bool operator==(const ContainerChangeListener& other) const noexcept;
    bool Subscribe(const IOnChildChanged::InterfaceTypePtr& onAdded, const IOnChildChanged::InterfaceTypePtr& onRemoved,
        const IOnChildMoved::InterfaceTypePtr& onMoved);
    void Unsubscribe();

private:
    mutable BASE_NS::pair<IEvent::Token, IOnChildChanged::InterfaceTypePtr> added_;
    mutable BASE_NS::pair<IEvent::Token, IOnChildChanged::InterfaceTypePtr> removed_;
    mutable BASE_NS::pair<IEvent::Token, IOnChildMoved::InterfaceTypePtr> moved_;
    mutable IContainer::WeakPtr container_;
};

class ContainerObserver
    : public Internal::MetaObjectFwd<ContainerObserver, ClassId::ContainerObserver, IContainerObserver> {
    using Super = Internal::MetaObjectFwd<ContainerObserver, ClassId::ContainerObserver, IContainerObserver>;
    using Super::Super;

protected: // LifeCycle
    bool Build(const IMetadata::Ptr&) override;

protected: // IContainerObjserver
    void SetContainer(const IContainer::Ptr& container) override;

    META_IMPLEMENT_INTERFACE_EVENT(IContainerObserver, IOnChildChanged, OnDescendantAdded)
    META_IMPLEMENT_INTERFACE_EVENT(IContainerObserver, IOnChildChanged, OnDescendantRemoved)
    META_IMPLEMENT_INTERFACE_EVENT(IContainerObserver, IOnChildMoved, OnDescendantMoved)

private:
    void Subscribe(const IContainer::Ptr& base);
    void Unsubscribe(const IContainer::Ptr& base);
    void InvokeAdded(const ChildChangedInfo& info);
    void InvokeRemoved(const ChildChangedInfo& info);
    void InvokeMoved(const ChildMovedInfo& info);
    IContainer::Ptr container_;
    IOnChildChanged::InterfaceTypePtr addedCallable_;
    IOnChildChanged::InterfaceTypePtr removedCallable_;
    IOnChildMoved::InterfaceTypePtr movedCallable_;

    struct Subscription {
        Subscription(IContainer::WeakPtr container, ContainerChangeListener&& listener)
            : container(BASE_NS::move(container)), listener(BASE_NS::move(listener))
        {}
        IContainer::WeakPtr container;
        ContainerChangeListener listener;
    };

    BASE_NS::vector<Subscription> subscriptions_;
};

META_END_NAMESPACE()

#endif // META_SRC_CONTAINER_OBSERVER_H
