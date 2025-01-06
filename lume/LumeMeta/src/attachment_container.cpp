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
#include "attachment_container.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>

META_BEGIN_NAMESPACE()

AttachmentContainer::AttachmentContainer()
{
    Super::SetImplementingIContainer(nullptr, this);
}

AttachmentContainer::~AttachmentContainer()
{
    RemoveAllAttachments();
}

bool AttachmentContainer::Add(const IObject::Ptr& object)
{
    return Attach(N_POS, object, {});
}

bool AttachmentContainer::Insert(IContainer::SizeType index, const IObject::Ptr& object)
{
    return Attach(index, object, {});
}

bool AttachmentContainer::Remove(IContainer::SizeType index)
{
    return Remove(GetAt(index));
}

bool AttachmentContainer::Remove(const IObject::Ptr& child)
{
    return Detach(child);
}

bool AttachmentContainer::Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways)
{
    const auto owner = owner_.lock();
    if (child && AlreadyAttached(replaceWith)) {
        return true;
    }

    if (auto i = interface_pointer_cast<IAttachable>(replaceWith)) {
        if (!i->Attaching(owner, nullptr)) {
            return false;
        }
    }

    bool res = Super::Replace(child, replaceWith, addAlways);
    if (res) {
        if (auto i = interface_pointer_cast<IAttachable>(child)) {
            i->Detaching(owner);
        }
    } else {
        if (auto i = interface_pointer_cast<IAttachable>(replaceWith)) {
            i->Detaching(owner);
        }
    }
    return res;
}

void AttachmentContainer::RemoveAll()
{
    RemoveAllAttachments();
}

bool AttachmentContainer::SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces)
{
    CORE_LOG_E("Setting the required interfaces of an attachment container is not allowed.");
    return false;
}

bool AttachmentContainer::Initialize(const META_NS::IAttach::Ptr& owner)
{
    if (!owner) {
        return false;
    }
    owner_ = owner;
    return true;
}

bool AttachmentContainer::Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext)
{
    return Attach(N_POS, attachment, dataContext);
}

bool AttachmentContainer::DetachFromOld(const IAttach::Ptr& me, const IObject::Ptr& attachment, bool& mine)
{
    // If attachment is attached to something, detach it
    if (const auto att = interface_cast<IAttachment>(attachment)) {
        if (const auto current = GetValue(att->AttachedTo()).lock()) {
            mine = current == me;
            if (!mine) {
                return current->Detach(attachment);
            }
        }
    }
    return true;
}

bool AttachmentContainer::Attach(
    IContainer::SizeType pos, const IObject::Ptr& attachment, const IObject::Ptr& dataContext)
{
    if (!attachment) {
        return false;
    }
    bool result = false;
    if (const auto owner = owner_.lock()) {
        // If attachment is attached to something, detach it
        bool mine = false;
        if (!DetachFromOld(owner, attachment, mine)) {
            return false;
        }
        if (mine) {
            return true;
        }
        if (auto i = interface_pointer_cast<IAttachable>(attachment)) {
            // If no data context given, use this as the data context
            const auto context = dataContext ? dataContext : interface_pointer_cast<IObject>(owner);
            if (i->Attaching(owner, context)) {
                result = Super::Insert(pos, attachment);
                if (!result) {
                    i->Detaching(owner);
                }
            }
        } else {
            result = Super::Insert(pos, attachment);
        }
    }
    return result;
}

bool AttachmentContainer::Detach(const IObject::Ptr& attachment)
{
    if (!attachment) {
        return false;
    }
    if (const auto owner = owner_.lock()) {
        bool res = Super::Remove(attachment);
        if (res) {
            if (auto i = interface_pointer_cast<IAttachable>(attachment)) {
                i->Detaching(owner);
            }
        }
        return res;
    }
    return false;
}

BASE_NS::vector<IObject::Ptr> AttachmentContainer::GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict)
{
    return Super::FindAll({ "", TraversalType::NO_HIERARCHY, uids, strict });
}

void AttachmentContainer::RemoveAllAttachments()
{
    const auto owner = owner_.lock();
    const auto all = Super::GetAll();
    for (const auto& object : all) {
        if (auto att = interface_cast<IAttachable>(object)) {
            // Ignore result
            att->Detaching(owner);
        }
    }
    Super::RemoveAll();
}

IObject::Ptr AttachmentContainer::FindByName(const BASE_NS::string& name) const
{
    return ContainerBase::FindByName(name);
}

bool AttachmentContainer::AlreadyAttached(const IObject::Ptr& object)
{
    if (const auto attachment = interface_pointer_cast<IAttachment>(object)) {
        if (const auto owner = owner_.lock(); attachment && owner) {
            if (const auto current = GetValue(attachment->AttachedTo()).lock()) {
                if (current == owner) {
                    // Already attached to this
                    return true;
                }
            }
        }
    } else {
        BASE_NS::shared_ptr<IContainer> self(this, [](auto) {});
        return META_NS::ContainsObject(self, object);
    }
    return false;
}

META_END_NAMESPACE()
