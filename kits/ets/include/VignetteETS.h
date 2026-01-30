/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_VIGNETTE_ETS_H
#define OHOS_3D_VIGNETTE_ETS_H

#include <memory>

#include <scene/interface/intf_scene.h>
#include <scene/interface/postprocess/intf_vignette.h>
#include <scene/interface/intf_postprocess.h>

namespace OHOS::Render3D {
class VignetteETS {
public:
    static constexpr float DEFAULT_ROUNDNESS = 0.5f;
    static constexpr float DEFAULT_INTENSITY = 0.4f;

    VignetteETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::IVignette::Ptr vignette);
    // store vignette settings from ETS
    VignetteETS(const float roundness, const float intensity);
    ~VignetteETS();

    float GetRoundness();
    void SetRoundness(const float roundness);

    float GetIntensity();
    void SetIntensity(const float intensity);

    bool IsEnabled();
    void SetEnabled(const bool enable);

    bool StrictEqual(const std::shared_ptr<VignetteETS> other) const
    {
        if (!other) {
            return false;
        }
        auto myPostProc = postProc_.lock();
        auto otherPostProc = other->postProc_.lock();
        auto myVignette = vignette_.lock();
        auto otherVignette = other->vignette_.lock();

        if (!myPostProc || !otherPostProc || !myVignette || !otherVignette) {
            return false;
        }
        return (myPostProc == otherPostProc && myVignette == otherVignette);
    }

    bool IsMatch(const std::shared_ptr<VignetteETS> other) const
    {
        // one vignette cannot be assigned to different post-processes.
        if (!other) {
            return true;
        }
        auto otherPostProcess = other->postProc_.lock();
        if (!otherPostProcess) {
            return true;
        }
        return postProc_.lock() == otherPostProcess;
    }

    static float ScaleToETSForRoundness(const float value);
    static float ScaleToNativeForRoundness(const float value);

private:
    float roundness_{DEFAULT_ROUNDNESS};
    float intensity_{DEFAULT_INTENSITY};

    SCENE_NS::IPostProcess::WeakPtr postProc_;
    SCENE_NS::IVignette::WeakPtr vignette_;  // vignette object from postproc_
};
}  // namespace OHOS::Render3D
#endif // OHOS_3D_VIGNETTE_ETS_H