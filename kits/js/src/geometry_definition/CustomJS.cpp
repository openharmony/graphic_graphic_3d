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

#include "BaseObjectJS.h"

namespace GeometryDefinition {

CustomJS::CustomJS(napi_env env, napi_callback_info info)
    : GeometryDefinition<CustomJS>(env, info, GeometryType::CUSTOM)
{}

void CustomJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> props;
    using namespace NapiApi;
    GetPropertyDescs(props);
    props.push_back(GetSetProperty<uint32_t, CustomJS, &CustomJS::GetTopology, &CustomJS::SetTopology>("topology"));
    props.push_back(
        GetSetProperty<NapiApi::Array, CustomJS, &CustomJS::GetVertices, &CustomJS::SetVertices>("vertices"));
    props.push_back(GetSetProperty<NapiApi::Array, CustomJS, &CustomJS::GetIndices, &CustomJS::SetIndices>("indices"));
    props.push_back(GetSetProperty<NapiApi::Array, CustomJS, &CustomJS::GetNormals, &CustomJS::SetNormals>("normals"));
    props.push_back(GetSetProperty<NapiApi::Array, CustomJS, &CustomJS::GetUvs, &CustomJS::SetUvs>("uvs"));
    props.push_back(GetSetProperty<NapiApi::Array, CustomJS, &CustomJS::GetColors, &CustomJS::SetColors>("colors"));

    const auto name = "CustomGeometry";
    const auto constructor = BaseObject::ctor<CustomJS>();
    napi_value jsConstructor;
    napi_define_class(env, name, NAPI_AUTO_LENGTH, constructor, nullptr, props.size(), props.data(), &jsConstructor);
    napi_set_named_property(env, exports, name, jsConstructor);

    NapiApi::MyInstanceState* mis {};
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor(name, jsConstructor);
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

void* CustomJS::GetInstanceImpl(uint32_t id)
{
    return (id == CustomJS::ID) ? this : nullptr;
}

SCENE_NS::CustomMeshData CustomJS::ToNative() const
{
    return SCENE_NS::CustomMeshData { GetTopology(), vertices_, indices_, normals_, uvs_, colors_ };
}

SCENE_NS::PrimitiveTopology CustomJS::GetTopology() const
{
    if (topology_ == PrimitiveTopology::TRIANGLE_LIST) {
        return SCENE_NS::PrimitiveTopology::TRIANGLE_LIST;
    } else if (topology_ == PrimitiveTopology::TRIANGLE_STRIP) {
        return SCENE_NS::PrimitiveTopology::TRIANGLE_STRIP;
    }
    LOG_E("Invalid primitive topology set for CustomGeometry");
    return {};
}

napi_value CustomJS::GetTopology(NapiApi::FunctionContext<>& ctx)
{
    return ctx.GetNumber(topology_);
}

napi_value CustomJS::GetVertices(NapiApi::FunctionContext<>& ctx)
{
    return ArrayToJs(ctx, vertices_);
}

napi_value CustomJS::GetIndices(NapiApi::FunctionContext<>& ctx)
{
    NapiApi::Array array(ctx.Env(), indices_.size());
    size_t index = 0;
    for (const auto& nativeItem : indices_) {
        array.Set(index, nativeItem);
        index++;
    }
    return array;
}

napi_value CustomJS::GetNormals(NapiApi::FunctionContext<>& ctx)
{
    return ArrayToJs(ctx, normals_);
}

napi_value CustomJS::GetUvs(NapiApi::FunctionContext<>& ctx)
{
    return ArrayToJs(ctx, uvs_);
}

napi_value CustomJS::GetColors(NapiApi::FunctionContext<>& ctx)
{
    return ArrayToJs(ctx, colors_);
}

void CustomJS::SetTopology(NapiApi::FunctionContext<uint32_t>& ctx)
{
    auto count = uint32_t {};
    if (napi_get_value_uint32(ctx.Env(), ctx.Arg<0>().ToNapiValue(), &count) == napi_ok) {
        topology_ = static_cast<PrimitiveTopology>(count);
    } else {
        LOG_E("Invalid primitive topology given to CustomGeometry");
    }
}

void CustomJS::SetVertices(NapiApi::FunctionContext<NapiApi::Array>& ctx)
{
    ArrayToNative(ctx, vertices_);
}

void CustomJS::SetIndices(NapiApi::FunctionContext<NapiApi::Array>& ctx)
{
    ArrayToNative(ctx, indices_);
}

void CustomJS::SetNormals(NapiApi::FunctionContext<NapiApi::Array>& ctx)
{
    ArrayToNative(ctx, normals_);
}

void CustomJS::SetUvs(NapiApi::FunctionContext<NapiApi::Array>& ctx)
{
    ArrayToNative(ctx, uvs_);
}

void CustomJS::SetColors(NapiApi::FunctionContext<NapiApi::Array>& ctx)
{
    ArrayToNative(ctx, colors_);
}

template<typename ItemType>
NapiApi::Array CustomJS::ArrayToJs(NapiApi::FunctionContext<>& ctx, const BASE_NS::vector<ItemType>& source)
{
    NapiApi::Array array(ctx.Env(), source.size());
    size_t index = 0;
    for (const auto& nativeItem : source) {
        array.Set(index, ToJs(ctx, nativeItem));
        index++;
    }
    return array;
}

template<typename ItemType>
void CustomJS::ArrayToNative(NapiApi::FunctionContext<NapiApi::Array>& ctx, BASE_NS::vector<ItemType>& target)
{
    const auto jsItems = NapiApi::Array { ctx.Arg<0>() };
    auto newItems = BASE_NS::vector<ItemType> {};
    newItems.reserve(jsItems.Count());
    for (auto i = 0; i < jsItems.Count(); i++) {
        const auto jsItem = jsItems.Get_value(i);
        if (!jsItem) {
            LOG_E("Invalid array given to CustomGeometry");
            return;
        }
        auto conversionOk { false };
        const auto item = ToNative<ItemType>(ctx.GetEnv(), jsItem, conversionOk);
        if (!conversionOk) {
            LOG_E("Invalid array given to CustomGeometry");
            return;
        }
        newItems.emplace_back(item);
    }
    target.swap(newItems);
}

template<>
NapiApi::Object CustomJS::ToJs(NapiApi::FunctionContext<>& ctx, const BASE_NS::Math::Vec2& nativeItem)
{
    return Vec2Proxy::ToNapiObject(nativeItem, ctx.Env());
}

template<>
NapiApi::Object CustomJS::ToJs(NapiApi::FunctionContext<>& ctx, const BASE_NS::Math::Vec3& nativeItem)
{
    return Vec3Proxy::ToNapiObject(nativeItem, ctx.Env());
}

template<>
NapiApi::Object CustomJS::ToJs(NapiApi::FunctionContext<>& ctx, const BASE_NS::Color& nativeItem)
{
    return ColorProxy::ToNapiObject(nativeItem, ctx.Env());
}

template<>
uint32_t CustomJS::ToNative(napi_env env, napi_value jsItem, bool& conversionOk)
{
    auto native = uint32_t {};
    conversionOk = (napi_get_value_uint32(env, jsItem, &native) == napi_ok);
    return native;
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
