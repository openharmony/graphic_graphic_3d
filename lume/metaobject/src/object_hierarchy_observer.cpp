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

#include "object_hierarchy_observer.h"

#include <algorithm>

#include <meta/api/iteration.h>
#include <meta/api/make_callback.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/api/util.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_content.h>

META_BEGIN_NAMESPACE()

// ContainerChangeListener

ObjectChangeListener::ObjectChangeListener(const IObject::Ptr& object, HierarchyChangeObjectType myType,
    const IObject::WeakPtr& parent, ObjectHierarchyObserver* observer, HierarchyChangeModeValue mode)
    : object_(object), type_(myType), parent_(parent), observer_(observer)
{
    CORE_ASSERT(observer_ && object);
    Subscribe(mode);
}

ObjectChangeListener::~ObjectChangeListener()
{
    Unsubscribe();
}

void ObjectChangeListener::SubscribeContainer(const IObject::Ptr& object)
{
    if (const auto container = interface_cast<IContainer>(object)) {
        if (auto trans = interface_cast<IContainerPreTransaction>(container)) {
            handlers_.emplace_back(trans->OnAdding(), [this](const ChildChangedInfo& info, bool&) {
                NotifyContainerChangeOp(info, HierarchyChangeType::ADDING, HierarchyChangeObjectType::CHILD);
            });
            handlers_.emplace_back(trans->OnRemoving(), [this](const ChildChangedInfo& info, bool&) {
                NotifyContainerChangeOp(info, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::CHILD);
            });
            containerPreTransaction_ = true;
        }

        handlers_.emplace_back(container->OnAdded(), [this](const ChildChangedInfo& info) {
            NotifyContainerChangeOp(info, HierarchyChangeType::ADDED, HierarchyChangeObjectType::CHILD);
        });
        handlers_.emplace_back(container->OnRemoved(), [this](const ChildChangedInfo& info) {
            NotifyContainerChangeOp(info, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::CHILD);
        });
        handlers_.emplace_back(container->OnMoved(), [this](const ChildMovedInfo& info) {
            NotifyContainerMoveOp(info, HierarchyChangeType::MOVED, HierarchyChangeObjectType::CHILD);
        });
    }
}

void ObjectChangeListener::SubscribeAttachment(const IObject::Ptr& object)
{
    if (const auto attach = interface_cast<IAttach>(object)) {
        if (const auto container = attach->GetAttachmentContainer(true)) {
            if (const auto trans = interface_cast<IContainerPreTransaction>(container)) {
                handlers_.emplace_back(trans->OnAdding(), [this](const ChildChangedInfo& info, bool&) {
                    NotifyContainerChangeOp(info, HierarchyChangeType::ADDING, HierarchyChangeObjectType::ATTACHMENT);
                });
                handlers_.emplace_back(trans->OnRemoving(), [this](const ChildChangedInfo& info, bool&) {
                    NotifyContainerChangeOp(info, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::ATTACHMENT);
                });
                attachmentPreTransaction_ = true;
            }
            handlers_.emplace_back(container->OnAdded(), [this](const ChildChangedInfo& info) {
                NotifyContainerChangeOp(info, HierarchyChangeType::ADDED, HierarchyChangeObjectType::ATTACHMENT);
            });
            handlers_.emplace_back(container->OnRemoved(), [this](const ChildChangedInfo& info) {
                NotifyContainerChangeOp(info, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::ATTACHMENT);
            });
            handlers_.emplace_back(container->OnMoved(), [this](const ChildMovedInfo& info) {
                NotifyContainerMoveOp(info, HierarchyChangeType::MOVED, HierarchyChangeObjectType::ATTACHMENT);
            });
        }
    }
}

bool ObjectChangeListener::Subscribe(HierarchyChangeModeValue mode)
{
    if (const auto object = object_.lock()) {
        // Figure out which hierarchical interfaces our target object implements and add listeners based on that
        if (mode & HierarchyChangeMode::NOTIFY_CONTAINER) {
            SubscribeContainer(object);
        }
        if (mode & HierarchyChangeMode::NOTIFY_CONTENT) {
            if (const auto content = interface_cast<IContent>(object)) {
                content_ = META_NS::GetValue(content->Content());
                handlers_.emplace_back(
                    content->Content()->OnChanged(), MakeCallback<IOnChanged>([this]() { NotifyContentChangeOp(); }));
            }
        }
        if (mode & HierarchyChangeMode::NOTIFY_ATTACHMENT) {
            SubscribeAttachment(object);
        }
        if (mode & HierarchyChangeMode::NOTIFY_OBJECT) {
            if (auto i = interface_pointer_cast<INotifyOnChange>(object)) {
                handlers_.emplace_back(i->OnChanged(), MakeCallback<IOnChanged>([this]() { NotifyObjectChangedOp(); }));
            }
        }
        return true;
    }
    return false;
}

void ObjectChangeListener::Unsubscribe()
{
    handlers_.clear();
    content_.reset();
}

void ObjectChangeListener::NotifyObjectChangedOp()
{
    if (auto object = object_.lock()) {
        HierarchyChangedInfo change;
        change.object = object;
        change.change = HierarchyChangeType::CHANGED;
        change.objectType = type_;
        change.parent = parent_;
        observer_->HierarchyChanged(change, this);
    }
}

void ObjectChangeListener::NotifyContainerChangeOp(
    const ChildChangedInfo& info, HierarchyChangeType operation, HierarchyChangeObjectType objectType)
{
    HierarchyChangedInfo change;
    change.object = info.object;
    change.change = operation;
    change.objectType = objectType;
    change.index = info.index;
    change.parent = object_;
    if ((objectType == HierarchyChangeObjectType::CHILD && !containerPreTransaction_) ||
        (objectType == HierarchyChangeObjectType::ATTACHMENT && !attachmentPreTransaction_)) {
        // Our target does not support pre transaction notifications, generate ones.
        if (operation == HierarchyChangeType::ADDED) {
            change.change = HierarchyChangeType::ADDING;
            observer_->HierarchyChanged(change, this);
            change.change = operation;
        } else if (operation == HierarchyChangeType::REMOVED) {
            change.change = HierarchyChangeType::REMOVING;
            observer_->HierarchyChanged(change, this);
            change.change = operation;
        }
    }
    observer_->HierarchyChanged(change, this);
}

void ObjectChangeListener::NotifyContainerMoveOp(
    const ChildMovedInfo& info, HierarchyChangeType operation, HierarchyChangeObjectType objectType)
{
    HierarchyChangedInfo change;
    change.object = info.object;
    change.change = operation;
    change.objectType = objectType;
    change.parent = object_;
    observer_->HierarchyChanged(change, this);
}

void ObjectChangeListener::NotifyContentChangeOp()
{
    if (const auto content = interface_pointer_cast<IContent>(object_)) {
        const auto newContent = GetValue(content->Content());
        const auto oldContent = content_;

        HierarchyChangedInfo change;
        change.objectType = HierarchyChangeObjectType::CONTENT;
        change.parent = object_;

        if (oldContent != newContent) {
            if (oldContent) {
                change.object = oldContent;
                change.change = HierarchyChangeType::REMOVING;
                observer_->HierarchyChanged(change, this);
                change.change = HierarchyChangeType::REMOVED;
                observer_->HierarchyChanged(change, this);
            }
            content_ = newContent;
            if (newContent) {
                change.object = newContent;
                change.change = HierarchyChangeType::ADDING;
                observer_->HierarchyChanged(change, this);
                change.change = HierarchyChangeType::ADDED;
                observer_->HierarchyChanged(change, this);
            }
        }
    }
}

// ContainerObserver

bool ObjectHierarchyObserver::Build(const IMetadata::Ptr& p)
{
    bool ret = Super::Build(p);
    if (ret) {
        // we don't want to serialise observers
        META_NS::SetObjectFlags(GetSelf(), ObjectFlagBits::SERIALIZE, false);
    }
    return ret;
}

void ObjectHierarchyObserver::Destroy()
{
    ClearSubscriptions();
}

void ObjectHierarchyObserver::SetTarget(const IObject::Ptr& root, HierarchyChangeModeValue mode)
{
    ClearSubscriptions();
    mode_ = mode;
    root_ = root;

    if (auto i = interface_cast<IAttach>(root)) {
        i->Attach(GetSelf<IAttachment>());
    }

    Subscribe(root, HierarchyChangeObjectType::ROOT);
}

void ObjectHierarchyObserver::ClearSubscriptions()
{
    // Delay subscription removal until outside lock
    decltype(subscriptions_) subs;
    decltype(immediateChildren_) immes;
    {
        std::unique_lock lock(mutex_);
        subs = BASE_NS::move(subscriptions_);
        immes = BASE_NS::move(immediateChildren_);
    }
    subs.clear();
    immes.clear();
}

IObject::Ptr ObjectHierarchyObserver::GetTarget() const
{
    return root_.lock();
}

BASE_NS::vector<IObject::Ptr> ObjectHierarchyObserver::GetAllObserved() const
{
    std::shared_lock lock(mutex_);
    BASE_NS::vector<IObject::Ptr> observed;
    observed.reserve(subscriptions_.size());
    for (auto&& sub : subscriptions_) {
        if (auto object = sub.second.object_.lock()) {
            observed.emplace_back(BASE_NS::move(object));
        }
    }
    return observed;
}

void ObjectHierarchyObserver::AddImmediateChild(const HierarchyChangedInfo& info)
{
    std::unique_lock lock(mutex_);
    if (info.index < immediateChildren_.size()) {
        immediateChildren_.insert(
            immediateChildren_.begin() + info.index, ImmediateChild { info.object, info.objectType });
    } else {
        immediateChildren_.push_back(ImmediateChild { info.object, info.objectType });
    }
}

void ObjectHierarchyObserver::RemoveImmediateChild(const HierarchyChangedInfo& info)
{
    std::unique_lock lock(mutex_);
    auto it = immediateChildren_.begin();
    for (; it != immediateChildren_.end() && it->object.lock() != info.object; ++it) {
    }
    if (it != immediateChildren_.end()) {
        immediateChildren_.erase(it);
    }
}

void ObjectHierarchyObserver::HierarchyChanged(const HierarchyChangedInfo& info, ObjectChangeListener* listener)
{
    if (info.object != GetSelf()) {
        META_ACCESS_EVENT(OnHierarchyChanged)->Invoke(info);
        if (info.objectType == HierarchyChangeObjectType::CHILD ||
            info.objectType == HierarchyChangeObjectType::CONTENT) {
            // We do not listen to attachments (other than monitoring for changes in an object's attachment list)
            if (info.change == HierarchyChangeType::ADDING) {
                Subscribe(info.object, info.objectType, info.parent);
                if (keepTrackOfImmediate_ && listener->GetType() == HierarchyChangeObjectType::ROOT) {
                    AddImmediateChild(info);
                }
            } else if (info.change == HierarchyChangeType::REMOVED) {
                Unsubscribe(info.object);
                if (keepTrackOfImmediate_ && listener->GetType() == HierarchyChangeObjectType::ROOT) {
                    RemoveImmediateChild(info);
                }
            }
        }
    }
}

void ObjectHierarchyObserver::Subscribe(
    const IObject::Ptr& root, HierarchyChangeObjectType type, const IObject::WeakPtr& parent)
{
    if (!root) {
        return;
    }

    AddSubscription(root, type, parent);

    if (const auto container = interface_cast<IContainer>(root)) {
        for (auto&& child : container->GetAll()) {
            if (keepTrackOfImmediate_ && type == HierarchyChangeObjectType::ROOT) {
                std::unique_lock lock(mutex_);
                immediateChildren_.push_back(ImmediateChild { child, HierarchyChangeObjectType::CHILD });
            }
            Subscribe(child, HierarchyChangeObjectType::CHILD, root);
        }
    }
    if (const auto content = interface_cast<IContent>(root)) {
        if (auto object = GetValue(content->Content())) {
            if (keepTrackOfImmediate_ && type == HierarchyChangeObjectType::ROOT) {
                std::unique_lock lock(mutex_);
                immediateChildren_.push_back(ImmediateChild { object, HierarchyChangeObjectType::CONTENT });
            }
            Subscribe(object, HierarchyChangeObjectType::CONTENT, root);
        }
    }
}

void ObjectHierarchyObserver::AddSubscription(
    const IObject::Ptr& object, HierarchyChangeObjectType type, const IObject::WeakPtr& parent)
{
    auto listener = BASE_NS::make_unique<ObjectChangeListener>(object, type, parent, this, mode_);

    {
        std::unique_lock lock(mutex_);
        if (subscriptions_.find(listener.get()) != subscriptions_.end()) {
            CORE_LOG_E(
                "Duplicate event subscription for %s:%s", object->GetClassName().data(), object->GetName().data());
        }
        subscriptions_.insert({ listener.get(), Subscription(object, BASE_NS::move(listener)) });
    }
}

void ObjectHierarchyObserver::Unsubscribe(const IObject::Ptr& root)
{
    if (!root) {
        return;
    }

    RemoveSubscription(root);

    if (root->GetInterface(IIterable::UID)) {
        ForEachShared(
            root, [this](const IObject::Ptr& object) { RemoveSubscription(object); },
            TraversalType::DEPTH_FIRST_PRE_ORDER);
    } else {
        if (const auto container = interface_cast<IContainer>(root)) {
            for (auto&& child : container->GetAll()) {
                Unsubscribe(child);
            }
        }
        if (const auto content = interface_cast<IContent>(root)) {
            if (auto object = GetValue(content->Content())) {
                Unsubscribe(object);
            }
        }
    }
}

void ObjectHierarchyObserver::RemoveSubscription(const IObject::Ptr& object)
{
    // Delay subscription destruction until we're out of the lock
    BASE_NS::vector<Subscription> subs;
    {
        std::unique_lock lock(mutex_);
        auto it = subscriptions_.begin();
        while (it != subscriptions_.end()) {
            const auto o = it->second.object_.lock();
            if (!o || o == object) {
                subs.emplace_back(BASE_NS::move(it->second));
                it = subscriptions_.erase(it);
            } else {
                ++it;
            }
        }
    }
    subs.clear();
}

void ObjectHierarchyObserver::NotifyOnDetach()
{
    // If we were attached to the target and we are getting detached when the target already died, lets
    // notify about the removing the immediate children as those are not otherwise notified when destroying
    // container
    decltype(immediateChildren_) ic;
    {
        std::unique_lock lock(mutex_);
        ic = BASE_NS::move(immediateChildren_);
    }
    if (root_.expired()) {
        for (auto&& c : ic) {
            if (auto o = c.object.lock()) {
                // Generate pre transaction notifications (even if coming in wrong time).
                HierarchyChangedInfo info { o, HierarchyChangeType::REMOVING, c.type, size_t(-1), nullptr };
                META_ACCESS_EVENT(OnHierarchyChanged)->Invoke(info);
                info.change = HierarchyChangeType::REMOVED;
                META_ACCESS_EVENT(OnHierarchyChanged)->Invoke(info);
            }
        }
    }
}

bool ObjectHierarchyObserver::Attaching(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext)
{
    META_ACCESS_PROPERTY(AttachedTo)->SetValue(target);
    META_ACCESS_PROPERTY(DataContext)->SetValue(dataContext);
    {
        std::unique_lock lock(mutex_);
        keepTrackOfImmediate_ = true;
    }
    return true;
}
bool ObjectHierarchyObserver::Detaching(const META_NS::IAttach::Ptr& target)
{
    NotifyOnDetach();
    {
        std::unique_lock lock(mutex_);
        keepTrackOfImmediate_ = false;
    }
    META_ACCESS_PROPERTY(AttachedTo)->SetValue({});
    META_ACCESS_PROPERTY(DataContext)->SetValue({});
    return true;
}

META_END_NAMESPACE()
