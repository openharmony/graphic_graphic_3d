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

#ifndef META_SRC_RESOURCE_ANIMATION_RESOURCE_H
#define META_SRC_RESOURCE_ANIMATION_RESOURCE_H

#include <core/resources/intf_resource_manager.h>

#include <meta/api/resource/resource_template_access.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/serialization/intf_refuri_builder.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class AnimationResourceType : public IntroduceInterfaces<BaseObject, CORE_NS::IResourceType, IRefUriBuilderAnchorType> {
    META_OBJECT(AnimationResourceType, ClassId::AnimationResourceType, IntroduceInterfaces)
public:
    BASE_NS::string GetResourceName() const override;
    BASE_NS::Uid GetResourceType() const override;
    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;

    void AddAnchorType(const META_NS::ObjectId& id) override
    {
        anchorTypes_.push_back(id);
    }

private:
    BASE_NS::vector<META_NS::ObjectId> anchorTypes_;
};

class PropertyAnimationTemplateAccess : public IntroduceInterfaces<ResourceTemplateAccess, BaseObject> {
    META_OBJECT(PropertyAnimationTemplateAccess, ClassId::PropertyAnimationTemplateAccess, IntroduceInterfaces)
public:
    PropertyAnimationTemplateAccess() : Super(ClassId::PropertyAnimation, ClassId::AnimationResourceTemplate) {}
};

class SequentialAnimationTemplateAccess : public IntroduceInterfaces<ResourceTemplateAccess, BaseObject> {
    META_OBJECT(SequentialAnimationTemplateAccess, ClassId::SequentialAnimationTemplateAccess, IntroduceInterfaces)
public:
    SequentialAnimationTemplateAccess() : Super(ClassId::SequentialAnimation, ClassId::AnimationResourceTemplate) {}
};

class ParallelAnimationTemplateAccess : public IntroduceInterfaces<ResourceTemplateAccess, BaseObject> {
    META_OBJECT(ParallelAnimationTemplateAccess, ClassId::ParallelAnimationTemplateAccess, IntroduceInterfaces)
public:
    ParallelAnimationTemplateAccess() : Super(ClassId::ParallelAnimation, ClassId::AnimationResourceTemplate) {}
};

class KeyframeAnimationTemplateAccess : public IntroduceInterfaces<ResourceTemplateAccess, BaseObject> {
    META_OBJECT(KeyframeAnimationTemplateAccess, ClassId::KeyframeAnimationTemplateAccess, IntroduceInterfaces)
public:
    KeyframeAnimationTemplateAccess() : Super(ClassId::KeyframeAnimation, ClassId::AnimationResourceTemplate) {}
};

class TrackAnimationTemplateAccess : public IntroduceInterfaces<ResourceTemplateAccess, BaseObject> {
    META_OBJECT(TrackAnimationTemplateAccess, ClassId::TrackAnimationTemplateAccess, IntroduceInterfaces)
public:
    TrackAnimationTemplateAccess() : Super(ClassId::TrackAnimation, ClassId::AnimationResourceTemplate) {}

    CORE_NS::IResource::Ptr CreateEmptyTemplate() const override
    {
        auto res = Super::CreateEmptyTemplate();
        if (auto md = interface_cast<IMetadata>(res)) {
            if (auto keyframes =
                    GetObjectRegistry().GetPropertyRegister().Create(ClassId::StackProperty, "Keyframes")) {
                md->AddProperty(keyframes);
            }
        }
        return res;
    }

    CORE_NS::IResource::Ptr CreateTemplateFromResource(const CORE_NS::IResource::ConstPtr& resource) const override
    {
        auto res = Super::CreateTemplateFromResource(resource);
        if (auto md = interface_cast<IMetadata>(res)) {
            if (auto track = interface_cast<ITrackAnimation>(resource)) {
                if (auto keyframes = track->Keyframes()) {
                    CloneToDefault(keyframes, *md);
                }
            }
        }
        return res;
    }

    bool SetValuesFromTemplate(
        const CORE_NS::IResource::ConstPtr& templ, const CORE_NS::IResource::Ptr& resource) const override
    {
        auto res = Super::SetValuesFromTemplate(templ, resource);
        if (res) {
            if (auto md = interface_cast<IMetadata>(templ)) {
                if (auto p = md->GetProperty("Keyframes", MetadataQuery::EXISTING)) {
                    if (auto track = interface_cast<ITrackAnimation>(resource)) {
                        if (auto keyframes = track->Keyframes()) {
                            CopyValue(p, *keyframes);
                        }
                    }
                }
            }
        }
        return res;
    }
};

META_END_NAMESPACE()

#endif
