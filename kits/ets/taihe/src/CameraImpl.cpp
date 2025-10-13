/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "CameraImpl.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "ColorImpl.h"
#include "Vec3Impl.h"

#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "CheckNapiEnv.h"
#include "BaseObjectJS.h"

namespace OHOS::Render3D::KITETS {
CameraImpl::CameraImpl(const std::shared_ptr<CameraETS> cameraETS) : NodeImpl(cameraETS), cameraETS_(cameraETS)
{}

CameraImpl::~CameraImpl()
{
    cameraETS_.reset();
}

double CameraImpl::getFov()
{
    if (cameraETS_) {
        return cameraETS_->GetFov();
    } else {
        return 0.0F;
    }
}

void CameraImpl::setFov(double fov)
{
    if (cameraETS_) {
        cameraETS_->SetFov(fov);
    }
}

double CameraImpl::getNearPlane()
{
    if (cameraETS_) {
        return cameraETS_->GetNear();
    } else {
        return 0.0F;
    }
}

void CameraImpl::setNearPlane(double nearPlane)
{
    if (cameraETS_) {
        cameraETS_->SetNear(nearPlane);
    }
}

double CameraImpl::getFarPlane()
{
    if (cameraETS_) {
        return cameraETS_->GetFar();
    } else {
        return 0.0F;
    }
}

void CameraImpl::setFarPlane(double farPlane)
{
    if (cameraETS_) {
        cameraETS_->SetFar(farPlane);
    }
}

bool CameraImpl::getEnabled()
{
    if (cameraETS_) {
        return cameraETS_->GetEnabled();
    } else {
        return false;
    }
}

void CameraImpl::setEnabled(bool enabled)
{
    if (cameraETS_) {
        cameraETS_->SetEnabled(enabled);
    }
}

::SceneNodes::PostProcessSettingsOrNull CameraImpl::getPostProcess()
{
    if (cameraETS_) {
        std::shared_ptr<PostProcessETS> pp = cameraETS_->GetPostProcess();
        if (pp) {
            auto pps = taihe::make_holder<PostProcessSettingsImpl, ScenePostProcessSettings::PostProcessSettings>(pp);
            return SceneNodes::PostProcessSettingsOrNull::make_postProcess(pps);
        }
    }
    return SceneNodes::PostProcessSettingsOrNull::make_nValue();
}

void CameraImpl::setPostProcess(::SceneNodes::PostProcessSettingsOrNull const &process)
{
    if (!cameraETS_) {
        WIDGET_LOGE("The camera is null when set post process settings");
        return;
    }
    if (process.holds_nValue()) {
        WIDGET_LOGI("Set post process to null");
        cameraETS_->SetPostProcess(nullptr);
        return;
    }
    std::shared_ptr<PostProcessETS> pp;
    ScenePostProcessSettings::PostProcessSettings ppValue = process.get_postProcess_ref();
    taihe::optional<int64_t> implOp = ppValue->getImpl();
    bool createNewOne = true;
    if (implOp.has_value()) {
        PostProcessSettingsImpl *ppsi = reinterpret_cast<PostProcessSettingsImpl *>(implOp.value());
        if (ppsi != nullptr) {
            WIDGET_LOGI("a post process settings that assosict with a camera");
            createNewOne = false;
            pp = ppsi->GetInternalSettings();
        }
    }
    if (createNewOne) {
        taihe::optional<ScenePostProcessSettings::ToneMappingSettings> tonemapOp = ppValue->getToneMapping();
        std::shared_ptr<TonemapETS> tonemap;
        if (tonemapOp.has_value()) {
            tonemap = ToneMappingSettingsImpl::CreateInternal(tonemapOp.value());
        } else {
            WIDGET_LOGI("CameraImpl::setPostProcess: disable tonemap");
            tonemap = nullptr;
        }

        taihe::optional<ScenePostProcessSettings::BloomSettings> bloomOp = ppValue->getBloom();
        std::shared_ptr<BloomETS> bloom;
        if (bloomOp.has_value()) {
            bloom = BloomSettingsImpl::CreateInternal(bloomOp.value());
        } else {
            WIDGET_LOGI("CameraImpl::setPostProcess: disable bloom");
            bloom = nullptr;
        }
        pp = std::make_shared<PostProcessETS>(tonemap, bloom);
    }
    cameraETS_->SetPostProcess(pp);
}

::SceneNodes::ColorOrNull CameraImpl::getClearColor()
{
    if (!cameraETS_) {
        return SceneNodes::ColorOrNull::make_nValue();
    }
    InvokeReturn<std::shared_ptr<Vec4Proxy>> color = cameraETS_->GetClearColor();
    if (color.error.empty()) {
        SceneTypes::Color sc = taihe::make_holder<ColorImpl, SceneTypes::Color>(color.value);
        return SceneNodes::ColorOrNull::make_color(sc);
    } else {
        return SceneNodes::ColorOrNull::make_nValue();
    }
}

void CameraImpl::setClearColor(::SceneNodes::ColorOrNull const &color)
{
    if (cameraETS_) {
        if (color.holds_color()) {
            const SceneTypes::Color colorValue = color.get_color_ref();
            cameraETS_->SetClearColor(true,
                BASE_NS::Math::Vec4(colorValue->getR(), colorValue->getG(), colorValue->getB(), colorValue->getA()));
        } else {
            cameraETS_->SetClearColor(false, BASE_NS::Math::ZERO_VEC4);
        }
    }
}

::taihe::array<::SceneTH::RaycastResult> CameraImpl::raycastSync(::SceneTypes::weak::Vec2 viewPosition,
    ::SceneTH::RaycastParameters const &params)
{
    if (!cameraETS_) {
        taihe::set_error("Invalid camera");
        return {};
    }
    BASE_NS::Math::Vec2 vp(viewPosition->getX(), viewPosition->getY());
    taihe::optional<SceneNodes::Node> rootNodeOp = params.rootNode;
    std::shared_ptr<NodeETS> rootNode{nullptr};
    if (rootNodeOp.has_value()) {
        auto nodeOptional = static_cast<::SceneResources::SceneResource>(rootNodeOp.value())->getImpl();
        if (!nodeOptional.has_value()) {
            taihe::set_error("invalid node in taihe object");
            return {};
        }
        auto ni = reinterpret_cast<NodeImpl *>(nodeOptional.value());
        if (ni != nullptr) {
            rootNode = ni->GetInternalNode();
        }
    }
    InvokeReturn<std::vector<CameraETS::RaycastResult>> result = cameraETS_->Raycast(vp, rootNode);
    if (result.error.empty()) {
        std::vector<CameraETS::RaycastResult> rrv = result.value;
        std::vector<::SceneTH::RaycastResult> rrVector;
        rrVector.reserve(rrv.size());
        for (size_t index = 0; index < rrv.size(); ++index) {
            rrVector.emplace_back(::SceneTH::RaycastResult{NodeImpl::MakeVariousNodes(rrv[index].node),
                rrv[index].centerDistance,
                taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(rrv[index].hitPosition)});
        }
        return taihe::array_view<::SceneTH::RaycastResult>(rrVector);
    } else {
        taihe::set_error(result.error);
        return {};
    }
}

::SceneTypes::Vec3 CameraImpl::worldToScreen(::SceneTypes::weak::Vec3 worldPosition)
{
    if (!cameraETS_) {
        WIDGET_LOGE("cameraETS_ is null");
        return SceneTypes::Vec3({nullptr, nullptr});
    }
    BASE_NS::Math::Vec3 world{worldPosition->getX(), worldPosition->getY(), worldPosition->getZ()};
    return taihe::make_holder<Vec3Impl, ::SceneTypes::Vec3>(cameraETS_->WorldToScreen(world));
}

::SceneTypes::Vec3 CameraImpl::screenToWorld(::SceneTypes::weak::Vec3 viewPosition)
{
    if (!cameraETS_) {
        WIDGET_LOGE("cameraETS_ is null");
        return SceneTypes::Vec3({nullptr, nullptr});
    }
    BASE_NS::Math::Vec3 screen{viewPosition->getX(), viewPosition->getY(), viewPosition->getZ()};
    return taihe::make_holder<Vec3Impl, ::SceneTypes::Vec3>(cameraETS_->ScreenToWorld(screen));
}

::SceneNodes::Camera cameraTransferStaticImpl(uintptr_t input)
{
    WIDGET_LOGI("cameraTransferStaticImpl");
    ani_object esValue = reinterpret_cast<ani_object>(input);
    void *nativePtr = nullptr;
    if (!arkts_esvalue_unwrap(taihe::get_env(), esValue, &nativePtr) || nativePtr == nullptr) {
        WIDGET_LOGE("unwrap esvalue failed");
        return SceneNodes::Camera({nullptr, nullptr});
    }
    TrueRootObject *tro = reinterpret_cast<TrueRootObject *>(nativePtr);
    if (tro == nullptr) {
        WIDGET_LOGE("transfer camera failed");
        return SceneNodes::Camera({nullptr, nullptr});
    }
    SCENE_NS::ICamera::Ptr cam = tro->GetNativeObject<SCENE_NS::ICamera>();
    return taihe::make_holder<CameraImpl, ::SceneNodes::Camera>(std::make_shared<CameraETS>(cam));
}

uintptr_t cameraTransferDynamicImpl(::SceneNodes::weak::Camera input)
{
    WIDGET_LOGI("cameraTransferDynamicImpl");
    RETURN_IF_NULL_WITH_VALUE(!input.is_error(), 0);
    taihe::optional<int64_t> implOp = static_cast<::SceneResources::weak::SceneResource>(input)->getImpl();
    RETURN_IF_NULL_WITH_VALUE(implOp.has_value(), 0);
    CameraImpl *camera = reinterpret_cast<CameraImpl *>(implOp.value());
    RETURN_IF_NULL_WITH_VALUE(camera, 0);
    std::shared_ptr<CameraETS> internalCamera = camera->getInternalCamera();
    RETURN_IF_NULL_WITH_VALUE(internalCamera, 0);
    META_NS::IObject::Ptr nativeObj = internalCamera->GetNativeObj();
    RETURN_IF_NULL_WITH_VALUE(nativeObj, 0);
    SCENE_NS::INode::Ptr cameraNode = interface_pointer_cast<SCENE_NS::INode>(nativeObj);
    RETURN_IF_NULL_WITH_VALUE(cameraNode, 0);
    napi_env jsenv;
    if (!arkts_napi_scope_open(taihe::get_env(), &jsenv)) {
        WIDGET_LOGE("arkts_napi_scope_open failed");
        return 0;
    }
    if (!CheckNapiEnv(jsenv)) {
        WIDGET_LOGE("CheckNapiEnv failed");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }
    auto sceneJs = CreateFromNativeInstance(jsenv, cameraNode->GetScene(), PtrType::STRONG, {});
    if (!sceneJs) {
        WIDGET_LOGE("create SceneJS failed.");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }

    // For some reason, the implementation of the 1.1 interface sets PostProcess to null when constructing CameraJS,
    // causing the post-processing configuration to be lost.
    // Here, we temporarily store it and restore it after the CameraJS construction is completed.
    auto nativeCamera = interface_cast<SCENE_NS::ICamera>(nativeObj);
    auto postProc = META_NS::GetValue(nativeCamera->PostProcess());
    auto toneMap = META_NS::GetValue(postProc->Tonemap());

    napi_value nullValue;
    napi_get_null(jsenv, &nullValue);
    napi_value args[] = {sceneJs.ToNapiValue(), nullValue};
    auto cameraObj = CreateFromNativeInstance(jsenv, cameraNode, PtrType::WEAK, args);
    if (!cameraObj) {
        WIDGET_LOGE("create CameraJS failed.");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }

    if (postProc) {
        // Restore the camera's post-processing configuration
        napi_value args[] = {cameraObj.ToNapiValue(), nullValue};
        auto postProcJS = CreateFromNativeInstance(jsenv, postProc, PtrType::WEAK, args);

        if (toneMap) {
            // temporarily store the postProc's toneMapping configuration
            auto type = toneMap->Type()->GetValue();
            auto exposure = toneMap->Exposure()->GetValue();
            // restore the postProc's toneMapping configuration
            auto toneMapJS = CreateFromNativeInstance(jsenv, toneMap, PtrType::WEAK, {});
            toneMapJS.Set("type", NapiApi::Value<uint32_t>(jsenv, static_cast<uint32_t>(type)));
            toneMapJS.Set("exposure", NapiApi::Value<float>(jsenv, exposure));
            postProcJS.Set("toneMapping", toneMapJS);
        }
        cameraObj.Set("postProcess", postProcJS);
    }

    napi_value cameraValue = cameraObj.ToNapiValue();
    ani_ref resAny;
    if (!arkts_napi_scope_close_n(jsenv, 1, &cameraValue, &resAny)) {
        WIDGET_LOGE("arkts_napi_scope_close_n failed");
        return 0;
    }
    return reinterpret_cast<uintptr_t>(resAny);
}
}  // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_cameraTransferStaticImpl(cameraTransferStaticImpl);
TH_EXPORT_CPP_API_cameraTransferDynamicImpl(cameraTransferDynamicImpl);
// NOLINTEND