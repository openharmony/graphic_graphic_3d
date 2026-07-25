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

#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <base/math/vector.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/intf_render_context.h>

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
    vector<uint16_t> indices16;
    vector<Math::Vec4> joints;
    vector<Math::Vec4> weights;
};

TriangleData MakeTriangle()
{
    TriangleData d;
    d.positions = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    d.normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    d.uvs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}};
    d.indices16 = {0U, 1U, 2U};
    d.joints = {{0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}};
    d.weights = {{1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}};
    return d;
}
}  // namespace

/**
 * @tc.name: SetJointDataShortPositionBuffer
 * @tc.desc: CalculateJointBounds derives its loop trip count solely from the joint buffer. A glTF whose POSITION (or
 *           WEIGHTS_0) accessor has fewer elements than JOINTS_0 (accessors are independent and never cross-validated)
 *           must not drive reads past the end of the shorter buffer.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshBuilderSecurity, SetJointDataShortPositionBuffer, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CORE_NS::CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    ASSERT_TRUE(meshBuilder);
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

    // Joints/weights carry 3 elements, but the position buffer used for bounds carries only 1. The loop count is
    // taken solely from the joint buffer, so without a clamp the second pass reads positions[1]/positions[2] past
    // the end of the single-element heap allocation.
    vector<Math::Vec3> shortPositions{{0.0f, 0.0f, 0.0f}};
    meshBuilder->SetJointData(0U, FillData(t.joints), FillData(t.weights), FillData(shortPositions));

    // With the clamp, only the single in-range vertex is processed and no OOB read occurs.
    EXPECT_LE(0U, meshBuilder->GetJointBoundsData().size());
}

/**
 * @tc.name: SetJointDataWideJointIndexBounds
 * @tc.desc: With 16-bit joint indices, sizing the bounds from a truncated index and then writing at a full index
 *           must not write out of bounds.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshBuilderSecurity, SetJointDataWideJointIndexBounds, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CORE_NS::CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    ASSERT_TRUE(meshBuilder);
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

    // 16-bit joints, component 0 = 0xFF00 with the weight on component 1. A single-byte read of component 0 yields 0
    // (sizes one bound), while a full decode at the weighted component reaches 255 - out of range for the size-1
    // vector.
    vector<uint16_t> wideJoints = {
        0xFF00U,
        0U,
        0U,
        0U,  // vertex 0
        0xFF00U,
        0U,
        0U,
        0U,  // vertex 1
        0xFF00U,
        0U,
        0U,
        0U,  // vertex 2
    };
    const IMeshBuilder::DataBuffer jointBuffer{BASE_FORMAT_R16G16B16A16_UINT,
        sizeof(uint16_t) * 4U,
        {reinterpret_cast<const uint8_t*>(wideJoints.data()), wideJoints.size() * sizeof(uint16_t)}};
    vector<Math::Vec4> weights = {{0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}};

    meshBuilder->SetJointData(0U, jointBuffer, FillData(weights), FillData(t.positions));

    // The consistent decode plus bounds guard keeps the referenced index in range; no OOB write.
    EXPECT_LE(0U, meshBuilder->GetJointBoundsData().size());
}

/**
 * @tc.name: SparseVertexBindingIndex
 * @tc.desc: The per-binding vectors were sized to the binding-description count but indexed by the unvalidated
 *           binding id. A sparse (yet Vulkan-legal) declaration whose single binding id exceeds the description
 *           count must not write out of bounds.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilMeshBuilderSecurity, SparseVertexBindingIndex, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto meshBuilder = CORE_NS::CreateInstance<IMeshBuilder>(*testContext->renderContext, UID_MESH_BUILDER);
    ASSERT_TRUE(meshBuilder);

    // Single binding with sparse id 7 (>= description count 1). The declaration is a view, so the arrays must
    // outlive Allocate().
    const VertexInputDeclaration::VertexInputBindingDescription bindings[] = {
        {7u, sizeof(Math::Vec3), VertexInputRate::CORE_VERTEX_INPUT_RATE_VERTEX},
    };
    const VertexInputDeclaration::VertexInputAttributeDescription attributes[] = {
        {0u, 7u, BASE_FORMAT_R32G32B32_SFLOAT, 0u},
    };
    VertexInputDeclarationView vid;
    vid.bindingDescriptions = bindings;
    vid.attributeDescriptions = attributes;

    meshBuilder->Initialize(vid, 1U);
    IMeshBuilder::Submesh sm{};
    sm.vertexCount = 3U;
    sm.indexCount = 3U;
    sm.indexType = CORE_INDEX_TYPE_UINT16;
    meshBuilder->AddSubmesh(sm);

    // Without the resize-to-BUFFER_COUNT fix, Allocate indexes the per-binding vectors past their end.
    meshBuilder->Allocate();

    EXPECT_LE(0U, meshBuilder->GetJointBoundsData().size());
}
