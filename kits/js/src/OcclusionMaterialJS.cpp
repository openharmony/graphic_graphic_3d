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
#include "OcclusionMaterialJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_material.h>
#include <meta/api/util.h>

#include "JsObjectCache.h"
#include "SceneJS.h"
#include "ShaderJS.h"
#include "MaterialJS.h"

using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

OcclusionMaterial::OcclusionMaterial(napi_env e, napi_callback_info i)
    : BaseObject(e, i), SceneResourceImpl(SceneResourceImpl::MATERIAL)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());
    AddBridge(CLASS_NAME, meJs);

    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene... (do i need it here?)
    NapiApi::Object args = fromJs.Arg<1>();  // other args

    scene_ = scene;
    if (!scene.GetNative<SCENE_NS::IScene>()) {
        LOG_F("Invalid scene");
    }

    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    const auto material = GetNativeObject<SCENE_NS::IMaterial>();
    if (material) {
        if (META_NS::GetValue(material->Type()) != SCENE_NS::MaterialType::OCCLUSION) {
            LOG_E("Invalid material type");
        }
    }

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name"); prm.IsDefined()) {
        name = prm;
    } else {
        if (auto named = interface_cast<META_NS::IObject>(material)) {
            name = named->GetName();
        }
    }
    meJs.Set("name", name);
}

OcclusionMaterial::~OcclusionMaterial() 
{}

void OcclusionMaterial::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;

    SceneResourceImpl::GetPropertyDescs(node_props);

    node_props.emplace_back(
        TROGetProperty<uint32_t, OcclusionMaterial, &OcclusionMaterial::GetMaterialType>("materialType"));

       napi_value func;
    auto status = napi_define_class(env, OcclusionMaterial::CLASS_NAME, NAPI_AUTO_LENGTH,
           BaseObject::ctor<OcclusionMaterial>(), nullptr,
           node_props.size(), node_props.data(), &func);
    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor(OcclusionMaterial::CLASS_NAME, func);
    }
}

void* OcclusionMaterial::GetInstanceImpl(uint32_t id)
{
    if (id == OcclusionMaterial::ID)
        return static_cast<OcclusionMaterial*>(this);
    return SceneResourceImpl::GetInstanceImpl(id);
}

void OcclusionMaterial::DisposeNative(void* scn)
{
    if (disposed_) {
        return;
    }
    DisposeBridge(this);
    if (auto material = GetNativeObject<META_NS::IObject>()) {
        DetachJsObj(material, "_JSW");
    }
    disposed_ = true;
    if (auto* sceneJS = static_cast<SceneJS*>(scn)) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    UnsetNativeObject();
    scene_.Reset();
}

void OcclusionMaterial::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
    FinalizeBridge(this);
}

napi_value OcclusionMaterial::GetMaterialType(NapiApi::FunctionContext<>& ctx)
{
    return ctx.GetNumber(BaseMaterial::OCCLUSION);
}
