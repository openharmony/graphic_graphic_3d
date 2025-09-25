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

#include "TonemapETS.h"

namespace OHOS::Render3D {
TonemapETS::TonemapETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::ITonemap::Ptr tonemap)
    : postProc_(postProc), tonemap_(tonemap)
{}

TonemapETS::TonemapETS(const ToneMappingType &type, const float exposure)
{
    type_ = type;
    exposure_ = exposure;
}

TonemapETS::~TonemapETS()
{
    tonemap_.reset();
    postProc_.reset();
}

TonemapETS::ToneMappingType TonemapETS::GetType()
{
    if (auto tonemap = tonemap_.lock()) {
        type_ = FromInternalType(tonemap->Type()->GetValue());
    }
    return type_;
}

void TonemapETS::SetType(const ToneMappingType type)
{
    type_ = type;
    if (auto tonemap = tonemap_.lock()) {
        tonemap->Type()->SetValue(ToInternalType(type_));
    }
}

float TonemapETS::GetExposure()
{
    if (auto tonemap = tonemap_.lock()) {
        exposure_ = tonemap->Exposure()->GetValue();
    }
    return exposure_;
}

void TonemapETS::SetExposure(const float exposure)
{
    exposure_ = exposure;
    if (auto tonemap = tonemap_.lock()) {
        tonemap->Exposure()->SetValue(exposure_);
    }
}

bool TonemapETS::IsEnabled()
{
    if (auto tonemap = tonemap_.lock()) {
        return tonemap->Enabled()->GetValue();
    }
    return false;
}

void TonemapETS::SetEnabled(const bool enable)
{
    if (auto tonemap = tonemap_.lock()) {
        tonemap->Enabled()->SetValue(enable);
    }
}

SCENE_NS::TonemapType TonemapETS::ToInternalType(const TonemapETS::ToneMappingType &type)
{
    switch (type) {
        case TonemapETS::ToneMappingType::ACES_2020:
            return SCENE_NS::TonemapType::ACES_2020;
        case TonemapETS::ToneMappingType::FILMIC:
            return SCENE_NS::TonemapType::FILMIC;
        default:
            return SCENE_NS::TonemapType::ACES;
    }
}

TonemapETS::ToneMappingType TonemapETS::FromInternalType(const SCENE_NS::TonemapType &type)
{
    switch (type) {
        case SCENE_NS::TonemapType::ACES_2020:
            return TonemapETS::ToneMappingType::ACES_2020;
        case SCENE_NS::TonemapType::FILMIC:
            return TonemapETS::ToneMappingType::FILMIC;
        default:
            return TonemapETS::ToneMappingType::ACES;
    }
}
}  // namespace OHOS::Render3D
