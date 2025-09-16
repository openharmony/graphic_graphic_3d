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

#include "geometry_definition/CubeETS.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D::Geometry {

CubeETS::CubeETS(const BASE_NS::Math::Vec3 &size) : GeometryDefinition(), size_(size)
{
    WIDGET_LOGD("CubeETS +++, size(%{public}f, %{public}f, %{public}f)", size.x, size.y, size.z);
}

GeometryType CubeETS::GetType()
{
    return GeometryType::CUBE;
}

SCENE_NS::IMesh::Ptr CubeETS::CreateMesh(
    const SCENE_NS::ICreateMesh::Ptr &creator, const SCENE_NS::MeshConfig &config) const
{
    return creator->CreateCube(config, size_.x, size_.y, size_.z).GetResult();
}

}  // namespace OHOS::Render3D::Geometry