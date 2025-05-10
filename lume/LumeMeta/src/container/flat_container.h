/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Another Object container implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-06-02
 */
#ifndef META_SRC_FLAT_CONTAINER_H
#define META_SRC_FLAT_CONTAINER_H

#include <meta/base/namespace.h>

#include "container_base.h"

META_BEGIN_NAMESPACE()

class FlatContainer : public ContainerBase {
public: // IContainer
    IObject::Ptr FindAny(const META_NS::IContainer::FindOptions& options) const override;
    BASE_NS::vector<IObject::Ptr> FindAll(const META_NS::IContainer::FindOptions& options) const override;
    bool Add(const META_NS::IObject::Ptr& object) override;
    bool Insert(SizeType index, const IObject::Ptr& object) override;
    bool Replace(const META_NS::IObject::Ptr& child, const META_NS::IObject::Ptr& replaceWith, bool addAlways) override;

private:
    void SetObjectParent(const IObject::Ptr& object, const IObject::Ptr& parent) const override;

public:
    BASE_NS::shared_ptr<IEvent> EventOnContainerChanged(MetadataQuery) const override;

protected:
    mutable BASE_NS::shared_ptr<IEvent> onChanged_;
};

META_END_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
