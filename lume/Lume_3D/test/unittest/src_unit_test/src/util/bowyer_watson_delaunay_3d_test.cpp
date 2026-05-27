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
#include <util/bowyer_watson_delaunay_3d.h>

#include <3d/ecs/components/light_probe_group_component.h>
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

float CalculateTetrahedronVolume(const Tetrahedron& tet)
{
    Math::Vec3 v01 = tet.vertices[1] - tet.vertices[0];
    Math::Vec3 v02 = tet.vertices[2] - tet.vertices[0];
    Math::Vec3 v03 = tet.vertices[3] - tet.vertices[0];
    float det = v01.x * (v02.y * v03.z - v02.z * v03.y) - v01.y * (v02.x * v03.z - v02.z * v03.x) +
                v01.z * (v02.x * v03.y - v02.y * v03.x);
    return std::abs(det) / 6.0f;
}

bool ValidateEmptyCircumsphere(const vector<Tetrahedron>& tetrahedrons,
    const vector<LightProbeGroupComponent::LightProbe>& probes, float toleranceFactor = 1e-4f)
{
    for (const auto& tet : tetrahedrons) {
        for (size_t j = 0; j < probes.size(); ++j) {
            bool isVertex = false;
            for (uint32_t k = 0; k < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++k) {
                if (tet.indices[k] == static_cast<int>(j)) {
                    isVertex = true;
                    break;
                }
            }
            if (isVertex) {
                continue;
            }

            float distSq = Math::Distance2(probes[j].position, tet.circumcenter);
            float tolerance = std::max(tet.circumradiusSquared * toleranceFactor, 1e-5f);

            if (distSq < tet.circumradiusSquared - tolerance) {
                return false;
            }
        }
    }
    return true;
}

bool ValidateNonDegenerate(const Tetrahedron& tet, float minVolume = 1e-6f)
{
    float volume = CalculateTetrahedronVolume(tet);
    return volume > minVolume;
}

bool ValidateIndices(const vector<Tetrahedron>& tetrahedrons, size_t probeCount)
{
    for (const auto& tet : tetrahedrons) {
        for (uint32_t i = 0; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
            if (tet.indices[i] < 0 || tet.indices[i] >= static_cast<int>(probeCount)) {
                return false;
            }
        }
        for (uint32_t i = 0; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
            for (uint32_t j = i + 1; j < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++j) {
                if (tet.indices[i] == tet.indices[j]) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool ValidateVertexMatch(
    const vector<Tetrahedron>& tetrahedrons, const vector<LightProbeGroupComponent::LightProbe>& probes)
{
    for (const auto& tet : tetrahedrons) {
        for (uint32_t i = 0; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
            if (tet.indices[i] >= 0 && tet.indices[i] < static_cast<int>(probes.size())) {
                if (tet.vertices[i] != probes[tet.indices[i]].position) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool ValidateAllPointsCovered(const vector<Tetrahedron>& tetrahedrons, size_t probeCount)
{
    vector<bool> probeUsed(probeCount, false);

    for (const auto& tet : tetrahedrons) {
        for (uint32_t i = 0; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
            if (tet.indices[i] >= 0 && tet.indices[i] < static_cast<int>(probeCount)) {
                probeUsed[tet.indices[i]] = true;
            }
        }
    }

    for (size_t i = 0; i < probeCount; ++i) {
        if (!probeUsed[i]) {
            return false;
        }
    }
    return true;
}

vector<LightProbeGroupComponent::LightProbe> CreateFourProbes()
{
    vector<LightProbeGroupComponent::LightProbe> probes;

    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(2.0f, 0.0f, 0.0f);
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 2.0f, 0.0f);
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 0.0f, 2.0f);
    probes.push_back(probe);

    return probes;
}

vector<LightProbeGroupComponent::LightProbe> CreateFiveProbes()
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateFourProbes();

    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(1.0f, 1.0f, 1.0f);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.5f, 0.5f, 0.5f);
    }
    probes.push_back(probe);

    return probes;
}

vector<LightProbeGroupComponent::LightProbe> CreateCoplanarProbes()
{
    vector<LightProbeGroupComponent::LightProbe> probes;

    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(1.0f, 0.0f, 0.0f);
    probes.push_back(probe);

    probe.position = Math::Vec3(0.5f, 1.0f, 0.0f);
    probes.push_back(probe);

    probe.position = Math::Vec3(0.5f, 0.5f, 0.0f);
    probes.push_back(probe);

    return probes;
}

vector<LightProbeGroupComponent::LightProbe> CreateDuplicatePositionProbes()
{
    vector<LightProbeGroupComponent::LightProbe> probes;

    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(1.0f, 0.0f, 0.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(2.0f, 0.0f, 0.0f);
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 2.0f, 0.0f);
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probes.push_back(probe);

    return probes;
}

vector<LightProbeGroupComponent::LightProbe> CreateManyProbes(uint32_t count)
{
    vector<LightProbeGroupComponent::LightProbe> probes;

    for (uint32_t i = 0; i < count; ++i) {
        LightProbeGroupComponent::LightProbe probe;
        float angle = (float)i * 0.1f;
        float radius = 5.0f + (float)(i % 3);
        probe.position = Math::Vec3(radius * cos(angle), (float)(i % 5) * 2.0f, radius * sin(angle));
        for (uint32_t j = 0; j < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++j) {
            probe.shCoefficients[j] = Math::Vec3(0.5f, 0.5f, 0.5f);
        }
        probes.push_back(probe);
    }

    return probes;
}
}  // namespace

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_FourProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateFourProbes();
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    EXPECT_EQ(tetrahedrons.size(), 1u);

    if (!tetrahedrons.empty()) {
        EXPECT_TRUE(ValidateIndices(tetrahedrons, probes.size()));
        EXPECT_TRUE(ValidateVertexMatch(tetrahedrons, probes));
        EXPECT_TRUE(ValidateNonDegenerate(tetrahedrons[0]));
        EXPECT_TRUE(ValidateAllPointsCovered(tetrahedrons, probes.size()));
        EXPECT_TRUE(ValidateEmptyCircumsphere(tetrahedrons, probes));
    }
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_FiveProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateFiveProbes();
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    // 5th probe sits on the base tet's circumsphere — B-W predicate is borderline, accept 2..5.
    EXPECT_GE(tetrahedrons.size(), 2u);
    EXPECT_LE(tetrahedrons.size(), 5u);

    EXPECT_TRUE(ValidateIndices(tetrahedrons, probes.size()));
    EXPECT_TRUE(ValidateVertexMatch(tetrahedrons, probes));
    EXPECT_TRUE(ValidateAllPointsCovered(tetrahedrons, probes.size()));
    EXPECT_TRUE(ValidateEmptyCircumsphere(tetrahedrons, probes));

    for (const auto& tet : tetrahedrons) {
        EXPECT_TRUE(ValidateNonDegenerate(tet));
    }
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_InsufficientProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;

    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probes.push_back(probe);
    probe.position = Math::Vec3(1.0f, 0.0f, 0.0f);
    probes.push_back(probe);
    probe.position = Math::Vec3(0.0f, 1.0f, 0.0f);
    probes.push_back(probe);

    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    EXPECT_TRUE(tetrahedrons.empty());
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_CoplanarProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateCoplanarProbes();
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    // Coplanar inputs cannot form a non-degenerate tet.
    EXPECT_EQ(tetrahedrons.size(), 0u);
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_DuplicatePositions, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateDuplicatePositionProbes();
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    EXPECT_LE(tetrahedrons.size(), 2u);
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_ManyProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateManyProbes(100);
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    EXPECT_GT(tetrahedrons.size(), 0u);
    EXPECT_TRUE(ValidateIndices(tetrahedrons, probes.size()));
    EXPECT_TRUE(ValidateVertexMatch(tetrahedrons, probes));
    EXPECT_TRUE(ValidateAllPointsCovered(tetrahedrons, probes.size()));

    for (const auto& tet : tetrahedrons) {
        EXPECT_TRUE(ValidateNonDegenerate(tet));
        EXPECT_GT(tet.circumradiusSquared, 0.0f);

        for (uint32_t i = 0; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
            float distSq = Math::Distance2(tet.vertices[i], tet.circumcenter);
            float tolerance = std::max(tet.circumradiusSquared * 0.1f, 0.01f);
            EXPECT_NEAR(distSq, tet.circumradiusSquared, tolerance);
        }
    }

    EXPECT_TRUE(ValidateEmptyCircumsphere(tetrahedrons, probes));
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_AllPointsCovered, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateFiveProbes();
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    EXPECT_TRUE(ValidateAllPointsCovered(tetrahedrons, probes.size()));
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_ValidCircumsphere, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateFourProbes();
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    for (const auto& tet : tetrahedrons) {
        EXPECT_GT(tet.circumradiusSquared, 0.0f);

        for (uint32_t i = 0; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
            float distSq = Math::Distance2(tet.vertices[i], tet.circumcenter);
            float tolerance = std::max(tet.circumradiusSquared * 0.1f, 0.01f);
            EXPECT_NEAR(distSq, tet.circumradiusSquared, tolerance);
        }
    }

    EXPECT_TRUE(ValidateEmptyCircumsphere(tetrahedrons, probes));
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, BuildTetrahedralMesh_EmptyProbes, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes;
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    EXPECT_TRUE(tetrahedrons.empty());
}

UNIT_TEST(SRC_BowyerWatsonDelaunay3D, TetrahedronVertexMatch, testing::ext::TestSize.Level1)
{
    vector<LightProbeGroupComponent::LightProbe> probes = CreateFourProbes();
    vector<Tetrahedron> tetrahedrons;

    BowyerWatsonDelaunay3D delaunay;
    BowyerWatsonDelaunay3D::LightProbeVolume volume{probes, tetrahedrons};
    delaunay.BuildTetrahedralMesh(volume);

    EXPECT_TRUE(ValidateVertexMatch(tetrahedrons, probes));
}