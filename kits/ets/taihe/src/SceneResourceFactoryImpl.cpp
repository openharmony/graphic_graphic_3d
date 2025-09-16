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

#include <algorithm>

#include <base/math/vector.h>
#include <base/util/color.h>

#include "SceneResourceFactoryImpl.h"
#include "PBRMaterialImpl.h"
#include "ParamUtils.h"

#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "BaseObjectJS.h"
#include "nodejstaskqueue.h"
#include "SceneJS.h"

namespace OHOS::Render3D::KITETS {
::SceneNodes::Camera SceneResourceFactoryImpl::createCameraSync(::SceneTH::SceneNodeParameters const &params)
{
    if (!sceneETS_) {
        taihe::set_error("Invalid scene");
        return SceneNodes::Camera({nullptr, nullptr});
    }
    std::string nodePath = ExtractNodePath(params);
    WIDGET_LOGE("SceneResourceFactoryImpl::createCameraSync, nodePath: %{public}s", nodePath.c_str());
    InvokeReturn<std::shared_ptr<CameraETS>> camera = sceneETS_->CreateCamera(nodePath);
    if (camera.error.empty()) {
        return taihe::make_holder<CameraImpl, ::SceneNodes::Camera>(camera.value);
    } else {
        taihe::set_error(camera.error);
        return SceneNodes::Camera({nullptr, nullptr});
    }
}

::SceneNodes::LightTypeUnion SceneResourceFactoryImpl::createLightSync(
    ::SceneTH::SceneNodeParameters const &params, ::SceneNodes::LightType lightType)
{
    if (!sceneETS_) {
        taihe::set_error("Invalid scene");
        return SceneNodes::LightTypeUnion::make_base(SceneNodes::Light({nullptr, nullptr}));
    }
    std::string nodeName = ExtractNodeName(params);
    std::string nodePath = ExtractNodePath(params);
    LightETS::LightType type = LightETS::LightType(lightType.get_value());
    WIDGET_LOGE(
        "SceneResourceFactoryImpl::createLightSync, nodeName: %{public}s, nodePath: %{public}s, lightType: %{public}d",
        nodeName.c_str(),
        nodePath.c_str(),
        type);
    InvokeReturn<std::shared_ptr<LightETS>> light = sceneETS_->CreateLight(nodeName, nodePath, type);
    if (!light) {
        taihe::set_error(light.error);
        return SceneNodes::LightTypeUnion::make_base(SceneNodes::Light({nullptr, nullptr}));
    }
    switch (type) {
        case LightETS::LightType::DIRECTIONAL:
            return ::SceneNodes::LightTypeUnion::make_directional(
                taihe::make_holder<DirectionalLightImpl, ::SceneNodes::DirectionalLight>(light.value));
        case LightETS::LightType::SPOT:
            return ::SceneNodes::LightTypeUnion::make_spot(
                taihe::make_holder<SpotLightImpl, ::SceneNodes::SpotLight>(light.value));
        case LightETS::LightType::POINT:
            return ::SceneNodes::LightTypeUnion::make_base(
                taihe::make_holder<LightImpl, ::SceneNodes::Light>(light.value));
        default:
            return ::SceneNodes::LightTypeUnion::make_base(
                taihe::make_holder<LightImpl, ::SceneNodes::Light>(light.value));
    }
}

::SceneNodes::Node SceneResourceFactoryImpl::createNodeSync(::SceneTH::SceneNodeParameters const &params)
{
    if (!sceneETS_) {
        taihe::set_error("Invalid scene");
        return SceneNodes::Node({nullptr, nullptr});
    }
    std::string nodePath = ExtractNodePath(params);
    WIDGET_LOGE("SceneResourceFactoryImpl::createNodeSync, nodePath: %{public}s", nodePath.c_str());
    InvokeReturn<std::shared_ptr<NodeETS>> node = sceneETS_->CreateNode(nodePath);
    if (node.error.empty()) {
        return taihe::make_holder<NodeImpl, ::SceneNodes::Node>(node.value);
    } else {
        taihe::set_error(node.error);
        return SceneNodes::Node({nullptr, nullptr});
    }
}

::SceneResources::VariousMaterial SceneResourceFactoryImpl::createMaterialSync(
    ::SceneTH::SceneResourceParameters const &params, ::SceneResources::MaterialType materialType)
{
    if (!sceneETS_) {
        taihe::set_error("Invalid scene ets");
        auto mat = ::SceneResources::MetallicRoughnessMaterial({nullptr, nullptr});
        return ::SceneResources::VariousMaterial::make_metaRough(mat);
    }
    std::string name = std::string(params.name);
    std::string uri = ExtractUri(params.uri);
    MaterialETS::MaterialType type;
    if (materialType == ::SceneResources::MaterialType::key_t::SHADER) {
        type = MaterialETS::MaterialType::SHADER;
    } else {
        type = MaterialETS::MaterialType::METALLIC_ROUGHNESS;
    }
    InvokeReturn<std::shared_ptr<MaterialETS>> material = sceneETS_->CreateMaterial(name, uri, type);
    if (!material) {
        taihe::set_error(material.error);
        auto mat = ::SceneResources::PBRMaterial({nullptr, nullptr});
        return ::SceneResources::VariousMaterial::make_pbr(mat);
    }
    auto mat = ::taihe::make_holder<PBRMaterialImpl, ::SceneResources::PBRMaterial>(material.value);
    return ::SceneResources::VariousMaterial::make_pbr(mat);
}

::SceneResources::Environment SceneResourceFactoryImpl::createEnvironmentSync(
    ::SceneTH::SceneResourceParameters const &params)
{
    if (!sceneETS_) {
        taihe::set_error("Invalid scene");
        return ::SceneResources::Environment({nullptr, nullptr});
    }
    std::string name = ExtractResourceName(params);
    std::string uri = ExtractUri(params.uri);
    WIDGET_LOGI("createEnvironmentSync, SceneResource name: %{public}s uri: %{public}s", name.c_str(), uri.c_str());
    InvokeReturn<std::shared_ptr<EnvironmentETS>> environment = sceneETS_->CreateEnvironment(name, uri);
    if (environment.error.empty()) {
        return taihe::make_holder<EnvironmentImpl, ::SceneResources::Environment>(environment.value);
    } else {
        taihe::set_error(environment.error);
        return ::SceneResources::Environment({nullptr, nullptr});
    }
}

::SceneNodes::Geometry SceneResourceFactoryImpl::createGeometrySync(
    ::SceneTH::SceneNodeParameters const &params, ::SceneResources::weak::MeshResource mesh)
{
    if (!sceneETS_) {
        taihe::set_error("Invalid scene");
        return SceneNodes::Geometry({nullptr, nullptr});
    }
    auto meshOptional = static_cast<::SceneResources::weak::SceneResource>(mesh)->getImpl();
    if (!meshOptional.has_value()) {
        taihe::set_error("invalid mesh in taihe object");
        return SceneNodes::Geometry({nullptr, nullptr});
    }
    auto mri = reinterpret_cast<MeshResourceImpl*>(meshOptional.value());
    if (mri == nullptr || !(mri->mrETS_)) {
        taihe::set_error("Invalid MeshResource");
        return SceneNodes::Geometry({nullptr, nullptr});
    }
    std::string nodePath = ExtractNodePath(params);
    InvokeReturn<std::shared_ptr<GeometryETS>> geom = sceneETS_->CreateGeometry(nodePath, mri->mrETS_);
    if (geom.error.empty()) {
        return taihe::make_holder<GeometryImpl, ::SceneNodes::Geometry>(geom.value);
    } else {
        taihe::set_error(geom.error);
        return SceneNodes::Geometry({nullptr, nullptr});
    }
}

::SceneTH::SceneResourceFactory sceneResourceFactoryTransferStaticImpl(uintptr_t input)
{
    ani_object esValue = reinterpret_cast<ani_object>(input);
    void *nativePtr = nullptr;
    if (!arkts_esvalue_unwrap(taihe::get_env(), esValue, &nativePtr) || nativePtr == nullptr) {
        WIDGET_LOGE("unwrap esvalue failed");
        return SceneTH::SceneResourceFactory({nullptr, nullptr});
    }
    auto sceneJS = reinterpret_cast<SceneJS *>(nativePtr);
    if (!sceneJS) {
        WIDGET_LOGE("transfer SceneResourceFactory failed");
        return SceneTH::SceneResourceFactory({nullptr, nullptr});
    }
    SCENE_NS::IScene::Ptr scene = sceneJS->GetNativeObject<SCENE_NS::IScene>();
    std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter = sceneJS->scene_;
    auto sceneETS = std::make_shared<SceneETS>(scene, sceneAdapter);
    return taihe::make_holder<SceneResourceFactoryImpl, SceneTH::SceneResourceFactory>(sceneETS);
}

uintptr_t sceneResourceFactoryTransferDynamicImpl(::SceneTH::weak::SceneResourceFactory input)
{
    taihe::optional<int64_t> implOp = input->getImpl();
    if (!implOp.has_value()) {
        WIDGET_LOGE("get SceneResourceFactoryImpl failed");
        return 0;
    }
    SceneResourceFactoryImpl *srfi = reinterpret_cast<SceneResourceFactoryImpl *>(implOp.value());
    if (srfi == nullptr) {
        WIDGET_LOGE("can't cast to SceneResourceFactoryImpl");
        return 0;
    }
    std::shared_ptr<SceneETS> internalScene = srfi->getInternalScene();
    if (!internalScene) {
        WIDGET_LOGE("get SceneETS failed");
        return 0;
    }
    SCENE_NS::IScene::Ptr nativeObj = internalScene->GetNativeScene();
    if (!nativeObj) {
        WIDGET_LOGE("get IScene failed");
        return 0;
    }
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
    auto sceneJs = CreateFromNativeInstance(jsenv, nativeObj, PtrType::STRONG, {});
    if (!sceneJs) {
        WIDGET_LOGE("create SceneJS failed");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }
    napi_value sceneValue = sceneJs.ToNapiValue();
    ani_ref resAny;
    if (!arkts_napi_scope_close_n(jsenv, 1, &sceneValue, &resAny)) {
        WIDGET_LOGE("arkts_napi_scope_close_n failed");
        return 0;
    }
    return reinterpret_cast<uintptr_t>(resAny);
}
}  // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_sceneResourceFactoryTransferStaticImpl(sceneResourceFactoryTransferStaticImpl);
TH_EXPORT_CPP_API_sceneResourceFactoryTransferDynamicImpl(sceneResourceFactoryTransferDynamicImpl);
// NOLINTEND