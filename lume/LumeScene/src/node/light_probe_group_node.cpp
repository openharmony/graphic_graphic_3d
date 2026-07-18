/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "light_probe_group_node.h"

#include <3d/ecs/components/light_component.h>

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity LightProbeGroupNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    if (!scene) {
        return {};
    }
    auto& ctx = scene->GetEcsContext();
    auto ecs = ctx.GetNativeEcs();
    auto manager = ecs ? CORE_NS::GetManager<CORE3D_NS::ILightProbeGroupComponentManager>(*ecs) : nullptr;
    if (!manager) {
        // Require that ILightProbeComponentManager is available
        return {};
    }
    auto entity = ecs->GetEntityManager().Create();
    manager->Create(entity);
    ctx.AddDefaultComponents(entity);
    return entity;
}

namespace Internal {
template <class Fn>
auto CallLightProbeComponentMananer(const IInternalScene::Ptr& is, CORE_NS::Entity entity, Fn&& fn)
{
    CORE3D_NS::ILightProbeGroupComponentManager* m{};
    using R = decltype(fn(*m, entity));

    auto ecs = is ? is->GetEcsContext().GetNativeEcs() : CORE_NS::IEcs::Ptr{};
    if (!(ecs && CORE_NS::EntityUtil::IsValid(entity))) {
        return R{};
    }
    return is->RunDirectlyOrInTask([=]() {
        auto manager = CORE_NS::GetManager<CORE3D_NS::ILightProbeGroupComponentManager>(*ecs);
        return manager ? fn(*manager, entity) : R{};
    });
}

}  // namespace Internal

LightProbeGroup LightProbeGroupNode::GetLightProbeGroup() const
{
    return Internal::CallLightProbeComponentMananer(GetInternalScene(),
        GetEntity(),
        [&](CORE3D_NS::ILightProbeGroupComponentManager& manager, CORE_NS::Entity entity) {
            LightProbeGroup group;
            if (auto rh = manager.Read(entity)) {
                group = rh->lightProbes;
            }
            return group;
        });
}
bool LightProbeGroupNode::SetLightProbeGroup(const LightProbeGroup& group)
{
    return Internal::CallLightProbeComponentMananer(GetInternalScene(),
        GetEntity(),
        [&](CORE3D_NS::ILightProbeGroupComponentManager& manager, CORE_NS::Entity entity) {
            if (auto wh = manager.Write(entity)) {
                wh->lightProbes = group;
                return true;
            }
            return false;
        });
}
LightProbe LightProbeGroupNode::GetLightProbeAt(size_t index) const
{
    return Internal::CallLightProbeComponentMananer(GetInternalScene(),
        GetEntity(),
        [&](CORE3D_NS::ILightProbeGroupComponentManager& manager, CORE_NS::Entity entity) {
            LightProbe probe{};
            if (auto rh = manager.Read(entity); rh && rh->lightProbes.size() > index) {
                probe = rh->lightProbes[index];
            }
            return probe;
        });
}
bool LightProbeGroupNode::SetLightProbeAt(const LightProbe& probe, size_t index)
{
    return Internal::CallLightProbeComponentMananer(GetInternalScene(),
        GetEntity(),
        [&](CORE3D_NS::ILightProbeGroupComponentManager& manager, CORE_NS::Entity entity) {
            if (auto rh = manager.Read(entity)) {
                if (rh->lightProbes.size() <= index) {
                    return false;
                }
                if (auto wh = manager.Write(entity)) {
                    wh->lightProbes[index] = probe;
                    return true;
                }
            }
            return false;
        });
}

SCENE_END_NAMESPACE()
