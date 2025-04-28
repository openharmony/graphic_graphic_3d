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

#include "geometry_definition/CustomJS.h"

#include <ColorProxy.h>
#include <Vec2Proxy.h>
#include <Vec3Proxy.h>
#include <napi_api.h>
#include <scene/interface/intf_create_mesh.h>

namespace GeometryDefinition {

CustomJS::CustomJS() : GeometryDefinition() {}

void CustomJS::Init(napi_env env, napi_value exports)
{
    auto ctor = [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value self;
        napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr);
        auto selfObj = NapiApi::Object { env, self };
        for (auto&& propertyName : { "vertices", "indices", "normals", "uvs", "colors" }) {
            napi_value array;
            napi_create_array(env, &array);
            const auto arrayObj = NapiApi::Object(env, array);
            selfObj.Set(propertyName, arrayObj);
        }
        return self;
    };

    auto getType = [](napi_env e, napi_callback_info) { return NapiApi::Env { e }.GetNumber(GeometryType::CUSTOM); };
    napi_value topology;
    napi_create_uint32(env, PrimitiveTopology::TRIANGLE_LIST, &topology);
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    const auto props = BASE_NS::vector<napi_property_descriptor> {
        // clang-format off
        { "geometryType", nullptr, nullptr, getType, nullptr, nullptr,   napi_default_jsproperty, nullptr },
        { "topology",     nullptr, nullptr, nullptr, nullptr, topology,  napi_default_jsproperty, nullptr },
        { "vertices",     nullptr, nullptr, nullptr, nullptr, undefined, napi_default_jsproperty, nullptr },
        { "indices",      nullptr, nullptr, nullptr, nullptr, undefined, napi_default_jsproperty, nullptr },
        { "normals",      nullptr, nullptr, nullptr, nullptr, undefined, napi_default_jsproperty, nullptr },
        { "uvs",          nullptr, nullptr, nullptr, nullptr, undefined, napi_default_jsproperty, nullptr },
        { "colors",       nullptr, nullptr, nullptr, nullptr, undefined, napi_default_jsproperty, nullptr },
        // clang-format on
    };
    DefineClass(env, exports, "CustomGeometry", props, ctor);
}

void CustomJS::RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object PrimitiveTopology(exports.GetEnv());

#define DECL_ENUM(enu, x)                                           \
    {                                                               \
        napi_create_uint32(enu.GetEnv(), PrimitiveTopology::x, &v); \
        enu.Set(#x, v);                                             \
    }
    DECL_ENUM(PrimitiveTopology, TRIANGLE_LIST);
    DECL_ENUM(PrimitiveTopology, TRIANGLE_STRIP);
#undef DECL_ENUM

    exports.Set("PrimitiveTopology", PrimitiveTopology);
}

GeometryDefinition* CustomJS::FromJs(NapiApi::Object jsDefinition)
{
    auto newObj = new CustomJS();
    if (newObj->SetTopology(jsDefinition)
        // clang-format off
        && ArrayToNative(jsDefinition, "vertices", newObj->vertices_)
        && ArrayToNative(jsDefinition, "indices",  newObj->indices_)
        && ArrayToNative(jsDefinition, "normals",  newObj->normals_)
        && ArrayToNative(jsDefinition, "uvs",      newObj->uvs_)
        && ArrayToNative(jsDefinition, "colors",   newObj->colors_)
        // clang-format on
    ) {
        return newObj;
    } else {
        delete newObj;
        LOG_E("Unable to create CustomJS: Invalid JS object given");
    }
    return {};
}

SCENE_NS::IMesh::Ptr CustomJS::CreateMesh(
    const SCENE_NS::ICreateMesh::Ptr& creator, const SCENE_NS::MeshConfig& config) const
{
    const auto meshData = SCENE_NS::CustomMeshData { GetTopology(), vertices_, indices_, normals_, uvs_, colors_ };
    return creator->Create(config, meshData).GetResult();
}

SCENE_NS::PrimitiveTopology CustomJS::GetTopology() const
{
    if (topology_ == PrimitiveTopology::TRIANGLE_LIST) {
        return SCENE_NS::PrimitiveTopology::TRIANGLE_LIST;
    } else {
        return SCENE_NS::PrimitiveTopology::TRIANGLE_STRIP;
    }
}

bool CustomJS::SetTopology(NapiApi::Object& jsDefinition)
{
    if (auto value = jsDefinition.Get<uint32_t>("topology"); value.IsValid()) {
        uint32_t newTopology = value;
        if (newTopology <= PrimitiveTopology::TRIANGLE_STRIP) {
            topology_ = static_cast<PrimitiveTopology>(newTopology);
            return true;
        }
    }
    LOG_E("Invalid primitive topology given to CustomGeometry");
    return false;
}

template<typename ItemType>
bool CustomJS::ArrayToNative(NapiApi::Object& obj, const BASE_NS::string& arrayName, BASE_NS::vector<ItemType>& target)
{
    NapiApi::Array jsArray = obj.Get<NapiApi::Array>(arrayName);
    auto newItems = BASE_NS::vector<ItemType> {};
    const auto length = jsArray.Count();
    newItems.reserve(length);
    for (auto i = 0; i < length; i++) {
        const auto jsItem = jsArray.Get_value(i);
        if (!jsItem) {
            LOG_E("Invalid array given to CustomGeometry");
            return false;
        }
        auto conversionOk { false };
        newItems.emplace_back(ToNative<ItemType>(obj.GetEnv(), jsItem, conversionOk));
        if (!conversionOk) {
            LOG_E("Invalid array given to CustomGeometry");
            return false;
        }
    }
    target.swap(newItems);
    return true;
}

template<>
uint32_t CustomJS::ToNative(napi_env env, napi_value jsItem, bool& conversionOk)
{
    auto tmpSigned = int64_t {};
    conversionOk = (napi_get_value_int64(env, jsItem, &tmpSigned) == napi_ok);
    conversionOk &= 0 <= tmpSigned && tmpSigned <= UINT32_MAX;
    return static_cast<uint32_t>(tmpSigned);
}

template<>
BASE_NS::Math::Vec2 CustomJS::ToNative(napi_env env, napi_value jsItem, bool& conversionOk)
{
    return Vec2Proxy::ToNative(NapiApi::Object { env, jsItem }, conversionOk);
}

template<>
BASE_NS::Math::Vec3 CustomJS::ToNative(napi_env env, napi_value jsItem, bool& conversionOk)
{
    return Vec3Proxy::ToNative(NapiApi::Object { env, jsItem }, conversionOk);
}

template<>
BASE_NS::Color CustomJS::ToNative(napi_env env, napi_value jsItem, bool& conversionOk)
{
    return ColorProxy::ToNative(NapiApi::Object { env, jsItem }, conversionOk);
}

} // namespace GeometryDefinition
