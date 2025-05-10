/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Object container implementation
 * Author: Lauri Jaaskela
 * Create: 2023-03-20
 */

#include "container_observer.h"

#include <algorithm>

#include <meta/api/make_callback.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>

META_BEGIN_NAMESPACE()

namespace Internal {

// ContainerChangeListener

ContainerChangeListener::ContainerChangeListener(const IContainer::Ptr& container)
    : changed_ { 0, {} }, container_(container)
{}

ContainerChangeListener::~ContainerChangeListener()
{
    Unsubscribe();
}

ContainerChangeListener::ContainerChangeListener(ContainerChangeListener&& other) = default;

ContainerChangeListener& ContainerChangeListener::operator=(ContainerChangeListener&& other)
{
    if (&other == this) {
        return *this;
    }
    Unsubscribe();

    container_ = BASE_NS::move(container_);
    changed_ = BASE_NS::move(changed_);

    return *this;
}

bool ContainerChangeListener::operator==(const ContainerChangeListener& other) const noexcept
{
    return other.changed_.first == changed_.first;
}

bool ContainerChangeListener::Subscribe(const IOnChildChanged::InterfaceTypePtr& onChanged)
{
    Unsubscribe();
    if (const auto container = container_.lock()) {
        changed_ = { container->OnContainerChanged()->AddHandler(onChanged), onChanged };
        return changed_.first;
    }
    return false;
}

void ContainerChangeListener::Unsubscribe()
{
    if (const auto container = container_.lock()) {
        if (changed_.first) {
            container->OnContainerChanged()->RemoveHandler(changed_.first);
        }
    }
    changed_ = {};
}

// ContainerObserver

bool ContainerObserver::Build(const IMetadata::Ptr& p)
{
    bool ret = Super::Build(p);
    if (ret) {
        changedCallable_ = MakeCallback<IOnChildChanged>(this, &ContainerObserver::InvokeChanged);
    }
    return ret;
}

void ContainerObserver::SetContainer(const IContainer::Ptr& container)
{
    subscriptions_.clear();
    container_ = container;
    Subscribe(container_);
}

void ContainerObserver::InvokeChanged(const ChildChangedInfo& info)
{
    Invoke<IOnChildChanged>(META_ACCESS_EVENT(OnDescendantChanged), info);
    if (info.type == ContainerChangeType::ADDED) {
        Subscribe(interface_pointer_cast<IContainer>(info.object));
    } else if (info.type == ContainerChangeType::REMOVED) {
        Unsubscribe(interface_pointer_cast<IContainer>(info.object));
    }
}

void ContainerObserver::Subscribe(const IContainer::Ptr& base)
{
    if (!base) {
        return;
    }
    auto listener = ContainerChangeListener(base);
    listener.Subscribe(changedCallable_);

    subscriptions_.emplace_back(base, BASE_NS::move(listener));

    for (const auto& child : base->GetAll<IContainer>()) {
        Subscribe(child);
    }
}

void ContainerObserver::Unsubscribe(const IContainer::Ptr& base)
{
    if (!base) {
        return;
    }
    auto it = subscriptions_.begin();
    while (it != subscriptions_.end()) {
        const auto& sub = *it;
        if (const auto c = sub.container.lock()) {
            const auto object = interface_pointer_cast<const IObject>(c);
            if (base == c || base->IsAncestorOf(object)) {
                it = subscriptions_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

} // namespace Internal

META_END_NAMESPACE()
