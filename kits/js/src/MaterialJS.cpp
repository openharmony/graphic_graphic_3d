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
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene_plugin/api/light_uid.h>
#include <scene_plugin/interface/intf_light.h>
#include <scene_plugin/interface/intf_scene.h>

#include "SceneJS.h"
using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

BaseMaterial::BaseMaterial(MaterialType lt) : SceneResourceImpl(SceneResourceImpl::MATERIAL), materialType_(lt) {}
BaseMaterial::~BaseMaterial() {}
void BaseMaterial::Init(const char* class_name, napi_env env, napi_value exports, napi_callback ctor,
    BASE_NS::vector<napi_property_descriptor>& node_props)
{
    SceneResourceImpl::GetPropertyDescs(node_props);

    using namespace NapiApi;
    node_props.push_back(TROGetProperty<float, BaseMaterial, &BaseMaterial::GetMaterialType>("materialType"));

    napi_value func;
    auto status = napi_define_class(
        env, class_name, NAPI_AUTO_LENGTH, ctor, nullptr, node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor(class_name, func);

    NapiApi::Object exp(env, exports);

    napi_value eType;
    napi_value v;
    napi_create_object(env, &eType);
#define DECL_ENUM(enu, x)                         \
    napi_create_uint32(env, MaterialType::x, &v); \
    napi_set_named_property(env, enu, #x, v)

    DECL_ENUM(eType, SHADER);
#undef DECL_ENUM
    exp.Set("MaterialType", eType);
}

void* BaseMaterial::GetInstanceImpl(uint32_t id)
{
    if (id == BaseMaterial::ID) {
        return (BaseMaterial*)this;
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}
void BaseMaterial::DisposeNative(TrueRootObject* tro)
{
    // do nothing for now..
    LOG_F("BaseMaterial::DisposeNative");
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(tro->GetNativeObject())) {
        // reset the native object refs
        tro->SetNativeObject(nullptr, false);
        tro->SetNativeObject(nullptr, true);

        ExecSyncTask([material = BASE_NS::move(material)]() mutable {
            auto node = interface_pointer_cast<SCENE_NS::INode>(material);
            if (node == nullptr) {
                return META_NS::IAny::Ptr {};
            }
            auto scene = node->GetScene();
            if (scene == nullptr) {
                return META_NS::IAny::Ptr {};
            }
            scene->ReleaseNode(node);
            return META_NS::IAny::Ptr {};
        });
    }
    scene_.Reset();
}
napi_value BaseMaterial::GetMaterialType(NapiApi::FunctionContext<>& ctx)
{
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto node = interface_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
        type = materialType_;
    }
    napi_value value;
    napi_status status = napi_create_uint32(ctx, type, &value);
    return value;
}

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
    // missing
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(e, fromJs.This());

    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene... (do i need it here?)
    NapiApi::Object args = fromJs.Arg<1>();  // other args

    scene_ = scene;
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    auto* tro = scene.Native<TrueRootObject>();
    auto* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
    sceneJS->DisposeHook((uintptr_t)&scene_, meJs);

    auto metaobj = GetNativeObjectParam<META_NS::IObject>(args); // Should be IMaterial
    SetNativeObject(metaobj, true);
    StoreJsObj(metaobj, meJs);

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name")) {
        name = prm;
    } else {
        if (auto named = interface_cast<META_NS::INamed>(metaobj)) {
            name = named->Name()->GetValue();
        }
    }
    meJs.Set("name", name);
}

ShaderMaterialJS::~ShaderMaterialJS() {}
void* ShaderMaterialJS::GetInstanceImpl(uint32_t id)
{
    if (id == ShaderMaterialJS::ID) {
        return this;
    }
    return BaseMaterial::GetInstanceImpl(id);
}
void ShaderMaterialJS::DisposeNative()
{
    NapiApi::Object obj = scene_.GetObject();
    if (obj) {
        auto* tro = obj.Native<TrueRootObject>();
        SceneJS* sceneJS;
        if (tro) {
            sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
            sceneJS->ReleaseDispose((uintptr_t)&scene_);
        }
    }
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject())) {
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);
        if (obj) {
            ExecSyncTask([mat = BASE_NS::move(material)]() -> META_NS::IAny::Ptr {
                mat->MaterialShader()->SetValue(nullptr);
                return {};
            });
        }
    } else {
        SetNativeObject(nullptr, false);
    }
    shader_.Reset();

    BaseMaterial::DisposeNative(this);
}
void ShaderMaterialJS::Finalize(napi_env env)
{
    BaseObject::Finalize(env);
}

void ShaderMaterialJS::SetColorShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::Object shaderJS = ctx.Arg<0>();
    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!material) {
        shader_.Reset();
        return;
    }
    // handle the case where a "bound shader" is attached too.
    auto shader = GetNativeMeta<SCENE_NS::IShader>(shaderJS);
    if (shader == nullptr) {
        // attaching to a bound shader. (if shader was bound to another material, we need to make a new copy)
        auto boundShader = GetNativeMeta<META_NS::IObject>(shaderJS);
        return;
    }
    // bind it to material (in native)
    ExecSyncTask([material, &shader]() -> META_NS::IAny::Ptr {
        material->MaterialShader()->SetValue(shader);
        material->MaterialShaderState()->SetValue(nullptr);
        return {};
    });

    // construct a "bound" shader object from the "non bound" one.
    NapiApi::Object parms(ctx);
    napi_value args[] = {
        scene_.GetValue(), // <- get the scene
        parms              // other constructor parameters
    };

    parms.Set("name", shaderJS.Get("name"));
    parms.Set("Material", ctx.This()); // js material object that we are bound to.

    shaderBind_ = META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object);
    interface_cast<META_NS::IMetadata>(shaderBind_)
        ->AddProperty(META_NS::ConstructProperty<IntfPtr>(
            "shader", nullptr, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
    interface_cast<META_NS::IMetadata>(shaderBind_)
        ->GetPropertyByName<IntfPtr>("shader")
        ->SetValue(interface_pointer_cast<CORE_NS::IInterface>(shader));

    auto argc = BASE_NS::countof(args);
    auto argv = args;
    MakeNativeObjectParam(ctx, shaderBind_, argc, argv);
    auto result = CreateJsObj(ctx, "Shader", shaderBind_, false, argc, argv);
    shader_ = StoreJsObj(shaderBind_, NapiApi::Object(ctx, result));
}
napi_value ShaderMaterialJS::GetColorShader(NapiApi::FunctionContext<>& ctx)
{
    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!material) {
        shader_.Reset();
        return ctx.GetNull();
    }
    if (shader_.IsEmpty()) {
        // no shader set yet..
        // see if we have one on the native side.
        // and create the "bound shader" object from it.

        // check native side..
        SCENE_NS::IShader::Ptr shader;
        ExecSyncTask([material, &shader]() -> META_NS::IAny::Ptr {
            shader = material->MaterialShader()->GetValue();
            return {};
        });
        if (!shader) {
            // no shader in native also.
            return ctx.GetNull();
        }

        // construct a "bound" shader object from the "non bound" one.
        NapiApi::Object parms(ctx);
        napi_value args[] = {
            scene_.GetValue(), // <- get the scene
            parms              // other constructor parameters
        };

        if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
            CORE_LOG_F("INVALID SCENE!");
        }
        parms.Set("Material", ctx.This()); // js material object that we are bound to.

        shaderBind_ = META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object);
        interface_cast<META_NS::IMetadata>(shaderBind_)
            ->AddProperty(META_NS::ConstructProperty<IntfPtr>(
                "shader", nullptr, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
        interface_cast<META_NS::IMetadata>(shaderBind_)
            ->GetPropertyByName<IntfPtr>("shader")
            ->SetValue(interface_pointer_cast<CORE_NS::IInterface>(shader));

        auto argc = BASE_NS::countof(args);
        auto argv = args;
        MakeNativeObjectParam(ctx, shaderBind_, argc, argv);
        auto result = CreateJsObj(ctx, "Shader", shaderBind_, false, argc, argv);
        shader_ = StoreJsObj(shaderBind_, NapiApi::Object(ctx, result));
    }
    return shader_.GetValue();
}
