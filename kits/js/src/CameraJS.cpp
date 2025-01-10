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
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_scene.h>

#include "PromiseBase.h"
#include "Raycast.h"
#include "SceneJS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
static constexpr uint32_t ACTIVE_RENDER_BIT = 1; //  CameraComponent::ACTIVE_RENDER_BIT  comes from lume3d...

void* CameraJS::GetInstanceImpl(uint32_t id)
{
    if (id == CameraJS::ID)
        return this;
    return NodeImpl::GetInstanceImpl(id);
}
void CameraJS::DisposeNative(void*)
{
    if (!disposed_) {
        LOG_V("CameraJS::DisposeNative");
        disposed_ = true;

        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            SceneJS* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
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
        if (auto cam = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);

            auto ptr = cam->PostProcess()->GetValue();
            ReleaseObject(interface_pointer_cast<META_NS::IObject>(ptr));
            ptr.reset();
            cam->PostProcess()->SetValue(nullptr);
            // dispose all extra objects.
            resources_.clear();

            if (auto camnode = interface_pointer_cast<SCENE_NS::INode>(cam)) {
                cam->SetActive(false);
                if (auto scene = camnode->GetScene()) {
                    scene->ReleaseNode(camnode);
                }
            }
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

    node_props.push_back(Method<FunctionContext<Object>, CameraJS, &CameraJS::WorldToScreen>("worldToScreen"));
    node_props.push_back(Method<FunctionContext<Object>, CameraJS, &CameraJS::ScreenToWorld>("screenToWorld"));
    node_props.push_back(Method<FunctionContext<Object, Object>, CameraJS, &CameraJS::Raycast>("raycast"));

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
        // hmm..
        LOG_F("Invalid scene for CameraJS!");
        return;
    }

    NapiApi::Object meJs(fromJs.This());
    auto* tro = scene.Native<TrueRootObject>();
    if (tro) {
        auto* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
        if (sceneJS) {
            sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
        }
    }

    NapiApi::Object args = fromJs.Arg<1>();
    auto obj = GetNativeObjectParam<META_NS::IObject>(args);
    if (obj) {
        // linking to an existing object.
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

    uint32_t pipeline = uint32_t(SCENE_NS::CameraPipeline::LIGHT_FORWARD);
    if (auto prm = args.Get("renderPipeline")) {
        pipeline = NapiApi::Value<uint32_t>(e, prm);
    }

    BASE_NS::string nodePath;

    if (path.IsDefined()) {
        // create using path
        nodePath = path.valueOrDefault("");
    } else if (name.IsDefined()) {
        // use the name as path (creates under root)
        nodePath = name.valueOrDefault("");
    }

    // Create actual camera object.
    SCENE_NS::ICamera::Ptr node =
        scn->CreateNode<SCENE_NS::ICamera>(nodePath, SCENE_NS::ClassId::CameraNode).GetResult();
    node->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline(pipeline));
    node->SetActive(false);
    node->ColorTargetCustomization()->SetValue({SCENE_NS::ColorFormat{BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT}});
    SetNativeObject(interface_pointer_cast<META_NS::IObject>(node), false);
    node.reset();
    StoreJsObj(GetNativeObject(), meJs);

    if (name.IsDefined()) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }
    meJs.Set("postProcess", fromJs.GetNull());
}
void CameraJS::Finalize(napi_env env)
{
    // make sure the camera gets deactivated (the actual c++ camera might not be destroyed here)
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        if (auto scene = interface_cast<SCENE_NS::INode>(camera)->GetScene()) {
            camera->SetActive(false);
        }
    }
    BaseObject<CameraJS>::Finalize(env);
}
CameraJS::~CameraJS()
{
    LOG_V("CameraJS -- ");
}
napi_value CameraJS::GetFov(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    float fov = 0.0;
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        fov = 0.0;
        if (camera) {
            fov = camera->FoV()->GetValue();
        }
    }

    return ctx.GetNumber(fov);
}

void CameraJS::SetFov(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    float fov = ctx.Arg<0>();
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        camera->FoV()->SetValue(fov);
    }
}

napi_value CameraJS::GetEnabled(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    bool activ = false;
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        if (camera) {
            activ = camera->IsActive();
        }
    }
    return ctx.GetBoolean(activ);
}

void CameraJS::SetEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    bool activ = ctx.Arg<0>();
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        ExecSyncTask([camera, activ]() {
            if (camera) {
                uint32_t flags = camera->SceneFlags()->GetValue();
                if (activ) {
                    flags |= uint32_t(SCENE_NS::CameraSceneFlag::MAIN_CAMERA_BIT);
                } else {
                    flags &= ~uint32_t(SCENE_NS::CameraSceneFlag::MAIN_CAMERA_BIT);
                }
                camera->SceneFlags()->SetValue(flags);
                camera->SetActive(activ);
            }
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value CameraJS::GetFar(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float far = 0.0;
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        far = camera->FarPlane()->GetValue();
    }
    return ctx.GetNumber(far);
}

void CameraJS::SetFar(NapiApi::FunctionContext<float>& ctx)
{
    float fov = ctx.Arg<0>();
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        camera->FarPlane()->SetValue(fov);
    }
}

napi_value CameraJS::GetNear(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float near = 0.0;
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        near = camera->NearPlane()->GetValue();
    }
    return ctx.GetNumber(near);
}

void CameraJS::SetNear(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    float fov = ctx.Arg<0>();
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        camera->NearPlane()->SetValue(fov);
    }
}

napi_value CameraJS::GetPostProcess(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    if (auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        SCENE_NS::IPostProcess::Ptr postproc = camera->PostProcess()->GetValue();
        if (!postproc) {
            // early out.
            return ctx.GetNull();
        }
        auto obj = interface_pointer_cast<META_NS::IObject>(postproc);
        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached.ToNapiValue();
        }
        NapiApi::Env env(ctx.Env());
        NapiApi::Object parms(env);
        napi_value args[] = { ctx.This().ToNapiValue(), parms.ToNapiValue() };
        // take ownership of the object.
        postProc_ = NapiApi::StrongRef(CreateFromNativeInstance(env, obj, false, BASE_NS::countof(args), args));
        return postProc_.GetValue();
    }
    return ctx.GetNull();
}

void CameraJS::SetPostProcess(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto camera = interface_cast<SCENE_NS::ICamera>(GetNativeObject());
    if (!camera) {
        return;
    }
    NapiApi::Object psp = ctx.Arg<0>();
    if (auto currentlySet = postProc_.GetObject()) {
        if (psp.StrictEqual(currentlySet)) {
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
    if (psp) {
        // see if we have a native backing for the input object..
        TrueRootObject* native = psp.Native<TrueRootObject>();
        if (!native) {
            // nope.. so create a new bridged object.
            napi_value args[] = {
                ctx.This().ToNapiValue(),  // Camera..
                ctx.Arg<0>().ToNapiValue() // "javascript object for values"
            };
            psp = { GetJSConstructor(ctx.Env(), "PostProcessSettings"), BASE_NS::countof(args), args };
            native = psp.Native<TrueRootObject>();
        }
        postProc_ = NapiApi::StrongRef(psp);

        if (native) {
            postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(native->GetNativeObject());
        }
    }
    camera->PostProcess()->SetValue(postproc);
}

napi_value CameraJS::GetColor(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetThisNativeObject(ctx));
    if (!camera) {
        return ctx.GetUndefined();
    }
    uint32_t curBits = camera->PipelineFlags()->GetValue();
    bool enabled = curBits & static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
    if (!enabled) {
        return ctx.GetNull();
    }

    if (clearColor_ == nullptr) {
        clearColor_ = BASE_NS::make_unique<ColorProxy>(ctx.Env(), camera->ClearColor());
    }
    return clearColor_->Value();
}
void CameraJS::SetColor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetThisNativeObject(ctx));
    if (!camera) {
        return;
    }
    if (clearColor_ == nullptr) {
        clearColor_ = BASE_NS::make_unique<ColorProxy>(ctx.Env(), camera->ClearColor());
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (obj) {
        clearColor_->SetValue(obj);
        clearColorEnabled_ = true;
    } else {
        clearColorEnabled_ = false;
    }
    // enable camera clear
    uint32_t curBits = camera->PipelineFlags()->GetValue();
    if (msaaEnabled_) {
        curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
    } else {
        curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
    }
    if (clearColorEnabled_) {
        curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
    } else {
        curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
    }
    camera->PipelineFlags()->SetValue(curBits);
}

napi_value CameraJS::WorldToScreen(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return ProjectCoords<ProjectionDirection::WORLD_TO_SCREEN>(ctx);
}

napi_value CameraJS::ScreenToWorld(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return ProjectCoords<ProjectionDirection::SCREEN_TO_WORLD>(ctx);
}

template <CameraJS::ProjectionDirection dir>
napi_value CameraJS::ProjectCoords(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::StrongRef scene;
    auto inCoordJs = ctx.Arg<0>();
    auto raycastSelf = SCENE_NS::ICameraRayCast::Ptr {};
    auto inCoord = BASE_NS::Math::Vec3 {};
    if (!ExtractRaycastStuff(inCoordJs, scene, raycastSelf, inCoord)) {
        return {};
    }
    auto outCoord = BASE_NS::Math::Vec3 {};
    if constexpr (dir == ProjectionDirection::WORLD_TO_SCREEN) {
        outCoord = raycastSelf->WorldPositionToScreen(inCoord).GetResult();
    } else {
        outCoord = raycastSelf->ScreenPositionToWorld(inCoord).GetResult();
    }
    return Vec3Proxy::ToNapiObject(outCoord, ctx.Env()).ToNapiValue();
}

napi_value CameraJS::Raycast(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef coordArg_;
        NapiApi::StrongRef optionArg_;
        bool SetResult() override
        {
            auto* rootObject = static_cast<CameraJS*>(this_.GetObject().Native<TrueRootObject>());
            result_ = rootObject->Raycast(this_.GetEnv(), coordArg_.GetObject(), optionArg_.GetObject());
            return (bool)result_;
        }
    };
    auto promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->coordArg_ = NapiApi::StrongRef(ctx.Arg<0>());
    promise->optionArg_ = NapiApi::StrongRef(ctx.Arg<1>());

    auto func = [promise]() {
        promise->SettleLater();
        return false;
    };
    auto task = META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move(func));
    META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD)->AddTask(task);

    return jsPromise;
}

napi_value CameraJS::Raycast(napi_env env, NapiApi::Object screenCoordJs, NapiApi::Object optionsJs)
{
    auto scene = NapiApi::StrongRef {};
    auto raycastSelf = SCENE_NS::ICameraRayCast::Ptr {};
    auto screenCoord = BASE_NS::Math::Vec2 {};
    if (!ExtractRaycastStuff(screenCoordJs, scene, raycastSelf, screenCoord)) {
        return {};
    }

    auto options = ToNativeOptions(env, optionsJs);
    const auto hitResults = raycastSelf->CastRay(screenCoord, options).GetResult();

    napi_value hitList;
    napi_create_array_with_length(env, hitResults.size(), &hitList);
    size_t i = 0;
    for (const auto& hitResult : hitResults) {
        auto hitObject = CreateRaycastResult(scene, env, hitResult);
        napi_set_element(env, hitList, i, hitObject.ToNapiValue());
        i++;
    }
    return hitList;
}

template<typename CoordType>
bool CameraJS::ExtractRaycastStuff(const NapiApi::Object& jsCoord, NapiApi::StrongRef& scene,
    SCENE_NS::ICameraRayCast::Ptr& raycastSelf, CoordType& nativeCoord)
{
    scene = NapiApi::StrongRef { scene_.GetObject() };
    if (!scene.GetValue()) {
        LOG_E("Scene is gone");
        return false;
    }

    raycastSelf = interface_pointer_cast<SCENE_NS::ICameraRayCast>(GetNativeObject());
    if (!raycastSelf) {
        LOG_F("Unable to access raycast API");
        return false;
    }

    bool conversionOk = false;
    if constexpr (BASE_NS::is_same_v<CoordType, BASE_NS::Math::Vec2>) {
        nativeCoord = Vec2Proxy::ToNative(jsCoord, conversionOk);
        } else {
        nativeCoord = Vec3Proxy::ToNative(jsCoord, conversionOk);
    }
    if (!conversionOk) {
        LOG_E("Invalid position argument");
        return false;
    }
    return true;
}

META_NS::IObject::Ptr CameraJS::CreateObject(const META_NS::ClassInfo& type)
{
    if (auto scn = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        META_NS::IObject::Ptr obj = scn->CreateObject(type).GetResult();
        if (obj) {
            resources_[(uintptr_t)obj.get()] = obj;
        }
        return obj;
    }
    return nullptr;
}
void CameraJS::ReleaseObject(const META_NS::IObject::Ptr& obj)
{
    if (obj) {
        resources_.erase((uintptr_t)obj.get());
    }
}

napi_value CameraJS::GetMSAA(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    bool enabled = false;
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        uint32_t curBits = camera->PipelineFlags()->GetValue();
        enabled = curBits & static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
    }
    return ctx.GetBoolean(enabled);
}

void CameraJS::SetMSAA(NapiApi::FunctionContext<bool>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    msaaEnabled_ = ctx.Arg<0>();
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        uint32_t curBits = camera->PipelineFlags()->GetValue();
        if (msaaEnabled_) {
            curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
        } else {
            curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
        }
        if (clearColorEnabled_) {
            curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
        } else {
            curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
        }
        camera->PipelineFlags()->SetValue(curBits);
    }
}
