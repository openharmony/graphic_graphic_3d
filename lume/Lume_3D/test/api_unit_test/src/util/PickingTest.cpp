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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/world_matrix_component.h>
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
void SetAabb(Entity entity, IEcs& ecs, Math::Vec3 aabbMin, Math::Vec3 aabbMax)
{
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
    auto meshManager = GetManager<IMeshComponentManager>(ecs);
    renderMeshManager->Create(entity);
    Entity mesh = ecs.GetEntityManager().Create();
    meshManager->Create(mesh);
    if (auto scopedHandle = meshManager->Write(mesh); scopedHandle) {
        scopedHandle->aabbMin = aabbMin;
        scopedHandle->aabbMax = aabbMax;
    }
    if (auto scopedHandle = renderMeshManager->Write(entity); scopedHandle) {
        scopedHandle->mesh = mesh;
    }
}

void SetWorldMatrixFromPosition(Entity entity, IEcs& ecs, Math::Vec3 position)
{
    auto worldMatrixManager = GetManager<IWorldMatrixComponentManager>(ecs);
    worldMatrixManager->Create(entity);
    if (auto scopedHandle = worldMatrixManager->Write(entity); scopedHandle) {
        scopedHandle->matrix =
            Math::Trs(position, Math::FromEulerRad(Math::Vec3 { 0.0f, 0.0f, 0.0f }), Math::Vec3 { 1.0f, 1.0f, 1.0f });
    }
}
} // namespace

class PickingTest : public testing::Test {
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
 * @tc.name: IInterfaceTest
 * @tc.desc: test IInterface
 * @tc.type: FUNC
 */
HWTEST_F(PickingTest, IInterfaceTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    picking->Ref();
    picking->Unref();
    EXPECT_EQ(picking, picking->GetInterface(IPicking::UID));
    EXPECT_EQ(picking, picking->GetInterface(IInterface::UID));
    EXPECT_EQ(nullptr, picking->GetInterface(INodeSystem::UID));
    EXPECT_EQ(picking, ((const IPicking*)picking)->GetInterface(IPicking::UID));
    EXPECT_EQ(picking, ((const IPicking*)picking)->GetInterface(IInterface::UID));
    EXPECT_EQ(nullptr, ((const IPicking*)picking)->GetInterface(INodeSystem::UID));
}

/**
 * @tc.name: GetTransformComponentAABBTest
 * @tc.desc: test GetTransformComponentAABB
 * @tc.type: FUNC
 */
HWTEST_F(PickingTest, GetTransformComponentAABBTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    ISceneNode* root = nodeSystem->CreateNode();
    root->SetPosition(Math::Vec3 { -5.0f, 0.0f, 5.0f });
    SetAabb(root->GetEntity(), *ecs, Math::Vec3 { -1.0f, -1.0f, -1.0f }, Math::Vec3 { 2.0f, 2.0f, 2.0f });

    ISceneNode* child1 = nodeSystem->CreateNode();
    child1->SetPosition(Math::Vec3 { 0.0f, 5.0f, -5.0f });
    child1->SetParent(*root);
    SetAabb(child1->GetEntity(), *ecs, Math::Vec3 { -2.0f, -2.0f, -2.0f }, Math::Vec3 { 0.0f, 0.0f, 0.0f });

    ISceneNode* child2 = nodeSystem->CreateNode();
    child2->SetPosition(Math::Vec3 { 5.0f, -5.0f, 0.0f });
    child2->SetParent(*root);
    SetAabb(child2->GetEntity(), *ecs, Math::Vec3 { 0.0f, 0.0f, 0.0f }, Math::Vec3 { 3.0f, 3.0f, 3.0f });

    MinAndMax mam = picking->GetTransformComponentAABB(root->GetEntity(), true, *ecs);
    EXPECT_EQ(-7.0, mam.minAABB.x);
    EXPECT_EQ(-5.0, mam.minAABB.y);
    EXPECT_EQ(-2.0, mam.minAABB.z);
    EXPECT_EQ(3.0, mam.maxAABB.x);
    EXPECT_EQ(5.0, mam.maxAABB.y);
    EXPECT_EQ(8.0, mam.maxAABB.z);
}

/**
 * @tc.name: GetWorldMatrixComponentAABBTest
 * @tc.desc: test GetWorldMatrixComponentAABB
 * @tc.type: FUNC
 */
HWTEST_F(PickingTest, GetWorldMatrixComponentAABBTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);
    auto jointMatricesManager = GetManager<IJointMatricesComponentManager>(*ecs);
    ASSERT_NE(nullptr, jointMatricesManager);

    ISceneNode* root = nodeSystem->CreateNode();
    SetWorldMatrixFromPosition(root->GetEntity(), *ecs, Math::Vec3 { -5.0f, 0.0f, 5.0f });
    jointMatricesManager->Create(root->GetEntity());
    if (auto scopedHandle = jointMatricesManager->Write(root->GetEntity()); scopedHandle) {
        scopedHandle->jointsAabbMin = Math::Vec3 { -6.0f, -1.0f, 4.0f };
        scopedHandle->jointsAabbMax = Math::Vec3 { -3.0f, 2.0f, 7.0f };
    }

    ISceneNode* child1 = nodeSystem->CreateNode();
    SetWorldMatrixFromPosition(child1->GetEntity(), *ecs, Math::Vec3 { -5.0f, 5.0f, 0.0f });
    child1->SetParent(*root);
    SetAabb(child1->GetEntity(), *ecs, Math::Vec3 { -2.0f, -2.0f, -2.0f }, Math::Vec3 { 0.0f, 0.0f, 0.0f });

    ISceneNode* child2 = nodeSystem->CreateNode();
    SetWorldMatrixFromPosition(child2->GetEntity(), *ecs, Math::Vec3 { 0.0f, -5.0f, 5.0f });
    child2->SetParent(*root);
    SetAabb(child2->GetEntity(), *ecs, Math::Vec3 { 0.0f, 0.0f, 0.0f }, Math::Vec3 { 3.0f, 3.0f, 3.0f });

    MinAndMax mam = picking->GetWorldMatrixComponentAABB(root->GetEntity(), true, *ecs);
    EXPECT_EQ(-7.0, mam.minAABB.x);
    EXPECT_EQ(-5.0, mam.minAABB.y);
    EXPECT_EQ(-2.0, mam.minAABB.z);
    EXPECT_EQ(3.0, mam.maxAABB.x);
    EXPECT_EQ(5.0, mam.maxAABB.y);
    EXPECT_EQ(8.0, mam.maxAABB.z);
}

/**
 * @tc.name: WorldAndScreenCoordinatesTest
 * @tc.desc: test WorldAndScreenCoordinates
 * @tc.type: FUNC
 */
HWTEST_F(PickingTest, WorldAndScreenCoordinatesTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);

    Entity camera = graphicsContext->GetSceneUtil().CreateCamera(*ecs, Math::Vec3(0.0f, 0.5f, 4.0f),
        Math::FromEulerRad(Math::Vec3 { 0.03f, 0.05f, 0.07f }), 2.0f, 100.0f, 75.0f);

    constexpr const float eps = 1e-6f;
    {
        Math::Vec3 worldCoords { -0.1f, 0.6f, -4.0f };
        Math::Vec3 screenCoords = picking->WorldToScreen(*ecs, camera, worldCoords);
        Math::Vec3 result = picking->ScreenToWorld(*ecs, camera, screenCoords);
        for (uint32_t i = 0; i < 3; ++i) {
            EXPECT_LE(worldCoords[i] - eps, result[i]);
            EXPECT_GE(worldCoords[i] + eps, result[i]);
        }
    }
    {
        Math::Vec3 screenCoords { -0.1f, 0.3f, -0.25f };
        Math::Vec3 worldCoords = picking->ScreenToWorld(*ecs, camera, screenCoords);
        Math::Vec3 result = picking->WorldToScreen(*ecs, camera, worldCoords);
        for (uint32_t i = 0; i < 3; ++i) {
            EXPECT_LE(screenCoords[i] - eps, result[i]);
            EXPECT_GE(screenCoords[i] + eps, result[i]);
        }
    }
    {
        EXPECT_EQ(Math::Vec3 {}, picking->WorldToScreen(*ecs, Entity {}, Math::Vec3 {}));
        EXPECT_EQ(Math::Vec3 {}, picking->ScreenToWorld(*ecs, Entity {}, Math::Vec3 {}));
    }
}
