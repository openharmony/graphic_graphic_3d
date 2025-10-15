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
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_scene.h>

#include "ParamParsing.h"
#include "Promise.h"
#include "Raycast.h"
#include "SceneJS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "nodejstaskqueue.h"

static constexpr uint32_t ACTIVE_RENDER_BIT = 1; //  CameraComponent::ACTIVE_RENDER_BIT  comes from lume3d...

void* CameraJS::GetInstanceImpl(uint32_t id)
{
    if (id == CameraJS::ID) {
        return this;
    }
    return NodeImpl::GetInstanceImpl(id);
}
void CameraJS::DisposeNative(void* sc)
{
    if (!disposed_) {
        LOG_V("CameraJS::DisposeNative");
        disposed_ = true;
        auto cam = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject());

        if (auto* sceneJS = static_cast<SceneJS*>(sc)) {
            sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
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
        if (cam) {
            UnsetNativeObject();

            auto ptr = cam->PostProcess()->GetValue();
            ReleaseObject(interface_pointer_cast<META_NS::IObject>(ptr));
            ptr.reset();
            cam->PostProcess()->SetValue(nullptr);
            // dispose all extra objects.
            resources_.clear();

            if (!IsAttached()) {
                cam->SetActive(false);
                if (auto node = interface_pointer_cast<SCENE_NS::INode>(cam)) {
                    if (auto scene = node->GetScene()) {
                        scene->RemoveNode(BASE_NS::move(node)).Wait();
                    }
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
    node_props.push_back(GetSetProperty<std::optional<bool>, CameraJS, &CameraJS::GetMSAA, &CameraJS::SetMSAA>("msaa"));
    node_props.push_back(
        GetSetProperty<uint32_t, CameraJS, &CameraJS::GetRenderingPipeline, &CameraJS::SetRenderingPipeline>(
            "renderingPipeline"));
    node_props.push_back(GetSetProperty<uint32_t, CameraJS, &CameraJS::GetRenderTargetColorFormat,
        &CameraJS::SetRenderTargetColorFormat>("defaultRenderTargetFormat"));
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
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("Camera", func);
    }
}

void CameraJS::RegisterEnums(NapiApi::Object exports)
{
    const auto env = exports.GetEnv();
    auto tmp = napi_value {};

    auto colorFormat = NapiApi::Object { env };
#define DECL_ENUM(enu, x)                                                \
    {                                                                    \
        napi_create_uint32(env, BASE_NS::Format::BASE_FORMAT_##x, &tmp); \
        enu.Set(#x, tmp);                                                \
    }
    DECL_ENUM(colorFormat, R8G8B8_SRGB);
    DECL_ENUM(colorFormat, R8G8B8A8_SRGB);
    DECL_ENUM(colorFormat, R16G16B16_SFLOAT);
    DECL_ENUM(colorFormat, R16G16B16A16_SFLOAT);
    DECL_ENUM(colorFormat, B10G11R11_UFLOAT_PACK32);
#undef DECL_ENUM
    exports.Set("ColorFormat", colorFormat);

    auto renderingPipelineType = NapiApi::Object { env };
    napi_create_uint32(env, static_cast<uint32_t>(SCENE_NS::CameraPipeline::LIGHT_FORWARD), &tmp);
    renderingPipelineType.Set("FORWARD_LIGHTWEIGHT", tmp);
    napi_create_uint32(env, static_cast<uint32_t>(SCENE_NS::CameraPipeline::FORWARD), &tmp);
    renderingPipelineType.Set("FORWARD", tmp);
    exports.Set("RenderingPipelineType", renderingPipelineType);
}

CameraJS::CameraJS(napi_env e, napi_callback_info i) : BaseObject(e, i), NodeImpl(NodeImpl::CAMERA)
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

    auto scn = scene.GetNative<SCENE_NS::IScene>();
    if (scn == nullptr) {
        // hmm..
        LOG_F("Invalid scene for CameraJS!");
        return;
    }

    NapiApi::Object meJs(fromJs.This());
    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    auto node = GetNativeObject<SCENE_NS::ICamera>();
    if (!node) {
        LOG_E("Cannot finish creating a camera: Native camera object missing");
        assert(false);
        return;
    }

    auto sceneNodeParameters = NapiApi::Object { fromJs.Arg<1>() };
    if (const auto name = ExtractName(sceneNodeParameters); !name.empty()) {
        meJs.Set("name", name);
    }
}
void CameraJS::Finalize(napi_env env)
{
    // make sure the camera gets deactivated (the actual c++ camera might not be destroyed here)
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        if (auto scene = interface_cast<SCENE_NS::INode>(camera)->GetScene()) {
            camera->SetActive(false);
        }
    }
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
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
        if (const auto postproc = camera->PostProcess()->GetValue()) {
            NapiApi::Env env(ctx.Env());
            NapiApi::Object parms(env);
            napi_value args[] = { ctx.This().ToNapiValue(), parms.ToNapiValue() };
            // The native camera owns the native post process. We, the JS camera, own the JS post process.
            postProc_ = NapiApi::StrongRef(CreateFromNativeInstance(env, postproc, PtrType::WEAK, args));
        }
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
        if (currentlySet.StrictEqual(psp)) {
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
    TrueRootObject* native = nullptr;

    if (psp) {
        native = psp.GetRoot();
    }

    if (!native) {
        // nope.. so create a new bridged object.
        napi_value args[] = {
            ctx.This().ToNapiValue(),  // Camera..
            psp ? ctx.Arg<0>().ToNapiValue()
                : NapiApi::Object(ctx.Env()).ToNapiValue() // "javascript object for values"
        };

        if (auto cameraJs = static_cast<CameraJS *>(ctx.This().GetRoot())) {
            auto postproc = cameraJs->CreateObject(SCENE_NS::ClassId::PostProcess);

            if (!psp) {
                if (auto pp = interface_pointer_cast<SCENE_NS::IPostProcess>(postproc)) {
                    pp->Vignette()->GetValue()->Enabled()->SetValue(true);
                    pp->ColorFringe()->GetValue()->Enabled()->SetValue(true);
                    pp->Bloom()->GetValue()->Enabled()->SetValue(true);
                }
            }
            // PostProcessSettings will store a weak ref of its native. We, the camera, own it.
            psp = CreateFromNativeInstance(ctx.Env(), postproc, PtrType::WEAK, args);
            native = psp.GetRoot();
        }
    }

    postProc_ = NapiApi::StrongRef(psp);

    if (native) {
        postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(native->GetNativeObject());
    }

    camera->PostProcess()->SetValue(postproc);
}

napi_value CameraJS::GetRenderingPipeline(NapiApi::FunctionContext<>& ctx)
{
    auto pipeline = SCENE_NS::CameraPipeline::LIGHT_FORWARD;
    if (const auto camera = ctx.This().GetNative<SCENE_NS::ICamera>()) {
        pipeline = camera->RenderingPipeline()->GetValue();
    } else {
        LOG_E("Native camera unset. Getting default rendering pipeline type");
    }
    return ctx.GetNumber(static_cast<uint32_t>(pipeline));
}

void CameraJS::SetRenderingPipeline(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (const auto camera = ctx.This().GetNative<SCENE_NS::ICamera>()) {
        const uint32_t pipeline = ctx.Arg<0>();
        camera->RenderingPipeline()->SetValue(static_cast<SCENE_NS::CameraPipeline>(pipeline));
    } else {
        LOG_E("Native camera unset. Setting rendering pipeline type has no effect");
    }
}

napi_value CameraJS::GetRenderTargetColorFormat(NapiApi::FunctionContext<>& ctx)
{
    auto colorFormat = SCENE_NS::ColorFormat {};
    if (const auto camera = ctx.This().GetNative<SCENE_NS::ICamera>()) {
        if (const auto formats = camera->ColorTargetCustomization()->GetValue(); !formats.empty()) {
            colorFormat = formats.front();
        }
    } else {
        LOG_E("Native camera unset. Getting default render target color format");
    }
    return ctx.GetNumber(static_cast<uint32_t>(colorFormat.format));
}

void CameraJS::SetRenderTargetColorFormat(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (const auto camera = ctx.This().GetNative<SCENE_NS::ICamera>()) {
        const uint32_t format = ctx.Arg<0>();
        auto colorFormat = SCENE_NS::ColorFormat {};
        colorFormat.format = static_cast<BASE_NS::Format>(format);
        // Just overwrite whatever the array had with our single value.
        camera->ColorTargetCustomization()->SetValue({ colorFormat });
    } else {
        LOG_E("Native camera unset. Setting render target color format has no effect");
    }
}

napi_value CameraJS::GetColor(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto camera = ctx.This().GetNative<SCENE_NS::ICamera>();
    if (!camera) {
        return ctx.GetUndefined();
    }
    uint32_t curBits = camera->PipelineFlags()->GetValue();
    bool enabled = curBits & static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
    if (!enabled) {
        return ctx.GetNull();
    }

    if (clearColor_ == nullptr) {
        // camera->ClearColor() is of type Vec4, convert to Color on the fly
        clearColor_ = BASE_NS::make_unique<ColorProxy>(ctx.Env(), camera->ClearColor());
    }
    return clearColor_->Value();
}
void CameraJS::SetColor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto camera = ctx.This().GetNative<SCENE_NS::ICamera>();
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

template<CameraJS::ProjectionDirection dir>
napi_value CameraJS::ProjectCoords(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto inCoordJs = ctx.Arg<0>();
    const auto res = GetRaycastResources<BASE_NS::Math::Vec3>(inCoordJs);
    if (!res.hasEverything) {
        LOG_E("%s", res.errorMsg.c_str());
        return {};
    }

    auto outCoord = BASE_NS::Math::Vec3 {};
    if constexpr (dir == ProjectionDirection::WORLD_TO_SCREEN) {
        outCoord = res.raycastSelf->WorldPositionToScreen(res.nativeCoord).GetResult();
    } else {
        outCoord = res.raycastSelf->ScreenPositionToWorld(res.nativeCoord).GetResult();
    }
    return Vec3Proxy::ToNapiObject(outCoord, ctx.Env()).ToNapiValue();
}

napi_value CameraJS::Raycast(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    const auto env = ctx.Env();
    auto promise = Promise(env);
    NapiApi::Object screenCoordJs = ctx.Arg<0>();
    NapiApi::Object optionsJs = ctx.Arg<1>();
    auto res = GetRaycastResources<BASE_NS::Math::Vec2>(screenCoordJs);
    if (!res.hasEverything) {
        return promise.Reject(res.errorMsg);
    }

    auto convertToJs = [promise, scene = BASE_NS::move(res.scene)](SCENE_NS::NodeHits hitResults) mutable {
        const auto env = promise.Env();
        napi_value hitList;
        napi_create_array_with_length(env, hitResults.size(), &hitList);
        size_t i = 0;
        for (const auto& hitResult : hitResults) {
            const auto hitObject = CreateRaycastResult(scene, env, hitResult);
            napi_set_element(env, hitList, i, hitObject.ToNapiValue());
            i++;
        }
        promise.Resolve(hitList);
    };

    const auto options = ToNativeOptions(env, optionsJs);
    auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    res.raycastSelf->CastRay(res.nativeCoord, options).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

template<typename CoordType>
CameraJS::RaycastResources<CoordType> CameraJS::GetRaycastResources(const NapiApi::Object& jsCoord)
{
    auto res = RaycastResources<CoordType> {};
    res.scene = NapiApi::StrongRef { scene_.GetObject() };
    if (!res.scene.GetValue()) {
        res.errorMsg = "Scene is gone. ";
    }

    res.raycastSelf = interface_pointer_cast<SCENE_NS::ICameraRayCast>(GetNativeObject());
    if (!res.raycastSelf) {
        res.errorMsg.append("Unable to access raycast API. ");
    }

    auto conversionOk = false;
    if constexpr (BASE_NS::is_same_v<CoordType, BASE_NS::Math::Vec2>) {
        res.nativeCoord = Vec2Proxy::ToNative(jsCoord, conversionOk);
    } else {
        res.nativeCoord = Vec3Proxy::ToNative(jsCoord, conversionOk);
    }
    if (!conversionOk) {
        res.errorMsg.append("Invalid position argument given");
    }
    res.hasEverything = res.errorMsg.empty();
    return res;
}

META_NS::IObject::Ptr CameraJS::CreateObject(const META_NS::ClassInfo& type)
{
    if (auto scn = scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
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

void CameraJS::SetMSAA(NapiApi::FunctionContext<std::optional<bool>>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    std::optional<bool> value = ctx.Arg<0>();
    if (value) {
        msaaEnabled_ = *value;
    } else {
        msaaEnabled_ = false;
    }

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
