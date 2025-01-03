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

#include "environment.h"

#include <3d/ecs/components/environment_component.h>
#include <core/ecs/intf_entity_manager.h>

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity Environment::CreateEntity(const IInternalScene::Ptr& scene)
{
    auto& ecs = scene->GetEcsContext();
    auto envm = ecs.FindComponent<CORE3D_NS::EnvironmentComponent>();
    if (!envm) {
        return CORE_NS::Entity {};
    }
    auto ent = ecs.GetNativeEcs()->GetEntityManager().Create();
    envm->Create(ent);
    return ent;
}

bool Environment::SetEcsObject(const IEcsObject::Ptr& obj)
{
    auto ret = Super::SetEcsObject(obj);
    if (ret) {
        Name()->SetValue(GetName());
        Name()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&] {
            if (this->object_) {
                this->object_->SetName(Name()->GetValue());
            }
        }));
    }
    return ret;
}

BASE_NS::string Environment::GetName() const
{
    return this->object_ ? this->object_->GetName() : "";
}

SCENE_END_NAMESPACE()
