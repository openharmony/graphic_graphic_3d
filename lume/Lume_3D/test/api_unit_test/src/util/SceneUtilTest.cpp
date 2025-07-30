/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_picking.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/implementation_uids.h>

#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using namespace testing::ext;

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<Core::ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

namespace {
Entity CreateJoint(IEcs& ecs, Base::string name)
{
    Entity joint = ecs.GetEntityManager().Create();
    auto nameManager = GetManager<INameComponentManager>(ecs);
    nameManager->Create(joint);
    if (auto scopedHandle = nameManager->Write(joint); scopedHandle) {
        scopedHandle->name = name;
    }
    return joint;
}
} // namespace

class SceneUtilTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};


/**
 * @tc.name: ShareSkinTest
 * @tc.desc: test ShareSkin
 * @tc.type: FUNC
 */
HWTEST_F(SceneUtilTest, ShareSkinTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto& sceneUtil = graphicsContext->GetSceneUtil();
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);

    Entity joint0 = CreateJoint(*ecs, "joint0");
    Entity joint1 = CreateJoint(*ecs, "joint1");
    Entity joint2 = CreateJoint(*ecs, "joint2");

    Entity sourceEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(sourceEntity);
    if (auto scopedHandle = skinJointsManager->Write(sourceEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint0;
        scopedHandle->jointEntities[1] = joint1;
        scopedHandle->jointEntities[2] = joint2;
    }

    Entity joint3 = CreateJoint(*ecs, "joint2");
    Entity joint4 = CreateJoint(*ecs, "joint3");
    Entity joint5 = CreateJoint(*ecs, "joint0");

    Entity targetEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(targetEntity);
    if (auto scopedHandle = skinJointsManager->Write(targetEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint3;
        scopedHandle->jointEntities[1] = joint4;
        scopedHandle->jointEntities[2] = joint5;
    }

    // Share to invalid entity should do nothing
    sceneUtil.ShareSkin(*ecs, Entity {}, sourceEntity);
    if (auto scopedHandle = skinJointsManager->Read(sourceEntity); scopedHandle) {
        EXPECT_EQ(3, scopedHandle->count);
        EXPECT_EQ(joint0, scopedHandle->jointEntities[0]);
        EXPECT_EQ(joint1, scopedHandle->jointEntities[1]);
        EXPECT_EQ(joint2, scopedHandle->jointEntities[2]);
    }

    // Share from invalid entity should do nothing
    sceneUtil.ShareSkin(*ecs, targetEntity, Entity {});
    if (auto scopedHandle = skinJointsManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(3, scopedHandle->count);
        EXPECT_EQ(joint3, scopedHandle->jointEntities[0]);
        EXPECT_EQ(joint4, scopedHandle->jointEntities[1]);
        EXPECT_EQ(joint5, scopedHandle->jointEntities[2]);
    }

    // Valid share should match joints by name component
    sceneUtil.ShareSkin(*ecs, targetEntity, sourceEntity);
    if (auto scopedHandle = skinJointsManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(3, scopedHandle->count);
        EXPECT_EQ(joint2, scopedHandle->jointEntities[0]);
        EXPECT_EQ(Entity {}, scopedHandle->jointEntities[1]);
        EXPECT_EQ(joint0, scopedHandle->jointEntities[2]);
    }
}

namespace {
Entity CreateAnimation(IEcs& ecs, Entity entity)
{
    auto animationManager = GetManager<IAnimationComponentManager>(ecs);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(ecs);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(ecs);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(ecs);
    auto animationSystem = GetSystem<IAnimationSystem>(ecs);
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(ecs);
    auto transformManager = GetManager<ITransformComponentManager>(ecs);
    auto nameManager = GetManager<INameComponentManager>(ecs);

    EntityReference keyFrameData = ecs.GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::VEC3_T;
        constexpr Math::Vec3 values[] = { { 0.0f, 0.0f, 0.0f }, { 5.0f, 5.0f, 5.0f } };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs.GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Base::vector<Entity> targetEntities;
    Base::vector<EntityReference> tracks;
    auto jointData = skinJointsManager->Read(entity);
    for (uint32_t i = 0; i < jointData->count; ++i) {
        Entity targetEntity = jointData->jointEntities[i];
        transformManager->Create(targetEntity);
        targetEntities.push_back(targetEntity);

        EntityReference track = ecs.GetEntityManager().CreateReferenceCounted();
        animationTrackManager->Create(track);
        if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
            scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::STEP;
            scopedHandle->timestamps = timeStamps;
            scopedHandle->data = keyFrameData;
            scopedHandle->component = ITransformComponentManager::UID;
            scopedHandle->property = "position";
        }
        tracks.push_back(track);
    }

    Entity animation = ecs.GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks = tracks;
    }
    nameManager->Create(animation);
    if (auto scopedHandle = nameManager->Write(animation); scopedHandle) {
        scopedHandle->name = "animation";
    }

    IAnimationPlayback* playback =
        animationSystem->CreatePlayback(animation, { targetEntities.data(), targetEntities.size() });
    EXPECT_TRUE(playback);
    ecs.ProcessEvents();
    return animation;
}
} // namespace

/**
 * @tc.name: RetargetSkinAnimationTest
 * @tc.desc: test RetargetSkinAnimation
 * @tc.type: FUNC
 */
HWTEST_F(SceneUtilTest, RetargetSkinAnimationTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto& sceneUtil = graphicsContext->GetSceneUtil();
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);
    auto nameManager = GetManager<INameComponentManager>(*ecs);
    ASSERT_NE(nullptr, nameManager);
    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, renderMeshManager);
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, meshManager);

    Entity joint0 = CreateJoint(*ecs, "joint0");
    Entity joint1 = CreateJoint(*ecs, "joint1");
    Entity joint2 = CreateJoint(*ecs, "joint2");

    Entity sourceEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(sourceEntity);
    if (auto scopedHandle = skinJointsManager->Write(sourceEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint0;
        scopedHandle->jointEntities[1] = joint1;
        scopedHandle->jointEntities[2] = joint2;
    }
    renderMeshManager->Create(sourceEntity);
    if (auto rmHandle = renderMeshManager->Write(sourceEntity); rmHandle) {
        rmHandle->mesh = ecs->GetEntityManager().Create();
        meshManager->Create(rmHandle->mesh);
        if (auto meshHandle = meshManager->Write(rmHandle->mesh); meshHandle) {
            meshHandle->aabbMin.y = 0.0f;
            meshHandle->aabbMax.y = 1.0f;
        }
    }

    Entity animation = CreateAnimation(*ecs, sourceEntity);

    Entity joint3 = CreateJoint(*ecs, "joint2");
    Entity joint4 = CreateJoint(*ecs, "joint0");
    Entity joint5 = CreateJoint(*ecs, "joint1");

    Entity targetEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(targetEntity);
    if (auto scopedHandle = skinJointsManager->Write(targetEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint3;
        scopedHandle->jointEntities[1] = joint4;
        scopedHandle->jointEntities[2] = joint5;
    }
    renderMeshManager->Create(targetEntity);
    if (auto rmHandle = renderMeshManager->Write(targetEntity); rmHandle) {
        rmHandle->mesh = ecs->GetEntityManager().Create();
        meshManager->Create(rmHandle->mesh);
        if (auto meshHandle = meshManager->Write(rmHandle->mesh); meshHandle) {
            meshHandle->aabbMin.y = 0.0f;
            meshHandle->aabbMax.y = 2.0f;
        }
    }

    auto playback = sceneUtil.RetargetSkinAnimation(*ecs, targetEntity, sourceEntity, animation);
    ASSERT_NE(nullptr, playback);
    auto targetAnimation = playback->GetEntity();
    EXPECT_TRUE(nameManager->HasComponent(targetAnimation));
    if (auto scopedHandle = nameManager->Read(targetAnimation); scopedHandle) {
        EXPECT_EQ("animation retargeted", scopedHandle->name);
    }
    EXPECT_TRUE(animationManager->HasComponent(targetAnimation));
    if (auto animationHandle = animationManager->Read(targetAnimation); animationHandle) {
        ASSERT_EQ(3, animationHandle->tracks.size());
        for (EntityReference track : animationHandle->tracks) {
            if (auto trackHandle = animationTrackManager->Read(track); trackHandle) {
                EXPECT_EQ("position", trackHandle->property);
                EXPECT_EQ(ITransformComponentManager::UID, trackHandle->component);
                EXPECT_EQ(AnimationTrackComponent::Interpolation::STEP, trackHandle->interpolationMode);
                if (auto outputHandle = animationOutputManager->Read(trackHandle->data); outputHandle) {
                    ASSERT_EQ(6u * sizeof(float), outputHandle->data.size());
                    ASSERT_EQ(PropertyType::VEC3_T, outputHandle->type);
                    const Math::Vec3* data = reinterpret_cast<const Math::Vec3*>(outputHandle->data.data());
                    EXPECT_EQ(0.0f, data[0].x);
                    EXPECT_EQ(0.0f, data[0].y);
                    EXPECT_EQ(0.0f, data[0].z);
                    EXPECT_EQ(10.0f, data[1].x);
                    EXPECT_EQ(10.0f, data[1].y);
                    EXPECT_EQ(10.0f, data[1].z);
                }
            }
        }
    }
}