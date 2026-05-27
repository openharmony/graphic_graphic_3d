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

#ifndef SCENE_SRC_NODE_LIGHT_PROBE_GROUP_NODE_H
#define SCENE_SRC_NODE_LIGHT_PROBE_GROUP_NODE_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_light_probe_group.h>

#include "node.h"

SCENE_BEGIN_NAMESPACE()

class LightProbeGroupNode : public META_NS::IntroduceInterfaces<Node, ILightProbeGroup, ICreateEntity> {
    META_OBJECT(LightProbeGroupNode, ClassId::LightProbeGroupNode, IntroduceInterfaces)

public:  // ILightProbeGroup
    LightProbeGroup GetLightProbeGroup() const override;
    bool SetLightProbeGroup(const LightProbeGroup& group) override;
    LightProbe GetLightProbeAt(size_t index) const override;
    bool SetLightProbeAt(const LightProbe& probe, size_t index) override;

public:
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
};

SCENE_END_NAMESPACE()

#endif  // SCENE_SRC_NODE_LIGHT_PROBE_GROUP_NODE_H
