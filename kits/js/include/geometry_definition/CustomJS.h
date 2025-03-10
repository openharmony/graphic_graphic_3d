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

#include <meta/interface/intf_object.h>
#include <napi_api.h>
#include <scene/interface/intf_create_mesh.h>

#include <base/math/vector.h>

#include "BaseObjectJS.h"
#include "geometry_definition/GeometryDefinition.h"

namespace GeometryDefinition {

class CustomJS : public GeometryDefinition<CustomJS> {
public:
    static constexpr uint32_t ID = 150;
    enum PrimitiveTopology {
        TRIANGLE_LIST = 0,
        TRIANGLE_STRIP = 1,
    };

    explicit CustomJS(napi_env, napi_callback_info);
    ~CustomJS() override = default;

    static void Init(napi_env env, napi_value exports);
    static void RegisterEnums(NapiApi::Object exports);

    virtual void* GetInstanceImpl(uint32_t id) override;

    SCENE_NS::CustomMeshData ToNative() const;

private:
    // Note: This converts to the internal type. The underlying integer values don't match
    SCENE_NS::PrimitiveTopology GetTopology() const;
    napi_value GetTopology(NapiApi::FunctionContext<>& ctx);
    napi_value GetVertices(NapiApi::FunctionContext<>& ctx);
    napi_value GetIndices(NapiApi::FunctionContext<>& ctx);
    napi_value GetNormals(NapiApi::FunctionContext<>& ctx);
    napi_value GetUvs(NapiApi::FunctionContext<>& ctx);
    napi_value GetColors(NapiApi::FunctionContext<>& ctx);

    void SetTopology(NapiApi::FunctionContext<uint32_t>& ctx);
    void SetVertices(NapiApi::FunctionContext<NapiApi::Array>& ctx);
    void SetIndices(NapiApi::FunctionContext<NapiApi::Array>& ctx);
    void SetNormals(NapiApi::FunctionContext<NapiApi::Array>& ctx);
    void SetUvs(NapiApi::FunctionContext<NapiApi::Array>& ctx);
    void SetColors(NapiApi::FunctionContext<NapiApi::Array>& ctx);

    PrimitiveTopology topology_ { PrimitiveTopology::TRIANGLE_LIST };
    BASE_NS::vector<BASE_NS::Math::Vec3> vertices_;
    BASE_NS::vector<uint32_t> indices_;
    BASE_NS::vector<BASE_NS::Math::Vec3> normals_;
    BASE_NS::vector<BASE_NS::Math::Vec2> uvs_;
    BASE_NS::vector<BASE_NS::Color> colors_;

    template<typename ItemType>
    static NapiApi::Array ArrayToJs(NapiApi::FunctionContext<>& ctx, const BASE_NS::vector<ItemType>& source);
    template<typename ItemType>
    static void ArrayToNative(NapiApi::FunctionContext<NapiApi::Array>& ctx, BASE_NS::vector<ItemType>& target);

    template<typename ItemType>
    static ItemType ToNative(napi_env env, napi_value jsItem, bool& conversionOk);
    template<typename ItemType>
    static NapiApi::Object ToJs(NapiApi::FunctionContext<>& ctx, const ItemType& nativeItem);
};

} // namespace GeometryDefinition

#endif
