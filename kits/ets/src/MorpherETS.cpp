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

#include "MorpherETS.h"

namespace OHOS::Render3D {
MorpherETS::MorpherETS(const SCENE_NS::IMorpher::Ptr morpher) : morpher_(morpher)
{
}

MorpherETS::~MorpherETS()
{
    morpher_.reset();
}

float MorpherETS::Get(const std::string &name)
{
    if (!morpher_) {
        return 0.0F;
    }
    auto index = morpher_->MorphNames()->FindFirstValueOf(name.c_str());
    if (index == -1) {
        CORE_LOG_E("Can not find morph weight, name: %s", name.c_str());
        return 0.0F;
    }
    return morpher_->MorphWeights()->GetValueAt(index);
}

void MorpherETS::Set(const std::string &name, const float weight)
{
    if (!morpher_) {
        return;
    }
    auto index = morpher_->MorphNames()->FindFirstValueOf(name.c_str());
    if (index == -1) {
        CORE_LOG_E("Can not find morph weight, name: %s", name.c_str());
        return;
    }
    morpher_->MorphWeights()->SetValueAt(index, weight);
}

std::vector<std::string> MorpherETS::GetMorpherNames() const
{
    if (!morpher_) {
        return {};
    }
    BASE_NS::vector<BASE_NS::string> names = morpher_->MorphNames()->GetValue();
    std::vector<std::string> ret;
    ret.reserve(names.size());
    for (const auto& name : names) {
        ret.emplace_back(name.c_str());
    }
    return ret;
}

int32_t MorpherETS::GetWeightsSize() const
{
    if (!morpher_) {
        return 0;
    }
    return morpher_->MorphWeights()->GetSize();
}
}  // namespace OHOS::Render3D