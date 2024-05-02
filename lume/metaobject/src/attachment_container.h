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

#ifndef META_SRC_ATTACHMENT_CONTAINER_H
#define META_SRC_ATTACHMENT_CONTAINER_H

#include <shared_mutex>

#include <base/containers/unordered_map.h>

#include <meta/base/namespace.h>
#include <meta/ext/object.h>
#include <meta/ext/object_container.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attachment_container.h>
#include <meta/interface/intf_object_hierarchy_observer.h>

META_BEGIN_NAMESPACE()

class AttachmentContainer final : public ObjectContainerFwd<AttachmentContainer, ClassId::AttachmentContainer,
                                      IAttachmentContainer, IContainerPreTransaction> {
    using Super = ObjectContainerFwd<AttachmentContainer, ClassId::AttachmentContainer, IAttachmentContainer,
        IContainerPreTransaction>;

public:
    ~AttachmentContainer() override;
    AttachmentContainer();
    META_NO_COPY_MOVE(AttachmentContainer)
    // ILifecycle
    bool Build(const IMetadata::Ptr& data) override;

protected: // IDerived
    void SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super) override;

protected: // IContainer
    bool Add(const IObject::Ptr& object) override;
    bool Insert(IContainer::SizeType index, const IObject::Ptr& object) override;
    bool Remove(IContainer::SizeType index) override;
    bool Remove(const IObject::Ptr& child) override;
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override;
    void RemoveAll() override;
    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) override;

protected: // IContainerPreTransaction
    META_FORWARD_EVENT(IOnChildChanging, OnAdding, transaction_->EventOnAdding());
    META_FORWARD_EVENT(IOnChildChanging, OnRemoving, transaction_->EventOnRemoving());

protected: // IAttachmentContainer
    bool Initialize(const META_NS::IAttach::Ptr& owner) override;
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override;
    bool Attach(IContainer::SizeType pos, const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override;
    bool Detach(const IObject::Ptr& attachment) override;
    BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) override;
    void RemoveAllAttachments() override;
    IObject::Ptr FindByName(const BASE_NS::string& name) const override;

private:
    void RemovedFromContainer(const ChildChangedInfo& info);
    void AddingToContainer(const ChildChangedInfo& info, bool& success);

    bool AlreadyAttached(const IObject::Ptr& object);
    IContainerPreTransaction* transaction_ {};
    META_NS::IAttach::WeakPtr owner_;
    BASE_NS::vector<BASE_NS::pair<IObject*, IObject::WeakPtr>> addingContexts_;
    static constexpr IContainer::SizeType N_POS = std::numeric_limits<IContainer::SizeType>::max();
};

META_END_NAMESPACE()

#endif
