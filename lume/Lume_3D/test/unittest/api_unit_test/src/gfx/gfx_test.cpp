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

#include <array>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/fog_component.h>
#include <3d/ecs/components/graphics_state_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/post_process_configuration_component.h>
#include <3d/ecs/components/post_process_effect_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_batch_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/unique_ptr.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/property_handle_util.h>
#include <core/property_tools/core_metadata.inl>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/gles/intf_device_gles.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>
#include <render/util/performance_data_structures.h>
#include <render/vulkan/intf_device_vk.h>

#include "gfx_cloud_test.h"
#include "gfx_culling_test.h"
#include "gfx_hello_world_boilerplate.h"
#include "test_framework.h"

// glTF test
#include <vulkan/vulkan.h>

#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_mesh_util.h>

#include "gfx/gfx_common.h"
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

void CheckRenderNodeCounters(
    const IPerformanceDataManager::IPerformanceDataManager::ConstCounterPairView defaultCounters)
{
    vector<pair<string_view, int64_t>> countList(RenderPerformanceDataConstants::BACKEND_COUNT_STRING_LIST,
        RenderPerformanceDataConstants::BACKEND_COUNT_STRING_LIST +
            countof(RenderPerformanceDataConstants::BACKEND_COUNT_STRING_LIST));
    HelloWorldBoilerplate::GetPerformanceDataCounters(
        RenderPerformanceDataConstants::RENDER_NODE_COUNTERS_NAME, countList);
    // compare
    IPerformanceDataManager::ComparisonData cd = HelloWorldBoilerplate::ComparePerformanceDataCounters(
        RenderPerformanceDataConstants::RENDER_NODE_COUNTERS_NAME, defaultCounters, countList);
    if (cd.flags != 0U) {
        for (const auto& ref : cd.errorCounters) {
            ADD_FAILURE() << "COUNTER COMPARISON ERROR: " << ref.first.c_str() << " DIFF: " << ref.second;
        }
    }
}
} // namespace

/**
 * @tc.name: HelloWorldTestVulkan
 * @tc.desc: Tests for Hello World Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, HelloWorldTestVulkan, testing::ext::TestSize.Level1)
{
    // generates cubes and in addition sorts them with material renderSort
    UTest::TestResources res(360u, 260u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    HelloWorldBoilerplate::HelloWorldTest(res);
    res.TickTest(10);

    if (res.GetByteArray()) {
        const BASE_NS::string appDIr = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDIr + "/hello.png").c_str(), res.GetWindowWidth(), res.GetWindowHeight(), 4,
            res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    // evaluate performance data counters
    static constexpr IPerformanceDataManager::CounterPair defaultCounts[] {
        { RenderPerformanceDataConstants::BACKEND_COUNT_TRIANGLE, 279 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_INSTANCECOUNT, 10 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAW, 10 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAWINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCH, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCHINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDPIPELINE, 5 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_RENDERPASS, 3 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_UPDATEDESCRIPTORSET, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDDESCRIPTORSET, 11 },
    };
    IPerformanceDataManager::ConstCounterPairView defaultCountsVec(
        defaultCounts, defaultCounts + countof(defaultCounts));
    CheckRenderNodeCounters(defaultCountsVec);

    res.ShutdownTest();
}

/**
 * @tc.name: CloudTestVulkan
 * @tc.desc: Tests for Clouds rendering. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, CloudTestVulkan, testing::ext::TestSize.Level1)
{
    // Run with clouds and sky enabled
    UTest::TestResources res(720u, 520u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    Cloud::CloudTest(res);
    Cloud::TickTest(res, 10u);

    // evaluate performance data counters
    static constexpr IPerformanceDataManager::CounterPair defaultCounts[] {
        { RenderPerformanceDataConstants::BACKEND_COUNT_TRIANGLE, 370299 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_INSTANCECOUNT, 152 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAW, 152 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAWINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCH, 4 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCHINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDPIPELINE, 25 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_RENDERPASS, 31 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_UPDATEDESCRIPTORSET, 4 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDDESCRIPTORSET, 158 },
    };
    IPerformanceDataManager::ConstCounterPairView defaultCountsVec(
        defaultCounts, defaultCounts + countof(defaultCounts));
    CheckRenderNodeCounters(defaultCountsVec);

    res.ShutdownTest();
}

/**
 * @tc.name: CloudTestOpenGL
 * @tc.desc: Tests for Clouds rendering. Disabled at the moment as some of the required shader don't support GL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, DISABLED_CloudTestOpenGL, testing::ext::TestSize.Level1)
{
    // Run with clouds and sky enabled
    UTest::TestResources res(720u, 520u, UTest::GetOpenGLBackend());
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    Cloud::CloudTest(res);
    Cloud::TickTest(res, 10u);

    // evaluate performance data counters
    static constexpr IPerformanceDataManager::CounterPair defaultCounts[] {
        { RenderPerformanceDataConstants::BACKEND_COUNT_TRIANGLE, 370299 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_INSTANCECOUNT, 152 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAW, 152 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAWINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCH, 4 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCHINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDPIPELINE, 25 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_RENDERPASS, 31 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_UPDATEDESCRIPTORSET, 4 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDDESCRIPTORSET, 158 },
    };
    IPerformanceDataManager::ConstCounterPairView defaultCountsVec(
        defaultCounts, defaultCounts + countof(defaultCounts));
    CheckRenderNodeCounters(defaultCountsVec);

    res.ShutdownTest();
}

/**
 * @tc.name: CullingTestVulkan
 * @tc.desc: Tests for Culling. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, CullingTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(720u, 520u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    Culling::CullingTest(res);
    res.TickTestAutoRng(1);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/3d_culling.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    // evaluate performance data counters
    static constexpr IPerformanceDataManager::CounterPair defaultCounts[] {
        { RenderPerformanceDataConstants::BACKEND_COUNT_TRIANGLE, 469290 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_INSTANCECOUNT, 78 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAW, 78 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAWINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCH, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCHINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDPIPELINE, 9 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_RENDERPASS, 15 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_UPDATEDESCRIPTORSET, 15 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDDESCRIPTORSET, 78 },
    };
    IPerformanceDataManager::ConstCounterPairView defaultCountsVec(
        defaultCounts, defaultCounts + countof(defaultCounts));
    CheckRenderNodeCounters(defaultCountsVec);

    res.ShutdownTest();
}

#ifndef __OHOS_PLATFORM__
/**
 * @tc.name: PerformanceCountersEnabledTest
 * @tc.desc: Tests for checking if performance counters are enabled. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, PerformanceCountersEnabledTest, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(CORE_PERF_ENABLED);
    EXPECT_TRUE(RENDER_PERF_ENABLED);
}
#endif // __OHOS_PLATFORM__

namespace {
enum class PostProcessType { POST_PROCESS_COMPONENT, POST_PROCESS_EFFECT_COMPONENT };
/*  Similar test case as we have the gltf viewer application
but in this case we do not have any control over model but
render only n number of frames of it for validation.
*/
void glTFmodelDrawTest(UTest::TestResources& res, const PostProcessType postProcessType)
{
    // glTF scene and model loading
    const vector<UTest::GltfImportInfo> gltfFiles {
        { "test://gltf/Environment/glTF/Environment.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
        { "test://gltf/FlightHelmet/FlightHelmet.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
    };

    auto& ecs = res.GetEcs();

    IGltf2& gltf2_ = res.GetGraphicsContext().GetGltf();

    Entity sceneEntity;
    // env
    {
        auto gltf = gltf2_.LoadGLTF(gltfFiles[0].filename);
        size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
        if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
            sceneIndex = 0;
        }
        auto importer = gltf2_.CreateGLTF2Importer(ecs);
        importer->ImportGLTF(*gltf.data, gltfFiles[0].resourceImportFlags);
        const auto& gltfImportResult = importer->GetResult();
        res.AppendResources(gltfImportResult.data);
        sceneEntity = gltf2_.ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, gltfFiles[0u].sceneImportFlags);
    }
    // fog
    Entity fogEntity;
    {
        fogEntity = ecs.GetEntityManager().Create();
        if (auto* fogMgr = GetManager<IFogComponentManager>(ecs); fogMgr) {
            fogMgr->Create(fogEntity);
            if (auto fogHandle = fogMgr->Write(fogEntity); fogHandle) {
                fogHandle->startDistance = 0.05f;
            }
        }
    }

    // model
    auto gltf = gltf2_.LoadGLTF(gltfFiles[1u].filename);
    auto importer = gltf2_.CreateGLTF2Importer(ecs);
    importer->ImportGLTF(*gltf.data, gltfFiles[1u].resourceImportFlags);
    const auto& gltfImportResult = importer->GetResult();
    res.AppendResources(gltfImportResult.data);
    size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
    if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
        sceneIndex = 0;
    }

    Entity importedSceneEntity;
    if (sceneIndex != CORE_GLTF_INVALID_INDEX) {
        importedSceneEntity = gltf2_.ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, gltfFiles[1u].sceneImportFlags);
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    RenderConfigurationComponent renderConfig = renderConfigMgr->Get(sceneEntity);
    auto nodeSystem = GetSystem<INodeSystem>(ecs);

    ISceneNode* model = nodeSystem->GetNode(importedSceneEntity);
    Math::Quat rot = Math::FromEulerRad(Math::Vec3 { 0.f, 0.f, 0.f });
    model->SetPosition(Math::Vec3 { 0.0f, 0.1f, 0.0f });
    model->SetRotation(rot);
    switch (postProcessType) {
        case PostProcessType::POST_PROCESS_COMPONENT: {
            auto postprocessMgr = GetManager<IPostProcessComponentManager>(ecs);
            postprocessMgr->Create(sceneEntity);
            if (auto handle = postprocessMgr->Write(sceneEntity)) {
                handle->enableFlags =
                    PostProcessComponent::FlagBits::TONEMAP_BIT | PostProcessComponent::FlagBits::TAA_BIT;
            }
            break;
        }
        case PostProcessType::POST_PROCESS_EFFECT_COMPONENT: {
            auto* renderClassFactory = res.GetRenderContext().GetInterface<CORE_NS::IClassFactory>();
            if (renderClassFactory) {
                auto combined = CreateInstance<IRenderPostProcess>(
                    *renderClassFactory, RENDER_NS::UID_RENDER_POST_PROCESS_COMBINED);
                CORE_NS::SetPropertyValue(combined->GetProperties(), "enabled", true);
                CORE_NS::SetPropertyValue(combined->GetProperties(), "postProcessConfiguration.enableFlags",
                    uint32_t(PostProcessConfiguration::ENABLE_TONEMAP_BIT));
                auto taa =
                    CreateInstance<IRenderPostProcess>(*renderClassFactory, RENDER_NS::UID_RENDER_POST_PROCESS_TAA);
                CORE_NS::SetPropertyValue(taa->GetProperties(), "enabled", true);
                CORE_NS::SetPropertyValue(taa->GetProperties(), "postProcessConfiguration.enableFlags",
                    uint32_t(PostProcessConfiguration::ENABLE_TONEMAP_BIT));

                auto postprocessEffectMgr = GetManager<IPostProcessEffectComponentManager>(ecs);
                postprocessEffectMgr->Create(sceneEntity);
                if (auto handle = postprocessEffectMgr->Write(sceneEntity)) {
                    handle->effects.push_back(taa);
                    handle->effects.push_back(combined);
                }
            }
            break;
        }
        default:
            break;
    }
    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.0f, 0.8f), {}, 0.1f, 100.0f, 60.0f);
    if (auto cameraHandle = cameraMgr->Write(cameraEntity); cameraHandle) {
        cameraHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        cameraHandle->pipelineFlags |=
            CameraComponent::PipelineFlagBits::HISTORY_BIT | CameraComponent::PipelineFlagBits::DEPTH_OUTPUT_BIT |
            CameraComponent::PipelineFlagBits::JITTER_BIT | CameraComponent::PipelineFlagBits::VELOCITY_OUTPUT_BIT;
        cameraHandle->postProcess = sceneEntity;
    }
    sceneUtil.UpdateCameraViewport(ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() });

    renderConfig.environment = sceneEntity;
    renderConfig.renderingFlags |= RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
    renderConfig.fog = fogEntity;

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.96f, 1.0f, 0.98f };
        lc.intensity = 2.8f;
        lc.shadowEnabled = true;
        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
    }

    renderConfigMgr->Set(sceneEntity, renderConfig);
}
} // namespace

/**
 * @tc.name: glTFmodelDrawTestVulkan
 * @tc.desc: Tests for Gl Tfmodel Draw Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, glTFmodelDrawTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    glTFmodelDrawTest(res, PostProcessType::POST_PROCESS_COMPONENT);
    res.TickTest(10);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/gltf.png").c_str(), res.GetWindowWidth(), res.GetWindowHeight(), 4,
            res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

/**
 * @tc.name: glTFmodelDrawBindlessTestVulkan
 * @tc.desc: Tests for glTF model loading and drawing with bindless.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, glTFmodelDrawBindlessTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);

    auto testConfig = UTest::CreateTestConfig();
    testConfig->enableBindless = true;

    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    glTFmodelDrawTest(res, PostProcessType::POST_PROCESS_EFFECT_COMPONENT);
    res.TickTest(10);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/gltfBindless.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

/**
 * @tc.name: glTFmodelDrawTestOpenGL
 * @tc.desc: Tests for Gl Tfmodel Draw Test Open Gl. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, glTFmodelDrawTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, UTest::GetOpenGLBackend());
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    glTFmodelDrawTest(res, PostProcessType::POST_PROCESS_COMPONENT);
    res.TickTest(10);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/gltfOpenGL.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

namespace {
void glTFmodelDrawTest2(UTest::TestResources& res)
{
    // glTF scene and model loading
    const vector<UTest::GltfImportInfo> gltfFiles {
        { "test://gltf/Environment/glTF/Environment.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
        { "test://gltf/TransmissionTest/TransmissionTest.glb", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
    };

    auto& ecs = res.GetEcs();

    IGltf2& gltf2_ = res.GetGraphicsContext().GetGltf();

    Entity sceneEntity;
    // env
    {
        auto gltf = gltf2_.LoadGLTF(gltfFiles[0].filename);
        size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
        if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
            sceneIndex = 0;
        }
        auto importer = gltf2_.CreateGLTF2Importer(ecs);
        importer->ImportGLTF(*gltf.data, gltfFiles[0].resourceImportFlags);
        const auto& gltfImportResult = importer->GetResult();
        res.AppendResources(gltfImportResult.data);
        sceneEntity = gltf2_.ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, gltfFiles[0u].sceneImportFlags);
    }

    // model
    auto gltf = gltf2_.LoadGLTF(gltfFiles[1u].filename);
    auto importer = gltf2_.CreateGLTF2Importer(ecs);
    importer->ImportGLTF(*gltf.data, gltfFiles[1u].resourceImportFlags);
    const auto& gltfImportResult = importer->GetResult();
    res.AppendResources(gltfImportResult.data);
    size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
    if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
        sceneIndex = 0;
    }

    Entity importedSceneEntity;
    if (sceneIndex != CORE_GLTF_INVALID_INDEX) {
        importedSceneEntity = gltf2_.ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, gltfFiles[1u].sceneImportFlags);
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    auto postProcessManager = GetManager<IPostProcessComponentManager>(ecs);
    RenderConfigurationComponent renderConfig = renderConfigMgr->Get(sceneEntity);
    renderConfig.renderingFlags = RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;

    auto nodeSystem = GetSystem<INodeSystem>(ecs);

    ISceneNode* model = nodeSystem->GetNode(importedSceneEntity);
    Math::Quat rot = Math::FromEulerRad(Math::Vec3 { 0.f, 0.f, 0.f });
    model->SetPosition(Math::Vec3 { 0.0f, 0.1f, 0.0f });
    model->SetRotation(rot);

    // post process component
    Entity postProcessEntity = ecs.GetEntityManager().Create();
    {
        postProcessManager->Create(postProcessEntity);
        if (auto postProcessHandle = postProcessManager->Write(postProcessEntity); postProcessHandle) {
            PostProcessComponent& postProcess = *postProcessHandle;
            postProcess.enableFlags = PostProcessConfiguration::ENABLE_TONEMAP_BIT;
        }
    }

    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.0f, 0.8f), {}, 0.1f, 100.0f, 60.0f);
    sceneUtil.UpdateCameraViewport(ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() });
    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity); camHandle) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT | CameraComponent::PipelineFlagBits::MSAA_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::LIGHT_FORWARD;
        camHandle->postProcess = postProcessEntity;
        // use frustum camera with zero offset
        camHandle->projection = CameraComponent::Projection::FRUSTUM;
        camHandle->xMag = 0.0f;
        camHandle->yMag = 0.0f;
    }
    renderConfig.environment = sceneEntity;

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.96f, 1.0f, 0.98f };
        lc.intensity = 2.8f;
        lc.shadowEnabled = true;
        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
    }

    renderConfigMgr->Set(sceneEntity, renderConfig);
}
} // namespace

/**
 * @tc.name: glTFmodelDrawTest2Vulkan
 * @tc.desc: Tests for Gl Tfmodel Draw Test2Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest2, glTFmodelDrawTest2Vulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    glTFmodelDrawTest2(res);
    res.TickTestAutoRng(7);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/gltf2.png").c_str(), res.GetWindowWidth(), res.GetWindowHeight(), 4,
            res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

/**
 * @tc.name: glTFmodelDrawTest2OpenGL
 * @tc.desc: Tests for Gl Tfmodel Draw Test2Open Gl. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest2, glTFmodelDrawTest2OpenGL, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, UTest::GetOpenGLBackend());
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    glTFmodelDrawTest2(res);
    res.TickTestAutoRng(7);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/gltf2OpenGL.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

namespace {
void glTFmodelDrawTest3(UTest::TestResources& res)
{
    // glTF scene and model loading
    const vector<UTest::GltfImportInfo> gltfFiles {
        { "test://gltf/Environment/glTF/Environment.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
        { "test://gltf/TextureTransformMultiTest/TextureTransformMultiTest.glb",
            UTest::GltfImportInfo::AnimateImportedScene, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL,
            CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
    };

    auto& ecs = res.GetEcs();

    IGltf2& gltf2_ = res.GetGraphicsContext().GetGltf();

    Entity sceneEntity;
    // env
    {
        auto gltf = gltf2_.LoadGLTF(gltfFiles[0].filename);
        size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
        if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
            sceneIndex = 0;
        }
        auto importer = gltf2_.CreateGLTF2Importer(ecs);
        importer->ImportGLTF(*gltf.data, gltfFiles[0].resourceImportFlags);
        const auto& gltfImportResult = importer->GetResult();
        res.AppendResources(gltfImportResult.data);
        sceneEntity = gltf2_.ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, gltfFiles[0u].sceneImportFlags);
    }

    // model
    auto gltf = gltf2_.LoadGLTF(gltfFiles[1u].filename);
    auto importer = gltf2_.CreateGLTF2Importer(ecs);
    importer->ImportGLTF(*gltf.data, gltfFiles[1u].resourceImportFlags);
    const auto& gltfImportResult = importer->GetResult();
    res.AppendResources(gltfImportResult.data);
    size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
    if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
        sceneIndex = 0;
    }

    Entity importedSceneEntity;
    if (sceneIndex != CORE_GLTF_INVALID_INDEX) {
        importedSceneEntity = gltf2_.ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, gltfFiles[1u].sceneImportFlags);
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    auto postProcessManager = GetManager<IPostProcessComponentManager>(ecs);
    RenderConfigurationComponent renderConfig = renderConfigMgr->Get(sceneEntity);
    renderConfig.renderingFlags = RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
    renderConfig.shadowType = RenderConfigurationComponent::SceneShadowType::VSM;

    auto nodeSystem = GetSystem<INodeSystem>(ecs);

    ISceneNode* model = nodeSystem->GetNode(importedSceneEntity);
    Math::Quat rot = Math::FromEulerRad(Math::Vec3 { 0.f, 0.f, 0.f });
    model->SetPosition(Math::Vec3 { 0.0f, 0.1f, 0.0f });
    model->SetRotation(rot);

    // post process component
    Entity postProcessEntity = ecs.GetEntityManager().Create();
    {
        postProcessManager->Create(postProcessEntity);
        if (auto postProcessHandle = postProcessManager->Write(postProcessEntity); postProcessHandle) {
            PostProcessComponent& postProcess = *postProcessHandle;
            postProcess.enableFlags = PostProcessConfiguration::ENABLE_TONEMAP_BIT;
        }
    }

    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 0.1f, 100.0f, 75.0f);
    sceneUtil.UpdateCameraViewport(ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() });
    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity); camHandle) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT |
                CameraComponent::PipelineFlagBits::HISTORY_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::DEFERRED;
        camHandle->postProcess = postProcessEntity;
    }
    renderConfig.environment = sceneEntity;

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.5f, 1.0f, 8.0f };
        lc.intensity = 4.0f;
        lc.shadowEnabled = true;

        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
    }

    renderConfigMgr->Set(sceneEntity, renderConfig);

    // ground plane
    {
        auto scene = nodeSystem->GetNode(sceneEntity);
        {
            auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();
            auto node = nodeSystem->GetNode(meshUtil.GeneratePlane(ecs, "plane", {}, 5.0f, 5.0f));
            node->SetPosition({ 0.0f, -2.0f, 0.0f });
            node->SetParent(*scene);
        }
    }
}
} // namespace

/**
 * @tc.name: glTFmodelDrawTest3Vulkan
 * @tc.desc: Tests for Gl Tfmodel Draw Test3Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest3, glTFmodelDrawTest3Vulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    glTFmodelDrawTest3(res);
    res.TickTestAutoRng(7);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/gltf3.png").c_str(), res.GetWindowWidth(), res.GetWindowHeight(), 4,
            res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

/**
 * @tc.name: Random
 * @tc.desc: Tests for Random. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, Random, testing::ext::TestSize.Level1) {}

namespace {
void NearFarClipTest(UTest::TestResources& res, CameraComponent::Projection projection)
{
    auto& ecs = res.GetEcs();

    const Entity sceneEntity = ecs.GetEntityManager().Create();

    // Create default components for scene.
    ITransformComponentManager& transformManager = *(GetManager<ITransformComponentManager>(ecs));
    transformManager.Create(sceneEntity);
    ILocalMatrixComponentManager& localMatrixManager = *(GetManager<ILocalMatrixComponentManager>(ecs));
    localMatrixManager.Create(sceneEntity);
    IWorldMatrixComponentManager& worldMatrixManager = *(GetManager<IWorldMatrixComponentManager>(ecs));
    worldMatrixManager.Create(sceneEntity);

    INodeComponentManager& nodeManager = *(GetManager<INodeComponentManager>(ecs));
    nodeManager.Create(sceneEntity);

    // Add name.
    {
        INameComponentManager& nameManager = *(GetManager<INameComponentManager>(ecs));
        nameManager.Create(sceneEntity);
        ScopedHandle<NameComponent> nameHandle = nameManager.Write(sceneEntity);
        nameHandle->name = "NearFarClipScene";
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    auto postProcessManager = GetManager<IPostProcessComponentManager>(ecs);
    RenderConfigurationComponent renderConfig = renderConfigMgr->Get(sceneEntity);
    renderConfig.renderingFlags = RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
    renderConfig.shadowType = RenderConfigurationComponent::SceneShadowType::VSM;

    auto nodeSystem = GetSystem<INodeSystem>(ecs);

    // post process component
    Entity postProcessEntity = ecs.GetEntityManager().Create();
    {
        postProcessManager->Create(postProcessEntity);
        if (auto postProcessHandle = postProcessManager->Write(postProcessEntity); postProcessHandle) {
            PostProcessComponent& postProcess = *postProcessHandle;
            postProcess.enableFlags = PostProcessConfiguration::ENABLE_TONEMAP_BIT;
        }
    }

    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 75.0f, 10.0f);
    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity); camHandle) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT |
                CameraComponent::PipelineFlagBits::HISTORY_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::DEFERRED;
        camHandle->postProcess = postProcessEntity;
        camHandle->projection = projection;
    }
    renderConfig.environment = sceneEntity;

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.5f, 1.0f, 8.0f };
        lc.intensity = 4.0f;
        lc.shadowEnabled = true;

        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
    }

    renderConfigMgr->Set(sceneEntity, renderConfig);

    // Scene meshes
    {
        auto scene = nodeSystem->GetNode(sceneEntity);
        auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();
        {
            // Red cube is partially visible because of far plane clip
            auto node = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "cube0",
                UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 0.0f, 0.0f, 1.0f }), 1.0f, 1.0f, 1.0f));
            node->SetPosition({ -3.0f, -2.0f, -96.5f });
            node->SetRotation(Math::FromEulerRad(Math::Vec3 { 1.4f, 5.2f, 0.14f }));
            node->SetParent(*scene);
        }
        {
            // Green cube is visible
            auto node = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "Green cube",
                UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 0.0f, 1.0f, 0.0f, 1.0f }), 1.0f, 1.0f, 1.0f));
            node->SetPosition({ -1.0f, -2.0f, -5.0f });
            node->SetRotation(Math::FromEulerRad(Math::Vec3 { 1.4f, 5.2f, 0.14f }));
            node->SetParent(*scene);
        }
        {
            // Blue cube is not visible because of near plane clip
            auto node = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "Blue cube",
                UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 0.0f, 0.0f, 1.0f, 1.0f }), 1.0f, 1.0f, 1.0f));
            node->SetPosition({ -1.0f, 1.0f, 3.0f });
            node->SetRotation(Math::FromEulerRad(Math::Vec3 { 1.4f, 5.2f, 0.14f }));
            node->SetParent(*scene);
        }
        {
            // Cone is visible
            auto node = nodeSystem->GetNode(meshUtil.GenerateCone(ecs, "Cone",
                UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 1.0f, 1.0f, 1.0f }), 1.0f, 2.0f, 16));
            node->SetPosition({ 1.0f, 0.0f, -4.0f });
            node->SetRotation(Math::FromEulerRad(Math::Vec3 { 1.4f, 5.2f, 0.14f }));
            node->SetParent(*scene);
        }
        {
            // Torus is visible
            auto node = nodeSystem->GetNode(meshUtil.GenerateTorus(ecs, "Torus",
                UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 1.0f, 0.0f, 1.0f }), 1.0f, 0.2f, 16, 16));
            node->SetPosition({ 1.0f, 3.0f, -4.0f });
            node->SetRotation(Math::FromEulerRad(Math::Vec3 { 1.4f, 5.2f, 0.14f }));
            node->SetParent(*scene);
        }
        {
            // Sphere is visible
            auto node = nodeSystem->GetNode(meshUtil.GenerateSphere(ecs, "Sphere",
                UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 0.0f, 1.0f, 1.0f }), 1.0f, 16, 16));
            node->SetPosition({ -1.0f, 3.0f, -4.0f });
            node->SetParent(*scene);
        }
        {
            // Cylinder is visible
            auto node = nodeSystem->GetNode(meshUtil.GenerateCylinder(ecs, "Cylinder",
                UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 0.0f, 1.0f, 1.0f, 1.0f }), 1.0f, 2.0f, 16));
            node->SetPosition({ -3.0f, -1.5f, -4.0f });
            node->SetParent(*scene);
        }
    }
}
} // namespace

/**
 * @tc.name: perspectiveNearFarClipTestVulkan
 * @tc.desc: Tests for Perspective Near Far Clip Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, perspectiveNearFarClipTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    NearFarClipTest(res, CameraComponent::Projection::PERSPECTIVE);
    res.TickTestAutoRng(1);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/perspectiveNearFarClipTestVulkan.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

/**
 * @tc.name: orthographicNearFarClipTestVulkan
 * @tc.desc: Tests for Orthographic Near Far Clip Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, orthographicNearFarClipTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    NearFarClipTest(res, CameraComponent::Projection::ORTHOGRAPHIC);
    res.TickTestAutoRng(1);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/orthographicNearFarClipTestVulkan.png").c_str(),
            res.GetWindowWidth(), res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(),
            res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

namespace {
void CreateBatch(IEcs& ecs, ISceneNode* node)
{
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
    auto renderMeshBatchManager = GetManager<IRenderMeshBatchComponentManager>(ecs);
    if (auto rmHandle = renderMeshManager->Write(node->GetEntity()); rmHandle) {
        rmHandle->renderMeshBatch = ecs.GetEntityManager().Create();
        renderMeshBatchManager->Create(rmHandle->renderMeshBatch);
        if (auto batchHandle = renderMeshBatchManager->Write(rmHandle->renderMeshBatch); batchHandle) {
            batchHandle->batchType = RenderMeshBatchComponent::BatchType::GPU_INSTANCING;
        }
    }
}

RenderHandleReference CreateGpuImage(IRenderContext& renderContext, bool isDepth, uint32_t width, uint32_t height)
{
    GpuImageDesc desc;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    desc.imageType = CORE_IMAGE_TYPE_2D;
    desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    desc.layerCount = 1;
    desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.mipCount = 1;
    desc.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
    if (isDepth) {
        desc.format = BASE_FORMAT_D24_UNORM_S8_UINT;
        desc.usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    } else {
        desc.format = BASE_FORMAT_R8G8B8A8_SRGB;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }
    return renderContext.GetDevice().GetGpuResourceManager().Create(desc);
}

EntityReference CreateCustomTarget(
    IRenderContext& renderContext, IEcs& ecs, bool isDepth, uint32_t width, uint32_t height)
{
    EntityReference entity = ecs.GetEntityManager().CreateReferenceCounted();
    auto rhManager = GetManager<IRenderHandleComponentManager>(ecs);
    rhManager->Create(entity);
    if (auto scopedHandle = rhManager->Write(entity); scopedHandle) {
        scopedHandle->reference = CreateGpuImage(renderContext, isDepth, width, height);
    }
    return entity;
}

EntityReference CreateCustomTarget(IEcs& ecs, RenderHandleReference image)
{
    EntityReference entity = ecs.GetEntityManager().CreateReferenceCounted();
    auto rhManager = GetManager<IRenderHandleComponentManager>(ecs);
    rhManager->Create(entity);
    if (auto scopedHandle = rhManager->Write(entity); scopedHandle) {
        scopedHandle->reference = image;
    }
    return entity;
}

EntityReference CreateShader(IEcs& ecs, IRenderContext& renderContext, string_view uri)
{
    auto uriManager = GetManager<IUriComponentManager>(ecs);
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    auto graphicsStateManager = GetManager<IGraphicsStateComponentManager>(ecs);
    auto& shaderMgr = renderContext.GetDevice().GetShaderManager();

    shaderMgr.LoadShaderFile(uri);
    auto shaderHandle = shaderMgr.GetShaderHandle(uri);
    EXPECT_TRUE(shaderHandle);

    auto stateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle);
    EXPECT_TRUE(stateHandle);
    Entity graphicsState = ecs.GetEntityManager().Create();
    graphicsStateManager->Create(graphicsState);
    if (auto gsHandle = graphicsStateManager->Write(graphicsState)) {
        gsHandle->graphicsState = shaderMgr.GetGraphicsState(stateHandle);
        gsHandle->renderSlot = shaderMgr.GetRenderSlotName(shaderMgr.GetRenderSlotId(shaderHandle));
    }

    EntityReference shader = ecs.GetEntityManager().CreateReferenceCounted();
    renderHandleManager->Create(shader);
    renderHandleManager->Write(shader)->reference = BASE_NS::move(shaderHandle);
    uriManager->Create(shader);
    uriManager->Write(shader)->uri = uri;
    return shader;
}

void CubeBatchTest(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();

    const Entity sceneEntity = ecs.GetEntityManager().Create();

    // Create default components for scene.
    ITransformComponentManager& transformManager = *(GetManager<ITransformComponentManager>(ecs));
    transformManager.Create(sceneEntity);
    ILocalMatrixComponentManager& localMatrixManager = *(GetManager<ILocalMatrixComponentManager>(ecs));
    localMatrixManager.Create(sceneEntity);
    IWorldMatrixComponentManager& worldMatrixManager = *(GetManager<IWorldMatrixComponentManager>(ecs));
    worldMatrixManager.Create(sceneEntity);

    INodeComponentManager& nodeManager = *(GetManager<INodeComponentManager>(ecs));
    nodeManager.Create(sceneEntity);

    // Add name.
    {
        INameComponentManager& nameManager = *(GetManager<INameComponentManager>(ecs));
        nameManager.Create(sceneEntity);
        ScopedHandle<NameComponent> nameHandle = nameManager.Write(sceneEntity);
        nameHandle->name = "CubeBatchTest";
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    auto postProcessManager = GetManager<IPostProcessComponentManager>(ecs);
    RenderConfigurationComponent renderConfig = renderConfigMgr->Get(sceneEntity);
    renderConfig.renderingFlags = RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
    renderConfig.shadowType = RenderConfigurationComponent::SceneShadowType::VSM;

    auto nodeSystem = GetSystem<INodeSystem>(ecs);

    // post process component
    Entity postProcessEntity = ecs.GetEntityManager().Create();
    {
        postProcessManager->Create(postProcessEntity);
        if (auto postProcessHandle = postProcessManager->Write(postProcessEntity); postProcessHandle) {
            PostProcessComponent& postProcess = *postProcessHandle;
            postProcess.enableFlags = PostProcessConfiguration::ENABLE_TONEMAP_BIT;
        }
    }

    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 75.0f, 10.0f);
    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity); camHandle) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT |
                CameraComponent::PipelineFlagBits::HISTORY_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::DEFERRED;
        camHandle->postProcess = postProcessEntity;
        camHandle->projection = CameraComponent::Projection::PERSPECTIVE;

        camHandle->customDepthTarget =
            CreateCustomTarget(res.GetRenderContext(), ecs, true, res.GetWindowWidth(), res.GetWindowHeight());
        camHandle->customColorTargets.push_back(CreateCustomTarget(ecs, res.GetImage()));
    }
    renderConfig.environment = sceneEntity;

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.5f / 8.f, 1.0f / 8.f, 8.0f / 8.f };
        lc.intensity = 8.0f;
        lc.shadowEnabled = false;
        lc.lightLayerMask = LayerFlagBits::CORE_LAYER_FLAG_BIT_00;

        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));

        lc.type = LightComponent::Type::POINT;
        lc.color = { 1.0f, 0.9f, 0.0f };
        lc.intensity = 400.0f;
        lc.shadowEnabled = false;
        lc.lightLayerMask = LayerFlagBits::CORE_LAYER_FLAG_BIT_01;

        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 1.0f, -5.0f), Math::Quat(0.f, 0.f, 0.f, 1.f));
    }

    renderConfigMgr->Set(sceneEntity, renderConfig);

    // Scene meshes
    {
        auto scene = nodeSystem->GetNode(sceneEntity);
        auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();

        constexpr const uint32_t cubeGridWidth = 5u;
        constexpr const uint32_t cubeGridHeight = 5u;

        Entity material = UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 0.5f, 0.5f, 1.0f });
        auto materialManager = GetManager<IMaterialComponentManager>(res.GetEcs());
        if (auto matHandle = materialManager->Write(material); matHandle) {
            matHandle->materialShader.shader =
                CreateShader(res.GetEcs(), res.GetRenderContext(), "test://shader/test.shader");
            matHandle->extraRenderingFlags = MaterialComponent::ALLOW_GPU_INSTANCING_BIT;
        }

        auto cube = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "Cube", material, 1.0f, 1.0f, 1.0f));
        // Add RenderMeshBatch entity and mark cube to that batch
        CreateBatch(ecs, cube);

        auto renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
        // Clone cube multiple times
        for (uint32_t i = 0; i < cubeGridWidth; ++i) {
            for (uint32_t j = 0; j < cubeGridHeight; ++j) {
                auto node = nodeSystem->CloneNode(*cube, true);
                node->SetPosition(
                    { -3.0f + static_cast<float>(j) * 1.3f, -2.0f, -5.5f + static_cast<float>(i) * 1.3f });
                node->SetParent(*scene);

                if (i <= (cubeGridWidth / 2U)) {
                    // Remove some of the cubes from the batch so that automatic instancing should batch them.
                    renderMeshManager->Write(node->GetEntity())->renderMeshBatch = {};
                }

                if ((i == (cubeGridWidth / 2U)) && (j == (cubeGridHeight / 2U))) {
                    // Include one cude in two layers so that it will be lit by an additional point light.
                    auto layerManager = GetManager<ILayerComponentManager>(res.GetEcs());
                    layerManager->Create(node->GetEntity());
                    layerManager->Write(node->GetEntity())->layerMask =
                        LayerFlagBits::CORE_LAYER_FLAG_BIT_01 | LayerFlagBits::CORE_LAYER_FLAG_BIT_00;
                }
            }
        }
        // Hide template cube
        cube->SetEnabled(false);
    }
}
} // namespace

/**
 * @tc.name: cubeBatchTestVulkan
 * @tc.desc: Tests for Cube Batch Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, cubeBatchTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    CubeBatchTest(res);
    res.TickTestAutoRng(1);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/cubeBatchTestVulkan.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    // evaluate performance data counters
    static constexpr IPerformanceDataManager::CounterPair defaultCounts[] {
        { RenderPerformanceDataConstants::BACKEND_COUNT_TRIANGLE, 918 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_INSTANCECOUNT, 27 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAW, 3 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DRAWINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCH, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_DISPATCHINDIRECT, 0 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDPIPELINE, 3 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_RENDERPASS, 2 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_UPDATEDESCRIPTORSET, 4 },
        { RenderPerformanceDataConstants::BACKEND_COUNT_BINDDESCRIPTORSET, 3 },
    };
    IPerformanceDataManager::ConstCounterPairView defaultCountsVec(
        defaultCounts, defaultCounts + countof(defaultCounts));
    CheckRenderNodeCounters(defaultCountsVec);

    res.ShutdownTest();
}

namespace {
constexpr Math::Vec3 CUBE_POS[8u] = {
    Math::Vec3(-0.5f, -0.5f, -0.5f), //
    Math::Vec3(0.5f, -0.5f, -0.5f),  //
    Math::Vec3(0.5f, 0.5f, -0.5f),   //
    Math::Vec3(-0.5f, 0.5f, -0.5f),  //
    Math::Vec3(-0.5f, -0.5f, 0.5f),  //
    Math::Vec3(0.5f, -0.5f, 0.5f),   //
    Math::Vec3(0.5f, 0.5f, 0.5f),    //
    Math::Vec3(-0.5f, 0.5f, 0.5f)    //
};

constexpr Math::Vec2 CUBE_UV[4u] = { Math::Vec2(1.0f, 1.0f), Math::Vec2(0.0f, 1.0f), Math::Vec2(0.0f, 0.0f),
    Math::Vec2(1.0f, 0.0f) };

constexpr uint16_t CUBE_INDICES[6u * 6u] = {
    0, 3, 1, 3, 2, 1, //
    1, 2, 5, 2, 6, 5, //
    5, 6, 4, 6, 7, 4, //
    4, 7, 0, 7, 3, 0, //
    3, 7, 2, 7, 6, 2, //
    4, 0, 5, 0, 1, 5  //
};

constexpr uint32_t CUBE_UV_INDICES[6u] = { 0, 3, 1, 3, 2, 1 };

void GenerateCubeGeometry(vector<Math::Vec3>& vertices, vector<Math::Vec3>& normals, vector<Math::Vec2>& uvs,
    vector<uint16_t>& indices, bool displace)
{
    constexpr const float width = 3.5f;
    constexpr const float height = 3.5f;
    constexpr const float depth = 3.5f;

    vertices.reserve(countof(CUBE_INDICES));
    normals.reserve(countof(CUBE_INDICES));
    uvs.reserve(countof(CUBE_INDICES));
    indices.reserve(countof(CUBE_INDICES));

    constexpr size_t triangleCount = countof(CUBE_INDICES) / 3u;
    for (size_t i = 0; i < triangleCount; ++i) {
        const size_t vertexIndex = i * 3u;

        Math::Vec3 v0 = CUBE_POS[CUBE_INDICES[vertexIndex + 0u]];
        Math::Vec3 v1 = CUBE_POS[CUBE_INDICES[vertexIndex + 1u]];
        Math::Vec3 v2 = CUBE_POS[CUBE_INDICES[vertexIndex + 2u]];

        if (displace) {
            if (CUBE_INDICES[vertexIndex + 0u] == 6u) {
                v0 *= Math::Vec3 { 3.0f, 3.0f, 3.0f };
            }
            if (CUBE_INDICES[vertexIndex + 1u] == 6u) {
                v1 *= Math::Vec3 { 3.0f, 3.0f, 3.0f };
            }
            if (CUBE_INDICES[vertexIndex + 2u] == 6u) {
                v2 *= Math::Vec3 { 3.0f, 3.0f, 3.0f };
            }
        }

        vertices.emplace_back(v0.x * width, v0.y * height, v0.z * depth);
        vertices.emplace_back(v1.x * width, v1.y * height, v1.z * depth);
        vertices.emplace_back(v2.x * width, v2.y * height, v2.z * depth);

        const Math::Vec3 normal = Math::Normalize(Math::Cross((v1 - v0), (v2 - v0)));
        normals.append(3u, normal);

        uvs.emplace_back(
            CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 0u) % 6u]].x, CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 0u) % 6u]].y);
        uvs.emplace_back(
            CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 1u) % 6u]].x, CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 1u) % 6u]].y);
        uvs.emplace_back(
            CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 2u) % 6u]].x, CUBE_UV[CUBE_UV_INDICES[(vertexIndex + 2u) % 6u]].y);
    }

    for (uint16_t i = 0u; i < countof(CUBE_INDICES); ++i) {
        indices.push_back(i);
    }
}

void CalculateTangents(const array_view<const uint16_t>& indices, const array_view<const Math::Vec3>& positions,
    const array_view<const Math::Vec3>& normals, const array_view<const Math::Vec2>& uvs,
    array_view<Math::Vec4>& outTangents)
{
    // http://www.terathon.com/code/tangent.html
    vector<Math::Vec3> tan1;
    vector<Math::Vec3> tan2;
    tan1.resize(positions.size(), { 0, 0, 0 });
    tan2.resize(positions.size(), { 0, 0, 0 });

    for (size_t i = 0; i < indices.size(); i += 3u) {
        const uint16_t aa = indices[i + 0u];
        const uint16_t bb = indices[i + 1u];
        const uint16_t cc = indices[i + 2u];

        const Math::Vec3& v1 = positions[aa];
        const Math::Vec3& v2 = positions[bb];
        const Math::Vec3& v3 = positions[cc];

        const Math::Vec2 uv1 = uvs[aa];
        const Math::Vec2 uv2 = uvs[bb];
        const Math::Vec2 uv3 = uvs[cc];

        const float x1 = v2.x - v1.x;
        const float x2 = v3.x - v1.x;
        const float y1 = v2.y - v1.y;
        const float y2 = v3.y - v1.y;
        const float z1 = v2.z - v1.z;
        const float z2 = v3.z - v1.z;

        const float s1 = uv2.x - uv1.x;
        const float s2 = uv3.x - uv1.x;
        const float t1 = uv2.y - uv1.y;
        const float t2 = uv3.y - uv1.y;

        auto d = (s1 * t2 - s2 * t1);
        if (Math::abs(d) < Math::EPSILON) {
            d = Math::EPSILON;
        }
        const float r = 1.0f / d;
        const Math::Vec3 sdir { (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r };
        const Math::Vec3 tdir { (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r };

        tan1[aa] += sdir;
        tan1[bb] += sdir;
        tan1[cc] += sdir;

        tan2[aa] += tdir;
        tan2[bb] += tdir;
        tan2[cc] += tdir;
    }

    for (size_t i = 0; i < positions.size(); i++) {
        const Math::Vec3& n = normals[i];
        const Math::Vec3& t = tan1[i];

        // Gram-Schmidt orthogonalize
        const Math::Vec3 tmp = Math::Normalize(t - n * Math::Dot(n, t));

        // Calculate handedness
        const float w = (Math::Dot(Math::Cross(n, t), tan2[i]) < 0.0F) ? 1.0F : -1.0F;

        outTangents[i] = Math::Vec4(tmp.x, tmp.y, tmp.z, w);
    }
}

template<typename T>
constexpr inline IMeshBuilder::DataBuffer FillData(array_view<const T> c) noexcept
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
    return IMeshBuilder::DataBuffer { format, sizeof(T),
        { reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(T) } };
}

template<typename T>
constexpr inline IMeshBuilder::DataBuffer FillData(const vector<T>& c) noexcept
{
    return FillData(array_view<const T> { c });
}

template<typename T, size_t N>
constexpr inline IMeshBuilder::DataBuffer FillData(const T (&c)[N]) noexcept
{
    return FillData(array_view<const T> { c, N });
}

void MeshBuilderTest(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();

    const Entity sceneEntity = ecs.GetEntityManager().Create();

    // Create default components for scene.
    ITransformComponentManager& transformManager = *(GetManager<ITransformComponentManager>(ecs));
    transformManager.Create(sceneEntity);
    ILocalMatrixComponentManager& localMatrixManager = *(GetManager<ILocalMatrixComponentManager>(ecs));
    localMatrixManager.Create(sceneEntity);
    IWorldMatrixComponentManager& worldMatrixManager = *(GetManager<IWorldMatrixComponentManager>(ecs));
    worldMatrixManager.Create(sceneEntity);

    INodeComponentManager& nodeManager = *(GetManager<INodeComponentManager>(ecs));
    nodeManager.Create(sceneEntity);

    // Add name.
    {
        INameComponentManager& nameManager = *(GetManager<INameComponentManager>(ecs));
        nameManager.Create(sceneEntity);
        ScopedHandle<NameComponent> nameHandle = nameManager.Write(sceneEntity);
        nameHandle->name = "MeshBuilderTest";
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    auto postProcessConfManager = GetManager<IPostProcessConfigurationComponentManager>(ecs);
    RenderConfigurationComponent renderConfig = renderConfigMgr->Get(sceneEntity);
    renderConfig.renderingFlags = RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
    renderConfig.shadowType = RenderConfigurationComponent::SceneShadowType::VSM;
    renderConfig.customPostSceneRenderNodeGraphFile = "3drendernodegraphs://core3d_rng_cam_scene_post_process.rng";

    Entity postProcessConfEntity = ecs.GetEntityManager().Create();
    {
        postProcessConfManager->Create(postProcessConfEntity);
        if (auto postProcessHandle = postProcessConfManager->Write(postProcessConfEntity); postProcessHandle) {
            PostProcessConfigurationComponent& postProcess = *postProcessHandle;
            postProcess.postProcesses.resize(1);
            postProcess.postProcesses[0].enabled = true;
            postProcess.postProcesses[0].name = "conf";
            postProcess.postProcesses[0].shader =
                CreateShader(res.GetEcs(), res.GetRenderContext(), "test://shader/test.shader");
        }
    }

    auto fogManager = GetManager<IFogComponentManager>(res.GetEcs());
    Entity fogEntity = res.GetEcs().GetEntityManager().Create();
    fogManager->Create(fogEntity);
    if (auto fogHandle = fogManager->Write(fogEntity); fogHandle) {
        fogHandle->startDistance = 100.0f;
    }

    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 75.0f, 10.0f);
    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity); camHandle) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT |
                CameraComponent::PipelineFlagBits::HISTORY_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::DEFERRED;
        camHandle->postProcess = postProcessConfEntity;
        camHandle->projection = CameraComponent::Projection::CUSTOM;
        camHandle->customProjectionMatrix =
            Math::PerspectiveRhZo(camHandle->yFov, camHandle->aspect, camHandle->zNear, camHandle->zFar);
        camHandle->customProjectionMatrix[1][1] *= -1.f;
        camHandle->fog = fogEntity;
    }
    renderConfig.environment = sceneEntity;

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.5f, 1.0f, 8.0f };
        lc.intensity = 4.0f;
        lc.shadowEnabled = true;

        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
    }

    renderConfigMgr->Set(sceneEntity, renderConfig);

    // Scene meshes
    {
        auto meshBuilder = CreateInstance<IMeshBuilder>(res.GetRenderContext(), UID_MESH_BUILDER);

        meshBuilder->Allocate();

        IShaderManager& shaderManager = res.GetRenderContext().GetDevice().GetShaderManager();
        const VertexInputDeclarationView vertexInputDeclaration =
            shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
                DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));
        meshBuilder->Initialize(vertexInputDeclaration, 1);

        IMeshBuilder::Submesh submesh;
        submesh.morphTargetCount = 1u;
        submesh.material = UTest::CreateSolidColorMaterial(res.GetEcs(), Math::Vec4 { 0.0f, 1.0f, 0.0f, 1.0f });
        auto materialManager = GetManager<IMaterialComponentManager>(res.GetEcs());
        if (auto matHandle = materialManager->Write(submesh.material); matHandle) {
            matHandle->materialShader.shader =
                CreateShader(res.GetEcs(), res.GetRenderContext(), "test://shader/test.shader");
            matHandle->customResources.push_back(res.GetEcs().GetEntityManager().CreateReferenceCounted());
        }
        submesh.vertexCount = static_cast<uint32_t>(countof(CUBE_INDICES));
        submesh.indexCount = static_cast<uint32_t>(countof(CUBE_INDICES));
        submesh.indexType = CORE_INDEX_TYPE_UINT16;
        submesh.tangents = true;
        meshBuilder->AddSubmesh(submesh);
        meshBuilder->Allocate();

        vector<Math::Vec3> vertices;
        vector<Math::Vec3> normals;
        vector<Math::Vec2> uvs;
        vector<uint16_t> indices;
        GenerateCubeGeometry(vertices, normals, uvs, indices, false);
        vector<Math::Vec4> tangents(vertices.size());
        array_view<Math::Vec4> tangentsView { tangents };
        CalculateTangents(indices, vertices, normals, uvs, tangentsView);
        meshBuilder->SetVertexData(
            0u, FillData(vertices), FillData(normals), FillData(uvs), {}, FillData(tangents), {});
        meshBuilder->CalculateAABB(0u, FillData(vertices));
        meshBuilder->SetIndexData(0u, FillData(indices));

        vector<Math::Vec3> targetVertices;
        vector<Math::Vec3> targetNormals;
        vector<Math::Vec2> targetUvs;
        vector<uint16_t> targetIndices;
        GenerateCubeGeometry(targetVertices, targetNormals, targetUvs, targetIndices, true);
        vector<Math::Vec4> targetTangentsVec4(vertices.size());
        array_view<Math::Vec4> targetTangentsView { targetTangentsVec4 };
        CalculateTangents(targetIndices, targetVertices, targetNormals, targetUvs, targetTangentsView);
        vector<Math::Vec3> targetTangents;
        for (Math::Vec4 tangent : targetTangentsVec4) {
            targetTangents.push_back(Math::Vec3 { tangent });
        }

        meshBuilder->SetMorphTargetData(0u, FillData(vertices), FillData(normals), FillData(tangents),
            FillData(targetVertices), FillData(targetNormals), FillData(targetTangents));

        EXPECT_EQ(1u, meshBuilder->GetSubmeshes().size());
        EXPECT_EQ(0u, meshBuilder->GetJointBoundsData().size());
        EXPECT_LT(0u, meshBuilder->GetMorphTargetData().size());
        EXPECT_LT(0u, meshBuilder->GetIndexData().size());
        EXPECT_LT(0u, meshBuilder->GetVertexData().size());
        EXPECT_EQ(0u, meshBuilder->GetJointData().size());

        auto meshEntity = meshBuilder->CreateMesh(ecs);

        INodeSystem* nodesystem = GetSystem<INodeSystem>(ecs);

        // Create node to scene.
        ISceneNode* node = nodesystem->CreateNode();

        // Add render mesh component.
        IRenderMeshComponentManager* renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
        renderMeshManager->Create(node->GetEntity());
        if (auto scopedHandle = renderMeshManager->Write(node->GetEntity()); scopedHandle) {
            scopedHandle->mesh = meshEntity;
        }

        node->SetPosition({ -1.0f, -1.0f, -5.5f });
        node->SetRotation(Math::FromEulerRad(Math::Vec3 { Math::DEG2RAD * 20.0f, Math::DEG2RAD * 30.0f, 0.0f }));

        IMorphComponentManager* morphManager = GetManager<IMorphComponentManager>(ecs);
        morphManager->Create(node->GetEntity());
        if (auto scopedHandle = morphManager->Write(node->GetEntity()); scopedHandle) {
            scopedHandle->morphNames.push_back("morph");
            scopedHandle->morphWeights.push_back(0.5f);
        }

        // Create two quads, first with triangle list and then with triangle strip
        constexpr Math::Vec3 quadVertices[] = {
            { 0.f, 0.f, 0.f },
            { 1.f, 0.f, 0.f },
            { 0.f, 1.f, 0.f },
            { 1.f, 1.f, 0.f },
        };
        constexpr Math::Vec2 quadUvs[] = {
            { 0.f, 0.f },
            { 1.f, 0.f },
            { 0.f, 1.f },
            { 1.f, 1.f },
        };
        constexpr uint16_t quadIndicesTriangleList[] = { 0U, 1U, 2U, 1U, 3U, 2U };
        constexpr uint16_t quadIndicesTriangleStrip[] = { 0U, 1U, 2U, 3U };

        meshBuilder->Initialize(vertexInputDeclaration, 1U);
        submesh.vertexCount = BASE_NS::countof(quadVertices);
        submesh.indexCount = BASE_NS::countof(quadIndicesTriangleList);
        submesh.morphTargetCount = 0U;
        submesh.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        meshBuilder->AddSubmesh(submesh);
        meshBuilder->Allocate();
        meshBuilder->SetVertexData(0u, FillData(quadVertices), {}, FillData(quadUvs), {}, {}, {});
        meshBuilder->CalculateAABB(0u, FillData(quadVertices));
        meshBuilder->SetIndexData(0u, FillData(quadIndicesTriangleList));

        auto quadMeshEntity = meshBuilder->CreateMesh(ecs);
        node = nodesystem->CreateNode();
        renderMeshManager->Create(node->GetEntity());
        if (auto scopedHandle = renderMeshManager->Write(node->GetEntity()); scopedHandle) {
            scopedHandle->mesh = quadMeshEntity;
        }
        node->SetPosition({ -3.0f, 1.0f, -0.5f });

        meshBuilder->Initialize(vertexInputDeclaration, 1U);
        submesh.vertexCount = BASE_NS::countof(quadVertices);
        submesh.indexCount = BASE_NS::countof(quadIndicesTriangleStrip);
        submesh.morphTargetCount = 0U;
        submesh.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        // Triangle strip requires a different graphics states so create a new material with correct values
        submesh.material = UTest::CreateSolidColorMaterial(res.GetEcs(), Math::Vec4 { 0.0f, 1.0f, 0.0f, 1.0f });

        // different states for material and depth shader
        auto materialGraphicsStateEntity = res.GetEcs().GetEntityManager().CreateReferenceCounted();
        auto depthGraphicsStateEntity = res.GetEcs().GetEntityManager().CreateReferenceCounted();
        auto stateManager = GetManager<IGraphicsStateComponentManager>(ecs);
        stateManager->Create(materialGraphicsStateEntity);
        stateManager->Create(depthGraphicsStateEntity);

        if (auto matHandle = materialManager->Write(submesh.material); matHandle) {
            matHandle->materialShader.shader =
                CreateShader(res.GetEcs(), res.GetRenderContext(), "test://shader/test.shader");
            matHandle->materialShader.graphicsState = materialGraphicsStateEntity;
            matHandle->depthShader.graphicsState = depthGraphicsStateEntity;
            matHandle->customResources.push_back(res.GetEcs().GetEntityManager().CreateReferenceCounted());
        }

        {
            auto shader = shaderManager.GetShaderHandle("test://shader/test.shader");
            auto slotId = shaderManager.GetRenderSlotId(shader);
            auto slotData = shaderManager.GetRenderSlotData(slotId);
            if (auto stateHandle = stateManager->Write(materialGraphicsStateEntity)) {
                stateHandle->graphicsState = shaderManager.GetGraphicsState(slotData.graphicsState);
                stateHandle->renderSlot = shaderManager.GetRenderSlotName(slotId);
                stateHandle->graphicsState.inputAssembly.primitiveTopology =
                    PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            }
        }
        {
            auto slotId = shaderManager.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH);
            auto slotData = shaderManager.GetRenderSlotData(slotId);
            if (auto stateHandle = stateManager->Write(depthGraphicsStateEntity)) {
                stateHandle->graphicsState = shaderManager.GetGraphicsState(slotData.graphicsState);
                stateHandle->renderSlot = shaderManager.GetRenderSlotName(slotId);
                stateHandle->graphicsState.inputAssembly.primitiveTopology =
                    PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            }
        }

        meshBuilder->AddSubmesh(submesh);
        meshBuilder->Allocate();
        meshBuilder->SetVertexData(0u, FillData(quadVertices), {}, FillData(quadUvs), {}, {}, {});
        meshBuilder->CalculateAABB(0u, FillData(quadVertices));
        meshBuilder->SetIndexData(0u, FillData(quadIndicesTriangleStrip));

        quadMeshEntity = meshBuilder->CreateMesh(ecs);
        node = nodesystem->CreateNode();
        renderMeshManager->Create(node->GetEntity());
        if (auto scopedHandle = renderMeshManager->Write(node->GetEntity()); scopedHandle) {
            scopedHandle->mesh = quadMeshEntity;
        }
        node->SetPosition({ -3.0f, -0.10f, -0.50f });
    }
}
} // namespace

/**
 * @tc.name: meshBuilderTestVulkan
 * @tc.desc: Tests for Mesh Builder Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, meshBuilderTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    MeshBuilderTest(res);
    res.TickTestAutoRng(2);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/meshBuilderTestVulkan.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

namespace {
constexpr string_view CAMERA_0_NAME { "MultiViewSampleCamera0" };
constexpr string_view CAMERA_1_NAME { "MultiViewSampleCamera1" };

RenderHandleReference CreateTargetGpuImage(
    IGpuResourceManager& gpuResourceMgr, const Math::UVec2& size, const string_view& name)
{
    GpuImageDesc desc;
    desc.width = size.x;
    desc.height = size.y;
    desc.depth = 1;
    desc.layerCount = 1U; // this is the final output
    desc.format = Format::BASE_FORMAT_R8G8B8A8_SRGB;
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.usageFlags =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    return gpuResourceMgr.Create(name, desc);
}

void NegativeScale(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();

    const Entity sceneEntity = ecs.GetEntityManager().Create();
    IGpuResourceManager& gpuResourceMgr = res.GetRenderContext().GetDevice().GetGpuResourceManager();

    // Create default components for scene.
    ITransformComponentManager& transformManager = *(GetManager<ITransformComponentManager>(ecs));
    transformManager.Create(sceneEntity);
    ILocalMatrixComponentManager& localMatrixManager = *(GetManager<ILocalMatrixComponentManager>(ecs));
    localMatrixManager.Create(sceneEntity);
    IWorldMatrixComponentManager& worldMatrixManager = *(GetManager<IWorldMatrixComponentManager>(ecs));
    worldMatrixManager.Create(sceneEntity);

    INodeComponentManager& nodeManager = *(GetManager<INodeComponentManager>(ecs));
    nodeManager.Create(sceneEntity);

    // Add name.
    {
        INameComponentManager& nameManager = *(GetManager<INameComponentManager>(ecs));
        nameManager.Create(sceneEntity);
        ScopedHandle<NameComponent> nameHandle = nameManager.Write(sceneEntity);
        nameHandle->name = "NegativeScaleTest";
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    renderConfigMgr->Create(sceneEntity);
    if (auto renderConfig = renderConfigMgr->Write(sceneEntity)) {
        renderConfig->renderingFlags = RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
        renderConfig->environment = sceneEntity;
    }
    auto nodeSystem = GetSystem<INodeSystem>(ecs);

    // post process component
    const Entity postProcessEntity = ecs.GetEntityManager().Create();
    {
        auto postProcessManager = GetManager<IPostProcessComponentManager>(ecs);
        postProcessManager->Create(postProcessEntity);
        if (auto postProcessHandle = postProcessManager->Write(postProcessEntity); postProcessHandle) {
            PostProcessComponent& postProcess = *postProcessHandle;
            postProcess.enableFlags = PostProcessConfiguration::ENABLE_TONEMAP_BIT;
        }
    }

    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    IEntityManager& em = ecs.GetEntityManager();
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    const Math::UVec2 imageSize = { res.GetWindowWidth() / 2U, res.GetWindowHeight() };

    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    Entity cameraEntity1 = sceneUtil.CreateCamera(ecs, Math::Vec3(0.5f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 75.0f, 10.0f);

    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity1, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 75.0f, 10.0f);

    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity)) {
        camHandle->multiViewCameras.clear();
        camHandle->multiViewCameras.push_back(cameraEntity1);
        camHandle->sceneFlags |=
            CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT | CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        camHandle->postProcess = postProcessEntity;
        camHandle->projection = CameraComponent::Projection::PERSPECTIVE;
    }

    if (auto camHandle = cameraMgr->Write(cameraEntity1)) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        camHandle->postProcess = postProcessEntity;
        camHandle->projection = CameraComponent::Projection::PERSPECTIVE;
        camHandle->pipelineFlags = CameraComponent::PipelineFlagBits::MULTI_VIEW_ONLY_BIT;

        {
            const RenderHandleReference handle = CreateTargetGpuImage(gpuResourceMgr, imageSize, CAMERA_1_NAME);
            EntityReference entity = GetOrCreateEntityReference(em, *renderHandleManager, handle);
            camHandle->customColorTargets.clear();
            camHandle->customColorTargets.push_back(entity);
        }
    }

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.5f, 1.0f, 8.0f };
        lc.intensity = 4.0f;
        lc.shadowEnabled = true;

        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
    }

    // Scene meshes
    {
        auto scene = nodeSystem->GetNode(sceneEntity);
        auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();

        constexpr const uint32_t cubeGridWidth = 5u;
        constexpr const uint32_t cubeGridHeight = 5u;

        const Entity material = UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 0.0f, 0.0f, 1.0f });

        auto cube = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "Cube", material, 1.0f, 1.0f, 1.0f));
        cube->SetPosition({ 1.f, 0.f, 0.f });
        cube->SetParent(*scene);

        // clone the cube and invert the geometry with a negative scaling factor.
        auto node = nodeSystem->CloneNode(*cube, true);
        node->SetPosition({ -1.f, 0.f, 0.f });
        node->SetScale({ -1.f, -1.f, -1.f });
        node->SetParent(*scene);
    }
}
} // namespace

/**
 * @tc.name: negativeScaleTestVulkan
 * @tc.desc: Tests for Negative Scale Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, negativeScaleTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(100u, 100u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    NegativeScale(res);
    res.TickTestAutoRng(1);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/negativeScaleTestVulkan.png"), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

namespace {
void OcclusionMaterial(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();

    const Entity sceneEntity = ecs.GetEntityManager().Create();
    IGpuResourceManager& gpuResourceMgr = res.GetRenderContext().GetDevice().GetGpuResourceManager();

    // Create default components for scene.
    ITransformComponentManager& transformManager = *(GetManager<ITransformComponentManager>(ecs));
    transformManager.Create(sceneEntity);
    ILocalMatrixComponentManager& localMatrixManager = *(GetManager<ILocalMatrixComponentManager>(ecs));
    localMatrixManager.Create(sceneEntity);
    IWorldMatrixComponentManager& worldMatrixManager = *(GetManager<IWorldMatrixComponentManager>(ecs));
    worldMatrixManager.Create(sceneEntity);

    INodeComponentManager& nodeManager = *(GetManager<INodeComponentManager>(ecs));
    nodeManager.Create(sceneEntity);

    // Add name.
    {
        INameComponentManager& nameManager = *(GetManager<INameComponentManager>(ecs));
        nameManager.Create(sceneEntity);
        ScopedHandle<NameComponent> nameHandle = nameManager.Write(sceneEntity);
        nameHandle->name = "OcclusionMaterial";
    }

    Entity environmentEntity;
    // env
    {
        IGltf2& gltf2_ = res.GetGraphicsContext().GetGltf();

        auto gltf = gltf2_.LoadGLTF("test://gltf/Environment/glTF/Environment.gltf");
        size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
        if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
            sceneIndex = 0;
        }
        auto importer = gltf2_.CreateGLTF2Importer(ecs);
        importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
        const auto& gltfImportResult = importer->GetResult();
        res.AppendResources(gltfImportResult.data);
        environmentEntity = gltf2_.ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL);
    }

    IRenderConfigurationComponentManager* renderConfigMgr = GetManager<IRenderConfigurationComponentManager>(ecs);
    renderConfigMgr->Create(sceneEntity);
    if (auto renderConfig = renderConfigMgr->Write(sceneEntity)) {
        renderConfig->renderingFlags = RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
        renderConfig->environment = environmentEntity;
    }
    auto nodeSystem = GetSystem<INodeSystem>(ecs);

    // post process component
    const Entity postProcessEntity = ecs.GetEntityManager().Create();
    {
        auto postProcessManager = GetManager<IPostProcessComponentManager>(ecs);
        postProcessManager->Create(postProcessEntity);
        if (auto postProcessHandle = postProcessManager->Write(postProcessEntity); postProcessHandle) {
            PostProcessComponent& postProcess = *postProcessHandle;
            postProcess.enableFlags = PostProcessConfiguration::ENABLE_TONEMAP_BIT;
        }
    }

    const auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    IEntityManager& em = ecs.GetEntityManager();
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    const Math::UVec2 imageSize = { res.GetWindowWidth() / 2U, res.GetWindowHeight() };

    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 75.0f, 10.0f);

    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity)) {
        camHandle->sceneFlags |=
            CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT | CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
        camHandle->pipelineFlags |= (CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        camHandle->postProcess = postProcessEntity;
        camHandle->projection = CameraComponent::Projection::PERSPECTIVE;
    }

    if (GetManager<ILightComponentManager>(ecs)->GetComponentCount() == 0) {
        LightComponent lc;
        lc.type = LightComponent::Type::DIRECTIONAL;
        lc.color = { 0.5f, 1.0f, 8.0f };
        lc.intensity = 4.0f;
        lc.shadowEnabled = true;

        sceneUtil.CreateLight(ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
            Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
    }

    // Scene meshes
    {
        auto scene = nodeSystem->GetNode(sceneEntity);
        auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();

        const Entity material = UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 0.0f, 0.0f, 1.0f });
        auto cube = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "Cube", material, 1.0f, 1.0f, 1.0f));
        cube->SetPosition({ 0.5f, 0.f, 0.f });
        cube->SetScale(Math::Vec3(2.f));
        cube->SetRotation(Math::FromEulerRad(Math::Vec3(25.f * Math::DEG2RAD, 45.f * Math::DEG2RAD, 0.f)));
        cube->SetParent(*scene);

        const Entity occlusionMaterial = UTest::CreateSolidColorMaterial(ecs, Math::Vec4 { 0.0f, 1.0f, 0.0f, 1.0f });
        if (auto materialHandle = GetManager<IMaterialComponentManager>(ecs)->Write(occlusionMaterial)) {
            materialHandle->type = MaterialComponent::Type::OCCLUSION;
            materialHandle->materialLightingFlags = 0U;
        }
        auto torus = nodeSystem->GetNode(meshUtil.GenerateTorus(ecs, "Torus", occlusionMaterial, 0.5f, 0.1f, 32U, 32U));
        torus->SetPosition({ 0.5f, 0.f, 0.f });
        torus->SetScale(Math::Vec3(2.f));
        torus->SetRotation(Math::FromEulerRad(Math::Vec3(25.f * Math::DEG2RAD, 45.f * Math::DEG2RAD, 0.f)));
        torus->SetParent(*scene);
    }
}
} // namespace

/**
 * @tc.name: occlusionMaterialTestVulkan
 * @tc.desc: Tests for Occlusion Material with Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, occlusionMaterialTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(100u, 100u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    OcclusionMaterial(res);
    res.TickTestAutoRng(3);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/occlusionMaterialTestVulkan.png"), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

/**
 * @tc.name: occlusionMaterialTestOpenGL
 * @tc.desc: Tests for Occlusion Material with OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, occlusionMaterialTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(100u, 100u, UTest::GetOpenGLBackend());
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    OcclusionMaterial(res);
    res.TickTestAutoRng(3);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/occlusionMaterialTestOpenGL.png"), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}
