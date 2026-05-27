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

#include <cmath>
#include <limits>
#include <util/bowyer_watson_delaunay_3d.h>
#include <util/light_probe_util.h>

#include <3d/light_probe_types/light_probe.h>
#include <3d/light_probe_types/light_probe_constants.h>
#include <base/math/vector.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

namespace {
Tetrahedron CreateTestTetrahedron()
{
    Tetrahedron tet;
    tet.vertices[0] = Math::Vec3(0.0f, 0.0f, 0.0f);
    tet.vertices[1] = Math::Vec3(1.0f, 0.0f, 0.0f);
    tet.vertices[2] = Math::Vec3(0.0f, 1.0f, 0.0f);
    tet.vertices[3] = Math::Vec3(0.0f, 0.0f, 1.0f);
    tet.indices[0] = 0;
    tet.indices[1] = 1;
    tet.indices[2] = 2;
    tet.indices[3] = 3;
    tet.circumcenter = Math::Vec3(0.25f, 0.25f, 0.25f);
    tet.circumradiusSquared = 0.75f;
    return tet;
}

vector<LightProbeGroupComponent::LightProbe> CreateTestLightProbes()
{
    vector<LightProbeGroupComponent::LightProbe> probes;

    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.ao = 0.8f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(1.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(1.0f, 0.0f, 0.0f);
    probe.ao = 0.6f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 1.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 0.0f, 1.0f);
    probe.ao = 0.4f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 0.0f, 1.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 0.0f, 1.0f);
    probe.bentNormal = Math::Vec3(0.5f, 0.5f, 0.5f);
    probe.ao = 0.5f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.5f, 0.5f, 0.5f);
    }
    probes.push_back(probe);

    return probes;
}

vector<Tetrahedron> CreateTestTetrahedrons(const vector<LightProbeGroupComponent::LightProbe>& probes)
{
    vector<Tetrahedron> tets;
    Tetrahedron tet;
    for (uint32_t i = 0; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
        tet.indices[i] = i;
        tet.vertices[i] = probes[i].position;
    }
    tet.circumcenter = Math::Vec3(0.25f, 0.25f, 0.25f);
    tet.circumradiusSquared = 0.75f;
    tets.push_back(tet);
    return tets;
}
}  // namespace

UNIT_TEST(SRC_LightProbeUtil, CalculateBarycentricCoordinates_InsideTetrahedron, testing::ext::TestSize.Level1)
{
    Tetrahedron tet = CreateTestTetrahedron();

    Math::Vec3 point(0.1f, 0.1f, 0.1f);
    Math::Vec4 bary = LightProbeUtil::CalculateBarycentricCoordinates(point, tet);

    EXPECT_GE(bary.x, 0.0f);
    EXPECT_GE(bary.y, 0.0f);
    EXPECT_GE(bary.z, 0.0f);
    EXPECT_GE(bary.w, 0.0f);

    float sum = bary.x + bary.y + bary.z + bary.w;
    EXPECT_NEAR(sum, 1.0f, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, CalculateBarycentricCoordinates_OnVertex, testing::ext::TestSize.Level1)
{
    Tetrahedron tet = CreateTestTetrahedron();

    Math::Vec3 pointAtV0(0.0f, 0.0f, 0.0f);
    Math::Vec4 baryV0 = LightProbeUtil::CalculateBarycentricCoordinates(pointAtV0, tet);
    EXPECT_NEAR(baryV0.x, 1.0f, 0.001f);
    EXPECT_NEAR(baryV0.y, 0.0f, 0.001f);
    EXPECT_NEAR(baryV0.z, 0.0f, 0.001f);
    EXPECT_NEAR(baryV0.w, 0.0f, 0.001f);

    Math::Vec3 pointAtV1(1.0f, 0.0f, 0.0f);
    Math::Vec4 baryV1 = LightProbeUtil::CalculateBarycentricCoordinates(pointAtV1, tet);
    EXPECT_NEAR(baryV1.x, 0.0f, 0.001f);
    EXPECT_NEAR(baryV1.y, 1.0f, 0.001f);
    EXPECT_NEAR(baryV1.z, 0.0f, 0.001f);
    EXPECT_NEAR(baryV1.w, 0.0f, 0.001f);

    Math::Vec3 pointAtV2(0.0f, 1.0f, 0.0f);
    Math::Vec4 baryV2 = LightProbeUtil::CalculateBarycentricCoordinates(pointAtV2, tet);
    EXPECT_NEAR(baryV2.x, 0.0f, 0.001f);
    EXPECT_NEAR(baryV2.y, 0.0f, 0.001f);
    EXPECT_NEAR(baryV2.z, 1.0f, 0.001f);
    EXPECT_NEAR(baryV2.w, 0.0f, 0.001f);

    Math::Vec3 pointAtV3(0.0f, 0.0f, 1.0f);
    Math::Vec4 baryV3 = LightProbeUtil::CalculateBarycentricCoordinates(pointAtV3, tet);
    EXPECT_NEAR(baryV3.x, 0.0f, 0.001f);
    EXPECT_NEAR(baryV3.y, 0.0f, 0.001f);
    EXPECT_NEAR(baryV3.z, 0.0f, 0.001f);
    EXPECT_NEAR(baryV3.w, 1.0f, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, CalculateBarycentricCoordinates_OnFace, testing::ext::TestSize.Level1)
{
    Tetrahedron tet = CreateTestTetrahedron();

    Math::Vec3 pointOnFaceV0V1V2(0.33f, 0.33f, 0.0f);
    Math::Vec4 bary = LightProbeUtil::CalculateBarycentricCoordinates(pointOnFaceV0V1V2, tet);
    EXPECT_NEAR(bary.w, 0.0f, 0.001f);
    EXPECT_GE(bary.x, 0.0f);
    EXPECT_GE(bary.y, 0.0f);
    EXPECT_GE(bary.z, 0.0f);
    EXPECT_NEAR(bary.x + bary.y + bary.z, 1.0f, 0.001f);

    Math::Vec3 pointOnFaceV0V1V3(0.33f, 0.0f, 0.33f);
    bary = LightProbeUtil::CalculateBarycentricCoordinates(pointOnFaceV0V1V3, tet);
    EXPECT_NEAR(bary.z, 0.0f, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, CalculateBarycentricCoordinates_DegenerateTetrahedron, testing::ext::TestSize.Level1)
{
    Tetrahedron degenerateTet;
    degenerateTet.vertices[0] = Math::Vec3(0.0f, 0.0f, 0.0f);
    degenerateTet.vertices[1] = Math::Vec3(1.0f, 0.0f, 0.0f);
    degenerateTet.vertices[2] = Math::Vec3(0.5f, 0.0f, 0.0f);
    degenerateTet.vertices[3] = Math::Vec3(0.25f, 0.0f, 0.0f);

    Math::Vec3 point(0.25f, 0.25f, 0.25f);
    Math::Vec4 bary = LightProbeUtil::CalculateBarycentricCoordinates(point, degenerateTet);

    EXPECT_NEAR(bary.x, -1.0f, 0.001f);
    EXPECT_NEAR(bary.y, -1.0f, 0.001f);
    EXPECT_NEAR(bary.z, -1.0f, 0.001f);
    EXPECT_NEAR(bary.w, -1.0f, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, CalculateBarycentricCoordinates_LargeCoordinates, testing::ext::TestSize.Level1)
{
    Tetrahedron tet;
    tet.vertices[0] = Math::Vec3(1000.0f, 1000.0f, 1000.0f);
    tet.vertices[1] = Math::Vec3(2000.0f, 1000.0f, 1000.0f);
    tet.vertices[2] = Math::Vec3(1000.0f, 2000.0f, 1000.0f);
    tet.vertices[3] = Math::Vec3(1000.0f, 1000.0f, 2000.0f);

    Math::Vec3 point(1250.0f, 1250.0f, 1250.0f);
    Math::Vec4 bary = LightProbeUtil::CalculateBarycentricCoordinates(point, tet);

    float sum = bary.x + bary.y + bary.z + bary.w;
    EXPECT_NEAR(sum, 1.0f, 0.01f);
    EXPECT_GE(bary.x, 0.0f);
    EXPECT_GE(bary.y, 0.0f);
    EXPECT_GE(bary.z, 0.0f);
    EXPECT_GE(bary.w, 0.0f);
}

UNIT_TEST(SRC_LightProbeUtil, FindContainingTetrahedron_PointInside, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 insidePoint(0.1f, 0.1f, 0.1f);
    TetrahedronOpt result = LightProbeUtil::FindContainingTetrahedron(insidePoint, volume);

    EXPECT_TRUE(result.valid);
}

UNIT_TEST(SRC_LightProbeUtil, FindContainingTetrahedron_PointOutside, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 outsidePoint(2.0f, 2.0f, 2.0f);
    TetrahedronOpt result = LightProbeUtil::FindContainingTetrahedron(outsidePoint, volume);

    EXPECT_FALSE(result.valid);
}

UNIT_TEST(SRC_LightProbeUtil, FindContainingTetrahedron_EmptyTetrahedrons, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    vector<Tetrahedron> tets;

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 point(0.5f, 0.5f, 0.5f);
    TetrahedronOpt result = LightProbeUtil::FindContainingTetrahedron(point, volume);

    EXPECT_FALSE(result.valid);
}

UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_AtVertex, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 positionAtV0(0.0f, 0.0f, 0.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, positionAtV0);

    EXPECT_TRUE(result.valid);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].x, 1.0f, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].y, 0.0f, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].z, 0.0f, 0.001f);
    }
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.x, 0.0f, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.y, 1.0f, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.z, 0.0f, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.w, 0.8f, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_AtCenter, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 positionAtCenter(0.25f, 0.25f, 0.25f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, positionAtCenter);

    EXPECT_TRUE(result.valid);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        float expectedX = (1.0f + 0.0f + 0.0f + 0.5f) * 0.25f;
        float expectedY = (0.0f + 1.0f + 0.0f + 0.5f) * 0.25f;
        float expectedZ = (0.0f + 0.0f + 1.0f + 0.5f) * 0.25f;

        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].x, expectedX, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].y, expectedY, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].z, expectedZ, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].w, 1.0f, 0.001f);
    }

    float expectedBentNormalX = (0.0f + 1.0f + 0.0f + 0.5f) * 0.25f;
    float expectedBentNormalY = (1.0f + 0.0f + 0.0f + 0.5f) * 0.25f;
    float expectedBentNormalZ = (0.0f + 0.0f + 1.0f + 0.5f) * 0.25f;
    Math::Vec3 expectedBentNormal =
        Math::Normalize(Math::Vec3(expectedBentNormalX, expectedBentNormalY, expectedBentNormalZ));
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.x, expectedBentNormal.x, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.y, expectedBentNormal.y, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.z, expectedBentNormal.z, 0.001f);

    float expectedAo = (0.8f + 0.6f + 0.4f + 0.5f) * 0.25f;
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.w, expectedAo, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_NegativeCoefficients, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    LightProbeGroupComponent::LightProbe probe;

    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(-1.0f, -1.0f, -1.0f);
    probe.ao = 0.0f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(-1.0f, -0.5f, -0.25f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(1.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(1.0f, 1.0f, 1.0f);
    probe.ao = 1.0f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.5f, 0.25f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.ao = 0.5f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 0.0f, 1.0f);
    probe.bentNormal = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.ao = 0.5f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 position(0.5f, 0.0f, 0.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, position);

    EXPECT_TRUE(result.valid);

    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        float expectedX = (-1.0f) * 0.5f + 1.0f * 0.5f;
        float expectedY = (-0.5f) * 0.5f + 0.5f * 0.5f;
        float expectedZ = (-0.25f) * 0.5f + 0.25f * 0.5f;

        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].x, expectedX, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].y, expectedY, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].z, expectedZ, 0.001f);
    }

    Math::Vec3 expectedBentNormal(0.0f, 1.0f, 0.0f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.x, expectedBentNormal.x, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.y, expectedBentNormal.y, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.z, expectedBentNormal.z, 0.001f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.w, 1.0f, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_ValidPosition, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 position(0.1f, 0.1f, 0.1f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, position);

    EXPECT_TRUE(result.valid);
}

UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_InvalidPosition, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 position(10.0f, 10.0f, 10.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, position);

    EXPECT_TRUE(result.valid);
}

UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_EmptyVolume, testing::ext::TestSize.Level1)
{
    LightProbeVolume volume;
    volume.lightProbes = {};
    volume.tetrahedrons = {};

    Math::Vec3 position(0.5f, 0.5f, 0.5f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, position);

    EXPECT_FALSE(result.valid);
}

UNIT_TEST(SRC_LightProbeUtil, BarycentricConsistencyWithPointReconstruction, testing::ext::TestSize.Level1)
{
    Tetrahedron tet = CreateTestTetrahedron();

    Math::Vec4 bary(0.2f, 0.3f, 0.15f, 0.35f);

    Math::Vec3 reconstructed;
    reconstructed.x = tet.vertices[0].x * bary.x + tet.vertices[1].x * bary.y + tet.vertices[2].x * bary.z +
                      tet.vertices[3].x * bary.w;
    reconstructed.y = tet.vertices[0].y * bary.x + tet.vertices[1].y * bary.y + tet.vertices[2].y * bary.z +
                      tet.vertices[3].y * bary.w;
    reconstructed.z = tet.vertices[0].z * bary.x + tet.vertices[1].z * bary.y + tet.vertices[2].z * bary.z +
                      tet.vertices[3].z * bary.w;

    Math::Vec4 computedBary = LightProbeUtil::CalculateBarycentricCoordinates(reconstructed, tet);

    EXPECT_NEAR(computedBary.x, bary.x, 0.001f);
    EXPECT_NEAR(computedBary.y, bary.y, 0.001f);
    EXPECT_NEAR(computedBary.z, bary.z, 0.001f);
    EXPECT_NEAR(computedBary.w, bary.w, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_BentNormalAndAo, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 position(0.6f, 0.25f, 0.1f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, position);

    EXPECT_TRUE(result.valid);

    float expectedBentNormalXRaw = 0.0f * 0.05f + 1.0f * 0.6f + 0.0f * 0.25f + 0.5f * 0.1f;
    float expectedBentNormalYRaw = 1.0f * 0.05f + 0.0f * 0.6f + 0.0f * 0.25f + 0.5f * 0.1f;
    float expectedBentNormalZRaw = 0.0f * 0.05f + 0.0f * 0.6f + 1.0f * 0.25f + 0.5f * 0.1f;
    Math::Vec3 expectedBentNormal =
        Math::Normalize(Math::Vec3(expectedBentNormalXRaw, expectedBentNormalYRaw, expectedBentNormalZRaw));

    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.x, expectedBentNormal.x, 0.01f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.y, expectedBentNormal.y, 0.01f);
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.z, expectedBentNormal.z, 0.01f);

    float expectedAo = 0.8f * 0.05f + 0.6f * 0.6f + 0.4f * 0.25f + 0.5f * 0.1f;
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.w, expectedAo, 0.01f);
}

UNIT_TEST(SRC_LightProbeUtil, InterpolateFromNearestProbes_SingleProbe, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.ao = 0.7f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 2.0f, 3.0f);
    }
    probes.push_back(probe);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = {};

    Math::Vec3 queryPosition(5.0f, 5.0f, 5.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, queryPosition);

    EXPECT_TRUE(result.valid);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].x, 1.0f, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].y, 2.0f, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].z, 3.0f, 0.001f);
    }
    EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.w, 0.7f, 0.001f);
}

UNIT_TEST(SRC_LightProbeUtil, InterpolateFromNearestProbes_TwoProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    LightProbeGroupComponent::LightProbe probe;

    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.ao = 0.5f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(2.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(1.0f, 0.0f, 0.0f);
    probe.ao = 0.9f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 1.0f, 0.0f);
    }
    probes.push_back(probe);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = {};

    Math::Vec3 queryPosition(1.0f, 0.0f, 0.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, queryPosition);

    EXPECT_TRUE(result.valid);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].x, 0.5f, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].y, 0.5f, 0.001f);
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].z, 0.0f, 0.001f);
    }
}

UNIT_TEST(SRC_LightProbeUtil, InterpolateFromNearestProbes_ThreeProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    LightProbeGroupComponent::LightProbe probe;

    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.ao = 0.5f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(2.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(1.0f, 0.0f, 0.0f);
    probe.ao = 0.7f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 1.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 2.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 0.0f, 1.0f);
    probe.ao = 0.9f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 0.0f, 1.0f);
    }
    probes.push_back(probe);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = {};

    Math::Vec3 queryPosition(0.5f, 0.5f, 0.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, queryPosition);

    EXPECT_TRUE(result.valid);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        EXPECT_GT(result.lightProbeInterpolatedData.shCoefficients[i].x, 0.0f);
        EXPECT_GT(result.lightProbeInterpolatedData.shCoefficients[i].y, 0.0f);
        EXPECT_GT(result.lightProbeInterpolatedData.shCoefficients[i].z, 0.0f);
    }
}

UNIT_TEST(SRC_LightProbeUtil, InterpolateFromNearestProbes_OutsideTetrahedron, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 outsidePosition(2.0f, 2.0f, 2.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, outsidePosition);

    EXPECT_TRUE(result.valid);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        EXPECT_NEAR(result.lightProbeInterpolatedData.shCoefficients[i].w, 1.0f, 0.001f);
    }
}

UNIT_TEST(SRC_LightProbeUtil, InterpolateFromNearestProbes_NoTetrahedronsHasProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = {};

    Math::Vec3 queryPosition(0.5f, 0.5f, 0.5f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, queryPosition);

    EXPECT_TRUE(result.valid);
}

UNIT_TEST(SRC_LightProbeUtil, InterpolateFromNearestProbes_NearestProbeWeight, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    LightProbeGroupComponent::LightProbe probe;

    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.ao = 1.0f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(100.0f, 0.0f, 0.0f);
    probe.bentNormal = Math::Vec3(1.0f, 0.0f, 0.0f);
    probe.ao = 0.0f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 100.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.0f, 0.0f, 1.0f);
    probe.ao = 0.0f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = {};

    Math::Vec3 queryPosition(0.1f, 0.0f, 0.0f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, queryPosition);

    EXPECT_TRUE(result.valid);
    EXPECT_GT(result.lightProbeInterpolatedData.shCoefficients[0].x, 0.9f);
}

// Tet indices past the probe array must fall back to IDW, not OOB-deref. Regression for 35334a53.
UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_StaleIndicesFallback, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestLightProbes();
    vector<Tetrahedron> tets = CreateTestTetrahedrons(probes);

    probes.pop_back();
    probes.pop_back();

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    Math::Vec3 position(0.1f, 0.1f, 0.1f);
    LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, position);

    EXPECT_TRUE(result.valid);
    // IDW fallback: convex blend of surviving probes (SH[0] = (1,0,0) and (0,1,0)).
    const auto& sh0 = result.lightProbeInterpolatedData.shCoefficients[0];
    EXPECT_GE(sh0.x, 0.0f);
    EXPECT_LE(sh0.x, 1.0f);
    EXPECT_GE(sh0.y, 0.0f);
    EXPECT_LE(sh0.y, 1.0f);
    EXPECT_NEAR(sh0.x + sh0.y, 1.0f, 0.01f);
    EXPECT_NEAR(sh0.z, 0.0f, 0.001f);
}

// Barycentric is exact for linear functions, so encoding SH = position must reproduce exactly at
// any interior query, regardless of which tet contains it. Catches winding/sign bugs in
// FindContainingTetrahedron, bad probe-index ordering in InterpolateLightProbeData, and
// any cross-tet face discontinuity (would break linear reproduction across boundaries).
UNIT_TEST(SRC_LightProbeUtil, GetInterpolatedLightProbeData_ManyProbesLinearReproduction, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    for (uint32_t z = 0; z < 3; ++z) {
        for (uint32_t y = 0; y < 3; ++y) {
            for (uint32_t x = 0; x < 3; ++x) {
                LightProbeGroupComponent::LightProbe p;
                p.position = Math::Vec3((float)x, (float)y, (float)z);
                for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
                    p.shCoefficients[i] = Math::Vec3((float)x, (float)y, (float)z);
                }
                p.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
                p.ao = (float)x * 0.1f + (float)y * 0.1f + (float)z * 0.1f;
                probes.push_back(p);
            }
        }
    }

    vector<Tetrahedron> tets;
    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume buildVolume{probes, tets};
    delaunay.BuildTetrahedralMesh(buildVolume);
    EXPECT_GT(tets.size(), 0u);

    LightProbeVolume volume;
    volume.lightProbes = probes;
    volume.tetrahedrons = tets;

    const Math::Vec3 queries[] = {
        {1.0f, 1.0f, 1.0f},
        {0.5f, 1.5f, 0.75f},
        {1.25f, 0.4f, 1.6f},
        {1.9f, 1.9f, 1.9f},
        {0.1f, 0.1f, 0.1f},
    };
    for (const auto& q : queries) {
        LightProbeInterpolatedDataOpt result = LightProbeUtil::GetInterpolatedLightProbeData(volume, q);
        ASSERT_TRUE(result.valid);
        for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
            const auto& sh = result.lightProbeInterpolatedData.shCoefficients[i];
            EXPECT_NEAR(sh.x, q.x, 0.01f);
            EXPECT_NEAR(sh.y, q.y, 0.01f);
            EXPECT_NEAR(sh.z, q.z, 0.01f);
        }
        const float expectedAo = q.x * 0.1f + q.y * 0.1f + q.z * 0.1f;
        EXPECT_NEAR(result.lightProbeInterpolatedData.bentNormalAo.w, expectedAo, 0.01f);
    }
}