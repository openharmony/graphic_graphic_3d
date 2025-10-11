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

#include "BloomETS.h"
#include "Utils.h"

namespace OHOS::Render3D {
BloomETS::BloomETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::IBloom::Ptr bloom)
    : postProc_(postProc), bloom_(bloom)
{}

BloomETS::BloomETS(const float thresholdHard, const float thresholdSoft, const float scaleFactor, const float scatter)
{
    thresholdHard_ = thresholdHard;
    thresholdSoft_ = thresholdSoft;
    scaleFactor_ = scaleFactor;
    scatter_ = scatter;
}

BloomETS::~BloomETS()
{
    bloom_.reset();
    postProc_.reset();
}

float BloomETS::GetAmountCoefficient()
{
    if (auto bloom = bloom_.lock()) {
        amountCoefficient_ = bloom->AmountCoefficient()->GetValue();
    }
    return amountCoefficient_;
}

void BloomETS::SetAmountCoefficient(const float amount)
{
    amountCoefficient_ = amount;
    if (auto bloom = bloom_.lock()) {
        bloom->AmountCoefficient()->SetValue(amountCoefficient_);
    }
}

float BloomETS::GetThresholdSoft()
{
    if (auto bloom = bloom_.lock()) {
        thresholdSoft_ = bloom->ThresholdSoft()->GetValue();
    }
    return thresholdSoft_;
}

void BloomETS::SetThresholdSoft(const float thresholdSoft)
{
    thresholdSoft_ = thresholdSoft;
    if (auto bloom = bloom_.lock()) {
        bloom->ThresholdSoft()->SetValue(thresholdSoft_);
    }
}

float BloomETS::GetThresholdHard()
{
    if (auto bloom = bloom_.lock()) {
        thresholdHard_ = bloom->ThresholdHard()->GetValue();
    }
    return thresholdHard_;
}

void BloomETS::SetThresholdHard(const float thresholdHard)
{
    thresholdHard_ = thresholdHard;
    if (auto bloom = bloom_.lock()) {
        bloom->ThresholdHard()->SetValue(thresholdHard_);
    }
}

float BloomETS::GetScatter()
{
    if (auto bloom = bloom_.lock()) {
        ExecSyncTask([this]() {
            if (auto bloom = bloom_.lock()) {
                scatter_ = bloom->Scatter()->GetValue();
            }
            return META_NS::IAny::Ptr{};
        });
    }
    return scatter_;
}

void BloomETS::SetScatter(const float scatter)
{
    scatter_ = scatter;
    if (auto bloom = bloom_.lock()) {
        bloom->Scatter()->SetValue(scatter_);
    }
}

float BloomETS::GetScaleFactor()
{
    if (auto bloom = bloom_.lock()) {
        ExecSyncTask([this]() {
            if (auto bloom = bloom_.lock()) {
                scaleFactor_ = bloom->ScaleFactor()->GetValue();
            }
            return META_NS::IAny::Ptr{};
        });
    }
    return scaleFactor_;
}

void BloomETS::SetScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
    if (auto bloom = bloom_.lock()) {
        bloom->ScaleFactor()->SetValue(scaleFactor_);
    }
}

BloomETS::Type BloomETS::GetType()
{
    if (auto bloom = bloom_.lock()) {
        type_ = FromInternalType(bloom->Type()->GetValue());
    }
    return type_;
}

void BloomETS::SetType(const BloomETS::Type &type)
{
    type_ = (BloomETS::Type)type;
    if (auto bloom = bloom_.lock()) {
        bloom->Type()->SetValue(ToInternalType(type_));
    }
}

BloomETS::Quality BloomETS::GetQuality()
{
    if (auto bloom = bloom_.lock()) {
        quality_ = FromInternalQuality(bloom->Quality()->GetValue());
    }
    return quality_;
}

void BloomETS::SetQuality(const BloomETS::Quality &quality)
{
    quality_ = (BloomETS::Quality)quality;
    if (auto bloom = bloom_.lock()) {
        bloom->Quality()->SetValue(ToInternalQuality(quality_));
    }
}

bool BloomETS::IsEnabled()
{
    if (auto bloom = bloom_.lock()) {
        return bloom->Enabled()->GetValue();
    }
    return false;
}

void BloomETS::SetEnabled(const bool enable)
{
    if (auto bloom = bloom_.lock()) {
        bloom->Enabled()->SetValue(enable);
    }
}

SCENE_NS::BloomType BloomETS::ToInternalType(const BloomETS::Type &type)
{
    switch (type) {
        case Type::HORIZONTAL:
            return SCENE_NS::BloomType::HORIZONTAL;
        case Type::VERTICAL:
            return SCENE_NS::BloomType::VERTICAL;
        case Type::BILATERAL:
            return SCENE_NS::BloomType::BILATERAL;
        case Type::NORMAL:
        default:
            return SCENE_NS::BloomType::NORMAL;
    }
}

BloomETS::Type BloomETS::FromInternalType(const SCENE_NS::BloomType &type)
{
    switch (type) {
        case SCENE_NS::BloomType::HORIZONTAL:
            return BloomETS::Type::HORIZONTAL;
        case SCENE_NS::BloomType::VERTICAL:
            return BloomETS::Type::VERTICAL;
        case SCENE_NS::BloomType::BILATERAL:
            return BloomETS::Type::BILATERAL;
        case SCENE_NS::BloomType::NORMAL:
        default:
            return BloomETS::Type::NORMAL;
    }
}

SCENE_NS::EffectQualityType BloomETS::ToInternalQuality(const BloomETS::Quality &quality)
{
    switch (quality) {
        case Quality::LOW:
            return SCENE_NS::EffectQualityType::LOW;
        case Quality::HIGH:
            return SCENE_NS::EffectQualityType::HIGH;
        case Quality::NORMAL:
        default:
            return SCENE_NS::EffectQualityType::NORMAL;
    }
}

BloomETS::Quality BloomETS::FromInternalQuality(const SCENE_NS::EffectQualityType &quality)
{
    switch (quality) {
        case SCENE_NS::EffectQualityType::LOW:
            return BloomETS::Quality::LOW;
        case SCENE_NS::EffectQualityType::HIGH:
            return BloomETS::Quality::HIGH;
        case SCENE_NS::EffectQualityType::NORMAL:
        default:
            return BloomETS::Quality::NORMAL;
    }
}
}  // namespace OHOS::Render3D