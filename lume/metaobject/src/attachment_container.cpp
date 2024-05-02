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

AttachmentContainer::AttachmentContainer() = default;

AttachmentContainer::~AttachmentContainer()
{
    RemoveAllAttachments();
}

void AttachmentContainer::SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super)
{
    Super::SetSuperInstance(aggr, super);
    transaction_ = interface_cast<IContainerPreTransaction>(super);
    CORE_ASSERT(transaction_);
}

bool AttachmentContainer::Build(const IMetadata::Ptr& data)
{
    // Always set null as the parent of all our attachments
    ObjectContainerFwd::SetProxyParent({});

    OnRemoved()->AddHandler(MakeCallback<IOnChildChanged>(this, &AttachmentContainer::RemovedFromContainer));
    OnAdding()->AddHandler(MakeCallback<IOnChildChanging>(this, &AttachmentContainer::AddingToContainer));
    return true;
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
    return ObjectContainerFwd::Replace(child, replaceWith, addAlways);
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

bool AttachmentContainer::Attach(
    IContainer::SizeType pos, const IObject::Ptr& attachment, const IObject::Ptr& dataContext)
{
    const auto object = interface_pointer_cast<IObject>(attachment);
    if (!object) {
        return false;
    }
    bool result = false;
    if (const auto owner = owner_.lock()) {
        // If attachment is attached to something, detach it
        if (const auto att = interface_cast<IAttachment>(attachment)) {
            if (const auto current = GetValue(att->AttachedTo()).lock()) {
                if (current == owner) {
                    // Already attached to this
                    return true;
                }
                if (!current->Detach(attachment)) {
                    return false;
                }
            }
        }
        // If no data context given, use this as the data context
        const auto context = dataContext ? dataContext : interface_pointer_cast<IObject>(owner);
        addingContexts_.emplace_back(BASE_NS::pair<IObject*, IObject::WeakPtr> { attachment.get(), context });
        result = ObjectContainerFwd::Insert(pos, object);
        if (!result) {
            addingContexts_.pop_back();
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
        if (const auto att = interface_cast<IAttachment>(attachment)) {
            const auto current = GetValue(att->AttachedTo()).lock();
            if (current != owner) {
                return false;
            }
        }
        return ObjectContainerFwd::Remove(attachment);
    }
    return false;
}

void AttachmentContainer::AddingToContainer(const ChildChangedInfo& info, bool& success)
{
    if (const auto owner = owner_.lock()) {
        if (auto object = info.object) {
            IObject::Ptr context;
            for (auto it = addingContexts_.begin(); it != addingContexts_.end(); it++) {
                if (it->first == object.get()) {
                    context = it->second.lock();
                    addingContexts_.erase(it);
                    break;
                }
            }
            if (auto attachment = interface_pointer_cast<IAttachment>(info.object)) {
                if (GetValue(attachment->AttachedTo()).lock() == owner) {
                    // Already attached to this
                    return;
                }
                if (!attachment->Attaching(owner, context)) {
                    success = false;
                }
            } else {
                success = true;
            }
        }
    }
}
void AttachmentContainer::RemovedFromContainer(const ChildChangedInfo& info)
{
    if (const auto owner = owner_.lock()) {
        if (auto attachment = interface_pointer_cast<IAttachment>(info.object)) {
            attachment->Detaching(owner);
        }
    }
}

BASE_NS::vector<IObject::Ptr> AttachmentContainer::GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict)
{
    return ObjectContainerFwd::FindAll({ "", TraversalType::NO_HIERARCHY, uids, strict });
}

void AttachmentContainer::RemoveAllAttachments()
{
    const auto owner = owner_.lock();
    const auto all = ObjectContainerFwd::GetAll();
    for (const auto& object : all) {
        if (auto att = interface_cast<IAttachment>(object)) {
            // Ignore result
            att->Detaching(owner);
        }
    }
    ObjectContainerFwd::RemoveAll();
}

IObject::Ptr AttachmentContainer::FindByName(const BASE_NS::string& name) const
{
    return ObjectContainerFwd::FindByName(name);
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
        return META_NS::ContainsObject(GetSelf<IContainer>(), object);
    }
    return false;
}

META_END_NAMESPACE()
