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

#include "SceneImpl.h"

#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "SceneJS.h"
#include "TransferEnvironment.h"

namespace OHOS::Render3D::KITETS {

SceneImpl::SceneImpl(const std::string &uriStr)
{
    WIDGET_LOGI("SceneImpl ctor");
    sceneETS_ = std::make_shared<SceneETS>();
    if (!sceneETS_->Load(uriStr)) {
        ::taihe::set_error("ohos_lume loadScene fail");
    }
}

SceneImpl::SceneImpl(SCENE_NS::IScene::Ptr scene, std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter)
{
    WIDGET_LOGI("SceneImpl transfer ctor");
    sceneETS_ = std::make_shared<SceneETS>(scene, sceneAdapter);
    if (!sceneETS_) {
        ::taihe::set_error("ohos_lume loadScene fail");
    }
}

SceneImpl::~SceneImpl()
{
    WIDGET_LOGI("SceneImpl dtor");
    destroy();
}

bool SceneImpl::renderFrame(::taihe::optional_view<::SceneTH::RenderParameters> params)
{
    WIDGET_LOGI("SceneImpl renderFrame");
    SceneETS::RenderParameters renderParams = ExtractRenderParameters(params);
    return sceneETS_->RenderFrame(renderParams);
}

::SceneResources::Environment SceneImpl::getEnvironment()
{
    InvokeReturn<std::shared_ptr<EnvironmentETS>> environemnt = sceneETS_->GetEnvironment();
    if (environemnt) {
        return taihe::make_holder<EnvironmentImpl, ::SceneResources::Environment>(environemnt.value);
    } else {
        taihe::set_error(environemnt.error);
        return ::SceneResources::Environment({nullptr, nullptr});
    }
}

void SceneImpl::setEnvironment(::SceneResources::weak::Environment env)
{
    auto environment = reinterpret_cast<EnvironmentImpl *>(env->GetImpl());
    if (environment) {
        sceneETS_->SetEnvironment(environment->GetEnvETS());
    } else {
        taihe::set_error("Invalid environment in setEnvironment");
    }
}

::taihe::array<::SceneResources::Animation> SceneImpl::getAnimations()
{
    WIDGET_LOGE("SceneImpl::getAnimations()");
    std::vector<std::shared_ptr<AnimationETS>> animationETSlist =
        sceneETS_->GetAnimations();  // use unique_ptr instead in the future

    std::vector<::SceneResources::Animation> result;
    for (const auto& animationETS : animationETSlist) {
        result.emplace_back(AnimationImpl::createAnimationFromETS(animationETS));
    }

    return taihe::array<::SceneResources::Animation>(result);
}

::SceneTH::RenderContextOrNull getDefaultRenderContext()
{
    TH_THROW(std::runtime_error, "getDefaultRenderContext not implemented");
}

::SceneTH::Scene loadScene(::taihe::optional_view<uintptr_t> uri)
{
    WIDGET_LOGI("loadScene begin");
    std::string uriStr = ExtractUri(uri);
    if (uriStr.empty()) {
        uriStr = "scene://empty";
    }
    ::SceneTH::Scene scene = taihe::make_holder<SceneImpl, ::SceneTH::Scene>(uriStr);
    return scene;
}

::SceneTH::Scene sceneTransferStaticImpl(uintptr_t input)
{
    WIDGET_LOGI("sceneTransferStaticImpl");
    ani_object esValue = reinterpret_cast<ani_object>(input);
    void *nativePtr = nullptr;
    if (!arkts_esvalue_unwrap(taihe::get_env(), esValue, &nativePtr) || nativePtr == nullptr) {
        WIDGET_LOGE("unwrap esvalue failed");
        return ::SceneTH::Scene({nullptr, nullptr});
    }
    auto sceneJS = reinterpret_cast<SceneJS *>(nativePtr);
    if (!sceneJS) {
        WIDGET_LOGE("transfer scene failed");
        taihe::set_error("transfer scene failed");
        return ::SceneTH::Scene({nullptr, nullptr});
    }
    SCENE_NS::IScene::Ptr scene = sceneJS->GetNativeObject<SCENE_NS::IScene>();
    std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter = sceneJS->scene_;
    return taihe::make_holder<SceneImpl, ::SceneTH::Scene>(scene, sceneAdapter);
}

uintptr_t sceneTransferDynamicImpl(::SceneTH::weak::Scene input)
{
    WIDGET_LOGI("sceneTransferDynamicImpl");
    if (input.is_error()) {
        WIDGET_LOGE("null input scene vtbl_ptr");
        return 0;
    }
    taihe::optional<int64_t> implOp = input->getImpl();
    if (!implOp.has_value()) {
        WIDGET_LOGE("get SceneImpl failed");
        return 0;
    }
    SceneImpl *si = reinterpret_cast<SceneImpl *>(implOp.value());
    if (si == nullptr) {
        WIDGET_LOGE("can't cast to SceneImpl");
        return 0;
    }
    std::shared_ptr<SceneETS> internalScene = si->getInternalScene();
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
    if (!TransferEnvironment::check(jsenv)) {
        WIDGET_LOGE("TransferEnvironment check failed");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }
    auto sceneJs = CreateFromNativeInstance(jsenv, nativeObj, PtrType::STRONG, {});
    if (!sceneJs) {
        WIDGET_LOGE("CreateFromNativeInstance failed");
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

int64_t SceneImpl::getSceneNative()
{
    WIDGET_LOGE("ace_lume getSceneNative ");
    auto scene = sceneETS_->GetSceneAdapter();
    return static_cast<int64_t>(reinterpret_cast<uintptr_t>(scene));
}

void SceneImpl::destroy()
{
    if (sceneETS_) {
        WIDGET_LOGI("scene.destroy");
        sceneETS_->Destroy();
        sceneETS_.reset();
    }
}

::SceneNodes::NodeOrNull SceneImpl::getRoot()
{
    WIDGET_LOGI("scene.getRoot");
    auto node = sceneETS_->GetRoot();
    return SceneNodes::NodeOrNull::make_node(taihe::make_holder<NodeImpl, SceneNodes::Node>(node.value));
}

::SceneNodes::VariousNodesOrNull SceneImpl::getNodeByPathInner(
    ::taihe::string_view path, ::taihe::optional_view<::SceneNodes::NodeType> type)
{
    if (!type || !type->is_valid()) {
        WIDGET_LOGE("scene.getNodeByPath invalid node type");
        // currently ignore the type
    }
    const auto &root = getRoot();
    if (!root.holds_node()) {
        WIDGET_LOGE("scene.getNodeByPath invalid root");
        return ::SceneNodes::VariousNodesOrNull::make_nValue();
    }
    // currently ignore the type
    return root.get_node_ref()->getNodeByPathInner(path);
}
} // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_getDefaultRenderContext(getDefaultRenderContext);
TH_EXPORT_CPP_API_loadScene(loadScene);
TH_EXPORT_CPP_API_sceneTransferStaticImpl(sceneTransferStaticImpl);
TH_EXPORT_CPP_API_sceneTransferDynamicImpl(sceneTransferDynamicImpl);
// NOLINTEND