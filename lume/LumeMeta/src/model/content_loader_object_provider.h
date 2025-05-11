/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Container Data Model Implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-05-25
 */

#ifndef META_SRC_MODEL_CONTENT_LOADER_OBJECT_PROVIDER_H
#define META_SRC_MODEL_CONTENT_LOADER_OBJECT_PROVIDER_H

#include "object_provider_base.h"

META_BEGIN_NAMESPACE()

class ContentLoaderObjectProvider : public IntroduceInterfaces<ObjectProviderBase, IContentLoaderObjectProvider> {
    META_OBJECT(ContentLoaderObjectProvider, ClassId::ContentLoaderObjectProvider, IntroduceInterfaces)
public:
    bool SetContentLoader(const IContentLoader::Ptr& loader) override;

protected:
    IObject::Ptr Construct(const IMetadata::Ptr& data) override;

private:
    IContentLoader::Ptr loader_;
};

META_END_NAMESPACE()

#endif
