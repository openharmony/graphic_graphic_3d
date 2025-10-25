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

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_material.h>

#include "MaterialJS.h"
#include "MaterialPropertyJS.h"
#include "SamplerJS.h"

#include <optional>

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using namespace SCENE_NS;

void MaterialPropertyJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;
    node_props.push_back(
        GetSetProperty<Object, MaterialPropertyJS, &MaterialPropertyJS::GetImage, &MaterialPropertyJS::SetImage>(
            "image"));
    node_props.push_back(
        GetSetProperty<Object, MaterialPropertyJS, &MaterialPropertyJS::GetFactor, &MaterialPropertyJS::SetFactor>(
            "factor"));
    node_props.push_back(
        GetSetProperty<Object, MaterialPropertyJS, &MaterialPropertyJS::GetSampler, &MaterialPropertyJS::SetSampler>(
            "sampler"));
    node_props.push_back(
        MakeTROMethod<FunctionContext<>, MaterialPropertyJS, &MaterialPropertyJS::Dispose>("destroy"));

    napi_value func;
    auto status = napi_define_class(env, "MaterialProperty", NAPI_AUTO_LENGTH, BaseObject::ctor<MaterialPropertyJS>(),
        nullptr, node_props.size(), node_props.data(), &func);

    MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("MaterialProperty", func);
    }
}
MaterialPropertyJS::MaterialPropertyJS(napi_env e, napi_callback_info i) : BaseObject(e, i) {}
MaterialPropertyJS::~MaterialPropertyJS()
{
    DisposeNative(nullptr);
    if (!GetNativeObject()) {
        return;
    }
}
void* MaterialPropertyJS::GetInstanceImpl(uint32_t id)
{
    if (id == MaterialPropertyJS::ID)
        return this;
    return nullptr;
}
napi_value MaterialPropertyJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    DisposeNative(nullptr);
    return {};
}
void MaterialPropertyJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
}
void MaterialPropertyJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        sampler_.Reset();
        factorProxy_.reset();
        UnsetNativeObject();
    }
}
napi_value MaterialPropertyJS::GetImage(NapiApi::FunctionContext<>& ctx)
{
    auto texture = ctx.This().GetNative<SCENE_NS::ITexture>();
    if (!texture) {
        return ctx.GetUndefined();
    }

    SCENE_NS::IBitmap::Ptr image = META_NS::GetValue(texture->Image());
    napi_value args[] = { ctx.This().ToNapiValue() };
    return CreateFromNativeInstance(ctx.GetEnv(), image, PtrType::STRONG, args).ToNapiValue();
}

void MaterialPropertyJS::SetImage(Scene::ITexture::Ptr texture, NapiApi::Object imageJS)
{
    if (!texture) {
        return;
    }
    // GetNative may return null. Then we are just unsetting the image.
    const auto image = imageJS.GetNative<SCENE_NS::IImage>();
    META_NS::SetValue(texture->Image(), image);
}

void MaterialPropertyJS::SetImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto texture = ctx.This().GetNative<SCENE_NS::ITexture>();
    if (!texture) {
        return;
    }

    NapiApi::Object imageJS = ctx.Arg<0>();
    SetImage(texture, imageJS);
}
napi_value MaterialPropertyJS::GetFactor(NapiApi::FunctionContext<> &ctx)
{
    if (auto texture = ctx.This().GetNative<SCENE_NS::ITexture>()) {
        if (!factorProxy_) {
            factorProxy_ = BASE_NS::make_unique<Vec4Proxy>(ctx.Env(), texture->Factor());
        }
        return factorProxy_->Value();
    }
    return ctx.GetUndefined();
}

void MaterialPropertyJS::SetFactor(Scene::ITexture::Ptr texture, NapiApi::Object factorJS)
{
    if (texture && factorJS) {
        auto x = factorJS.Get<float>("x");
        auto y = factorJS.Get<float>("y");
        auto z = factorJS.Get<float>("z");
        auto w = factorJS.Get<float>("w");
        META_NS::SetValue(texture->Factor(), { x, y, z, w });
    }
}

void MaterialPropertyJS::SetFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto texture = ctx.This().GetNative<SCENE_NS::ITexture>();
    if (!texture) {
        return;
    }
    if (NapiApi::Object factorJS = ctx.Arg<0>()) {
        SetFactor(texture, factorJS);
    }
}
napi_value MaterialPropertyJS::GetSampler(NapiApi::FunctionContext<>& ctx)
{
    if (auto existing = sampler_.GetObject()) {
        return existing.ToNapiValue();
    }
    
    if (auto texture = GetNativeObject<ITexture>()) {
        if (auto sampler = META_NS::GetValue(texture->Sampler())) {
            napi_value args[] = { ctx.This().ToNapiValue() };
            auto object = CreateFromNativeInstance(ctx.GetEnv(), sampler, PtrType::STRONG, args);
            sampler_ = NapiApi::StrongRef(object);
            return object.ToNapiValue();
        }
    }
    return ctx.GetUndefined();
}

namespace {
struct SamplerChangeSet {
    std::optional<SCENE_NS::SamplerFilter> minFilter;
    std::optional<SCENE_NS::SamplerFilter> magFilter;
    std::optional<SCENE_NS::SamplerFilter> mipMapMode;
    std::optional<SCENE_NS::SamplerAddressMode> addressModeU;
    std::optional<SCENE_NS::SamplerAddressMode> addressModeV;
    std::optional<SCENE_NS::SamplerAddressMode> addressModeW;

    bool HasChanges() const
    {
        return minFilter.has_value() || magFilter.has_value() || mipMapMode.has_value() ||
               addressModeU.has_value() || addressModeV.has_value() || addressModeW.has_value();
    }
};

void PopulateChanges(SamplerChangeSet& changes, NapiApi::Object& source)
{
    if (source.Has("magFilter")) {
        changes.magFilter = SamplerJS::ConvertToFilter(source.Get<uint32_t>("magFilter"));
    }
    if (source.Has("minFilter")) {
        changes.minFilter = SamplerJS::ConvertToFilter(source.Get<uint32_t>("minFilter"));
    }
    if (source.Has("mipMapMode")) {
        changes.mipMapMode = SamplerJS::ConvertToFilter(source.Get<uint32_t>("mipMapMode"));
    }
    if (source.Has("addressModeU")) {
        changes.addressModeU = SamplerJS::ConvertToAddressMode(source.Get<uint32_t>("addressModeU"));
    }
    if (source.Has("addressModeV")) {
        changes.addressModeV = SamplerJS::ConvertToAddressMode(source.Get<uint32_t>("addressModeV"));
    }
    if (source.Has("addressModeW")) {
        changes.addressModeW = SamplerJS::ConvertToAddressMode(source.Get<uint32_t>("addressModeW"));
    }
}
}

void MaterialPropertyJS::SetSampler(Scene::ITexture::Ptr texture, NapiApi::Object source)
{
    if (!texture) {
        return;
    }
    auto target = META_NS::GetValue(texture->Sampler());
    SamplerChangeSet changes;
    bool defined = source && source.IsDefined() && !source.IsNull();
    if (defined) {
        PopulateChanges(changes, source);
    }
    ExecSyncTask([&]() {
        // Apply given object as a changeset on top of default sampler
        if (auto resetable = interface_cast<META_NS::IResetableObject>(target)) {
            resetable->ResetObject(); // First, reset to default state
        }
        // Then apply those properties that have been set in our input object
        if (changes.HasChanges()) {
            if (changes.magFilter) {
                META_NS::SetValue(target->MagFilter(), changes.magFilter.value());
            }
            if (changes.minFilter) {
                META_NS::SetValue(target->MinFilter(), changes.minFilter.value());
            }
            if (changes.mipMapMode) {
                META_NS::SetValue(target->MipMapMode(), changes.mipMapMode.value());
            }
            if (changes.addressModeU) {
                META_NS::SetValue(target->AddressModeU(), changes.addressModeU.value());
            }
            if (changes.addressModeV) {
                META_NS::SetValue(target->AddressModeV(), changes.addressModeV.value());
            }
            if (changes.addressModeW) {
                META_NS::SetValue(target->AddressModeW(), changes.addressModeW.value());
            }
        }
        return META_NS::IAny::Ptr {};
    });
}

void MaterialPropertyJS::SetSampler(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto texture = ctx.This().GetNative<SCENE_NS::ITexture>();
    if (!texture) {
        return;
    }
    NapiApi::Object samplerJS = ctx.Arg<0>();
    SetSampler(texture, samplerJS);
}