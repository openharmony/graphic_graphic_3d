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

#ifndef GEOMETRY_DEFINITION_SPHERE_ETS_H
#define GEOMETRY_DEFINITION_SPHERE_ETS_H

#include <scene/interface/intf_create_mesh.h>

#include "geometry_definition/GeometryDefinition.h"

namespace GeometryDefinition {

class SphereETS : public GeometryDefinition {
public:
    explicit SphereETS(float radius, uint32_t segmentCount);
    ~SphereETS() override = default;
    GeometryType GetType() override;
    virtual SCENE_NS::IMesh::Ptr CreateMesh(
        const SCENE_NS::ICreateMesh::Ptr &creator, const SCENE_NS::MeshConfig &config) const override;

private:
    float radius_;
    uint32_t segmentCount_;
};

}  // namespace GeometryDefinition

#endif  // GEOMETRY_DEFINITION_SPHERE_ETS_H