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

#include <scene/ext/util.h>

#include <base/math/mathf.h>

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/animation/modifiers/intf_loop.h>
#include <meta/interface/animation/modifiers/intf_speed.h>

#include "../converting_value.h"

SCENE_BEGIN_NAMESPACE()

using META_NS::GetValue;
using META_NS::ResetValue;
using META_NS::SetValue;

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

void EcsAnimation::Destroy()
{
    timeChanged_.Unsubscribe();
    speedChanged_.Unsubscribe();
    repeatCountChanged_.Unsubscribe();
    durationChanged_.Unsubscribe();
    completedChanged_.Unsubscribe();
    Super::Destroy();
}

bool EcsAnimation::SetEcsObject(const IEcsObject::Ptr& obj)
{
    auto p = META_NS::GetObjectRegistry().Create<IInternalAnimation>(ClassId::AnimationComponent);
    if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
        if (acc->SetEcsObject(obj)) {
            animation_ = p;
            props_ = TargetProperties(p);
            Init();
        }
    }
    return animation_ != nullptr;
}
void EcsAnimation::Init()
{
    SetValue(META_ACCESS_PROPERTY(Valid), true);
    SetValue(props_.repeatCount, 0);
    auto updateProgress = META_NS::MakeCallbackSafe<META_NS::IOnChanged>(
        [this](const auto& self) {
            if (self) {
                UpdateProgressFromEcs();
            }
        },
        GetSelf());
    auto updateTotalDuration = META_NS::MakeCallbackSafe<META_NS::IOnChanged>(
        [this](const auto& self) {
            if (self) {
                UpdateTotalDuration();
            }
        },
        GetSelf());
    auto updateCompletion = META_NS::MakeCallbackSafe<META_NS::IOnChanged>(
        [this](const auto& self) {
            if (self) {
                UpdateCompletion();
            }
        },
        GetSelf());
    timeChanged_.Subscribe(props_.time, updateProgress);
    speedChanged_.Subscribe(props_.speed, updateTotalDuration);
    repeatCountChanged_.Subscribe(props_.repeatCount, updateTotalDuration);
    durationChanged_.Subscribe(props_.duration, updateTotalDuration);
    completedChanged_.Subscribe(props_.completed, updateCompletion);

    Enabled()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this] {
        if (!META_ACCESS_PROPERTY_VALUE(Enabled)) {
            InternalStop();
        }
    }));
    UpdateTotalDuration();
}
IEcsObject::Ptr EcsAnimation::GetEcsObject() const
{
    auto acc = interface_cast<IEcsObjectAccess>(animation_);
    return acc ? acc->GetEcsObject() : nullptr;
}
BASE_NS::string EcsAnimation::GetName() const
{
    return META_ACCESS_PROPERTY_VALUE(Name);
}
void EcsAnimation::UpdateCompletion()
{
    if (META_ACCESS_PROPERTY_VALUE(Running) && GetValue(props_.completed)) {
        SetValue(META_ACCESS_PROPERTY(Running), false);
        META_NS::Invoke<META_NS::IOnChanged>(EventOnFinished(META_NS::MetadataQuery::EXISTING));
    }
}
void EcsAnimation::UpdateProgressFromEcs()
{
    auto progress = 0.f;
    const auto duration = GetValue(props_.duration);
    if (duration != 0.f) {
        progress = GetValue(props_.time) / duration;
    }
    if (GetValue(props_.speed) < 0.f) {
        progress = 1.f - progress;
    }
    SetValue(META_ACCESS_PROPERTY(Progress), progress);
}

void EcsAnimation::UpdateTotalDuration()
{
    auto speed = GetValue(props_.speed);
    auto dur = GetValue(props_.duration);
    uint32_t rcount = GetValue(props_.repeatCount);
    META_NS::TimeSpan newDur = META_NS::TimeSpan::Infinite();
    if (rcount != CORE3D_NS::AnimationComponent::REPEAT_COUNT_INFINITE && speed != 0.f) {
        float times = 1.0f + rcount;
        newDur = META_NS::TimeSpan::Seconds(BASE_NS::Math::abs(dur * times / speed));
    }
    // We only update Duration from ECS whenever total duration gets updated. Any changes made by the user to Duration
    // are not synced to ECS nor have any effect.
    SetValue(META_ACCESS_PROPERTY(Duration), META_NS::TimeSpan::Seconds(dur));
    SetValue(META_ACCESS_PROPERTY(TotalDuration), newDur);
}
void EcsAnimation::InternalStop()
{
    SetValue(props_.state, CORE3D_NS::AnimationComponent::PlaybackState::STOP);
    SetValue(props_.currentLoop, 0);
    SetValue(META_ACCESS_PROPERTY(Running), false);
    SetProgress(0.0f);
}
void EcsAnimation::SetProgress(float progress)
{
    progress = Base::Math::clamp01(progress);
    if (animation_) {
        // ECS component expects the time to be set without any modifiers
        auto duration = GetValue(props_.duration);
        if (GetValue(props_.speed) < 0.0) {
            progress = 1.f - progress;
        }
        // Our OnChanged handler for animation_->Time() will update Progress property
        SetValue(props_.time, progress * duration);
    } else {
        SetValue(META_ACCESS_PROPERTY(Progress), progress);
    }
}
void EcsAnimation::Pause()
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
        SetValue(props_.state, CORE3D_NS::AnimationComponent::PlaybackState::PAUSE);
        SetValue(META_ACCESS_PROPERTY(Running), false);
    }
}
void EcsAnimation::Restart()
{
    if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
        SetProgress(0.0f);
        // Go back to first loop
        SetValue(props_.currentLoop, 0);
        SetValue(props_.state, CORE3D_NS::AnimationComponent::PlaybackState::PLAY);
        SetValue(META_ACCESS_PROPERTY(Running), true);
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
    if (animation_ && META_ACCESS_PROPERTY_VALUE(Enabled) &&
        META_NS::GetValue(animation_->State()) != CORE3D_NS::AnimationComponent::PlaybackState::PLAY &&
        META_NS::GetValue(animation_->Time()) < META_NS::GetValue(animation_->Duration())) {
        animation_->State()->SetValue(CORE3D_NS::AnimationComponent::PlaybackState::PLAY);
    }
    // Handle "Running" state according to expectations.
    if (!META_NS::GetValue(META_ACCESS_PROPERTY(Running))) {
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
        SetValue(props_.state, CORE3D_NS::AnimationComponent::PlaybackState::PAUSE);
        SetValue(META_ACCESS_PROPERTY(Running), false);
        Seek(1.0f);
        META_NS::Invoke<META_NS::IOnChanged>(EventOnFinished(META_NS::MetadataQuery::EXISTING));
    }
}
bool EcsAnimation::Attach(const META_NS::IObject::Ptr& attachment, const META_NS::IObject::Ptr& dataContext)
{
    bool res = Super::Attach(attachment, dataContext);
    if (res) {
        if (auto loop = interface_pointer_cast<META_NS::AnimationModifiers::ILoop>(attachment)) {
            auto loopCountProperty = loop->LoopCount();
            auto value = GetValue(loopCountProperty);
            SetValue(props_.repeatCount, LoopCountConverter::ConvertToTarget(value));
            PushPropertyValue(loopCountProperty,
                META_NS::IValue::Ptr { new ConvertingValue<LoopCountConverter>(props_.repeatCount) });
        } else if (auto speed = interface_pointer_cast<META_NS::AnimationModifiers::ISpeed>(attachment)) {
            auto speedFactorProperty = speed->SpeedFactor();
            auto value = GetValue(speedFactorProperty);
            SetValue(props_.speed, value);
            PushPropertyAsValue(speedFactorProperty, props_.speed);
        }
    }
    return res;
}
bool EcsAnimation::Detach(const META_NS::IObject::Ptr& attachment)
{
    if (auto loop = interface_cast<META_NS::AnimationModifiers::ILoop>(attachment)) {
        ResetValue(loop->LoopCount());
        SetValue(props_.repeatCount, 0);
    } else if (auto speed = interface_cast<META_NS::AnimationModifiers::ISpeed>(attachment)) {
        ResetValue(speed->SpeedFactor());
        SetValue(props_.speed, 1.f);
    }
    return Super::Detach(attachment);
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
