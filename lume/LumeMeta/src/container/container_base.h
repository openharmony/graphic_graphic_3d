/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Common Object container implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-06-02
 */
#ifndef META_SRC_CONTAINER_CONTAINER_BASE_H
#define META_SRC_CONTAINER_CONTAINER_BASE_H

#include <shared_mutex>

#include <meta/base/namespace.h>
#include <meta/ext/event_impl.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_lockable.h>
#include <meta/interface/intf_required_interfaces.h>

#include "../object.h"

META_BEGIN_NAMESPACE()

META_REGISTER_CLASS(ContainerBase, "28ff5f18-2bf0-45c7-b82f-830e94d15cc9", ObjectCategory::NO_CATEGORY)

class ContainerBase
    : public IntroduceInterfaces<IContainer, IContainerProxyParent, IRequiredInterfaces, ILockable, IIterable> {
public:
    void SetImplementingIContainer(IObject*, IContainer*);

public: // IContainer
    using typename IContainer::SizeType;

    IObject::Ptr FindAnyImpl(const META_NS::IContainer::FindOptions& options, bool isFlat) const;
    BASE_NS::vector<IObject::Ptr> FindAllImpl(const META_NS::IContainer::FindOptions& options, bool isFlat) const;

    BASE_NS::vector<IObject::Ptr> GetAll() const override;
    IObject::Ptr GetAt(SizeType index) const override;
    SizeType GetSize() const override;
    IObject::Ptr FindByName(BASE_NS::string_view name) const override;
    bool Remove(SizeType index) override;
    bool Remove(const META_NS::IObject::Ptr& child) override;
    bool Move(SizeType fromIndex, SizeType toIndex) override;
    bool Move(const IObject::Ptr& child, SizeType toIndex) override;
    bool IsAncestorOf(const IObject::ConstPtr& object) const override;

    void RemoveAll() override;

    BASE_NS::shared_ptr<IEvent> EventOnContainerChanged(META_NS::MetadataQuery) const override;

    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) override;
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const override;

public: // ILockable
    void LockShared() const override;
    void UnlockShared() const override;
    void Lock() const override;
    void Unlock() const override;

public: // IContainerProxyParent
    bool SetProxyParent(const IContainer::Ptr& parent) override;

public: // IIterable
    IterationResult Iterate(const IterationParameters& params) override;
    IterationResult Iterate(const IterationParameters& params) const override;

protected:
    virtual void SetObjectParent(const IObject::Ptr& object, const IObject::Ptr& parent) const = 0;
    ChildChangedInfo MoveInternal(SizeType fromIndex, SizeType toIndex);
    bool IsCompatible(const IObject::Ptr& object) const;
    bool MatchCriteria(const META_NS::IContainer::FindOptions& options, const IObject::Ptr& object) const;
    void InternalRemoveAll();

protected:
    mutable std::shared_mutex mutex_;
    IObject* me_ {};
    IContainer* impl_ {};
    IContainer::WeakPtr parent_; // Proxy parent (or me_ if not set)
    BASE_NS::vector<IObject::Ptr> children_;
    BASE_NS::vector<TypeId> required_;
};

META_END_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
