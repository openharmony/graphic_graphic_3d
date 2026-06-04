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
#include <util/light_probe_util.h>

#include <3d/ecs/components/light_probe_group_component.h>
#include <3d/light_probe_types/light_probe.h>
#include <3d/light_probe_types/light_probe_constants.h>
#include <3d/render/intf_render_data_store_light_probe.h>
#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace RENDER_NS;

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

vector<LightProbeGroupComponent::LightProbe> CreateTestProbesForIntegration()
{
    vector<LightProbeGroupComponent::LightProbe> probes;

    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(-2.0f, -2.0f, 2.0f);
    probe.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
    probe.ao = 0.8f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.6f, 0.7f, 1.0f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(2.0f, -2.0f, 2.0f);
    probe.bentNormal = Math::Vec3(1.0f, 0.0f, 0.0f);
    probe.ao = 0.6f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.8f, 0.7f, 0.5f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, -2.0f, -2.0f);
    probe.bentNormal = Math::Vec3(0.0f, 0.0f, 1.0f);
    probe.ao = 0.4f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.5f, 0.4f, 0.3f);
    }
    probes.push_back(probe);

    probe.position = Math::Vec3(0.0f, 2.0f, 0.0f);
    probe.bentNormal = Math::Vec3(0.5f, 0.5f, 0.5f);
    probe.ao = 0.5f;
    for (uint32_t i = 0; i < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++i) {
        probe.shCoefficients[i] = Math::Vec3(0.3f, 0.35f, 0.4f);
    }
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
        probe.bentNormal = Math::Vec3(0.0f, 1.0f, 0.0f);
        probe.ao = 0.5f;
        for (uint32_t j = 0; j < LightProbeConstants::LIGHT_PROBE_SH_COEFFICIENT_COUNT; ++j) {
            probe.shCoefficients[j] = Math::Vec3(0.5f, 0.5f, 0.5f);
        }
        probes.push_back(probe);
    }

    return probes;
}

vector<Tetrahedron> ConvertArrayViewToVector(BASE_NS::array_view<const Tetrahedron> view)
{
    vector<Tetrahedron> result;
    for (const auto& tet : view) {
        result.push_back(tet);
    }
    return result;
}

vector<LightProbeGroupComponent::LightProbe> ConvertArrayViewToVectorProbes(
    BASE_NS::array_view<const LightProbeGroupComponent::LightProbe> view)
{
    vector<LightProbeGroupComponent::LightProbe> result;
    for (const auto& probe : view) {
        result.push_back(probe);
    }
    return result;
}
}  // namespace

UNIT_TEST(SRC_RenderDataStoreLightProbe, AddLightProbes_ValidData, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    EXPECT_TRUE(volume.valid);
    EXPECT_EQ(volume.lightProbeVolume.lightProbes.size(), probes.size());
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_ValidId, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    bool result = lightProbeStore->BuildTetrahedralMesh(testId);

    EXPECT_TRUE(result);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, GetLightProbeVolume_AfterBuild, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);

    EXPECT_TRUE(volume.valid);
    EXPECT_EQ(volume.lightProbeVolume.lightProbes.size(), probes.size());
    EXPECT_FALSE(volume.lightProbeVolume.tetrahedrons.empty());
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, RemoveLightProbes_Cleanup, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volumeBefore = lightProbeStore->GetLightProbeVolume(testId);
    EXPECT_TRUE(volumeBefore.valid);

    lightProbeStore->RemoveLightProbes(testId);

    LightProbeVolumeOpt volumeAfter = lightProbeStore->GetLightProbeVolume(testId);
    EXPECT_FALSE(volumeAfter.valid);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, GetLightProbeVolume_InvalidId, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    uint64_t invalidId = 99999u;

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(invalidId);

    EXPECT_FALSE(volume.valid);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_InsufficientProbes, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes;
    LightProbeGroupComponent::LightProbe probe;
    probe.position = Math::Vec3(0.0f, 0.0f, 0.0f);
    probes.push_back(probe);
    probe.position = Math::Vec3(1.0f, 0.0f, 0.0f);
    probes.push_back(probe);

    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    bool result = lightProbeStore->BuildTetrahedralMesh(testId);

    EXPECT_FALSE(result);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, MultipleLightProbeVolumes, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes1 = CreateTestProbesForIntegration();
    vector<LightProbeGroupComponent::LightProbe> probes2 = CreateManyProbes(10);

    uint64_t id1 = 1u;
    uint64_t id2 = 2u;

    lightProbeStore->AddLightProbes(id1, probes1);
    lightProbeStore->AddLightProbes(id2, probes2);

    lightProbeStore->BuildTetrahedralMesh(id1);
    lightProbeStore->BuildTetrahedralMesh(id2);

    LightProbeVolumeOpt volume1 = lightProbeStore->GetLightProbeVolume(id1);
    LightProbeVolumeOpt volume2 = lightProbeStore->GetLightProbeVolume(id2);

    EXPECT_TRUE(volume1.valid);
    EXPECT_TRUE(volume2.valid);

    EXPECT_NE(volume1.lightProbeVolume.lightProbes.size(), volume2.lightProbeVolume.lightProbes.size());
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_DelaunayValidation, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    vector<Tetrahedron> tets = ConvertArrayViewToVector(volume.lightProbeVolume.tetrahedrons);
    vector<LightProbeGroupComponent::LightProbe> probesCopy =
        ConvertArrayViewToVectorProbes(volume.lightProbeVolume.lightProbes);

    EXPECT_TRUE(ValidateIndices(tets, probes.size()));
    EXPECT_TRUE(ValidateEmptyCircumsphere(tets, probesCopy));
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_VertexIndexMatch, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    vector<Tetrahedron> tets = ConvertArrayViewToVector(volume.lightProbeVolume.tetrahedrons);
    vector<LightProbeGroupComponent::LightProbe> probesCopy =
        ConvertArrayViewToVectorProbes(volume.lightProbeVolume.lightProbes);

    EXPECT_TRUE(ValidateVertexMatch(tets, probesCopy));
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_NonDegenerateTetrahedrons, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    for (const auto& tet : volume.lightProbeVolume.tetrahedrons) {
        EXPECT_TRUE(ValidateNonDegenerate(tet));
    }
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_AllPointsCovered, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    vector<Tetrahedron> tets = ConvertArrayViewToVector(volume.lightProbeVolume.tetrahedrons);
    EXPECT_TRUE(ValidateAllPointsCovered(tets, probes.size()));
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_ManyProbes, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateManyProbes(100);
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    bool result = lightProbeStore->BuildTetrahedralMesh(testId);

    EXPECT_TRUE(result);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    vector<Tetrahedron> tets = ConvertArrayViewToVector(volume.lightProbeVolume.tetrahedrons);
    vector<LightProbeGroupComponent::LightProbe> probesCopy =
        ConvertArrayViewToVectorProbes(volume.lightProbeVolume.lightProbes);

    EXPECT_GT(tets.size(), 0u);
    EXPECT_TRUE(ValidateIndices(tets, probes.size()));
    EXPECT_TRUE(ValidateVertexMatch(tets, probesCopy));
    EXPECT_TRUE(ValidateAllPointsCovered(tets, probes.size()));

    for (const auto& tet : tets) {
        EXPECT_TRUE(ValidateNonDegenerate(tet));
        EXPECT_GT(tet.circumradiusSquared, 0.0f);
    }

    EXPECT_TRUE(ValidateEmptyCircumsphere(tets, probesCopy));
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, AddLightProbes_EmptyList, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> emptyProbes;
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, emptyProbes);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    EXPECT_TRUE(volume.valid);
    EXPECT_EQ(volume.lightProbeVolume.lightProbes.size(), 0u);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, AddLightProbes_OverwriteExistingId, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes1 = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes1);
    LightProbeVolumeOpt volume1 = lightProbeStore->GetLightProbeVolume(testId);
    EXPECT_EQ(volume1.lightProbeVolume.lightProbes.size(), probes1.size());

    vector<LightProbeGroupComponent::LightProbe> probes2 = CreateManyProbes(10);
    lightProbeStore->AddLightProbes(testId, probes2);

    LightProbeVolumeOpt volume2 = lightProbeStore->GetLightProbeVolume(testId);
    EXPECT_EQ(volume2.lightProbeVolume.lightProbes.size(), probes2.size());
    EXPECT_NE(volume1.lightProbeVolume.lightProbes.size(), volume2.lightProbeVolume.lightProbes.size());
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, AddRemoveAddCycle, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes1 = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes1);
    EXPECT_TRUE(lightProbeStore->GetLightProbeVolume(testId).valid);

    lightProbeStore->RemoveLightProbes(testId);
    EXPECT_FALSE(lightProbeStore->GetLightProbeVolume(testId).valid);

    vector<LightProbeGroupComponent::LightProbe> probes2 = CreateManyProbes(20);
    lightProbeStore->AddLightProbes(testId, probes2);
    EXPECT_TRUE(lightProbeStore->GetLightProbeVolume(testId).valid);
    EXPECT_EQ(lightProbeStore->GetLightProbeVolume(testId).lightProbeVolume.lightProbes.size(), probes2.size());
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, BuildTetrahedralMesh_NonExistentId, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    uint64_t nonExistentId = 99999u;
    bool result = lightProbeStore->BuildTetrahedralMesh(nonExistentId);

    EXPECT_FALSE(result);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, RemoveLightProbes_NonExistentId, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;
    lightProbeStore->AddLightProbes(testId, probes);

    uint64_t nonExistentId = 99999u;
    lightProbeStore->RemoveLightProbes(nonExistentId);

    EXPECT_TRUE(lightProbeStore->GetLightProbeVolume(testId).valid);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, GetTypeName_ReturnsCorrectValue, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    EXPECT_EQ(dataStore->GetTypeName(), "RenderDataStoreLightProbe");
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, GetUid_ReturnsCorrectValue, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    EXPECT_EQ(lightProbeStore->GetUid(), IRenderDataStoreLightProbe::UID);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, GetName_ReturnsGivenName, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    constexpr string_view customName = "CustomLightProbeStore";
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, customName.data());
    ASSERT_TRUE(dataStore);

    EXPECT_EQ(dataStore->GetName(), customName);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, Integration_FindContainingTetrahedron, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    Math::Vec3 insidePoint(0.0f, 0.0f, 0.0f);
    TetrahedronOpt tetOpt = LightProbeUtil::FindContainingTetrahedron(insidePoint, volume.lightProbeVolume);

    EXPECT_TRUE(tetOpt.valid);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, Integration_GetInterpolatedData_ValidPosition, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    Math::Vec3 insidePoint(0.0f, 0.0f, 0.0f);
    LightProbeInterpolatedDataOpt dataOpt =
        LightProbeUtil::GetInterpolatedLightProbeData(volume.lightProbeVolume, insidePoint);

    EXPECT_TRUE(dataOpt.valid);
}

UNIT_TEST(SRC_RenderDataStoreLightProbe, Integration_GetInterpolatedData_OutsideVolume, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto renderContext = testContext->renderContext;
    ASSERT_TRUE(renderContext);

    auto& dataStoreManager = renderContext->GetRenderDataStoreManager();
    auto dataStore = dataStoreManager.Create(IRenderDataStoreLightProbe::UID, "LightProbeDataStore");
    ASSERT_TRUE(dataStore);

    auto lightProbeStore = static_cast<IRenderDataStoreLightProbe*>(dataStore.get());
    ASSERT_NE(lightProbeStore, nullptr);

    vector<LightProbeGroupComponent::LightProbe> probes = CreateTestProbesForIntegration();
    uint64_t testId = 12345u;

    lightProbeStore->AddLightProbes(testId, probes);
    lightProbeStore->BuildTetrahedralMesh(testId);

    LightProbeVolumeOpt volume = lightProbeStore->GetLightProbeVolume(testId);
    ASSERT_TRUE(volume.valid);

    Math::Vec3 outsidePoint(100.0f, 100.0f, 100.0f);
    LightProbeInterpolatedDataOpt dataOpt =
        LightProbeUtil::GetInterpolatedLightProbeData(volume.lightProbeVolume, outsidePoint);

    EXPECT_TRUE(dataOpt.valid);
}