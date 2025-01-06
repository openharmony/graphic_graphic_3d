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

bool FlatContainer::Add(const META_NS::IObject::Ptr& object)
{
    return Insert(-1, object);
}

bool FlatContainer::Insert(SizeType index, const IObject::Ptr& object)
{
    if (!object) {
        return false;
    }
    {
        std::unique_lock lock(mutex_);
        if (!IsCompatible(object)) {
            return false;
        }
        index = BASE_NS::Math::min(index, children_.size());
        children_.insert(children_.begin() + index, object);
    }
    ChildChangedInfo info { ContainerChangeType::ADDED, object, parent_, size_t(-1), index };
    SetObjectParent(object, interface_pointer_cast<IObject>(parent_));
    Invoke<IOnChildChanged>(EventOnContainerChanged(MetadataQuery::EXISTING), info);
    return true;
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
        auto it = children_.begin();
        for (; it != children_.end() && *it != child; ++it, ++index) {
        }
        if (it != children_.end()) {
            removed = *it;
            if (removed == replaceWith) {
                return removed != nullptr;
            }
            if (replaceWith) {
                *it = replaceWith;
                added = replaceWith;
            } else {
                children_.erase(it);
            }
        } else if (addAlways && replaceWith) {
            children_.push_back(replaceWith);
            added = replaceWith;
        }
    }
    ChildChangedInfo addedInfo { ContainerChangeType::ADDED, added, parent_, size_t(-1), index };
    ChildChangedInfo removedInfo { ContainerChangeType::REMOVED, removed, parent_, index };
    if (removed) {
        SetObjectParent(removed, nullptr);
        Invoke<IOnChildChanged>(EventOnContainerChanged(MetadataQuery::EXISTING), removedInfo);
    }
    if (added) {
        SetObjectParent(added, interface_pointer_cast<IObject>(parent_));
        Invoke<IOnChildChanged>(EventOnContainerChanged(MetadataQuery::EXISTING), addedInfo);
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

BASE_NS::shared_ptr<IEvent> FlatContainer::EventOnContainerChanged(MetadataQuery q) const
{
    std::unique_lock lock(this->mutex_);
    if (!onChanged_ && q == MetadataQuery::CONSTRUCT_ON_REQUEST) {
        onChanged_ = { CreateShared<EventImpl<IOnChildChanged>>("OnContainerChanged") };
    }
    return onChanged_;
}

META_END_NAMESPACE()
