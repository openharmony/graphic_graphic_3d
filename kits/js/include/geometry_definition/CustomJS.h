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

#ifndef GEOMETRY_DEFINITION_CUSTOM_JS_H
#define GEOMETRY_DEFINITION_CUSTOM_JS_H

#include <napi_api.h>
#include <scene/interface/intf_create_mesh.h>

#include <base/math/vector.h>

#include "geometry_definition/GeometryDefinition.h"

namespace GeometryDefinition {

class CustomJS : public GeometryDefinition {
public:
    enum PrimitiveTopology {
        TRIANGLE_LIST = 0,
        TRIANGLE_STRIP = 1,
    };

    ~CustomJS() override = default;
    static GeometryDefinition* FromJs(NapiApi::Object jsDefinition);
    virtual SCENE_NS::IMesh::Ptr CreateMesh(
        const SCENE_NS::ICreateMesh::Ptr& creator, const SCENE_NS::MeshConfig& config) const override;

    static void Init(napi_env env, napi_value exports);
    static void RegisterEnums(NapiApi::Object exports);

private:
    explicit CustomJS();

    // Note: This converts to the internal type. The underlying integer values don't match.
    SCENE_NS::PrimitiveTopology GetTopology() const;

    bool SetTopology(NapiApi::Object& jsDefinition);
    PrimitiveTopology topology_ { PrimitiveTopology::TRIANGLE_LIST };
    BASE_NS::vector<BASE_NS::Math::Vec3> vertices_;
    BASE_NS::vector<uint32_t> indices_;
    BASE_NS::vector<BASE_NS::Math::Vec3> normals_;
    BASE_NS::vector<BASE_NS::Math::Vec2> uvs_;
    BASE_NS::vector<BASE_NS::Color> colors_;

    template<typename ItemType>
    static bool ArrayToNative(
        NapiApi::Object& obj, const BASE_NS::string& arrayName, BASE_NS::vector<ItemType>& target);
    template<typename ItemType>
    static ItemType ToNative(napi_env env, napi_value jsItem, bool& conversionOk);
};

} // namespace GeometryDefinition

#endif
