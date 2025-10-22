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
#include "CheckNapiEnv.h"
#include "RenderContextImpl.h"
#include "scene_adapter/scene_adapter.h"

namespace OHOS::Render3D::KITETS {

SceneImpl::SceneImpl(const std::string &uriStr)
{
    sceneETS_ = std::make_shared<SceneETS>();
    if (!sceneETS_->Load(uriStr)) {
        WIDGET_LOGE("ohos_lume loadScene fail");
    }
}

SceneImpl::SceneImpl(SCENE_NS::IScene::Ptr scene, std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter)
{
    sceneETS_ = std::make_shared<SceneETS>(scene, sceneAdapter);
    if (!sceneETS_) {
        WIDGET_LOGE("ohos_lume loadScene fail");
    }
}

SceneImpl::~SceneImpl()
{
    destroy();
}

bool SceneImpl::renderFrame(::taihe::optional_view<::SceneTH::RenderParameters> params)
{
    if (!sceneETS_) {
        return false;
    }
    SceneETS::RenderParameters renderParams = ExtractRenderParameters(params);
    return sceneETS_->RenderFrame(renderParams);
}

::SceneResources::Environment SceneImpl::getEnvironment()
{
    if (!sceneETS_) {
        return ::SceneResources::Environment({nullptr, nullptr});
    }
    InvokeReturn<std::shared_ptr<EnvironmentETS>> environemnt = sceneETS_->GetEnvironment();
    if (environemnt) {
        return taihe::make_holder<EnvironmentImpl, ::SceneResources::Environment>(environemnt.value);
    } else {
        WIDGET_LOGE("getEnvironment error: %s", environemnt.error.c_str());
        return ::SceneResources::Environment({nullptr, nullptr});
    }
}

void SceneImpl::setEnvironment(::SceneResources::weak::Environment env)
{
    if (!sceneETS_ || env.is_error()) {
        return;
    }
    auto envOptional = static_cast<::SceneResources::weak::SceneResource>(env)->getImpl();
    if (!envOptional.has_value()) {
        WIDGET_LOGE("invalid environment in taihe object");
        return;
    }
    auto envImpl = reinterpret_cast<EnvironmentImpl*>(envOptional.value());
    if (envImpl) {
        sceneETS_->SetEnvironment(envImpl->GetEnvETS());
    } else {
        WIDGET_LOGE("Invalid environment in setEnvironment");
    }
}

::taihe::array<::SceneResources::Animation> SceneImpl::getAnimations()
{
    if (!sceneETS_) {
        return {};
    }
    if (animations_) {
        return animations_.value();
    }
    std::vector<std::shared_ptr<AnimationETS>> animationETSlist =
        sceneETS_->GetAnimations();  // use unique_ptr instead in the future

    std::vector<::SceneResources::Animation> result;
    for (const auto& animationETS : animationETSlist) {
        result.emplace_back(AnimationImpl::createAnimationFromETS(animationETS));
    }

    animations_ = taihe::array<::SceneResources::Animation>(result);
    return animations_.value();
}

::SceneTH::RenderContextOrNull getDefaultRenderContext()
{
    if (!OHOS::Render3D::SceneAdapter::IsEngineInitSuccessful()) {
        return ::SceneTH::RenderContextOrNull::make_nValue();
    }
    auto rc = taihe::make_holder<RenderContextImpl, ::SceneTH::RenderContext>();
    return ::SceneTH::RenderContextOrNull::make_rc(rc);
}

::SceneTH::Scene loadScene(::taihe::optional_view<uintptr_t> uri)
{
    std::string uriStr = ExtractUri(uri);
    if (uriStr.empty()) {
        uriStr = "scene://empty";
    }
    ::SceneTH::Scene scene = taihe::make_holder<SceneImpl, ::SceneTH::Scene>(uriStr);
    return scene;
}

::SceneNodes::Node SceneImpl::importNode(::taihe::string_view name, ::SceneNodes::weak::Node node,
    ::SceneNodes::NodeOrNull parent)
{
    if (!sceneETS_ || node.is_error()) {
        return ::SceneNodes::Node({nullptr, nullptr});
    }
    auto nodeOptional = static_cast<::SceneResources::weak::SceneResource>(node)->getImpl();
    if (!nodeOptional.has_value()) {
        WIDGET_LOGE("invalid node in taihe object");
        return ::SceneNodes::Node({nullptr, nullptr});
    }
    auto nodeImpl = reinterpret_cast<NodeImpl*>(nodeOptional.value());
    if (!nodeImpl) {
        WIDGET_LOGE("Invalid node in importNode");
        return ::SceneNodes::Node({nullptr, nullptr});
    }
    std::shared_ptr<NodeETS> res{nullptr};
    if (parent.holds_node()) {
        auto parentNodeOptional = static_cast<::SceneResources::weak::SceneResource>(parent.get_node_ref())->getImpl();
        if (!parentNodeOptional.has_value()) {
            WIDGET_LOGE("invalid node in taihe object");
            return ::SceneNodes::Node({nullptr, nullptr});
        }
        auto parentNodeImpl = reinterpret_cast<NodeImpl*>(parentNodeOptional.value());
        if (!parentNodeImpl) {
            WIDGET_LOGE("Invalid parent node in importNode");
            return ::SceneNodes::Node({nullptr, nullptr});
        }
        res = sceneETS_->ImportNode(std::string(name), nodeImpl->GetInternalNode(), parentNodeImpl->GetInternalNode());
    } else {
        res = sceneETS_->ImportNode(std::string(name), nodeImpl->GetInternalNode(), nullptr);
    }
    if (!res) {
        WIDGET_LOGE("ImportNode fail with null value");
        return ::SceneNodes::Node({nullptr, nullptr});
    }
    animations_ = std::nullopt;
    return taihe::make_holder<NodeImpl, ::SceneNodes::Node>(res);
}

::SceneNodes::Node SceneImpl::importScene(::taihe::string_view name, ::SceneTH::weak::Scene scene,
    ::SceneNodes::NodeOrNull parent)
{
    if (!sceneETS_ || scene.is_error()) {
        return ::SceneNodes::Node({nullptr, nullptr});
    }
    ::taihe::optional<int64_t> sceneOptional = scene->getImpl();
    if (!sceneOptional.has_value()) {
        WIDGET_LOGE("Invalid scene in importScene");
        return ::SceneNodes::Node({nullptr, nullptr});
    }
    auto sceneImpl = reinterpret_cast<SceneImpl*>(sceneOptional.value());
    std::shared_ptr<NodeETS> res{nullptr};
    if (parent.holds_node()) {
        auto parentNodeOptional = static_cast<::SceneResources::weak::SceneResource>(parent.get_node_ref())->getImpl();
        if (!parentNodeOptional.has_value()) {
            WIDGET_LOGE("invalid node in taihe object");
            return ::SceneNodes::Node({nullptr, nullptr});
        }
        auto parentNodeImpl = reinterpret_cast<NodeImpl*>(parentNodeOptional.value());
        if (!parentNodeImpl) {
            WIDGET_LOGE("Invalid parent node in importNode");
            return ::SceneNodes::Node({nullptr, nullptr});
        }
        res =
            sceneETS_->ImportScene(std::string(name), sceneImpl->getInternalScene(), parentNodeImpl->GetInternalNode());
    } else {
        res = sceneETS_->ImportScene(std::string(name), sceneImpl->getInternalScene(), nullptr);
    }
    if (!res) {
        WIDGET_LOGE("ImportNode fail with null value");
        return ::SceneNodes::Node({nullptr, nullptr});
    }
    animations_ = std::nullopt;
    return taihe::make_holder<NodeImpl, ::SceneNodes::Node>(res);
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
    if (!CheckNapiEnv(jsenv)) {
        WIDGET_LOGE("CheckNapiEnv failed");
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
    if (!sceneETS_) {
        return 0;
    }
    auto scene = sceneETS_->GetSceneAdapter();
    return static_cast<int64_t>(reinterpret_cast<uintptr_t>(scene));
}

void SceneImpl::destroy()
{
    if (sceneETS_) {
        sceneETS_->Destroy();
        sceneETS_.reset();
    }
    animations_ = std::nullopt;
}

::SceneNodes::NodeOrNull SceneImpl::getRoot()
{
    if (!sceneETS_) {
        return SceneNodes::NodeOrNull::make_nValue();
    }
    auto node = sceneETS_->GetRoot();
    return SceneNodes::NodeOrNull::make_node(taihe::make_holder<NodeImpl, SceneNodes::Node>(node.value));
}

::SceneNodes::VariousNodesOrNull SceneImpl::getNodeByPath(
    ::taihe::string_view path, ::taihe::optional_view<::SceneNodes::NodeType> type)
{
    if (!type || !type->is_valid()) {
        WIDGET_LOGE("scene.getNodeByPath invalid node type");
        // currently ignore the type
    }
    std::shared_ptr<NodeETS> node = nullptr;
    if (sceneETS_) {
        node = sceneETS_->GetNodeByPath(std::string(path));
    }
    return NodeImpl::MakeVariousNodesOrNull(node);
}

::SceneTH::SceneComponent SceneImpl::createComponentSync(::SceneNodes::weak::Node node, ::taihe::string_view name)
{
    if (!sceneETS_ || node.is_error()) {
        taihe::set_error("Invalid parameters given");
        return ::SceneTH::SceneComponent({nullptr, nullptr});
    }
    auto nodeOptional = static_cast<::SceneResources::weak::SceneResource>(node)->getImpl();
    if (!nodeOptional.has_value()) {
        taihe::set_error("invalid node in taihe object");
        return ::SceneTH::SceneComponent({nullptr, nullptr});
    }
    auto nodeImpl = reinterpret_cast<NodeImpl*>(nodeOptional.value());
    if (!nodeImpl) {
        taihe::set_error("Invalid node in createComponent");
        return ::SceneTH::SceneComponent({nullptr, nullptr});
    }
    auto component = sceneETS_->CreateComponent(nodeImpl->GetInternalNode(), std::string(name));
    if (component) {
        return taihe::make_holder<SceneComponentImpl, ::SceneTH::SceneComponent>(component.value);
    } else {
        taihe::set_error(component.error);
        return ::SceneTH::SceneComponent({nullptr, nullptr});
    }
}

::SceneTH::SceneComponentOrNull SceneImpl::getComponent(::SceneNodes::weak::Node node, ::taihe::string_view name)
{
    if (!sceneETS_ || node.is_error()) {
        return ::SceneTH::SceneComponentOrNull::make_nValue();
    }
    auto nodeOptional = static_cast<::SceneResources::weak::SceneResource>(node)->getImpl();
    if (!nodeOptional.has_value()) {
        WIDGET_LOGE("invalid node in taihe object");
        return ::SceneTH::SceneComponentOrNull::make_nValue();
    }
    auto nodeImpl = reinterpret_cast<NodeImpl*>(nodeOptional.value());
    if (!nodeImpl) {
        WIDGET_LOGE("Invalid node in getComponent");
        return ::SceneTH::SceneComponentOrNull::make_nValue();
    }
    auto component = sceneETS_->GetComponent(nodeImpl->GetInternalNode(), std::string(name));
    if (component) {
        auto holder = taihe::make_holder<SceneComponentImpl, ::SceneTH::SceneComponent>(component.value);
        return ::SceneTH::SceneComponentOrNull::make_sc(holder);
    } else {
        WIDGET_LOGE("getComponent error: %s", component.error.c_str());
        return ::SceneTH::SceneComponentOrNull::make_nValue();
    }
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