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

#include <scene/api/scene.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <gmock/gmock-matchers.h>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/ext/attachment/behavior.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_startable.h>
#include <meta/interface/intf_tickable.h>

#include "scene/scene_test.h"

META_REGISTER_INTERFACE(ITestAttachment, "0af32c00-9037-47f9-b7dd-3e43679ffdfa", "This is test attachment")
META_REGISTER_INTERFACE(ITestStartable, "bb8188a9-aeca-4414-98c3-7a989d54d430", "This is test startable")

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class ITestStartable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestStartable)
public:
    virtual uint32_t GetAttachCount() const = 0;
    virtual uint32_t GetDetachCount() const = 0;
    virtual uint32_t GetStartCount() const = 0;
    virtual uint32_t GetStopCount() const = 0;
};

using META_NS::StartableState;

META_REGISTER_CLASS(
    TestStartable, "e9470f5b-97ce-404c-9d1e-b37a4f1ac594", META_NS::ObjectCategoryBits::NO_CATEGORY, "Test startable")

class TestStartable : public META_NS::Behavior<ITestStartable> {
    META_OBJECT(TestStartable, ClassId::TestStartable, Behavior)
public:
    bool OnAttach() override
    {
        attachCount_++;
        return true;
    }
    void OnDetach() override
    {
        detachCount_++;
    }
    bool OnStart() override
    {
        startCount_++;
        return true;
    }
    void OnStop() override
    {
        stopCount_++;
    }

protected: // ITestStartable
    uint32_t GetAttachCount() const override
    {
        return attachCount_;
    }
    uint32_t GetDetachCount() const override
    {
        return detachCount_;
    }
    uint32_t GetStartCount() const override
    {
        return startCount_;
    }
    uint32_t GetStopCount() const override
    {
        return stopCount_;
    }

private:
    uint32_t attachCount_ {};
    uint32_t detachCount_ {};
    uint32_t startCount_ {};
    uint32_t stopCount_ {};
};

class API_ScenePluginNodeStartableTest : public ScenePluginTest {
    using Super = ScenePluginTest;

public:
    void SetUp() override
    {
        Super::SetUp();

        auto& registry = META_NS::GetObjectRegistry();
        registry.RegisterObjectType(TestStartable::GetFactory());

        scene_ = Scene(CreateEmptyScene());
        ASSERT_TRUE(scene_);
        parent_ = scene_.GetResourceFactory().CreateNode("//root");
        ASSERT_TRUE(parent_);
    }

    void TearDown() override
    {
        scene_.Release();
        auto& registry = META_NS::GetObjectRegistry();
        registry.UnregisterObjectType(TestStartable::GetFactory());
        Super::TearDown();
    }
    Scene& GetScene()
    {
        return scene_;
    }

private:
    Node parent_ { nullptr };
    Scene scene_ { nullptr };
};

/**
 * @tc.name: StartableState
 * @tc.desc: Tests for Startable State. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeStartableTest, StartableState, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("//camera");

    auto object = META_NS::CreateObjectInstance(ClassId::TestStartable);
    auto startable = interface_cast<META_NS::IStartable>(object);
    ASSERT_TRUE(startable);
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::DETACHED);
    EXPECT_TRUE(META_NS::AttachmentContainer(camera).Attach(object));
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::ATTACHED);
    UpdateScene();
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::STARTED);
    EXPECT_TRUE(META_NS::AttachmentContainer(camera).Detach(object));
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::DETACHED);
    auto ts = interface_cast<ITestStartable>(object);
    ASSERT_TRUE(ts);
    EXPECT_EQ(ts->GetAttachCount(), 1);
    EXPECT_EQ(ts->GetDetachCount(), 1);
    EXPECT_EQ(ts->GetStartCount(), 1);
    EXPECT_EQ(ts->GetStopCount(), 1);
}

/**
 * @tc.name: SceneStartableState
 * @tc.desc: Tests for Scene Startable State. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeStartableTest, SceneStartableState, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("//camera");

    auto object = META_NS::CreateObjectInstance(ClassId::TestStartable);
    auto startable = interface_cast<META_NS::IStartable>(object);
    ASSERT_TRUE(startable);
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::DETACHED);
    EXPECT_TRUE(META_NS::AttachmentContainer(camera).Attach(object));
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::ATTACHED);
    UpdateScene();
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::STARTED);
    auto ts = interface_cast<ITestStartable>(object);
    ASSERT_TRUE(ts);
    EXPECT_EQ(ts->GetAttachCount(), 1);
    EXPECT_EQ(ts->GetDetachCount(), 0);
    EXPECT_EQ(ts->GetStartCount(), 1);
    EXPECT_EQ(ts->GetStopCount(), 0);

    auto internal = scene.GetPtr<IScene>()->GetInternalScene();
    internal->StopAllStartables(META_NS::IStartableController::ControlBehavior::CONTROL_AUTOMATIC);
    EXPECT_EQ(ts->GetAttachCount(), 1);
    EXPECT_EQ(ts->GetDetachCount(), 0);
    EXPECT_EQ(ts->GetStartCount(), 1);
    EXPECT_EQ(ts->GetStopCount(), 1);

    internal->StartAllStartables(META_NS::IStartableController::ControlBehavior::CONTROL_AUTOMATIC);
    UpdateScene();
    EXPECT_EQ(ts->GetAttachCount(), 1);
    EXPECT_EQ(ts->GetDetachCount(), 0);
    EXPECT_EQ(ts->GetStartCount(), 2);
    EXPECT_EQ(ts->GetStopCount(), 1);
}

/**
 * @tc.name: StartableHierarchyState
 * @tc.desc: Tests for Startable Hierarchy State. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeStartableTest, StartableHierarchyState, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto parent = factory.CreateNode("//hierarchy_root");
    auto camera = factory.CreateCameraNode("//hierarchy_root/camera");

    auto object = META_NS::CreateObjectInstance(ClassId::TestStartable);
    auto startable = interface_cast<META_NS::IStartable>(object);
    auto ts = interface_cast<ITestStartable>(object);
    ASSERT_TRUE(startable);
    ASSERT_TRUE(ts);

    EXPECT_TRUE(META_NS::AttachmentContainer(camera).Attach(object));
    UpdateScene();
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::STARTED);

    ASSERT_TRUE(parent);
    parent.RemoveChild(camera);
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::ATTACHED);
    EXPECT_EQ(ts->GetStopCount(), 1);

    parent.AddChild(camera);
    UpdateScene();
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::STARTED);

    auto root = parent.GetParent();
    ASSERT_TRUE(root);
    root.RemoveChild(parent);
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::ATTACHED);
    EXPECT_EQ(ts->GetStopCount(), 2);
    root.AddChild(parent);
    UpdateScene();
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::STARTED);

    EXPECT_EQ(ts->GetAttachCount(), 1);
    EXPECT_EQ(ts->GetDetachCount(), 0);
    EXPECT_EQ(ts->GetStartCount(), 3);
    EXPECT_EQ(ts->GetStopCount(), 2);
}

/**
 * @tc.name: DisableStartables
 * @tc.desc: Tests for Disable Startables. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeStartableTest, DisableStartables, testing::ext::TestSize.Level1)
{
    auto manager = GetSceneManager();
    ASSERT_TRUE(manager);
    SceneOptions options;
    options.enableStartables = false;
    auto scene = Scene(manager->CreateScene(options).GetResult());
    ASSERT_TRUE(scene);
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("//camera");

    auto object = META_NS::CreateObjectInstance(ClassId::TestStartable);
    auto startable = interface_cast<META_NS::IStartable>(object);
    ASSERT_TRUE(startable);
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::DETACHED);
    EXPECT_TRUE(META_NS::AttachmentContainer(camera).Attach(object));
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::ATTACHED);
    UpdateScene();
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::ATTACHED);
    EXPECT_TRUE(META_NS::AttachmentContainer(camera).Detach(object));
    EXPECT_EQ(META_NS::GetValue(startable->StartableState()), META_NS::StartableState::DETACHED);
    auto ts = interface_cast<ITestStartable>(object);
    ASSERT_TRUE(ts);
    EXPECT_EQ(ts->GetAttachCount(), 1);
    EXPECT_EQ(ts->GetDetachCount(), 1);
    EXPECT_EQ(ts->GetStartCount(), 0);
    EXPECT_EQ(ts->GetStopCount(), 0);
}

} // namespace UTest

SCENE_END_NAMESPACE()
