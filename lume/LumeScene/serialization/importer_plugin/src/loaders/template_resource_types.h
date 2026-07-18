/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_LOADERS_TEMPLATE_RESOURCE_TYPES_H
#define SCENE_IMP_SRC_LOADERS_TEMPLATE_RESOURCE_TYPES_H

#include "resource_type_base.h"
#include "resource_types.h"

SCENE_IMP_BEGIN_NAMESPACE()

class AnimationTemplateResourceType : public META_NS::IntroduceInterfaces<SceneResourceTemplateTypeBase> {
    META_OBJECT(AnimationTemplateResourceType, ClassId::AnimationTemplateLoader, IntroduceInterfaces)
public:
    AnimationTemplateResourceType() : Super(SCENE_NS::ClassId::AnimationResourceTemplate)
    {}
};

class EnvironmentTemplateResourceType : public META_NS::IntroduceInterfaces<SceneResourceTemplateTypeBase> {
    META_OBJECT(EnvironmentTemplateResourceType, ClassId::EnvironmentTemplateLoader, IntroduceInterfaces)
public:
    EnvironmentTemplateResourceType() : Super(SCENE_NS::ClassId::EnvironmentResourceTemplate)
    {}
};

class MaterialTemplateResourceType : public META_NS::IntroduceInterfaces<SceneResourceTemplateTypeBase> {
    META_OBJECT(MaterialTemplateResourceType, ClassId::MaterialTemplateLoader, IntroduceInterfaces)
public:
    MaterialTemplateResourceType() : Super(SCENE_NS::ClassId::MaterialResourceTemplate)
    {}
};

class OcclusionMaterialTemplateResourceType : public META_NS::IntroduceInterfaces<SceneResourceTemplateTypeBase> {
    META_OBJECT(OcclusionMaterialTemplateResourceType, ClassId::OcclusionMaterialTemplateLoader, IntroduceInterfaces)
public:
    OcclusionMaterialTemplateResourceType() : Super(SCENE_NS::ClassId::OcclusionMaterialResourceTemplate)
    {}
};

class PostProcessTemplateResourceType : public META_NS::IntroduceInterfaces<SceneResourceTemplateTypeBase> {
    META_OBJECT(PostProcessTemplateResourceType, ClassId::PostProcessTemplateLoader, IntroduceInterfaces)
public:
    PostProcessTemplateResourceType() : Super(SCENE_NS::ClassId::PostProcessResourceTemplate)
    {}
};

SCENE_IMP_END_NAMESPACE()

#endif
