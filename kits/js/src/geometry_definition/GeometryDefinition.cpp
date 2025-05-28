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

#include "geometry_definition/GeometryDefinition.h"

#include <geometry_definition/CubeJS.h>
#include <geometry_definition/CustomJS.h>
#include <geometry_definition/PlaneJS.h>
#include <geometry_definition/SphereJS.h>
#include <napi_api.h>

namespace GeometryDefinition {

GeometryDefinition::GeometryDefinition() {}

void RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object GeometryType(exports.GetEnv());

#define DECL_ENUM(enu, x)                                      \
    {                                                          \
        napi_create_uint32(enu.GetEnv(), GeometryType::x, &v); \
        enu.Set(#x, v);                                        \
    }
    DECL_ENUM(GeometryType, CUSTOM);
    DECL_ENUM(GeometryType, CUBE);
    DECL_ENUM(GeometryType, PLANE);
    DECL_ENUM(GeometryType, SPHERE);
#undef DECL_ENUM

    exports.Set("GeometryType", GeometryType);
}

BASE_NS::unique_ptr<GeometryDefinition> GeometryDefinition::FromJs(NapiApi::Object jsDefinition)
{
    GeometryDefinition* result = nullptr;
    if (auto value = jsDefinition.Get<uint32_t>("geometryType"); value.IsValid()) {
        uint32_t type = value;
        if (type == GeometryType::CUSTOM) {
            result = CustomJS::FromJs(jsDefinition);
        } else if (type == GeometryType::CUBE) {
            result = CubeJS::FromJs(jsDefinition);
        } else if (type == GeometryType::PLANE) {
            result = PlaneJS::FromJs(jsDefinition);
        } else if (type == GeometryType::SPHERE) {
            result = SphereJS::FromJs(jsDefinition);
        } else {
            LOG_E("Can't create a native geometry definition: unsupported geometryType");
        }
    } else {
        LOG_E("Can't create a native geometry definition: geometryType missing");
    }
    return BASE_NS::unique_ptr<GeometryDefinition> { result };
}

void GeometryDefinition::DefineClass(napi_env env, napi_value exports, BASE_NS::string_view name,
    BASE_NS::vector<napi_property_descriptor> props, napi_callback ctor)
{
    napi_value classCtor = nullptr;
    napi_define_class(env, name.data(), NAPI_AUTO_LENGTH, ctor, nullptr, props.size(), props.data(), &classCtor);
    NapiApi::MyInstanceState* mis {};
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor(name, classCtor);
    }
    NapiApi::Object { env, exports }.Set(name, classCtor);
}

} // namespace GeometryDefinition
