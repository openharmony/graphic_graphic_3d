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

#include "ColorFringeETS.h"
#include "Utils.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace {
static constexpr uint32_t COLOR_FRINGE_FACTOR = 10;
}
namespace OHOS::Render3D {
ColorFringeETS::ColorFringeETS(
    const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::IColorFringe::Ptr colorFringe)
    : postProc_(postProc), colorFringe_(colorFringe)
{}

ColorFringeETS::ColorFringeETS(const float intensity)
{
    intensity_ = intensity;
}

ColorFringeETS::~ColorFringeETS()
{
    colorFringe_.reset();
    postProc_.reset();
}

float ColorFringeETS::GetIntensity()
{
    if (auto colorFringe = colorFringe_.lock()) {
        intensity_ = colorFringe->DistanceCoefficient()->GetValue();
    }
    return ScaleToETSForIntensity(intensity_);
}

void ColorFringeETS::SetIntensity(const float intensity)
{
    intensity_ = ScaleToNativeForIntensity(intensity);
    if (auto colorFringe = colorFringe_.lock()) {
        colorFringe->DistanceCoefficient()->SetValue(intensity_);
    }
}

float ColorFringeETS::ScaleToETSForIntensity(const float value)
{
    return value / COLOR_FRINGE_FACTOR;
}

float ColorFringeETS::ScaleToNativeForIntensity(const float value)
{
    return value * COLOR_FRINGE_FACTOR;
}

bool ColorFringeETS::IsEnabled()
{
    if (auto colorFringe = colorFringe_.lock()) {
        return colorFringe->Enabled()->GetValue();
    }
    return false;
}

void ColorFringeETS::SetEnabled(const bool enable)
{
    if (auto colorFringe = colorFringe_.lock()) {
        colorFringe->Enabled()->SetValue(enable);
    }
}
}  // namespace OHOS::Render3D
