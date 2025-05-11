/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Staggered animation container class implementation
 * Author: Lauri Jaaskela
 * Create: 2021-11-17
 */

#ifndef META_SRC_STAGGERED_ANIMATION_STATE_H
#define META_SRC_STAGGERED_ANIMATION_STATE_H

#include <algorithm>
#include <functional>

#include <meta/api/property/property_event_handler.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/ext/object_container.h>
#include <meta/interface/animation/intf_animation.h>

#include "animation_state.h"

META_BEGIN_NAMESPACE()

namespace Internal {

/**
 * @brief The Base state class for animation containers.
 */
class StaggeredAnimationState : public AnimationState {
    using Super = AnimationState;

public:
    struct AnimationSegment {
        IAnimation::Ptr animation_;                   // Animation in the segment
        IAnimationController::WeakPtr controller_;    // The view animation was originally associated with (if any)
        float startProgress_ {};                      // Progress [0..1] of the animation at start of the segment
        float endProgress_ {};                        // Percentage [0..1] of the animation length in this segment
        PropertyChangedEventHandler durationChanged_; // Handler for animation duration change
        PropertyChangedEventHandler validChanged_;    // Handler for animation validity change
    };

    using SegmentVector = BASE_NS::vector<AnimationSegment>;

public:
    META_NO_COPY_MOVE(StaggeredAnimationState)
    StaggeredAnimationState() = default;
    ~StaggeredAnimationState() override = default;

    IContainer& GetContainerInterface() noexcept
    {
        CORE_ASSERT(container_);
        return *container_;
    }
    const IContainer& GetContainerInterface() const noexcept
    {
        CORE_ASSERT(container_);
        return *container_;
    }

    IEvent::Ptr OnChildrenChanged() const noexcept
    {
        return onChildrenChanged_;
    }
    bool IsValid() const;

    virtual void ChildrenChanged();

protected: // AnimationState
    bool Initialize(AnimationStateParams&& params) override;
    void Uninitialize() override;
    void UpdateTotalDuration() override;

private:
    void ChildAdded(const ChildChangedInfo& info);
    void ChildRemoved(const ChildChangedInfo& info);
    void ChildMoved(const ChildChangedInfo& info);
    void RemoveChild(typename SegmentVector::iterator& item);

protected:
    SegmentVector& GetChildren() noexcept
    {
        return children_;
    }
    const SegmentVector& GetChildren() const noexcept
    {
        return children_;
    }

    virtual IContainer::Ptr CreateContainer() const = 0;

protected:
    TimeSpan baseDuration_ { TimeSpan::Zero() };

private:
    IEvent::Ptr onChildrenChanged_;
    IOnChanged::InterfaceTypePtr childrenChanged_;
    EventHandler containerChanged_;

    IContainer::Ptr container_;
    IAnimationController::WeakPtr controller_;
    SegmentVector children_;
};

/**
 * @brief State class for parallel animations
 */
class ParallelAnimationState : public StaggeredAnimationState {
    using Super = StaggeredAnimationState;

public:
    AnyReturnValue Evaluate();

protected:
    IContainer::Ptr CreateContainer() const override;
    void ChildrenChanged() override;
    TimeSpan GetAnimationBaseDuration() const override;
};

/**
 * @brief State class for sequential animations
 */
class SequentialAnimationState : public StaggeredAnimationState {
    using Super = StaggeredAnimationState;

public:
    AnyReturnValue Evaluate();

protected:
    IContainer::Ptr CreateContainer() const override;
    void ChildrenChanged() override;
    TimeSpan GetAnimationBaseDuration() const override;

    struct ActiveAnimation {
        const AnimationSegment* segment {};
        int64_t index {};
        explicit operator bool() const noexcept
        {
            return segment != nullptr;
        }
    };

    ActiveAnimation GetActiveAnimation(float progress, bool reverse) const;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_STAGGERED_ANIMATION_STATE_H
