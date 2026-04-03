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

#include <limits>

#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/util/intf_mesh_util.h>
#include <base/math/vector.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/plugin/intf_plugin.h>

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
template<typename ComponentManager>
void EnsureComponentManager(IEcs& ecs)
{
    if (GetManager<ComponentManager>(ecs)) {
        return;
    }

    bool created = false;
    for (const auto* component : CORE3D_NS::GetPluginRegister().GetTypeInfos(ComponentManagerTypeInfo::UID)) {
        const auto* info = static_cast<const ComponentManagerTypeInfo*>(component);
        if (info->uid == ComponentManager::UID) {
            EXPECT_TRUE(ecs.CreateComponentManager(*info));
            created = true;
            break;
        }
    }

    ASSERT_TRUE(created);
    ASSERT_NE(nullptr, GetManager<ComponentManager>(ecs));
}

void EnsureMeshResourceManagers(IEcs& ecs)
{
    EnsureComponentManager<IMeshComponentManager>(ecs);
    EnsureComponentManager<INameComponentManager>(ecs);
    EnsureComponentManager<IUriComponentManager>(ecs);
}

void ExpectMeshCreationFails(IEcs& ecs, const char* testCase, Entity meshEntity)
{
    SCOPED_TRACE(testCase);
    EXPECT_EQ(Entity {}, meshEntity);
    if ((meshEntity != Entity {}) && ecs.GetEntityManager().IsAlive(meshEntity)) {
        ecs.GetEntityManager().Destroy(meshEntity);
    }
}
} // namespace

/**
 * @tc.name: GenerateCubeMeshCreatesNamedMeshResource
 * @tc.desc: Tests for Generate Cube Mesh Creates Named Mesh Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshUtil, GenerateCubeMeshCreatesNamedMeshResource, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto ecs = testContext->ecs;
    ASSERT_TRUE(ecs);
    EnsureMeshResourceManagers(*ecs);

    auto& meshUtil = testContext->graphicsContext->GetMeshUtil();
    const Entity meshEntity = meshUtil.GenerateCubeMesh(*ecs, "namedCube", {}, 1.0f, 2.0f, 3.0f);

    ASSERT_NE(Entity {}, meshEntity);
    ASSERT_TRUE(ecs->GetEntityManager().IsAlive(meshEntity));

    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    auto nameManager = GetManager<INameComponentManager>(*ecs);
    auto uriManager = GetManager<IUriComponentManager>(*ecs);
    ASSERT_NE(nullptr, meshManager);
    ASSERT_NE(nullptr, nameManager);
    ASSERT_NE(nullptr, uriManager);
    ASSERT_TRUE(meshManager->HasComponent(meshEntity));
    ASSERT_TRUE(nameManager->HasComponent(meshEntity));
    ASSERT_TRUE(uriManager->HasComponent(meshEntity));

    const auto meshHandle = meshManager->Read(meshEntity);
    ASSERT_TRUE(meshHandle);
    ASSERT_EQ(1u, meshHandle->submeshes.size());
    EXPECT_EQ(Math::Vec3(-0.5f, -1.0f, -1.5f), meshHandle->aabbMin);
    EXPECT_EQ(Math::Vec3(0.5f, 1.0f, 1.5f), meshHandle->aabbMax);
    EXPECT_EQ(36u, meshHandle->submeshes[0].vertexCount);
    EXPECT_EQ(36u, meshHandle->submeshes[0].indexCount);

    const auto nameHandle = nameManager->Read(meshEntity);
    const auto uriHandle = uriManager->Read(meshEntity);
    ASSERT_TRUE(nameHandle);
    ASSERT_TRUE(uriHandle);
    EXPECT_EQ("namedCube", nameHandle->name);
    EXPECT_EQ("namedCube", uriHandle->uri);
}

/**
 * @tc.name: GeneratePlaneMeshRejectsDegenerateAndNonFiniteDimensions
 * @tc.desc: Tests for Generate Plane Mesh Rejects Degenerate And Non Finite Dimensions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshUtil, GeneratePlaneMeshRejectsDegenerateAndNonFiniteDimensions, testing::ext::TestSize.Level1)
{
    struct PlaneCase {
        const char* name;
        float width;
        float depth;
    };

    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const PlaneCase testCases[] = {
        { "zero_width", 0.0f, 1.0f },
        { "zero_depth", 1.0f, 0.0f },
        { "negative_width", -1.0f, 1.0f },
        { "negative_depth", 1.0f, -1.0f },
        { "infinite_width", inf, 1.0f },
        { "nan_depth", 1.0f, nan },
    };

    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto ecs = testContext->ecs;
    ASSERT_TRUE(ecs);
    EnsureMeshResourceManagers(*ecs);
    auto& meshUtil = testContext->graphicsContext->GetMeshUtil();

    for (const auto& testCase : testCases) {
        ExpectMeshCreationFails(
            *ecs, testCase.name, meshUtil.GeneratePlaneMesh(*ecs, "", {}, testCase.width, testCase.depth));
    }
}

/**
 * @tc.name: GenerateCubeMeshRejectsDegenerateAndNonFiniteDimensions
 * @tc.desc: Tests for Generate Cube Mesh Rejects Degenerate And Non Finite Dimensions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshUtil, GenerateCubeMeshRejectsDegenerateAndNonFiniteDimensions, testing::ext::TestSize.Level1)
{
    struct CubeCase {
        const char* name;
        float width;
        float height;
        float depth;
    };

    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const CubeCase testCases[] = {
        { "zero_width", 0.0f, 1.0f, 1.0f },
        { "zero_height", 1.0f, 0.0f, 1.0f },
        { "zero_depth", 1.0f, 1.0f, 0.0f },
        { "negative_width", -1.0f, 1.0f, 1.0f },
        { "negative_height", 1.0f, -1.0f, 1.0f },
        { "negative_depth", 1.0f, 1.0f, -1.0f },
        { "infinite_width", inf, 1.0f, 1.0f },
        { "nan_height", 1.0f, nan, 1.0f },
    };

    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto ecs = testContext->ecs;
    ASSERT_TRUE(ecs);
    EnsureMeshResourceManagers(*ecs);
    auto& meshUtil = testContext->graphicsContext->GetMeshUtil();

    for (const auto& testCase : testCases) {
        ExpectMeshCreationFails(*ecs, testCase.name,
            meshUtil.GenerateCubeMesh(*ecs, "", {}, testCase.width, testCase.height, testCase.depth));
    }
}

/**
 * @tc.name: GenerateSphereMeshRejectsInvalidRadiusAndTopology
 * @tc.desc: Tests for Generate Sphere Mesh Rejects Invalid Radius And Topology. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshUtil, GenerateSphereMeshRejectsInvalidRadiusAndTopology, testing::ext::TestSize.Level1)
{
    struct SphereCase {
        const char* name;
        float radius;
        uint32_t rings;
        uint32_t sectors;
    };

    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const SphereCase testCases[] = {
        { "zero_radius", 0.0f, 8u, 8u },
        { "negative_radius", -1.0f, 8u, 8u },
        { "zero_rings", 1.0f, 0u, 8u },
        { "one_ring", 1.0f, 1u, 8u },
        { "zero_sectors", 1.0f, 8u, 0u },
        { "one_sector", 1.0f, 8u, 1u },
        { "infinite_radius", inf, 8u, 8u },
        { "nan_radius", nan, 8u, 8u },
    };

    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto ecs = testContext->ecs;
    ASSERT_TRUE(ecs);
    EnsureMeshResourceManagers(*ecs);
    auto& meshUtil = testContext->graphicsContext->GetMeshUtil();

    for (const auto& testCase : testCases) {
        ExpectMeshCreationFails(*ecs, testCase.name,
            meshUtil.GenerateSphereMesh(*ecs, "", {}, testCase.radius, testCase.rings, testCase.sectors));
    }
}

/**
 * @tc.name: GenerateConeMeshRejectsInvalidRadiusLengthAndSectors
 * @tc.desc: Tests for Generate Cone Mesh Rejects Invalid Radius Length And Sectors. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshUtil, GenerateConeMeshRejectsInvalidRadiusLengthAndSectors, testing::ext::TestSize.Level1)
{
    struct ConeCase {
        const char* name;
        float radius;
        float length;
        uint32_t sectors;
    };

    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const ConeCase testCases[] = {
        { "zero_radius", 0.0f, 1.0f, 8u },
        { "negative_radius", -1.0f, 1.0f, 8u },
        { "zero_length", 1.0f, 0.0f, 8u },
        { "negative_length", 1.0f, -1.0f, 8u },
        { "zero_sectors", 1.0f, 1.0f, 0u },
        { "one_sector", 1.0f, 1.0f, 1u },
        { "two_sectors", 1.0f, 1.0f, 2u },
        { "infinite_length", 1.0f, inf, 8u },
        { "nan_radius", nan, 1.0f, 8u },
    };

    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto ecs = testContext->ecs;
    ASSERT_TRUE(ecs);
    EnsureMeshResourceManagers(*ecs);
    auto& meshUtil = testContext->graphicsContext->GetMeshUtil();

    for (const auto& testCase : testCases) {
        ExpectMeshCreationFails(*ecs, testCase.name,
            meshUtil.GenerateConeMesh(*ecs, "", {}, testCase.radius, testCase.length, testCase.sectors));
    }
}

/**
 * @tc.name: GenerateTorusMeshRejectsInvalidRadiiAndSectorCounts
 * @tc.desc: Tests for Generate Torus Mesh Rejects Invalid Radii And Sector Counts. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshUtil, GenerateTorusMeshRejectsInvalidRadiiAndSectorCounts, testing::ext::TestSize.Level1)
{
    struct TorusCase {
        const char* name;
        float majorRadius;
        float minorRadius;
        uint32_t majorSectors;
        uint32_t minorSectors;
    };

    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const TorusCase testCases[] = {
        { "zero_major_radius", 0.0f, 0.25f, 8u, 8u },
        { "zero_minor_radius", 1.0f, 0.0f, 8u, 8u },
        { "negative_major_radius", -1.0f, 0.25f, 8u, 8u },
        { "negative_minor_radius", 1.0f, -0.25f, 8u, 8u },
        { "zero_major_sectors", 1.0f, 0.25f, 0u, 8u },
        { "one_major_sector", 1.0f, 0.25f, 1u, 8u },
        { "two_major_sectors", 1.0f, 0.25f, 2u, 8u },
        { "zero_minor_sectors", 1.0f, 0.25f, 8u, 0u },
        { "one_minor_sector", 1.0f, 0.25f, 8u, 1u },
        { "two_minor_sectors", 1.0f, 0.25f, 8u, 2u },
        { "infinite_major_radius", inf, 0.25f, 8u, 8u },
        { "nan_minor_radius", 1.0f, nan, 8u, 8u },
    };

    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto ecs = testContext->ecs;
    ASSERT_TRUE(ecs);
    EnsureMeshResourceManagers(*ecs);
    auto& meshUtil = testContext->graphicsContext->GetMeshUtil();

    for (const auto& testCase : testCases) {
        ExpectMeshCreationFails(*ecs, testCase.name,
            meshUtil.GenerateTorusMesh(*ecs, "", {}, testCase.majorRadius, testCase.minorRadius, testCase.majorSectors,
                testCase.minorSectors));
    }
}

/**
 * @tc.name: GenerateCylinderMeshRejectsInvalidRadiusHeightSegmentsAndNonFiniteValues
 * @tc.desc: Tests for Generate Cylinder Mesh Rejects Invalid Radius Height Segments And Non Finite Values.
 * [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshUtil, GenerateCylinderMeshRejectsInvalidRadiusHeightSegmentsAndNonFiniteValues,
    testing::ext::TestSize.Level1)
{
    struct CylinderCase {
        const char* name;
        float radius;
        float height;
        uint32_t segments;
    };

    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const CylinderCase testCases[] = {
        { "zero_radius", 0.0f, 1.0f, 8u },
        { "negative_radius", -1.0f, 1.0f, 8u },
        { "zero_height", 1.0f, 0.0f, 8u },
        { "negative_height", 1.0f, -1.0f, 8u },
        { "zero_segments", 1.0f, 1.0f, 0u },
        { "one_segment", 1.0f, 1.0f, 1u },
        { "two_segments", 1.0f, 1.0f, 2u },
        { "infinite_radius", inf, 1.0f, 8u },
        { "nan_height", 1.0f, nan, 8u },
    };

    UTest::TestContext* testContext = UTest::RecreateTestContext();
    ASSERT_NE(nullptr, testContext);

    auto ecs = testContext->ecs;
    ASSERT_TRUE(ecs);
    EnsureMeshResourceManagers(*ecs);
    auto& meshUtil = testContext->graphicsContext->GetMeshUtil();

    for (const auto& testCase : testCases) {
        ExpectMeshCreationFails(*ecs, testCase.name,
            meshUtil.GenerateCylinderMesh(*ecs, "", {}, testCase.radius, testCase.height, testCase.segments));
    }
}