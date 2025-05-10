/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Object container implementation
 * Author: Lauri Jaaskela
 * Create: 2023-01-18
 */
#ifndef META_SRC_CONTAINER_H
#define META_SRC_CONTAINER_H

#include <meta/base/namespace.h>

#include "container_base.h"

META_BEGIN_INTERNAL_NAMESPACE()

class Container : public ContainerBase {
public: // IContainer
    IObject::Ptr FindAny(const META_NS::IContainer::FindOptions& options) const override;
    BASE_NS::vector<IObject::Ptr> FindAll(const META_NS::IContainer::FindOptions& options) const override;
    bool Add(const META_NS::IObject::Ptr& object) override;
    bool Insert(SizeType index, const IObject::Ptr& object) override;
    bool Replace(const META_NS::IObject::Ptr& child, const META_NS::IObject::Ptr& replaceWith, bool addAlways) override;

private:
    bool CheckLoop(const IObject::Ptr& object) const;
    void SetObjectParent(const IObject::Ptr& object, const IObject::Ptr& parent) const override;
};

META_END_INTERNAL_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
