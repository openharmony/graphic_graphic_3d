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

#ifndef SCENE_INTERFACE_ILIGHTPROBEGROUP_H
#define SCENE_INTERFACE_ILIGHTPROBEGROUP_H

#include <scene/base/namespace.h>

#include <3d/ecs/components/light_probe_group_component.h>
#include <base/util/color.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

using LightProbe = CORE3D_NS::LightProbeGroupComponent::LightProbe;
using LightProbeGroup = BASE_NS::vector<CORE3D_NS::LightProbeGroupComponent::LightProbe>;

class ILightProbeGroup : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ILightProbeGroup, "29aeeeff-ea06-4ffb-b8a8-34c6cf79f7aa")
public:
    virtual LightProbeGroup GetLightProbeGroup() const = 0;
    virtual bool SetLightProbeGroup(const LightProbeGroup& group) = 0;
    virtual LightProbe GetLightProbeAt(size_t index) const = 0;
    virtual bool SetLightProbeAt(const LightProbe& probe, size_t index) = 0;
};

META_REGISTER_CLASS(
    LightProbeGroupNode, "6aa0224a-67a3-493e-b4d2-f7acd9e6e8ba", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::LightProbe)

META_INTERFACE_TYPE(SCENE_NS::ILightProbeGroup)

#endif  // SCENE_INTERFACE_ILIGHTPROBEGROUP_H
