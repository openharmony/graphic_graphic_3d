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

#ifndef OHOS_3D_MESH_RESOURCE_IMPL_H
#define OHOS_3D_MESH_RESOURCE_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "SceneTypes.proj.hpp"
#include "SceneTypes.impl.hpp"

#include "SceneResourceImpl.h"
#include "MeshResourceETS.h"

#include "geometry_definition/CubeETS.h"
#include "geometry_definition/CustomETS.h"
#include "geometry_definition/PlaneETS.h"
#include "geometry_definition/SphereETS.h"

namespace OHOS::Render3D::KITETS {
class MeshResourceImpl : public SceneResourceImpl {
    friend class SceneResourceFactoryImpl;

public:
    static SceneResources::MeshResource Create(
        SceneTH::SceneResourceParameters const &params, ::SceneTypes::GeometryDefinitionType const &geometry);

    explicit MeshResourceImpl(const std::shared_ptr<MeshResourceETS> mrETS);
    ~MeshResourceImpl();

    int64_t GetImpl()
    {
        return reinterpret_cast<int64_t>(this);
    }

private:
    static BASE_NS::unique_ptr<Geometry::CustomETS> MakeCustomETS(
        const SceneTypes::CustomGeometry &customGeometry);

    std::shared_ptr<MeshResourceETS> mrETS_{nullptr};
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_MESH_RESOURCE_IMPL_H