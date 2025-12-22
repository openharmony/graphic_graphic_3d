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

#include "GeometryImpl.h"

namespace OHOS::Render3D::KITETS {
GeometryImpl::GeometryImpl(const std::shared_ptr<GeometryETS> geometryETS)
    : NodeImpl(geometryETS), geometryETS_(geometryETS)
{}

GeometryImpl::~GeometryImpl()
{
    if (geometryETS_) {
        geometryETS_.reset();
    }
}

::SceneResources::Mesh GeometryImpl::getMesh()
{
    if (geometryETS_) {
        return taihe::make_holder<MeshImpl, SceneResources::Mesh>(geometryETS_->GetMesh());
    } else {
        return SceneResources::Mesh({nullptr, nullptr});
    }
}

::taihe::optional<::SceneResources::Morpher> GeometryImpl::getMorpher()
{
    if (geometryETS_) {
        SceneResources::Morpher m =
            taihe::make_holder<MorpherImpl, SceneResources::Morpher>(geometryETS_->GetMorpher());
        return taihe::optional<SceneResources::Morpher>(std::in_place, m);
    } else {
        return std::nullopt;
    }
}
}  // namespace OHOS::Render3D::KITETS