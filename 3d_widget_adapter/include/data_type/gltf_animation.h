/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_GLTF_ANIMATION_H
#define OHOS_RENDER_3D_GLTF_ANIMATION_H

#include <string>
#include "data_type/constants.h"

namespace OHOS::Render3D {
class GLTFAnimation {
public:
    GLTFAnimation(const std::string& name, AnimationState state, int repeatCount = -1,
        float speed = 1.0, float duration = -1, bool reverse = false)
        : name_(name), state_(state), repeatCount_(repeatCount), speed_(speed),
        duration_(duration), reverse_(reverse) {} ;
    ~GLTFAnimation() = default;

    const std::string& GetName() const { return name_; }
    AnimationState GetState() const { return state_; }
    int GetRepeatCount() const { return repeatCount_; }
    float GetSpeed() const { return speed_; }
    float GetDuration() const { return duration_; }
    bool GetReverse() const { return reverse_; }

private:
    std::string name_ = "";
    AnimationState state_ = AnimationState::PLAY;
    int repeatCount_ = -1;
    float speed_ = 1.0;
    float duration_ = -1.0;
    bool reverse_ = false;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GLTF_ANIMATION_H
