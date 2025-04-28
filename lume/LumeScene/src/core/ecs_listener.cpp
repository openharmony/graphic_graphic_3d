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

#include "ecs_listener.h"

SCENE_BEGIN_NAMESPACE()

bool EcsListener::Initialize(IEcsContext& context)
{
    ecs_ = &context;
    ecs_->GetNativeEcs()->AddListener((CORE_NS::IEcs::EntityListener&)*this);
    ecs_->GetNativeEcs()->AddListener((CORE_NS::IEcs::ComponentListener&)*this);
    return true;
}
void EcsListener::Uninitialize()
{
    ecs_->GetNativeEcs()->RemoveListener((CORE_NS::IEcs::EntityListener&)*this);
    ecs_->GetNativeEcs()->RemoveListener((CORE_NS::IEcs::ComponentListener&)*this);
}

void EcsListener::RegisterEcsObject(const IEcsObject::Ptr& p)
{
    objects_[p->GetEntity()] = p;
}
void EcsListener::DeregisterEcsObject(const IEcsObject::ConstPtr& p)
{
    DeregisterEcsObject(p->GetEntity());
}
void EcsListener::DeregisterEcsObject(CORE_NS::Entity ent)
{
    objects_.erase(ent);
}
IEcsObject::Ptr EcsListener::FindEcsObject(CORE_NS::Entity ent) const
{
    auto it = objects_.find(ent);
    return it != objects_.end() ? it->second.lock() : nullptr;
}

void EcsListener::OnEntityEvent(
    CORE_NS::IEcs::EntityListener::EventType type, BASE_NS::array_view<const CORE_NS::Entity> entities)
{
    for (auto&& ent : entities) {
        if (auto obj = interface_pointer_cast<IEcsEventListener>(FindEcsObject(ent))) {
            obj->OnEntityEvent(type);
        }
        if (type == CORE_NS::IEcs::EntityListener::EventType::DESTROYED) {
            DeregisterEcsObject(ent);
        }
    }
}
void EcsListener::OnComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
    const CORE_NS::IComponentManager& manager, BASE_NS::array_view<const CORE_NS::Entity> entities)
{
    for (auto&& ent : entities) {
        if (auto obj = interface_pointer_cast<IEcsEventListener>(FindEcsObject(ent))) {
            obj->OnComponentEvent(type, manager);
        }
    }
}

SCENE_END_NAMESPACE()
