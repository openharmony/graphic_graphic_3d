/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Container observer implementation
 * Author: Lauri Jaaskela
 * Create: 2023-03-20
 */
#ifndef META_SRC_CONTAINER_OBSERVER_H
#define META_SRC_CONTAINER_OBSERVER_H

#include <base/containers/unordered_map.h>

#include <meta/base/namespace.h>
#include <meta/ext/event_impl.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_container_observer.h>

#include "object.h"

META_BEGIN_NAMESPACE()

namespace Internal {

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
    bool Subscribe(const IOnChildChanged::InterfaceTypePtr& onChanged);
    void Unsubscribe();

private:
    mutable BASE_NS::pair<IEvent::Token, IOnChildChanged::InterfaceTypePtr> changed_;
    mutable IContainer::WeakPtr container_;
};

class ContainerObserver : public IntroduceInterfaces<MetaObject, IContainerObserver> {
    META_OBJECT(ContainerObserver, ClassId::ContainerObserver, IntroduceInterfaces)
protected: // LifeCycle
    bool Build(const IMetadata::Ptr&) override;

public: // IContainerObjserver
    void SetContainer(const IContainer::Ptr& container) override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_EVENT_DATA(IContainerObserver, IOnChildChanged, OnDescendantChanged)
    META_END_STATIC_DATA()
    META_IMPLEMENT_EVENT(IOnChildChanged, OnDescendantChanged)

private:
    void Subscribe(const IContainer::Ptr& base);
    void Unsubscribe(const IContainer::Ptr& base);
    void InvokeChanged(const ChildChangedInfo& info);

    IContainer::Ptr container_;
    IOnChildChanged::InterfaceTypePtr changedCallable_;

    struct Subscription {
        Subscription(IContainer::WeakPtr container, ContainerChangeListener&& listener)
            : container(BASE_NS::move(container)), listener(BASE_NS::move(listener))
        {}
        IContainer::WeakPtr container;
        ContainerChangeListener listener;
    };

    BASE_NS::vector<Subscription> subscriptions_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_CONTAINER_OBSERVER_H
