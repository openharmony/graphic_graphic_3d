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

#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginCameraNodeTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        auto scene = CreateEmptyScene();
        auto node = scene->CreateNode("//test", ClassId::CameraNode).GetResult();
        ASSERT_TRUE(node);
        node->Position()->SetValue({0, 0, 0});
        camera_ = interface_pointer_cast<ICamera>(node);
        ASSERT_TRUE(camera_);
        UpdateScene();
    }

    void TearDown() override
    {
        camera_.reset();
    }

protected:
    ICamera::Ptr camera_;
};

/**
 * @tc.name: ScreenAndWorldPosition
 * @tc.desc: Tests for Screen And World Position. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCameraNodeTest, ScreenAndWorldPosition, testing::ext::TestSize.Level1)
{
    auto rc = interface_cast<ICameraRayCast>(camera_);
    ASSERT_TRUE(rc);
    {
        auto res = rc->ScreenPositionToWorld({0.5, 0.5, 0.0}).GetResult();
        EXPECT_EQ(res, (BASE_NS::Math::Vec3{0, 0, -camera_->NearPlane()->GetValue()}));
    }
    {
        auto res = rc->WorldPositionToScreen({0, 0, -camera_->NearPlane()->GetValue()}).GetResult();
        EXPECT_EQ(res, (BASE_NS::Math::Vec3{0.5, 0.5, 0.0}));
    }
}

/**
 * @tc.name: CameraRayCast
 * @tc.desc: Tests for Camera Ray Cast. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCameraNodeTest, CameraRayCast, testing::ext::TestSize.Level1)
{
    auto rc = interface_cast<ICameraRayCast>(camera_);
    ASSERT_TRUE(rc);

    auto node = scene->CreateNode("//node", ClassId::MeshNode).GetResult();
    ASSERT_TRUE(node);
    node->Position()->SetValue({0, 0, -10});
    auto mesh = interface_pointer_cast<IMesh>(node);
    ASSERT_TRUE(mesh);
    AddSubMesh(mesh);
    auto submesh = mesh->SubMeshes()->GetValueAt(0);
    ASSERT_TRUE(submesh);
    submesh->AABBMin()->SetValue({-1, -1, -1});
    submesh->AABBMax()->SetValue({1, 1, 1});
    UpdateScene();
    EXPECT_EQ(mesh->AABBMin()->GetValue(), (BASE_NS::Math::Vec3{-1, -1, -1}));
    EXPECT_EQ(mesh->AABBMax()->GetValue(), (BASE_NS::Math::Vec3{1, 1, 1}));

    auto res = rc->CastRay({0.5, 0.5}, {}).GetResult();
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].position, (BASE_NS::Math::Vec3{0, 0, -9}));
    EXPECT_EQ(res[0].distance, 9);
    EXPECT_EQ(res[0].distanceToCenter, 10);
    EXPECT_EQ(res[0].node, node);
}

#define EXPECT_NEAR_MAT4(m1, m2, eps)                 \
    for (size_t ii = 0; ii != 4; ii++) {              \
        for (size_t jj = 0; jj != 4; jj++) {          \
            EXPECT_NEAR(m1[ii][jj], m2[ii][jj], eps); \
        }                                             \
    }

/**
 * @tc.name: CameraMatrixAccessor
 * @tc.desc: Tests camera matrix accessor functionality with known camera values
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCameraNodeTest, CameraMatrixAccessor, testing::ext::TestSize.Level1)
{
    UpdateScene();

    BASE_NS::Math::Mat4X4 expectedView{0.9808309078216553f,
        -0.005208088085055351f,
        0.19479140639305115f,
        0.0f,
        0.0f,
        0.9996427893638611,
        0.02672719396650791f,
        0.0f,
        -0.19486099481582642f,
        -0.026214856654405594,
        0.9804804921150208f,
        0.0f,
        -3.345266580581665f,
        2.2350447177886963f,
        -8.764347076416016f,
        1.0f};

    BASE_NS::Math::Mat4X4 expectedProj{1.732050895690918f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        -1.732050895690918f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        -1.0003000497817993f,
        -1.0f,
        0.0f,
        0.0f,
        -0.3000900149345398f,
        0.0f};

    auto matrixAccessor = interface_cast<ICameraMatrixAccessor>(camera_);
    ASSERT_TRUE(matrixAccessor);

    auto cameraNode = interface_cast<INode>(camera_);

    META_NS::SetValue(cameraNode->Position(), BASE_NS::Math::Vec3(5.0f, -2.0f, 8.0f));
    META_NS::SetValue(cameraNode->Rotation(),
        BASE_NS::Math::Quat(-0.01330058928579092, 0.09789206087589264, 0.0013084238162264228, 0.9951072931289673));

    auto viewMatrix = matrixAccessor->GetViewMatrix();
    auto projMatrix = matrixAccessor->GetProjectionMatrix();

    auto printMatrix = [](const BASE_NS::Math::Mat4X4& m) {
        BASE_NS::string matrix;
        for (int i = 0; i < 4; ++i) {
            std::string s = "[";
            for (int j = 0; j < 4; ++j) {
                s += std::to_string(m[j][i]);
                if (j < 3) {
                    s += " ";
                }
            }
            matrix.append(s.c_str()).append("\n");
        }
        return matrix;
    };

    for (int i = 0; i < 16; ++i) {
        EXPECT_NEAR(viewMatrix.data[i], expectedView.data[i], 0.0001f)
            << "i:" << i << "\nview matrix:\n"
            << printMatrix(viewMatrix).c_str() << "\nexpected:\n"
            << printMatrix(expectedView).c_str();
    }

    for (int i = 0; i < 16; ++i) {
        EXPECT_NEAR(projMatrix.data[i], expectedProj.data[i], 0.0001f)
            << "i:" << i << "\nprojection matrix:\n"
            << printMatrix(projMatrix).c_str() << "\nexpected:\n"
            << printMatrix(expectedProj).c_str();
    }

    META_NS::SetValue(camera_->Projection(), CameraProjection::PERSPECTIVE);
    EXPECT_NEAR_MAT4(viewMatrix, matrixAccessor->GetViewMatrix(), 0.0001f);
    EXPECT_NEAR_MAT4(projMatrix, matrixAccessor->GetProjectionMatrix(), 0.0001f);

    META_NS::SetValue(camera_->Projection(), CameraProjection::ORTHOGRAPHIC);
    EXPECT_NEAR_MAT4(viewMatrix, matrixAccessor->GetViewMatrix(), 0.0001f);
    EXPECT_NE(projMatrix, matrixAccessor->GetProjectionMatrix());

    META_NS::SetValue(camera_->Projection(), CameraProjection::FRUSTUM);
    EXPECT_NEAR_MAT4(viewMatrix, matrixAccessor->GetViewMatrix(), 0.0001f);
    EXPECT_NEAR_MAT4(projMatrix, matrixAccessor->GetProjectionMatrix(), 0.0001f);

    META_NS::SetValue(camera_->Projection(), CameraProjection::CUSTOM);
    EXPECT_NEAR_MAT4(viewMatrix, matrixAccessor->GetViewMatrix(), 0.0001f);
    EXPECT_NE(projMatrix, matrixAccessor->GetProjectionMatrix());
}

/**
 * @tc.name: CameraRenderTargetReassign
 * @tc.desc: Tests camera render target assignment, reassignment, and clearing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCameraNodeTest, CameraRenderTargetReassign, testing::ext::TestSize.Level1)
{
    // Set render target to a bitmap
    IImage::Ptr bmap = CreateTestBitmap();
    ASSERT_TRUE(bmap);
    EXPECT_TRUE(camera_->SetRenderTarget(interface_pointer_cast<IRenderTarget>(bmap)).GetResult());
    UpdateScene();

    // Create a second bitmap with different size and reassign
    IImage::Ptr bmap2 = CreateTestBitmap();
    ASSERT_TRUE(bmap2);
    EXPECT_TRUE(camera_->SetRenderTarget(interface_pointer_cast<IRenderTarget>(bmap2)).GetResult());
    UpdateScene();

    // Clear render target by setting to null
    EXPECT_TRUE(camera_->SetRenderTarget(nullptr).GetResult());
    UpdateScene();
}

}  // namespace UTest

SCENE_END_NAMESPACE()
