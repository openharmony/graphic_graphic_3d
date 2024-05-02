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
#include "object.h"

#include <algorithm>
#include <limits>

#include <base/util/uid_util.h>
#include <core/plugin/intf_class_factory.h>

#include <meta/api/iteration.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_proxy_object.h>
#include <meta/interface/intf_required_interfaces.h>
#include <meta/interface/property/intf_property_internal.h>

#include "ref_uri_util.h"

META_BEGIN_NAMESPACE()
namespace Internal {

// ILifecycle
bool Object::Build(const IMetadata::Ptr& data)
{
    return Super::Build(data);
}

void Object::Destroy()
{
    if (attachments_) {
        attachments_->RemoveAllAttachments();
        attachments_.reset();
    }
    Super::Destroy();
}

// IAttach
bool Object::Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext)
{
    ValidateAttachmentContainer();
    if (attachments_) {
        return attachments_->Attach(attachment, dataContext);
    }
    return false;
}

bool Object::Detach(const IObject::Ptr& attachment)
{
    if (attachments_) {
        return attachments_->Detach(attachment);
    }
    return false;
}

BASE_NS::vector<IObject::Ptr> Object::GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) const
{
    if (attachments_) {
        return attachments_->GetAttachments(uids, strict);
    }
    return {};
}
bool Object::HasAttachments() const
{
    if (const auto container = interface_cast<IContainer>(attachments_)) {
        return container->GetSize() > 0;
    }
    return false;
}
IContainer::Ptr Object::GetAttachmentContainer(bool initializeAlways) const
{
    if (initializeAlways) {
        ValidateAttachmentContainer();
    }
    return interface_pointer_cast<IContainer>(attachments_);
}

void Object::ValidateAttachmentContainer() const
{
    if (!attachments_) {
        if (attachments_ =
                META_NS::GetObjectRegistry().Create<META_NS::IAttachmentContainer>(ClassId::AttachmentContainer);
            attachments_) {
            attachments_->Initialize(GetSelf<META_NS::IAttach>());
        } else {
            CORE_LOG_E("Failed to create container for attachments");
        }
    }
}

bool CheckRequiredInterfaces(const IContainer::Ptr& container, const BASE_NS::vector<BASE_NS::Uid>& uids)
{
    if (uids.empty()) {
        return true;
    }
    if (auto req = interface_cast<IRequiredInterfaces>(container)) {
        const auto reqs = req->GetRequiredInterfaces();
        if (reqs.empty()) {
            return true; // Container has no requirements related to the interfaces it accepts
        }
        size_t matches = 0;
        for (const auto& uid : uids) {
            if (std::find(reqs.begin(), reqs.end(), uid) != reqs.end()) {
                matches++;
            }
        }
        return matches == uids.size();
    }

    // If container is valid but it does not implement IRequiredInterfaces, anything goes
    return container.operator bool();
}

BASE_NS::vector<IContainer::Ptr> Object::FindAllContainers(const ContainerFindOptions& options) const
{
    BASE_NS::vector<IContainer::Ptr> containers;
    const auto maxCount = options.maxCount ? options.maxCount : std::numeric_limits<size_t>::max();
    const auto& uids = options.uids;
    const auto addIfMatches = [&containers, &uids](const IContainer::Ptr& container) {
        if (container) {
            if (CheckRequiredInterfaces(container, uids)) {
                containers.push_back(container);
            }
        }
    };
    if (const auto me = GetSelf<IContainer>()) {
        // This object is itself a container
        addIfMatches(me);
    }
    if (containers.size() < maxCount) {
        if (HasAttachments()) {
            // Check the attachment container
            addIfMatches(interface_pointer_cast<IContainer>(attachments_));
            // Check the attachments themselves
            if (containers.size() < maxCount) {
                IterateShared(attachments_, [&addIfMatches, &containers, &maxCount](const IObject::Ptr& object) {
                    addIfMatches(interface_pointer_cast<IContainer>(object));
                    return containers.size() < maxCount;
                });
            }
        } else {
            // No attachments, but the user has requested IAttachment so we need to create the container
            if (uids.empty() || std::find(uids.begin(), uids.end(), IAttachment::UID) != uids.end()) {
                ValidateAttachmentContainer();
                addIfMatches(interface_pointer_cast<IContainer>(attachments_));
            }
        }
    }
    return containers;
}

const StaticObjectMetadata& Object::GetStaticMetadata() const
{
    return GetStaticObjectMetadata();
}

} // namespace Internal

META_END_NAMESPACE()
