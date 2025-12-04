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

#include "PBRMaterialImpl.h"

namespace OHOS::Render3D::KITETS {
PBRMaterialImpl::PBRMaterialImpl(const std::shared_ptr<MaterialETS> mat)
    : MaterialImpl(mat), MetallicRoughnessMaterialImpl(mat), ShaderMaterialImpl(mat),
    UnlitMaterialImpl(mat), UnlitShadowAlphaImpl(mat), materialETS_(mat)
{}

PBRMaterialImpl::~PBRMaterialImpl()
{
    if (materialETS_) {
        materialETS_.reset();
    }
}
}  // namespace OHOS::Render3D::KITETS