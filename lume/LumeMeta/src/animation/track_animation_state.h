/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Track animation state implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
 */

#ifndef META_SRC_ANIMATION_TRACK_ANIMATION_STATE_H
#define META_SRC_ANIMATION_TRACK_ANIMATION_STATE_H

#include <base/containers/unordered_map.h>

#include <meta/base/meta_types.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/property/array_property.h>

#include "animation_state.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class TrackAnimationState final : public PropertyAnimationState {
public:
    struct TrackDataParams {
        ArrayProperty<float> timestamps;
    };
    /**
     * @brief Set TrackAnimationState parameters
     */
    void SetTrackDataParams(TrackDataParams&& params);
    /**
     * @brief Updates the validity of the track animation.
     * @return True if the animation is in valid state, false otherwise.
     */
    bool UpdateValid();
    /** Return latest evaluated value */
    const IAny::Ptr& GetCurrentValue() const noexcept
    {
        return currentValue_;
    }
    /** Return value of the start of current track */
    const IAny::Ptr& GetCurrentTrackStart() const noexcept
    {
        return trackStart_;
    }
    /** Return value of the end of current track */
    const IAny::Ptr& GetCurrentTrackEnd() const noexcept
    {
        return trackEnd_;
    }
    /** Return TypeId of each keyframe */
    TypeId GetKeyframeItemTypeId() const noexcept
    {
        return keyframeArray_ ? keyframeArray_->GetTypeId(TypeIdRole::ITEM) : TypeId {};
    }
    /**
     * @brief Updates keyframe index based on given progress.
     * @param progress The animation progress to update to.
     * @return Index and track progress at given animation progress.
     */
    BASE_NS::pair<uint32_t, float> UpdateIndex(float progress);
    /**
     * @brief Jump to given keyframe
     * @param index Index to jump to.
     * @param progress Current progress
     * @return Current index of the track animation after jump.
     */
    uint32_t JumpTo(size_t index, float progress);
    /** Add a keyframe at given index */
    size_t AddKeyframe(float timestamp, const IAny::ConstPtr& value);
    /** Remove a keyframe at given index */
    bool RemoveKeyframe(size_t index);
    /** Resets the current track */
    void ResetCurrentTrack();
    /**  Updates the keyframe array containing the keyframes of the animation */
    bool SetKeyframes(const IArrayAny::Ptr& keyframes);

private:
    void Reset();
    bool ValidateValues();
    float GetCurrentTrackProgress(float progress) const noexcept;
    bool IsInCurrentRange(float progress) const noexcept;
    void SetPrePostFrameValues(float progress);

private:
    IArrayAny::Ptr keyframeArray_; // Keyframe values
    IAny::Ptr currentValue_;       // Latest evaluated value (between trackStart_ and trackEnd_)
    IAny::Ptr trackStart_;         // Current keyframe value
    IAny::Ptr trackEnd_;           // Next keyframe value
    float startProgress_ {};       // First timestamp
    float endProgress_ {};         // Last timestamp
    float currentRangeStartTs_ {}; // Timestamp of the current keyframe
    float currentRangeEndTs_ {};   // Timestamp of the next keyframe
    size_t currentIndex_ { ITrackAnimation::INVALID_INDEX };

    ArrayProperty<float>& GetTimeStamps() noexcept
    {
        return trackParams_.timestamps;
    }
    TrackDataParams trackParams_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_TRACK_ANIMATION_STATE_H
