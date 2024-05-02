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

#include "container.h"

#include <algorithm>

#include <base/math/mathf.h>

#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

IObject::Ptr Container::FindAny(const META_NS::IContainer::FindOptions& options) const
{
    return ContainerBase::FindAnyImpl(options, false);
}

BASE_NS::vector<IObject::Ptr> Container::FindAll(const META_NS::IContainer::FindOptions& options) const
{
    return ContainerBase::FindAllImpl(options, false);
}

bool Container::Add(const META_NS::IObject::Ptr& object)
{
    if (!object) {
        return false;
    }
    const auto direct = !OnAdding()->HasHandlers();
    SizeType index;
    {
        std::unique_lock lock(mutex_);
        if (!IsCompatible(object) || !CheckLoop(object)) {
            return false;
        }
        for (const auto& child : children_) {
            if (child == object) {
                return true; // Already a child of ours
            }
        }
        index = children_.size();
        if (direct) {
            children_.push_back(object);
        }
    }
    // Calling external interface methods outside of our internal lock
    bool success = true;
    ChildChangedInfo info { object, index, parent_ };
    if (!direct) {
        Invoke<IOnChildChanging>(OnAdding(), info, success);
        if (success) {
            std::unique_lock lock(mutex_);
            children_.push_back(object);
        }
    }
    if (success) {
        SetObjectParent(object, interface_pointer_cast<IObject>(parent_));
        Invoke<IOnChildChanged>(OnAdded(), info);
    }
    return success;
}

bool Container::Insert(SizeType index, const IObject::Ptr& object)
{
    if (!object) {
        return false;
    }
    const auto direct = !OnAdding()->HasHandlers();
    {
        std::unique_lock lock(mutex_);
        if (!IsCompatible(object) || !CheckLoop(object)) {
            return false;
        }
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (*it == object) {
                // Already in the container, remove from current position
                children_.erase(it);
                break;
            }
        }
        index = BASE_NS::Math::min(index, children_.size());
        if (direct) {
            children_.insert(children_.begin() + index, object);
        }
    }
    // Calling external interface methods outside of our internal lock
    bool success = true;
    ChildChangedInfo info { object, index, parent_ };
    if (!direct) {
        Invoke<IOnChildChanging>(OnAdding(), info, success);
        if (success) {
            std::unique_lock lock(mutex_);
            children_.insert(children_.begin() + index, object);
        }
    }
    if (success) {
        SetObjectParent(object, interface_pointer_cast<IObject>(parent_));
        Invoke<IOnChildChanged>(OnAdded(), info);
    }
    return success;
}

bool Container::Replace(const META_NS::IObject::Ptr& child, const META_NS::IObject::Ptr& replaceWith, bool addAlways)
{
    IObject::Ptr removed;
    IObject::Ptr added;
    IObject::Ptr moved;
    IContainer::SizeType fromIndex = 0;
    IContainer::SizeType toIndex = 0;
    bool changed = false; // True if any changes were made
    {
        std::unique_lock lock(mutex_);
        if (replaceWith && (!IsCompatible(replaceWith) || !CheckLoop(replaceWith))) {
            return false;
        }
        auto removedIt = children_.end();
        auto movedIt = children_.end();
        SizeType index = 0;
        // Check if 'child' and 'replaceWith' can already be found from the container
        for (auto it = children_.begin(); it != children_.end(); ++it, ++index) {
            if (*it == child) {
                removedIt = it;
                removed = *it;
                toIndex = index;
            }
            if (*it == replaceWith) {
                movedIt = it;
                moved = *it;
                fromIndex = index;
            }
            if (removed && moved) {
                break;
            }
        }
        if (removed && removed == moved) {
            // 'child' and 'replaceWith' are the same item (which is already in the container)
            return true;
        }
        if (removed) {
            // 'child' was found
            if (replaceWith) {
                *removedIt = replaceWith;
                if (moved) {
                    // 'replaceWith' was already in the container, remove it from its previous position
                    children_.erase(movedIt);
                } else {
                    // 'replaceWith' was added
                    added = replaceWith;
                }
            } else {
                // 'replaceWith' == null, remove 'child'
                children_.erase(removedIt);
            }
            changed = true;
        } else {
            moved.reset();
        }
        if (!changed && addAlways && replaceWith && movedIt == children_.end()) {
            // 'child' was not found in container but addAlways is specified with a valid new item which
            // was not previously in the container, add it
            toIndex = children_.size();
            children_.push_back(replaceWith);
            added = replaceWith;
            changed = true;
        }
    }
    if (changed) {
        // We did some changes to the container
        ChildChangedInfo addedInfo { added, toIndex, parent_ };
        ChildChangedInfo removedInfo { removed, toIndex, parent_ };
        bool success = true;
        // Replace operation does not support
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
        SetObjectParent(removed, nullptr);
        SetObjectParent(added, interface_pointer_cast<IObject>(parent_));
        if (removed) {
            Invoke<IOnChildChanged>(OnRemoved(), removedInfo);
        }
        if (added) {
            Invoke<IOnChildChanged>(OnAdded(), addedInfo);
        }
        if (moved) {
            Invoke<IOnChildMoved>(OnMoved(), ChildMovedInfo { moved, fromIndex, toIndex, parent_ });
        }
    }
    return changed;
}

void Container::SetObjectParent(const IObject::Ptr& object, const IObject::Ptr& parent) const
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
    set->SetParent(parent);
}

bool Container::CheckLoop(const IObject::Ptr& object) const
{
    // Check if adding object to "this" would lead to a loop in the hierarchy.
    // This can only happen if object itself is a container which happens to be
    // an ancestor of ours.
    if (const auto cc = interface_cast<const IContainer>(object)) {
        if (const auto me = interface_cast<IObjectInstance>(impl_)) {
            if (cc->IsAncestorOf(me->GetSelf())) {
                CORE_LOG_E("Adding '%s' to '%s' would lead to a loop in the container", object->GetName().c_str(),
                    me->GetName().c_str());
                return false;
            }
        }
    }
    return true;
}

META_END_NAMESPACE()
