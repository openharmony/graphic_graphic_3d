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

#include "RenderConfigurationImpl.h"
#include "Vec2Impl.h"
#include "Utils.h"

namespace OHOS::Render3D::KITETS {
RenderConfigurationImpl::RenderConfigurationImpl(const std::shared_ptr<RenderConfigurationETS> rcETS)
    : rcETS_(rcETS)
{
    WIDGET_LOGD("RenderConfigurationImpl ++");
}

RenderConfigurationImpl::~RenderConfigurationImpl()
{
    rcETS_.reset();
}

std::shared_ptr<RenderConfigurationETS> RenderConfigurationImpl::GetRenderConfigurationETS()
{
    return rcETS_;
}

::taihe::optional<::SceneTypes::Vec2> RenderConfigurationImpl::getShadowResolution()
{
    RETURN_IF_NULL_WITH_VALUE(rcETS_, std::nullopt);
    if (resolution_.x == 0 || resolution_.y == 0) {
        return std::nullopt;
    }
    auto res = taihe::make_holder<UVec2Impl, ::SceneTypes::Vec2>(resolution_);
    return ::taihe::optional<::SceneTypes::Vec2>(std::in_place, res);
}

void RenderConfigurationImpl::setShadowResolution(::taihe::optional_view<::SceneTypes::Vec2> resolution)
{
    RETURN_IF_NULL(rcETS_);
    if (resolution.has_value()) {
        auto res = resolution.value();
        if (res->getX() > 0 && res->getY() > 0) {
            auto x = static_cast<uint32_t>(res->getX());
            auto y = static_cast<uint32_t>(res->getY());
            if (x > 0 && y > 0) {
                resolution_ = { x, y };
                rcETS_->SetShadowResolution(resolution_);
            } else {
                WIDGET_LOGE("Invalid resolution value: input resolution should be positive integers");
            }
        } else {
            WIDGET_LOGE("Invalid resolution value: input resolution should be positive integers");
        }
    } else {
        WIDGET_LOGE("Invalid resolution");
    }
}
}  // namespace OHOS::Render3D::KITETS