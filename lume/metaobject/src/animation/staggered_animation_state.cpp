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
#include "staggered_animation_state.h"

META_BEGIN_NAMESPACE()

namespace Internal {

bool StaggeredAnimationState::Initialize(AnimationStateParams&& params)
{
    if (Super::Initialize(BASE_NS::move(params))) {
        onChildrenChanged_ = CreateShared<META_NS::EventImpl<IOnChanged>>();

        container_ = CreateContainer();
        if (auto required = interface_cast<IRequiredInterfaces>(container_)) {
            // Require all children to implement IAnimation
            required->SetRequiredInterfaces({ IAnimation::UID, IStartableAnimation::UID });
        }
        if (auto proxy = interface_cast<IContainerProxyParent>(container_)) {
            // The container should use our parent animation as the parent object for children (and not itself)
            proxy->SetProxyParent(interface_pointer_cast<IContainer>(GetOwner()));
        }
        container_->OnAdded()->AddHandler(MakeCallback<IOnChildChanged>(this, &StaggeredAnimationState::ChildAdded));
        container_->OnRemoved()->AddHandler(
            MakeCallback<IOnChildChanged>(this, &StaggeredAnimationState::ChildRemoved));
        container_->OnMoved()->AddHandler(MakeCallback<IOnChildMoved>(this, &StaggeredAnimationState::ChildMoved));

        childrenChanged_ = MakeCallback<IOnChanged>(this, &StaggeredAnimationState::ChildrenChanged);
    }
    return container_ != nullptr;
}

void StaggeredAnimationState::Uninitialize()
{
    for (auto& child : children_) {
        if (child.animation_) {
            child.durationChanged_.Unsubscribe();
            child.validChanged_.Unsubscribe();
            if (auto controller = child.controller_.lock()) {
                // If the animation was associated with a controller before it was
                // added to this container, add it back to that controller
                controller->AddAnimation(child.animation_);
            }
        }
    }
    Super::Uninitialize();
}

void StaggeredAnimationState::UpdateTotalDuration()
{
    baseDuration_ = TimeSpan::Zero(); // Reset base duration
    Super::UpdateTotalDuration();
}

void StaggeredAnimationState::ChildrenChanged()
{
    UpdateTotalDuration();
    Invoke<IOnChanged>(onChildrenChanged_);
}

void StaggeredAnimationState::ChildAdded(const ChildChangedInfo& info)
{
    // Take a strong reference to the animation
    IAnimation::Ptr animation = interface_pointer_cast<IAnimation>(info.object);
    if (!animation) {
        return;
    }

    // Remove the animation from it's controller as the staggered animation is handling it
    auto controller = GetValue(animation->Controller()).lock();
    SetValue(animation->Controller(), nullptr);

    size_t inContainerCount = 0;
    for (auto& child : GetChildren()) {
        if (child.animation_ == animation) {
            inContainerCount++;
        }
    }

    AnimationSegment segment { animation, controller };
    if (inContainerCount == 1) {
        segment.durationChanged_.Subscribe(animation->TotalDuration(), childrenChanged_);
        segment.validChanged_.Subscribe(animation->Valid(), childrenChanged_);
    }
    children_.push_back(std::move(segment));
    ChildrenChanged();
}

void StaggeredAnimationState::ChildRemoved(const ChildChangedInfo& info)
{
    IAnimation::Ptr animation = interface_pointer_cast<IAnimation>(info.object);
    for (auto it = children_.begin(); it != children_.end(); it++) {
        if (it->animation_ == animation) {
            RemoveChild(it);
            ChildrenChanged();
            break;
        }
    }
}

void StaggeredAnimationState::RemoveChild(typename SegmentVector::iterator& item)
{
    auto& segment = *item;
    segment.durationChanged_.Unsubscribe();
    segment.validChanged_.Unsubscribe();

    // If the animation was associated with a controller before it was
    // added to this container, return it to that controller
    SetValue(item->animation_->Controller(), segment.controller_.lock());

    children_.erase(item);

    // Empty staggered animation, remove it from it's controller
    if (children_.empty()) {
        if (auto controller = controller_.lock()) {
            if (auto animation = GetOwner()) {
                controller->RemoveAnimation(animation);
            }
        }
    }
}

void StaggeredAnimationState::ChildMoved(const ChildMovedInfo& info)
{
    auto fromIndex = info.from;
    auto toIndex = info.to;
    const auto size = children_.size();
    fromIndex = BASE_NS::Math::min(fromIndex, size - 1);
    toIndex = BASE_NS::Math::min(toIndex, size - 1);
    if (fromIndex == toIndex) {
        return;
    }
    auto& child = children_[fromIndex];
    if (fromIndex > toIndex) {
        const auto first = children_.rbegin() + static_cast<SegmentVector::difference_type>(size - fromIndex - 1);
        const auto last = children_.rbegin() + static_cast<SegmentVector::difference_type>(size - toIndex);
        std::rotate(first, first + 1, last);
    } else {
        const auto first = children_.begin() + static_cast<SegmentVector::difference_type>(fromIndex);
        const auto last = children_.begin() + static_cast<SegmentVector::difference_type>(toIndex + 1);
        std::rotate(first, first + 1, last);
    }
    ChildrenChanged();
}

bool StaggeredAnimationState::IsValid() const
{
    // If any of the children of this container are valid, then
    // the container is valid
    for (const auto& child : children_) {
        auto& animation = child.animation_;
        if (animation && GetValue(animation->Valid())) {
            return true;
        }
    }
    return false;
}

constexpr float MapTo01Range(float value, float inputStart, float inputEnd, bool reverse) noexcept
{
    if (reverse) {
        const auto offset = 1.f - inputEnd;
        inputStart += offset;
        inputEnd += offset;
    }
    return (1 / (inputEnd - inputStart)) * (value - inputStart);
}

constexpr IAnimationInternal::MoveParams TransformChild(const StaggeredAnimationState::AnimationSegment& segment,
    float parentProgress, IAnimationInternal::AnimationTargetState parentState, bool reverse) noexcept
{
    using AnimationTargetState = IAnimationInternal::AnimationTargetState;
    IAnimationInternal::MoveParams params;
    params.step.progress = MapTo01Range(parentProgress, segment.startProgress_, segment.endProgress_, reverse);

    if (parentState == AnimationTargetState::RUNNING) {
        params.state = AnimationTargetState::RUNNING;
        if (params.step.progress < 0.f) {
            params.state = AnimationTargetState::STOPPED;
        } else if (params.step.progress >= 1.f) {
            params.state = AnimationTargetState::FINISHED;
        }
    } else {
        params.state = parentState;
    }

    if (reverse) {
        params.step.progress = 1.f - params.step.progress;
    }
    params.step.reverse = reverse;
    return params;
}

// ParallelAnimationState

IContainer::Ptr ParallelAnimationState::CreateContainer() const
{
    return META_NS::GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
}

TimeSpan ParallelAnimationState::GetAnimationBaseDuration() const
{
    if (baseDuration_ != TimeSpan::Zero()) {
        return baseDuration_;
    }
    // Duration of a ParallelAnimation is the max of the total duration of children
    TimeSpan totalDuration = TimeSpan::Zero();
    for (const auto& segment : GetChildren()) {
        const auto& animation = segment.animation_;
        if (animation) {
            const TimeSpan childDuration = GetValue(animation->TotalDuration(), TimeSpan::Zero());
            totalDuration = BASE_NS::Math::max(totalDuration, childDuration);
        }
    }
    return totalDuration;
}

void ParallelAnimationState::ChildrenChanged()
{
    Super::ChildrenChanged();
    auto containerDuration = GetAnimationBaseDuration();
    for (auto&& segment : GetChildren()) {
        segment.startProgress_ = 0;
        segment.endProgress_ = 1.f;
        if (containerDuration.IsFinite()) {
            const auto duration = segment.animation_ ? GetValue(segment.animation_->TotalDuration()) : TimeSpan::Zero();
            segment.endProgress_ =
                static_cast<float>(duration.ToMilliseconds()) / static_cast<float>(containerDuration.ToMilliseconds());
        }
    }
}

AnyReturnValue ParallelAnimationState::Evaluate()
{
    AnyReturnValue status = AnyReturn::NOTHING_TO_DO;
    const float containerProgress = GetValue(GetOwner()->Progress());
    const auto step = ApplyStepModifiers(containerProgress);
    for (const auto& segment : GetChildren()) {
        if (const auto internal = interface_cast<IAnimationInternal>(segment.animation_)) {
            if (internal->Move(TransformChild(segment, containerProgress, GetAnimationTargetState(), step.reverse))) {
                status = AnyReturn::SUCCESS;
            }
        }
    }
    return status;
}

// SequentialAnimationState

IContainer::Ptr SequentialAnimationState::CreateContainer() const
{
    return META_NS::GetObjectRegistry().Create<IContainer>(ClassId::ObjectFlatContainer);
}

TimeSpan SequentialAnimationState::GetAnimationBaseDuration() const
{
    if (baseDuration_ != TimeSpan::Zero()) {
        return baseDuration_;
    }
    // Duration of a SequentialAnimation is the sum of the total duration of children
    TimeSpan totalDuration = TimeSpan::Zero();
    for (const auto& segment : GetChildren()) {
        const auto& animation = segment.animation_;
        if (animation) {
            totalDuration += GetValue(animation->TotalDuration(), TimeSpan::Zero());
        }
    }
    return totalDuration;
}

void SequentialAnimationState::ChildrenChanged()
{
    Super::ChildrenChanged();
    auto containerDuration = GetAnimationBaseDuration();
    float startProgress = 0.f;

    for (auto&& segment : GetChildren()) {
        segment.startProgress_ = startProgress;
        segment.endProgress_ = 1.f;
        if (containerDuration.IsFinite()) {
            const auto duration = segment.animation_ ? GetValue(segment.animation_->TotalDuration()) : TimeSpan::Zero();
            segment.endProgress_ = startProgress + static_cast<float>(duration.ToMilliseconds()) /
                                                       static_cast<float>(containerDuration.ToMilliseconds());
            startProgress = segment.endProgress_;
        } else {
            CORE_LOG_E("Infinite animation in a sequential animation");
            startProgress = 1.f;
        }
    }
}

SequentialAnimationState::ActiveAnimation SequentialAnimationState::GetActiveAnimation(
    float progress, bool reverse) const
{
    const auto& children = GetChildren();

    int64_t index = 0;
    for (const auto& anim : GetChildren()) {
        const auto transform = TransformChild(anim, progress, GetAnimationTargetState(), reverse);
        if (transform.state == IAnimationInternal::AnimationTargetState::RUNNING) {
            return { &anim, index };
        }
        index++;
    }
    return { nullptr, children.empty() ? -1 : index };
}

AnyReturnValue SequentialAnimationState::Evaluate()
{
    AnyReturnValue status = AnyReturn::NOTHING_TO_DO;
    const float containerProgress = GetValue(GetOwner()->Progress());
    const auto step = ApplyStepModifiers(containerProgress);
    const auto& children = GetChildren();

    auto active = GetActiveAnimation(containerProgress, step.reverse);
    if (!active) {
        // No active animation, make sure all animations are stopped/finished
        for (const auto& segment : children) {
            if (auto internal = interface_cast<IAnimationInternal>(segment.animation_)) {
                internal->Move(TransformChild(segment, containerProgress, GetAnimationTargetState(), step.reverse));
            }
        }
        return children.empty() ? AnyReturn::SUCCESS : AnyReturn::NOTHING_TO_DO;
    }

    // Iterate first->last or last->first depending on direction
    int mod = 1;
    size_t index = 0;
    size_t target = children.size();
    if (step.reverse) {
        mod = -1;
        std::swap(index, target);
    }
    for (; index != target; index += mod) {
        const auto& segment = children[index];
        const auto params = TransformChild(children[index], containerProgress, GetAnimationTargetState(), step.reverse);
        if (params.state == IAnimationInternal::AnimationTargetState::RUNNING ||
            segment.animation_ != active.segment->animation_) {
            // If one instance of the animation is in running state, don't touch the animation for any other state
            if (const auto internal = interface_cast<IAnimationInternal>(segment.animation_)) {
                internal->Move(params);
            }
        }
    }

    return status;
}

} // namespace Internal

META_END_NAMESPACE()
