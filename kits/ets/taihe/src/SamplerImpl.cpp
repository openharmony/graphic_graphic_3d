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

#include "SamplerImpl.h"

namespace OHOS::Render3D::KITETS {
::SceneResources::SamplerFilter FromNativeFilter(const SCENE_NS::SamplerFilter &filter)
{
    switch (filter) {
        case SCENE_NS::SamplerFilter::LINEAR:
            return ::SceneResources::SamplerFilter::key_t::LINEAR;
        default:
            return ::SceneResources::SamplerFilter::key_t::NEAREST;
    }
}

SCENE_NS::SamplerFilter ToNativeFilter(const ::SceneResources::SamplerFilter &filter)
{
    switch (filter.get_key()) {
        case ::SceneResources::SamplerFilter::key_t::LINEAR:
            return SCENE_NS::SamplerFilter::LINEAR;
        default:
            return SCENE_NS::SamplerFilter::NEAREST;
    }
}

::SceneResources::SamplerAddressMode FromNativeAddressMode(const SCENE_NS::SamplerAddressMode &mode)
{
    switch (mode) {
        case SCENE_NS::SamplerAddressMode::MIRRORED_REPEAT:
            return ::SceneResources::SamplerAddressMode::key_t::MIRRORED_REPEAT;
        case SCENE_NS::SamplerAddressMode::CLAMP_TO_EDGE:
            return ::SceneResources::SamplerAddressMode::key_t::CLAMP_TO_EDGE;
        default:
            return ::SceneResources::SamplerAddressMode::key_t::REPEAT;
    }
}

SCENE_NS::SamplerAddressMode ToNativeAddressMode(const ::SceneResources::SamplerAddressMode &mode)
{
    switch (mode.get_key()) {
        case ::SceneResources::SamplerAddressMode::key_t::MIRRORED_REPEAT:
            return SCENE_NS::SamplerAddressMode::MIRRORED_REPEAT;
        case ::SceneResources::SamplerAddressMode::key_t::CLAMP_TO_EDGE:
            return SCENE_NS::SamplerAddressMode::CLAMP_TO_EDGE;
        default:
            return SCENE_NS::SamplerAddressMode::REPEAT;
    }
}

SamplerImpl::SamplerImpl(const std::shared_ptr<SamplerETS> &sampler) : sampler_(sampler)
{
}

SamplerImpl::~SamplerImpl()
{
    if (sampler_) {
        sampler_.reset();
    }
}

::taihe::optional<::SceneResources::SamplerFilter> SamplerImpl::getMagFilter()
{
    if (!sampler_) {
        return std::nullopt;
    }
    const auto filter = FromNativeFilter(sampler_->GetMagFilter());
    return ::taihe::optional<::SceneResources::SamplerFilter>(std::in_place, filter);
}

void SamplerImpl::setMagFilter(::taihe::optional_view<::SceneResources::SamplerFilter> filter)
{
    auto filterV = filter.has_value() ? filter.value() : ::SceneResources::SamplerFilter::key_t::NEAREST;
    if (sampler_) {
        sampler_->SetMagFilter(ToNativeFilter(filterV));
    }
}

::taihe::optional<::SceneResources::SamplerFilter> SamplerImpl::getMinFilter()
{
    if (!sampler_) {
        return std::nullopt;
    }
    const auto filter = FromNativeFilter(sampler_->GetMinFilter());
    return ::taihe::optional<::SceneResources::SamplerFilter>(std::in_place, filter);
}

void SamplerImpl::setMinFilter(::taihe::optional_view<::SceneResources::SamplerFilter> filter)
{
    auto filterV = filter.has_value() ? filter.value() : ::SceneResources::SamplerFilter::key_t::NEAREST;
    if (sampler_) {
        sampler_->SetMinFilter(ToNativeFilter(filterV));
    }
}

::taihe::optional<::SceneResources::SamplerFilter> SamplerImpl::getMipMapMode()
{
    if (!sampler_) {
        return std::nullopt;
    }
    const auto filter = FromNativeFilter(sampler_->GetMipMapMode());
    return ::taihe::optional<::SceneResources::SamplerFilter>(std::in_place, filter);
}

void SamplerImpl::setMipMapMode(::taihe::optional_view<::SceneResources::SamplerFilter> filter)
{
    auto filterV = filter.has_value() ? filter.value() : ::SceneResources::SamplerFilter::key_t::NEAREST;
    if (sampler_) {
        sampler_->SetMipMapMode(ToNativeFilter(filterV));
    }
}

::taihe::optional<::SceneResources::SamplerAddressMode> SamplerImpl::getAddressModeU()
{
    if (!sampler_) {
        return std::nullopt;
    }
    const auto mode = FromNativeAddressMode(sampler_->GetAddressModeU());
    return ::taihe::optional<::SceneResources::SamplerAddressMode>(std::in_place, mode);
}

void SamplerImpl::setAddressModeU(::taihe::optional_view<::SceneResources::SamplerAddressMode> mode)
{
    auto modeV = mode.has_value() ? mode.value() : ::SceneResources::SamplerAddressMode::key_t::REPEAT;
    if (sampler_) {
        sampler_->SetAddressModeU(ToNativeAddressMode(modeV));
    }
}

::taihe::optional<::SceneResources::SamplerAddressMode> SamplerImpl::getAddressModeV()
{
    if (!sampler_) {
        return std::nullopt;
    }
    const auto mode = FromNativeAddressMode(sampler_->GetAddressModeV());
    return ::taihe::optional<::SceneResources::SamplerAddressMode>(std::in_place, mode);
}

void SamplerImpl::setAddressModeV(::taihe::optional_view<::SceneResources::SamplerAddressMode> mode)
{
    auto modeV = mode.has_value() ? mode.value() : ::SceneResources::SamplerAddressMode::key_t::REPEAT;
    if (sampler_) {
        sampler_->SetAddressModeV(ToNativeAddressMode(modeV));
    }
}
}  // namespace OHOS::Render3D::KITETS