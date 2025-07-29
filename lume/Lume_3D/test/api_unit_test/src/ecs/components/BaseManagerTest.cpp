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
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/fog_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/physical_camera_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_batch_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/rsdz_model_id_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

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
template<typename ComponentType, typename ManagerType>
void BaseManagerCreateTest(BASE_NS::string componentName)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto baseManager = GetManager<ManagerType>(*ecs);
    auto componentManager = static_cast<IComponentManager*>(baseManager);

    ASSERT_TRUE(baseManager);
    EXPECT_EQ(componentName, componentManager->GetName());

    Entity entity = ecs->GetEntityManager().Create();
    uint32_t generation;
    {
        baseManager->Create(entity);
        ASSERT_TRUE(baseManager->HasComponent(entity));
        auto id = baseManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        EXPECT_EQ(IComponentManager::INVALID_COMPONENT_ID, baseManager->GetComponentId(Entity {}));
        ASSERT_EQ(entity, baseManager->GetEntity(id));
        generation = baseManager->GetComponentGeneration(id);
        EXPECT_LE(generation, baseManager->GetGenerationCounter());
        EXPECT_EQ(0u, baseManager->GetComponentGeneration(IComponentManager::INVALID_COMPONENT_ID));
        EXPECT_TRUE(baseManager->GetModifiedFlags() & CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT);
    }
    // Recreate entity
    {
        baseManager->Create(entity);
        ASSERT_TRUE(baseManager->HasComponent(entity));
        auto id = baseManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        ASSERT_EQ(entity, baseManager->GetEntity(id));
        ASSERT_NE(generation, baseManager->GetComponentGeneration(id));
    }
}

template<typename ComponentType, typename ManagerType>
void BaseManagerIPropertyApiTest(BASE_NS::string componentName)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto baseManager = GetManager<ManagerType>(*ecs);
    ASSERT_TRUE(baseManager);
    auto& propertyApi = baseManager->GetPropertyApi();
    const uint64_t typeHash = BASE_NS::FNV1aHash(componentName.data(), componentName.size());
    EXPECT_EQ(typeHash, propertyApi.Type());

    {
        auto propertyHandle = propertyApi.Create();
        ASSERT_NE(nullptr, propertyHandle);
        ASSERT_EQ(sizeof(ComponentType), propertyHandle->Size());
        {
            auto propertyApi2 = propertyHandle->Owner();
            ASSERT_EQ(propertyApi.Type(), propertyApi2->Type());
            ASSERT_EQ(propertyApi.PropertyCount(), propertyApi2->PropertyCount());
            for (uint32_t i = 0; i < propertyApi2->PropertyCount(); ++i) {
                ASSERT_EQ(propertyApi.MetaData(i), propertyApi2->MetaData(i));
            }
            ASSERT_EQ(nullptr, propertyApi2->MetaData(propertyApi2->PropertyCount()));
            auto propertyHandle2 = propertyApi2->Create();
            auto clone = propertyApi2->Clone(propertyHandle2);
            EXPECT_NE(nullptr, clone);
            propertyApi2->Release(propertyHandle2);
            propertyApi2->Release(clone);
        }
        auto clone = propertyApi.Clone(propertyHandle);
        EXPECT_NE(nullptr, clone);
        propertyApi.Release(propertyHandle);
        propertyApi.Release(clone);
    }

    {
        size_t propertyCount = propertyApi.PropertyCount();
        auto metadata = propertyApi.MetaData();
        ASSERT_EQ(propertyCount, metadata.size());
        for (size_t i = 0; i < propertyCount; ++i) {
            ASSERT_EQ(&metadata[i], propertyApi.MetaData(i));
        }
        ASSERT_EQ(nullptr, propertyApi.MetaData(propertyCount));
    }
}
} // namespace

class BaseManagerTest : public testing::Test {
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
 * @tc.name: UriComponent_CreateTest
 * @tc.desc: test UriComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, UriComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<UriComponent, IUriComponentManager>("UriComponent");
}
/**
 * @tc.name: UriComponent_IPropertyApiTest
 * @tc.desc: test UriComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, UriComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<UriComponent, IUriComponentManager>("UriComponent");
}

/**
 * @tc.name: AnimationComponent_CreateTest
 * @tc.desc: test AnimationComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<AnimationComponent, IAnimationComponentManager>("AnimationComponent");
}

/**
 * @tc.name: AnimationComponent_IPropertyApiTest
 * @tc.desc: test AnimationComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationComponent, IAnimationComponentManager>("AnimationComponent");
}

/**
 * @tc.name: AnimationInputComponent_CreateTest
 * @tc.desc: test AnimationInputComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationInputComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<AnimationInputComponent, IAnimationInputComponentManager>("AnimationInputComponent");
}
/**
 * @tc.name: AnimationInputComponent_IPropertyApiTest
 * @tc.desc: test AnimationInputComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationInputComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationInputComponent, IAnimationInputComponentManager>("AnimationInputComponent");
}

/**
 * @tc.name: AnimationOutputComponent_CreateTest
 * @tc.desc: test AnimationOutputComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationOutputComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<AnimationOutputComponent, IAnimationOutputComponentManager>("AnimationOutputComponent");
}
/**
 * @tc.name: AnimationOutputComponent_IPropertyApiTest
 * @tc.desc: test AnimationOutputComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationOutputComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationOutputComponent, IAnimationOutputComponentManager>("AnimationOutputComponent");
}

/**
 * @tc.name: AnimationStateComponent_CreateTest
 * @tc.desc: test AnimationStateComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationStateComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<AnimationStateComponent, IAnimationStateComponentManager>("AnimationStateComponent");
}
/**
 * @tc.name: AnimationStateComponent_IPropertyApiTest
 * @tc.desc: test AnimationStateComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationStateComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationStateComponent, IAnimationStateComponentManager>("AnimationStateComponent");
}

/**
 * @tc.name: AnimationTrackComponent_CreateTest
 * @tc.desc: test AnimationTrackComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationTrackComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<AnimationTrackComponent, IAnimationTrackComponentManager>("AnimationTrackComponent");
}
/**
 * @tc.name: AnimationTrackComponent_IPropertyApiTest
 * @tc.desc: test AnimationTrackComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, AnimationTrackComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationTrackComponent, IAnimationTrackComponentManager>("AnimationTrackComponent");
}

/**
 * @tc.name: CameraComponent_CreateTest
 * @tc.desc: test CameraComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, CameraComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<CameraComponent, ICameraComponentManager>("CameraComponent");
}
/**
 * @tc.name: CameraComponent_IPropertyApiTest
 * @tc.desc: test CameraComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, CameraComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<CameraComponent, ICameraComponentManager>("CameraComponent");
}

/**
 * @tc.name: EnvironmentComponent_CreateTest
 * @tc.desc: test EnvironmentComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, EnvironmentComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<EnvironmentComponent, IEnvironmentComponentManager>("EnvironmentComponent");
}
/**
 * @tc.name: EnvironmentComponent_IPropertyApiTest
 * @tc.desc: test EnvironmentComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, EnvironmentComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<EnvironmentComponent, IEnvironmentComponentManager>("EnvironmentComponent");
}

/**
 * @tc.name: FogComponent_CreateTest
 * @tc.desc: test FogComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, FogComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<FogComponent, IFogComponentManager>("FogComponent");
}
/**
 * @tc.name: FogComponent_IPropertyApiTest
 * @tc.desc: test FogComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, FogComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<FogComponent, IFogComponentManager>("FogComponent");
}

/**
 * @tc.name: JointMatricesComponent_CreateTest
 * @tc.desc: test JointMatricesComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, JointMatricesComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<JointMatricesComponent, IJointMatricesComponentManager>("JointMatricesComponent");
}
/**
 * @tc.name: JointMatricesComponent_IPropertyApiTest
 * @tc.desc: test JointMatricesComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, JointMatricesComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<JointMatricesComponent, IJointMatricesComponentManager>("JointMatricesComponent");
}

/**
 * @tc.name: LayerComponent_CreateTest
 * @tc.desc: test LayerComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, LayerComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<LayerComponent, ILayerComponentManager>("LayerComponent");
}
/**
 * @tc.name: LayerComponent_IPropertyApiTest
 * @tc.desc: test LayerComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, LayerComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<LayerComponent, ILayerComponentManager>("LayerComponent");
}

/**
 * @tc.name: LightComponent_CreateTest
 * @tc.desc: test LightComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, LightComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<LightComponent, ILightComponentManager>("LightComponent");
}
/**
 * @tc.name: LightComponent_IPropertyApiTest
 * @tc.desc: test LightComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, LightComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<LightComponent, ILightComponentManager>("LightComponent");
}

/**
 * @tc.name: LocalMatrixComponent_CreateTest
 * @tc.desc: test LocalMatrixComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, LocalMatrixComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<LocalMatrixComponent, ILocalMatrixComponentManager>("LocalMatrixComponent");
}
/**
 * @tc.name: LocalMatrixComponent_IPropertyApiTest
 * @tc.desc: test LocalMatrixComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, LocalMatrixComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<LocalMatrixComponent, ILocalMatrixComponentManager>("LocalMatrixComponent");
}

/**
 * @tc.name: MeshComponent_CreateTest
 * @tc.desc: test MeshComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, MeshComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<MeshComponent, IMeshComponentManager>("MeshComponent");
}
/**
 * @tc.name: MeshComponent_IPropertyApiTest
 * @tc.desc: test MeshComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, MeshComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<MeshComponent, IMeshComponentManager>("MeshComponent");
}

/**
 * @tc.name: MorphComponent_CreateTest
 * @tc.desc: test MorphComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, MorphComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<MorphComponent, IMorphComponentManager>("MorphComponent");
}
/**
 * @tc.name: MorphComponent_IPropertyApiTest
 * @tc.desc: test MorphComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, MorphComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<MorphComponent, IMorphComponentManager>("MorphComponent");
}

/**
 * @tc.name: NameComponent_CreateTest
 * @tc.desc: test NameComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, NameComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<NameComponent, INameComponentManager>("NameComponent");
}
/**
 * @tc.name: NameComponent_IPropertyApiTest
 * @tc.desc: test NameComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest, NameComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<NameComponent, INameComponentManager>("NameComponent");
}

class BaseManagerTest1 : public testing::Test {
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
 * @tc.name: WorldMatrixComponent_CreateTest
 * @tc.desc: test WorldMatrixComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, WorldMatrixComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<WorldMatrixComponent, IWorldMatrixComponentManager>("WorldMatrixComponent");
}
/**
 * @tc.name: WorldMatrixComponent_IPropertyApiTest
 * @tc.desc: test WorldMatrixComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, WorldMatrixComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<WorldMatrixComponent, IWorldMatrixComponentManager>("WorldMatrixComponent");
}

/**
 * @tc.name: NodeComponent_CreateTest
 * @tc.desc: test NodeComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, NodeComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<NodeComponent, INodeComponentManager>("NodeComponent");
}
/**
 * @tc.name: NodeComponent_IPropertyApiTest
 * @tc.desc: test NodeComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, NodeComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<NodeComponent, INodeComponentManager>("NodeComponent");
}

/**
 * @tc.name: PlanarReflectionComponent_CreateTest
 * @tc.desc: test PlanarReflectionComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, PlanarReflectionComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<PlanarReflectionComponent, IPlanarReflectionComponentManager>("PlanarReflectionComponent");
}
/**
 * @tc.name: PlanarReflectionComponent_IPropertyApiTest
 * @tc.desc: test PlanarReflectionComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, PlanarReflectionComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<PlanarReflectionComponent, IPlanarReflectionComponentManager>(
        "PlanarReflectionComponent");
}

/**
 * @tc.name: PostProcessComponent_CreateTest
 * @tc.desc: test PostProcessComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, PostProcessComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<PostProcessComponent, IPostProcessComponentManager>("PostProcessComponent");
}
/**
 * @tc.name: PostProcessComponent_IPropertyApiTest
 * @tc.desc: test PostProcessComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, PostProcessComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<PostProcessComponent, IPostProcessComponentManager>("PostProcessComponent");
}

/**
 * @tc.name: RenderConfigurationComponent_CreateTest
 * @tc.desc: test RenderConfigurationComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderConfigurationComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<RenderConfigurationComponent, IRenderConfigurationComponentManager>(
        "RenderConfigurationComponent");
}
/**
 * @tc.name: RenderConfigurationComponent_IPropertyApiTest
 * @tc.desc: test RenderConfigurationComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderConfigurationComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderConfigurationComponent, IRenderConfigurationComponentManager>(
        "RenderConfigurationComponent");
}
/**
 * @tc.name: RenderHandleComponent_CreateTest
 * @tc.desc: test RenderHandleComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderHandleComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<RenderHandleComponent, IRenderHandleComponentManager>("RenderHandleComponent");
}

/**
 * @tc.name: RenderHandleComponent_IPropertyApiTest
 * @tc.desc: test RenderHandleComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderHandleComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderHandleComponent, IRenderHandleComponentManager>("RenderHandleComponent");
}

/**
 * @tc.name: RenderMeshBatchComponent_CreateTest
 * @tc.desc: test RenderMeshBatchComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderMeshBatchComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<RenderMeshBatchComponent, IRenderMeshBatchComponentManager>("RenderMeshBatchComponent");
}
/**
 * @tc.name: RenderMeshBatchComponent_IPropertyApiTest
 * @tc.desc: test RenderMeshBatchComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderMeshBatchComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderMeshBatchComponent, IRenderMeshBatchComponentManager>("RenderMeshBatchComponent");
}

/**
 * @tc.name: RenderMeshComponent_CreateTest
 * @tc.desc: test RenderMeshComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderMeshComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<RenderMeshComponent, IRenderMeshComponentManager>("RenderMeshComponent");
}
/**
 * @tc.name: RenderMeshComponent_IPropertyApiTest
 * @tc.desc: test RenderMeshComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RenderMeshComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderMeshComponent, IRenderMeshComponentManager>("RenderMeshComponent");
}

/**
 * @tc.name: RSDZModelIdComponent_CreateTest
 * @tc.desc: test RSDZModelIdComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RSDZModelIdComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<RSDZModelIdComponent, IRSDZModelIdComponentManager>("RSDZModelIdComponent");
}
/**
 * @tc.name: RSDZModelIdComponent_IPropertyApiTest
 * @tc.desc: test RSDZModelIdComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, RSDZModelIdComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RSDZModelIdComponent, IRSDZModelIdComponentManager>("RSDZModelIdComponent");
}

/**
 * @tc.name: SkinComponent_CreateTest
 * @tc.desc: test SkinComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, SkinComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<SkinComponent, ISkinComponentManager>("SkinComponent");
}
/**
 * @tc.name: SkinComponent_IPropertyApiTest
 * @tc.desc: test SkinComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, SkinComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<SkinComponent, ISkinComponentManager>("SkinComponent");
}

/**
 * @tc.name: SkinIbmComponent_CreateTest
 * @tc.desc: test SkinIbmComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, SkinIbmComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<SkinIbmComponent, ISkinIbmComponentManager>("SkinIbmComponent");
}
/**
 * @tc.name: SkinIbmComponent_IPropertyApiTest
 * @tc.desc: test SkinIbmComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, SkinIbmComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<SkinIbmComponent, ISkinIbmComponentManager>("SkinIbmComponent");
}

/**
 * @tc.name: SkinJointsComponent_CreateTest
 * @tc.desc: test SkinJointsComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, SkinJointsComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<SkinJointsComponent, ISkinJointsComponentManager>("SkinJointsComponent");
}
/**
 * @tc.name: SkinJointsComponent_IPropertyApiTest
 * @tc.desc: test SkinJointsComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, SkinJointsComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<SkinJointsComponent, ISkinJointsComponentManager>("SkinJointsComponent");
}

/**
 * @tc.name: TransformComponent_CreateTest
 * @tc.desc: test TransformComponent_CreateTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, TransformComponent_CreateTest, TestSize.Level1)
{
    BaseManagerCreateTest<TransformComponent, ITransformComponentManager>("TransformComponent");
}
/**
 * @tc.name: TransformComponent_IPropertyApiTest
 * @tc.desc: test TransformComponent_IPropertyApiTest
 * @tc.type: FUNC
 */
HWTEST_F(BaseManagerTest1, TransformComponent_IPropertyApiTest, TestSize.Level1)
{
    BaseManagerIPropertyApiTest<TransformComponent, ITransformComponentManager>("TransformComponent");
}