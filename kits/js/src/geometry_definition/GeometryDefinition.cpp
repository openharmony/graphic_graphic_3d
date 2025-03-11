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

#include <napi_api.h>

namespace GeometryDefinition {

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

} // namespace GeometryDefinition
