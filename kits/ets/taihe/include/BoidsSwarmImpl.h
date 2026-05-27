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

#ifndef OHOS_3D_SCENE_BOIDSSWARM_IMPL_H
#define OHOS_3D_SCENE_BOIDSSWARM_IMPL_H

#include "SceneBoidsSwarm.proj.hpp"
#include "SceneBoidsSwarm.impl.hpp"
#include "taihe/runtime.hpp"
#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "SceneNodes.proj.hpp"
#include "SceneNodes.impl.hpp"
#include "SceneTypes.proj.hpp"
#include "SceneTypes.impl.hpp"

#include <scene/interface/intf_scene.h>

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D::KITETS {

class BoidsSimWorldImpl {
public:
    explicit BoidsSimWorldImpl(SCENE_NS::IScene::Ptr scene);
    ~BoidsSimWorldImpl();

    void play();
    void pause();
    void stop();
    bool getIsPlaying();

    void addBoidsSimComponent(::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimParameters params);
    void setBoidsSimComponent(::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimParameters params);
    ::SceneBoidsSwarm::BoidsSimParametersOrNull getBoidsSimComponent(::SceneNodes::weak::Node node);
    void removeBoidsSimComponent(::SceneNodes::weak::Node node);

    void addBoidsSimGravityComponent(
        ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimGravityParameters params);
    void setBoidsSimGravityComponent(
        ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimGravityParameters params);
    ::SceneBoidsSwarm::BoidsSimGravityParametersOrNull getBoidsSimGravityComponent(::SceneNodes::weak::Node node);
    void removeBoidsSimGravityComponent(::SceneNodes::weak::Node node);

    void addBoidsSimRepulsionComponent(
        ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimRepulsionParameters params);
    void setBoidsSimRepulsionComponent(
        ::SceneNodes::weak::Node node, ::SceneBoidsSwarm::BoidsSimRepulsionParameters params);
    ::SceneBoidsSwarm::BoidsSimRepulsionParametersOrNull getBoidsSimRepulsionComponent(::SceneNodes::weak::Node node);
    void removeBoidsSimRepulsionComponent(::SceneNodes::weak::Node node);

private:
    SCENE_NS::IScene::Ptr scene_;

    void RemoveComponent(const SCENE_NS::INode::Ptr& node, BASE_NS::string_view compName, const BASE_NS::Uid& uid);
};

::SceneBoidsSwarm::BoidsSimWorldOrNull getDefaultBoidsSimWorld(::SceneTH::weak::Scene scene);

}  // namespace OHOS::Render3D::KITETS

#endif  // OHOS_3D_SCENE_BOIDSSWARM_IMPL_H
