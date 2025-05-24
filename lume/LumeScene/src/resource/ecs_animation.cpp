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

#include "ecs_animation.h"

#include "base/math/mathf.h"

#include <meta/api/make_callback.h>
#include <meta/interface/animation/modifiers/intf_loop.h>
#include <meta/interface/animation/modifiers/intf_speed.h>

#include "converting_value.h"

SCENE_BEGIN_NAMESPACE()

struct LoopCountConverter {
    using SourceType = int32_t;
    using TargetType = uint32_t;
    static SourceType ConvertToSource(META_NS::IAny&, const TargetType& v)
    {
        return v == CORE3D_NS::AnimationComponent::REPEAT_COUNT_INFINITE ? -1 : static_cast<int32_t>(v) + 1;
    }
    static TargetType ConvertToTarget(const SourceType& v)
    {
        // we don't support v == 0 to stop animation for now
        return v <= 0 ? CORE3D_NS::AnimationComponent::REPEAT_COUNT_INFINITE : static_cast<uint32_t>(v) - 1;
    }
};

EcsAnimation::~EcsAnimation()
{
    if (anim_) {
        anim_->Time()->OnChanged()->RemoveHandler(intptr_t(this));
        anim_->Speed()->OnChanged()->RemoveHandler(intptr_t(this));
        anim_->RepeatCount()->OnChanged()->RemoveHandler(intptr_t(this));
        anim_->Duration()->OnChanged()->RemoveHandler(intptr_t(this));
        anim_->Completed()->OnChanged()->RemoveHandler(intptr_t(this));
    }
}

bool EcsAnimation::SetEcsObject(const IEcsObject::Ptr& obj)
{
    auto p = META_NS::GetObjectRegistry().Create<IInternalAnimation>(ClassId::AnimationComponent);
    if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
        if (acc->SetEcsObject(obj)) {
            anim_ = p;
            Init();
        }
    }
    return anim_ != nullptr;
}
void EcsAnimation::Init()
{
    META_ACCESS_PROPERTY(Valid)->SetValue(true);
    anim_->RepeatCount()->SetValue(0);
    anim_->Time()->OnChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChanged>([this] { UpdateProgressFromEcs(); }), intptr_t(this));
    anim_->Speed()->OnChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChanged>([this] { UpdateTotalDuration(); }), intptr_t(this));
    anim_->RepeatCount()->OnChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChanged>([this] { UpdateTotalDuration(); }), intptr_t(this));
    anim_->Duration()->OnChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChanged>([this] { UpdateTotalDuration(); }), intptr_t(this));
    anim_->Completed()->OnChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChanged>([this] { UpdateCompletion(); }), intptr_t(this));

    Enabled()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this] {
        if (!Enabled()->GetValue()) {
            InternalStop();
        }
    }),
        intptr_t(this));

    UpdateTotalDuration();
}
IEcsObject::Ptr EcsAnimation::GetEcsObject() const
{
    auto acc = interface_cast<IEcsObjectAccess>(anim_);
    return acc ? acc->GetEcsObject() : nullptr;
}
BASE_NS::string EcsAnimation::GetName() const
{
    return META_NS::GetValue(Name());
}
void EcsAnimation::UpdateCompletion()
{
    if (Running()->GetValue() && anim_->Completed()->GetValue()) {
        META_ACCESS_PROPERTY(Running)->SetValue(false);
        META_NS::Invoke<META_NS::IOnChanged>(EventOnFinished(META_NS::MetadataQuery::EXISTING));
    }
}
void EcsAnimation::UpdateProgressFromEcs()
{
    auto currentTime = anim_->Time()->GetValue();
    auto totalDuration = TotalDuration()->GetValue().ToSecondsFloat();
    auto progress = totalDuration != 0.f ? currentTime / totalDuration : 1.f;
    META_ACCESS_PROPERTY(Progress)->SetValue(progress);
}
void EcsAnimation::UpdateTotalDuration()
{
    auto speed = anim_->Speed()->GetValue();
    auto dur = anim_->Duration()->GetValue();
    uint32_t rcount = anim_->RepeatCount()->GetValue();
    META_NS::TimeSpan newDur = META_NS::TimeSpan::Infinite();
    if (rcount != CORE3D_NS::AnimationComponent::REPEAT_COUNT_INFINITE && speed != 0.f) {
        float times = 1.0f + rcount;
        newDur = META_NS::TimeSpan::Seconds(BASE_NS::Math::abs(dur * times / speed));
    }
    META_ACCESS_PROPERTY(TotalDuration)->SetValue(newDur);
}
void EcsAnimation::InternalStop()
{
    anim_->State()->SetValue(CORE3D_NS::AnimationComponent::PlaybackState::STOP);
    META_ACCESS_PROPERTY(Running)->SetValue(false);
    SetProgress(0.0f);
}
void EcsAnimation::SetProgress(float progress)
{
    progress = Base::Math::clamp01(progress);
    if (anim_->Speed()->GetValue() < 0.0) {
        anim_->Time()->SetValue((1.0 - progress) * GetValue(TotalDuration()).ToSecondsFloat());
    } else {
        anim_->Time()->SetValue(progress * GetValue(TotalDuration()).ToSecondsFloat());
    }
    META_ACCESS_PROPERTY(Progress)->SetValue(progress);
}
void EcsAnimation::Pause()
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
        anim_->State()->SetValue(CORE3D_NS::AnimationComponent::PlaybackState::PAUSE);
        META_ACCESS_PROPERTY(Running)->SetValue(false);
    }
}
void EcsAnimation::Restart()
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled))
    {
        SetProgress(0.0f);
        anim_->State()->SetValue(CORE3D_NS::AnimationComponent::PlaybackState::PLAY);
        META_ACCESS_PROPERTY(Running)->SetValue(true);
        META_NS::Invoke<META_NS::IOnChanged>(EventOnStarted(META_NS::MetadataQuery::EXISTING));
    }
    
}
void EcsAnimation::Seek(float position)
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
        SetProgress(position);
    }
}
void EcsAnimation::Start()
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
        anim_->State()->SetValue(CORE3D_NS::AnimationComponent::PlaybackState::PLAY);
        META_ACCESS_PROPERTY(Running)->SetValue(true);
        META_NS::Invoke<META_NS::IOnChanged>(EventOnStarted(META_NS::MetadataQuery::EXISTING));
    }
}
void EcsAnimation::Stop()
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
        InternalStop();
    }
}
void EcsAnimation::Finish()
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
        anim_->State()->SetValue(CORE3D_NS::AnimationComponent::PlaybackState::STOP);
        META_ACCESS_PROPERTY(Running)->SetValue(false);
        Seek(1.0f);
        META_NS::Invoke<META_NS::IOnChanged>(EventOnFinished(META_NS::MetadataQuery::EXISTING));
    }
}
bool EcsAnimation::Attach(const META_NS::IObject::Ptr& attachment, const META_NS::IObject::Ptr& dataContext)
{
    bool res = Super::Attach(attachment, dataContext);
    if (res) {
        if (auto loop = interface_pointer_cast<META_NS::AnimationModifiers::ILoop>(attachment)) {
            auto value = loop->LoopCount()->GetValue();
            anim_->RepeatCount()->SetValue(LoopCountConverter::ConvertToTarget(value));
            loop->LoopCount()->PushValue(
                META_NS::IValue::Ptr(new ConvertingValue<LoopCountConverter>(anim_->RepeatCount())));
        } else if (auto speed = interface_pointer_cast<META_NS::AnimationModifiers::ISpeed>(attachment)) {
            auto value = speed->SpeedFactor()->GetValue();
            anim_->Speed()->SetValue(value);
            speed->SpeedFactor()->PushValue(anim_->Speed().GetProperty());
        }
    }
    return res;
}
bool EcsAnimation::Detach(const META_NS::IObject::Ptr& attachment)
{
    bool res = Super::Detach(attachment);
    if (res) {
        if (auto loop = interface_cast<META_NS::AnimationModifiers::ILoop>(attachment)) {
            loop->LoopCount()->ResetValue();
            anim_->RepeatCount()->SetValue(0);
        } else if (auto speed = interface_cast<META_NS::AnimationModifiers::ISpeed>(attachment)) {
            speed->SpeedFactor()->ResetValue();
            anim_->Speed()->SetValue(1.f);
        }
    }
    return res;
}
bool EcsAnimation::AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext)
{
    return true;
}
bool EcsAnimation::DetachFrom(const META_NS::IAttach::Ptr& target)
{
    return true;
}

SCENE_END_NAMESPACE()