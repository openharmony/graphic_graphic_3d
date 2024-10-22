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

#ifndef META_TEST_ANIMATION_TEST_BASE_H
#define META_TEST_ANIMATION_TEST_BASE_H

#include <gtest/gtest.h>

#include <meta/base/time_span.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_manual_clock.h>

META_BEGIN_NAMESPACE()

class AnimationTestBase : public ::testing::Test {
protected:
    void SetUp() override;

    TimeSpan Time() const;

    void IncrementClockTime(const TimeSpan& time);

    void Update(const TimeSpan& time = TimeSpan::Milliseconds(0));

    IClock::Ptr GetTestClock() const;

    void RunFrames(uint32_t frames, std::function<void(uint32_t frame)> updateFn);

    void StepAnimations(const std::vector<IAnimation::Ptr> animations, uint32_t frames, int64_t frameStepMs,
        std::function<void(uint32_t frame)> updateFn = {});
    
    void StepAnimations(
        const std::vector<IAnimation::Ptr> animations, uint32_t frames, std::function<void(uint32_t frame)> updateFn);

    void StepAnimationController(int64_t stepMs = 10);

    IAnimationController::Ptr GetAnimationController();

private:
    IManualClock::Ptr clock_;
};

META_END_NAMESPACE()

#endif