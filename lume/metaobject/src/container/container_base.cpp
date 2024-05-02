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
#include "container_base.h"

#include <algorithm>

#include <base/math/mathf.h>

#include <meta/api/iteration.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>

META_BEGIN_NAMESPACE()

void ContainerBase::SetImplementingIContainer(IObject* me, IContainer* c)
{
    me_ = me;
    impl_ = c;
    CORE_ASSERT(impl_);
    implPreTrans_ = interface_cast<IContainerPreTransaction>(impl_);
    CORE_ASSERT(implPreTrans_);
}

void ContainerBase::LockShared() const
{
    mutex_.lock_shared();
}

void ContainerBase::UnlockShared() const
{
    mutex_.unlock_shared();
}

void ContainerBase::Lock() const
{
    mutex_.lock();
}

void ContainerBase::Unlock() const
{
    mutex_.unlock();
}

bool ContainerBase::SetProxyParent(const IContainer::Ptr& parent)
{
    parent_ = parent;
    return true;
}

IObject::Ptr ContainerBase::FindAnyImpl(const IContainer::FindOptions& options, bool isFlat) const
{
    IObject::Ptr res;
    ConstIterate(
        GetSelf(impl_),
        [&](const IObject::Ptr& obj) {
            if (MatchCriteria(options, obj)) {
                res = obj;
            }
            return !res;
        },
        IterateStrategy { isFlat ? TraversalType::NO_HIERARCHY : options.behavior, LockType::SHARED_LOCK });
    return res;
}

BASE_NS::vector<IObject::Ptr> ContainerBase::FindAllImpl(const IContainer::FindOptions& options, bool isFlat) const
{
    BASE_NS::vector<IObject::Ptr> res;
    ConstIterate(
        GetSelf(impl_),
        [&](const IObject::Ptr& obj) {
            if (options.name.empty() || obj->GetName() == options.name) {
                if (CheckInterfaces(obj, options.uids, options.strict)) {
                    res.push_back(obj);
                }
            }
            return true;
        },
        IterateStrategy { isFlat ? TraversalType::NO_HIERARCHY : options.behavior, LockType::SHARED_LOCK });
    return res;
}

BASE_NS::vector<IObject::Ptr> ContainerBase::GetAll() const
{
    std::shared_lock lock(mutex_);
    return children_;
}

IObject::Ptr ContainerBase::GetAt(SizeType index) const
{
    std::shared_lock lock(mutex_);
    if (index >= children_.size()) {
        return {};
    }
    return children_[index];
}

IContainer::SizeType ContainerBase::GetSize() const
{
    std::shared_lock lock(mutex_);
    return children_.size();
}

IObject::Ptr ContainerBase::FindByName(BASE_NS::string_view name) const
{
    std::shared_lock lock(mutex_);
    for (const auto& child : children_) {
        if (child->GetName() == name) {
            return child;
        }
    }
    return {};
}

bool ContainerBase::MatchCriteria(const IContainer::FindOptions& options, const IObject::Ptr& object) const
{
    return object && (options.name.empty() || object->GetName() == options.name) &&
           CheckInterfaces(object, options.uids, options.strict);
}

bool ContainerBase::Remove(SizeType index)
{
    IObject::Ptr child;
    const auto direct = !implPreTrans_->OnRemoving()->HasHandlers();
    {
        std::unique_lock lock(mutex_);
        if (children_.size() <= index) {
            return false;
        }
        auto it = children_.begin() + index;
        child = *it;
        if (direct) {
            children_.erase(it);
        }
    }
    bool success = true;
    ChildChangedInfo info { child, index, parent_ };
    if (!direct) {
        Invoke<IOnChildChanging>(implPreTrans_->OnRemoving(), info, success);
        if (success) {
            std::unique_lock lock(mutex_);
            children_.erase(children_.begin() + index);
        }
    }
    if (success) {
        SetObjectParent(child, nullptr);
        Invoke<IOnChildChanged>(impl_->OnRemoved(), info);
    }
    return success;
}

bool ContainerBase::Remove(const IObject::Ptr& child)
{
    if (!child) {
        return false;
    }
    const auto direct = !implPreTrans_->OnRemoving()->HasHandlers();
    bool success = false;
    SizeType index = 0;
    {
        std::unique_lock lock(mutex_);
        for (auto it = children_.cbegin(); it != children_.cend(); ++it) {
            if (*it == child) {
                success = true;
                if (direct) {
                    children_.erase(children_.begin() + index);
                }
                break;
            }
            index++;
        }
    }
    if (!success) {
        return false;
    }
    ChildChangedInfo info { child, index, parent_ };
    if (!direct) {
        Invoke<IOnChildChanging>(implPreTrans_->OnRemoving(), info, success);
        if (success) {
            std::unique_lock lock(mutex_);
            children_.erase(children_.begin() + index);
        }
    }
    if (success) {
        SetObjectParent(child, nullptr);
        Invoke<IOnChildChanged>(impl_->OnRemoved(), info);
    }
    return success;
}

ChildMovedInfo ContainerBase::MoveInternal(SizeType fromIndex, SizeType toIndex)
{
    if (children_.empty()) {
        return {};
    }
    const auto size = children_.size();
    fromIndex = BASE_NS::Math::min(fromIndex, size - 1);
    toIndex = BASE_NS::Math::min(toIndex, size - 1);
    const IObject::Ptr child = children_[fromIndex];
    if (fromIndex == toIndex) {
        return { child, 0, 0 };
    }
    if (fromIndex > toIndex) {
        const auto first = children_.rbegin() + (size - fromIndex - 1);
        const auto last = children_.rbegin() + (size - toIndex);
        std::rotate(first, first + 1, last);
    } else {
        const auto first = children_.begin() + fromIndex;
        const auto last = children_.begin() + toIndex + 1;
        std::rotate(first, first + 1, last);
    }
    return { child, fromIndex, toIndex, parent_ };
}

bool ContainerBase::Move(SizeType fromIndex, SizeType toIndex)
{
    ChildMovedInfo info;
    {
        std::unique_lock lock(mutex_);
        info = MoveInternal(fromIndex, toIndex);
    }
    if (info.object) {
        if (info.from != info.to) {
            Invoke<IOnChildMoved>(impl_->OnMoved(), BASE_NS::move(info));
        }
        return true;
    }
    return false;
}

bool ContainerBase::Move(const IObject::Ptr& child, SizeType toIndex)
{
    ChildMovedInfo info;
    {
        std::unique_lock lock(mutex_);
        SizeType fromIndex = 0;
        for (const auto& c : children_) {
            if (c == child) {
                info = MoveInternal(fromIndex, toIndex);
                break;
            }
            fromIndex++;
        }
    }
    if (info.object) {
        if (info.from != info.to) {
            Invoke<IOnChildMoved>(impl_->OnMoved(), BASE_NS::move(info));
        }
        return true;
    }
    return false;
}

void ContainerBase::RemoveAll()
{
    BASE_NS::vector<IObject::Ptr> children;
    {
        std::unique_lock lock(mutex_);
        children_.swap(children);
    }

    SizeType index = 0;
    for (const auto& child : children) {
        ChildChangedInfo info { child, index++, parent_ };
        bool success = true;
        Invoke<IOnChildChanging>(implPreTrans_->OnRemoving(), info, success); // Ignore result
        if (!success) {
            CORE_LOG_E("Failing a remove transaction during remove all operation is not supported");
        }
        SetObjectParent(child, nullptr);
        Invoke<IOnChildChanged>(impl_->OnRemoved(), info);
    }
}

void ContainerBase::InternalRemoveAll()
{
    BASE_NS::vector<IObject::Ptr> children;
    {
        std::unique_lock lock(mutex_);
        children_.swap(children);
    }
    for (const auto& child : children) {
        if (auto c = interface_cast<IMutableContainable>(child)) {
            c->SetParent(nullptr);
        }
    }
}

bool ContainerBase::SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces)
{
    std::unique_lock lock(mutex_);
    required_ = interfaces;

    BASE_NS::vector<IObject::Ptr> compatible;
    compatible.reserve(children_.size());
    for (const auto& child : children_) {
        if (IsCompatible(child)) {
            compatible.push_back(child);
        }
    }
    children_.swap(compatible);
    return true;
}

BASE_NS::vector<TypeId> ContainerBase::GetRequiredInterfaces() const
{
    std::shared_lock lock(mutex_);
    return required_;
}

bool ContainerBase::IsCompatible(const IObject::Ptr& object) const
{
    return ObjectImplementsAll(object, required_);
}

bool ContainerBase::IsAncestorOf(const IObject::ConstPtr& object) const
{
    if (!object || !me_) {
        return false;
    }
    if (me_ == object.get()) {
        return true;
    }
    const auto containable = interface_pointer_cast<IContainable>(object);
    if (!containable) {
        return false;
    }
    auto parent = containable->GetParent();
    while (parent) {
        if (parent.get() == me_) {
            return true;
        }
        if (auto parentContainable = interface_cast<IContainable>(parent)) {
            parent = parentContainable->GetParent();
        } else {
            break;
        }
    }
    return false;
}

BASE_NS::shared_ptr<IEvent> ContainerBase::EventOnAdded() const
{
    return impl_->OnAdded();
}

BASE_NS::shared_ptr<IEvent> ContainerBase::EventOnRemoved() const
{
    return impl_->OnRemoved();
}

BASE_NS::shared_ptr<IEvent> ContainerBase::EventOnMoved() const
{
    return impl_->OnMoved();
}

BASE_NS::shared_ptr<IEvent> ContainerBase::EventOnAdding() const
{
    return implPreTrans_->OnAdding();
}

BASE_NS::shared_ptr<IEvent> ContainerBase::EventOnRemoving() const
{
    return implPreTrans_->OnRemoving();
}

template<typename Cont, typename Func>
static IterationResult IterateImpl(Cont& cont, const Func& func)
{
    for (auto&& child : cont) {
        auto res = func->Invoke(child);
        if (!res.Continue()) {
            return res;
        }
    }
    return IterationResult::CONTINUE;
}

IterationResult ContainerBase::Iterate(const IterationParameters& params)
{
    auto f = params.function.GetInterface<IIterableCallable<IObject::Ptr>>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return IterationResult::FAILED;
    }
    return IterateImpl(children_, f);
}

IterationResult ContainerBase::Iterate(const IterationParameters& params) const
{
    auto f = params.function.GetInterface<IIterableConstCallable<IObject::Ptr>>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return IterationResult::FAILED;
    }
    return IterateImpl(children_, f);
}

META_END_NAMESPACE()
