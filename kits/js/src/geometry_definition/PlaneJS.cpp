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

#include "geometry_definition/PlaneJS.h"

#include <Vec2Proxy.h>
#include <napi_api.h>

namespace GeometryDefinition {

PlaneJS::PlaneJS(const BASE_NS::Math::Vec2& size) : GeometryDefinition(), size_(size) {}

void PlaneJS::Init(napi_env env, napi_value exports)
{
    auto ctor = [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value thisVar = nullptr;
        napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
        NapiApi::Object { env, thisVar }.Set("size", NapiApi::Object(env, "Vec2", {}));
        return thisVar;
    };
    auto getType = [](napi_env e, napi_callback_info) { return NapiApi::Env { e }.GetNumber(GeometryType::PLANE); };

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    const auto props = BASE_NS::vector<napi_property_descriptor> {
        // clang-format off
        { "geometryType", nullptr, nullptr, getType, nullptr, nullptr,   napi_default_jsproperty, nullptr },
        { "size",         nullptr, nullptr, nullptr, nullptr, undefined, napi_default_jsproperty, nullptr },
        // clang-format on
    };
    DefineClass(env, exports, "PlaneGeometry", props, ctor);
}

GeometryDefinition* PlaneJS::FromJs(NapiApi::Object& jsDefinition)
{
    if (NapiApi::Object jsSize = jsDefinition.Get<NapiApi::Object>("size")) {
        bool conversionOk = false;
        const auto size = Vec2Proxy::ToNative(jsSize, conversionOk);
        if (conversionOk) {
            return new PlaneJS(size);
        }
    }
    LOG_E("Unable to create PlaneJS: Invalid JS object given");
    return {};
}

SCENE_NS::IMesh::Ptr PlaneJS::CreateMesh(
    const SCENE_NS::ICreateMesh::Ptr& creator, const SCENE_NS::MeshConfig& config) const
{
    return creator->CreatePlane(config, size_.x, size_.y).GetResult();
}

} // namespace GeometryDefinition
