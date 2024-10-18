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

#ifndef API_ECS_SERIALIZER_ECS_ANIMATION_UTIL_H
#define API_ECS_SERIALIZER_ECS_ANIMATION_UTIL_H

#include <algorithm>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <base/containers/fixed_string.h>
#include <base/containers/vector.h>
#include <core/ecs/intf_ecs.h>
#include <core/property/scoped_handle.h>
#include <ecs_serializer/namespace.h>

ECS_SERIALIZER_BEGIN_NAMESPACE()

void UpdateAnimationTrackTargets(CORE_NS::IEcs& ecs, CORE_NS::Entity animationEntity, CORE_NS::Entity rootNode)
{
    auto& nameManager = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(ecs);
    auto animationManager = CORE_NS::GetManager<CORE3D_NS::IAnimationComponentManager>(ecs);
    auto animationTrackManager = CORE_NS::GetManager<CORE3D_NS::IAnimationTrackComponentManager>(ecs);
    auto& entityManager = ecs.GetEntityManager();

    auto* nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
    CORE_ASSERT(nodeSystem);
    if (!nodeSystem) {
        return;
    }
    auto* node = nodeSystem->GetNode(rootNode);
    CORE_ASSERT(node);
    if (!node) {
        return;
    }

    if (const CORE_NS::ScopedHandle<const CORE3D_NS::AnimationComponent> animationData =
            animationManager->Read(animationEntity);
        animationData) {
        BASE_NS::vector<CORE_NS::Entity> targetEntities;
        targetEntities.reserve(animationData->tracks.size());
        std::transform(animationData->tracks.begin(), animationData->tracks.end(), std::back_inserter(targetEntities),
            [&manager = nameManager, &node](const CORE_NS::Entity& trackEntity) {
                if (auto nameHandle = manager.Read(trackEntity); nameHandle) {
                    if (nameHandle->name.empty()) {
                        return node->GetEntity();
                    } else {
                        if (auto targetNode = node->LookupNodeByPath(nameHandle->name); targetNode) {
                            return targetNode->GetEntity();
                        }
                    }
                }
                return CORE_NS::Entity {};
            });
        if (animationData->tracks.size() == targetEntities.size()) {
            auto targetIt = targetEntities.begin();
            for (const auto& trackEntity : animationData->tracks) {
                if (auto track = animationTrackManager->Write(trackEntity); track) {
                    if (track->target) {
                        CORE_LOG_D("AnimationTrack %s already targetted",
                            BASE_NS::to_hex(static_cast<const CORE_NS::Entity&>(track->target).id).data());
                    }
                    track->target = entityManager.GetReferenceCounted(*targetIt);
                }
                ++targetIt;
            }
        }
    }
}

ECS_SERIALIZER_END_NAMESPACE()

#endif // API_ECS_SERIALIZER_ECS_ANIMATIONUTIL_H
