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

#ifndef SUB_MESH_ETS_H
#define SUB_MESH_ETS_H

#include <meta/interface/intf_object.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_material.h>

class SubMeshETS {
public:
    SubMeshETS(const SCENE_NS::ISubMesh::Ptr subMesh);
    ~SubMeshETS();

    BASE_NS::string GetName();
    void SetName(const BASE_NS::string &name);
    BASE_NS::Math::Vec3 GetAABBMin();
    BASE_NS::Math::Vec3 GetAABBMax();

    SCENE_NS::IMaterial::Ptr GetMaterial();
    void SetMaterial(const SCENE_NS::IMaterial::Ptr &material);

private:
    SCENE_NS::ISubMesh::Ptr subMesh_{nullptr};
};
#endif  // SUB_MESH_ETS_H