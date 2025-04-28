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

#include "geometry_definition/SphereJS.h"

#include <napi_api.h>

namespace GeometryDefinition {

SphereJS::SphereJS(float radius, uint32_t segmentCount)
    : GeometryDefinition(), radius_(radius), segmentCount_(segmentCount)
{}

void SphereJS::Init(napi_env env, napi_value exports)
{
    auto ctor = [](napi_env env, napi_callback_info info) -> napi_value {
        return NapiApi::FunctionContext(env, info).This().ToNapiValue();
    };
    auto getType = [](napi_env e, napi_callback_info) { return NapiApi::Env { e }.GetNumber(GeometryType::SPHERE); };

    napi_value zeroFloat = nullptr;
    napi_value zeroInt = nullptr;
    napi_create_double(env, 0.0, &zeroFloat);
    napi_create_uint32(env, 0, &zeroInt);
    const auto props = BASE_NS::vector<napi_property_descriptor> {
        // clang-format off
        { "geometryType", nullptr, nullptr, getType, nullptr, nullptr,   napi_default_jsproperty, nullptr },
        { "radius",       nullptr, nullptr, nullptr, nullptr, zeroFloat, napi_default_jsproperty, nullptr },
        { "segmentCount", nullptr, nullptr, nullptr, nullptr, zeroInt,   napi_default_jsproperty, nullptr },
        // clang-format on
    };
    DefineClass(env, exports, "SphereGeometry", props, ctor);
}

GeometryDefinition* SphereJS::FromJs(NapiApi::Object& jsDefinition)
{
    if (auto radius = jsDefinition.Get<float>("radius"); radius.IsValid()) {
        // It's fine for radius to be negative. Then we will just have an inside-out sphere.
        // Check valid segmentCount range with a signed type to avoid underflow. Internally we use uint32_t.
        if (auto segmentCount = jsDefinition.Get<int64_t>("segmentCount"); segmentCount.IsValid()) {
            if (segmentCount >= 0) {
                return new SphereJS(radius, segmentCount);
            }
        }
    }
    LOG_E("Unable to create SphereJS: Invalid JS object given");
    return {};
}

SCENE_NS::IMesh::Ptr SphereJS::CreateMesh(
    const SCENE_NS::ICreateMesh::Ptr& creator, const SCENE_NS::MeshConfig& config) const
{
    return creator->CreateSphere(config, radius_, segmentCount_, segmentCount_).GetResult();
}

} // namespace GeometryDefinition
