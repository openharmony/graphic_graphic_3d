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

#include "track_animation_state.h"

#include <limits>

META_BEGIN_NAMESPACE()

namespace Internal {

void TrackAnimationState::SetTrackDataParams(TrackDataParams&& params)
{
    trackParams_ = BASE_NS::move(params);
}

size_t TrackAnimationState::AddKeyframe(float timestamp, const IAny::ConstPtr& value)
{
    if (!keyframeArray_) {
        CORE_LOG_E("Invalid keyframe array target.");
        return ITrackAnimation::INVALID_INDEX;
    }

    timestamp = BASE_NS::Math::clamp01(timestamp);
    size_t index = 0;

    // Find index for timestamp which maintains ascending timestamp order
    auto timestamps = GetTimeStamps()->GetValue();

    for (auto t : timestamps) {
        if (t > timestamp) {
            break;
        }
        index++;
    }

    if (keyframeArray_->InsertAnyAt(index, *value)) {
        GetTimeStamps()->InsertValueAt(index, timestamp);
        ValidateValues();
    } else {
        CORE_LOG_E("Failed to add keyframe to TrackAnimation");
        index = ITrackAnimation::INVALID_INDEX;
    }

    return index;
}

bool TrackAnimationState::RemoveKeyframe(size_t index)
{
    if (!keyframeArray_ || index >= GetTimeStamps()->GetSize()) {
        return false;
    }
    if (keyframeArray_->RemoveAt(index)) {
        GetTimeStamps()->RemoveAt(index);
        return true;
    }
    return false;
}

bool TrackAnimationState::UpdateValid()
{
    if (!(keyframeArray_ && trackStart_ && trackEnd_ && currentValue_)) {
        return false;
    }
    auto& timestamps = GetTimeStamps();
    auto size = timestamps->GetSize();
    if (timestamps && size > 1 && keyframeArray_->GetSize() == size) {
        // Update our timestamp range
        startProgress_ = timestamps->GetValueAt(0);
        endProgress_ = timestamps->GetValueAt(size - 1);
        return true;
    }
    return false;
}

void TrackAnimationState::ResetCurrentTrack()
{
    currentRangeStartTs_ = std::numeric_limits<float>::max();
    currentRangeEndTs_ = {};
    currentIndex_ = ITrackAnimation::INVALID_INDEX;
}

void TrackAnimationState::Reset()
{
    trackStart_.reset();
    trackEnd_.reset();
    currentValue_.reset();
}

bool TrackAnimationState::ValidateValues()
{
    if (!keyframeArray_) {
        return false;
    }
    if (trackStart_ && trackStart_->GetTypeId() != keyframeArray_->GetTypeId(TypeIdRole::ITEM)) {
        Reset();
    }
    if (!trackStart_) {
        if (auto size = keyframeArray_->GetSize()) {
            if (trackStart_ = keyframeArray_->Clone({ CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM }); trackStart_) {
                trackEnd_ = trackStart_->Clone(false);
                keyframeArray_->GetAnyAt(0, *trackStart_);
                keyframeArray_->GetAnyAt(size - 1, *trackEnd_);
                currentValue_ = trackStart_->Clone(true);
            }
        } else {
            Reset();
        }
    }
    return trackStart_ != nullptr;
}

bool TrackAnimationState::SetKeyframes(const IArrayAny::Ptr& keyframes)
{
    bool valid = false;
    if (keyframes) {
        if (keyframes != keyframeArray_) {
            keyframeArray_ = keyframes;
            valid = ValidateValues();
        } else {
            valid = true;
        }
    }
    if (auto& timestamps = GetTimeStamps()) {
        if (const auto size = timestamps->GetSize()) {
            startProgress_ = timestamps->GetValueAt(0);
            endProgress_ = timestamps->GetValueAt(size - 1);
        }
    }
    if (!valid) {
        Reset();
    }
    return valid;
}

inline static constexpr bool IsBetween(float value, float rangeStart, float rangeEnd, bool includeEnd)
{
    if (includeEnd) {
        return value >= rangeStart && value <= rangeEnd;
    }
    return value >= rangeStart && value < rangeEnd;
}

bool TrackAnimationState::IsInCurrentRange(float progress) const noexcept
{
    if (currentIndex_ == ITrackAnimation::INVALID_INDEX || !keyframeArray_) {
        return false;
    }

    // Include also end in the range if last keyframe
    return IsBetween(
        progress, currentRangeStartTs_, currentRangeEndTs_, currentIndex_ == keyframeArray_->GetSize() - 1);
}

BASE_NS::pair<uint32_t, float> TrackAnimationState::UpdateIndex(float progress)
{
    uint32_t index = static_cast<uint32_t>(currentIndex_);
    if (IsInCurrentRange(progress)) {
        return { index, GetCurrentTrackProgress(progress) };
    }

    const auto size = keyframeArray_ ? keyframeArray_->GetSize() : 0;
    auto& timestamps = GetTimeStamps();
    if (!size || !timestamps || timestamps->GetSize() != size) {
        index = JumpTo(size_t(ITrackAnimation::INVALID_INDEX));
    } else {
        size_t lo = 0;
        auto hi = size - 1;
        auto startTs = timestamps->GetValueAt(lo);
        auto endTs = timestamps->GetValueAt(hi);
        if (progress < startTs || progress > endTs) {
            index = JumpTo(size_t(ITrackAnimation::INVALID_INDEX));
        } else {
            while (lo <= hi) {
                const auto mid = lo + (hi - lo) / 2;
                const auto endIndex = mid < size - 1 ? mid + 1 : mid;
                startTs = timestamps->GetValueAt(mid);
                endTs = timestamps->GetValueAt(endIndex);
                if (IsBetween(progress, startTs, endTs, mid >= endIndex)) {
                    // Found correct keyframe
                    index = JumpTo(mid);
                    break;
                }
                if (progress < startTs) {
                    hi = mid - 1;
                } else {
                    lo = mid + 1;
                }
            }
        }
    }
    return { index, GetCurrentTrackProgress(progress) };
}

uint32_t TrackAnimationState::JumpTo(size_t index)
{
    if (index == currentIndex_) {
        return currentIndex_;
    }

    if (index == ITrackAnimation::INVALID_INDEX) {
        currentIndex_ = index;
        ResetCurrentTrack();
    } else {
        const auto size = keyframeArray_ ? keyframeArray_->GetSize() : 0;
        auto& timestamps = GetTimeStamps();
        if (!size || !timestamps || timestamps->GetSize() != size) {
            return ITrackAnimation::INVALID_INDEX;
        }
        index = BASE_NS::Math::min(index, size - 1);

        bool last = index == size - 1;
        auto start = last ? index - 1 : index;
        auto end = start + 1; // < size - 1 ? start + 1 : start;

        keyframeArray_->GetAnyAt(end, *trackEnd_);
        keyframeArray_->GetAnyAt(start, *trackStart_);
        currentRangeStartTs_ = timestamps->GetValueAt(start);
        currentRangeEndTs_ = timestamps->GetValueAt(end);
        currentIndex_ = last ? end : index;
    }
    return static_cast<uint32_t>(currentIndex_);
}

float TrackAnimationState::GetCurrentTrackProgress(float progress) const noexcept
{
    if (currentIndex_ == ITrackAnimation::INVALID_INDEX) {
        return 0.f;
    }
    float currentTrackRange = currentRangeEndTs_ - currentRangeStartTs_;
    auto rangeProgress = BASE_NS::Math::clamp(progress, startProgress_, endProgress_);
    if (currentTrackRange < BASE_NS::Math::EPSILON) {
        return 1.f;
    }
    return 1.f / currentTrackRange * (rangeProgress - currentRangeStartTs_);
}

} // namespace Internal

META_END_NAMESPACE()
