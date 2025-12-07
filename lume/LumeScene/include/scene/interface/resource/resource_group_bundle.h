/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_RESOURCE_RESOURCE_GROUP_BUNDLE_H
#define SCENE_INTERFACE_RESOURCE_RESOURCE_GROUP_BUNDLE_H

#include <scene/base/namespace.h>

#include <meta/interface/resource/intf_resource_group_handle.h>

SCENE_BEGIN_NAMESPACE()

class ResourceGroupBundle {
public:
    ResourceGroupBundle(BASE_NS::vector<META_NS::IResourceGroupHandle::Ptr> h = {}) : handles_(BASE_NS::move(h)) {}

    bool IsValid() const
    {
        return !handles_.empty();
    }

    BASE_NS::string PrimaryGroup() const
    {
        auto h = PrimaryGroupHandle();
        return h ? h->GetGroup() : "";
    }
    META_NS::IResourceGroupHandle::Ptr PrimaryGroupHandle() const
    {
        return handles_.empty() ? nullptr : handles_.front();
    }
    const BASE_NS::vector<META_NS::IResourceGroupHandle::Ptr>& GetAllHandles() const
    {
        return handles_;
    }

    void PushGroupHandleToFront(META_NS::IResourceGroupHandle::Ptr handle)
    {
        handles_.insert(handles_.begin(), BASE_NS::move(handle));
    }

    void PushGroupHandleToBack(META_NS::IResourceGroupHandle::Ptr handle)
    {
        handles_.push_back(BASE_NS::move(handle));
    }

    META_NS::IResourceGroupHandle::Ptr GetHandle(BASE_NS::string_view group)
    {
        for (auto it = handles_.begin(); it != handles_.end(); ++it) {
            if ((*it)->GetGroup() == group) {
                return *it;
            }
        }
        return nullptr;
    }

    META_NS::IResourceGroupHandle::Ptr RemoveHandle(BASE_NS::string_view group)
    {
        for (auto it = handles_.begin(); it != handles_.end(); ++it) {
            if ((*it)->GetGroup() == group) {
                auto v = *it;
                handles_.erase(it);
                return v;
            }
        }
        return nullptr;
    }

private:
    BASE_NS::vector<META_NS::IResourceGroupHandle::Ptr> handles_;
};

SCENE_END_NAMESPACE()

#endif
