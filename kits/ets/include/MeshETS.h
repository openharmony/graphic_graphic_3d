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
#ifndef MESH_ETS_H
#define MESH_ETS_H

#include <meta/interface/intf_object.h>
#include <scene/interface/intf_mesh.h>

#include "SubMeshETS.h"
#include "SceneResourceETS.h"

class MeshETS : public SceneResourceETS {
public:
    MeshETS(const SCENE_NS::IMesh::Ptr mesh);
    ~MeshETS();

    META_NS::IObject::Ptr GetNativeObj() const override;

    std::vector<std::shared_ptr<SubMeshETS>> GetSubMeshes();
    BASE_NS::Math::Vec3 GetAABBMin();
    BASE_NS::Math::Vec3 GetAABBMax();

    SCENE_NS::IMaterial::Ptr GetMaterialOverride();
    void SetMaterialOverride(const SCENE_NS::IMaterial::Ptr &material);

private:
    SCENE_NS::IMesh::WeakPtr mesh_{nullptr};
};
#endif  // MESH_ETS_H