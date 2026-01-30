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

#ifndef OHOS_3D_COLOR_FRINGE_ETS_H
#define OHOS_3D_COLOR_FRINGE_ETS_H

#include <memory>

#include <scene/interface/intf_scene.h>
#include <scene/interface/postprocess/intf_color_fringe.h>
#include <scene/interface/intf_postprocess.h>

namespace OHOS::Render3D {
class ColorFringeETS {
public:
    static constexpr float DEFAULT_INTENSITY = 2.0f;

    ColorFringeETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::IColorFringe::Ptr colorFringe);
    // store colorFringe settings from ETS
    ColorFringeETS(const float intensity);
    ~ColorFringeETS();

    float GetIntensity();
    void SetIntensity(const float intensity);

    bool IsEnabled();
    void SetEnabled(const bool enable);

    bool StrictEqual(const std::shared_ptr<ColorFringeETS> other) const
    {
        if (!other) {
            return false;
        }
        auto myPostProc = postProc_.lock();
        auto otherPostProc = other->postProc_.lock();
        auto myColorFringe = colorFringe_.lock();
        auto otherColorFringe = other->colorFringe_.lock();

        if (!myPostProc || !otherPostProc || !myColorFringe || !otherColorFringe) {
            return false;
        }
        return (myPostProc == otherPostProc && myColorFringe == otherColorFringe);
    }

    bool IsMatch(const std::shared_ptr<ColorFringeETS> other) const
    {
        // one colorFringe cannot be assigned to different post-processes.
        if (!other) {
            return true;
        }
        auto otherPostProcess = other->postProc_.lock();
        if (!otherPostProcess) {
            return true;
        }
        return postProc_.lock() == otherPostProcess;
    }

    static float ScaleToETSForIntensity(const float value);
    static float ScaleToNativeForIntensity(const float value);

private:
    float intensity_{DEFAULT_INTENSITY};

    SCENE_NS::IPostProcess::WeakPtr postProc_;
    SCENE_NS::IColorFringe::WeakPtr colorFringe_;  // colorFringe object from postproc_
};
}  // namespace OHOS::Render3D
#endif // OHOS_3D_COLOR_FRINGE_ETS_H