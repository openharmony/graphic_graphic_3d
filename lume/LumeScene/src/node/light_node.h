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

#ifndef SCENE_SRC_NODE_LIGHT_NODE_H
#define SCENE_SRC_NODE_LIGHT_NODE_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_light.h>

#include "node.h"

SCENE_BEGIN_NAMESPACE()

class LightNode : public META_NS::IntroduceInterfaces<Node, ILight, ICreateEntity> {
    using Super = IntroduceInterfaces;
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::LightNode)
    META_DEFINE_OBJECT_TYPE_INFO(LightNode, ClassId::LightNode)

public:
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Color, Color, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, Intensity, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, NearPlane, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(bool, ShadowEnabled, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, ShadowStrength, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, ShadowDepthBias, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, ShadowNormalBias, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, SpotInnerAngle, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, SpotOuterAngle, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(LightType, Type, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec4, AdditionalFactor, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(uint64_t, LightLayerMask, "LightComponent")
    SCENE_USE_COMPONENT_PROPERTY(uint64_t, ShadowLayerMask, "LightComponent")

    bool SetEcsObject(const IEcsObject::Ptr&) override;

public:
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
};

SCENE_END_NAMESPACE()

#endif