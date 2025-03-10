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

#include "geometry_definition/PlaneJS.h"

#include <Vec2Proxy.h>
#include <napi_api.h>

#include "BaseObjectJS.h"

namespace GeometryDefinition {

PlaneJS::PlaneJS(napi_env env, napi_callback_info info) : GeometryDefinition<PlaneJS>(env, info, GeometryType::PLANE) {}

void PlaneJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> props;
    using namespace NapiApi;
    GetPropertyDescs(props);
    props.push_back(GetSetProperty<Object, PlaneJS, &PlaneJS::GetSize, &PlaneJS::SetSize>("size"));

    const auto name = "PlaneGeometry";
    const auto constructor = BaseObject::ctor<PlaneJS>();
    napi_value jsConstructor;
    napi_define_class(env, name, NAPI_AUTO_LENGTH, constructor, nullptr, props.size(), props.data(), &jsConstructor);
    napi_set_named_property(env, exports, name, jsConstructor);

    NapiApi::MyInstanceState* mis {};
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor(name, jsConstructor);
}

void* PlaneJS::GetInstanceImpl(uint32_t id)
{
    return (id == PlaneJS::ID) ? this : nullptr;
}

BASE_NS::Math::Vec2 PlaneJS::GetSize() const
{
    return size_;
}

napi_value PlaneJS::GetSize(NapiApi::FunctionContext<>& ctx)
{
    return Vec2Proxy::ToNapiObject(size_, ctx.Env()).ToNapiValue();
}

void PlaneJS::SetSize(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::Object jsSize = ctx.Arg<0>();
    bool conversionOk = false;
    const auto size = Vec2Proxy::ToNative(jsSize, conversionOk);
    if (conversionOk) {
        size_ = size;
    } else {
        LOG_E("Invalid size given for a PlaneJS");
    }
}

} // namespace GeometryDefinition