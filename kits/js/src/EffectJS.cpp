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

#include "EffectJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>
#include <scene/ext/util.h>
#include "PropertyProxy.h"

#include <render/intf_render_context.h>

#include "SceneJS.h"

#include <cctype>

using namespace SCENE_NS;

// EffectsContainerJS

template<typename FC, napi_value (EffectsContainerJS::*F)(FC&)>
static inline napi_value ECMethodI(napi_env env, napi_callback_info info)
{
    FC fc(env, info);
    if (auto value = fc.RawThis()) {
        EffectsContainerJS* me = nullptr;
        napi_unwrap(env, value, (void**)&me);
        if (me) {
            return (me->*F)(fc);
        }
    }
    return fc.GetUndefined();
};

template<typename FC, napi_value (EffectsContainerJS::*F)(FC&)>
static inline napi_property_descriptor ECMethod(
    const char* const name, napi_property_attributes flags = napi_default_method)
{
    return napi_property_descriptor { name, nullptr, ECMethodI<FC, F>, nullptr, nullptr, nullptr, flags,
        nullptr };
}

void EffectsContainerJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    napi_property_descriptor props[] = {
        ECMethod<FunctionContext<Object>, &EffectsContainerJS::AppendChild>("append"),
        ECMethod<FunctionContext<Object, Object>, &EffectsContainerJS::InsertChildAfter>("insertAfter"),
        ECMethod<FunctionContext<Object>, &EffectsContainerJS::RemoveChild>("remove"),
        ECMethod<FunctionContext<uint32_t>,  &EffectsContainerJS::GetChild>("get"),
        ECMethod<FunctionContext<>, &EffectsContainerJS::ClearChildren>("clear"),
        ECMethod<FunctionContext<>, &EffectsContainerJS::GetCount>("count")
    };
    napi_callback ctor = [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value thisVar = nullptr;
        // fetch the "this" from javascript.
        napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
        // The EffectsContainerJS constructor calls napi_wrap
        auto r = BASE_NS::make_unique<EffectsContainerJS>(env, info);
        r.release();
        return thisVar;
    };

    napi_value func;
    auto status = napi_define_class(env, "EffectsContainer", NAPI_AUTO_LENGTH, ctor,
                        nullptr, sizeof(props) / sizeof(props[0]), props, &func);
    if (status != napi_ok) {
        LOG_E("export class failed in %s", __func__);
    }

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("EffectsContainer", func);
    }
}

EffectsContainerJS::EffectsContainerJS(napi_env e, napi_callback_info i)
{
    LOG_V("EffectsContainerJS ++");

    auto dtor = [](napi_env env, void* nativeObject, void* finalizeHint) {
        auto* effectsContainer = reinterpret_cast<EffectsContainerJS*>(nativeObject);
        if (effectsContainer) {
            delete effectsContainer;
        }
    };

    napi_value thisVar = nullptr;
    auto status = napi_get_cb_info(e, i, nullptr, nullptr, &thisVar, nullptr);
    if (status != napi_ok) {
        LOG_E("Unable to get napi callback info");
    }

    status = napi_wrap(e, thisVar, reinterpret_cast<void*>((EffectsContainerJS*)this), dtor, nullptr, nullptr);
    if (status != napi_ok) {
        LOG_E("Unable to wrap EffectsContainerJS");
    }

    if (auto fromJs = NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>(e, i)) {
        camera_ = NapiApi::Object(fromJs.Arg<0>());
        scene_ = NapiApi::Object(fromJs.Arg<1>());
    }
}

EffectsContainerJS::~EffectsContainerJS()
{
    LOG_V("EffectsContainerJS --");
    scene_.Reset();
    camera_.Reset();
}

META_NS::ArrayProperty<SCENE_NS::IEffect::Ptr> EffectsContainerJS::GetEffects() const
{
    META_NS::ArrayProperty<SCENE_NS::IEffect::Ptr> effects;
    if (auto camera = camera_.GetObject<SCENE_NS::ICameraEffect>()) {
        effects = camera->Effects();
    }
    return effects;
}

napi_value EffectsContainerJS::GetCount(NapiApi::FunctionContext<>& ctx)
{
    auto effects = GetEffects();
    return ctx.GetNumber(effects ? static_cast<uint32_t>(effects->GetSize()) : 0u);
}

napi_value EffectsContainerJS::GetChild(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (auto effects = GetEffects()) {
        uint32_t index = ctx.Arg<0>();
        if (auto effect = effects->GetValueAt(index)) {
            const auto env = ctx.GetEnv();
            auto argJS = NapiApi::Object { env };
            napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
            // These are owned by the underlying camera effect array property, so store only weak reference.
            return CreateFromNativeInstance(ctx.Env(), effect, PtrType::WEAK, args).ToNapiValue();
        }
    }
    return ctx.GetNull();
}

napi_value EffectsContainerJS::ClearChildren(NapiApi::FunctionContext<>& ctx)
{
    if (auto effects = GetEffects()) {
        effects->Reset();
    }
    return ctx.GetUndefined();
}

napi_value EffectsContainerJS::InsertChildAfter(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    auto effects = GetEffects();
    if (!effects) {
        return ctx.GetUndefined();
    }
    auto childJs = ctx.Arg<0>();
    auto afterJs = ctx.Arg<1>();
    if (!childJs.IsDefinedAndNotNull()) {
        return ctx.GetUndefined();
    }
    NapiApi::Object child = childJs;
    size_t index = 0;

    if (afterJs.IsDefinedAndNotNull()) {
        NapiApi::Object after = afterJs;
        if (auto afterEffect = after.GetNative<IEffect>()) {
            index = effects->FindFirstValueOf(afterEffect);
            if (index != size_t(-1)) {
                index = index + 1;
            }
        }
    }
    if (auto effect = child.GetNative<SCENE_NS::IEffect>()) {
        effects->InsertValueAt(index, effect);
    }
    return ctx.GetUndefined();
}

napi_value EffectsContainerJS::AppendChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (auto effects = GetEffects()) {
        auto effectJs = ctx.Arg<0>();
        if (effectJs.IsDefinedAndNotNull()) {
            NapiApi::Object childJS = effectJs;
            if (auto effect = childJS.GetNative<SCENE_NS::IEffect>()) {
                effects->AddValue(effect);
            }
        }
    }
    return ctx.GetUndefined();
}

napi_value EffectsContainerJS::RemoveChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (auto effects = GetEffects()) {
        auto effectJs = ctx.Arg<0>();
        if (effectJs.IsDefinedAndNotNull()) {
            NapiApi::Object childJS = effectJs;
            if (auto effect = childJS.GetNative<SCENE_NS::IEffect>()) {
                effects->RemoveAt(effects->FindFirstValueOf(effect));
            }
        }
    }
    return ctx.GetUndefined();
}

// EffectJS

SCENE_NS::IEffect::Ptr EffectJS::CreateEffectInstance()
{
    return interface_pointer_cast<SCENE_NS::IEffect>(META_NS::GetObjectRegistry().Create(SCENE_NS::ClassId::Effect));
 }

void EffectJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> props;
    props.push_back(GetSetProperty<bool, EffectJS, &EffectJS::GetEnabled, &EffectJS::SetEnabled>("enabled"));
    props.push_back(GetProperty<BASE_NS::string, EffectJS, &EffectJS::GetEffectId>("effectId"));
    props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, EffectJS, &EffectJS::Dispose>("destroy"));

    napi_value func;
    auto status = napi_define_class(env, "Effect", NAPI_AUTO_LENGTH, BaseObject::ctor<EffectJS>(),
        nullptr, props.size(), props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("Effect", func);
    }
}

napi_value EffectJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_V("EffectJS::Dispose");
    DisposeNative(nullptr);
    return {};
}
void EffectJS::DisposeNative(void* scene)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("EffectJS::DisposeNative");
        if (auto* sceneJS = static_cast<SceneJS*>(scene)) {
            sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
        scene_.Reset();
        proxies_.clear();
    }
}

void* EffectJS::GetInstanceImpl(uint32_t id)
{
    if (id == EffectJS::ID) {
        return this;
    }
    return nullptr;
}

void EffectJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}

EffectJS::EffectJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
{
    LOG_V("EffectJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        return;
    }
    scene_ = { NapiApi::Object(fromJs.Arg<0>()) };
    const auto native = GetNativeObject();
    if (!native) {
        return;
    }

    NapiApi::Object meJs(fromJs.This());

    BASE_NS::string name = native->GetName();
    meJs.Set("name", name);
    AddProperties(meJs, native);
}

EffectJS::~EffectJS()
{
    LOG_V("EffectJS --");
    DisposeNative(nullptr);
}

SCENE_NS::IEffect::Ptr EffectJS::GetEffect(NapiApi::Object o) const
{
    return o.GetNative<SCENE_NS::IEffect>();
}

void EffectJS::SetEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    if (auto effect = GetEffect(ctx.This())) {
        META_NS::SetValue(effect->Enabled(), ctx.Arg<0>());
    }
}

napi_value EffectJS::GetEnabled(NapiApi::FunctionContext<>& ctx)
{
    bool enabled = false;
    if (auto effect = GetEffect(ctx.This())) {
        enabled = META_NS::GetValue(effect->Enabled());
    }
    return ctx.GetBoolean(enabled);
}

napi_value EffectJS::GetEffectId(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string id;
    if (auto effect = GetEffect(ctx.This())) {
        id = effect->GetEffectClassId().ToString();
    }
    return ctx.GetString(id);
}

void EffectJS::AddProperties(NapiApi::Object meJs, const META_NS::IObject::Ptr& obj)
{
    auto effect = interface_cast<SCENE_NS::IEffect>(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    if (!(effect && meta)) {
        return;
    }

    const auto& properties = meta->GetProperties();
    for (auto&& p : properties) {
        if (p) {
            const auto name = BASE_NS::string(SCENE_NS::PropertyName(p->GetName()));
            if (name == "Enabled" || name == "enabled") {
                // Ignore enabled property, it is accessible from Effect directly
                continue;
            }
            // Only expose properties starting with a lower case letter
            // (this will limit the exposed properties to ones that are automatically exposed from the ECS side post
            // process)
            if (!name.empty() && std::islower(*name.cbegin())) {
                if (auto proxy = PropertyToProxy(scene_.GetNapiObject(), meJs, p)) {
                    auto res = proxies_.insert_or_assign(name, proxy);
                    auto desc = CreateProxyDesc(name.c_str(), BASE_NS::move(proxy));

                    // The properties need to be defined one at a time because name.c_str() points to a
                    // stack variable. Otherwise we would need to allocate space for every name and we
                    // would lose the speed benefit of defining everything in a single go.
                    napi_define_properties(meJs.GetEnv(), meJs.ToNapiValue(), 1, &desc);
                }
            }
        }
    }
}
