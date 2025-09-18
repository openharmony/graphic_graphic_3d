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

#include "SamplerETS.h"

namespace OHOS::Render3D {
SamplerETS::SamplerETS()
{
    magFilter_ = DEFAULT_FILTER;
    minFilter_ = DEFAULT_FILTER;
    mipMapMode_ = DEFAULT_FILTER;
    addressModeU_ = DEFAULT_ADDESS_MODE;
    addressModeV_ = DEFAULT_ADDESS_MODE;
}

SamplerETS::SamplerETS(const SCENE_NS::ISampler::Ptr sampler) : sampler_(sampler)
{
}

SamplerETS::~SamplerETS()
{
    sampler_.reset();
}

SCENE_NS::SamplerFilter SamplerETS::GetMagFilter()
{
    if (sampler_) {
        return META_NS::GetValue(sampler_->MagFilter());
    }
    return magFilter_;
}

void SamplerETS::SetMagFilter(const SCENE_NS::SamplerFilter filter)
{
    if (sampler_) {
        META_NS::SetValue(sampler_->MagFilter(), filter);
    }
    magFilter_ = filter;
}

SCENE_NS::SamplerFilter SamplerETS::GetMinFilter()
{
    if (sampler_) {
        return META_NS::GetValue(sampler_->MinFilter());
    }
    return minFilter_;
}

void SamplerETS::SetMinFilter(const SCENE_NS::SamplerFilter filter)
{
    if (sampler_) {
        META_NS::SetValue(sampler_->MinFilter(), filter);
    }
    minFilter_ = filter;
}

SCENE_NS::SamplerFilter SamplerETS::GetMipMapMode()
{
    if (sampler_) {
        return META_NS::GetValue(sampler_->MipMapMode());
    }
    return mipMapMode_;
}

void SamplerETS::SetMipMapMode(const SCENE_NS::SamplerFilter filter)
{
    if (sampler_) {
        META_NS::SetValue(sampler_->MipMapMode(), filter);
    }
    mipMapMode_ = filter;
}

SCENE_NS::SamplerAddressMode SamplerETS::GetAddressModeU()
{
    if (sampler_) {
        return META_NS::GetValue(sampler_->AddressModeU());
    }
    return addressModeU_;
}

void SamplerETS::SetAddressModeU(const SCENE_NS::SamplerAddressMode mode)
{
    if (sampler_) {
        META_NS::SetValue(sampler_->AddressModeU(), mode);
    }
    addressModeU_ = mode;
}

SCENE_NS::SamplerAddressMode SamplerETS::GetAddressModeV()
{
    if (sampler_) {
        return META_NS::GetValue(sampler_->AddressModeV());
    }
    return addressModeV_;
}

void SamplerETS::SetAddressModeV(const SCENE_NS::SamplerAddressMode mode)
{
    if (sampler_) {
        META_NS::SetValue(sampler_->AddressModeV(), mode);
    }
    addressModeV_ = mode;
}
}  // namespace OHOS::Render3D