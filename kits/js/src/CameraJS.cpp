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
#include "CameraJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/interface/intf_camera.h>
#include <scene_plugin/interface/intf_scene.h>

#include "SceneJS.h"
static constexpr uint32_t ACTIVE_RENDER_BIT = 1; //  CameraComponent::ACTIVE_RENDER_BIT  comes from lume3d...

void* CameraJS::GetInstanceImpl(uint32_t id)
{
    if (id == CameraJS::ID) {
        return this;
    }
    return NodeImpl::GetInstanceImpl(id);
}
void CameraJS::DisposeNative()
{
    if (!disposed_) {
        LOG_F("CameraJS::DisposeNative");
        disposed_ = true;

        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            SceneJS* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->ReleaseStrongDispose((uintptr_t)&scene_);
            }
        }

        // make sure we release postProc settings
        if (auto ps = postProc_.GetObject()) {
            NapiApi::Function func = ps.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(ps);
            }
        }
        postProc_.Reset();

        clearColor_.reset();
        if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);

            ExecSyncTask([this, cam = BASE_NS::move(camera), res = BASE_NS::move(resources_)]() mutable {
                auto ptr = cam->PostProcess()->GetValue();
                ReleaseObject(interface_pointer_cast<META_NS::IObject>(ptr));
                ptr.reset();
                cam->PostProcess()->SetValue(nullptr);
                // dispose all extra objects.
                res.clear();

                auto camnode = interface_pointer_cast<SCENE_NS::INode>(cam);
                if (camnode == nullptr) {
                    return META_NS::IAny::Ptr {};
                }
                auto scene = camnode->GetScene();
                if (scene == nullptr) {
                    return META_NS::IAny::Ptr {};
                }
                scene->DeactivateCamera(cam);
                scene->ReleaseNode(camnode);
                return META_NS::IAny::Ptr {};
            });
        }
        scene_.Reset();
    }
}
void CameraJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    NodeImpl::GetPropertyDescs(node_props);

    using namespace NapiApi;
    node_props.push_back(GetSetProperty<float, CameraJS, &CameraJS::GetFov, &CameraJS::SetFov>("fov"));
    node_props.push_back(GetSetProperty<float, CameraJS, &CameraJS::GetNear, &CameraJS::SetNear>("nearPlane"));
    node_props.push_back(GetSetProperty<float, CameraJS, &CameraJS::GetFar, &CameraJS::SetFar>("farPlane"));
    node_props.push_back(GetSetProperty<bool, CameraJS, &CameraJS::GetEnabled, &CameraJS::SetEnabled>("enabled"));
    node_props.push_back(GetSetProperty<bool, CameraJS, &CameraJS::GetMSAA, &CameraJS::SetMSAA>("msaa"));
    node_props.push_back(
        GetSetProperty<Object, CameraJS, &CameraJS::GetPostProcess, &CameraJS::SetPostProcess>("postProcess"));
    node_props.push_back(GetSetProperty<Object, CameraJS, &CameraJS::GetColor, &CameraJS::SetColor>("clearColor"));

    napi_value func;
    auto status = napi_define_class(env, "Camera", NAPI_AUTO_LENGTH, BaseObject::ctor<CameraJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Camera", func);
}

CameraJS::CameraJS(napi_env e, napi_callback_info i) : BaseObject<CameraJS>(e, i), NodeImpl(NodeImpl::CAMERA)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish initialization
        return;
    }
    // java script call.. with arguments
    NapiApi::Object scene = fromJs.Arg<0>();
    scene_ = scene;

    auto scn = GetNativeMeta<SCENE_NS::IScene>(scene);
    if (scn == nullptr) {
        CORE_LOG_F("Invalid scene for CameraJS!");
        return;
    }

    NapiApi::Object meJs(e, fromJs.This());
    auto* tro = scene.Native<TrueRootObject>();
    auto* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
    sceneJS->StrongDisposeHook((uintptr_t)&scene_, meJs);

    NapiApi::Object args = fromJs.Arg<1>();
    auto obj = GetNativeObjectParam<META_NS::IObject>(args);
    if (obj) {
        // linking to an existing object.
        NapiApi::Object meJs(e, fromJs.This());
        SetNativeObject(obj, false);
        StoreJsObj(obj, meJs);
        return;
    }

    // collect parameters
    NapiApi::Value<BASE_NS::string> name;
    NapiApi::Value<BASE_NS::string> path;

    if (auto prm = args.Get("name")) {
        name = NapiApi::Value<BASE_NS::string>(e, prm);
    }
    if (auto prm = args.Get("path")) {
        path = NapiApi::Value<BASE_NS::string>(e, prm);
    }

    uint32_t pipeline = SCENE_NS::ICamera::SceneCameraPipeline::SCENE_CAM_PIPELINE_LIGHT_FORWARD;
    if (auto prm = args.Get("renderPipeline")) {
        pipeline = NapiApi::Value<uint32_t>(e, prm);
    }

    BASE_NS::string nodePath;

    if (path) {
        // create using path
        nodePath = path.valueOrDefault("");
    } else if (name) {
        // use the name as path (creates under root)
        nodePath = name.valueOrDefault("");
    } else {
        // no name or path defined should this just fail?
    }

    // Create actual camera object.
    SCENE_NS::ICamera::Ptr node;
    ExecSyncTask([scn, nodePath, &node, pipeline]() {
        node = scn->CreateNode<SCENE_NS::ICamera>(nodePath, true);
        node->RenderingPipeline()->SetValue(pipeline);
        scn->DeactivateCamera(node);
        return META_NS::IAny::Ptr {};
    });

    SetNativeObject(interface_pointer_cast<META_NS::IObject>(node), false);
    node.reset();
    StoreJsObj(GetNativeObject(), meJs);

    if (name) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }
    meJs.Set("postProcess", fromJs.GetNull());
}
void CameraJS::Finalize(napi_env env)
{
    // make sure the camera gets deactivated (the actual c++ camera might not be destroyed here)
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera]() {
            if (auto scene = interface_cast<SCENE_NS::INode>(camera)->GetScene()) {
                scene->DeactivateCamera(camera);
            }
            return META_NS::IAny::Ptr {};
        });
    }
    BaseObject<CameraJS>::Finalize(env);
}
CameraJS::~CameraJS()
{
    LOG_F("CameraJS -- ");
}
napi_value CameraJS::GetFov(NapiApi::FunctionContext<>& ctx)
{
    float fov = 0.0;
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, &fov]() {
            fov = 0.0;
            if (camera) {
                fov = camera->FoV()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    napi_value value;
    napi_status status = napi_create_double(ctx, fov, &value);
    return value;
}

void CameraJS::SetFov(NapiApi::FunctionContext<float>& ctx)
{
    float fov = ctx.Arg<0>();
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, fov]() {
            camera->FoV()->SetValue(fov);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value CameraJS::GetEnabled(NapiApi::FunctionContext<>& ctx)
{
    bool activ = false;
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, &activ]() {
            if (camera) {
                if (auto scene = interface_cast<SCENE_NS::INode>(camera)->GetScene()) {
                    activ = scene->IsCameraActive(camera);
                }
            }
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_get_boolean(ctx, activ, &value);
    return value;
}

void CameraJS::SetEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    bool activ = ctx.Arg<0>();
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, activ]() {
            if (auto scene = interface_cast<SCENE_NS::INode>(camera)->GetScene()) {
                if (activ) {
                    scene->ActivateCamera(camera);
                } else {
                    scene->DeactivateCamera(camera);
                }
            }
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value CameraJS::GetFar(NapiApi::FunctionContext<>& ctx)
{
    float fov = 0.0;
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, &fov]() {
            fov = 0.0;
            if (camera) {
                fov = camera->FarPlane()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    napi_value value;
    napi_status status = napi_create_double(ctx, fov, &value);
    return value;
}

void CameraJS::SetFar(NapiApi::FunctionContext<float>& ctx)
{
    float fov = ctx.Arg<0>();
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, fov]() {
            camera->FarPlane()->SetValue(fov);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value CameraJS::GetNear(NapiApi::FunctionContext<>& ctx)
{
    float fov = 0.0;
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, &fov]() {
            fov = 0.0;
            if (camera) {
                fov = camera->NearPlane()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    napi_value value;
    napi_status status = napi_create_double(ctx, fov, &value);
    return value;
}

void CameraJS::SetNear(NapiApi::FunctionContext<float>& ctx)
{
    float fov = ctx.Arg<0>();
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, fov]() {
            camera->NearPlane()->SetValue(fov);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value CameraJS::GetPostProcess(NapiApi::FunctionContext<>& ctx)
{
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        SCENE_NS::IPostProcess::Ptr postproc;
        ExecSyncTask([camera, &postproc]() {
            postproc = camera->PostProcess()->GetValue();
            return META_NS::IAny::Ptr {};
        });
        auto obj = interface_pointer_cast<META_NS::IObject>(postproc);
        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached;
        }
        NapiApi::Object parms;            /*empty object*/
        napi_value args[] = { ctx.This(), // Camera..
            parms };
        napi_value postProcJS = CreateFromNativeInstance(ctx, obj, false, BASE_NS::countof(args), args);
        postProc_ = { ctx, postProcJS }; // take ownership of the object.
        return postProcJS;
    }
    return ctx.GetNull();
}

void CameraJS::SetPostProcess(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::Object psp = ctx.Arg<0>();
    if (auto currentlySet = postProc_.GetObject()) {
        if ((napi_value)currentlySet == (napi_value)psp) {
            // setting the exactly the same postprocess setting. do nothing.
            return;
        }
        NapiApi::Function func = currentlySet.Get<NapiApi::Function>("destroy");
        if (func) {
            func.Invoke(currentlySet);
        }
        postProc_.Reset();
    }

    SCENE_NS::IPostProcess::Ptr postproc;
    // see if we have a native backing for the input object..
    TrueRootObject* native = psp.Native<TrueRootObject>();
    if (!native) {
        // nope.. so create a new bridged object.
        napi_value args[] = {
            ctx.This(), // Camera..
            ctx.Arg<0>() // "javascript object for values"
        };
        psp = { GetJSConstructor(ctx, "PostProcessSettings"), BASE_NS::countof(args), args };
        native = psp.Native<TrueRootObject>();
    }
    postProc_ = { ctx, psp };

    if (native) {
        postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(native->GetNativeObject());
    }
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, postproc = BASE_NS::move(postproc)]() {
            camera->PostProcess()->SetValue(postproc);
            return META_NS::IAny::Ptr {};
        });
    }
}

// the PipelineFlagBits are not declared in sceneplugin api..
namespace {

enum PipelineFlagBits : uint32_t {
    /** Target clear flags depth. Override camera render node graph loadOp with clear.
     * Without clear the default render node graph based loadOp is used. (Default pipelines use depth clear)
     */
    CLEAR_DEPTH_BIT = (1 << 0),
    /** Target clear flags color. Override camera render node graph loadOp with clear.
     * Without clear the default render node graph based loadOp is used. (Default pipelines do not use color clear)
     */
    CLEAR_COLOR_BIT = (1 << 1),
    /** Enable MSAA for rendering. Only affects non deferred default pipelines. */
    MSAA_BIT = (1 << 2),
    /** Automatically use pre-pass if there are default material needs (e.g. for transmission). Automatic RNG
       generation needs to be enabled for the ECS scene. */
    ALLOW_COLOR_PRE_PASS_BIT = (1 << 3),
    /** Force pre-pass every frame. Use for e.g. custom shaders without default material needs. Automatic RNG
       generation needs to be enabled for the ECS scene. */
    FORCE_COLOR_PRE_PASS_BIT = (1 << 4),
    /** Store history (store history for next frame, needed for e.g. temporal filtering) */
    HISTORY_BIT = (1 << 5),
    /** Jitter camera. With Halton sampling */
    JITTER_BIT = (1 << 6),
    /** Output samplable velocity / normal */
    VELOCITY_OUTPUT_BIT = (1 << 7),
    /** Output samplable depth */
    DEPTH_OUTPUT_BIT = (1 << 8),
    /** Is a multi-view camera and is not be rendered separately at all
     * The camera is added to other camera as multiViewCameras
     */
    MULTI_VIEW_ONLY_BIT = (1 << 9),
    /** Generate environment cubemap dynamically for the camera
     */
    DYNAMIC_CUBEMAP_BIT = (1 << 10),
    /** Disallow reflection plane for camera
     */
    DISALLOW_REFLECTION_BIT = (1 << 11),
};
}

napi_value CameraJS::GetColor(NapiApi::FunctionContext<>& ctx)
{
    auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetThisNativeObject(ctx));
    if (!camera) {
        return {};
    }
    bool enabled { false };
    ExecSyncTask([camera, &enabled]() {
        // enable camera clear
        uint32_t curBits = camera->PipelineFlags()->GetValue();
        enabled = curBits & PipelineFlagBits::CLEAR_COLOR_BIT;
        return META_NS::IAny::Ptr {};
    });
    if (!enabled) {
        return ctx.GetNull();
    }

    if (clearColor_ == nullptr) {
        clearColor_ = BASE_NS::make_unique<ColorProxy>(ctx, camera->ClearColor());
    }
    return clearColor_->Value();
}
void CameraJS::SetColor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetThisNativeObject(ctx));
    if (!camera) {
        return;
    }
    if (clearColor_ == nullptr) {
        clearColor_ = BASE_NS::make_unique<ColorProxy>(ctx, camera->ClearColor());
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (obj) {
        clearColor_->SetValue(obj);
        clearColorEnabled_ = true;
    } else {
        clearColorEnabled_ = false;
    }
    ExecSyncTask([camera, clearColorEnabled = clearColorEnabled_, msaaEnabled = msaaEnabled_]() {
        // enable camera clear
        uint32_t curBits = camera->PipelineFlags()->GetValue();
        if (msaaEnabled) {
            curBits |= PipelineFlagBits::MSAA_BIT;
        } else {
            curBits &= ~PipelineFlagBits::MSAA_BIT;
        }
        if (clearColorEnabled) {
            curBits |= PipelineFlagBits::CLEAR_COLOR_BIT;
        } else {
            curBits &= ~PipelineFlagBits::CLEAR_COLOR_BIT;
        }
        camera->PipelineFlags()->SetValue(curBits);
        return META_NS::IAny::Ptr {};
    });
}

META_NS::IObject::Ptr CameraJS::CreateObject(const META_NS::ClassInfo& type)
{
    META_NS::IObject::Ptr obj = META_NS::GetObjectRegistry().Create(type);
    if (obj) {
        resources_[(uintptr_t)obj.get()] = obj;
    }
    return obj;
}
void CameraJS::ReleaseObject(const META_NS::IObject::Ptr& obj)
{
    if (obj) {
        resources_.erase((uintptr_t)obj.get());
    }
}

napi_value CameraJS::GetMSAA(NapiApi::FunctionContext<>& ctx)
{
    bool enabled = false;
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, &enabled]() {
            uint32_t curBits = camera->PipelineFlags()->GetValue();
            enabled = curBits & PipelineFlagBits::MSAA_BIT;
            return META_NS::IAny::Ptr {};
        });
    }

    napi_value value;
    napi_status status = napi_get_boolean(ctx, enabled, &value);
    return value;
}

void CameraJS::SetMSAA(NapiApi::FunctionContext<bool>& ctx)
{
    msaaEnabled_ = ctx.Arg<0>();
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, msaaEnabled = msaaEnabled_, clearColorEnabled = clearColorEnabled_]() {
            uint32_t curBits = camera->PipelineFlags()->GetValue();
            if (msaaEnabled) {
                curBits |= PipelineFlagBits::MSAA_BIT;
            } else {
                curBits &= ~PipelineFlagBits::MSAA_BIT;
            }
            if (clearColorEnabled) {
                curBits |= PipelineFlagBits::CLEAR_COLOR_BIT;
            } else {
                curBits &= ~PipelineFlagBits::CLEAR_COLOR_BIT;
            }
            camera->PipelineFlags()->SetValue(curBits);
            return META_NS::IAny::Ptr {};
        });
    }
}
