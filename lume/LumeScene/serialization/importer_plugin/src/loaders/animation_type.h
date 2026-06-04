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

#ifndef SCENE_IMP_SRC_LOADERS_ANIMATION_TYPE_H
#define SCENE_IMP_SRC_LOADERS_ANIMATION_TYPE_H

#include <scene/interface/resource/types.h>

#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/intf_metadata.h>

#include "resource_type_base.h"
#include "resource_types.h"

SCENE_IMP_BEGIN_NAMESPACE()

class GltfAnimationResourceType : public META_NS::IntroduceInterfaces<SceneResourceTypeBase> {
    META_OBJECT(GltfAnimationResourceType, ClassId::GltfAnimationLoader, IntroduceInterfaces)
public:
    GltfAnimationResourceType() : Super(SCENE_NS::ClassId::AnimationResource, META_NS::ClassInfo{})
    {}

    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
};

class MetaAnimationResourceType : public META_NS::IntroduceInterfaces<SceneResourceTemplateTypeBase> {
    META_OBJECT(MetaAnimationResourceType, ClassId::MetaAnimationLoader, IntroduceInterfaces)
public:
    MetaAnimationResourceType() : Super(META_NS::ClassId::AnimationResource)
    {}
    CORE_NS::IResource::Ptr LoadResource(const StorageInfo& s) const override;
};

SCENE_IMP_END_NAMESPACE()

#endif
