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

#ifndef SCENE_API_TEMPLATE_ANIMATION_TEMPLATE_BASE_H
#define SCENE_API_TEMPLATE_ANIMATION_TEMPLATE_BASE_H

#include <scene/api/resource.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>

#include <meta/api/interface_object.h>
#include <meta/api/object.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>

SCENE_BEGIN_NAMESPACE()

class AnimationTemplateBase : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(AnimationTemplateBase, META_NS::Object, IResourceTemplate)
    META_INTERFACE_OBJECT_INSTANTIATE(AnimationTemplateBase, ::SCENE_NS::ClassId::AnimationTemplate)

    bool ApplyTo(META_NS::IAnimation& target) const
    {
        if (auto obj = interface_cast<META_NS::IObject>(&target)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.ApplyTo(*obj); });
        }
        return false;
    }
    bool CopyFrom(const META_NS::IAnimation& source, bool onlySetValues = true)
    {
        if (auto obj = interface_cast<const META_NS::IObject>(&source)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.CopyFrom(*obj, onlySetValues); });
        }
        return false;
    }

    template <typename Type>
    auto GetProperty(BASE_NS::string_view name) const
    {
        auto meta = META_NS::Metadata(*this);
        return meta.GetProperty<Type>(name);
    }

    auto AnimationType() const
    {
        return GetProperty<BASE_NS::string>("AnimationType");
    }
    auto Name() const
    {
        return GetProperty<BASE_NS::string>("Name");
    }
    auto Enabled() const
    {
        return GetProperty<bool>("Enabled");
    }
    auto LoopCount() const
    {
        return GetProperty<int32_t>("LoopCount");
    }
    auto SpeedFactor() const
    {
        return GetProperty<float>("SpeedFactor");
    }

    META_NS::IAnimation::Ptr CreateInstance() const
    {
        if (!GetPtr()) {
            return nullptr;
        }
        const auto typeName = META_NS::GetValue(AnimationType());
        META_NS::ClassInfo classInfo;
        if (typeName == "trackAnimation") {
            classInfo = META_NS::ClassId::TrackAnimation;
        } else if (typeName == "keyframeAnimation") {
            classInfo = META_NS::ClassId::KeyframeAnimation;
        } else if (typeName == "sequentialAnimation") {
            classInfo = META_NS::ClassId::SequentialAnimation;
        } else if (typeName == "parallelAnimation") {
            classInfo = META_NS::ClassId::ParallelAnimation;
        } else {
            return nullptr;
        }
        auto anim = META_NS::GetObjectRegistry().Create<META_NS::IAnimation>(classInfo);
        if (!anim || !ApplyTo(*anim)) {
            return nullptr;
        }
        return anim;
    }
};

SCENE_END_NAMESPACE()

#endif  // SCENE_API_TEMPLATE_ANIMATION_TEMPLATE_BASE_H
