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

#include "MorpherImpl.h"

namespace OHOS::Render3D::KITETS {
MorpherImpl::MorpherImpl(const std::shared_ptr<MorpherETS> morpherETS) : morpherETS_(morpherETS)
{
}

MorpherImpl::~MorpherImpl()
{
    if (morpherETS_) {
        morpherETS_.reset();
    }
}

int32_t MorpherImpl::getTargetsSize()
{
    return morpherETS_ ? morpherETS_->GetWeightsSize() : 0;
}

::taihe::array<::taihe::string> MorpherImpl::getTargetsKeys()
{
    if (morpherETS_) {
        std::vector<std::string> targetsKeys = morpherETS_->GetMorpherNames();
        std::vector<::taihe::string> keys;
        keys.reserve(targetsKeys.size());
        for (size_t i = 0; i < targetsKeys.size(); ++i) {
            keys.emplace_back(::taihe::string(targetsKeys[i]));
        }
        return ::taihe::array_view<::taihe::string>(keys);
    }
    return {};
}

::taihe::optional<double> MorpherImpl::getTarget(::taihe::string_view key)
{
    if (morpherETS_) {
        return ::taihe::optional<double>(std::in_place, morpherETS_->Get(std::string(key)));
    } else {
        return ::taihe::optional<double>(std::in_place, 0);
    }
}

void MorpherImpl::setTarget(::taihe::string_view key, double const &value)
{
    if (morpherETS_) {
        morpherETS_->Set(std::string(key), value);
    }
}
}  // namespace OHOS::Render3D::KITETS