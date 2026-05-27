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

#ifndef SCENE_API_TEMPLATE_CONTAINER_ANIMATION_TEMPLATE_H
#define SCENE_API_TEMPLATE_CONTAINER_ANIMATION_TEMPLATE_H

#include <scene/api/template/animation_template_base.h>

SCENE_BEGIN_NAMESPACE()

class ContainerAnimationTemplate : public AnimationTemplateBase {
public:
    META_INTERFACE_OBJECT(ContainerAnimationTemplate, AnimationTemplateBase, IResourceTemplate)

    auto Animations() const
    {
        if (auto meta = interface_cast<META_NS::IMetadata>(GetPtr().get())) {
            return meta->GetArrayProperty<META_NS::IObject::Ptr>("Animations", META_NS::MetadataQuery::EXISTING);
        }
        return META_NS::ArrayProperty<META_NS::IObject::Ptr>{};
    }
};

SCENE_END_NAMESPACE()

#endif  // SCENE_API_TEMPLATE_CONTAINER_ANIMATION_TEMPLATE_H
