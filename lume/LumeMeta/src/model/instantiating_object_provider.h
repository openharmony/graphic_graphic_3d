/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Container Data Model Implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-05-25
 */

#ifndef META_SRC_MODEL_INSTANTIATING_OBJECT_PROVIDER_H
#define META_SRC_MODEL_INSTANTIATING_OBJECT_PROVIDER_H

#include "object_provider_base.h"

META_BEGIN_NAMESPACE()

class InstantiatingObjectProvider : public IntroduceInterfaces<ObjectProviderBase, IInstantiatingObjectProvider> {
    META_OBJECT(InstantiatingObjectProvider, ClassId::InstantiatingObjectProvider, IntroduceInterfaces)
public:
    bool SetObjectClassId(const ObjectId& id) override;

protected:
    IObject::Ptr Construct(const IMetadata::Ptr& data) override;

private:
    ObjectId id_;
};

META_END_NAMESPACE()

#endif
