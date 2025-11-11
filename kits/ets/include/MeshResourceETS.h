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

#ifndef OHOS_3D_MESH_RESOURCE_ETS_H
#define OHOS_3D_MESH_RESOURCE_ETS_H

#include <string>

#include "geometry_definition/GeometryDefinition.h"
#include <scene/interface/intf_scene.h>
#include "SceneResourceETS.h"

namespace OHOS::Render3D {
class MeshResourceETS : public SceneResourceETS {
public:
    MeshResourceETS(const std::string &name, const std::string &uri,
        BASE_NS::unique_ptr<Geometry::GeometryDefinition> geometryDefinition);

    ~MeshResourceETS() override;

    void Destroy() override;

    META_NS::IObject::Ptr GetNativeObj() const override;

    SCENE_NS::IMesh::Ptr CreateMesh(const SCENE_NS::IScene::Ptr &scene) const;

private:
    // This is a temporary solution. When IMeshResource is implemented, this can be removed.
    BASE_NS::unique_ptr<Geometry::GeometryDefinition> geometryDefinition_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_MESH_RESOURCE_ETS_H
