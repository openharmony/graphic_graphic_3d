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
#ifndef META_SRC_OBJECT_HIERARCHY_OBSERVER_H
#define META_SRC_OBJECT_HIERARCHY_OBSERVER_H

#include <shared_mutex>

#include <base/containers/unordered_map.h>

#include <meta/api/event_handler.h>
#include <meta/ext/event_impl.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_object_hierarchy_observer.h>

#include "object.h"

META_BEGIN_NAMESPACE()

class ObjectHierarchyObserver;

class ObjectChangeListener final {
public:
    META_NO_COPY_MOVE(ObjectChangeListener)
    ObjectChangeListener() = delete;
    ~ObjectChangeListener();

    ObjectChangeListener(const IObject::Ptr& object, HierarchyChangeObjectType myType, const IObject::WeakPtr& parent,
        ObjectHierarchyObserver* observer, HierarchyChangeModeValue mode);

    HierarchyChangeObjectType GetType() const
    {
        return type_;
    }

private:
    bool Subscribe(HierarchyChangeModeValue mode);
    void Unsubscribe();

    void SubscribeContainer(const IObject::Ptr& object);
    void SubscribeAttachment(const IObject::Ptr& object);

    IObject::WeakPtr object_;
    HierarchyChangeObjectType type_;
    IObject::WeakPtr parent_;

    // Currently IContent does not support "OnChanging" and so the content might be destroyed already
    // which would make it impossible to notify about
    IObject::Ptr content_;

    ObjectHierarchyObserver* observer_ {};
    BASE_NS::vector<EventHandler> handlers_;
    bool containerPreTransaction_ { false };
    bool attachmentPreTransaction_ { false };

    void NotifyObjectChangedOp();
    void NotifyContainerChangeOp(
        const ChildChangedInfo& info, HierarchyChangeType operation, HierarchyChangeObjectType objectType);
    void NotifyContainerMoveOp(
        const ChildMovedInfo& info, HierarchyChangeType operation, HierarchyChangeObjectType objectType);
    void NotifyContentChangeOp();
};

class ObjectHierarchyObserver final : public Internal::MetaObjectFwd<ObjectHierarchyObserver,
                                          ClassId::ObjectHierarchyObserver, IObjectHierarchyObserver, IAttachment> {
    using Super = Internal::MetaObjectFwd<ObjectHierarchyObserver, ClassId::ObjectHierarchyObserver,
        IObjectHierarchyObserver, IAttachment>;

public:
    void HierarchyChanged(const HierarchyChangedInfo& info, ObjectChangeListener* listener);

protected: // LifeCycle
    bool Build(const IMetadata::Ptr&) override;
    void Destroy() override;

protected: // IObjectHierarchyObserver
    void SetTarget(const IObject::Ptr& root, HierarchyChangeModeValue mode) override;
    IObject::Ptr GetTarget() const override;
    BASE_NS::vector<IObject::Ptr> GetAllObserved() const override;
    META_IMPLEMENT_INTERFACE_EVENT(IObjectHierarchyObserver, IOnHierarchyChanged, OnHierarchyChanged)

protected: // IAttachment
    bool Attaching(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override;
    bool Detaching(const META_NS::IAttach::Ptr& target) override;

    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAttachment, IObject::WeakPtr, DataContext);
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAttachment, IAttach::WeakPtr, AttachedTo, {});

private:
    void Subscribe(const IObject::Ptr& root, HierarchyChangeObjectType type, const IObject::WeakPtr& parent = nullptr);
    void Unsubscribe(const IObject::Ptr& root);
    void ClearSubscriptions();
    void NotifyOnDetach();

    HierarchyChangeModeValue mode_ {};
    IObject::WeakPtr root_;

    struct Subscription final {
        META_NO_COPY(Subscription)
        Subscription(IObject::WeakPtr object, BASE_NS::unique_ptr<ObjectChangeListener>&& listener)
            : object_(BASE_NS::move(object)), listener_(BASE_NS::move(listener))
        {}
        ~Subscription() = default;
        Subscription(Subscription&& other) noexcept = default;
        Subscription& operator=(Subscription&& other) = default;
        IObject::WeakPtr object_;
        BASE_NS::unique_ptr<ObjectChangeListener> listener_;
    };

    void AddSubscription(const IObject::Ptr& object, HierarchyChangeObjectType type, const IObject::WeakPtr& parent);
    void RemoveSubscription(const IObject::Ptr& object);

    void AddImmediateChild(const HierarchyChangedInfo& info);
    void RemoveImmediateChild(const HierarchyChangedInfo& info);

    BASE_NS::unordered_map<ObjectChangeListener*, Subscription> subscriptions_;
    struct ImmediateChild {
        BASE_NS::weak_ptr<IObject> object;
        HierarchyChangeObjectType type {};
    };
    bool keepTrackOfImmediate_ {};
    BASE_NS::vector<ImmediateChild> immediateChildren_;
    mutable std::shared_mutex mutex_;
};

META_END_NAMESPACE()

#endif // META_SRC_OBJECT_HIERARCHY_OBSERVER_H
