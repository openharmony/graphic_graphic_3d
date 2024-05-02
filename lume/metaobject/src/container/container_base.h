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
#ifndef META_SRC_CONTAINER_CONTAINER_BASE_H
#define META_SRC_CONTAINER_CONTAINER_BASE_H

#include <shared_mutex>

#include <meta/ext/event_impl.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_lockable.h>
#include <meta/interface/intf_required_interfaces.h>

#include "../object.h"

META_BEGIN_NAMESPACE()

META_REGISTER_CLASS(ContainerBase, "28ff5f18-2bf0-45c7-b82f-830e94d15cc9", ObjectCategory::NO_CATEGORY);

class ContainerBase : public IntroduceInterfaces<IContainer, IContainerProxyParent, IRequiredInterfaces, ILockable,
                          IContainerPreTransaction, IIterable> {
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

    BASE_NS::shared_ptr<IEvent> EventOnAdded() const override;
    BASE_NS::shared_ptr<IEvent> EventOnRemoved() const override;
    BASE_NS::shared_ptr<IEvent> EventOnMoved() const override;

    BASE_NS::shared_ptr<IEvent> EventOnAdding() const override;
    BASE_NS::shared_ptr<IEvent> EventOnRemoving() const override;

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
    void InternalRemoveAll();
    virtual void SetObjectParent(const IObject::Ptr& object, const IObject::Ptr& parent) const = 0;
    ChildMovedInfo MoveInternal(SizeType fromIndex, SizeType toIndex);
    bool IsCompatible(const IObject::Ptr& object) const;
    bool MatchCriteria(const META_NS::IContainer::FindOptions& options, const IObject::Ptr& object) const;

protected:
    mutable std::shared_mutex mutex_;
    IObject* me_;
    IContainer* impl_ {};
    IContainerPreTransaction* implPreTrans_ {};
    IContainer::WeakPtr parent_; // Proxy parent (or me_ if not set)
    BASE_NS::vector<IObject::Ptr> children_;
    BASE_NS::vector<TypeId> required_;
};

META_END_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
