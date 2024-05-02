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

#include "track_animation.h"

#include <meta/ext/serialization/serializer.h>

META_BEGIN_NAMESPACE()

namespace Internal {

AnimationState::AnimationStateParams TrackAnimation::GetParams()
{
    AnimationState::AnimationStateParams params;
    params.owner = GetSelf<IAnimation>();
    params.runningProperty = META_ACCESS_PROPERTY(Running);
    params.progressProperty = META_ACCESS_PROPERTY(Progress);
    params.totalDuration = META_ACCESS_PROPERTY(TotalDuration);
    return params;
}

bool TrackAnimation::Build(const IMetadata::Ptr& data)
{
    if (Super::Build(data)) {
        TrackAnimationState::TrackDataParams params { META_ACCESS_PROPERTY(Timestamps) };
        GetState().SetTrackDataParams(BASE_NS::move(params));

        auto updateKf = MakeCallback<IOnChanged>(this, &TrackAnimation::UpdateKeyframes);

        META_ACCESS_PROPERTY(Timestamps)->OnChanged()->AddHandler(updateKf);
        constexpr BASE_NS::string_view name = "Keyframes";
        keyframes_ = GetObjectRegistry().GetPropertyRegister().Create(ClassId::StackProperty, name);
        if (keyframes_) {
            keyframes_->OnChanged()->AddHandler(updateKf);
            AddProperty(keyframes_);
        }
        UpdateKeyframes();
        return keyframes_ != nullptr;
    }
    return false;
}

void TrackAnimation::Initialize()
{
    ResetTrack();
}

void TrackAnimation::OnAnimationStateChanged(const IAnimationInternal::AnimationStateChangedInfo& info)
{
    using AnimationTargetState = IAnimationInternal::AnimationTargetState;
    if (auto p = GetTargetProperty()) {
        switch (info.state) {
            case AnimationTargetState::FINISHED:
                [[fallthrough]];
            case AnimationTargetState::STOPPED:
                // Evaluate current value
                Evaluate();
                // Remove ourselves from the target property's stack
                RemoveModifier(p.stack);
                // Then set the correct keyframe value to the underlying property
                if (auto value = GetState().GetCurrentValue()) {
                    p.property->SetValue(*value);
                }
                break;
            case AnimationTargetState::RUNNING:
                // Evaluate current value
                Evaluate();
                // Add ourselves to the target property's stack
                p.stack->AddModifier(GetSelf<IModifier>());
                break;
            default:
                break;
        }
    }
}

EvaluationResult TrackAnimation::ProcessOnGet(IAny& value)
{
    if (auto& currentValue = GetState().GetCurrentValue()) {
        if (auto result = value.CopyFrom(*currentValue)) {
            return result == AnyReturn::NOTHING_TO_DO ? EvaluationResult::EVAL_CONTINUE
                                                      : EvaluationResult::EVAL_VALUE_CHANGED;
        }
    }
    return EvaluationResult::EVAL_CONTINUE;
}

void TrackAnimation::Start()
{
    GetState().Start();
}

void TrackAnimation::Stop()
{
    GetState().Stop();
}

void TrackAnimation::Pause()
{
    GetState().Pause();
}

void TrackAnimation::Restart()
{
    GetState().Restart();
}

void TrackAnimation::Finish()
{
    GetState().Finish();
}

void TrackAnimation::Seek(float position)
{
    GetState().Seek(position);
}

void TrackAnimation::Step(const IClock::ConstPtr& clock)
{
    GetState().Step(clock);
}

void TrackAnimation::Evaluate()
{
    float progress = META_ACCESS_PROPERTY_VALUE(Progress);
    if (auto curve = META_ACCESS_PROPERTY_VALUE(Curve)) {
        progress = curve->Transform(progress);
    }
    const auto trackState = GetState().UpdateIndex(progress);
    const PropertyAnimationState::EvaluationData data { GetState().GetCurrentValue(), GetState().GetCurrentTrackStart(),
        GetState().GetCurrentTrackEnd(), trackState.second,
        META_ACCESS_PROPERTY(KeyframeCurves)->GetValueAt(trackState.first) };
    const auto status = GetState().EvaluateValue(data);
    UpdateCurrentTrack(trackState.first);
    if (status == AnyReturn::SUCCESS) {
        NotifyChanged();
    }
}

void TrackAnimation::RemoveModifier(const IStackProperty::Ptr& stack)
{
    if (stack) {
        stack->RemoveModifier(GetSelf<IModifier>());
    }
}

void TrackAnimation::OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous)
{
    if (previous && GetState().IsRunning()) {
        // Property changed while running, clean up previous property's stack
        RemoveModifier(previous);
        if (auto p = interface_cast<IProperty>(previous)) {
            p->SetValue(*GetState().GetCurrentValue());
        }
    }

    Initialize();

    if (auto p = GetTargetProperty()) {
        auto& property = p.property;
        auto& value = property->GetValue();
        bool alreadyCompatible = value.GetTypeId() == GetState().GetKeyframeItemTypeId();
        if (!alreadyCompatible) {
            IAny::Ptr array {};
            if (!value.IsArray()) {
                // Clone the target property's value to an array of the value's underlying type
                array = value.Clone(AnyCloneOptions { CloneValueType::DEFAULT_VALUE, TypeIdRole::ARRAY });
            } else {
                CORE_LOG_E("TrackAnimation: Cannot animate array types");
            }
            if (!array) {
                CORE_LOG_E("TrackAnimation: Failed to create an array of target property type");
            }
            if (auto kf = interface_pointer_cast<IArrayAny>(array)) {
                keyframes_->SetValue(*kf);
            } else {
                keyframes_->ResetValue();
            }
        }
    }

    UpdateValid();
}

size_t TrackAnimation::AddKeyframe(float timestamp, const IAny::ConstPtr& value)
{
    auto index = GetState().AddKeyframe(timestamp, value);
    if (index != ITrackAnimation::INVALID_INDEX) {
        keyframes_->NotifyChange();
    }
    return index;
}

bool TrackAnimation::RemoveKeyframe(size_t index)
{
    bool success = false;
    if (GetState().RemoveKeyframe(index)) {
        keyframes_->NotifyChange();
        success = true;
    } else {
        CORE_LOG_E("TrackAnimation: Cannot remove keyframe from index %u.", static_cast<uint32_t>(index));
    }
    return success;
}

void TrackAnimation::RemoveAllKeyframes()
{
    Stop();
    META_ACCESS_PROPERTY(Timestamps)->Reset();
    keyframes_->ResetValue();
    UpdateValid();
}

void TrackAnimation::UpdateKeyframes()
{
    IArrayAny::Ptr kfArray;
    if (auto stack = interface_cast<IStackProperty>(keyframes_)) {
        // Get the topmost IArrayAny value, that should be our keyframe array
        auto values = stack->GetValues({}, true);
        for (auto it = values.rbegin(); it != values.rend(); ++it) {
            if (auto arr = interface_pointer_cast<IArrayAny>(*it)) {
                kfArray = arr;
                break;
            }
        }
    }
    GetState().SetKeyframes(kfArray);
    UpdateValid();
}

void TrackAnimation::UpdateValid()
{
    bool valid = false;
    auto& timestamps = META_ACCESS_PROPERTY(Timestamps);

    if (const auto p = GetTargetProperty(); p && timestamps) {
        valid = GetState().UpdateValid();
    }

    if (valid != META_ACCESS_PROPERTY_VALUE(Valid)) {
        if (!valid) {
            Stop();
            ResetTrack();
        }
        SetValue(META_ACCESS_PROPERTY(Valid), valid);
    }

    GetState().ResetCurrentTrack();
}

void TrackAnimation::ResetTrack()
{
    GetState().ResetCurrentTrack();
    SetValue(META_ACCESS_PROPERTY(CurrentKeyframeIndex), -1);
}

void TrackAnimation::UpdateCurrentTrack(uint32_t index)
{
    auto currentIndex = META_ACCESS_PROPERTY_VALUE(CurrentKeyframeIndex);
    if (currentIndex != index) {
        SetValue(META_ACCESS_PROPERTY(CurrentKeyframeIndex), index);
        if (auto f = META_ACCESS_PROPERTY(KeyframeHandlers)->GetValueAt(index)) {
            CallMetaFunction<void>(f);
        }
    }
}

ReturnError TrackAnimation::Export(IExportContext& c) const
{
    return Serializer(c) & AutoSerialize() & NamedValue("Keyframes", keyframes_);
}
ReturnError TrackAnimation::Import(IImportContext& c)
{
    return Serializer(c) & AutoSerialize() & NamedValue("Keyframes", keyframes_);
}
ReturnError TrackAnimation::Finalize(IImportFunctions& f)
{
    auto res = Super::Finalize(f);
    if (res) {
        UpdateKeyframes();
    }
    return res;
}
} // namespace Internal

META_END_NAMESPACE()
