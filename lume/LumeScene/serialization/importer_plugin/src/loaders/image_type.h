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

#ifndef SCENE_IMP_SRC_LOADERS_BITMAP_TYPE_H
#define SCENE_IMP_SRC_LOADERS_BITMAP_TYPE_H

#include "resource_type_base.h"
#include "resource_types.h"

SCENE_IMP_BEGIN_NAMESPACE()

class ImageResourceType : public META_NS::IntroduceInterfaces<ResourceTypeBase> {
    META_OBJECT(ImageResourceType, ClassId::ImageLoader, IntroduceInterfaces)
public:
    ImageResourceType() : Super(SCENE_NS::ClassId::ImageResource)
    {}

    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr&) const override;
};

SCENE_IMP_END_NAMESPACE()

#endif
