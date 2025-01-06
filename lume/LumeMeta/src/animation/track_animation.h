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

#ifndef META_SRC_ANIMATION_TRACK_ANIMATION_H
#define META_SRC_ANIMATION_TRACK_ANIMATION_H

#include <meta/base/meta_types.h>

#include "animation.h"
#include "track_animation_state.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class TrackAnimation final : public IntroduceInterfaces<BasePropertyAnimationFwd<ITimedAnimation>, ITrackAnimation,
                                 IStartableAnimation, ISerializable> {
    META_OBJECT(TrackAnimation, META_NS::ClassId::TrackAnimation, IntroduceInterfaces)
public:
    bool Build(const IMetadata::Ptr& data) override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_ARRAY_PROPERTY_DATA(ITrackAnimation, float, Timestamps)
    META_STATIC_ARRAY_PROPERTY_DATA(ITrackAnimation, ICurve1D::Ptr, KeyframeCurves)
    META_STATIC_ARRAY_PROPERTY_DATA(ITrackAnimation, IFunction::Ptr, KeyframeHandlers)
    META_STATIC_PROPERTY_DATA(ITrackAnimation, uint32_t, CurrentKeyframeIndex)
    META_END_STATIC_DATA()

    META_IMPLEMENT_ARRAY_PROPERTY(float, Timestamps)
    META_IMPLEMENT_ARRAY_PROPERTY(ICurve1D::Ptr, KeyframeCurves)
    META_IMPLEMENT_ARRAY_PROPERTY(IFunction::Ptr, KeyframeHandlers)
    META_IMPLEMENT_READONLY_PROPERTY(uint32_t, CurrentKeyframeIndex)

protected: // IAnimation
    void Step(const IClock::ConstPtr& clock) override;

protected: // ITrackAnimation
    IProperty::Ptr Keyframes() const override
    {
        return keyframes_;
    }
    size_t AddKeyframe(float timestamp, const IAny::ConstPtr& value) override;
    bool RemoveKeyframe(size_t index) override;
    void RemoveAllKeyframes() override;

protected: // IStartableAnimation
    void Pause() override;
    void Restart() override;
    void Seek(float position) override;
    void Start() override;
    void Stop() override;
    void Finish() override;

public: // IModifier
    EvaluationResult ProcessOnGet(IAny& value) override;

    TrackAnimationState& GetState() noexcept override
    {
        return state_;
    }

protected: // IAnimationInternal
    void OnAnimationStateChanged(const IAnimationInternal::AnimationStateChangedInfo& info) override;

protected:
    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;
    ReturnError Finalize(IImportFunctions&) override;

private:
    AnimationState::AnimationStateParams GetParams() override;
    void OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous) override;
    void Initialize();
    void RemoveModifier(const IStackProperty::Ptr& stack);

    void UpdateValid();
    void ResetTrack();
    void UpdateAnimation();
    void UpdateCurrentTrack(uint32_t index);
    void UpdateKeyframes();

private:
    void Evaluate() override;
    IProperty::Ptr keyframes_; // Array property containing the keyframes

    TrackAnimationState state_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_TRACK_ANIMATION_H
