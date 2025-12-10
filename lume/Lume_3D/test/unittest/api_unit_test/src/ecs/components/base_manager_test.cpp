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

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/dynamic_environment_blender_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/fog_component.h>
#include <3d/ecs/components/graphics_state_component.h>
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
#include <3d/ecs/components/post_process_effect_component.h>
#include <3d/ecs/components/reflection_probe_component.h>
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
#include <3d/ecs/components/water_ripple_component.h>
#include <3d/ecs/components/weather_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

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
template<typename ComponentType, typename ManagerType>
void BaseManagerCreateTest(BASE_NS::string_view componentName)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto baseManager = GetManager<ManagerType>(*ecs);
    auto componentManager = static_cast<IComponentManager*>(baseManager);
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
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto baseManager = GetManager<ManagerType>(*ecs);
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

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<AnimationComponent, IAnimationComponentManager>("AnimationComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationComponent, IAnimationComponentManager>("AnimationComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationInputComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<AnimationInputComponent, IAnimationInputComponentManager>("AnimationInputComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationInputComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationInputComponent, IAnimationInputComponentManager>("AnimationInputComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationOutputComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<AnimationOutputComponent, IAnimationOutputComponentManager>("AnimationOutputComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationOutputComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationOutputComponent, IAnimationOutputComponentManager>("AnimationOutputComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationStateComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<AnimationStateComponent, IAnimationStateComponentManager>("AnimationStateComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationStateComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationStateComponent, IAnimationStateComponentManager>("AnimationStateComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationTrackComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<AnimationTrackComponent, IAnimationTrackComponentManager>("AnimationTrackComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationTrackComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<AnimationTrackComponent, IAnimationTrackComponentManager>("AnimationTrackComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsCameraComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<CameraComponent, ICameraComponentManager>("CameraComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsCameraComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<CameraComponent, ICameraComponentManager>("CameraComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsDynamicEnvironmentComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<DynamicEnvironmentBlenderComponent, IDynamicEnvironmentBlenderComponentManager>(
        "DynamicEnvironmentBlenderComponent");
}

/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsDynamicEnvironmentComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<DynamicEnvironmentBlenderComponent, IDynamicEnvironmentBlenderComponentManager>(
        "DynamicEnvironmentBlenderComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsEnvironmentComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<EnvironmentComponent, IEnvironmentComponentManager>("EnvironmentComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsEnvironmentComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<EnvironmentComponent, IEnvironmentComponentManager>("EnvironmentComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsFogComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<FogComponent, IFogComponentManager>("FogComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsFogComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<FogComponent, IFogComponentManager>("FogComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsGraphicsStateComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<GraphicsStateComponent, IGraphicsStateComponentManager>("GraphicsStateComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsGraphicsStateComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<GraphicsStateComponent, IGraphicsStateComponentManager>("GraphicsStateComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsJointMatricesComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<JointMatricesComponent, IJointMatricesComponentManager>("JointMatricesComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsJointMatricesComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<JointMatricesComponent, IJointMatricesComponentManager>("JointMatricesComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsLayerComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<LayerComponent, ILayerComponentManager>("LayerComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsLayerComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<LayerComponent, ILayerComponentManager>("LayerComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsLightComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<LightComponent, ILightComponentManager>("LightComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsLightComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<LightComponent, ILightComponentManager>("LightComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsLocalMatrixComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<LocalMatrixComponent, ILocalMatrixComponentManager>("LocalMatrixComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsLocalMatrixComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<LocalMatrixComponent, ILocalMatrixComponentManager>("LocalMatrixComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMeshComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<MeshComponent, IMeshComponentManager>("MeshComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMeshComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<MeshComponent, IMeshComponentManager>("MeshComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMorphComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<MorphComponent, IMorphComponentManager>("MorphComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMorphComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<MorphComponent, IMorphComponentManager>("MorphComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNameComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<NameComponent, INameComponentManager>("NameComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNameComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<NameComponent, INameComponentManager>("NameComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<NodeComponent, INodeComponentManager>("NodeComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<NodeComponent, INodeComponentManager>("NodeComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPhysicalCameraComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<PhysicalCameraComponent, IPhysicalCameraComponentManager>("PhysicalCameraComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPhysicalCameraComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<PhysicalCameraComponent, IPhysicalCameraComponentManager>("PhysicalCameraComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPlanarReflectionComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<PlanarReflectionComponent, IPlanarReflectionComponentManager>("PlanarReflectionComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPlanarReflectionComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<PlanarReflectionComponent, IPlanarReflectionComponentManager>(
        "PlanarReflectionComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<PostProcessComponent, IPostProcessComponentManager>("PostProcessComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<PostProcessComponent, IPostProcessComponentManager>("PostProcessComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Test component creation.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessEffectComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<PostProcessEffectComponent, IPostProcessEffectComponentManager>("PostProcessEffectComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Test IPropertyApi usage with the component.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessEffectComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<PostProcessEffectComponent, IPostProcessEffectComponentManager>(
        "PostProcessEffectComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsReflectionProbeComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<ReflectionProbeComponent, IReflectionProbeComponentManager>("ReflectionProbeComponent");
}

/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for IPropertyApi Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsReflectionProbeComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<ReflectionProbeComponent, IReflectionProbeComponentManager>("ReflectionProbeComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderConfigurationComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<RenderConfigurationComponent, IRenderConfigurationComponentManager>(
        "RenderConfigurationComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderConfigurationComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderConfigurationComponent, IRenderConfigurationComponentManager>(
        "RenderConfigurationComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderHandleComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<RenderHandleComponent, IRenderHandleComponentManager>("RenderHandleComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderHandleComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderHandleComponent, IRenderHandleComponentManager>("RenderHandleComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderMeshBatchComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<RenderMeshBatchComponent, IRenderMeshBatchComponentManager>("RenderMeshBatchComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderMeshBatchComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderMeshBatchComponent, IRenderMeshBatchComponentManager>("RenderMeshBatchComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderMeshComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<RenderMeshComponent, IRenderMeshComponentManager>("RenderMeshComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderMeshComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RenderMeshComponent, IRenderMeshComponentManager>("RenderMeshComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRSDZModelIdComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<RSDZModelIdComponent, IRSDZModelIdComponentManager>("RSDZModelIdComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRSDZModelIdComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<RSDZModelIdComponent, IRSDZModelIdComponentManager>("RSDZModelIdComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<SkinComponent, ISkinComponentManager>("SkinComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<SkinComponent, ISkinComponentManager>("SkinComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinIbmComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<SkinIbmComponent, ISkinIbmComponentManager>("SkinIbmComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinIbmComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<SkinIbmComponent, ISkinIbmComponentManager>("SkinIbmComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinJointsComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<SkinJointsComponent, ISkinJointsComponentManager>("SkinJointsComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinJointsComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<SkinJointsComponent, ISkinJointsComponentManager>("SkinJointsComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsTransformComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<TransformComponent, ITransformComponentManager>("TransformComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsTransformComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<TransformComponent, ITransformComponentManager>("TransformComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsUriComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<UriComponent, IUriComponentManager>("UriComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsUriComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<UriComponent, IUriComponentManager>("UriComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWaterRippleComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<WaterRippleComponent, IWaterRippleComponentManager>("WaterRippleComponent");
}

/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWaterRippleComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<WaterRippleComponent, IWaterRippleComponentManager>("WaterRippleComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWeatherComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<WeatherComponent, IWeatherComponentManager>("WeatherComponent");
}

/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for IProperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWeatherComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<WeatherComponent, IWeatherComponentManager>("WeatherComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWorldMatrixComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<WorldMatrixComponent, IWorldMatrixComponentManager>("WorldMatrixComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWorldMatrixComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<WorldMatrixComponent, IWorldMatrixComponentManager>("WorldMatrixComponent");
}