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
#include "RenderConfigurationJS.h"

#include "class_definition_helpers.h"
#include "BaseObjectJS.h"

#include <3d/render/intf_render_data_store_default_light.h>
#include <meta/api/metadata_util.h>

static constexpr napi_type_tag RENDER_CONFIG_TAG = { SCENE_NS::IRenderConfiguration::UID.data[0],
    SCENE_NS::IRenderConfiguration::UID.data[1] };

template<typename Class>
Class* UnwrapObject(napi_env env, const NapiApi::Object& object, const napi_type_tag& tag)
{
    Class* impl = nullptr;
    bool isObject = false;
    auto value = object.ToNapiValue();
    napi_unwrap(env, value, (void**)&impl);
    napi_status status = napi_check_object_type_tag(env, value, &tag, &isObject);
    return isObject ? impl : nullptr;
}

template<typename Class>
Class* UnwrapObject(napi_env env, napi_callback_info info, const napi_type_tag& tag)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        return UnwrapObject<Class>(fc.GetEnv(), fc.This(), tag);
    }
    return nullptr;
}
template<typename Class, napi_value (Class::*F)(NapiApi::FunctionContext<>&)>
napi_value ClassMethod(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        if (auto impl = UnwrapObject<Class>(fc.GetEnv(), fc.This(), RENDER_CONFIG_TAG)) {
            return (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
}

template<typename Class, napi_value (Class::*F)(NapiApi::FunctionContext<>&)>
napi_value ClassGetter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        if (auto impl = UnwrapObject<Class>(fc.GetEnv(), fc.This(), RENDER_CONFIG_TAG)) {
           return  (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
}
template<typename Class, typename Type, void (Class::*F)(NapiApi::FunctionContext<Type>&)>
inline napi_value ClassSetter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<Type> fc(env, info);
    if (fc && fc.This()) {
        if (auto impl = UnwrapObject<Class>(fc.GetEnv(), fc.This(), RENDER_CONFIG_TAG)) {
             (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
};


RenderConfiguration::RenderConfiguration() {}

void RenderConfiguration::Detach()
{
    rc_.reset();
}

RenderConfiguration::~RenderConfiguration()
{
    Detach();
}

UVec2Proxy* RenderConfiguration::GetResolutionProxy(napi_env env)
{
    if (!rc_) {
        return {};
    }
    if (!resolutionProxy_) {
        resolutionProxy_ = BASE_NS::make_unique<UVec2Proxy>(env, rc_->ShadowResolution());
    }
    return resolutionProxy_.get();
}

napi_value RenderConfiguration::GetShadowResolutionValue(napi_env env)
{
    auto proxy = GetResolutionProxy(env);
    if (proxy) {
        auto property = proxy->GetPropertyPtr();
        if (property && property->IsValueSet()) {
            return proxy->Value();
        }
    }
    auto e = NapiApi::Env(env);
    return e.GetUndefined();
}

void RenderConfiguration::SetTo(NapiApi::Object obj)
{
    obj.Set("shadowResolution", GetShadowResolutionValue(obj.GetEnv()));
}

void RenderConfiguration::SetFrom(napi_env env, SCENE_NS::IRenderConfiguration::Ptr rc)
{
    if (rc_ != rc) {
        resolutionProxy_.reset();
        rc_ = rc;
    }
 }

RenderConfiguration* RenderConfiguration::Unwrap(NapiApi::Object obj)
{
    return UnwrapObject<RenderConfiguration>(obj.GetEnv(), obj, RENDER_CONFIG_TAG);
 }

napi_value RenderConfiguration::Dispose(NapiApi::FunctionContext<>& ctx)
{
    Detach();
    return ctx.GetUndefined();
}

NapiApi::StrongRef RenderConfiguration::Wrap(NapiApi::Object obj)
{
    //  wrap it..
    auto DTOR = [](napi_env env, void* nativeObject, void* finalize) {
        RenderConfiguration* ptr = static_cast<RenderConfiguration*>(nativeObject);
        delete ptr;
    };
    auto env = obj.GetEnv();
    napi_wrap(env, obj.ToNapiValue(), this, DTOR, nullptr, nullptr);

    // clang-format off
    napi_property_descriptor descs[] = {
        { "shadowResolution", nullptr, nullptr, ClassGetter<RenderConfiguration, &RenderConfiguration::GetShadowResolution>,
            ClassSetter<RenderConfiguration, NapiApi::Object, &RenderConfiguration::SetShadowResolution>, nullptr, napi_default_jsproperty, this },
        { "destroy", nullptr, ClassMethod<RenderConfiguration, &RenderConfiguration::Dispose>,
            nullptr, nullptr, nullptr, napi_default_jsproperty, this }
    };
    // clang-format on
    auto wrapped = obj.ToNapiValue();
    napi_define_properties(obj.GetEnv(), wrapped, BASE_NS::countof(descs), descs);
    napi_type_tag_object(env, wrapped, &RENDER_CONFIG_TAG); // Used for type checking in UnwrapObject
    SetTo(obj);
    return NapiApi::StrongRef(obj);
}

napi_value RenderConfiguration::GetShadowResolution(NapiApi::FunctionContext<>& ctx)
{
    auto r = GetResolutionProxy(ctx.Env());
    if (r) {
        auto p = r->GetPropertyPtr();
        if (p && p->IsValueSet()) {
            // We return a valid object only if the value has been set by the user, as defined in .d.ts
            return r->Value(); 
        }
    }
    return ctx.GetUndefined();
}

void RenderConfiguration::SetShadowResolution(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (auto r = GetResolutionProxy(ctx.GetEnv())) {
        NapiApi::Object obj = ctx.Arg<0>();
        if (obj.IsDefined()) {
            r->SetValue(obj);
        } else {
            // shadowResolution = undefined leads to resetting the property, setting shadow resolution back to default value
            r->ResetValue();
        }
    }
}
