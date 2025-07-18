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

using namespace OHOS::Render3D;

SceneImpl::SceneImpl(const std::string &uriStr)
{
    WIDGET_LOGI("SceneImpl ctor");
    sceneETS_ = std::make_shared<SceneETS>();
    if (!sceneETS_->Load(uriStr)) {
        ::taihe::set_error("ohos_lume loadScene fail");
    }
}

bool SceneImpl::renderFrame(::taihe::optional_view<::SceneTH::RenderParameters> params)
{
    WIDGET_LOGI("SceneImpl renderFrame");
    SceneETS::RenderParameters renderParams = ExtractRenderParameters(params);
    return sceneETS_->RenderFrame(renderParams);;
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

int64_t SceneImpl::getSceneNative()
{
    WIDGET_LOGE("ace_lume getSceneNative ");
    auto scene = sceneETS_->GetSceneAdapter();
    return static_cast<int64_t>(reinterpret_cast<uintptr_t>(scene));
}

void SceneImpl::destroy()
{
    WIDGET_LOGI("scene.destroy");
    sceneETS_->Destroy();
}

::SceneNodes::NodeOrNull SceneImpl::getRoot()
{
    WIDGET_LOGI("scene.getRoot");
    //     if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
    //     SCENE_NS::INode::Ptr root = scene->GetRootNode().GetResult();

    //     NapiApi::StrongRef sceneRef { ctx.This() };
    //     if (!sceneRef.GetObject().GetNative<SCENE_NS::IScene>()) {
    //         LOG_F("INVALID SCENE!");
    //     }

    //     NapiApi::Object argJS(ctx.GetEnv());
    //     napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };

    //     // Store a weak ref, as these are owned by the scene.
    //     auto js = CreateFromNativeInstance(ctx.GetEnv(), root, PtrType::WEAK, args);
    //     if (auto node = js.GetJsWrapper<NodeImpl>()) {
    //         node->Attached(true);
    //     }
    //     return js.ToNapiValue();
    // }
    // return ctx.GetUndefined();
    auto node = sceneETS_->GetRoot();
    return SceneNodes::NodeOrNull::make_node(taihe::make_holder<NodeImpl, SceneNodes::Node>(node.value));
}

::SceneNodes::NodeOrNull SceneImpl::getNodeByPath(::taihe::string_view path, ::taihe::optional_view<::SceneNodes::NodeType> type)
{
    WIDGET_LOGI("scene.getNodeByPath");
    if (!type || !type->is_valid()) {
        WIDGET_LOGE("scene.getNodeByPath invalid node type");
        // currently ignore the type
        // return ::SceneNodes::NodeOrNull::make_nValue();
    }
    // auto node = sceneETS_->GetNodeByPath(path.c_str(), type.get_value());
    // std::shared_ptr<NodeETS> node = nodeETS_->GetNodeByPath(std::string(path));
    const auto& root = getRoot();
    if (!root.holds_node()) {
        WIDGET_LOGE("scene.getNodeByPath invalid root");
        return ::SceneNodes::NodeOrNull::make_nValue();
    }
    // currently ignore the type
    // return SceneNodes::NodeOrNull::make_node(taihe::make_holder<NodeImpl, SceneNodes::Node>(root.get_node_ref().getNodeByPath(path)));
    // return ::SceneNodes::NodeOrNull::make_node();
    return root.get_node_ref()->getNodeByPath(path);
}

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_getDefaultRenderContext(getDefaultRenderContext);
TH_EXPORT_CPP_API_loadScene(loadScene);
// NOLINTEND