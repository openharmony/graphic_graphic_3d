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

#ifndef META_TEST_ANIMATION_TEST_BASE_H
#define META_TEST_ANIMATION_TEST_BASE_H

#include <meta/base/time_span.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_manual_clock.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {

class API_AnimationTestBase : public ::testing::Test {
protected:
    void SetUp() override;
    /**
     * @brief Gives time stored in manual clock.
     * @return time of test clock.
     */
    TimeSpan Time() const;
    /**
     * @brief Increment clock time.
     *        Replacement for sleeping thread to avoid extending test application run time.
     * @param time Amount of time added to clock time.
     */
    void IncrementClockTime(const TimeSpan& time);

    void Update(const TimeSpan& time = TimeSpan::Milliseconds(0));

    /**
     * @brief Provides interface to test clock
     * @return Pointer to test clock
     */
    IClock::Ptr GetTestClock() const;
    /**
     * @brief Updates the view for a configurable number of frames.
     * @param frames number of frames to process.
     * @param updateFn Calls updateFn after each update.
     */
    void RunFrames(uint32_t frames, std::function<void(uint32_t frame)> updateFn);
    /**
     * @brief Steps the given list of animations over for a configurable number of frames.
     *        This function is useful for updating animations that are not part of a view
     *        hierarchy. For animations that animate widget properties, use RunFrames.
     * @param animations A list of animations to run.
     * @param frames Number of frames to process.
     * @param frameStepMs Number of ms to step between each frame
     * @param updateFn Calls updateFn after each step.
     */
    void StepAnimations(const std::vector<IAnimation::Ptr> animations, uint32_t frames, int64_t frameStepMs,
        std::function<void(uint32_t frame)> updateFn = {});
    /**
     * @brief Steps the given list of animations over for a configurable number of frames.
     *        This function is useful for updating animations that are not part of a view
     *        hierarchy. For animations that animate widget properties, use RunFrames.
     * @param animations A list of animations to run.
     * @param frames Number of frames to process.
     * @param updateFn Calls updateFn after each step.
     */
    void StepAnimations(
        const std::vector<IAnimation::Ptr> animations, uint32_t frames, std::function<void(uint32_t frame)> updateFn);
    /**
     * @brief Steps the global animation controller.
     * @param stepMs Milliseconds to step.
     */
    void StepAnimationController(int64_t stepMs = 10);
    /**
     * @brief Returns the animation controller used by the tests.
     */
    IAnimationController::Ptr GetAnimationController();

private:
    IManualClock::Ptr clock_;
};

} // namespace UTest
META_END_NAMESPACE()

#endif // META_TEST_ANIMATION_TEST_BASE_H
