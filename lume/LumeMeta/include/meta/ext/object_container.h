/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Helper class for implementing IContainer interface
 * Author: Lauri Jaaskela
 * Create: 2023-01-18
 */

#ifndef META_EXT_OBJECT_CONTAINER_H
#define META_EXT_OBJECT_CONTAINER_H

#include <meta/ext/object_fwd.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_required_interfaces.h>

META_BEGIN_NAMESPACE()

/**
 * @brief A helper template class for implementing classes which implement the IContainer interface by
 *        using ClassId::ObjectContainer as their base class.
 */

class CommonObjectContainerFwd : public IntroduceInterfaces<ObjectFwd, IContainer, IContainerProxyParent,
                                     IRequiredInterfaces, ILockable, IIterable> {
    META_OBJECT_NO_CLASSINFO(CommonObjectContainerFwd, IntroduceInterfaces, ClassId::ObjectContainer)

protected:
    void SetSuperInstance(const META_NS::IObject::Ptr& aggr, const META_NS::IObject::Ptr& super) override
    {
        Super::SetSuperInstance(aggr, super);
        container_ = interface_cast<IContainer>(super);
        CORE_ASSERT_MSG(container_, "Invalid super object");
    }

protected: // IContainer
    BASE_NS::vector<IObject::Ptr> GetAll() const override
    {
        return container_->GetAll();
    }
    IContainer::SizeType GetSize() const override
    {
        return container_->GetSize();
    }
    BASE_NS::vector<IObject::Ptr> FindAll(const IContainer::FindOptions& options) const override
    {
        return container_->FindAll(options);
    }
    IObject::Ptr FindAny(const IContainer::FindOptions& options) const override
    {
        return container_->FindAny(options);
    }
    IObject::Ptr FindByName(BASE_NS::string_view name) const override
    {
        return container_->FindByName(name);
    }
    bool Add(const IObject::Ptr& object) override
    {
        return container_->Add(object);
    }
    bool Insert(IContainer::SizeType index, const IObject::Ptr& object) override
    {
        return container_->Insert(index, object);
    }
    using IContainer::GetAt;
    IObject::Ptr GetAt(IContainer::SizeType index) const override
    {
        return container_->GetAt(index);
    }
    bool Remove(IContainer::SizeType index) override
    {
        return container_->Remove(index);
    }
    bool Remove(const IObject::Ptr& child) override
    {
        return container_->Remove(child);
    }
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override
    {
        return container_->Replace(child, replaceWith, addAlways);
    }
    bool Move(IContainer::SizeType fromIndex, IContainer::SizeType toIndex) override
    {
        return container_->Move(fromIndex, toIndex);
    }
    bool Move(const IObject::Ptr& child, IContainer::SizeType toIndex) override
    {
        return container_->Move(child, toIndex);
    }
    void RemoveAll() override
    {
        container_->RemoveAll();
    }
    META_FORWARD_EVENT(IOnChildChanged, OnContainerChanged, container_->EventOnContainerChanged)

    bool IsAncestorOf(const IObject::ConstPtr& object) const override
    {
        return container_->IsAncestorOf(object);
    }

protected: // IRequiredInterfaces
    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) override
    {
        return META_EXT_CALL_BASE(IRequiredInterfaces, SetRequiredInterfaces(interfaces));
    }
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const override
    {
        return META_EXT_CALL_BASE(IRequiredInterfaces, GetRequiredInterfaces());
    }

protected: // IContainerProxyParent
    bool SetProxyParent(const IContainer::Ptr& parent) override
    {
        return META_EXT_CALL_BASE(IContainerProxyParent, SetProxyParent(parent));
    }

protected: // ILockable
    void LockShared() const override
    {
        META_EXT_CALL_BASE(ILockable, LockShared());
    }
    void UnlockShared() const override
    {
        META_EXT_CALL_BASE(ILockable, UnlockShared());
    }
    void Lock() const override
    {
        META_EXT_CALL_BASE(ILockable, Lock());
    }
    void Unlock() const override
    {
        META_EXT_CALL_BASE(ILockable, Unlock());
    }

protected: // IIterable
    IterationResult Iterate(const IterationParameters& params) override
    {
        return META_EXT_CALL_BASE(IIterable, Iterate(params));
    }
    IterationResult Iterate(const IterationParameters& params) const override
    {
        return META_EXT_CALL_BASE(IIterable, Iterate(params));
    }

private:
    IContainer* container_ {};
};

META_END_NAMESPACE()

#endif
