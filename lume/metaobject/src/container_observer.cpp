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

#include "container_observer.h"

#include <algorithm>

#include <meta/api/make_callback.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>

META_BEGIN_NAMESPACE()

// ContainerChangeListener

ContainerChangeListener::ContainerChangeListener(const IContainer::Ptr& container)
    : added_ { 0, {} }, removed_ { 0, {} }, moved_ { 0, {} }, container_(container)
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
    added_ = BASE_NS::move(added_);
    removed_ = BASE_NS::move(removed_);
    moved_ = BASE_NS::move(moved_);

    return *this;
}

bool ContainerChangeListener::operator==(const ContainerChangeListener& other) const noexcept
{
    return other.added_.first == added_.first && other.removed_.first == removed_.first &&
           other.moved_.first == moved_.first;
}

bool ContainerChangeListener::Subscribe(const IOnChildChanged::InterfaceTypePtr& onAdded,
    const IOnChildChanged::InterfaceTypePtr& onRemoved, const IOnChildMoved::InterfaceTypePtr& onMoved)
{
    Unsubscribe();
    if (const auto container = container_.lock()) {
        added_ = { container->OnAdded()->AddHandler(onAdded), onAdded };
        removed_ = { container->OnRemoved()->AddHandler(onRemoved), onRemoved };
        moved_ = { container->OnMoved()->AddHandler(onMoved), onMoved };
        return added_.first && removed_.first && moved_.first;
    }
    return false;
}

void ContainerChangeListener::Unsubscribe()
{
    if (const auto container = container_.lock()) {
        if (added_.first) {
            container->OnAdded()->RemoveHandler(added_.first);
        }
        if (removed_.first) {
            container->OnRemoved()->RemoveHandler(removed_.first);
        }
        if (moved_.first) {
            container->OnMoved()->RemoveHandler(moved_.first);
        }
    }
    added_ = {};
    removed_ = {};
    moved_ = {};
}

// ContainerObserver

bool ContainerObserver::Build(const IMetadata::Ptr& p)
{
    bool ret = Super::Build(p);
    if (ret) {
        addedCallable_ = MakeCallback<IOnChildChanged>(this, &ContainerObserver::InvokeAdded);
        removedCallable_ = MakeCallback<IOnChildChanged>(this, &ContainerObserver::InvokeRemoved);
        movedCallable_ = MakeCallback<IOnChildMoved>(this, &ContainerObserver::InvokeMoved);
    }
    return ret;
}

void ContainerObserver::SetContainer(const IContainer::Ptr& container)
{
    subscriptions_.clear();
    container_ = container;
    Subscribe(container_);
}

void ContainerObserver::InvokeAdded(const ChildChangedInfo& info)
{
    META_ACCESS_EVENT(OnDescendantAdded)->Invoke(info);
    Subscribe(interface_pointer_cast<IContainer>(info.object));
}

void ContainerObserver::InvokeRemoved(const ChildChangedInfo& info)
{
    META_ACCESS_EVENT(OnDescendantRemoved)->Invoke(info);
    Unsubscribe(interface_pointer_cast<IContainer>(info.object));
}

void ContainerObserver::InvokeMoved(const ChildMovedInfo& info)
{
    META_ACCESS_EVENT(OnDescendantMoved)->Invoke(info);
}

void ContainerObserver::Subscribe(const IContainer::Ptr& base)
{
    if (!base) {
        return;
    }
    auto listener = ContainerChangeListener(base);
    listener.Subscribe(addedCallable_, removedCallable_, movedCallable_);

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

META_END_NAMESPACE()
