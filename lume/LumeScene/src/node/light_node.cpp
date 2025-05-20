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
#include <3d/util/intf_scene_util.h>
#include <base/math/quaternion_util.h>

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity LightNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& sceneUtil = scene->GetGraphicsContext().GetSceneUtil();
    CORE3D_NS::LightComponent lc;
    return sceneUtil.CreateLight(*scene->GetEcsContext().GetNativeEcs(), lc, BASE_NS::Math::Vec3(0.0f, 0.0f, 0.0f),
        BASE_NS::Math::Quat(0.f, 0.f, 0.f, 1.f));
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