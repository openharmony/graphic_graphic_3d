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

#include "VignetteETS.h"
#include "Utils.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace {
static constexpr double VIGNETTE_LOG_OFFSET = 5.562684646268003e-309;
}
namespace OHOS::Render3D {
VignetteETS::VignetteETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::IVignette::Ptr vignette)
    : postProc_(postProc), vignette_(vignette)
{}

VignetteETS::VignetteETS(const float roundness, const float intensity)
{
    roundness_ = roundness;
    intensity_ = intensity;
}

VignetteETS::~VignetteETS()
{
    vignette_.reset();
    postProc_.reset();
}

float VignetteETS::GetRoundness()
{
    if (auto vignette = vignette_.lock()) {
        roundness_ = vignette->Coefficient()->GetValue();
    }
    return ScaleToETSForRoundness(roundness_);
}

void VignetteETS::SetRoundness(const float roundness)
{
    roundness_ = ScaleToNativeForRoundness(roundness);
    if (auto vignette = vignette_.lock()) {
        vignette->Coefficient()->SetValue(roundness_);
    }
}

float VignetteETS::ScaleToETSForRoundness(const float value)
{
    float roundness = BASE_NS::Math::clamp01(BASE_NS::Math::pow(2, -value) - VIGNETTE_LOG_OFFSET);
    return roundness;
}

float VignetteETS::ScaleToNativeForRoundness(const float value)
{
    auto nativeValue = BASE_NS::Math::max(value, 0.0f);
    float roundness = BASE_NS::Math::clamp(-std::log2(nativeValue + VIGNETTE_LOG_OFFSET), 0.0, 1024.0);
    return roundness;
}

float VignetteETS::GetIntensity()
{
    if (auto vignette = vignette_.lock()) {
        intensity_ = vignette->Power()->GetValue();
    }
    return intensity_;
}

void VignetteETS::SetIntensity(const float intensity)
{
    intensity_ = intensity;
    if (auto vignette = vignette_.lock()) {
        vignette->Power()->SetValue(intensity_);
    }
}

bool VignetteETS::IsEnabled()
{
    if (auto vignette = vignette_.lock()) {
        return vignette->Enabled()->GetValue();
    }
    return false;
}

void VignetteETS::SetEnabled(const bool enable)
{
    if (auto vignette = vignette_.lock()) {
        vignette->Enabled()->SetValue(enable);
    }
}
}  // namespace OHOS::Render3D
