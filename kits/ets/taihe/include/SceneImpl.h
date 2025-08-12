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
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "SceneResources.user.hpp"
#include "SceneNodes.user.hpp"

#include "AnimationImpl.h"
#include "SceneResourceFactoryImpl.h"
#include "SceneETS.h"

#include "scene_adapter/intf_scene_adapter.h"
#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

class SceneComponentImpl {
public:
    SceneComponentImpl()
    {
        // Don't forget to implement the constructor.
    }

    ::taihe::string getName()
    {
        TH_THROW(std::runtime_error, "getName not implemented");
    }

    void setName(::taihe::string_view name)
    {
        TH_THROW(std::runtime_error, "setName not implemented");
    }

    ::taihe::map<::taihe::string, ::SceneTH::ComponentPropertyType> getProperty()
    {
        TH_THROW(std::runtime_error, "getProperty not implemented");
    }
};

class SceneImpl {
private:
    std::shared_ptr<OHOS::Render3D::SceneETS> sceneETS_;

public:
    SceneImpl(const std::string &uriStr);

    ::SceneResources::Environment getEnvironment();

    void setEnvironment(::SceneResources::weak::Environment env);

    ::taihe::array<::SceneResources::Animation> getAnimations()
    {
        WIDGET_LOGE("SceneImpl::getAnimations()"); // move implementation to SceneImpl.cpp
        std::vector<std::shared_ptr<AnimationETS>> animationETSlist = sceneETS_->GetAnimations();

        std::vector<::SceneResources::Animation> result;
        for (const auto& animationETS : animationETSlist) {
            result.emplace_back(AnimationImpl::createAnimationFromETS(animationETS));
        }

        return taihe::array<::SceneResources::Animation>(result);
    }

    ::SceneNodes::NodeOrNull getRoot();

    ::SceneNodes::NodeOrNull getNodeByPath(::taihe::string_view path,
                                           ::taihe::optional_view<::SceneNodes::NodeType> type);

    ::SceneTH::SceneResourceFactory getResourceFactory()
    {
        return taihe::make_holder<SceneResourceFactoryImpl, ::SceneTH::SceneResourceFactory>(sceneETS_);
    }

    void destroy();

    ::SceneNodes::Node importNode(
        ::taihe::string_view name, ::SceneNodes::weak::Node node, ::SceneNodes::NodeOrNull parent)
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        TH_THROW(std::runtime_error, "importNode not implemented");
    }

    ::SceneNodes::Node importScene(
        ::taihe::string_view name, ::SceneTH::weak::Scene scene, ::SceneNodes::NodeOrNull parent)
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        TH_THROW(std::runtime_error, "importScene not implemented");
    }

    bool renderFrame(::taihe::optional_view<::SceneTH::RenderParameters> params);

    ::SceneTH::SceneComponent createComponentSync(::SceneNodes::weak::Node node, ::taihe::string_view name)
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<SceneComponentImpl, ::SceneTH::SceneComponent>();
    }

    ::SceneTH::SceneComponentOrNull getComponent(::SceneNodes::weak::Node node, ::taihe::string_view name)
    {
        TH_THROW(std::runtime_error, "getComponent not implemented");
    }

    int64_t getSceneNative();

    // std::shared_ptr<ISceneAdapter> GetSceneAdapter();
};

::SceneTH::RenderContextOrNull getDefaultRenderContext();

::SceneTH::Scene loadScene(::taihe::optional_view<uintptr_t> uri);

#endif  // OHOS_3D_SCENE_IMPL_H