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

#include "animation_template.h"

#include <meta/api/util.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/modifiers/intf_loop.h>
#include <meta/interface/animation/modifiers/intf_speed.h>
#include <meta/interface/curves/intf_curve_1d.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/property/intf_stack_property.h>

SCENE_BEGIN_NAMESPACE()

bool AnimationTemplate::ValidateResourceType(const CORE_NS::IResource& res) const
{
    return interface_cast<META_NS::IAnimation>(&res) != nullptr;
}

void AnimationTemplate::ApplyDuration(const META_NS::IMetadata& meta, META_NS::IAnimation& anim)
{
    auto prop = meta.GetProperty<float>("DurationMs");
    if (!prop) {
        return;
    }
    auto durationMs = META_NS::GetValue<float>(&meta, "DurationMs", 0.0f);
    if (auto timed = interface_cast<META_NS::ITimedAnimation>(&anim)) {
        timed->Duration()->SetValue(META_NS::TimeSpan::Milliseconds(static_cast<int64_t>(durationMs)));
    }
}

void AnimationTemplate::AttachLoopModifier(const META_NS::IMetadata& meta, META_NS::IAnimation& anim)
{
    auto prop = meta.GetProperty<int32_t>("LoopCount");
    if (!prop) {
        return;
    }
    auto loopCount = META_NS::GetValue<int32_t>(&meta, "LoopCount", 1);
    if (loopCount == 1) {
        return;
    }
    auto attach = interface_cast<META_NS::IAttach>(&anim);
    if (!attach) {
        return;
    }
    auto existing = attach->GetAttachments<META_NS::AnimationModifiers::ILoop>();
    if (!existing.empty()) {
        existing[0]->LoopCount()->SetValue(loopCount);
        return;
    }
    auto loop = META_NS::GetObjectRegistry().Create<META_NS::AnimationModifiers::ILoop>(
        META_NS::ClassId::LoopAnimationModifier);
    if (loop) {
        loop->LoopCount()->SetValue(loopCount);
        attach->Attach(loop);
    }
}

void AnimationTemplate::AttachSpeedModifier(const META_NS::IMetadata& meta, META_NS::IAnimation& anim)
{
    auto prop = meta.GetProperty<float>("SpeedFactor");
    if (!prop) {
        return;
    }
    auto speedFactor = META_NS::GetValue<float>(&meta, "SpeedFactor", 1.0f);
    if (speedFactor == 1.0f) {
        return;
    }
    auto attach = interface_cast<META_NS::IAttach>(&anim);
    if (!attach) {
        return;
    }
    auto existing = attach->GetAttachments<META_NS::AnimationModifiers::ISpeed>();
    if (!existing.empty()) {
        existing[0]->SpeedFactor()->SetValue(speedFactor);
        return;
    }
    auto speed = META_NS::GetObjectRegistry().Create<META_NS::AnimationModifiers::ISpeed>(
        META_NS::ClassId::SpeedAnimationModifier);
    if (speed) {
        speed->SpeedFactor()->SetValue(speedFactor);
        attach->Attach(speed);
    }
}

static bool TrackHasKeyframeBackingArray(META_NS::ITrackAnimation& track)
{
    auto stack = interface_cast<META_NS::IStackProperty>(track.Keyframes());
    if (!stack) {
        return false;
    }
    for (auto&& v : stack->GetValues({}, true)) {
        if (interface_cast<META_NS::IArrayAny>(v.get())) {
            return true;
        }
    }
    return false;
}

static META_NS::IAny::Ptr FindFirstNonNull(const BASE_NS::vector<META_NS::IAny::Ptr>& values)
{
    for (auto&& v : values) {
        if (v) {
            return v;
        }
    }
    return {};
}

// Normally TrackAnimation::OnPropertyChanged seeds the backing array from the target property's
// type. When the target Property is not bound yet (e.g. when keyframes are inherited from a base
// template before a derived options sets Property), seed it from the first non-null keyframe's
// type instead — the keyframes themselves carry the type. If Property binds later with a matching
// type, OnPropertyChanged's compatibility check keeps these keyframes; with a mismatching type,
// it would have rebuilt the array regardless.
static void EnsureKeyframeBackingArray(
    META_NS::ITrackAnimation& track, const BASE_NS::vector<META_NS::IAny::Ptr>& kfValues)
{
    if (TrackHasKeyframeBackingArray(track)) {
        return;
    }
    auto typed = FindFirstNonNull(kfValues);
    if (!typed) {
        return;
    }
    auto array = typed->Clone({META_NS::CloneValueType::DEFAULT_VALUE, META_NS::TypeIdRole::ARRAY});
    if (auto kfArr = interface_pointer_cast<META_NS::IArrayAny>(array)) {
        track.Keyframes()->SetValue(*kfArr);
    }
}

void AnimationTemplate::ApplyKeyframes(const META_NS::IMetadata& meta, META_NS::IAnimation& anim)
{
    auto track = interface_cast<META_NS::ITrackAnimation>(&anim);
    if (!track) {
        return;
    }
    auto keyframes =
        meta.GetArrayProperty<const META_NS::IAny::Ptr>("KeyframeValues", META_NS::MetadataQuery::EXISTING);
    if (!keyframes) {
        return;
    }
    auto timestamps = meta.GetArrayProperty<const float>("Timestamps", META_NS::MetadataQuery::EXISTING);
    if (!timestamps) {
        return;
    }
    auto kfValues = keyframes->GetValue();
    EnsureKeyframeBackingArray(*track, kfValues);
    // AddKeyframe inserts a timestamp alongside each keyframe; clear any pre-set timestamps so
    // the rebuilt arrays stay paired (otherwise N keyframes + N pre-set timestamps end up as 2N
    // timestamps and Valid stays false).
    if (TrackHasKeyframeBackingArray(*track)) {
        track->Timestamps()->Reset();
    }
    auto tsValues = timestamps->GetValue();
    auto count = BASE_NS::Math::min(kfValues.size(), tsValues.size());
    for (size_t i = 0; i < count; ++i) {
        if (kfValues[i]) {
            track->AddKeyframe(tsValues[i], kfValues[i]);
        }
    }
}

void AnimationTemplate::ApplyProperties(const META_NS::IMetadata& meta, META_NS::IAnimation& anim)
{
    auto name = META_NS::GetValue<BASE_NS::string>(&meta, "Name");
    if (!name.empty()) {
        anim.Name()->SetValue(name);
    }
    anim.Enabled()->SetValue(META_NS::GetValue<bool>(&meta, "Enabled", true));

    ApplyDuration(meta, anim);

    if (auto track = interface_cast<META_NS::ITrackAnimation>(&anim)) {
        auto timestamps = meta.GetArrayProperty<const float>("Timestamps", META_NS::MetadataQuery::EXISTING);
        if (timestamps) {
            track->Timestamps()->SetValue(timestamps->GetValue());
        }
    }

    if (auto propAnim = interface_cast<META_NS::IPropertyAnimation>(&anim)) {
        auto prop = meta.GetProperty<const META_NS::IProperty::WeakPtr>("Property", META_NS::MetadataQuery::EXISTING);
        if (prop) {
            propAnim->Property()->SetValue(META_NS::GetValue(prop));
        }
    }

    if (auto kfa = interface_cast<META_NS::IKeyframeAnimation>(&anim)) {
        auto fromProp = meta.GetProperty<const META_NS::IAny::Ptr>("From", META_NS::MetadataQuery::EXISTING);
        if (fromProp) {
            kfa->From()->SetValue(META_NS::GetValue(fromProp));
        }
        auto toProp = meta.GetProperty<const META_NS::IAny::Ptr>("To", META_NS::MetadataQuery::EXISTING);
        if (toProp) {
            kfa->To()->SetValue(META_NS::GetValue(toProp));
        }
    }

    ApplyKeyframes(meta, anim);

    auto curveProp = meta.GetProperty<const META_NS::IObject::Ptr>("Curve", META_NS::MetadataQuery::EXISTING);
    if (curveProp) {
        if (auto curve = interface_pointer_cast<META_NS::ICurve1D>(META_NS::GetValue(curveProp))) {
            anim.Curve()->SetValue(curve);
        }
    }
}

void AnimationTemplate::ApplyModifiers(const META_NS::IMetadata& meta, META_NS::IAnimation& anim)
{
    AttachLoopModifier(meta, anim);
    AttachSpeedModifier(meta, anim);
}

static META_NS::IAnimation::Ptr CreateAnimationByType(BASE_NS::string_view animationType)
{
    META_NS::ClassInfo classInfo = META_NS::ClassId::TrackAnimation;
    if (animationType == "keyframeAnimation") {
        classInfo = META_NS::ClassId::KeyframeAnimation;
    } else if (animationType == "sequentialAnimation") {
        classInfo = META_NS::ClassId::SequentialAnimation;
    } else if (animationType == "parallelAnimation") {
        classInfo = META_NS::ClassId::ParallelAnimation;
    }
    return META_NS::GetObjectRegistry().Create<META_NS::IAnimation>(classInfo);
}

static void LoadContainerChildren(const META_NS::IMetadata& meta, META_NS::IStaggeredAnimation& container);

static META_NS::IAnimation::Ptr LoadAnimationFromTemplate(const META_NS::IMetadata& meta)
{
    auto animationType = META_NS::GetValue<BASE_NS::string>(&meta, "AnimationType");
    auto anim = CreateAnimationByType(animationType);
    if (!anim) {
        CORE_LOG_W("Failed to create animation of type: %s", animationType.c_str());
        return nullptr;
    }

    AnimationTemplate::ApplyProperties(meta, *anim);
    AnimationTemplate::ApplyModifiers(meta, *anim);

    if (auto staggered = interface_cast<META_NS::IStaggeredAnimation>(anim)) {
        LoadContainerChildren(meta, *staggered);
    }
    return anim;
}

static void LoadContainerChildren(const META_NS::IMetadata& meta, META_NS::IStaggeredAnimation& container)
{
    auto childrenProp =
        meta.GetArrayProperty<const META_NS::IObject::Ptr>("Animations", META_NS::MetadataQuery::EXISTING);
    if (!childrenProp) {
        return;
    }
    auto children = childrenProp->GetValue();
    for (auto&& child : children) {
        if (!child) {
            CORE_LOG_W("Null child in container animation");
        } else if (auto childAnim = interface_pointer_cast<META_NS::IAnimation>(child)) {
            container.AddAnimation(childAnim);
        } else if (auto childMeta = interface_cast<META_NS::IMetadata>(child)) {
            auto anim = LoadAnimationFromTemplate(*childMeta);
            if (anim) {
                container.AddAnimation(anim);
            }
        }
    }
}

META_NS::IAnimation::Ptr AnimationTemplate::CreateAnimation() const
{
    const META_NS::IMetadata& meta = *this;
    return LoadAnimationFromTemplate(meta);
}

bool AnimationTemplate::ApplyTo(META_NS::IObject& target) const
{
    auto anim = interface_cast<META_NS::IAnimation>(&target);
    if (!anim) {
        return false;
    }
    const META_NS::IMetadata& meta = *this;
    ApplyProperties(meta, *anim);
    ApplyModifiers(meta, *anim);
    if (auto staggered = interface_cast<META_NS::IStaggeredAnimation>(anim)) {
        LoadContainerChildren(meta, *staggered);
    }
    return true;
}

SCENE_END_NAMESPACE()
