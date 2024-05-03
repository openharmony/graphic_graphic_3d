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
#include "SceneResourceImpl.h"

#include <scene_plugin/interface/intf_mesh.h>
#include <scene_plugin/interface/intf_node.h>

#include "BaseObjectJS.h"
SceneResourceImpl::SceneResourceImpl(SceneResourceType type) : type_(type)
{
    LOG_F("SceneResourceImpl ++");
}
SceneResourceImpl::~SceneResourceImpl()
{
    LOG_F("SceneResourceImpl --");
}

void SceneResourceImpl::RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object SceneResourceType(exports.GetEnv());

#define DECL_ENUM(enu, x)                                     \
    {                                                         \
        napi_create_uint32(enu.GetEnv(), SceneResourceType::x, &v); \
        enu.Set(#x, v);                                 \
    }
    DECL_ENUM(SceneResourceType, UNKNOWN);
    DECL_ENUM(SceneResourceType, NODE);
    DECL_ENUM(SceneResourceType, ENVIRONMENT);
    DECL_ENUM(SceneResourceType, MATERIAL);
    DECL_ENUM(SceneResourceType, MESH);
    DECL_ENUM(SceneResourceType, ANIMATION);
    DECL_ENUM(SceneResourceType, SHADER);
    DECL_ENUM(SceneResourceType, IMAGE);
#undef DECL_ENUM

    exports.Set("SceneResourceType", SceneResourceType);
}

void SceneResourceImpl::GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props)
{
    props.push_back(
        TROGetSetProperty<BASE_NS::string, SceneResourceImpl, &SceneResourceImpl::GetName, &SceneResourceImpl::SetName>(
            "name"));
    props.push_back(TROGetProperty<NapiApi::Object, SceneResourceImpl, &SceneResourceImpl::GetUri>("uri"));
    props.push_back(
        TROGetProperty<BASE_NS::string, SceneResourceImpl, &SceneResourceImpl::GetObjectType>("resourceType"));
    props.push_back(
        MakeTROMethod<NapiApi::FunctionContext<>, SceneResourceImpl, &SceneResourceImpl::Dispose>("destroy"));
}

void* SceneResourceImpl::GetInstanceImpl(uint32_t id)
{
    if (id == SceneResourceImpl::ID) {
        return this;
    }
    return nullptr;
}

napi_value SceneResourceImpl::Dispose(NapiApi::FunctionContext<>& ctx)
{
    // Dispose of the native object. (makes the js object invalid)
    if (TrueRootObject* instance = GetThisRootObject(ctx)) {
        instance->DisposeNative();
    }
    uri_.Reset();
    return ctx.GetUndefined();
}

napi_value SceneResourceImpl::GetObjectType(NapiApi::FunctionContext<>& ctx)
{
    uint32_t type = -1; // return -1 if the resource does not exist anymore
    if (GetThisNativeObject(ctx)) {
        type = type_;
    }
    napi_value value;
    napi_status status = napi_create_uint32(ctx, type, &value);
    return value;
}

napi_value SceneResourceImpl::GetName(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string name;
    if (auto node = interface_pointer_cast<META_NS::INamed>(GetThisNativeObject(ctx))) {
        ExecSyncTask([node, &name]() {
            name = node->Name()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_create_string_utf8(ctx, name.c_str(), name.length(), &value);
    return value;
}
void SceneResourceImpl::SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (auto node = interface_pointer_cast<META_NS::INamed>(GetThisNativeObject(ctx))) {
        BASE_NS::string name = ctx.Arg<0>();
        ExecSyncTask([node, name]() {
            node->Name()->SetValue(name);
            return META_NS::IAny::Ptr {};
        });
    }
}
void SceneResourceImpl::SetUri(const NapiApi::StrongRef& uri)
{
    uri_ = uri;
}
napi_value SceneResourceImpl::GetUri(NapiApi::FunctionContext<>& ctx)
{
    return uri_.GetValue();
}