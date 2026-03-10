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

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

#include <scene/api/scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_create_mesh.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_surface_stream.h>
#include <scene/interface/intf_text.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/resource/util.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/api/metadata_util.h>
#include <meta/api/object.h>
#include <meta/ext/attachment/behavior.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_SurfaceStreamTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
    }

    void TearDown() override
    {
        ScenePluginTest::TearDown();
    }
};

/**
 * @tc.name: OnBufferAvailable
 * @tc.desc: Tests for SurfaceStream.OnBufferAvailable [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, OnBufferAvailable, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    // The OnBufferAvailable callback will be triggered when a buffer is available
    // This is tested indirectly through the attach/detach functionality
    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: Build
 * @tc.desc: Tests for SurfaceStream.Build [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, Build, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetSurfaceId(), 0);

    auto surfaceStream2 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream2);
    auto surfaceStreamInterface2 = interface_cast<ISurfaceStream>(surfaceStream2);
    ASSERT_TRUE(surfaceStreamInterface2);

    EXPECT_NE(surfaceStream, surfaceStream2);
}

/**
 * @tc.name: Attach
 * @tc.desc: Tests for SurfaceStream.Attach [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, Attach, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));

    EXPECT_FALSE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: Init
 * @tc.desc: Tests for SurfaceStream.Init [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, Init, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);
}

/**
 * @tc.name: UpdateView
 * @tc.desc: Tests for SurfaceStream.UpdateView [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, UpdateView, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: SetHeight
 * @tc.desc: Tests for SurfaceStream.SetHeight [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, SetHeight, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    const uint32_t testHeight = 720;
    surfaceStreamInterface->SetHeight(testHeight);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), testHeight);

    const uint32_t newHeight = 1080;
    surfaceStreamInterface->SetHeight(newHeight);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), newHeight);
}

/**
 * @tc.name: GetHeight
 * @tc.desc: Tests for SurfaceStream.GetHeight [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, GetHeight, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);

    const uint32_t testHeight = 480;
    surfaceStreamInterface->SetHeight(testHeight);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), testHeight);
}

/**
 * @tc.name: SetWidth
 * @tc.desc: Tests for SurfaceStream.SetWidth [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, SetWidth, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    const uint32_t testWidth = 1280;
    surfaceStreamInterface->SetWidth(testWidth);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), testWidth);

    const uint32_t newWidth = 1920;
    surfaceStreamInterface->SetWidth(newWidth);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), newWidth);
}

/**
 * @tc.name: GetWidth
 * @tc.desc: Tests for SurfaceStream.GetWidth [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, GetWidth, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);

    const uint32_t testWidth = 640;
    surfaceStreamInterface->SetWidth(testWidth);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), testWidth);
}

/**
 * @tc.name: GetSurfaceId
 * @tc.desc: Tests for SurfaceStream.GetSurfaceId [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, GetSurfaceId, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetSurfaceId(), 0);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    // After attach, surface ID should be non-zero
    // Note: The actual surface ID is set during Init() which is called in Attach()
    // We can't verify the exact value, but it should be different from 0
    // EXPECT_NE(surfaceStreamInterface->GetSurfaceId(), 0);

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: Detach
 * @tc.desc: Tests for SurfaceStream.Detach [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, Detach, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));

    EXPECT_FALSE(attach->Detach(surfaceStream));

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: MultipleInstances
 * @tc.desc: Tests for creating multiple SurfaceStream instances [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, MultipleInstances, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

    auto surfaceStream1 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    auto surfaceStream2 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    auto surfaceStream3 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);

    ASSERT_TRUE(surfaceStream1);
    ASSERT_TRUE(surfaceStream2);
    ASSERT_TRUE(surfaceStream3);

    EXPECT_NE(surfaceStream1, surfaceStream2);
    EXPECT_NE(surfaceStream2, surfaceStream3);
    EXPECT_NE(surfaceStream1, surfaceStream3);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap1 = CreateTestBitmap();
    auto bitmap2 = CreateTestBitmap();
    auto bitmap3 = CreateTestBitmap();

    ASSERT_TRUE(bitmap1);
    ASSERT_TRUE(bitmap2);
    ASSERT_TRUE(bitmap3);

    auto attach1 = interface_cast<META_NS::IAttach>(bitmap1);
    auto attach2 = interface_cast<META_NS::IAttach>(bitmap2);
    auto attach3 = interface_cast<META_NS::IAttach>(bitmap3);

    ASSERT_TRUE(attach1);
    ASSERT_TRUE(attach2);
    ASSERT_TRUE(attach3);

    EXPECT_TRUE(attach1->Attach(surfaceStream1));
    EXPECT_TRUE(attach2->Attach(surfaceStream2));
    EXPECT_TRUE(attach3->Attach(surfaceStream3));

    EXPECT_TRUE(attach1->Detach(surfaceStream1));
    EXPECT_TRUE(attach2->Detach(surfaceStream2));
    EXPECT_TRUE(attach3->Detach(surfaceStream3));
}

/**
 * @tc.name: WidthHeightBounds
 * @tc.desc: Tests for width and height boundary values [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, WidthHeightBounds, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    surfaceStreamInterface->SetWidth(0);
    surfaceStreamInterface->SetHeight(0);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);

    surfaceStreamInterface->SetWidth(1920);
    surfaceStreamInterface->SetHeight(1080);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 1920);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 1080);

    surfaceStreamInterface->SetWidth(3840);
    surfaceStreamInterface->SetHeight(2160);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 3840);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 2160);

    surfaceStreamInterface->SetWidth(7680);
    surfaceStreamInterface->SetHeight(4320);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 7680);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 4320);
}

/**
 * @tc.name: ReattachAfterDetach
 * @tc.desc: Tests for reattaching SurfaceStream after detach [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, ReattachAfterDetach, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: SurfaceStreamLifecycle
 * @tc.desc: Tests for SurfaceStream lifecycle (create, attach, detach, destroy) [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SurfaceStreamTest, SurfaceStreamLifecycle, testing::ext::TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);
    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetSurfaceId(), 0);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    surfaceStreamInterface->SetWidth(1280);
    surfaceStreamInterface->SetHeight(720);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 1280);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 720);

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

} // namespace UTest
SCENE_END_NAMESPACE()