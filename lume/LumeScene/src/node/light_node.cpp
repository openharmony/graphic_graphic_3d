/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "light_node.h"

#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <core/ecs/intf_ecs.h>

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity LightNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    // make directly light entity by hand, we don't want to upgrade (and wait fix) Lume3D to use sceneUtil.CreateLight
    // (will be used in future)
    auto ecs = scene->GetEcsContext().GetNativeEcs();

    auto lmm = CORE_NS::GetManager<CORE3D_NS::ILocalMatrixComponentManager>(*ecs);
    auto wmm = CORE_NS::GetManager<CORE3D_NS::IWorldMatrixComponentManager>(*ecs);
    auto ncm = CORE_NS::GetManager<CORE3D_NS::INodeComponentManager>(*ecs);
    auto nameM = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
    auto tcm = CORE_NS::GetManager<CORE3D_NS::ITransformComponentManager>(*ecs);
    auto lcm = CORE_NS::GetManager<CORE3D_NS::ILightComponentManager>(*ecs);
    if (!lmm || !wmm || !ncm || !nameM || !tcm || !lcm) {
        return {};
    }

    auto light = ecs->GetEntityManager().Create();

    lmm->Create(light);
    wmm->Create(light);
    ncm->Create(light);
    nameM->Create(light);
    nameM->Write(light)->name = "Light";
    tcm->Create(light);
    lcm->Create(light);

    return light;
}

bool LightNode::SetEcsObject(const IEcsObject::Ptr& o)
{
    if (Super::SetEcsObject(o)) {
        auto att = GetSelf<META_NS::IAttach>()->GetAttachments<ILight>();
        if (!att.empty()) {
            return true;
        }
    }
    return false;
}

SCENE_END_NAMESPACE()