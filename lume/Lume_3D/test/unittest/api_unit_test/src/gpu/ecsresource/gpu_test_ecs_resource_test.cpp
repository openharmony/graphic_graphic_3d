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
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/gltf/gltf.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>

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

template<typename ComponentManager>
Entity LookupResourceByUri(
    string_view uri, const IUriComponentManager& uriManager, const ComponentManager& componentManager)
{
    const auto& entityManager = uriManager.GetEcs().GetEntityManager();
    const auto uriComponents = uriManager.GetComponentCount();
    for (auto i = 0u; i < uriComponents; ++i) {
        Entity entity = uriManager.GetEntity(i);
        if (entityManager.IsAlive(uriManager.GetEntity(i))) {
            if (const auto uriHandle = uriManager.Read(i); uriHandle) {
                const bool found = uriHandle->uri == uri;
                if (found && componentManager.HasComponent(entity)) {
                    return entity;
                }
            }
        }
    }
    return {};
}

template<typename Container, typename Pred>
bool any_of(const Container& c, Pred&& p)
{
    return std::any_of(std::begin(c), std::end(c), std::forward<Pred>(p));
}

struct PropertyValue {
    const IPropertyHandle* handle;
    const void* offset;
    const Property* info;
    operator bool()
    {
        return (handle) && (info);
    }
    template<typename Type>
    Type& get() const
    {
        return *static_cast<Type*>(const_cast<void*>(offset));
    }
};

PropertyValue GetPropertyValue(const PropertyValue value, const string_view name)
{
    if ((value.handle) && (value.info) && (!value.info->metaData.memberProperties.empty())) {
        for (const auto& prop : value.info->metaData.memberProperties) {
            if (prop.name == name) {
                return { value.handle, static_cast<const uint8_t*>(value.offset) + (uintptr_t)prop.offset, &prop };
            }
        }
    }
    return {};
}

PropertyValue GetPropertyArray(const PropertyValue value, size_t index)
{
    if ((value.handle) && (value.info) && (value.info->type.isArray) && (value.info->metaData.containerMethods)) {
        if (index < value.info->count) {
            auto size = value.info->size / value.info->count;
            return { value.handle, static_cast<const uint8_t*>(value.offset) + index * size,
                &value.info->metaData.containerMethods->property };
        }
    }
    return {};
}

PropertyValue GetProperty(const IPropertyHandle* handle, const string_view name, const void* offset)
{
    auto* api = handle->Owner();
    for (const auto& prop : api->MetaData()) {
        if (prop.name == name) {
            return { handle, static_cast<const uint8_t*>(offset) + prop.offset, &prop };
        }
    }
    return {};
}

/**
 * @tc.name: testMaterialResource
 * @tc.desc: Tests for Test Material Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_ResourceTest, testMaterialResource, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto components = GetPluginRegister().GetTypeInfos(ComponentManagerTypeInfo::UID);
    constexpr Uid requiredManagers[] = { IMaterialComponentManager::UID, INameComponentManager::UID,
        IUriComponentManager::UID };
    for (const auto component : components) {
        const auto info = static_cast<const ComponentManagerTypeInfo*>(component);
        if (any_of(requiredManagers, [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
            ecs->CreateComponentManager(*info);
        }
    }
    // Create material through resource manager.
    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    auto nameManager = GetManager<INameComponentManager>(*ecs);
    auto uriManager = GetManager<IUriComponentManager>(*ecs);
    auto materialEntity = ecs->GetEntityManager().Create();
    materialManager->Create(materialEntity);
    uriManager->Set(materialEntity, { string("test://material") });
    nameManager->Set(materialEntity, { string("material") });

    {
        // Manipulate values directly.
        auto data = materialManager->GetData(materialEntity);
        ASSERT_TRUE(data != nullptr);
        auto& desc = *static_cast<MaterialComponent*>(data->WLock());
        desc.textures[MaterialComponent::TextureIndex::MATERIAL]
            .factor[MaterialComponent::MetallicRoughnessChannel::ROUGHNESS] = 0.85f;
        data->WUnlock();
    }

    {
        // Manipulate values through properties
        auto data = materialManager->GetData(materialEntity);
        ASSERT_TRUE(data != nullptr);

        auto offset = data->WLock();
        // Just a sanity check
        auto NoSuchProperty = GetProperty(data, "NoSuchProperty", offset);
        ASSERT_TRUE(!NoSuchProperty);

        // first find "textures" array
        auto textureArray = GetProperty(data, "textures", offset);
        if (textureArray) {
            ASSERT_TRUE(textureArray.info->type.name == "MaterialComponent::TextureInfo"); // Can verify by name also
            ASSERT_TRUE((textureArray.info->type ==
                         PROPERTYTYPE_ARRAY(MaterialComponent::TextureInfo))); // must be a "TextureInfo"
            ASSERT_TRUE(textureArray.info->type.isArray);                      // must be an array
            // get the correct element from the array
            auto info = GetPropertyArray(textureArray, MaterialComponent::TextureIndex::MATERIAL);
            if (info) {
                ASSERT_TRUE(info.info->type.name == "MaterialComponent::TextureInfo"); // Can verify by name also
                ASSERT_TRUE(
                    (info.info->type == PROPERTYTYPE(MaterialComponent::TextureInfo))); // must be a "TextureInfo"
                // find the "factor" property
                auto value = GetPropertyValue(info, "factor");
                if (value) {
                    // modify it.
                    ASSERT_TRUE((value.info->type == PROPERTYTYPE(BASE_NS::Math::Vec4))); // must be a "Vec4"
                    value.get<Math::Vec4>()[MaterialComponent::MetallicRoughnessChannel::METALLIC] = 0.5f;
                }
            }
        }
        data->WUnlock();
    }

    {
        MaterialComponent desc = materialManager->Get(materialEntity);

        float MetallicFactor = Math::infinity;
        float RoughnessFactor = Math::infinity;

        // Get the factors with Properties
        // first find "textures" array
        auto data = materialManager->GetData(materialEntity);
        ASSERT_TRUE(data != nullptr);
        auto offset = data->RLock();
        auto textureArray = GetProperty(data, "textures", offset);
        if (textureArray) {
            ASSERT_TRUE(textureArray.info->type.isArray); // must be an array
            // get the correct element from the array
            auto info = GetPropertyArray(textureArray, MaterialComponent::TextureIndex::MATERIAL);
            if (info) {
                ASSERT_TRUE(info.info->type.name == "MaterialComponent::TextureInfo");
                ASSERT_TRUE(
                    (info.info->type == PROPERTYTYPE(MaterialComponent::TextureInfo))); // must be a "TextureInfo"
                // find the "factor" property
                auto value = GetPropertyValue(info, "factor");
                if (value) {
                    // modify it.
                    ASSERT_TRUE((value.info->type == PROPERTYTYPE(BASE_NS::Math::Vec4))); // must be a "Vec4"
                    MetallicFactor = value.get<Math::Vec4>()[MaterialComponent::MetallicRoughnessChannel::METALLIC];
                    RoughnessFactor = value.get<Math::Vec4>()[MaterialComponent::MetallicRoughnessChannel::ROUGHNESS];
                }
            }
        }
        data->RUnlock();

        // Ensure values match.
        ASSERT_EQ(desc.textures[MaterialComponent::TextureIndex::MATERIAL]
                      .factor[MaterialComponent::MetallicRoughnessChannel::ROUGHNESS],
            RoughnessFactor);

        ASSERT_EQ(desc.textures[MaterialComponent::TextureIndex::MATERIAL]
                      .factor[MaterialComponent::MetallicRoughnessChannel::ROUGHNESS],
            0.85f);

        ASSERT_EQ(desc.textures[MaterialComponent::TextureIndex::MATERIAL]
                      .factor[MaterialComponent::MetallicRoughnessChannel::METALLIC],
            MetallicFactor);

        ASSERT_EQ(desc.textures[MaterialComponent::TextureIndex::MATERIAL]
                      .factor[MaterialComponent::MetallicRoughnessChannel::METALLIC],
            0.5f);
    }

    // Destroy material.
    ecs->GetEntityManager().Destroy(materialEntity);

    // Ensure it is gone.
    ASSERT_EQ(LookupResourceByUri("test://material", *uriManager, *materialManager), Entity {});
}

/**
 * @tc.name: testMaterialCustomProperties
 * @tc.desc: Tests for Test Material Custom Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_ResourceTest, testMaterialCustomProperties, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto components = GetPluginRegister().GetTypeInfos(ComponentManagerTypeInfo::UID);
    constexpr Uid requiredManagers[] = { IMaterialComponentManager::UID, INameComponentManager::UID,
        IUriComponentManager::UID, IRenderHandleComponentManager::UID };
    for (const auto component : components) {
        const auto info = static_cast<const ComponentManagerTypeInfo*>(component);
        if (any_of(requiredManagers, [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
            ecs->CreateComponentManager(*info);
        }
    }
    // Create material through resource manager.
    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    auto nameManager = GetManager<INameComponentManager>(*ecs);
    auto uriManager = GetManager<IUriComponentManager>(*ecs);
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(*ecs);
    auto& shaderMgr = renderContext->GetDevice().GetShaderManager();
    auto createShader = [renderHandleManager, uriManager, &shaderMgr, &em = ecs->GetEntityManager()](
                            const string_view uri) {
        const EntityReference shader = em.CreateReferenceCounted();
        renderHandleManager->Create(shader);
        shaderMgr.LoadShaderFile(uri);
        renderHandleManager->Write(shader)->reference = shaderMgr.GetShaderHandle(uri);
        uriManager->Create(shader);
        uriManager->Write(shader)->uri = uri;
        return shader;
    };
    auto materialEntity = ecs->GetEntityManager().Create();
    materialManager->Create(materialEntity);
    uriManager->Set(materialEntity, { string("test://material") });
    nameManager->Set(materialEntity, { string("material") });
    {
        auto data = materialManager->GetData(materialEntity);
        EXPECT_FALSE(ScopedHandle<const MaterialComponent>(data)->customProperties);
        ScopedHandle<MaterialComponent>(data)->materialShader.shader = createShader("test://shader/test.shader");
        EXPECT_TRUE(ScopedHandle<const MaterialComponent>(data)->customProperties);
        if (auto material = ScopedHandle<const MaterialComponent>(data); material->customProperties) {
            auto customMetaData = material->customProperties->Owner()->MetaData();
            const auto* buffer = static_cast<const uint8_t*>(material->customProperties->RLock());
            for (const auto& meta : customMetaData) {
                switch (meta.type) {
                    case PropertyType::FLOAT_T: {
                        const auto value = *static_cast<const float*>(static_cast<const void*>(buffer + meta.offset));
                        EXPECT_EQ(meta.name, "f_");
                        EXPECT_EQ(value, 1.f);
                        break;
                    }
                    case PropertyType::UINT32_T: {
                        const auto value =
                            *static_cast<const uint32_t*>(static_cast<const void*>(buffer + meta.offset));
                        if (meta.name == "u_") {
                            EXPECT_EQ(value, 7U);
                        }
                        break;
                    }
                    case PropertyType::VEC4_T: {
                        const auto value =
                            *static_cast<const Math::Vec4*>(static_cast<const void*>(buffer + meta.offset));
                        EXPECT_EQ(meta.name, "v4_");
                        EXPECT_EQ(value, Math::Vec4(0.1f, 0.2f, 0.3f, 0.4f));
                        break;
                    }
                    case PropertyType::UVEC4_T: {
                        const auto value =
                            *static_cast<const Math::UVec4*>(static_cast<const void*>(buffer + meta.offset));
                        EXPECT_EQ(meta.name, "uv4_");
                        EXPECT_EQ(value, Math::UVec4(3U, 4U, 5U, 6U));
                        break;
                    }
                }
            }
            material->customProperties->RUnlock();
        }
    }
}

/**
 * @tc.name: testAnimationImport
 * @tc.desc: Tests for Test Animation Import. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_ResourceTest, testAnimationImport, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& gltf2 = graphicsContext->GetGltf();

    constexpr string_view filename = "test://gltf/SimpleSkin/SimpleSkin.gltf";

    auto gltf = gltf2.LoadGLTF(filename);
    ASSERT_TRUE(gltf.success);

    auto importer = gltf2.CreateGLTF2Importer(*ecs);
    importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);

    auto importResult1 = importer->GetResult();
    ASSERT_TRUE(importResult1.success);

    INodeSystem* nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_TRUE(nodeSystem);
    auto rootNode = nodeSystem->CreateNode();

    auto scene1 = gltf2.ImportGltfScene(0, *gltf.data, importResult1.data, *ecs, rootNode->GetEntity());
    EXPECT_EQ(importResult1.data.animations.size(), 1U);

    IAnimationSystem* animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_TRUE(animationSystem);

    {
        // creating with the node under which the scene was import should fail as the track target names do not match.
        IAnimationPlayback* playback = animationSystem->CreatePlayback(importResult1.data.animations[0], *rootNode);
        EXPECT_FALSE(playback);
    }
    {
        // creating with the node which is the root of the imported scene should find the track targets.
        auto sceneNode1 = nodeSystem->GetNode(scene1);
        ASSERT_TRUE(sceneNode1);

        IAnimationPlayback* playback = animationSystem->CreatePlayback(importResult1.data.animations[0], *sceneNode1);
        EXPECT_TRUE(playback);
    }
}

/**
 * @tc.name: testAnimationRetargeting
 * @tc.desc: Tests for Test Animation Retargeting. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_ResourceTest, testAnimationRetargeting, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& gltf2 = graphicsContext->GetGltf();

    constexpr string_view filename = "test://gltf/SimpleSkin/SimpleSkin.gltf";

    auto gltf = gltf2.LoadGLTF(filename);
    ASSERT_TRUE(gltf.success);

    auto importer = gltf2.CreateGLTF2Importer(*ecs);
    importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);

    auto importResult1 = importer->GetResult();
    ASSERT_TRUE(importResult1.success);
    auto scene1 = gltf2.ImportGltfScene(0, *gltf.data, importResult1.data, *ecs);
    EXPECT_EQ(importResult1.data.animations.size(), 1U);

    INodeSystem* nodeSystem = GetSystem<INodeSystem>(*ecs);
    IAnimationSystem* animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_TRUE(nodeSystem && animationSystem);

    auto sceneNode1 = nodeSystem->GetNode(scene1);
    ASSERT_TRUE(sceneNode1);

    IAnimationPlayback* playback = animationSystem->CreatePlayback(importResult1.data.animations[0], *sceneNode1);
    ASSERT_TRUE(playback);
    playback->SetPlaybackState(AnimationComponent::PlaybackState::PLAY);
    playback->SetRepeatCount(AnimationComponent::REPEAT_COUNT_INFINITE);

    importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL & ~CORE_GLTF_IMPORT_RESOURCE_ANIMATION);
    auto importResult2 = importer->GetResult();
    ASSERT_TRUE(importResult2.success);
    auto scene2 = gltf2.ImportGltfScene(0, *gltf.data, importResult2.data, *ecs);
    EXPECT_EQ(importResult2.data.animations.size(), 0U);

    ASSERT_EQ(sceneNode1->GetChildren().size(), 2U);
    auto modelNode1 = sceneNode1->GetChildren()[0];

    auto sceneNode2 = nodeSystem->GetNode(scene2);
    ASSERT_TRUE(sceneNode2);
    ASSERT_EQ(sceneNode2->GetChildren().size(), 2U);
    auto modelNode2 = sceneNode2->GetChildren()[0];

    auto& sceneUtil = graphicsContext->GetSceneUtil();
    auto playback2 =
        sceneUtil.RetargetSkinAnimation(*ecs, modelNode2->GetEntity(), modelNode1->GetEntity(), playback->GetEntity());
    ASSERT_TRUE(playback2);

    IEcs* ecsArray[] = { ecs.get() };
    engine->TickFrame(ecsArray);
}

/**
 * @tc.name: loadAndImportScene
 * @tc.desc: Tests for Load And Import Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_ResourceTest, loadAndImportScene, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    IEcs* ecsArray[] = { ecs.get() };

    auto initialEntities = 0;
    for (const auto& e : ecs->GetEntityManager()) {
        CORE_UNUSED(e);
        ++initialEntities;
    }
    constexpr string_view filename = "test://gltf/WaterBottle/WaterBottle.glb";

    auto loader = graphicsContext->GetSceneUtil().GetSceneLoader(filename);
    ASSERT_TRUE(loader);

    auto loadResult = loader->Load(filename);
    ASSERT_TRUE(!loadResult.error);

    {
        auto importer = loader->CreateSceneImporter(*ecs);
        importer->ImportResources(loadResult.data, CORE_IMPORT_RESOURCE_FLAG_BITS_ALL);

        auto importResult = importer->GetResult();
        ASSERT_TRUE(!importResult.error);

        ASSERT_EQ(importResult.data.textures.size(), 4);
        ASSERT_EQ(importResult.data.materials.size(), 1);
        ASSERT_EQ(importResult.data.meshes.size(), 1);

        // by default no cpu access to mesh data.
        const auto& meshData = importer->GetMeshData();
        EXPECT_TRUE(meshData.meshes.empty());

        auto sceneRoot = importer->ImportScene(0);
        EXPECT_TRUE(EntityUtil::IsValid(sceneRoot));

        auto nodeSystem = GetSystem<INodeSystem>(*ecs);
        nodeSystem->DestroyNode(*nodeSystem->GetNode(sceneRoot));
    }
    engine->TickFrame(ecsArray);
    {
        auto entities = 0;
        for (const auto& e : ecs->GetEntityManager()) {
            CORE_UNUSED(e);
            ++entities;
        }
        EXPECT_EQ(initialEntities, entities);
    }
    {
        auto importer = loader->CreateSceneImporter(*ecs);
        importer->ImportResources(loadResult.data, CORE_IMPORT_RESOURCE_FLAG_BITS_ALL, nullptr);
        while (!importer->Execute(0)) {
        }
        auto importResult = importer->GetResult();
        ASSERT_TRUE(!importResult.error);

        ASSERT_EQ(importResult.data.textures.size(), 4);
        ASSERT_EQ(importResult.data.materials.size(), 1);
        ASSERT_EQ(importResult.data.meshes.size(), 1);

        // by default no cpu access to mesh data.
        const auto& meshData = importer->GetMeshData();
        EXPECT_TRUE(meshData.meshes.empty());

        auto sceneRoot = importer->ImportScene(0);
        EXPECT_TRUE(EntityUtil::IsValid(sceneRoot));

        auto nodeSystem = GetSystem<INodeSystem>(*ecs);
        nodeSystem->DestroyNode(*nodeSystem->GetNode(sceneRoot));
    }

    engine->TickFrame(ecsArray);

    auto entities = 0;
    for (const auto& e : ecs->GetEntityManager()) {
        CORE_UNUSED(e);
        ++entities;
    }
    EXPECT_EQ(initialEntities, entities);
}
