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

#include "geometry_definition/CubeJS.h"

#include <Vec3Proxy.h>
#include <napi_api.h>

namespace GeometryDefinition {

CubeJS::CubeJS(const BASE_NS::Math::Vec3& size) : GeometryDefinition(), size_(size) {}

void CubeJS::Init(napi_env env, napi_value exports)
{
    auto ctor = [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value thisVar = nullptr;
        napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
        NapiApi::Object { env, thisVar }.Set("size", NapiApi::Object(env, "Vec3", {}));
        return thisVar;
    };
    auto getType = [](napi_env e, napi_callback_info) { return NapiApi::Env { e }.GetNumber(GeometryType::CUBE); };

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    const auto props = BASE_NS::vector<napi_property_descriptor> {
        // clang-format off
        { "geometryType", nullptr, nullptr, getType, nullptr, nullptr,   napi_default_jsproperty, nullptr },
        { "size",         nullptr, nullptr, nullptr, nullptr, undefined, napi_default_jsproperty, nullptr },
        // clang-format on
    };
    DefineClass(env, exports, "CubeGeometry", props, ctor);
}

GeometryDefinition* CubeJS::FromJs(NapiApi::Object& jsDefinition)
{
    if (NapiApi::Object jsSize = jsDefinition.Get<NapiApi::Object>("size")) {
        bool conversionOk = false;
        const auto size = Vec3Proxy::ToNative(jsSize, conversionOk);
        if (conversionOk) {
            return new CubeJS(size);
        }
    }
    LOG_E("Unable to create CubeJS: Invalid JS object given");
    return {};
}

SCENE_NS::IMesh::Ptr CubeJS::CreateMesh(
    const SCENE_NS::ICreateMesh::Ptr& creator, const SCENE_NS::MeshConfig& config) const
{
    return creator->CreateCube(config, size_.x, size_.y, size_.z).GetResult();
}

} // namespace GeometryDefinition
