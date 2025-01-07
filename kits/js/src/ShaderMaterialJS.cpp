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
#include "MaterialJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_material.h>

#include "SceneJS.h"
using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

void ShaderMaterialJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> props = {
        NapiApi::GetSetProperty<NapiApi::Object, ShaderMaterialJS, &ShaderMaterialJS::GetColorShader,
            &ShaderMaterialJS::SetColorShader>("colorShader"),
    };

    BaseMaterial::Init("ShaderMaterial", env, exports, BaseObject::ctor<ShaderMaterialJS>(), props);
}

ShaderMaterialJS::ShaderMaterialJS(napi_env e, napi_callback_info i)
    : BaseObject<ShaderMaterialJS>(e, i), BaseMaterial(BaseMaterial::MaterialType::SHADER)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());

    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene... (do i need it here?)
    NapiApi::Object args = fromJs.Arg<1>();  // other args

    scene_ = scene;
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }

    auto* tro = scene.Native<TrueRootObject>();
    if (tro) {
        auto* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
        if (sceneJS) {
            sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
        }
    }

    auto metaobj = GetNativeObjectParam<META_NS::IObject>(args); // Should be IMaterial
    SetNativeObject(metaobj, true);
    StoreJsObj(metaobj, meJs);

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name"); prm.IsDefined()) {
        name = prm;
    } else {
        if (auto named = interface_cast<META_NS::IObject>(metaobj)) {
            name = named->GetName();
        }
    }
    meJs.Set("name", name);
}

ShaderMaterialJS::~ShaderMaterialJS() {}
void* ShaderMaterialJS::GetInstanceImpl(uint32_t id)
{
    if (id == ShaderMaterialJS::ID)
        return this;
    return BaseMaterial::GetInstanceImpl(id);
}
void ShaderMaterialJS::DisposeNative(void*)
{
    NapiApi::Object obj = scene_.GetObject();
    if (obj) {
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            auto sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
            }
        }
    }
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject())) {
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);
        if (obj) {
            material->MaterialShader()->SetValue(nullptr);
        }
    } else {
        SetNativeObject(nullptr, false);
    }
    shaderBind_.reset();
    shader_.Reset();

    BaseMaterial::DisposeNative(this);
}
void ShaderMaterialJS::Finalize(napi_env env)
{
    BaseObject::Finalize(env);
}
void ShaderMaterialJS::SetColorShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        // owning scene has been destroyed.
        // the material etc should also be gone.
        // but there is a possible issue where the native object is still alive.

        // most likely could happen if scene.dispose is not called
        // but all references to the scene have been released,
        // and garbage collection may or may not have been done yet. (or is partially done)
        // if the scene is garbage collected then all the resources should be disposed.
        return;
    }

    NapiApi::Object shaderJS = ctx.Arg<0>();
    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        shader_.Reset();
        return;
    }
    auto shader = GetNativeMeta<SCENE_NS::IShader>(shaderJS);
    if (shader == nullptr) {
        // attaching to a bound shader. (if shader was bound to another material, we need to make a new copy)
        auto boundShader = GetNativeMeta<META_NS::IObject>(shaderJS);
        return;
    }
    // bind it to material (in native)
    material->MaterialShader()->SetValue(shader);

    // construct a "bound" shader object from the "non bound" one.
    NapiApi::Env env(ctx.Env());
    NapiApi::Object parms(env);
    napi_value args[] = {
        scene_.GetValue(),  // bind the new instance of the shader to this javascript scene object
        parms.ToNapiValue() // other constructor parameters
    };

    parms.Set("name", shaderJS.Get("name"));
    parms.Set("Material", ctx.This()); // js material object that we are bound to.

    shaderBind_ = META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object);
    interface_cast<META_NS::IMetadata>(shaderBind_)
        ->AddProperty(META_NS::ConstructProperty<IntfPtr>(
            "shader", nullptr, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
    interface_cast<META_NS::IMetadata>(shaderBind_)
        ->GetProperty<IntfPtr>("shader")
        ->SetValue(interface_pointer_cast<CORE_NS::IInterface>(shader));

    auto argc = BASE_NS::countof(args);
    auto argv = args;
    MakeNativeObjectParam(env, shaderBind_, argc, argv);
    auto result = CreateJsObj(env, "Shader", shaderBind_, false, argc, argv);
    shader_ = NapiApi::StrongRef(StoreJsObj(shaderBind_, result));
}
napi_value ShaderMaterialJS::GetColorShader(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        shader_.Reset();
        return ctx.GetNull();
    }
    if (shader_.IsEmpty()) {
        // no shader set yet..
        // see if we have one on the native side.
        // and create the "bound shader" object from it.

        // check native side..
        SCENE_NS::IShader::Ptr shader = material->MaterialShader()->GetValue();
        if (!shader) {
            // no shader in native also.
            return ctx.GetNull();
        }

        // construct a "bound" shader object from the "non bound" one.
        NapiApi::Env env(ctx.Env());
        NapiApi::Object parms(env);
        napi_value args[] = {
            scene_.GetValue(),  // bind the new instance of the shader to this javascript scene object
            parms.ToNapiValue() // other constructor parameters
        };

        if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
            LOG_F("INVALID SCENE!");
        }
        parms.Set("Material", ctx.This()); // js material object that we are bound to.

        shaderBind_ = META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object);
        interface_cast<META_NS::IMetadata>(shaderBind_)
            ->AddProperty(META_NS::ConstructProperty<IntfPtr>(
                "shader", nullptr, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
        interface_cast<META_NS::IMetadata>(shaderBind_)
            ->GetProperty<IntfPtr>("shader")
            ->SetValue(interface_pointer_cast<CORE_NS::IInterface>(shader));

        auto argc = BASE_NS::countof(args);
        auto argv = args;
        MakeNativeObjectParam(env, shaderBind_, argc, argv);
        auto result = CreateJsObj(env, "Shader", shaderBind_, false, argc, argv);
        shader_ = NapiApi::StrongRef(StoreJsObj(shaderBind_, result));
    }
    return shader_.GetValue();
}
