/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Another Object container implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-06-02
 */
#ifndef META_SRC_OBJECT_FLAT_CONTAINER_H
#define META_SRC_OBJECT_FLAT_CONTAINER_H

#include <meta/base/namespace.h>
#include <meta/ext/implementation_macros.h>

#include "flat_container.h"

META_BEGIN_NAMESPACE()

class ObjectFlatContainer : public IntroduceInterfaces<MetaObject, FlatContainer> {
    META_OBJECT(ObjectFlatContainer, ClassId::ObjectFlatContainer, IntroduceInterfaces)

protected:
    bool Build(const IMetadata::Ptr& data) override;
    void Destroy() override;

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_EVENT_DATA(IContainer, IOnChildChanged, OnContainerChanged)
    META_END_STATIC_DATA()
    META_IMPLEMENT_EVENT(IOnChildChanged, OnContainerChanged)
};

META_END_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
