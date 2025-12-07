/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "animation_test_base.h"

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {

void API_AnimationTestBase::SetUp()
{
    ::testing::Test::SetUp();
    clock_ = GetObjectRegistry().Create<IManualClock>(META_NS::ClassId::ManualClock);
}

TimeSpan API_AnimationTestBase::Time() const
{
    return clock_->GetTime();
}

void API_AnimationTestBase::IncrementClockTime(const TimeSpan& time)
{
    clock_->IncrementTime(time);
}

void API_AnimationTestBase::Update(const TimeSpan& time)
{
    IncrementClockTime(time);
    GetAnimationController()->Step(clock_);
}

IClock::Ptr API_AnimationTestBase::GetTestClock() const
{
    return clock_;
}

IAnimationController::Ptr API_AnimationTestBase::GetAnimationController()
{
    return META_NS::GetAnimationController();
}

void API_AnimationTestBase::RunFrames(uint32_t frames, std::function<void(uint32_t frame)> updateFn)
{
    // Sleep for a bit before first update to allow e.g. animations to have some time
    // to proceed before the first animation frame.
    IncrementClockTime(TimeSpan::Milliseconds(10));
    for (uint32_t i = 0; i < frames; i++) {
        GetAnimationController()->Step(GetTestClock());
        updateFn(i + 1);
        if (i < frames - 1)
            IncrementClockTime(TimeSpan::Milliseconds(10));
    }
}

void API_AnimationTestBase::StepAnimationController(int64_t stepMs)
{
    clock_->IncrementTime(TimeSpan::Milliseconds(stepMs));
    GetAnimationController()->Step(GetTestClock());
}

void API_AnimationTestBase::StepAnimations(
    const std::vector<IAnimation::Ptr> animations, uint32_t frames, std::function<void(uint32_t frame)> updateFn)
{
    StepAnimations(animations, frames, 10, updateFn);
}

void API_AnimationTestBase::StepAnimations(const std::vector<IAnimation::Ptr> animations, uint32_t frames,
    int64_t frameStepMs, std::function<void(uint32_t frame)> updateFn)
{
    IncrementClockTime(TimeSpan::Milliseconds(frameStepMs));
    for (uint32_t i = 0; i < frames; i++) {
        for (auto& animation : animations) {
            animation->Step(GetTestClock());
        }
        if (updateFn) {
            updateFn(i + 1);
        }
        if (i < frames - 1)
            IncrementClockTime(TimeSpan::Milliseconds(frameStepMs));
    }
}

} // namespace UTest
META_END_NAMESPACE()
