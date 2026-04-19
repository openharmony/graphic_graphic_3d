/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "ShadowConfigurationImpl.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D::KITETS {
PCFConfigImpl::PCFConfigImpl(const std::shared_ptr<PCFConfigETS> pcfConfigETS) : pcfConfigETS_(pcfConfigETS) {}

PCFConfigImpl::~PCFConfigImpl()
{
    if (pcfConfigETS_) {
        pcfConfigETS_.reset();
    }
}

::taihe::optional<double> PCFConfigImpl::getShadowSampleRadius()
{
    if (!pcfConfigETS_ || isRadiusUndefined_) {
        return ::taihe::optional<double>(std::nullopt);
    } else {
        float radius = pcfConfigETS_->GetRadius();
        return ::taihe::optional<double>(std::in_place, radius);
    }
}

void PCFConfigImpl::setShadowSampleRadius(::taihe::optional<double> shadowSampleRadius)
{
    if (!pcfConfigETS_) {
        WIDGET_LOGE("no pcfConfigETS_ return it");
        return;
    }
    if (shadowSampleRadius) {
        double radius = shadowSampleRadius.value();
        pcfConfigETS_->SetRadius(static_cast<float>(radius));
        isRadiusUndefined_ = false;
    } else {
        WIDGET_LOGE("Undefined shadow sample radius given, lume engine back to default value, js value as undefined.");
        pcfConfigETS_->SetDefaultShadowSampleRadius();
        isRadiusUndefined_ = true;
    }
}

::taihe::optional<int32_t> PCFConfigImpl::getShadowSampleCount()
{
    if (!pcfConfigETS_ || isCountUndefined_) {
        return ::taihe::optional<int32_t>(std::nullopt);
    } else {
        int32_t count = pcfConfigETS_->GetCount();
        return ::taihe::optional<int32_t>(std::in_place, count);
    }
}

void PCFConfigImpl::setShadowSampleCount(::taihe::optional<int32_t> shadowSampleCount)
{
    if (!pcfConfigETS_) {
        WIDGET_LOGE("no pcfConfigETS_ return it");
        return;
    }
    if (shadowSampleCount) {
        int32_t count = shadowSampleCount.value();
        pcfConfigETS_->SetCount(count);
        isCountUndefined_ = false;
    } else {
        WIDGET_LOGE("Undefined shadow sample count given, lume engine back to default value, js value as undefined.");
        pcfConfigETS_->SetDefaultShadowSampleCount();
        isCountUndefined_ = true;
    }
}

::SceneTypes::ShadowAlgorithmType PCFConfigImpl::getShadowAlgorithmType()
{
    return ::SceneTypes::ShadowAlgorithmType::key_t::PCF;
}

int64_t PCFConfigImpl::getPCFConfigImpl()
{
    return reinterpret_cast<uintptr_t>(this);
}

}  // namespace OHOS::Render3D::KITETS