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

std::shared_ptr<TonemapETS> TonemapETS::FromJS(const TonemapETS::ToneMappingType &type, const float exposure)
{
    auto tm = std::shared_ptr<TonemapETS>(new TonemapETS());
    tm->SetType(type);
    tm->SetExposure(exposure);
    return tm;
}

TonemapETS::TonemapETS()
{}

TonemapETS::TonemapETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::ITonemap::Ptr tonemap)
    : postProc_(postProc), tonemap_(tonemap)
{}

TonemapETS::~TonemapETS()
{
    tonemap_.reset();
    postProc_.reset();
}

TonemapETS::ToneMappingType TonemapETS::GetType()
{
    if (tonemap_) {
        type_ = FromInternalType(tonemap_->Type()->GetValue());
    }
    return type_;
}

void TonemapETS::SetType(const ToneMappingType type)
{
    type_ = type;
    if (tonemap_) {
        tonemap_->Type()->SetValue(ToInternalType(type_));
    }
}

float TonemapETS::GetExposure()
{
    if (tonemap_) {
        exposure_ = tonemap_->Exposure()->GetValue();
    }
    return exposure_;
}

void TonemapETS::SetExposure(const float exposure)
{
    exposure_ = exposure;
    if (tonemap_) {
        tonemap_->Exposure()->SetValue(exposure_);
    }
}

bool TonemapETS::IsEnabled()
{
    if (tonemap_) {
        return tonemap_->Enabled()->GetValue();
    }
    return false;
}

void TonemapETS::SetEnabled(const bool enable)
{
    if (tonemap_) {
        tonemap_->Enabled()->SetValue(enable);
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
