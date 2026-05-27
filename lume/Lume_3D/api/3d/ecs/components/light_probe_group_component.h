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

#if !defined(API_3D_ECS_COMPONENTS_LIGHT_PROBE_GROUP_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_LIGHT_PROBE_GROUP_COMPONENT_H
#if !defined(IMPLEMENT_MANAGER)
#include <3d/light_probe_types/light_probe_constants.h>
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>
CORE3D_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(ILightProbeGroupComponentManager, LightProbeGroupComponent)
#if !defined(IMPLEMENT_MANAGER)
struct LightProbe {
    BASE_NS::Math::Vec3 shCoefficients[LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT];
    BASE_NS::Math::Vec3 position;
    BASE_NS::Math::Vec3 bentNormal;
    float ao;
};
#endif

DEFINE_PROPERTY(BASE_NS::vector<LightProbe>, lightProbes, "light probes", 0, )

END_COMPONENT(ILightProbeGroupComponentManager, LightProbeGroupComponent, "03b2e0c3-1645-4185-ad5f-969e1c4d632a")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
