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

#ifndef OHOS_3D_MORPHER_IMPL_H
#define OHOS_3D_MORPHER_IMPL_H

#include <stdexcept>
#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"

#include "SceneResources.proj.hpp"
#include "SceneResources.impl.hpp"

#include "MorpherETS.h"

namespace OHOS::Render3D::KITETS {
class MorpherImpl {
public:
    MorpherImpl(const std::shared_ptr<MorpherETS> morpherETS);

    ~MorpherImpl();

    ::taihe::map<::taihe::string, double> getTargets();

private:
    std::shared_ptr<MorpherETS> morpherETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_MORPHER_IMPL_H