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

#ifndef SCENE_SRC_TEMPLATE_ANIMATION_TEMPLATE_H
#define SCENE_SRC_TEMPLATE_ANIMATION_TEMPLATE_H

#include <scene/interface/resource/types.h>

#include <meta/interface/animation/intf_animation.h>

#include "resource_template_base.h"

SCENE_BEGIN_NAMESPACE()

class AnimationTemplate : public META_NS::IntroduceInterfaces<ResourceTemplateBase> {
    META_OBJECT(AnimationTemplate, ClassId::AnimationTemplate, IntroduceInterfaces)

public:
    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::AnimationResourceTemplate.Id().ToUid();
    }

    META_NS::ObjectId GetTemplateClassId() const override
    {
        return ClassId::AnimationTemplate;
    }

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

    // Creates a live animation from the template metadata and applies all
    // animation-specific properties (duration, loop, speed, keyframes, etc.).
    META_NS::IAnimation::Ptr CreateAnimation() const;

    static void ApplyProperties(const META_NS::IMetadata& meta, META_NS::IAnimation& anim);
    static void ApplyModifiers(const META_NS::IMetadata& meta, META_NS::IAnimation& anim);

protected:
    bool ValidateResourceType(const CORE_NS::IResource& res) const override;

    // Override the apply to handle animation-specific property mappings. Animation does not
    // use the asDefault flag — its property/modifier/container application is identical for the
    // standalone and ApplyOptions paths.
    bool ApplyToTarget(META_NS::IObject& target, bool asDefault) const override;

private:
    static void ApplyDuration(const META_NS::IMetadata& meta, META_NS::IAnimation& anim);
    static void ApplyKeyframes(const META_NS::IMetadata& meta, META_NS::IAnimation& anim);
    static void AttachLoopModifier(const META_NS::IMetadata& meta, META_NS::IAnimation& anim);
    static void AttachSpeedModifier(const META_NS::IMetadata& meta, META_NS::IAnimation& anim);
};

SCENE_END_NAMESPACE()

#endif
