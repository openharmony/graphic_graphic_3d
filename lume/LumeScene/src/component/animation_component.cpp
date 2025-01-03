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

#include "animation_component.h"

#include <scene/ext/intf_ecs_context.h>

#include <3d/ecs/components/animation_state_component.h>

SCENE_BEGIN_NAMESPACE()

BASE_NS::string AnimationComponent::GetName() const
{
    return "AnimationComponent";
}

bool AnimationComponent::SetEcsObject(const IEcsObject::Ptr& obj)
{
    bool ret = Super::SetEcsObject(obj);
    if (ret) {
        auto& ecsc = obj->GetScene()->GetEcsContext();
        if (auto m = ecsc.FindComponent<CORE3D_NS::AnimationStateComponent>()) {
            if (!m->HasComponent(GetEcsObject()->GetEntity())) {
                m->Create(GetEcsObject()->GetEntity());
            }
        }
    }
    return ret;
}

SCENE_END_NAMESPACE()
