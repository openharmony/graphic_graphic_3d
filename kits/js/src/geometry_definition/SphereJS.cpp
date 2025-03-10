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

#include "geometry_definition/SphereJS.h"

#include <Vec2Proxy.h>
#include <napi_api.h>

#include "BaseObjectJS.h"

namespace GeometryDefinition {

SphereJS::SphereJS(napi_env env, napi_callback_info info)
    : GeometryDefinition<SphereJS>(env, info, GeometryType::SPHERE)
{}

void SphereJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> props;
    using namespace NapiApi;
    GetPropertyDescs(props);
    props.push_back(GetSetProperty<float, SphereJS, &SphereJS::GetRadius, &SphereJS::SetRadius>("radius"));
    props.push_back(
        GetSetProperty<uint32_t, SphereJS, &SphereJS::GetSegmentCount, &SphereJS::SetSegmentCount>("segmentCount"));

    const auto name = "SphereGeometry";
    const auto constructor = BaseObject::ctor<SphereJS>();
    napi_value jsConstructor;
    napi_define_class(env, name, NAPI_AUTO_LENGTH, constructor, nullptr, props.size(), props.data(), &jsConstructor);
    napi_set_named_property(env, exports, name, jsConstructor);

    NapiApi::MyInstanceState* mis {};
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor(name, jsConstructor);
}

void* SphereJS::GetInstanceImpl(uint32_t id)
{
    return (id == SphereJS::ID) ? this : nullptr;
}

float SphereJS::GetRadius() const
{
    return radius_;
}

uint32_t SphereJS::GetSegmentCount() const
{
    return segmentCount_;
}

napi_value SphereJS::GetRadius(NapiApi::FunctionContext<>& ctx)
{
    return ctx.GetNumber(radius_);
}

napi_value SphereJS::GetSegmentCount(NapiApi::FunctionContext<>& ctx)
{
    return ctx.GetNumber(segmentCount_);
}

void SphereJS::SetRadius(NapiApi::FunctionContext<float>& ctx)
{
    auto radius = double {};
    if (napi_get_value_double(ctx.Env(), ctx.Arg<0>().ToNapiValue(), &radius) == napi_ok) {
        radius_ = static_cast<float>(radius);
    } else {
        LOG_E("Invalid radius given for a SphereJS");
    }
}

void SphereJS::SetSegmentCount(NapiApi::FunctionContext<uint32_t>& ctx)
{
    auto count = uint32_t {};
    if (napi_get_value_uint32(ctx.Env(), ctx.Arg<0>().ToNapiValue(), &count) == napi_ok) {
        segmentCount_ = count;
    } else {
        LOG_E("Invalid segment count given for a SphereJS");
    }
}

} // namespace GeometryDefinition
