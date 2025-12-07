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

#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/camera_component.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/api/make_callback.h>
// #include <meta/api/property/property_info.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

#include "scene/scene_component_test.h"
#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginCameraComponentTest : public ScenePluginComponentTest<CORE3D_NS::ICameraComponentManager> {
public:
    template<class T>
    void TestICameraPropertyGetters(T* camera)
    {
        EXPECT_TRUE(camera->IsActive());

        EXPECT_TRUE(camera->FoV());
        EXPECT_TRUE(camera->AspectRatio());
        EXPECT_TRUE(camera->NearPlane());
        EXPECT_TRUE(camera->FarPlane());
        EXPECT_TRUE(camera->XMagnification());
        EXPECT_TRUE(camera->YMagnification());
        EXPECT_TRUE(camera->XOffset());
        EXPECT_TRUE(camera->YOffset());
        EXPECT_TRUE(camera->Projection());
        EXPECT_TRUE(camera->Culling());
        EXPECT_TRUE(camera->RenderingPipeline());
        EXPECT_TRUE(camera->SceneFlags());
        EXPECT_TRUE(camera->PipelineFlags());
        EXPECT_TRUE(camera->Viewport());
        EXPECT_TRUE(camera->Scissor());
        EXPECT_TRUE(camera->RenderTargetSize());
        EXPECT_TRUE(camera->ClearColor());
        EXPECT_TRUE(camera->ClearDepth());
        EXPECT_TRUE(camera->CameraLayerMask());
        EXPECT_TRUE(camera->MSAASampleCount());
        EXPECT_TRUE(camera->ColorTargetCustomization());
        EXPECT_TRUE(camera->CustomProjectionMatrix());
        EXPECT_TRUE(camera->PostProcess());
        EXPECT_TRUE(camera->DownsamplePercentage());
    }

    void TestICameraFunctions(ICamera* camera)
    {
        ASSERT_TRUE(camera);

        TestICameraPropertyGetters<ICamera>(camera);
        TestICameraPropertyGetters<const ICamera>(camera);
        TestICameraPropertyGetters<ICamera>(camera);
        TestICameraPropertyGetters<const ICamera>(camera);
        TestInvalidComponentPropertyInit(camera);

        EXPECT_TRUE(camera->IsActive());
        EXPECT_TRUE(camera->SetActive(false).GetResult());
        EXPECT_FALSE(camera->IsActive());
        EXPECT_TRUE(camera->SetActive(true).GetResult());
        EXPECT_TRUE(camera->IsActive());

        IImage::Ptr bmap = CreateTestBitmap();
        EXPECT_TRUE(camera->SetRenderTarget(interface_pointer_cast<IRenderTarget>(bmap)).GetResult());

        size_t count = 0;
        auto dr = interface_cast<META_NS::IDynamicResource>(bmap);
        ASSERT_TRUE(dr);
        dr->OnResourceChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&] { ++count; }));

        UpdateComponentMembers();

        EXPECT_EQ(count, 1);

        // fix?
        // auto entity = interface_cast<IEcsResource>(bmap)->GetEntity();
        ASSERT_EQ(nativeComponent.customColorTargets.size(), 1);
        // EXPECT_EQ(nativeComponent.customColorTargets[0], entity);
        EXPECT_EQ(nativeComponent.renderResolution[0], bmap->Size()->GetValue().x);
        EXPECT_EQ(nativeComponent.renderResolution[1], bmap->Size()->GetValue().y);

        UpdateScene();
        EXPECT_EQ(count, 2);

        camera->SetActive(false).Wait();
        UpdateScene();
        EXPECT_EQ(count, 2);

        camera->SetActive(true).Wait();
        UpdateScene();
        EXPECT_EQ(count, 3);

        EXPECT_TRUE(camera->SetRenderTarget(nullptr).GetResult());
        UpdateScene();
        EXPECT_EQ(count, 3);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCameraComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::CameraNode).GetResult();
    SetComponent(node, "CameraComponent");

    TestEngineProperty<float>("FoV", 0.7, nativeComponent.yFov);
    TestEngineProperty<float>("AspectRatio", 0.7, nativeComponent.aspect);
    TestEngineProperty<float>("NearPlane", 0.7, nativeComponent.zNear);
    TestEngineProperty<float>("FarPlane", 0.7, nativeComponent.zFar);
    TestEngineProperty<float>("XMagnification", 0.7, nativeComponent.xMag);
    TestEngineProperty<float>("YMagnification", 0.7, nativeComponent.yMag);
    TestEngineProperty<float>("XOffset", 0.7, nativeComponent.xOffset);
    TestEngineProperty<float>("YOffset", 0.7, nativeComponent.yOffset);

    TestEngineProperty<CameraProjection>("Projection", CameraProjection::ORTHOGRAPHIC, nativeComponent.projection);
    TestEngineProperty<CameraCulling>("Culling", CameraCulling::NONE, nativeComponent.culling);
    TestEngineProperty<CameraPipeline>(
        "RenderingPipeline", CameraPipeline::LIGHT_FORWARD, nativeComponent.renderingPipeline);
    TestEngineProperty<CameraSampleCount>(
        "MSAASampleCount", CameraSampleCount::COUNT_2, nativeComponent.msaaSampleCount);
    TestEngineProperty<uint32_t>("SceneFlags", 12345, nativeComponent.sceneFlags);
    TestEngineProperty<uint32_t>("PipelineFlags", 2, nativeComponent.pipelineFlags);
    TestEngineProperty<BASE_NS::Math::Vec4>("Viewport", { 0.7, 1.0, 2.0, 0.5 }, nativeComponent.viewport);
    TestEngineProperty<BASE_NS::Math::Vec4>("Scissor", { 0.7, 1.0, 2.0, 0.5 }, nativeComponent.scissor);
    TestEngineProperty<BASE_NS::Math::UVec2>("RenderTargetSize", { 12345, 12345 }, nativeComponent.renderResolution);
    TestEngineProperty<BASE_NS::Math::Vec4>("ClearColor", { 0.5, 0.5, 0.5, 0 }, nativeComponent.clearColorValue);
    TestEngineProperty<float>("ClearDepth", 0.7, nativeComponent.clearDepthValue);
    TestEngineProperty<float>("DownsamplePercentage", 0.7, nativeComponent.screenPercentage);
    TestEngineProperty<uint64_t>("CameraLayerMask", 4, nativeComponent.layerMask);

    TestEngineProperty<BASE_NS::Math::Mat4X4>(
        "CustomProjectionMatrix", BASE_NS::Math::Mat4X4 { 2.0 }, nativeComponent.customProjectionMatrix);

    {
        auto pp = scene->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult();
        ASSERT_TRUE(pp);

        auto p = GetProperty<IPostProcess::Ptr>("PostProcess");
        p->SetValue(pp);
        UpdateComponentMembers();

        EXPECT_EQ(p->GetValue(), pp);
        EXPECT_EQ(nativeComponent.postProcess, interface_cast<IEcsObjectAccess>(pp)->GetEcsObject()->GetEntity());
    }
    {
        auto p = GetArrayProperty<ColorFormat>("ColorTargetCustomization");
        BASE_NS::vector<ColorFormat> value { { BASE_NS::BASE_FORMAT_R8_UNORM }, { BASE_NS::BASE_FORMAT_R8_UINT } };
        p->SetValue(value);
        UpdateComponentMembers();
        auto res = p->GetValue();
        ASSERT_EQ(res.size(), 2);
        EXPECT_EQ(res[0].format, BASE_NS::BASE_FORMAT_R8_UNORM);
        EXPECT_EQ(res[1].format, BASE_NS::BASE_FORMAT_R8_UINT);
        ASSERT_EQ(nativeComponent.colorTargetCustomization.size(), 2);
        EXPECT_EQ(nativeComponent.colorTargetCustomization[0].format, BASE_NS::BASE_FORMAT_R8_UNORM);
        EXPECT_EQ(nativeComponent.colorTargetCustomization[1].format, BASE_NS::BASE_FORMAT_R8_UINT);
    }
}

// waiting for LUME-7098
/**
 * @tc.name: SceneFalgs
 * @tc.desc: Tests for Scene Falgs. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST_F(ScenePluginCameraComponentTest, SceneFalgs, testing::ext::TestSize.Level1)
{
   auto scene = CreateEmptyScene();
   auto node = scene->CreateNode("//test", ClassId::CameraNode).GetResult();
   auto camera = interface_cast<ICamera>(node);
   ASSERT_TRUE(camera);

   META_NS::PropertyEnumInfo info(camera->SceneFlags());
   ASSERT_TRUE(info);
   EXPECT_EQ(info->GetEnumType(), META_NS::EnumType::BIT_FIELD);
}
*/

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCameraComponentTest, Functions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::CameraNode).GetResult();
    SetComponent(node, "CameraComponent");
    // The member "object" has a CameraComponent and node has CameraNode.
    TestICameraFunctions(interface_cast<ICamera>(node));
    TestICameraFunctions(interface_cast<ICamera>(object));
}

/**
 * @tc.name: IsActive
 * @tc.desc: Tests for Is Active. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCameraComponentTest, IsActive, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    {
        auto node = scene->CreateNode("//test", ClassId::CameraNode).GetResult();
        auto camera = interface_cast<ICamera>(node);
        ASSERT_TRUE(camera);
        EXPECT_TRUE(camera->SetActive(false).GetResult());
        EXPECT_FALSE(camera->IsActive());

        EXPECT_TRUE(scene->ReleaseNode(BASE_NS::move(node), false).GetResult());
    }
    {
        auto node = scene->FindNode("//test", ClassId::CameraNode).GetResult();
        auto camera = interface_cast<ICamera>(node);
        ASSERT_TRUE(camera);
        EXPECT_FALSE(camera->IsActive());
        EXPECT_TRUE(camera->SetActive(true).GetResult());
        EXPECT_TRUE(camera->IsActive());
    }
}

} // namespace UTest

SCENE_END_NAMESPACE()
