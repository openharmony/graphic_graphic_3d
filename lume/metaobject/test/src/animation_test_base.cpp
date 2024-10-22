/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <gmock/gmock-matchers.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>

using namespace testing;

namespace {
    const int STEPMS = 10; // step ms
}

META_BEGIN_NAMESPACE()

void AnimationTestBase::SetUp()
{
    ::testing::Test::SetUp();
    clock_ = GetObjectRegistry().Create<IManualClock>(META_NS::ClassId::ManualClock);
}

TimeSpan AnimationTestBase::Time() const
{
    return clock_->GetTime();
}

void AnimationTestBase::IncrementClockTime(const TimeSpan& time)
{
    clock_->IncrementTime(time);
}

void AnimationTestBase::Update(const TimeSpan& time)
{
    IncrementClockTime(time);
    GetAnimationController()->Step(clock_);
}

IClock::Ptr AnimationTestBase::GetTestClock() const
{
    return clock_;
}


IAnimationController::Ptr AnimationTestBase::GetAnimationController()
{
    return META_NS::GetAnimationController();
}

void AnimationTestBase::RunFrames(uint32_t frames, std::function<void(uint32_t frame)> updateFn)
{
    IncrementClockTime(TimeSpan::Milliseconds(STEPMS));
    for (uint32_t i = 0; i < frames; i++) {
        GetAnimationController()->Step(GetTestClock());
        updateFn(i + 1);
        if (i < frames - 1)
            IncrementClockTime(TimeSpan::Milliseconds(STEPMS));
    }
}

void AnimationTestBase::StepAnimationController(int64_t stepMs)
{
    clock_->IncrementTime(TimeSpan::Milliseconds(stepMs));
    GetAnimationController()->Step(GetTestClock());
}

void AnimationTestBase::StepAnimations(
    const std::vector<IAnimation::Ptr> animations, uint32_t frames, std::function<void(uint32_t frame)> updateFn)
{
    StepAnimations(animations, frames, STEPMS, updateFn);
}

void AnimationTestBase::StepAnimations(const std::vector<IAnimation::Ptr> animations, uint32_t frames,
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

META_END_NAMESPACE()