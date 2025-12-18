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

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/weather_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/perf/intf_performance_data_manager.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>
#include <render/util/performance_data_structures.h>

// Cloud test
#include <3d/util/intf_mesh_util.h>

#include "gfx_common.h"

// Windows macro conflicts with our LoadImage functions
#undef LoadImage

#define WIDTH 800
#define HEIGHT 500

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {

struct PlaneMat {
    Entity entity;
    Entity matEntity;
};

EntityReference LoadImage(IImageLoaderManager& imageManager, const string_view& uri,
    IGpuResourceManager& gpuResourceMgr, IRenderHandleComponentManager& handleManager)
{
    auto result = imageManager.LoadImage(uri, 0);
    if (!result.success || !result.image) {
        return EntityReference {};
    }
    CORE_ASSERT(result.success);
    GpuImageDesc gpuImageDesc = gpuResourceMgr.CreateGpuImageDesc(result.image->GetImageDesc());
    RenderHandleReference backgroundImageHandle = gpuResourceMgr.Create(gpuImageDesc, std::move(result.image));

    auto imageEntity = handleManager.GetEcs().GetEntityManager().CreateReferenceCounted();
    handleManager.Create(imageEntity);
    handleManager.Write(imageEntity)->reference = move(backgroundImageHandle);
    return imageEntity;
}

void TickTest(
    IEcs& ecs, IEngine& engine, const IGraphicsContext& gfxCtx, IRenderContext& renderCtx, int frameCountToTick)
{
    auto& rdsMgr = renderCtx.GetRenderDataStoreManager();
    uint32_t numBytes = WIDTH * HEIGHT * 4;
    unique_ptr<ByteArray> byteArray = make_unique<ByteArray>(numBytes);

    IGpuResourceManager& gpuResourceMgr = renderCtx.GetDevice().GetGpuResourceManager();
    IRenderFrameUtil& rfUtil = renderCtx.GetRenderUtil().GetRenderFrameUtil();
    const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle("TestOutputImage");

    for (int i = 0; i < frameCountToTick; i++) {
        IEcs* pEcs = &ecs;
        const bool needsRender = engine.TickFrame(array_view(&pEcs, 1));
        IRenderer& renderer = renderCtx.GetRenderer();

        if (i == frameCountToTick - 1) {
            const IRenderFrameUtil::CopyFlags copyFlags = IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE;
            rfUtil.CopyToCpu(outputImageHandle0, copyFlags);
            renderer.RenderFrame({});
        } else {
            if (needsRender) {
                renderer.RenderFrame(gfxCtx.GetRenderNodeGraphs(ecs));
            }
        }
        if (i == frameCountToTick - 1) {
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(outputImageHandle0);
            const GpuBufferDesc desc = gpuResourceMgr.GetBufferDescriptor(data.bufferHandle);
            const uint32_t minSize = Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));

            if (data.byteBuffer && (data.copyFlags & IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE)) {
                const uint32_t minSize =
                    Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));
                // map the GPU buffer memory and copy to CPU ByteArray buffer
                if (void* mem = gpuResourceMgr.MapBufferMemory(data.bufferHandle); mem) {
                    BASE_NS::CloneData(byteArray->GetData().data(), data.byteBuffer->GetData().size(), mem, minSize);
                }
            }
        }
    }
}

vector<GLTFResourceData> MultiEcsTest(
    IEcs& ecs, IEngine& engine, const IGraphicsContext& gfxCtx, IRenderContext& renderCtx)
{
    vector<GLTFResourceData> outResourceRefs = {};

    IEntityManager& em = ecs.GetEntityManager();

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    ISceneNode* scene = nodeSystem->CreateNode();
    const Entity sceneRoot = scene->GetEntity();

    auto materialManager = GetManager<IMaterialComponentManager>(ecs);
    auto nameManager = GetManager<INameComponentManager>(ecs);
    auto uriManager = GetManager<IUriComponentManager>(ecs);
    auto evnManager = GetManager<IEnvironmentComponentManager>(ecs);

    auto& meshUtil = gfxCtx.GetMeshUtil();
    auto& sceneUtil = gfxCtx.GetSceneUtil();

    // camera component
    Entity cameraEntity;
    {
        cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 2.0f, 3.5f),
            Math::AngleAxis((Math::DEG2RAD * -20.0f), Math::Vec3(1.0f, 0.0f, 0.0f)), 0.1f, 100.0f, 60.0f);
        ICameraComponentManager* cameraManager = GetManager<ICameraComponentManager>(ecs);
        ScopedHandle<CameraComponent> cameraComponent = cameraManager->Write(cameraEntity);
        cameraComponent->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        cameraComponent->pipelineFlags |=
            CameraComponent::PipelineFlagBits::MSAA_BIT | CameraComponent::PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT;
        cameraComponent->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        cameraComponent->clearColorValue = { 1.0f, 0.0f, 0.0f, 1.0f };
    }
    sceneUtil.UpdateCameraViewport(ecs, cameraEntity, { WIDTH, HEIGHT }, true, Math::DEG2RAD * 90.0f, 1.0f);

    // render configuration component
    {
        IRenderConfigurationComponentManager* rccm = GetManager<IRenderConfigurationComponentManager>(ecs);
        rccm->Create(sceneRoot);
        ScopedHandle<RenderConfigurationComponent> renderConfiguration = rccm->Write(sceneRoot);
        renderConfiguration->renderingFlags |= RenderConfigurationComponent::CREATE_RNGS_BIT;

        IEnvironmentComponentManager* environmentManager = GetManager<IEnvironmentComponentManager>(ecs);
        if (environmentManager->GetComponentCount() == 0) {
            environmentManager->Create(sceneRoot);
        }
        renderConfiguration->environment = environmentManager->GetEntity(0);

        if (auto envDataHandle = environmentManager->Write(renderConfiguration->environment); envDataHandle) {
            EnvironmentComponent& envComponent = *envDataHandle;
            envComponent.indirectDiffuseFactor.w = 0.5f;
            envComponent.indirectSpecularFactor.w = 0.5f;
            envComponent.envMapFactor.w = 0.5f;
            envComponent.background = EnvironmentComponent::Background::SKY;
            envComponent.weather = em.GetReferenceCounted(sceneRoot);
        }
    }

    // Reflection plane
    Entity reflectionPlaneMaterial = em.Create();
    Entity reflectionPlane;
    {
        nameManager->Create(reflectionPlaneMaterial);
        nameManager->Set(reflectionPlaneMaterial, { { "ReflectionPlaneMaterial" } });
        materialManager->Create(reflectionPlaneMaterial);
        reflectionPlane = meshUtil.GeneratePlane(ecs, "ReflectionPlane", reflectionPlaneMaterial, 10.0f, 10.0f);
        if (ISceneNode* node = nodeSystem->GetNode(reflectionPlane); node) {
            node->SetPosition(Math::Vec3(0.0f, 0.0f, 0.0f));
            node->SetScale(Math::Vec3(10, 0.01f, 10)); // 10: parm
        }
        sceneUtil.CreateReflectionPlaneComponent(ecs, reflectionPlane);
        if (auto materialHandle = materialManager->Write(reflectionPlaneMaterial); materialHandle) {
            materialHandle->materialLightingFlags &= ~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;
            materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                Math::Vec4(0.1f, 0.1f, 0.1f, 1.0f);
            materialHandle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                Math::Vec4(0.f, 0.01f, 0.0f, 0.3f);
        }
    }
    // Gltf model
    const vector<UTest::GltfImportInfo> files = {
        { "test://gltf/BrainStem/glTF/BrainStem.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
        { "test://gltf/MorphStressTest/MorphStressTest.glb", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
    };

    const Math::Vec3 modelPositions[] = {
        { 2.0f, 0.0f, -1.0f },
        { -2.0f, 0.0f, -1.0f },
    };

    // Load and import all gltf files.
    for (size_t importedFileIndex = 0; importedFileIndex < files.size(); ++importedFileIndex) {
        auto const& info = files[importedFileIndex];

        auto gltf = gfxCtx.GetGltf().LoadGLTF(info.filename);
        if (!gltf.success) {
            CORE_LOG_E("Loading '%s' resulted in errors:\n%s", info.filename, gltf.error.c_str());
            continue;
        }

        auto importer = gfxCtx.GetGltf().CreateGLTF2Importer(ecs);
        importer->ImportGLTF(*gltf.data, info.resourceImportFlags);

        const auto& gltfImportResult = importer->GetResult();

        if (gltfImportResult.success) {
            size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
            if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
                sceneIndex = 0;
            }

            Entity importedSceneEntity;
            if (sceneIndex != CORE_GLTF_INVALID_INDEX) {
                importedSceneEntity = gfxCtx.GetGltf().ImportGltfScene(
                    sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneRoot, info.sceneImportFlags);
            }
            outResourceRefs.push_back(gltfImportResult.data);
            ISceneNode* model = nodeSystem->GetNode(importedSceneEntity);
            Math::Quat rot = Math::FromEulerRad(Math::Vec3 { 0.f, 0.f, 0.f });
            model->SetPosition(modelPositions[importedFileIndex]);
            model->SetRotation(rot);

            // Animations
            if (!gltfImportResult.data.animations.empty()) {
                INodeSystem* nodeSystem = GetSystem<INodeSystem>(ecs);
                IAnimationSystem* animationSystem = GetSystem<IAnimationSystem>(ecs);
                if (auto animationRootNode = nodeSystem->GetNode(importedSceneEntity); animationRootNode) {
                    for (const auto& animation : gltfImportResult.data.animations) {
                        IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, *animationRootNode);
                        if (playback) {
                            constexpr auto state = AnimationComponent::PlaybackState::PLAY;
                            playback->SetPlaybackState(state);
                            playback->SetRepeatCount(AnimationComponent::REPEAT_COUNT_INFINITE);
                        }
                    }
                }
            }
        } else {
            CORE_LOG_E("Importing of '%s' failed: %s", info.filename, gltfImportResult.error.c_str());
        }
    }

    // Weather component
    auto* wcm = GetManager<IWeatherComponentManager>(ecs);
    wcm->Create(sceneRoot);
    if (auto handle = wcm->Write(sceneRoot)) {
        auto& imageManager = engine.GetImageLoaderManager();
        auto& gpuResourceMgr = renderCtx.GetDevice().GetGpuResourceManager();
        auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);

        handle->lowFrequencyImage =
            LoadImage(imageManager, "test://image/weather/HighFrequency.ktx", gpuResourceMgr, *handleManager);
        handle->highFrequencyImage =
            LoadImage(imageManager, "test://image/weather/HighFrequency.ktx", gpuResourceMgr, *handleManager);
        handle->curlNoiseImage =
            LoadImage(imageManager, "test://image/weather/curlNoise.png", gpuResourceMgr, *handleManager);
        handle->cirrusImage =
            LoadImage(imageManager, "test://image/weather/curlNoise.png", gpuResourceMgr, *handleManager);
        handle->weatherMapImage =
            LoadImage(imageManager, "test://image/weather/curlNoise.png", gpuResourceMgr, *handleManager);
        handle->coverage = 0.6f;
        handle->windSpeed = 0.0f;
    }
    return outResourceRefs;
}
} // namespace

struct NodeContainer {
    string type;
    bool found;
};
/**
 * @tc.name: MultiEcs
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, MultiEcs, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(WIDTH, HEIGHT, DeviceBackendType::VULKAN);
    res.LiftTestUp(WIDTH, HEIGHT);
    auto& rc = res.GetRenderContext();
    auto& gc = res.GetGraphicsContext();
    auto& engine = res.GetEngine();

    // refs
    IEcs::Ptr pEcs[] = { UTest::CreateAndInitializeDefaultEcs(res.GetEngine()),
        UTest::CreateAndInitializeDefaultEcs(res.GetEngine()), UTest::CreateAndInitializeDefaultEcs(res.GetEngine()) };
    IEcs& renderEcs = *pEcs[1];

    // Keeps them alive
    auto entRefs = MultiEcsTest(renderEcs, engine, gc, rc);
    TickTest(renderEcs, engine, gc, rc, 10);

    // Validate that we have the expected render data store / render nodes on only one ecs instance
    uint32_t numEcsForRender = 0;
    for (IEcs::Ptr ecs : pEcs) {
        string expectedRDSName = "";
        if (ecs->GetId() != 0) { // 0 id isn't appended into the name
            // to_string returns fixed 21 char long string, so this is a hacky way to convert it to a full string.
            expectedRDSName = to_string(ecs->GetId()).c_str();
        }
        expectedRDSName.append("RenderDataStoreDefaultScene");

        auto rds = rc.GetRenderDataStoreManager().GetRenderDataStore(expectedRDSName);
        EXPECT_NE(rds, nullptr);

        auto rngs = gc.GetRenderNodeGraphs(*ecs);
        if (!rngs.empty()) {
            vector<NodeContainer> expectedNodeTypes = { { "RenderNodeMorph", false },
                { "RenderNodeWeatherSimulation", false }, { "RenderNodeDefaultLights", false },
                { "RenderNodeCameraWeather", false } };

            for (auto& rng : rngs) {
                auto info = rc.GetRenderNodeGraphManager().GetInfo(rng);
                for (auto& node : info.nodes) {
                    for (auto& expType : expectedNodeTypes) {
                        if (node.typeName == expType.type) {
                            expType.found = true;
                        }
                    }
                }
            }

            uint32_t numNodesFound = std::count_if(expectedNodeTypes.begin(), expectedNodeTypes.end(),
                [](NodeContainer& container) { return container.found; });
            EXPECT_EQ(numNodesFound, expectedNodeTypes.size());

            numEcsForRender++;
        }
    }

    EXPECT_EQ(numEcsForRender, 1);

    res.ShutdownTest();
}
