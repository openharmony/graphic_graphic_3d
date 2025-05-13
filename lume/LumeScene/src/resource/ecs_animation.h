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

#ifndef SCENE_SRC_ECS_ANIMATION_H
#define SCENE_SRC_ECS_ANIMATION_H

#include <scene/interface/resource/types.h>

#include <meta/ext/attachment/attachment.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/animation/intf_animation.h>

#include "../component/animation_component.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(EcsAnimation, "5513d745-958f-4aa6-bab7-7561cebdc3dd", META_NS::ObjectCategoryBits::NO_CATEGORY)

class EcsAnimation : public META_NS::IntroduceInterfaces<META_NS::AttachmentFwd, META_NS::IStartableAnimation,
                         META_NS::IAnimation, META_NS::Resource, IEcsObjectAccess> {
    META_OBJECT(EcsAnimation, SCENE_NS::ClassId::EcsAnimation, IntroduceInterfaces)

public:
    EcsAnimation() = default;
    ~EcsAnimation();

    bool SetEcsObject(const IEcsObject::Ptr&) override;
    IEcsObject::Ptr GetEcsObject() const override;

    BASE_NS::string GetName() const override;

public: // IAnimation
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, bool, Enabled, true)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, bool, Valid, false)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, META_NS::TimeSpan, TotalDuration)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, bool, Running)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, float, Progress)

    META_STATIC_EVENT_DATA(META_NS::IAnimation, META_NS::IOnChanged, OnFinished)
    META_STATIC_EVENT_DATA(META_NS::IAnimation, META_NS::IOnChanged, OnStarted)

    META_STATIC_FORWARDED_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)

    //--- not supported
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, META_NS::ICurve1D::Ptr, Curve)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, BASE_NS::weak_ptr<META_NS::IAnimationController>, Controller)
    //---
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_READONLY_PROPERTY(bool, Valid)
    META_IMPLEMENT_READONLY_PROPERTY(META_NS::TimeSpan, TotalDuration)
    META_IMPLEMENT_READONLY_PROPERTY(bool, Running)
    META_IMPLEMENT_READONLY_PROPERTY(float, Progress)

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnFinished)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnStarted)

    META_FORWARD_PROPERTY(BASE_NS::string, Name, GetNameProperty())

    //--- not supported
    META_IMPLEMENT_PROPERTY(META_NS::ICurve1D::Ptr, Curve)
    META_IMPLEMENT_PROPERTY(BASE_NS::weak_ptr<META_NS::IAnimationController>, Controller)

    //--- not supported
    void Step(const META_NS::IClock::ConstPtr&) override {}
    //---

    META_NS::IProperty::Ptr GetNameProperty() const
    {
        auto i = interface_cast<META_NS::INamed>(GetEcsObject());
        return i ? i->Name() : nullptr;
    }

public: // IStartableAnimation
    void Pause() override;
    void Restart() override;
    void Seek(float position) override;
    void Start() override;
    void Stop() override;
    void Finish() override;

public:
    bool Attach(const META_NS::IObject::Ptr& attachment, const META_NS::IObject::Ptr& dataContext) override;
    bool Detach(const META_NS::IObject::Ptr& attachment) override;
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override;
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override;

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::AnimationResource.Id().ToUid();
    }

private:
    void Init();
    void UpdateCompletion();
    void UpdateProgressFromEcs();
    void UpdateTotalDuration();
    void InternalStop();
    void SetProgress(float);

private:
    IInternalAnimation::Ptr anim_;
};

SCENE_END_NAMESPACE()

#endif