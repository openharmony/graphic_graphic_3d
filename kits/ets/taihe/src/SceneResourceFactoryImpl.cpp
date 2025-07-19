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
#include "ParamUtils.h"

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

::SceneResources::Environment SceneResourceFactoryImpl::createEnvironmentSync(
    ::SceneTH::SceneResourceParameters const &params)
{
    if (!sceneETS_) {
        taihe::set_error("Invalid scene");
        return ::SceneResources::Environment({nullptr, nullptr});
    }
    std::string name = ExtractResourceName(params);
    std::string uri = OHOS::Render3D::ExtractUri(params.uri);
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
    MeshResourceImpl *mri = reinterpret_cast<MeshResourceImpl *>(mesh->GetImpl());
    if (mri == nullptr || !(mri->mrETS_)) {
        taihe::set_error("Invalid MeshResource");
        return SceneNodes::Geometry({nullptr, nullptr});
    }
    std::string nodePath = ExtractNodePath(params);
    WIDGET_LOGE("SceneResourceFactoryImpl::createGeometrySync, nodePath: %{public}s", nodePath.c_str());
    InvokeReturn<std::shared_ptr<GeometryETS>> geom = sceneETS_->CreateGeometry(nodePath, mri->mrETS_);
    if (geom.error.empty()) {
        return taihe::make_holder<GeometryImpl, ::SceneNodes::Geometry>(geom.value);
    } else {
        taihe::set_error(geom.error);
        return SceneNodes::Geometry({nullptr, nullptr});
    }
}