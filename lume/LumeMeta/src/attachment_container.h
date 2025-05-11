/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Attachment container implementation
 * Author: Lauri Jaaskela
 * Create: 2022-09-26
 */

#ifndef META_SRC_ATTACHMENT_CONTAINER_H
#define META_SRC_ATTACHMENT_CONTAINER_H

#include <meta/base/namespace.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attachment_container.h>
#include <meta/interface/intf_object_hierarchy_observer.h>

#include "container/flat_container.h"

META_BEGIN_INTERNAL_NAMESPACE()

class AttachmentContainer : public IntroduceInterfaces<MinimalObject, FlatContainer, IAttachmentContainer> {
    using Super = IntroduceInterfaces<MinimalObject, FlatContainer, IAttachmentContainer>;
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::AttachmentContainer)
    META_NO_COPY_MOVE(AttachmentContainer)
public:
    AttachmentContainer();
    ~AttachmentContainer() override;

protected: // IContainer
    bool Add(const IObject::Ptr& object) override;
    bool Insert(IContainer::SizeType index, const IObject::Ptr& object) override;
    bool Remove(IContainer::SizeType index) override;
    bool Remove(const IObject::Ptr& child) override;
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override;
    void RemoveAll() override;
    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) override;

protected: // IAttachmentContainer
    bool Initialize(const META_NS::IAttach::Ptr& owner) override;
    using IAttachmentContainer::Attach;
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override;
    bool Attach(IContainer::SizeType pos, const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override;
    using IAttachmentContainer::Detach;
    bool Detach(const IObject::Ptr& attachment) override;
    BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) override;
    void RemoveAllAttachments() override;
    IObject::Ptr FindByName(const BASE_NS::string& name) const override;

    template<typename Interface>
    typename Interface::Ptr GetOwner() const
    {
        return interface_pointer_cast<Interface>(owner_);
    }

    bool DetachFromOld(const IAttach::Ptr& me, const IObject::Ptr& attachment, bool& mine);
    bool InternalAttach(const IObject::Ptr& attachment);
    template<typename T>
    bool InternalAttach(const T& obj)
    {
        return InternalAttach(interface_pointer_cast<IObject>(obj));
    }

private:
    void RemovedFromContainer(const ChildChangedInfo& info);
    void AddingToContainer(const ChildChangedInfo& info, bool& success);

    bool AlreadyAttached(const IObject::Ptr& object);
    IAttach::WeakPtr owner_;
    static constexpr IContainer::SizeType N_POS = std::numeric_limits<IContainer::SizeType>::max();
};

META_END_INTERNAL_NAMESPACE()

#endif
