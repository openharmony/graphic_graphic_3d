/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
