/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef GEOMETRY_DEFINITION_GEOMETRY_DEFINITION_H
#define GEOMETRY_DEFINITION_GEOMETRY_DEFINITION_H

#include <napi_api.h>
#include <scene/interface/intf_create_mesh.h>

#include <base/containers/vector.h>

namespace GeometryDefinition {

enum GeometryType {
    CUSTOM = 0,
    CUBE = 1,
    PLANE = 2,
    SPHERE = 3,
    CYLINDER = 4,
};
void RegisterEnums(NapiApi::Object exports);

class GeometryDefinition {
public:
    virtual ~GeometryDefinition() = default;
    static BASE_NS::unique_ptr<GeometryDefinition> FromJs(NapiApi::Object jsDefinition);
    virtual SCENE_NS::IMesh::Ptr CreateMesh(
        const SCENE_NS::ICreateMesh::Ptr& creator, const SCENE_NS::MeshConfig& config) const = 0;

protected:
    GeometryDefinition();
    static void DefineClass(napi_env env, napi_value exports, BASE_NS::string_view name,
        BASE_NS::vector<napi_property_descriptor> props, napi_callback ctor);

private:
};

} // namespace GeometryDefinition

#endif
