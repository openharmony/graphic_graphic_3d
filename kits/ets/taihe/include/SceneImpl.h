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

#ifndef OHOS_3D_SCENE_IMPL_H
#define OHOS_3D_SCENE_IMPL_H

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "SceneTH.Transfer.proj.hpp"
#include "SceneTH.Transfer.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "SceneResources.user.hpp"
#include "SceneNodes.user.hpp"

#include "AnimationImpl.h"
#include "SceneComponentImpl.h"
#include "SceneResourceFactoryImpl.h"
#include "SceneETS.h"

#include "scene_adapter/intf_scene_adapter.h"
#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D::KITETS {
class SceneImpl {
private:
    std::shared_ptr<SceneETS> sceneETS_;
    std::optional<::taihe::array<::SceneResources::Animation>> animations_;

public:
    explicit SceneImpl(const std::string &uriStr);

    SceneImpl(SCENE_NS::IScene::Ptr scene, std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter);

    ~SceneImpl();

    ::SceneResources::Environment getEnvironment();

    void setEnvironment(::SceneResources::weak::Environment env);

    ::taihe::array<::SceneResources::Animation> getAnimations();

    ::SceneNodes::NodeOrNull getRoot();

    ::SceneNodes::VariousNodesOrNull getNodeByPath(
        ::taihe::string_view path, ::taihe::optional_view<::SceneNodes::NodeType> type);

    ::SceneTH::SceneResourceFactory getResourceFactory()
    {
        return taihe::make_holder<SceneResourceFactoryImpl, ::SceneTH::SceneResourceFactory>(sceneETS_);
    }

    void destroy();

    ::SceneNodes::Node importNode(
        ::taihe::string_view name, ::SceneNodes::weak::Node node, ::SceneNodes::NodeOrNull parent);

    ::SceneNodes::Node importScene(
        ::taihe::string_view name, ::SceneTH::weak::Scene scene, ::SceneNodes::NodeOrNull parent);

    bool renderFrame(::taihe::optional_view<::SceneTH::RenderParameters> params);

    ::SceneTH::SceneComponent createComponentSync(::SceneNodes::weak::Node node, ::taihe::string_view name);

    ::SceneTH::SceneComponentOrNull getComponent(::SceneNodes::weak::Node node, ::taihe::string_view name);

    ::SceneTH::RenderConfiguration getRenderConfiguration();

    int64_t getSceneNative();

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<uintptr_t>(this));
    }

    std::shared_ptr<SceneETS> getInternalScene() const
    {
        return sceneETS_;
    }
};

::SceneTH::RenderContextOrNull getDefaultRenderContext();

::SceneTH::Scene loadScene(::taihe::optional_view<uintptr_t> uri);

::SceneTH::Scene sceneTransferStaticImpl(uintptr_t input);

uintptr_t sceneTransferDynamicImpl(::SceneTH::weak::Scene input);
} // namespace OHOS::Render3D::KITETS

#endif  // OHOS_3D_SCENE_IMPL_H