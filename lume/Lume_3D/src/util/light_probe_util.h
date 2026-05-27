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
#ifndef CORE_UTIL_LIGHT_PROBE_UTIL_H
#define CORE_UTIL_LIGHT_PROBE_UTIL_H

#include <3d/light_probe_types/light_probe.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
class Vec2;
class Vec3;
class Vec4;
}  // namespace Math
BASE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class LightProbeUtil {
public:
    static TetrahedronOpt FindContainingTetrahedron(
        const BASE_NS::Math::Vec3& position, const LightProbeVolume& lightProbeVolume);
    static BASE_NS::Math::Vec4 CalculateBarycentricCoordinates(
        const BASE_NS::Math::Vec3& point, const Tetrahedron& tet);
    static LightProbeInterpolatedDataOpt GetInterpolatedLightProbeData(
        const LightProbeVolume& lightProbeVolume, const BASE_NS::Math::Vec3& worldCenter);

    LightProbeUtil() = default;
    ~LightProbeUtil() = default;
};

CORE3D_END_NAMESPACE()
#endif  // CORE_UTIL_LIGHT_PROBE_UTIL_H
