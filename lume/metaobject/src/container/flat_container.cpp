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
#include "flat_container.h"

#include <algorithm>

#include <base/math/mathf.h>

#include <meta/api/internal/iteration.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>

META_BEGIN_NAMESPACE()

IObject::Ptr FlatContainer::FindAny(const IContainer::FindOptions& options) const
{
    return ContainerBase::FindAnyImpl(options, true);
}

BASE_NS::vector<IObject::Ptr> FlatContainer::FindAll(const IContainer::FindOptions& options) const
{
    return ContainerBase::FindAllImpl(options, true);
}

bool FlatContainer::Add(const IObject::Ptr& object)
{
    if (!object) {
        return false;
    }
    const auto direct = !OnAdding()->HasHandlers();
    SizeType index = 0;
    {
        std::unique_lock lock(mutex_);
        if (!IsCompatible(object)) {
            return false;
        }
        index = children_.size();
        if (direct) {
            children_.push_back(object);
        }
    }
    bool success = true;
    ChildChangedInfo info { object, index, parent_ };
    if (!direct) {
        Invoke<IOnChildChanging>(OnAdding(), info, success);
        if (success) {
            std::unique_lock lock(mutex_);
            children_.push_back(object);
        }
    }
    // Calling external interface methods outside of our internal lock
    if (success) {
        SetObjectParent(object, interface_pointer_cast<IObject>(parent_));
        Invoke<IOnChildChanged>(OnAdded(), info);
    }
    return success;
}

bool FlatContainer::Insert(SizeType index, const IObject::Ptr& object)
{
    if (!object) {
        return false;
    }
    const auto direct = !OnAdding()->HasHandlers();
    {
        std::unique_lock lock(mutex_);
        if (!IsCompatible(object)) {
            return false;
        }
        index = BASE_NS::Math::min(index, children_.size());
        if (direct) {
            children_.insert(children_.begin() + index, object);
        }
    }
    bool success = true;
    ChildChangedInfo info { object, index, parent_ };
    if (!direct) {
        Invoke<IOnChildChanging>(OnAdding(), info, success);
        if (success) {
            std::unique_lock lock(mutex_);
            children_.insert(children_.begin() + index, object);
        }
    }
    // Calling external interface methods outside of our internal lock
    if (success) {
        SetObjectParent(object, interface_pointer_cast<IObject>(parent_));
        Invoke<IOnChildChanged>(OnAdded(), ChildChangedInfo { object, index, parent_ });
    }
    return success;
}

bool FlatContainer::Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways)
{
    SizeType index = 0;
    IObject::Ptr added;
    IObject::Ptr removed;
    {
        std::unique_lock lock(mutex_);
        if (replaceWith && !IsCompatible(replaceWith)) {
            return false;
        }
        for (auto&& v : children_) {
            if (child == v) {
                break;
            }
            ++index;
        }
        if (index < children_.size()) {
            removed = children_[index];
            if (removed == replaceWith) {
                return removed != nullptr;
            }
            if (replaceWith) {
                children_[index] = replaceWith;
                added = replaceWith;
            } else {
                children_.erase(children_.begin() + index);
            }
        } else if (addAlways && replaceWith) {
            children_.push_back(replaceWith);
            added = replaceWith;
        }
    }
    ChildChangedInfo addedInfo { added, index, parent_ };
    ChildChangedInfo removedInfo { removed, index, parent_ };
    bool success = true;
    if (removed) {
        Invoke<IOnChildChanging>(OnRemoving(), removedInfo, success);
        if (!success) {
            CORE_LOG_E("Failing a remove transaction during replace operation is not supported");
            success = true;
        }
    }
    if (added) {
        Invoke<IOnChildChanging>(OnAdding(), addedInfo, success);
        if (!success) {
            CORE_LOG_E("Failing an add transaction during replace operation is not supported");
        }
    }
    if (removed) {
        SetObjectParent(removed, nullptr);
        Invoke<IOnChildChanged>(OnRemoved(), removedInfo);
    }
    if (added) {
        SetObjectParent(added, interface_pointer_cast<IObject>(parent_));
        Invoke<IOnChildChanged>(OnAdded(), addedInfo);
    }
    return added || removed;
}

void FlatContainer::SetObjectParent(const IObject::Ptr& object, const IObject::Ptr& parent) const
{
    const auto set = interface_cast<IMutableContainable>(object);
    if (!set) {
        // Object does not support setting a parent
        return;
    }
    if (const auto cont = interface_cast<IContainable>(object)) {
        // Remove from old parent (if any)
        if (const auto old = interface_pointer_cast<IContainer>(cont->GetParent())) {
            if (old == interface_pointer_cast<IContainer>(parent)) {
                // The object is already a child of the new parent container
                return;
            }
            old->Remove(object);
        }
    }
    if (!parent) {
        for (auto&& c : children_) {
            // we have another, don't remove the parent
            if (c == object) {
                return;
            }
        }
    }
    set->SetParent(parent);
}

META_END_NAMESPACE()
