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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#if !defined(JSON_IMPL)
#define JSON_IMPL
#include <core/json/json.h>
#else
#include <core/json/json.h>
#endif
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/render_data_structures.h>

#include "gltf/gltf2.h"
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
GLTFImportResult LoadAndImport(string_view filename, Gltf2& gltf2, IEcs& ecs, Entity& root)
{
    auto gltf = gltf2.LoadGLTF(filename);
    EXPECT_TRUE(gltf.success);
    if (gltf.success) {
        auto importer = gltf2.CreateGLTF2Importer(ecs);
        importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);

        GLTFImportResult result = importer->GetResult();
        EXPECT_TRUE(result.success);
        if (result.success) {
            root = gltf2.ImportGltfScene(gltf.data->GetDefaultSceneIndex(), *gltf.data, result.data, ecs, {},
                CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL);
            EXPECT_TRUE(EntityUtil::IsValid(root));
            GetManager<IRenderConfigurationComponentManager>(ecs)->Create(root);
        }
        return result;
    }
    GLTFImportResult error;
    error.success = false;
    return error;
}
} // namespace

/**
 * @tc.name: ExportGltfAnimation
 * @tc.desc: Tests for Export Gltf Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFExporterTest, ExportGltfAnimation, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/BrainStem/glTF/BrainStem.gltf";

    Entity root;
    GLTFImportResult originalGltf = LoadAndImport(filename, *gltf2, *ecs, root);
    ASSERT_TRUE(originalGltf.success);
    EXPECT_EQ(0, originalGltf.data.textures.size());
    EXPECT_EQ(59, originalGltf.data.materials.size());
    EXPECT_EQ(1, originalGltf.data.meshes.size());
    EXPECT_EQ(0, originalGltf.data.images.size());
    EXPECT_EQ(1, originalGltf.data.animations.size());
    EXPECT_EQ(1, originalGltf.data.skins.size());

    // Create animation playbacks
    INodeSystem* nodeSystem = GetSystem<INodeSystem>(*ecs);
    IAnimationSystem* animationSystem = GetSystem<IAnimationSystem>(*ecs);
    if (auto animationRootNode = nodeSystem->GetNode(root); animationRootNode) {
        for (const auto& animation : originalGltf.data.animations) {
            IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, *animationRootNode);
            if (playback) {
                playback->SetPlaybackState(AnimationComponent::PlaybackState::PLAY);
                playback->SetRepeatCount(AnimationComponent::REPEAT_COUNT_INFINITE);
            }
        }
    }

    constexpr string_view exportFilename = "cache://BrainStem.gltf";

    ASSERT_TRUE(gltf2->SaveGLTF(*ecs, exportFilename));

    GLTFImportResult exportedGltf = LoadAndImport(exportFilename, *gltf2, *ecs, root);
    ASSERT_TRUE(exportedGltf.success);
    EXPECT_EQ(originalGltf.data.textures.size(), exportedGltf.data.textures.size());
    EXPECT_EQ(originalGltf.data.materials.size(), exportedGltf.data.materials.size());
    EXPECT_EQ(originalGltf.data.meshes.size(), exportedGltf.data.meshes.size());
    EXPECT_EQ(originalGltf.data.images.size(), exportedGltf.data.images.size());
    EXPECT_EQ(originalGltf.data.animations.size(), exportedGltf.data.animations.size());
    EXPECT_EQ(originalGltf.data.skins.size(), exportedGltf.data.skins.size());

    delete gltf2;
}

namespace {
Entity CreateSolidColorMaterial(IEcs& ecs, Math::Vec4 color)
{
    IMaterialComponentManager* materialManager = GetManager<IMaterialComponentManager>(ecs);
    Entity entity = ecs.GetEntityManager().Create();
    materialManager->Create(entity);
    if (auto materialHandle = materialManager->Write(entity); materialHandle) {
        materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = color;
    }
    return entity;
}
void CreateScene(IEcs& ecs, IGraphicsContext& graphicsContext)
{
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
        nameHandle->name = "TestScene";
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

    const auto& sceneUtil = graphicsContext.GetSceneUtil();
    Entity cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    sceneUtil.UpdateCameraViewport(ecs, cameraEntity, { 128u, 128u }, true, Math::DEG2RAD * 75.0f, 10.0f);
    auto cameraMgr = GetManager<ICameraComponentManager>(ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity); camHandle) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT |
                CameraComponent::PipelineFlagBits::HISTORY_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::DEFERRED;
        camHandle->postProcess = postProcessEntity;
        camHandle->projection = CameraComponent::Projection::PERSPECTIVE;
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

    // Materials
    Entity material;
    {
        auto materialManager = GetManager<IMaterialComponentManager>(ecs);
        ASSERT_NE(nullptr, materialManager);
        material = ecs.GetEntityManager().Create();
        materialManager->Create(material);
        if (auto materialHandle = materialManager->Write(material); materialHandle) {
            materialHandle->type = MaterialComponent::Type::METALLIC_ROUGHNESS;
            materialHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT].factor =
                Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
            materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
        }
    }

    // Scene meshes
    {
        auto scene = nodeSystem->GetNode(sceneEntity);
        auto& meshUtil = graphicsContext.GetMeshUtil();

        constexpr const uint32_t cubeGridWidth = 5u;
        constexpr const uint32_t cubeGridHeight = 5u;

        {
            auto cube = nodeSystem->GetNode(meshUtil.GenerateCube(
                ecs, "Cube", CreateSolidColorMaterial(ecs, Math::Vec4 { 1.0f, 0.0f, 0.0f, 1.0f }), 1.0f, 1.0f, 1.0f));
            cube->SetPosition({ -3.0f, -2.0f, -5.5f });
            cube->SetParent(*scene);
        }
        {
            auto cube = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "Cube", material, 1.0f, 1.0f, 1.0f));
            cube->SetPosition({ -3.0f, -2.0f, -5.5f });
            cube->SetParent(*scene);
        }
    }
}
} // namespace

/**
 * @tc.name: ExportManuallyCreatedScene
 * @tc.desc: Tests for Export Manually Created Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFExporterTest, ExportManuallyCreatedScene, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    CreateScene(*ecs, *graphicsContext);

    constexpr string_view exportFilename = "cache://Scene.gltf";

    ASSERT_TRUE(gltf2->SaveGLTF(*ecs, exportFilename));

    auto file = engine->GetFileManager().OpenFile(exportFilename);
    ASSERT_TRUE(file);
    auto byteLength = file->GetLength();
    string raw;
    raw.resize(static_cast<size_t>(byteLength));
    ASSERT_EQ(byteLength, file->Read(raw.data(), byteLength));

    auto jsonValue = json::parse(raw.data());
    ASSERT_TRUE(jsonValue.is_object());
    ASSERT_TRUE(jsonValue.find("asset"));
    {
        auto cameras = jsonValue.find("cameras");
        ASSERT_TRUE(cameras);
        ASSERT_TRUE(cameras->is_array());
        ASSERT_EQ(1, cameras->array_.size());
    }
    {
        auto nodes = jsonValue.find("nodes");
        ASSERT_TRUE(nodes);
        ASSERT_TRUE(nodes->is_array());
        ASSERT_EQ(5, nodes->array_.size());
    }
    {
        auto extensions = jsonValue.find("extensions");
        ASSERT_TRUE(extensions);
        ASSERT_TRUE(extensions->is_object());
        auto khrLightsPunctual = extensions->find("KHR_lights_punctual");
        ASSERT_TRUE(khrLightsPunctual);
        ASSERT_TRUE(khrLightsPunctual->is_object());
        auto lights = khrLightsPunctual->find("lights");
        ASSERT_TRUE(lights);
        ASSERT_TRUE(lights->is_array());
        ASSERT_EQ(1, lights->array_.size());
    }

    delete gltf2;
}

namespace {
EntityReference CreateShader(CORE_NS::IEcs& ecs, RENDER_NS::IRenderContext& renderContext, BASE_NS::string uri)
{
    auto uriManager = GetManager<IUriComponentManager>(ecs);
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    auto& shaderMgr = renderContext.GetDevice().GetShaderManager();

    const EntityReference shader = ecs.GetEntityManager().CreateReferenceCounted();
    renderHandleManager->Create(shader);
    shaderMgr.LoadShaderFile(uri);
    renderHandleManager->Write(shader)->reference = shaderMgr.GetShaderHandle(uri);
    uriManager->Create(shader);
    uriManager->Write(shader)->uri = uri;
    return shader;
}
void EditScene(IEcs& ecs, IRenderContext& renderContext, Entity root)
{
    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
    auto meshManager = GetManager<IMeshComponentManager>(ecs);
    auto materialManager = GetManager<IMaterialComponentManager>(ecs);
    auto rhManager = GetManager<IRenderHandleComponentManager>(ecs);

    ISceneNode* sceneNode = nodeSystem->GetNode(root)->GetChild("TestScene");
    ASSERT_NE(nullptr, sceneNode);
    EntityReference samplers[4];
    EntityReference image;
    {
        for (uint32_t i = 0; i < countof(samplers); ++i) {
            samplers[i] = ecs.GetEntityManager().CreateReferenceCounted();
            rhManager->Create(samplers[i]);
            if (auto samplerHandle = rhManager->Write(samplers[i]); samplerHandle) {
                GpuSamplerDesc desc;
                desc.magFilter = CORE_FILTER_LINEAR;
                desc.mipMapMode = (i % 2 == 0) ? CORE_FILTER_NEAREST : CORE_FILTER_LINEAR;   // 2: parm
                desc.minFilter = (i < 2) ? CORE_FILTER_LINEAR : CORE_FILTER_NEAREST;         // 2: parm
                desc.addressModeU = (i % 2 == 0) ? CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : // 2: parm
                                        CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                desc.addressModeV = CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                desc.addressModeW = CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerHandle->reference = renderContext.GetDevice().GetGpuResourceManager().Create(desc);
            }
        }

        image = ecs.GetEntityManager().CreateReferenceCounted();
        rhManager->Create(image);
        if (auto imageHandle = rhManager->Write(image); imageHandle) {
            GpuImageDesc imageDesc;
            imageDesc.width = 16u;
            imageDesc.height = 16u;
            imageDesc.depth = 1;
            imageDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
            imageDesc.format = BASE_FORMAT_R8G8B8A8_SRGB;
            imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
            imageDesc.imageType = CORE_IMAGE_TYPE_2D;
            imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
            imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageHandle->reference = renderContext.GetDevice().GetGpuResourceManager().Create(imageDesc);
        }

        ISceneNode* node = sceneNode->GetChild("weightNode");
        ASSERT_NE(nullptr, node);
        Entity nodeEntity = node->GetEntity();
        if (auto rmHandle = renderMeshManager->Read(nodeEntity); rmHandle) {
            if (auto meshHandle = meshManager->Read(rmHandle->mesh); meshHandle) {
                ASSERT_EQ(1, meshHandle->submeshes.size());
                Entity material = meshHandle->submeshes[0].material;
                if (auto matHandle = materialManager->Write(material); matHandle) {
                    matHandle->type = MaterialComponent::Type::SPECULAR_GLOSSINESS;
                    matHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
                    matHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].sampler = samplers[0];
                    matHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
                    matHandle->textures[MaterialComponent::TextureIndex::MATERIAL].sampler = samplers[1];
                    matHandle->textures[MaterialComponent::TextureIndex::MATERIAL].image = image;
                    matHandle->materialShader.shader = CreateShader(ecs, renderContext, "test://shader/test.shader");
                    matHandle->materialShader.graphicsState = EntityReference {};
                }
            }
        }
    }
    {
        ISceneNode* node = sceneNode->GetChild("samplerNode");
        ASSERT_NE(nullptr, node);
        Entity nodeEntity = node->GetEntity();
        if (auto rmHandle = renderMeshManager->Read(nodeEntity); rmHandle) {
            if (auto meshHandle = meshManager->Read(rmHandle->mesh); meshHandle) {
                ASSERT_EQ(1, meshHandle->submeshes.size());
                Entity material = meshHandle->submeshes[0].material;
                if (auto matHandle = materialManager->Write(material); matHandle) {
                    matHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 1.0f };
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 1.0f };
                    matHandle->textures[MaterialComponent::TextureIndex::MATERIAL].factor.w = 0.03f;
                    matHandle->textures[MaterialComponent::TextureIndex::SHEEN].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 1.0f };
                    matHandle->textures[MaterialComponent::TextureIndex::TRANSMISSION].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 1.0f };

                    matHandle->textures[MaterialComponent::TextureIndex::AO].sampler = samplers[2]; // 2: index
                    matHandle->textures[MaterialComponent::TextureIndex::AO].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::AO].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };

                    matHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].sampler = samplers[3]; // 3: index
                    matHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::MATERIAL].sampler = samplers[0];
                    matHandle->textures[MaterialComponent::TextureIndex::MATERIAL].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT].sampler = samplers[1];
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL].sampler =
                        samplers[2]; // 2: index
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].sampler =
                        samplers[3]; // 3: index
                    matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::SHEEN].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
                    matHandle->textures[MaterialComponent::TextureIndex::SHEEN].sampler = samplers[0];
                    matHandle->textures[MaterialComponent::TextureIndex::SHEEN].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::SPECULAR].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
                    matHandle->textures[MaterialComponent::TextureIndex::SPECULAR].sampler = samplers[1];
                    matHandle->textures[MaterialComponent::TextureIndex::SPECULAR].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::TRANSMISSION].sampler =
                        samplers[2]; // 2: index
                    matHandle->textures[MaterialComponent::TextureIndex::TRANSMISSION].image = image;

                    matHandle->textures[MaterialComponent::TextureIndex::EMISSIVE].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };

                    matHandle->textures[MaterialComponent::TextureIndex::NORMAL].transform.scale =
                        Math::Vec2 { 0.5f, 0.5f };
                    matHandle->textures[MaterialComponent::TextureIndex::NORMAL].sampler = samplers[3]; // 3: index
                    matHandle->textures[MaterialComponent::TextureIndex::NORMAL].image = image;
                    matHandle->textures[MaterialComponent::TextureIndex::NORMAL].factor =
                        Math::Vec4 { 0.5f, 0.5f, 0.5f, 0.5f };
                }
            }
        }
    }
    {
        ISceneNode* node = sceneNode->GetChild("sparseNode");
        ASSERT_NE(nullptr, node);
        Entity nodeEntity = node->GetEntity();
        if (auto rmHandle = renderMeshManager->Read(nodeEntity); rmHandle) {
            if (auto meshHandle = meshManager->Read(rmHandle->mesh); meshHandle) {
                ASSERT_EQ(1, meshHandle->submeshes.size());
                Entity material = meshHandle->submeshes[0].material;
                if (auto matHandle = materialManager->Write(material); matHandle) {
                    matHandle->type = MaterialComponent::Type::UNLIT;
                    matHandle->alphaCutoff = 0.5f;
                }
            }
        }
    }
}
} // namespace

#ifdef NDEBUG
/**
 * @tc.name: ExportEditedScene
 * @tc.desc: Tests for Export Edited Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFExporterTest, ExportEditedScene, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/Custom/Custom.gltf";

    Entity root;
    GLTFImportResult originalGltf = LoadAndImport(filename, *gltf2, *ecs, root);
    ASSERT_TRUE(originalGltf.success);
    EXPECT_EQ(1, originalGltf.data.images.size());
    EXPECT_EQ(1, originalGltf.data.specularRadianceCubemaps.size());
    EXPECT_EQ(1, originalGltf.data.textures.size());
    EXPECT_EQ(11, originalGltf.data.materials.size());
    EXPECT_EQ(4, originalGltf.data.samplers.size());
    EXPECT_EQ(10, originalGltf.data.meshes.size());
    EXPECT_EQ(6, originalGltf.data.animations.size());

    EditScene(*ecs, *renderContext, root);

    constexpr string_view exportFilename = "cache://Edited.gltf";
    ASSERT_TRUE(gltf2->SaveGLTF(*ecs, exportFilename));

    delete gltf2;
}
#endif // NDEBUG

/**
 * @tc.name: ExportInvalidLight
 * @tc.desc: Tests for Export Invalid Light. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFExporterTest, ExportInvalidLight, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    LightComponent lc;
    lc.type = static_cast<LightComponent::Type>(53u);
    lc.color = { 0.5f, 1.0f, 8.0f };
    lc.intensity = 4.0f;
    lc.shadowEnabled = true;

    graphicsContext->GetSceneUtil().CreateLight(*ecs, lc, Math::Vec3(0.0f, 0.0f, 3.0f),
        Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));

    constexpr string_view exportFilename = "cache://Scene.gltf";

    ASSERT_FALSE(gltf2->SaveGLTF(*ecs, exportFilename));

    delete gltf2;
}

/**
 * @tc.name: ExportInvalidCamera
 * @tc.desc: Tests for Export Invalid Camera. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFExporterTest, ExportInvalidCamera, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    Entity cameraEntity =
        graphicsContext->GetSceneUtil().CreateCamera(*ecs, Math::Vec3(0.0f, 0.5f, 4.0f), {}, 2.0f, 100.0f, 75.0f);
    auto cameraMgr = GetManager<ICameraComponentManager>(*ecs);
    if (auto camHandle = cameraMgr->Write(cameraEntity); camHandle) {
        camHandle->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        camHandle->pipelineFlags |=
            (CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT | CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT |
                CameraComponent::PipelineFlagBits::HISTORY_BIT);
        camHandle->renderingPipeline = CameraComponent::RenderingPipeline::DEFERRED;
        camHandle->projection = static_cast<CameraComponent::Projection>(44u);
    }

    constexpr string_view exportFilename = "cache://Scene.gltf";

    ASSERT_FALSE(gltf2->SaveGLTF(*ecs, exportFilename));

    delete gltf2;
}
