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

#ifndef META_SRC_MODEL_CONTENT_LOADER_OBJECT_PROVIDER_H
#define META_SRC_MODEL_CONTENT_LOADER_OBJECT_PROVIDER_H

#include "object_provider_base.h"

META_BEGIN_NAMESPACE()

class ContentLoaderObjectProvider
    : public Internal::ConcreteBaseFwd<ContentLoaderObjectProvider, ClassId::ContentLoaderObjectProvider,
          ObjectProviderBase, IContentLoaderObjectProvider> {
public:
    bool SetContentLoader(const IContentLoader::Ptr& loader) override;

protected:
    IObject::Ptr Construct(const IMetadata::Ptr& data) override;

private:
    IContentLoader::Ptr loader_;
};

META_END_NAMESPACE()

#endif
