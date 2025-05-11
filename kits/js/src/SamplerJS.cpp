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

#include "SamplerJS.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_texture.h>
#include <render/intf_render_context.h>

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;

void SamplerJS::RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    auto env = exports.GetEnv();
    NapiApi::Object SceneFilter(env);
    NapiApi::Object SceneSamplerAddressMode(env);

    using SCENE_NS::SamplerFilter;
    using SCENE_NS::SamplerAddressMode;

#define DECL_ENUM(enu, type, value)                                                 \
    {                                                                               \
        napi_create_uint32(enu.GetEnv(), static_cast<uint32_t>(type::value), &v);   \
        enu.Set(#value, v);                                                         \
    }
    DECL_ENUM(SceneFilter, SamplerFilter, NEAREST);
    DECL_ENUM(SceneFilter, SamplerFilter, LINEAR);

    DECL_ENUM(SceneSamplerAddressMode, SamplerAddressMode, REPEAT);
    DECL_ENUM(SceneSamplerAddressMode, SamplerAddressMode, MIRRORED_REPEAT);
    DECL_ENUM(SceneSamplerAddressMode, SamplerAddressMode, CLAMP_TO_EDGE);
    // Do not declare CLAMP_TO_BORDER or MIRROR_CLAMP_TO_EDGE as they are not part of SamplerAddressMode in sceneResources.d.ts
    //DECL_ENUM(SceneSamplerAddressMode, SamplerAddressMode, CLAMP_TO_BORDER);
    //DECL_ENUM(SceneSamplerAddressMode, SamplerAddressMode, MIRROR_CLAMP_TO_EDGE);
#undef DECL_ENUM
    exports.Set("SamplerFilter", SceneFilter);
    exports.Set("SamplerAddressMode", SceneSamplerAddressMode);
}

constexpr auto DEFAULT_FILTER = SCENE_NS::SamplerFilter::NEAREST;
constexpr auto DEFAULT_ADDESS_MODE = SCENE_NS::SamplerAddressMode::REPEAT;

SCENE_NS::SamplerFilter SamplerJS::ConvertToFilter(uint32_t value)
{
    static constexpr auto MAX_FILTER = static_cast<uint32_t>(SCENE_NS::SamplerFilter::LINEAR);
    if (value <= MAX_FILTER) {
        return static_cast<SCENE_NS::SamplerFilter>(value);
    }
    return DEFAULT_FILTER;
}

uint32_t SamplerJS::ConvertFromFilter(SCENE_NS::SamplerFilter value)
{
    return static_cast<uint32_t>(value);
}

SCENE_NS::SamplerAddressMode SamplerJS::ConvertToAddressMode(uint32_t value)
{
    static constexpr auto MAX_ADDRESS_MDOE = static_cast<uint32_t>(SCENE_NS::SamplerAddressMode::MIRROR_CLAMP_TO_EDGE);
    if (value <= MAX_ADDRESS_MDOE) {
        return static_cast<SCENE_NS::SamplerAddressMode>(value);
    }
    return DEFAULT_ADDESS_MODE;
}

uint32_t SamplerJS::ConvertFromAddressMode(SCENE_NS::SamplerAddressMode value)
{
    return static_cast<uint32_t>(value);
}

void SamplerJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;

    node_props.emplace_back(
        GetSetProperty<uint32_t, SamplerJS, &SamplerJS::GetMagFilter, &SamplerJS::SetMagFilter>("magFilter"));
    node_props.emplace_back(
        GetSetProperty<uint32_t, SamplerJS, &SamplerJS::GetMinFilter, &SamplerJS::SetMinFilter>("minFilter"));
    node_props.emplace_back(
        GetSetProperty<uint32_t, SamplerJS, &SamplerJS::GetMipMapMode, &SamplerJS::SetMipMapMode>("mipMapMode"));
    node_props.emplace_back(
        GetSetProperty<uint32_t, SamplerJS, &SamplerJS::GetAddressModeU, &SamplerJS::SetAddressModeU>("addressModeU"));
    node_props.emplace_back(
        GetSetProperty<uint32_t, SamplerJS, &SamplerJS::GetAddressModeV, &SamplerJS::SetAddressModeV>("addressModeV"));
    node_props.emplace_back(
        GetSetProperty<uint32_t, SamplerJS, &SamplerJS::GetAddressModeW, &SamplerJS::SetAddressModeW>("addressModeW"));

    node_props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, SamplerJS, &SamplerJS::Dispose>("destroy"));

    napi_value func;
    auto status = napi_define_class(env, "Sampler", NAPI_AUTO_LENGTH, BaseObject::ctor<SamplerJS>(),
        nullptr, node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    mis->StoreCtor("Sampler", func);

    NapiApi::Object exp1(env, exports);
    napi_value eType1, v1;
    napi_create_object(env, &eType1);
}

napi_value SamplerJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_V("SamplerJS::Dispose");
    DisposeNative(nullptr);
    return {};
}
void SamplerJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("SamplerJS::DisposeNative");
        UnsetNativeObject();
    }
}
void SamplerJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
}
void* SamplerJS::GetInstanceImpl(uint32_t id)
{
    if (id == SamplerJS::ID)
        return this;
    return nullptr;
}


SamplerJS::SamplerJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
{
    LOG_V("Sampler ++");
    if (!GetSampler()) {
        LOG_E("Cannot finish creating a texture sampler: Native sampler object missing");
        assert(false);
    }
}

SamplerJS::~SamplerJS()
{
    LOG_V("Sampler --");
    DisposeNative(nullptr);
}

SCENE_NS::ISampler::Ptr SamplerJS::GetSampler() const
{
    return GetNativeObject<SCENE_NS::ISampler>();
}

napi_value SamplerJS::GetMagFilter(NapiApi::FunctionContext<>& ctx)
{
    auto sampler = GetSampler();
    auto filter = sampler ? META_NS::GetValue(sampler->MagFilter()) : DEFAULT_FILTER;
    return ctx.GetNumber(ConvertFromFilter(filter));
}

void SamplerJS::SetMagFilter(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (auto sampler = GetSampler()) {
        auto value = ConvertToFilter(ctx.Arg<0>());
        META_NS::SetValue(sampler->MagFilter(), value);
    }
}

napi_value SamplerJS::GetMinFilter(NapiApi::FunctionContext<>& ctx) 
{
    auto sampler = GetSampler();
    auto filter = sampler ? META_NS::GetValue(sampler->MinFilter()) : DEFAULT_FILTER;
    return ctx.GetNumber(ConvertFromFilter(filter));
}

void SamplerJS::SetMinFilter(NapiApi::FunctionContext<uint32_t>& ctx) 
{
    if (auto sampler = GetSampler()) {
        auto value = ConvertToFilter(ctx.Arg<0>());
        META_NS::SetValue(sampler->MinFilter(), value);
    }
}

napi_value SamplerJS::GetMipMapMode(NapiApi::FunctionContext<>& ctx)
{
    auto sampler = GetSampler();
    auto filter = sampler ? META_NS::GetValue(sampler->MipMapMode()) : DEFAULT_FILTER;
    return ctx.GetNumber(ConvertFromFilter(filter));
}

void SamplerJS::SetMipMapMode(NapiApi::FunctionContext<uint32_t>& ctx) 
{
    if (auto sampler = GetSampler()) {
        auto value = ConvertToFilter(ctx.Arg<0>());
        META_NS::SetValue(sampler->MipMapMode(), value);
    }
}

napi_value SamplerJS::GetAddressModeU(NapiApi::FunctionContext<>& ctx)
{
    auto sampler = GetSampler();
    auto mode = sampler ? META_NS::GetValue(sampler->AddressModeU()) : DEFAULT_ADDESS_MODE;
    return ctx.GetNumber(ConvertFromAddressMode(mode));
}

void SamplerJS::SetAddressModeU(NapiApi::FunctionContext<uint32_t>& ctx) 
{
    if (auto sampler = GetSampler()) {
        auto value = ConvertToAddressMode(ctx.Arg<0>());
        META_NS::SetValue(sampler->AddressModeU(), value);
    }
}

napi_value SamplerJS::GetAddressModeV(NapiApi::FunctionContext<>& ctx)
{
    auto sampler = GetSampler();
    auto mode = sampler ? META_NS::GetValue(sampler->AddressModeV()) : DEFAULT_ADDESS_MODE;
    return ctx.GetNumber(ConvertFromAddressMode(mode));
}

void SamplerJS::SetAddressModeV(NapiApi::FunctionContext<uint32_t>& ctx) 
{
    if (auto sampler = GetSampler()) {
        auto value = ConvertToAddressMode(ctx.Arg<0>());
        META_NS::SetValue(sampler->AddressModeV(), value);
    }
}

napi_value SamplerJS::GetAddressModeW(NapiApi::FunctionContext<>& ctx)
{
    auto sampler = GetSampler();
    auto mode = sampler ? META_NS::GetValue(sampler->AddressModeW()) : DEFAULT_ADDESS_MODE;
    return ctx.GetNumber(ConvertFromAddressMode(mode));
}

void SamplerJS::SetAddressModeW(NapiApi::FunctionContext<uint32_t>& ctx) 
{
    if (auto sampler = GetSampler()) {
        auto value = ConvertToAddressMode(ctx.Arg<0>());
        META_NS::SetValue(sampler->AddressModeW(), value);
    }
}
