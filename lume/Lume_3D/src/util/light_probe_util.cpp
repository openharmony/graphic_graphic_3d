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

#include "light_probe_util.h"

#include <algorithm>
#include <cfloat>
#include <util/log.h>

#include <3d/light_probe_types/light_probe_constants.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>
#include <core/namespace.h>

#include "bowyer_watson_delaunay_3d.h"

namespace {
void InterpolateLightProbeData(const BASE_NS::Math::Vec4& barycentric, const CORE3D_NS::Tetrahedron& tet,
    const CORE3D_NS::LightProbeVolume& lightProbeVolume,
    CORE3D_NS::LightProbeInterpolatedData& lightProbeInterpolatedData)
{
    for (auto i = 0u; i < CORE3D_NS::LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        auto sh = lightProbeVolume.lightProbes[tet.indices[0]].shCoefficients[i] * barycentric.x +
                  lightProbeVolume.lightProbes[tet.indices[1]].shCoefficients[i] * barycentric.y +
                  lightProbeVolume.lightProbes[tet.indices[2]].shCoefficients[i] * barycentric.z +
                  lightProbeVolume.lightProbes[tet.indices[3]].shCoefficients[i] * barycentric.w;
        lightProbeInterpolatedData.shCoefficients[i].x = sh.x;
        lightProbeInterpolatedData.shCoefficients[i].y = sh.y;
        lightProbeInterpolatedData.shCoefficients[i].z = sh.z;
        lightProbeInterpolatedData.shCoefficients[i].w = 1;
    }

    float ao0 = lightProbeVolume.lightProbes[tet.indices[0]].ao;
    float ao1 = lightProbeVolume.lightProbes[tet.indices[1]].ao;
    float ao2 = lightProbeVolume.lightProbes[tet.indices[2]].ao;
    float ao3 = lightProbeVolume.lightProbes[tet.indices[3]].ao;

    float ao = ao0 * barycentric.x + ao1 * barycentric.y + ao2 * barycentric.z + ao3 * barycentric.w;
    lightProbeInterpolatedData.bentNormalAo.w = ao;

    auto bentNormal = lightProbeVolume.lightProbes[tet.indices[0]].bentNormal * barycentric.x +
                      lightProbeVolume.lightProbes[tet.indices[1]].bentNormal * barycentric.y +
                      lightProbeVolume.lightProbes[tet.indices[2]].bentNormal * barycentric.z +
                      lightProbeVolume.lightProbes[tet.indices[3]].bentNormal * barycentric.w;

    auto normalized = BASE_NS::Math::Normalize(bentNormal);
    if (BASE_NS::Math::Magnitude(normalized) <= CORE3D_NS::LightProbeConstants::EPSILON) {
        normalized = BASE_NS::Math::Vec3(0.0f, 1.0f, 0.0f);
        ao = 1.0;
    }
    lightProbeInterpolatedData.bentNormalAo = BASE_NS::Math::Vec4(normalized, ao);
}

constexpr uint32_t IDW_NEAREST_PROBES = 3U;

struct ProbeDistance {
    size_t index;
    float distance;
};

uint32_t FindNearestProbes(const CORE3D_NS::LightProbeVolume& volume, const BASE_NS::Math::Vec3& worldCenter,
    ProbeDistance (&nearest)[IDW_NEAREST_PROBES])
{
    for (auto& slot : nearest) {
        slot = {0U, FLT_MAX};
    }
    for (size_t i = 0; i < volume.lightProbes.size(); ++i) {
        const float dist = BASE_NS::Math::distance(worldCenter, volume.lightProbes[i].position);
        if (dist < nearest[0].distance) {
            nearest[2] = nearest[1];
            nearest[1] = nearest[0];
            nearest[0] = {i, dist};
        } else if (dist < nearest[1].distance) {
            nearest[2] = nearest[1];
            nearest[1] = {i, dist};
        } else if (dist < nearest[2].distance) {
            nearest[2] = {i, dist};
        }
    }
    uint32_t count = 0U;
    for (const auto& slot : nearest) {
        if (slot.distance < FLT_MAX) {
            ++count;
        }
    }
    return count;
}

void ComputeIdwWeights(
    const ProbeDistance (&nearest)[IDW_NEAREST_PROBES], uint32_t probeCount, float (&weights)[IDW_NEAREST_PROBES])
{
    constexpr float MIN_DISTANCE = 0.001f;
    float totalWeight = 0.0f;
    for (uint32_t i = 0U; i < probeCount; ++i) {
        weights[i] = 1.0f / std::max(nearest[i].distance, MIN_DISTANCE);
        totalWeight += weights[i];
    }
    for (uint32_t i = 0U; i < probeCount; ++i) {
        weights[i] /= totalWeight;
    }
}

bool InterpolateFromNearestProbes(const CORE3D_NS::LightProbeVolume& lightProbeVolume,
    const BASE_NS::Math::Vec3& worldCenter, CORE3D_NS::LightProbeInterpolatedData& lightProbeInterpolatedData)
{
    if (lightProbeVolume.lightProbes.empty()) {
        return false;
    }
    auto& result = lightProbeInterpolatedData;

    ProbeDistance nearest[IDW_NEAREST_PROBES] = {};
    const uint32_t probeCount = FindNearestProbes(lightProbeVolume, worldCenter, nearest);

    float weights[IDW_NEAREST_PROBES] = {0.0f, 0.0f, 0.0f};
    ComputeIdwWeights(nearest, probeCount, weights);

    for (uint32_t i = 0U; i < probeCount; ++i) {
        const auto& probe = lightProbeVolume.lightProbes[nearest[i].index];
        const float weight = weights[i];
        for (auto j = 0U; j < CORE3D_NS::LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++j) {
            result.shCoefficients[j].x += probe.shCoefficients[j].x * weight;
            result.shCoefficients[j].y += probe.shCoefficients[j].y * weight;
            result.shCoefficients[j].z += probe.shCoefficients[j].z * weight;
        }
        result.bentNormalAo.x += probe.bentNormal.x * weight;
        result.bentNormalAo.y += probe.bentNormal.y * weight;
        result.bentNormalAo.z += probe.bentNormal.z * weight;
        result.bentNormalAo.w += probe.ao * weight;
    }

    for (auto i = 0U; i < CORE3D_NS::LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        result.shCoefficients[i].w = 1.0f;
    }

    BASE_NS::Math::Vec3 bentNormal(result.bentNormalAo.x, result.bentNormalAo.y, result.bentNormalAo.z);
    auto normalized = BASE_NS::Math::Normalize(bentNormal);
    if (BASE_NS::Math::Magnitude(normalized) <= CORE3D_NS::LightProbeConstants::EPSILON) {
        normalized = BASE_NS::Math::Vec3(0.0f, 1.0f, 0.0f);
        result.bentNormalAo.w = 1.0f;
    }
    result.bentNormalAo = BASE_NS::Math::Vec4(normalized, result.bentNormalAo.w);

    return true;
}
}  // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

BASE_NS::Math::Vec4 LightProbeUtil::CalculateBarycentricCoordinates(
    const BASE_NS::Math::Vec3& point, const Tetrahedron& tet)
{
    const BASE_NS::Math::Vec3& v0 = tet.vertices[0];
    const BASE_NS::Math::Vec3& v1 = tet.vertices[1];
    const BASE_NS::Math::Vec3& v2 = tet.vertices[2];
    const BASE_NS::Math::Vec3& v3 = tet.vertices[3];

    const BASE_NS::Math::Vec3 v1v0 = v1 - v0;
    const BASE_NS::Math::Vec3 v2v0 = v2 - v0;
    const BASE_NS::Math::Vec3 v3v0 = v3 - v0;

    const float det = BASE_NS::Math::Dot(v1v0, BASE_NS::Math::Cross(v2v0, v3v0));

    if (BASE_NS::Math::abs(det) < 1e-6f) {
        return {-1.0f, -1.0f, -1.0f, -1.0f};
    }

    const float invDet = 1.0f / det;

    const BASE_NS::Math::Vec3 pv0 = point - v0;

    const float u = BASE_NS::Math::Dot(pv0, BASE_NS::Math::Cross(v2v0, v3v0)) * invDet;
    const float v = BASE_NS::Math::Dot(v1v0, BASE_NS::Math::Cross(pv0, v3v0)) * invDet;
    const float w = BASE_NS::Math::Dot(v1v0, BASE_NS::Math::Cross(v2v0, pv0)) * invDet;
    const float t = 1.0f - u - v - w;

    return {t, u, v, w};
}

TetrahedronOpt LightProbeUtil::FindContainingTetrahedron(
    const BASE_NS::Math::Vec3& position, const LightProbeVolume& lightProbeVolume)
{
    if (lightProbeVolume.tetrahedrons.empty()) {
        return {};
    }
    constexpr bool DBG_HIT_TET = false;
    for (const auto& tet : lightProbeVolume.tetrahedrons) {
        BASE_NS::Math::Vec4 bary = CalculateBarycentricCoordinates(position, tet);

        if constexpr (DBG_HIT_TET) {
            PLUGIN_LOG_E("position: %f, %f %f, bary: %f %f %f %f",
                position.x,
                position.y,
                position.z,
                bary.x,
                bary.y,
                bary.z,
                bary.w);
            PLUGIN_LOG_E("lightProbe[0]: %f, %f %f, lightProbe[1] %f %f %f",
                tet.vertices[0].x,
                tet.vertices[0].y,
                tet.vertices[0].z,
                tet.vertices[1].x,
                tet.vertices[1].y,
                tet.vertices[1].z);
            PLUGIN_LOG_E("lightProbe[2]: %f, %f %f, lightProbe[2] %f %f %f",
                tet.vertices[2].x,
                tet.vertices[2].y,
                tet.vertices[2].z,
                tet.vertices[3].x,
                tet.vertices[3].y,
                tet.vertices[3].z);
        }

        if (bary.x >= 0.0f && bary.y >= 0.0f && bary.z >= 0.0f && bary.w >= 0.0f &&
            std::abs(bary.x + bary.y + bary.z + bary.w - 1.0f) < LightProbeConstants::EPSILON) {
            if constexpr (DBG_HIT_TET) {
                PLUGIN_LOG_W(
                    "find material position(%f, %f, %f) correct tetrahedron", position.x, position.y, position.z);
            }
            return {tet, true};
        }
    }
    if constexpr (DBG_HIT_TET) {
        PLUGIN_LOG_W(
            "Do not find material position(%f, %f, %f) correct tetrahedron", position.x, position.y, position.z);
    }

    return {};
}

LightProbeInterpolatedDataOpt LightProbeUtil::GetInterpolatedLightProbeData(
    const LightProbeVolume& lightProbeVolume, const BASE_NS::Math::Vec3& worldCenter)
{
    LightProbeInterpolatedData lightProbeInterpolatedData;
    if (lightProbeVolume.lightProbes.empty()) {
        return {lightProbeInterpolatedData, false};
    }
    auto tetOpt = FindContainingTetrahedron(worldCenter, lightProbeVolume);
    if (!tetOpt) {
        auto res = InterpolateFromNearestProbes(lightProbeVolume, worldCenter, lightProbeInterpolatedData);
        return {lightProbeInterpolatedData, res};
    }
    const auto& tet = tetOpt.tetrahedrons;
    const uint32_t probeCount = static_cast<uint32_t>(lightProbeVolume.lightProbes.size());
    if (tet.indices[0] >= probeCount || tet.indices[1] >= probeCount || tet.indices[2] >= probeCount ||
        tet.indices[3] >= probeCount) {
        auto res = InterpolateFromNearestProbes(lightProbeVolume, worldCenter, lightProbeInterpolatedData);
        return {lightProbeInterpolatedData, res};
    }
    auto barycentric = CalculateBarycentricCoordinates(worldCenter, tet);
    InterpolateLightProbeData(barycentric, tet, lightProbeVolume, lightProbeInterpolatedData);
    return {lightProbeInterpolatedData, true};
}
CORE3D_END_NAMESPACE()
