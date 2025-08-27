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

#include "MaterialPropertyETS.h"

namespace OHOS::Render3D {
MaterialPropertyETS::MaterialPropertyETS(const SCENE_NS::ITexture::Ptr tex) : tex_(tex)
{
}

MaterialPropertyETS::~MaterialPropertyETS()
{
    factorProxy_.reset();
    tex_.reset();
}

std::shared_ptr<ImageETS> MaterialPropertyETS::GetImage()
{
    if (!tex_) {
        CORE_LOG_E("MaterialPropertyETS::GetImage tex_ is null");
        return nullptr;
    }
    return nullptr;
}

void MaterialPropertyETS::SetImage(const std::shared_ptr<ImageETS> img)
{}

std::shared_ptr<Vec4Proxy> MaterialPropertyETS::GetFactor()
{
    if (!tex_) {
        CORE_LOG_E("MaterialPropertyETS::GetFactor tex_ is null");
        return nullptr;
    }
    if (!factorProxy_) {
        factorProxy_ = std::make_shared<Vec4Proxy>(tex_->Factor());
    }
    return factorProxy_;
}

void MaterialPropertyETS::SetFactor(const BASE_NS::Math::Vec4 &factor)
{
    if (!tex_) {
        CORE_LOG_E("MaterialPropertyETS::SetFactor tex_ is null");
        return;
    }
    if (!factorProxy_) {
        factorProxy_ = std::make_shared<Vec4Proxy>(tex_->Factor());
    }
    factorProxy_->SetValue(factor);
}
}  // namespace OHOS::Render3D