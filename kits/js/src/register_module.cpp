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
#include "AnimationJS.h"
#include "CameraJS.h"
#include "EnvironmentJS.h"
#include "GeometryJS.h"
#include "ImageJS.h"
#include "LightJS.h"
#include "MaterialJS.h"
#include "MeshJS.h"
#include "NodeJS.h"
#include "PostProcJS.h"
#include "SceneJS.h"
#include "ShaderJS.h"
#include "SubMeshJS.h"
#include "ToneMapJS.h"

void RegisterClasses(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value zero;
    napi_value one;
    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);

    status = napi_create_double(env, 0.0, &zero);
    status = napi_create_double(env, 1.0, &one);

    // Declare color class
    {
        /// Color
        auto colorCtor = [](napi_env e, napi_callback_info c) -> napi_value { return {}; };

        // clang-format off
        napi_property_descriptor desc4[] = {
            {"r", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"g", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"b", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"a", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr}
        };
        // clang-format on
        napi_value color_class = nullptr;
        status = napi_define_class(
            env, "Color", NAPI_AUTO_LENGTH, colorCtor, nullptr, BASE_NS::countof(desc4), desc4, &color_class);
        mis->StoreCtor("Color", color_class);
    }
    // Declare math classes.. "simply" for now.
    {
        /// Vec
        auto vec3Ctor = [](napi_env e, napi_callback_info c) -> napi_value { return {}; };

        // clang-format off
        napi_property_descriptor desc3[] = {
            {"x", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"y", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"z", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr}
        };
        // clang-format on
        napi_value vec3_class = nullptr;
        status = napi_define_class(
            env, "Vec3", NAPI_AUTO_LENGTH, vec3Ctor, nullptr, BASE_NS::countof(desc3), desc3, &vec3_class);
        mis->StoreCtor("Vec3", vec3_class);

        /// Vec4
        auto vec4Ctor = [](napi_env e, napi_callback_info c) -> napi_value { return {}; };

        // clang-format off
        napi_property_descriptor desc4[] = {
            {"x", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"y", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"z", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"w", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr}
        };
        // clang-format on
        napi_value vec4_class = nullptr;
        status = napi_define_class(
            env, "Vec4", NAPI_AUTO_LENGTH, vec4Ctor, nullptr, BASE_NS::countof(desc4), desc4, &vec4_class);
        mis->StoreCtor("Vec4", vec4_class);

        /// Quaternion
        auto QuatCtor = [](napi_env e, napi_callback_info c) -> napi_value { return {}; };

        // clang-format off
        napi_property_descriptor qdesc[] = {
            {"x", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"y", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"z", nullptr, nullptr, nullptr, nullptr, zero, napi_default_jsproperty, nullptr},
            {"w", nullptr, nullptr, nullptr, nullptr, one, napi_default_jsproperty, nullptr}
        };
        // clang-format on
        napi_value quaternion_class = nullptr;
        status = napi_define_class(
            env, "Quaternion", NAPI_AUTO_LENGTH, QuatCtor, nullptr, BASE_NS::countof(qdesc), qdesc, &quaternion_class);
        mis->StoreCtor("Quaternion", quaternion_class);
    }

    napi_value scene3dNS = exports;
    SceneJS::Init(env, scene3dNS);
    NodeJS::Init(env, scene3dNS);
    CameraJS::Init(env, scene3dNS);
    EnvironmentJS::Init(env, scene3dNS);
    PointLightJS::Init(env, scene3dNS);
    DirectionalLightJS::Init(env, scene3dNS);
    SpotLightJS::Init(env, scene3dNS);
    GeometryJS::Init(env, scene3dNS);
    MeshJS::Init(env, scene3dNS);
    SubMeshJS::Init(env, scene3dNS);
    ShaderMaterialJS::Init(env, scene3dNS);
    ImageJS::Init(env, scene3dNS);
    PostProcJS::Init(env, scene3dNS);
    ToneMapJS::Init(env, scene3dNS);
    ShaderJS::Init(env, scene3dNS);
    AnimationJS::Init(env, scene3dNS);

    BaseLight::RegisterEnums({ env, scene3dNS });
    NodeImpl::RegisterEnums({ env, scene3dNS });
    SceneResourceImpl::RegisterEnums({ env, scene3dNS });
}