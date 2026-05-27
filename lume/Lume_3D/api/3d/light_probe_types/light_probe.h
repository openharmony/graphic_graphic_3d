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
#ifndef API_3D_LIGHT_PROBE_TYPES_LIGHT_PROBE_H
#define API_3D_LIGHT_PROBE_TYPES_LIGHT_PROBE_H
#include <3d/ecs/components/light_probe_group_component.h>
#include <3d/render/render_data_defines_3d.h>
#include <render/render_data_structures.h>

CORE3D_BEGIN_NAMESPACE()

struct LightProbeInterpolatedData {
    BASE_NS::Math::Vec4 shCoefficients[LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT];
    BASE_NS::Math::Vec4 bentNormalAo;
};

struct Tetrahedron {
    uint32_t indices[LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT];
    BASE_NS::Math::Vec3 circumcenter;
    float circumradiusSquared;
    BASE_NS::Math::Vec3 vertices[LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT];
};

struct TetrahedronOpt {
    Tetrahedron tetrahedrons;
    bool valid = false;
    operator bool() const
    {
        return valid;
    }
};

struct LightProbeVolume {
    BASE_NS::array_view<const LightProbeGroupComponent::LightProbe> lightProbes;
    BASE_NS::array_view<const Tetrahedron> tetrahedrons;
};

struct LightProbeInterpolatedDataOpt {
    LightProbeInterpolatedData lightProbeInterpolatedData;
    bool valid = false;
    operator bool() const
    {
        return valid;
    }
};

struct LightProbeVolumeOpt {
    LightProbeVolume lightProbeVolume;
    bool valid = false;
    operator bool() const
    {
        return valid;
    }
};

CORE3D_END_NAMESPACE()
#endif  // API_3D_LIGHT_PROBE_TYPES_LIGHT_PROBE_H