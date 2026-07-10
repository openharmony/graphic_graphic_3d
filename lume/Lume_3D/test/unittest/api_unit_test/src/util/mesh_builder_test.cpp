/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <algorithm>

#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <3d/util/intf_mesh_builder.h>
#include <base/math/float_packer.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>
#include <render/device/intf_shader_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
template<typename T>
inline IMeshBuilder::DataBuffer FillData(array_view<const T> c) noexcept
{
    Format format = BASE_FORMAT_UNDEFINED;
    if constexpr (is_same_v<T, Math::Vec2>) {
        format = BASE_FORMAT_R32G32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec3>) {
        format = BASE_FORMAT_R32G32B32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec4>) {
        format = BASE_FORMAT_R32G32B32A32_SFLOAT;
    } else if constexpr (is_same_v<T, uint16_t>) {
        format = BASE_FORMAT_R16_UINT;
    } else if constexpr (is_same_v<T, uint32_t>) {
        format = BASE_FORMAT_R32_UINT;
    }
    return IMeshBuilder::DataBuffer{
        format, sizeof(T), {reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(T)}};
}

template<typename T>
inline IMeshBuilder::DataBuffer FillData(const vector<T>& c) noexcept
{
    return FillData(array_view<const T>{c});
}

template<typename T, size_t N>
inline IMeshBuilder::DataBuffer FillData(const T (&c)[N]) noexcept
{
    return FillData(array_view<const T>{c, N});
}

VertexInputDeclarationView GetForwardVertexInputDeclaration(IRenderContext& renderContext)
{
    IShaderManager& shaderManager = renderContext.GetDevice().GetShaderManager();
    return shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
        DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));
}

struct TriangleData {
    vector<Math::Vec3> positions;
    vector<Math::Vec3> normals;
    vector<Math::Vec2> uvs;
    vector<Math::Vec4> tangents;
    vector<Math::Vec4> colors;
    vector<uint16_t> indices16;
    vector<uint32_t> indices32;
    vector<Math::Vec4> joints;
    vector<Math::Vec4> weights;
};

TriangleData MakeTriangle()
{
    TriangleData d;
    d.positions = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    d.normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    d.uvs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}};
    d.tangents = {{1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};
    d.colors = {{1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
    d.indices16 = {0U, 1U, 2U};
    d.indices32 = {0U, 1U, 2U};
    d.joints = {{0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}};
    d.weights = {{1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}};
    return d;
}
}  // namespace

/**
 * @tc.name: GetInterfaceTest
 * @tc.desc: Tests for Get Interface Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, GetInterfaceTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);

    EXPECT_TRUE(meshBuilder->GetInterface(IMeshBuilder::UID));
    EXPECT_TRUE(meshBuilder->GetInterface(IInterface::UID));
    EXPECT_FALSE(meshBuilder->GetInterface(IClassFactory::UID));

    const IMeshBuilder* meshBuilderConst = static_cast<const IMeshBuilder*>(meshBuilder.get());
    EXPECT_TRUE(meshBuilderConst->GetInterface(IMeshBuilder::UID));
    EXPECT_TRUE(meshBuilderConst->GetInterface(IInterface::UID));
    EXPECT_FALSE(meshBuilderConst->GetInterface(IClassFactory::UID));
}

/**
 * @tc.name: InitializeOverloads
 * @tc.desc: Both Initialize overloads (with/without Configuration) work.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, InitializeOverloads, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    meshBuilder->Initialize(vid, 2U);
    EXPECT_EQ(0U, meshBuilder->GetVertexCount());
    EXPECT_EQ(0U, meshBuilder->GetIndexCount());

    IMeshBuilder::Configuration cfg{vid, 3U, 0U};
    meshBuilder->Initialize(cfg);
    EXPECT_EQ(0U, meshBuilder->GetVertexCount());

    IMeshBuilder::Configuration cfgNoStaging{vid, 1U, IMeshBuilder::ConfigurationFlagBits::NO_STAGING_BUFFER};
    meshBuilder->Initialize(cfgNoStaging);
    EXPECT_EQ(0U, meshBuilder->GetVertexCount());
}

/**
 * @tc.name: AddAndGetSubmesh
 * @tc.desc: AddSubmesh stores, GetSubmesh returns matching info; out-of-range returns default.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, AddAndGetSubmesh, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    meshBuilder->Initialize(vid, 2U);

    IMeshBuilder::Submesh sm0{};
    sm0.vertexCount = 4U;
    sm0.indexCount = 6U;
    sm0.indexType = CORE_INDEX_TYPE_UINT16;
    sm0.tangents = true;
    sm0.colors = true;
    sm0.joints = false;
    meshBuilder->AddSubmesh(sm0);

    IMeshBuilder::Submesh sm1{};
    sm1.vertexCount = 3U;
    sm1.indexCount = 3U;
    sm1.indexType = CORE_INDEX_TYPE_UINT32;
    sm1.joints = true;
    meshBuilder->AddSubmesh(sm1);

    EXPECT_EQ(4U, meshBuilder->GetSubmesh(0U).vertexCount);
    EXPECT_EQ(CORE_INDEX_TYPE_UINT16, meshBuilder->GetSubmesh(0U).indexType);
    EXPECT_TRUE(meshBuilder->GetSubmesh(0U).tangents);

    EXPECT_EQ(3U, meshBuilder->GetSubmesh(1U).vertexCount);
    EXPECT_EQ(CORE_INDEX_TYPE_UINT32, meshBuilder->GetSubmesh(1U).indexType);
    EXPECT_TRUE(meshBuilder->GetSubmesh(1U).joints);
}

/**
 * @tc.name: AllocateAndGetCounts
 * @tc.desc: Allocate produces nonzero counts and getters return data after fills.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, AllocateAndGetCounts, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    meshBuilder->Initialize(vid, 1U);

    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    EXPECT_EQ(3U, meshBuilder->GetVertexCount());
    EXPECT_GT(meshBuilder->GetIndexCount(), 0U);
    EXPECT_EQ(1U, meshBuilder->GetSubmeshes().size());
}

/**
 * @tc.name: SetVertexDataMinimal
 * @tc.desc: SetVertexData with only required attributes; getters return non-empty buffers.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetVertexDataMinimal, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = static_cast<uint32_t>(t.positions.size());
    sm.indexCount = static_cast<uint32_t>(t.indices16.size());
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    meshBuilder->SetIndexData(0U, FillData(t.indices16));

    EXPECT_LT(0U, meshBuilder->GetVertexData().size());
    EXPECT_LT(0U, meshBuilder->GetIndexData().size());
    EXPECT_EQ(0U, meshBuilder->GetJointData().size());
    EXPECT_EQ(0U, meshBuilder->GetMorphTargetData().size());
}

/**
 * @tc.name: SetVertexDataFull
 * @tc.desc: SetVertexData with all optional attributes (texcoords1, tangents, colors).
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetVertexDataFull, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = static_cast<uint32_t>(t.positions.size());
    sm.indexCount = static_cast<uint32_t>(t.indices32.size());
    sm.indexType = CORE_INDEX_TYPE_UINT32;
    sm.tangents = true;
    sm.colors = true;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    meshBuilder->SetVertexData(0U,
        FillData(t.positions),
        FillData(t.normals),
        FillData(t.uvs),
        FillData(t.uvs),
        FillData(t.tangents),
        FillData(t.colors));
    meshBuilder->SetIndexData(0U, FillData(t.indices32));

    EXPECT_LT(0U, meshBuilder->GetVertexData().size());
    EXPECT_LT(0U, meshBuilder->GetIndexData().size());
}

/**
 * @tc.name: SetVertexDataGenerateTangents
 * @tc.desc: tangents=true with empty tangents buffer triggers tangent generation path.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetVertexDataGenerateTangents, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = static_cast<uint32_t>(t.positions.size());
    sm.indexCount = static_cast<uint32_t>(t.indices16.size());
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.tangents = true;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    meshBuilder->SetIndexData(0U, FillData(t.indices16));
    EXPECT_LT(0U, meshBuilder->GetVertexData().size());
}

/**
 * @tc.name: SetVertexDataMissingNormals
 * @tc.desc: Empty normals buffer triggers normal-generation copy path.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetVertexDataMissingNormals, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = static_cast<uint32_t>(t.positions.size());
    sm.indexCount = static_cast<uint32_t>(t.indices16.size());
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    meshBuilder->SetVertexData(0U, FillData(t.positions), {}, FillData(t.uvs), {}, {}, {});
    meshBuilder->SetIndexData(0U, FillData(t.indices16));
    EXPECT_LT(0U, meshBuilder->GetVertexData().size());
}

/**
 * @tc.name: SetIndexDataUInt16AndUInt32
 * @tc.desc: SetIndexData both index types take their respective branches.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetIndexDataUInt16AndUInt32, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
        mb->SetIndexData(0U, FillData(t.indices16));
        EXPECT_LT(0U, mb->GetIndexData().size());
    }
    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.indexType = CORE_INDEX_TYPE_UINT32;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
        mb->SetIndexData(0U, FillData(t.indices32));
        EXPECT_LT(0U, mb->GetIndexData().size());
    }
}

/**
 * @tc.name: SetAABBAndCalculateAABB
 * @tc.desc: SetAABB sets explicit bounds; CalculateAABB computes from positions.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetAABBAndCalculateAABB, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();
    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    meshBuilder->SetIndexData(0U, FillData(t.indices16));

    meshBuilder->SetAABB(0U, Math::Vec3{-1.0f, -1.0f, -1.0f}, Math::Vec3{2.0f, 2.0f, 2.0f});
    auto submeshes = meshBuilder->GetSubmeshes();
    ASSERT_EQ(1U, submeshes.size());
    EXPECT_EQ(-1.0f, submeshes[0].aabbMin.x);
    EXPECT_EQ(2.0f, submeshes[0].aabbMax.x);

    meshBuilder->CalculateAABB(0U, FillData(t.positions));
    submeshes = meshBuilder->GetSubmeshes();
    EXPECT_FLOAT_EQ(0.0f, submeshes[0].aabbMin.x);
    EXPECT_FLOAT_EQ(1.0f, submeshes[0].aabbMax.x);
}

/**
 * @tc.name: CalculateAABBVec2Positions
 * @tc.desc: CalculateAABB path for R32G32_SFLOAT positions.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, CalculateAABBVec2Positions, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();
    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});

    vector<Math::Vec2> pos2{{-2.0f, -3.0f}, {5.0f, -1.0f}, {1.0f, 4.0f}};
    meshBuilder->CalculateAABB(0U, FillData(pos2));
    auto submeshes = meshBuilder->GetSubmeshes();
    EXPECT_FLOAT_EQ(-2.0f, submeshes[0].aabbMin.x);
    EXPECT_FLOAT_EQ(5.0f, submeshes[0].aabbMax.x);
}

/**
 * @tc.name: CalculateAABBUnsupportedFormat
 * @tc.desc: CalculateAABB with undefined format takes the early-return branch.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, CalculateAABBUnsupportedFormat, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    uint8_t bogus[12] = {};
    IMeshBuilder::DataBuffer bad{BASE_FORMAT_UNDEFINED, 4U, {bogus, sizeof(bogus)}};
    meshBuilder->CalculateAABB(0U, bad);
    // Should not crash; aabb remains default (unset).
    EXPECT_EQ(1U, meshBuilder->GetSubmeshes().size());
}

/**
 * @tc.name: SetJointData
 * @tc.desc: SetJointData populates joint buffer and joint bounds.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetJointData, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.joints = true;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();
    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    meshBuilder->SetIndexData(0U, FillData(t.indices16));
    meshBuilder->SetJointData(0U, FillData(t.joints), FillData(t.weights), FillData(t.positions));

    EXPECT_LT(0U, meshBuilder->GetJointData().size());
    EXPECT_LT(0U, meshBuilder->GetJointBoundsData().size());
}

/**
 * @tc.name: SetMorphTargetDataPaths
 * @tc.desc: Run SetMorphTargetData with multiple combinations of normal/tangent presence.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetMorphTargetDataPaths, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto runMorph = [&](bool withTargetNormals, bool withTargetTangents) {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.morphTargetCount = 1U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        sm.tangents = true;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(
            0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
        mb->SetIndexData(0U, FillData(t.indices16));

        vector<Math::Vec3> tgtPos{{0.5f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.5f}};
        vector<Math::Vec3> tgtNor{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        vector<Math::Vec3> tgtTan{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
        mb->SetMorphTargetData(0U,
            FillData(t.positions),
            FillData(t.normals),
            FillData(t.tangents),
            FillData(tgtPos),
            withTargetNormals ? FillData(tgtNor) : IMeshBuilder::DataBuffer{},
            withTargetTangents ? FillData(tgtTan) : IMeshBuilder::DataBuffer{});
        EXPECT_LT(0U, mb->GetMorphTargetData().size());
    };

    runMorph(false, false);
    runMorph(true, false);
    runMorph(false, true);
    runMorph(true, true);
}

/**
 * @tc.name: SetMorphTargetDataNoMorph
 * @tc.desc: SetMorphTargetData early-returns when morphTargetCount is 0.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetMorphTargetDataNoMorph, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();
    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});

    meshBuilder->SetMorphTargetData(0U, FillData(t.positions), {}, {}, FillData(t.positions), {}, {});
    EXPECT_EQ(0U, meshBuilder->GetMorphTargetData().size());
}

/**
 * @tc.name: CreateGpuResourcesBothOverloads
 * @tc.desc: Both CreateGpuResources overloads complete.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, CreateGpuResourcesBothOverloads, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
        mb->SetIndexData(0U, FillData(t.indices16));
        mb->CreateGpuResources();
        EXPECT_LT(0U, mb->GetVertexCount());
    }
    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
        mb->SetIndexData(0U, FillData(t.indices16));
        IMeshBuilder::GpuBufferCreateInfo info{};
        info.usage = 0U;
        mb->CreateGpuResources(info);
        EXPECT_LT(0U, mb->GetVertexCount());
    }
}

/**
 * @tc.name: CreateGpuResourcesNoStaging
 * @tc.desc: NO_STAGING_BUFFER flag path through Allocate + CreateGpuResources.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, CreateGpuResourcesNoStaging, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    IMeshBuilder::Configuration cfg{vid, 1U, IMeshBuilder::ConfigurationFlagBits::NO_STAGING_BUFFER};
    mb->Initialize(cfg);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    mb->SetIndexData(0U, FillData(t.indices16));
    mb->CreateGpuResources();
    EXPECT_LT(0U, mb->GetVertexCount());
}

/**
 * @tc.name: CreateMeshOverloads
 * @tc.desc: Both CreateMesh overloads create a mesh entity.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, CreateMeshOverloads, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    mb->SetIndexData(0U, FillData(t.indices16));

    Entity meshEntity = mb->CreateMesh(*ecs);
    EXPECT_TRUE(EntityUtil::IsValid(meshEntity));

    // Second overload — append to existing entity.
    Entity preExisting = ecs->GetEntityManager().Create();
    Entity reused = mb->CreateMesh(*ecs, preExisting);
    EXPECT_EQ(preExisting, reused);
}

/**
 * @tc.name: CreateMeshEmpty
 * @tc.desc: CreateMesh on a freshly-initialized builder returns an invalid entity.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, CreateMeshEmpty, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);

    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    mb->Initialize(vid, 0U);
    // No submeshes / no allocate; vertexDataSize is 0.
    Entity e = mb->CreateMesh(*ecs);
    EXPECT_FALSE(EntityUtil::IsValid(e));

    Entity placeholder = ecs->GetEntityManager().Create();
    Entity e2 = mb->CreateMesh(*ecs, placeholder);
    EXPECT_FALSE(EntityUtil::IsValid(e2));
}

/**
 * @tc.name: ReinitializeResets
 * @tc.desc: Initialize called again after data was set clears prior state.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, ReinitializeResets, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();
    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    EXPECT_EQ(3U, meshBuilder->GetVertexCount());

    // Re-init: should reset counts and clear buffers.
    meshBuilder->Initialize(vid, 2U);
    EXPECT_EQ(0U, meshBuilder->GetVertexCount());
    EXPECT_EQ(0U, meshBuilder->GetIndexCount());
}

/**
 * @tc.name: TwoSubmeshes
 * @tc.desc: Multiple submeshes with different attributes exercise the per-submesh loops.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, TwoSubmeshes, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 2U);

    IMeshBuilder::Submesh sm0{};
    sm0.vertexCount = 3U;
    sm0.indexCount = 3U;
    sm0.indexType = CORE_INDEX_TYPE_UINT16;
    sm0.tangents = true;
    sm0.colors = true;
    meshBuilder->AddSubmesh(sm0);

    IMeshBuilder::Submesh sm1{};
    sm1.vertexCount = 3U;
    sm1.indexCount = 3U;
    sm1.indexType = CORE_INDEX_TYPE_UINT32;
    sm1.joints = true;
    meshBuilder->AddSubmesh(sm1);

    meshBuilder->Allocate();

    meshBuilder->SetVertexData(0U,
        FillData(t.positions),
        FillData(t.normals),
        FillData(t.uvs),
        FillData(t.uvs),
        FillData(t.tangents),
        FillData(t.colors));
    meshBuilder->SetIndexData(0U, FillData(t.indices16));

    meshBuilder->SetVertexData(1U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    meshBuilder->SetIndexData(1U, FillData(t.indices32));
    meshBuilder->SetJointData(1U, FillData(t.joints), FillData(t.weights), FillData(t.positions));

    EXPECT_EQ(6U, meshBuilder->GetVertexCount());
    EXPECT_EQ(2U, meshBuilder->GetSubmeshes().size());
}

/**
 * @tc.name: SetIndexDataNeedsCpuCopy
 * @tc.desc: Indices written before vertex data take the cpu-copy branch (needCpuCopy = true).
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, SetIndexDataNeedsCpuCopy, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();
    // SetIndexData first — hasNormals/hasTangents both false, needCpuCopy true.
    meshBuilder->SetIndexData(0U, FillData(t.indices16));
    meshBuilder->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    EXPECT_LT(0U, meshBuilder->GetIndexData().size());
}

/**
 * @tc.name: GenerateMissingNormalsUv16NoIndex
 * @tc.desc: Empty normals + empty indices + triangle list topology forces FlatNormal path during
 *           GenerateMissingAttributes (via CreateMesh).
 */
UNIT_TEST(API_UtilMeshBuilder, GenerateMissingNormalsUv16NoIndex, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 0U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.tangents = true;
    sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    mb->AddSubmesh(sm);
    mb->Allocate();
    // Empty normals (triggers FlatNormal) and empty uvs (triggers GenerateDefaultUvs)
    mb->SetVertexData(0U, FillData(t.positions), {}, {}, {}, {}, {});
    // No SetIndexData - empty indices triggers FlatNormal not SmoothNormal.
    Entity e = mb->CreateMesh(*ecs);
    EXPECT_TRUE(EntityUtil::IsValid(e));
}

/**
 * @tc.name: GenerateMissingNormalsSmoothUInt16
 * @tc.desc: Empty normals + uint16 indices triggers SmoothNormal<uint16_t> path.
 */
UNIT_TEST(API_UtilMeshBuilder, GenerateMissingNormalsSmoothUInt16, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.tangents = true;
    sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetIndexData(0U, FillData(t.indices16));
    mb->SetVertexData(0U, FillData(t.positions), {}, {}, {}, {}, {});
    Entity e = mb->CreateMesh(*ecs);
    EXPECT_TRUE(EntityUtil::IsValid(e));
}

/**
 * @tc.name: GenerateMissingNormalsSmoothUInt32
 * @tc.desc: Empty normals + uint32 indices triggers SmoothNormal<uint32_t> path.
 */
UNIT_TEST(API_UtilMeshBuilder, GenerateMissingNormalsSmoothUInt32, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT32;
    sm.tangents = true;
    sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetIndexData(0U, FillData(t.indices32));
    mb->SetVertexData(0U, FillData(t.positions), {}, {}, {}, {}, {});
    Entity e = mb->CreateMesh(*ecs);
    EXPECT_TRUE(EntityUtil::IsValid(e));
}

/**
 * @tc.name: TopologyTriangleStripFlatAndSmooth
 * @tc.desc: TRIANGLE_STRIP topology paths in FlatNormal and SmoothNormal.
 */
UNIT_TEST(API_UtilMeshBuilder, TopologyTriangleStripFlatAndSmooth, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    vector<Math::Vec3> positions{
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {2.0f, 0.0f, 0.0f}};
    vector<uint16_t> indices{0U, 1U, 2U, 3U, 4U};

    // Strip path with no indices - FlatNormal strip branch.
    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = static_cast<uint32_t>(positions.size());
        sm.indexCount = 0U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(0U, FillData(positions), {}, {}, {}, {}, {});
        Entity e = mb->CreateMesh(*ecs);
        EXPECT_TRUE(EntityUtil::IsValid(e));
    }
    // Strip path with indices - SmoothNormal strip branch.
    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = static_cast<uint32_t>(positions.size());
        sm.indexCount = static_cast<uint32_t>(indices.size());
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        sm.tangents = true;
        sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetIndexData(0U, FillData(indices));
        mb->SetVertexData(0U, FillData(positions), {}, {}, {}, {}, {});
        Entity e = mb->CreateMesh(*ecs);
        EXPECT_TRUE(EntityUtil::IsValid(e));
    }
}

/**
 * @tc.name: TopologyLineListSmooth
 * @tc.desc: LINE_LIST topology path in SmoothNormal (advance count 2U).
 */
UNIT_TEST(API_UtilMeshBuilder, TopologyLineListSmooth, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();
    vector<uint16_t> lineIdx{0U, 1U, 1U, 2U};

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = static_cast<uint32_t>(lineIdx.size());
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_LINE_LIST;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetIndexData(0U, FillData(lineIdx));
    mb->SetVertexData(0U, FillData(t.positions), {}, {}, {}, {}, {});
    Entity e = mb->CreateMesh(*ecs);
    EXPECT_TRUE(EntityUtil::IsValid(e));
}

/**
 * @tc.name: SetMorphTargetDataNonR32G32B32
 * @tc.desc: Use R16G16B16_SFLOAT for morph target positions to exercise the non-R32G32B32 branch
 *           in GatherDeltasP / GatherDeltasPN / GatherDeltasPNT.
 */
UNIT_TEST(API_UtilMeshBuilder, SetMorphTargetDataNonR32G32B32, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    // Three uint16 (half-float) components per vertex.
    struct Half3 {
        uint16_t x, y, z;
    };
    auto makeHalf = [](float v) -> uint16_t { return Math::F32ToF16(v); };
    vector<Half3> tgtPos{{makeHalf(0.5f), makeHalf(0.0f), makeHalf(0.0f)},
        {makeHalf(0.0f), makeHalf(0.5f), makeHalf(0.0f)},
        {makeHalf(0.0f), makeHalf(0.0f), makeHalf(0.5f)}};
    vector<Half3> tgtNor{{makeHalf(0.0f), makeHalf(1.0f), makeHalf(0.0f)},
        {makeHalf(0.0f), makeHalf(1.0f), makeHalf(0.0f)},
        {makeHalf(0.0f), makeHalf(1.0f), makeHalf(0.0f)}};
    vector<Half3> tgtTan{{makeHalf(1.0f), makeHalf(0.0f), makeHalf(0.0f)},
        {makeHalf(1.0f), makeHalf(0.0f), makeHalf(0.0f)},
        {makeHalf(1.0f), makeHalf(0.0f), makeHalf(0.0f)}};

    auto makeDataBuffer = [&](const vector<Half3>& c) {
        return IMeshBuilder::DataBuffer{BASE_FORMAT_R16G16B16_SFLOAT,
            sizeof(Half3),
            {reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(Half3)}};
    };

    // P-only path (non-R32G32B32).
    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.morphTargetCount = 1U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        sm.tangents = true;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(
            0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
        mb->SetIndexData(0U, FillData(t.indices16));
        mb->SetMorphTargetData(
            0U, FillData(t.positions), FillData(t.normals), FillData(t.tangents), makeDataBuffer(tgtPos), {}, {});
        EXPECT_LT(0U, mb->GetMorphTargetData().size());
    }
    // PN path (non-R32G32B32).
    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.morphTargetCount = 1U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        sm.tangents = true;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(
            0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
        mb->SetIndexData(0U, FillData(t.indices16));
        mb->SetMorphTargetData(0U,
            FillData(t.positions),
            FillData(t.normals),
            FillData(t.tangents),
            makeDataBuffer(tgtPos),
            makeDataBuffer(tgtNor),
            {});
        EXPECT_LT(0U, mb->GetMorphTargetData().size());
    }
    // PNT path (non-R32G32B32).
    {
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.morphTargetCount = 1U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        sm.tangents = true;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetVertexData(
            0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
        mb->SetIndexData(0U, FillData(t.indices16));
        mb->SetMorphTargetData(0U,
            FillData(t.positions),
            FillData(t.normals),
            FillData(t.tangents),
            makeDataBuffer(tgtPos),
            makeDataBuffer(tgtNor),
            makeDataBuffer(tgtTan));
        EXPECT_LT(0U, mb->GetMorphTargetData().size());
    }
}

/**
 * @tc.name: SetMorphTargetDataInvalidFormat
 * @tc.desc: Invalid (undefined) target position format -> GatherDeltasP* early-return.
 */
UNIT_TEST(API_UtilMeshBuilder, SetMorphTargetDataInvalidFormat, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.morphTargetCount = 1U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.tangents = true;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
    mb->SetIndexData(0U, FillData(t.indices16));

    uint8_t bogus[36] = {};
    IMeshBuilder::DataBuffer bad{BASE_FORMAT_UNDEFINED, 12U, {bogus, sizeof(bogus)}};
    // Bad position -> early return inside GatherDeltasP.
    mb->SetMorphTargetData(0U, FillData(t.positions), {}, {}, bad, {}, {});
    // Bad normal -> early return inside GatherDeltasPN.
    mb->SetMorphTargetData(0U, FillData(t.positions), FillData(t.normals), {}, FillData(t.positions), bad, {});
    // Bad tangent -> early return inside GatherDeltasPNT.
    mb->SetMorphTargetData(0U,
        FillData(t.positions),
        FillData(t.normals),
        FillData(t.tangents),
        FillData(t.positions),
        FillData(t.normals),
        bad);
    EXPECT_EQ(1U, mb->GetSubmeshes().size());
}

/**
 * @tc.name: CalculateAABBVariousFormats
 * @tc.desc: Exercise multiple format-specific paths in CalculateAABB (R16G16_UNORM, R8G8B8_SNORM,
 *           R16G16B16_UINT, R16G16B16A16_UINT) and the default fallback for an unhandled format.
 */
UNIT_TEST(API_UtilMeshBuilder, CalculateAABBVariousFormats, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 0U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    // R16G16_UNORM (4 bytes per vertex).
    uint16_t posU16x2[6] = {0U, 0U, 1000U, 2000U, 3000U, 4000U};
    IMeshBuilder::DataBuffer b1{
        BASE_FORMAT_R16G16_UNORM, 4U, {reinterpret_cast<const uint8_t*>(posU16x2), sizeof(posU16x2)}};
    meshBuilder->CalculateAABB(0U, b1);

    // R8G8B8_SNORM (3 bytes per vertex).
    int8_t posI8x3[9] = {-64, -32, 0, 0, 0, 0, 64, 32, 16};
    IMeshBuilder::DataBuffer b2{
        BASE_FORMAT_R8G8B8_SNORM, 3U, {reinterpret_cast<const uint8_t*>(posI8x3), sizeof(posI8x3)}};
    meshBuilder->CalculateAABB(0U, b2);

    // R16G16B16_UINT (6 bytes per vertex).
    uint16_t posU16x3[9] = {0U, 0U, 0U, 100U, 200U, 300U, 50U, 150U, 250U};
    IMeshBuilder::DataBuffer b3{
        BASE_FORMAT_R16G16B16_UINT, 6U, {reinterpret_cast<const uint8_t*>(posU16x3), sizeof(posU16x3)}};
    meshBuilder->CalculateAABB(0U, b3);

    // R16G16B16A16_UINT (8 bytes per vertex).
    uint16_t posU16x4[12] = {0U, 0U, 0U, 1U, 100U, 200U, 300U, 1U, 50U, 150U, 250U, 1U};
    IMeshBuilder::DataBuffer b4{
        BASE_FORMAT_R16G16B16A16_UINT, 8U, {reinterpret_cast<const uint8_t*>(posU16x4), sizeof(posU16x4)}};
    meshBuilder->CalculateAABB(0U, b4);

    // Format not handled by switch (e.g. R8_UNORM) -> default branch.
    uint8_t pos8[3] = {10U, 20U, 30U};
    IMeshBuilder::DataBuffer b5{BASE_FORMAT_R8_UNORM, 1U, {pos8, sizeof(pos8)}};
    meshBuilder->CalculateAABB(0U, b5);

    EXPECT_EQ(1U, meshBuilder->GetSubmeshes().size());
}

/**
 * @tc.name: CalculateAABBStrideTooSmall
 * @tc.desc: dataBuffer.stride > buffer.size()/vertexCount -> Verify-style early return inside
 *           CalculateAABB (posElementSize > stride).
 */
UNIT_TEST(API_UtilMeshBuilder, CalculateAABBStrideTooSmall, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 0U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);
    meshBuilder->Allocate();

    // Vec3 format (componentCount * componentByteSize = 12) with stride=4 -> elementSize > stride
    uint8_t pos[12] = {};
    IMeshBuilder::DataBuffer bad{BASE_FORMAT_R32G32B32_SFLOAT, 4U, {pos, sizeof(pos)}};
    meshBuilder->CalculateAABB(0U, bad);
    EXPECT_EQ(1U, meshBuilder->GetSubmeshes().size());
}

/**
 * @tc.name: SetJointDataInvalidFormat
 * @tc.desc: Invalid joint/weight/position formats in CalculateJointBounds early-return paths.
 */
UNIT_TEST(API_UtilMeshBuilder, SetJointDataInvalidFormat, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.joints = true;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    mb->SetIndexData(0U, FillData(t.indices16));

    uint8_t bogus[64] = {};
    IMeshBuilder::DataBuffer bad{BASE_FORMAT_UNDEFINED, 16U, {bogus, sizeof(bogus)}};
    // Bad joint format -> early return on jointFormat check
    mb->SetJointData(0U, bad, FillData(t.weights), FillData(t.positions));
    // Bad weight format -> early return on weightFormat check
    mb->SetJointData(0U, FillData(t.joints), bad, FillData(t.positions));
    // Bad position format -> early return on positionFormat check
    mb->SetJointData(0U, FillData(t.joints), FillData(t.weights), bad);
    EXPECT_EQ(1U, mb->GetSubmeshes().size());
}

/**
 * @tc.name: CreateMeshWithExistingMeshComponent
 * @tc.desc: CreateMesh(ecs, existingEntity) with a pre-existing MeshComponent triggers the
 *           append-to-existing path.
 */
UNIT_TEST(API_UtilMeshBuilder, CreateMeshWithExistingMeshComponent, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    mb->SetIndexData(0U, FillData(t.indices16));

    Entity first = mb->CreateMesh(*ecs);
    EXPECT_TRUE(EntityUtil::IsValid(first));

    // Build again and pass the existing entity -> append branch.
    auto mb2 = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb2->Initialize(vid, 1U);
    mb2->AddSubmesh(sm);
    mb2->Allocate();
    mb2->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    mb2->SetIndexData(0U, FillData(t.indices16));
    Entity reused = mb2->CreateMesh(*ecs, first);
    EXPECT_EQ(first, reused);
}

/**
 * @tc.name: SetMorphTargetDataAfterIndexAndZeroDelta
 * @tc.desc: Use zero deltas in target positions so the zero-skip branch in GatherDeltasR32G32B32 hits.
 */
UNIT_TEST(API_UtilMeshBuilder, SetMorphTargetDataZeroDeltas, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();
    // Mix of zero and non-zero deltas.
    vector<Math::Vec3> tgtPos{{0.0f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    vector<Math::Vec3> tgtNor{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    vector<Math::Vec3> tgtTan{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.morphTargetCount = 1U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.tangents = true;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
    mb->SetIndexData(0U, FillData(t.indices16));
    // PNT path with zero deltas
    mb->SetMorphTargetData(0U,
        FillData(t.positions),
        FillData(t.normals),
        FillData(t.tangents),
        FillData(tgtPos),
        FillData(tgtNor),
        FillData(tgtTan));
    EXPECT_LT(0U, mb->GetMorphTargetData().size());
}

/**
 * @tc.name: AllocateOversizedMorph
 * @tc.desc: Force overflow guard in CalculateSizes (morphTargetCount * vertexCount overflow).
 */
UNIT_TEST(API_UtilMeshBuilder, AllocateOversizedMorph, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 100000U;
    sm.indexCount = 3U;
    sm.morphTargetCount = std::numeric_limits<uint32_t>::max();
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    mb->AddSubmesh(sm);
    mb->Allocate();
    // Overflow path should reset morphTargetCount to 0.
    EXPECT_EQ(0U, mb->GetSubmesh(0U).morphTargetCount);
}

/**
 * @tc.name: SetMorphTargetDataOversized
 * @tc.desc: Trigger overflow guard inside SetMorphTargetData by forcing vc>(u32max/targetElem/(mc+1u)).
 */
UNIT_TEST(API_UtilMeshBuilder, SetMorphTargetDataOversized, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    // We can't actually allocate huge buffers, so we set up a normal mesh then call
    // SetMorphTargetData with a forged DataBuffer of crafted format - but it's simpler to
    // exercise the early return when buffer ptr is null by calling SetMorphTargetData on a
    // builder that was never Allocate()d (buffer is null) -> if (!buffer) return.
    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.morphTargetCount = 1U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    mb->AddSubmesh(sm);
    // No Allocate(): buffer is null.
    mb->SetMorphTargetData(0U, FillData(t.positions), {}, {}, FillData(t.positions), {}, {});
    EXPECT_EQ(0U, mb->GetMorphTargetData().size());
}

/**
 * @tc.name: SetIndexBeforeAllocate
 * @tc.desc: SetIndexData / SetVertexData / SetJointData / SetMorphTargetData when buffer is null
 *           (no Allocate) take the buffer-null early-return branches.
 */
UNIT_TEST(API_UtilMeshBuilder, SetOperationsNoBuffer, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.joints = true;
    mb->AddSubmesh(sm);
    // Note: no Allocate().
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, {}, {});
    mb->SetIndexData(0U, FillData(t.indices16));
    mb->SetJointData(0U, FillData(t.joints), FillData(t.weights), FillData(t.positions));
    EXPECT_EQ(0U, mb->GetVertexCount());
}

/**
 * @tc.name: PrimitiveRestartSentinelIndexNoOob
 * @tc.desc: A primitive-restart sentinel index (0xFFFF / 0xFFFFFFFF) reaching CPU normal generation must
 *           be bounds-checked against vertexCount and skipped, not used to index posPtr / norPtr.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, PrimitiveRestartSentinelIndexNoOob, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();  // 3 vertices

    // uint16 sentinel 0xFFFF as a triangle index; with vertexCount 3 it is far out of range.
    {
        const vector<uint16_t> idx{0xFFFFU, 1U, 2U};
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.indexType = CORE_INDEX_TYPE_UINT16;
        sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetIndexData(0U, FillData(idx));
        // Empty normals -> GenerateMissingAttributes -> SmoothNormal<uint16_t> over the sentinel index.
        mb->SetVertexData(0U, FillData(t.positions), {}, {}, {}, {}, {});
        const Entity e = mb->CreateMesh(*ecs);  // Must not read posPtr[0xFFFF] / norPtr[0xFFFF].
        EXPECT_TRUE(EntityUtil::IsValid(e));
    }
    // uint32 sentinel 0xFFFFFFFF.
    {
        const vector<uint32_t> idx{0xFFFFFFFFU, 1U, 2U};
        auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
        mb->Initialize(vid, 1U);
        IMeshBuilder::Submesh sm{};
        sm.vertexCount = 3U;
        sm.indexCount = 3U;
        sm.indexType = CORE_INDEX_TYPE_UINT32;
        sm.inputAssembly.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        mb->AddSubmesh(sm);
        mb->Allocate();
        mb->SetIndexData(0U, FillData(idx));
        mb->SetVertexData(0U, FillData(t.positions), {}, {}, {}, {}, {});
        const Entity e = mb->CreateMesh(*ecs);
        EXPECT_TRUE(EntityUtil::IsValid(e));
    }
}

/**
 * @tc.name: MorphTargetMissingPositionNoOob
 * @tc.desc: morphTargetCount can exceed the number of targets that carry POSITION, so targetPositions
 *           holds fewer than morphTargetCount * vertexCount elements. The float (R32G32B32) gather path
 *           must stop at the end of the supplied data and not underflow the morph buffer size. Three
 *           targets are declared but only one target's worth of float position deltas is provided.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, MorphTargetMissingPositionNoOob, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();  // vertexCount 3

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.morphTargetCount = 3U;  // Declares three targets...
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.tangents = true;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
    mb->SetIndexData(0U, FillData(t.indices16));

    // ...but only one target's worth (3 vertices) of position deltas is supplied.
    const vector<Math::Vec3> tgtPos{{0.5f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.5f}};
    mb->SetMorphTargetData(
        0U, FillData(t.positions), FillData(t.normals), FillData(t.tangents), FillData(tgtPos), {}, {});

    // The morph staging region for a 3-vertex, 3-target submesh is tiny; an underflowed size would
    // balloon it past this bound.
    EXPECT_LT(mb->GetMorphTargetData().size(), 0x100000U);
}

/**
 * @tc.name: MorphTargetMissingPositionNonFloatNoOob
 * @tc.desc: The non-R32G32B32 (generic) gather path. With half-float (R16G16B16) morph positions and
 *           morphTargetCount greater than the supplied target count, the generic gather loop must
 *           bounds-check each target against the buffer instead of reading morphTargetCount *
 *           vertexCount elements past the end. Best run under ASan, which traps the read directly.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilMeshBuilder, MorphTargetMissingPositionNonFloatNoOob, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    const auto vid = GetForwardVertexInputDeclaration(*testContext->renderContext);
    const auto t = MakeTriangle();  // vertexCount 3

    struct Half3 {
        uint16_t x, y, z;
    };
    auto makeHalf = [](float v) -> uint16_t { return Math::F32ToF16(v); };
    // Exactly one target's worth (3 vertices) of half-float position deltas.
    const vector<Half3> tgtPos{{makeHalf(0.5f), makeHalf(0.0f), makeHalf(0.0f)},
        {makeHalf(0.0f), makeHalf(0.5f), makeHalf(0.0f)},
        {makeHalf(0.0f), makeHalf(0.0f), makeHalf(0.5f)}};
    const IMeshBuilder::DataBuffer tgtPosBuffer{BASE_FORMAT_R16G16B16_SFLOAT,
        sizeof(Half3),
        {reinterpret_cast<const uint8_t*>(tgtPos.data()), tgtPos.size() * sizeof(Half3)}};

    auto mb = CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    mb->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.morphTargetCount = 3U;  // Three targets declared, one target of data supplied.
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    sm.tangents = true;
    mb->AddSubmesh(sm);
    mb->Allocate();
    mb->SetVertexData(0U, FillData(t.positions), FillData(t.normals), FillData(t.uvs), {}, FillData(t.tangents), {});
    mb->SetIndexData(0U, FillData(t.indices16));
    mb->SetMorphTargetData(0U, FillData(t.positions), FillData(t.normals), FillData(t.tangents), tgtPosBuffer, {}, {});

    EXPECT_LT(mb->GetMorphTargetData().size(), 0x100000U);
}
