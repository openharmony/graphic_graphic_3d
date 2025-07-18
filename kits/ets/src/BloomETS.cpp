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

std::shared_ptr<BloomETS> BloomETS::FromJS(
    const float thresholdHard, const float thresholdSoft, const float scaleFactor, const float scatter)
{
    auto bc = std::shared_ptr<BloomETS>(new BloomETS());
    bc->SetThresholdHard(thresholdHard);
    bc->SetThresholdSoft(thresholdSoft);
    bc->SetScatter(scatter);
    bc->SetScaleFactor(scaleFactor);
    return bc;
}

BloomETS::BloomETS()
{}

BloomETS::BloomETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::IBloom::Ptr bloom)
    : postProc_(postProc), bloom_(bloom)
{
    // SetFrom(bloom);
}

BloomETS::~BloomETS()
{
    bloom_.reset();
    postProc_.reset();
}

float BloomETS::GetAmountCoefficient()
{
    if (bloom_) {
        amountCoefficient_ = bloom_->AmountCoefficient()->GetValue();
    }
    return amountCoefficient_;
}

void BloomETS::SetAmountCoefficient(const float amount)
{
    amountCoefficient_ = amount;
    if (bloom_) {
        bloom_->AmountCoefficient()->SetValue(amountCoefficient_);
    }
}

float BloomETS::GetThresholdSoft()
{
    if (bloom_) {
        thresholdSoft_ = bloom_->ThresholdSoft()->GetValue();
    }
    return thresholdSoft_;
}

void BloomETS::SetThresholdSoft(const float thresholdSoft)
{
    thresholdSoft_ = thresholdSoft;
    if (bloom_) {
        bloom_->ThresholdSoft()->SetValue(thresholdSoft_);
    }
}

float BloomETS::GetThresholdHard()
{
    if (bloom_) {
        thresholdHard_ = bloom_->ThresholdHard()->GetValue();
    }
    return thresholdHard_;
}

void BloomETS::SetThresholdHard(const float thresholdHard)
{
    thresholdHard_ = thresholdHard;
    if (bloom_) {
        bloom_->ThresholdHard()->SetValue(thresholdHard_);
    }
}

float BloomETS::GetScatter()
{
    if (bloom_) {
        ExecSyncTask([this]() {
            scatter_ = bloom_->Scatter()->GetValue();
            return META_NS::IAny::Ptr{};
        });
    }
    return scatter_;
}

void BloomETS::SetScatter(const float scatter)
{
    scatter_ = scatter;
    if (bloom_) {
        bloom_->Scatter()->SetValue(scatter_);
    }
}

float BloomETS::GetScaleFactor()
{
    if (bloom_) {
        ExecSyncTask([this]() {
            scaleFactor_ = bloom_->ScaleFactor()->GetValue();
            return META_NS::IAny::Ptr{};
        });
    }
    return scaleFactor_;
}

void BloomETS::SetScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
    if (bloom_) {
        bloom_->ScaleFactor()->SetValue(scaleFactor_);
    }
}

BloomETS::Type BloomETS::GetType()
{
    if (bloom_) {
        type_ = FromInternalType(bloom_->Type()->GetValue());
    }
    return type_;
}

void BloomETS::SetType(const BloomETS::Type &type)
{
    type_ = (BloomETS::Type)type;
    if (bloom_) {
        bloom_->Type()->SetValue(ToInternalType(type_));
    }
}

BloomETS::Quality BloomETS::GetQuality()
{
    if (bloom_) {
        quality_ = FromInternalQuality(bloom_->Quality()->GetValue());
    }
    return quality_;
}

void BloomETS::SetQuality(const BloomETS::Quality &quality)
{
    quality_ = (BloomETS::Quality)quality;
    if (bloom_) {
        bloom_->Quality()->SetValue(ToInternalQuality(quality_));
    }
}

bool BloomETS::IsEnabled()
{
    if (bloom_) {
        return bloom_->Enabled()->GetValue();
    }
    return false;
}

void BloomETS::SetEnabled(const bool enable)
{
    if (bloom_) {
        bloom_->Enabled()->SetValue(enable);
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
            return BloomETS::Type::NORMAL;
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