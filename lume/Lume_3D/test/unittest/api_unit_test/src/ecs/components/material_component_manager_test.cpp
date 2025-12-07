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

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/uri_component.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>
#include <core/property_tools/core_metadata.inl>
#include <render/device/intf_shader_manager.h>
#include <render/intf_renderer.h>

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

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::MaterialComponent;
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureInfo);
CORE_END_NAMESPACE()

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMaterialComponent, CreateTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    auto componentManager = static_cast<IComponentManager*>(materialManager);
    EXPECT_EQ("MaterialComponent", componentManager->GetName());
    EXPECT_EQ(IMaterialComponentManager::UID, componentManager->GetUid());

    Entity entity = ecs->GetEntityManager().Create();
    uint32_t generation;
    {
        materialManager->Create(entity);
        ASSERT_TRUE(materialManager->HasComponent(entity));
        auto id = materialManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        EXPECT_EQ(IComponentManager::INVALID_COMPONENT_ID, materialManager->GetComponentId(Entity {}));
        ASSERT_EQ(entity, materialManager->GetEntity(id));
        generation = materialManager->GetComponentGeneration(id);
        EXPECT_LE(generation, materialManager->GetGenerationCounter());
        EXPECT_EQ(0u, materialManager->GetComponentGeneration(IComponentManager::INVALID_COMPONENT_ID));
        EXPECT_TRUE(materialManager->GetModifiedFlags() & CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT);
    }
    // Recreate entity
    {
        materialManager->Create(entity);
        ASSERT_TRUE(materialManager->HasComponent(entity));
        auto id = materialManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        ASSERT_EQ(entity, materialManager->GetEntity(id));
        ASSERT_NE(generation, materialManager->GetComponentGeneration(id));
    }
}

/**
 * @tc.name: GetSetTest
 * @tc.desc: Tests for Get Set Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMaterialComponent, GetSetTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);

    {
        Entity entity = ecs->GetEntityManager().Create();
        materialManager->Create(entity);
        MaterialComponent material;
        material.type = MaterialComponent::Type::METALLIC_ROUGHNESS;
        material.alphaCutoff = 0.9f;
        material.extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT;
        material.materialLightingFlags = MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT;
        materialManager->Set(entity, material);
        auto result = materialManager->Get(entity);
        ASSERT_EQ(material.type, result.type);
        ASSERT_EQ(material.alphaCutoff, result.alphaCutoff);
        ASSERT_EQ(material.extraRenderingFlags, result.extraRenderingFlags);
        ASSERT_EQ(material.materialLightingFlags, result.materialLightingFlags);
    }
    // Set without Create
    {
        Entity entity = ecs->GetEntityManager().Create();
        MaterialComponent material;
        material.type = MaterialComponent::Type::METALLIC_ROUGHNESS;
        material.alphaCutoff = 0.9f;
        material.extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT;
        material.materialLightingFlags = MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT;
        materialManager->Set(entity, material);
        auto result = materialManager->Get(entity);
        ASSERT_EQ(material.type, result.type);
        ASSERT_EQ(material.alphaCutoff, result.alphaCutoff);
        ASSERT_EQ(material.extraRenderingFlags, result.extraRenderingFlags);
        ASSERT_EQ(material.materialLightingFlags, result.materialLightingFlags);
    }
    // Set with ComponentId
    {
        Entity entity = ecs->GetEntityManager().Create();
        materialManager->Create(entity);
        auto componentId = materialManager->GetComponentId(entity);
        MaterialComponent material;
        material.type = MaterialComponent::Type::METALLIC_ROUGHNESS;
        material.alphaCutoff = 0.9f;
        material.extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT;
        material.materialLightingFlags = MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT;
        materialManager->Set(componentId, material);
        auto result = materialManager->Get(componentId);
        ASSERT_EQ(material.type, result.type);
        ASSERT_EQ(material.alphaCutoff, result.alphaCutoff);
        ASSERT_EQ(material.extraRenderingFlags, result.extraRenderingFlags);
        ASSERT_EQ(material.materialLightingFlags, result.materialLightingFlags);
    }
    // Get with invalid Entity
    {
        MaterialComponent defaultMaterial {};
        auto result = materialManager->Get(Entity {});
        ASSERT_EQ(defaultMaterial.type, result.type);
        ASSERT_EQ(defaultMaterial.alphaCutoff, result.alphaCutoff);
        ASSERT_EQ(defaultMaterial.extraRenderingFlags, result.extraRenderingFlags);
        ASSERT_EQ(defaultMaterial.materialLightingFlags, result.materialLightingFlags);
    }
    // Get with invalid ComponentId
    {
        MaterialComponent defaultMaterial {};
        auto componentId = materialManager->GetComponentId(Entity {});
        auto result = materialManager->Get(componentId);
        ASSERT_EQ(defaultMaterial.type, result.type);
        ASSERT_EQ(defaultMaterial.alphaCutoff, result.alphaCutoff);
        ASSERT_EQ(defaultMaterial.extraRenderingFlags, result.extraRenderingFlags);
        ASSERT_EQ(defaultMaterial.materialLightingFlags, result.materialLightingFlags);
    }
}

/**
 * @tc.name: ReadWriteTest
 * @tc.desc: Tests for Read Write Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMaterialComponent, ReadWriteTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);

    // Write with Entity
    {
        Entity entity = ecs->GetEntityManager().Create();
        materialManager->Create(entity);
        if (auto scopedHandle = materialManager->Write(entity); scopedHandle) {
            scopedHandle->type = MaterialComponent::Type::METALLIC_ROUGHNESS;
            scopedHandle->alphaCutoff = 0.9f;
            scopedHandle->extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT;
            scopedHandle->materialLightingFlags = MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT;
        }
        if (auto result = materialManager->Read(entity); result) {
            ASSERT_EQ(MaterialComponent::Type::METALLIC_ROUGHNESS, result->type);
            ASSERT_EQ(0.9f, result->alphaCutoff);
            ASSERT_EQ(MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT, result->extraRenderingFlags);
            ASSERT_EQ(MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT, result->materialLightingFlags);
        }
    }
    // Write with ComponentId
    {
        Entity entity = ecs->GetEntityManager().Create();
        materialManager->Create(entity);
        auto componentId = materialManager->GetComponentId(entity);
        if (auto scopedHandle = materialManager->Write(componentId); scopedHandle) {
            scopedHandle->type = MaterialComponent::Type::METALLIC_ROUGHNESS;
            scopedHandle->alphaCutoff = 0.9f;
            scopedHandle->extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT;
            scopedHandle->materialLightingFlags = MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT;
        }
        if (auto result = materialManager->Read(entity); result) {
            ASSERT_EQ(MaterialComponent::Type::METALLIC_ROUGHNESS, result->type);
            ASSERT_EQ(0.9f, result->alphaCutoff);
            ASSERT_EQ(MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT, result->extraRenderingFlags);
            ASSERT_EQ(MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT, result->materialLightingFlags);
        }
    }
    // Read with invalid Entity
    {
        auto result = materialManager->Read(Entity {});
        ASSERT_FALSE(result);
    }
    // Read with invalid ComponentId
    {
        auto componentId = materialManager->GetComponentId(Entity {});
        auto result = materialManager->Read(componentId);
        ASSERT_FALSE(result);
    }
}

/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMaterialComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    auto& propertyApi = materialManager->GetPropertyApi();
    constexpr uint64_t typeHash = BASE_NS::CompileTime::FNV1aHash("MaterialComponent");
    EXPECT_EQ(typeHash, propertyApi.Type());

    {
        auto propertyHandle = propertyApi.Create();
        ASSERT_NE(nullptr, propertyHandle);
        ASSERT_EQ(sizeof(MaterialComponent), propertyHandle->Size());
        {
            auto propertyApi2 = propertyHandle->Owner();
            ASSERT_EQ(propertyApi.Type(), propertyApi2->Type());
            ASSERT_EQ(propertyApi.PropertyCount(), propertyApi2->PropertyCount());
            EXPECT_EQ(propertyApi2->PropertyCount(), propertyApi2->MetaData().size());
            for (uint32_t i = 0; i < propertyApi2->PropertyCount(); ++i) {
                ASSERT_EQ(propertyApi.MetaData(i), propertyApi2->MetaData(i));
            }
            ASSERT_EQ(nullptr, propertyApi2->MetaData(propertyApi2->PropertyCount()));
            auto createdPropertyHandle = propertyApi2->Create();
            EXPECT_TRUE(createdPropertyHandle);
            auto clonedPropertyHandle = propertyApi2->Clone(createdPropertyHandle);
            EXPECT_TRUE(clonedPropertyHandle);
            propertyApi2->Release(clonedPropertyHandle);
            propertyApi2->Release(createdPropertyHandle);
        }
        auto clonedPropertyHandle = propertyApi.Clone(propertyHandle);
        EXPECT_TRUE(clonedPropertyHandle);
        propertyApi.Release(clonedPropertyHandle);
        propertyApi.Release(propertyHandle);
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

/**
 * @tc.name: GetSetDataTest
 * @tc.desc: Tests for Get Set Data Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMaterialComponent, GetSetDataTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);

    {
        Entity entity1 = ecs->GetEntityManager().Create();
        materialManager->Create(entity1);
        MaterialComponent material;
        material.type = MaterialComponent::Type::METALLIC_ROUGHNESS;
        material.alphaCutoff = 0.9f;
        material.extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT;
        material.materialLightingFlags = MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT;
        materialManager->Set(entity1, material);
        auto propertyHandle = materialManager->GetData(entity1);
        ASSERT_NE(nullptr, propertyHandle);

        // SetData with Entity
        {
            Entity entity2 = ecs->GetEntityManager().Create();
            materialManager->Create(entity2);
            materialManager->SetData(entity2, *materialManager->GetData(entity1));
            auto result = materialManager->Get(entity2);
            ASSERT_EQ(material.type, result.type);
            ASSERT_EQ(material.alphaCutoff, result.alphaCutoff);
            ASSERT_EQ(material.extraRenderingFlags, result.extraRenderingFlags);
            ASSERT_EQ(material.materialLightingFlags, result.materialLightingFlags);
        }
        // SetData with ComponentId
        {
            Entity entity2 = ecs->GetEntityManager().Create();
            materialManager->Create(entity2);
            auto componentId = materialManager->GetComponentId(entity2);
            materialManager->SetData(componentId, *materialManager->GetData(entity1));
            auto result = materialManager->Get(entity2);
            ASSERT_EQ(material.type, result.type);
            ASSERT_EQ(material.alphaCutoff, result.alphaCutoff);
            ASSERT_EQ(material.extraRenderingFlags, result.extraRenderingFlags);
            ASSERT_EQ(material.materialLightingFlags, result.materialLightingFlags);
        }
        // GetData with invalid ComponentId
        {
            auto componentId = materialManager->GetComponentId(Entity {});
            ASSERT_EQ(nullptr, materialManager->GetData(componentId));
        }
    }
}

namespace {
CORE_NS::EntityReference CreateShader(CORE_NS::IEcs& ecs, RENDER_NS::IRenderContext& renderContext, BASE_NS::string uri)
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

class ComponentListener final : IEcs::ComponentListener {
public:
    ComponentListener(IEcs& ecs, IComponentManager* manager) : ecs_ { ecs }, manager_ { manager }
    {
        if (manager) {
            ecs.AddListener(*manager, *this);
        } else {
            ecs.AddListener(*this);
        }
    }

    ~ComponentListener()
    {
        if (manager_) {
            ecs_.RemoveListener(*manager_, *this);
        } else {
            ecs_.RemoveListener(*this);
        }
    }

    void OnComponentEvent(
        EventType type, const IComponentManager& componentManager, const array_view<const Entity> entities)
    {
        events_.push_back(Event { type, componentManager, vector<Entity> { entities.begin(), entities.end() } });
    }

    IEcs& ecs_;
    IComponentManager* manager_;
    struct Event {
        EventType type;
        const IComponentManager& manager;
        vector<Entity> entities;
    };
    vector<Event> events_;
};

} // namespace

/**
 * @tc.name: CustomPropertyMetadataTest
 * @tc.desc: Tests for Custom Property Metadata Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMaterialComponent, CustomPropertyMetadataTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto validateCustomProperties = [](IMaterialComponentManager* materialManager, Entity entity) {
        auto propertyApi = materialManager->GetData(entity)->Owner();
        if (auto scopedHandle = materialManager->Read(entity); scopedHandle) {
            ASSERT_TRUE(scopedHandle->customProperties);
            auto customMetaData = scopedHandle->customProperties->Owner()->MetaData();
            EXPECT_EQ(propertyApi->MetaData().size(), propertyApi->PropertyCount());
            const auto* buffer = static_cast<const uint8_t*>(scopedHandle->customProperties->RLock());
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
            if (auto* cb = scopedHandle->customBindingProperties; cb) {
                {
                    const EntityReference entRef({ 3U }, nullptr);
                    const bool boolVal =
                        CORE_NS::SetPropertyValue(*cb, "uUserCustomImage", PropertyType::ENTITY_REFERENCE_T, entRef);
                    EXPECT_EQ(boolVal, true);
                    const auto entVal = CORE_NS::GetPropertyValue<EntityReference>(*cb, "uUserCustomImage");
                    EXPECT_EQ(entVal.operator CORE_NS::Entity(), Entity { 3U });
                }
                {
                    const EntityReference entRef({ 9U }, nullptr);
                    const bool boolVal =
                        CORE_NS::SetPropertyValue(*cb, "uUserCustomSampler", PropertyType::ENTITY_REFERENCE_T, entRef);
                    EXPECT_EQ(boolVal, true);
                    const auto entVal = CORE_NS::GetPropertyValue<EntityReference>(*cb, "uUserCustomSampler");
                    EXPECT_EQ(entVal.operator CORE_NS::Entity(), Entity { 9U });
                }
            }

            EXPECT_TRUE(propertyApi->MetaData(0u));
            EXPECT_FALSE(propertyApi->MetaData(~0u));
            scopedHandle->customProperties->RUnlock();
        }
    };

    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    auto listener = ComponentListener(*ecs, materialManager);

    const Entity entity0 = ecs->GetEntityManager().Create();
    materialManager->Create(entity0);
    if (auto scopedHandle = materialManager->Write(entity0); scopedHandle) {
        scopedHandle->materialShader.shader = CreateShader(*ecs, *renderContext, "test://shader/test.shader");
    }

    const Entity entity = ecs->GetEntityManager().Create();
    materialManager->Create(entity);
    if (auto scopedHandle = materialManager->Write(entity); scopedHandle) {
        scopedHandle->materialShader.shader = CreateShader(*ecs, *renderContext, "test://shader/test.shader");
    }

    const MaterialComponent defaultMaterial;
    const auto baseColorTexture = ecs->GetEntityManager().CreateReferenceCounted();
    const auto normalMapTexture = ecs->GetEntityManager().CreateReferenceCounted();
    const auto materialTexture = ecs->GetEntityManager().CreateReferenceCounted();
    const auto emissiveTexture = ecs->GetEntityManager().CreateReferenceCounted();
    const auto specularTexture = ecs->GetEntityManager().CreateReferenceCounted();
    const auto baseColorFactor = Math::Vec4(0.1f, 0.1f, 0.1f, 1.f);
    const auto normalFactor = Math::Vec4(0.5f, 0.0f, 0.0f, 0.0f);
    const auto materialFactor = Math::Vec4(0.0f, 0.5f, 0.9f, 0.f);
    const auto emissiveFactor = Math::Vec4(0.8f, 0.6f, 0.4f, 10.f);
    const auto specularFactor = Math::Vec4(0.9f, 0.7f, 0.5f, 10.f);
    // update texture slots
    auto updateTextureSlots = [&]() {
        auto propertyHandle = materialManager->GetData(entity);
        if (auto baseColor = CORE_NS::MakeScopedHandle<MaterialComponent::TextureInfo>(*propertyHandle, "baseColor");
            baseColor) {
            baseColor->image = baseColorTexture;
            baseColor->factor = baseColorFactor;
        }
        if (auto normal = CORE_NS::MakeScopedHandle<MaterialComponent::TextureInfo>(*propertyHandle, "normal");
            normal) {
            normal->image = normalMapTexture;
            normal->factor = normalFactor;
        }
        if (auto material = CORE_NS::MakeScopedHandle<MaterialComponent::TextureInfo>(*propertyHandle, "material");
            material) {
            material->image = materialTexture;
            material->factor = materialFactor;
        }
        if (auto emissive = CORE_NS::MakeScopedHandle<MaterialComponent::TextureInfo>(*propertyHandle, "emissive");
            emissive) {
            emissive->image = emissiveTexture;
            emissive->factor = emissiveFactor;
        }
        if (auto specular = CORE_NS::MakeScopedHandle<MaterialComponent::TextureInfo>(*propertyHandle, "specular");
            specular) {
            specular->image = specularTexture;
            specular->factor = specularFactor;
        }
    };

    updateTextureSlots();
    // check that the "textures" ended up in the expected slots accroding to the .shader file
    if (auto material = materialManager->Read(entity); material) {
        EXPECT_EQ(material->textures[0].image, baseColorTexture);
        EXPECT_EQ(material->textures[1].image, normalMapTexture);
        EXPECT_EQ(material->textures[2].image, materialTexture);
        EXPECT_EQ(material->textures[3].image, emissiveTexture);
        EXPECT_FALSE(material->textures[5].image);
        EXPECT_FALSE(material->textures[6].image);
        EXPECT_FALSE(material->textures[7].image);
        EXPECT_FALSE(material->textures[8].image);
        EXPECT_FALSE(material->textures[9].image);
        EXPECT_EQ(material->textures[10].image, specularTexture);
    }

    validateCustomProperties(materialManager, entity);

    // switch to another shader with values in different order
    if (auto scopedHandle = materialManager->Write(entity); scopedHandle) {
        scopedHandle->materialShader.shader = CreateShader(*ecs, *renderContext, "test://shader/test2.shader");
    }

    // switching to new shader shouldn't remap texture slots
    if (auto material = materialManager->Read(entity)) {
        EXPECT_EQ(material->textures[0].image, baseColorTexture);
        EXPECT_EQ(material->textures[0].factor, baseColorFactor);
        EXPECT_EQ(material->textures[1].image, normalMapTexture);
        EXPECT_EQ(material->textures[1].factor, normalFactor);
        EXPECT_EQ(material->textures[2].image, materialTexture);
        EXPECT_EQ(material->textures[2].factor, materialFactor);
        EXPECT_EQ(material->textures[3].image, emissiveTexture);
        EXPECT_EQ(material->textures[3].factor, emissiveFactor);
        EXPECT_FALSE(material->textures[5].image);
        EXPECT_FALSE(material->textures[6].image);
        EXPECT_FALSE(material->textures[7].image);
        EXPECT_FALSE(material->textures[8].image);
        EXPECT_FALSE(material->textures[9].image);
        EXPECT_EQ(material->textures[10].image, specularTexture);
        EXPECT_EQ(material->textures[10].factor, specularFactor);
    }

    // move values to correct slots for test2.shader
    updateTextureSlots();

    validateCustomProperties(materialManager, entity);

    renderContext->GetRenderer().RenderFrame({});

    // switch to same shader after it has been recompiled
    if (auto scopedHandle = materialManager->Write(entity); scopedHandle) {
        auto& shaderMgr = renderContext->GetDevice().GetShaderManager();
        shaderMgr.ReloadShaderFile("test://shader/test2.shader");
        scopedHandle->materialShader.shader = CreateShader(*ecs, *renderContext, "test://shader/test2.shader");
    }

    // switching to same shader should reset unused slots
    if (auto material = materialManager->Read(entity)) {
        EXPECT_EQ(material->textures[0].image, normalMapTexture);
        EXPECT_EQ(material->textures[0].factor, normalFactor);
        EXPECT_EQ(material->textures[1].image, materialTexture);
        EXPECT_EQ(material->textures[1].factor, materialFactor);
        EXPECT_EQ(material->textures[2].image, baseColorTexture);
        EXPECT_EQ(material->textures[2].factor, baseColorFactor);
        EXPECT_FALSE(material->textures[3].image);
        EXPECT_EQ(material->textures[3].factor, MaterialComponent::DEFAULT_EMISSIVE);
        EXPECT_EQ(material->textures[4].image, emissiveTexture);
        EXPECT_EQ(material->textures[4].factor, emissiveFactor);
        EXPECT_FALSE(material->textures[5].image);
        EXPECT_EQ(material->textures[5].factor, MaterialComponent::DEFAULT_CLEARCOAT);
        EXPECT_FALSE(material->textures[6].image);
        EXPECT_EQ(material->textures[6].factor, MaterialComponent::DEFAULT_CLEARCOAT_ROUGHNESS);
        EXPECT_FALSE(material->textures[7].image);
        EXPECT_EQ(material->textures[7].factor, MaterialComponent::DEFAULT_CLEARCOAT_NORMAL);
        EXPECT_FALSE(material->textures[8].image);
        EXPECT_EQ(material->textures[8].factor, MaterialComponent::DEFAULT_SHEEN);
        EXPECT_FALSE(material->textures[9].image);
        EXPECT_EQ(material->textures[9].factor, MaterialComponent::DEFAULT_TRANSMISSION);
        EXPECT_FALSE(material->textures[10].image);
        EXPECT_EQ(material->textures[10].factor, MaterialComponent::DEFAULT_SPECULAR);
    }
    validateCustomProperties(materialManager, entity);

    ecs->ProcessEvents();
    EXPECT_EQ(listener.events_.size(), 2U); // expected CREATED and MODIFIED events
    EXPECT_EQ(listener.events_[0U].type, IEcs::ComponentListener::EventType::CREATED);
    EXPECT_EQ(listener.events_[0U].entities[0], entity0);
    EXPECT_EQ(listener.events_[0U].entities[1], entity);
    EXPECT_EQ(listener.events_[1U].type, IEcs::ComponentListener::EventType::MODIFIED);
    EXPECT_EQ(listener.events_[1U].entities[0], entity0);
    EXPECT_EQ(listener.events_[1U].entities[1], entity);
    listener.events_.clear();
    ecs->GetEntityManager().Destroy(entity0);

    ecs->ProcessEvents();
    EXPECT_EQ(listener.events_.size(), 2U); // expected MOVED and DESTROYED events
    EXPECT_EQ(listener.events_[0U].type, IEcs::ComponentListener::EventType::MOVED);
    EXPECT_EQ(listener.events_[0U].entities[0], entity);
    EXPECT_EQ(listener.events_[1U].type, IEcs::ComponentListener::EventType::DESTROYED);
    EXPECT_EQ(listener.events_[1U].entities[0], entity0);
    listener.events_.clear();

    // modify custom values and change to the first shader
    if (auto scopedHandle = materialManager->Read(entity); scopedHandle) {
        if (scopedHandle->customProperties) {
            CORE_NS::SetPropertyValue(*scopedHandle->customProperties, "f_", 2.f);
            CORE_NS::SetPropertyValue(*scopedHandle->customProperties, "u_", 42U);
            CORE_NS::SetPropertyValue(*scopedHandle->customProperties, "v4_", Math::Vec4(0.3f, 0.4f, 0.1f, 0.2f));
            CORE_NS::SetPropertyValue(*scopedHandle->customProperties, "uv4_", Math::UVec4(8U, 9U, 10U, 11U));
        }
    }
    ecs->ProcessEvents();
    EXPECT_EQ(listener.events_.size(), 1U); // expecting one MODIFIED event
    EXPECT_EQ(listener.events_[0U].type, IEcs::ComponentListener::EventType::MODIFIED);
    EXPECT_EQ(listener.events_[0U].entities[0], entity);
    listener.events_.clear();

    auto validate2 = [](IMaterialComponentManager* materialManager, Entity entity) {
        auto propertyApi = materialManager->GetData(entity)->Owner();
        if (auto scopedHandle = materialManager->Read(entity); scopedHandle) {
            ASSERT_TRUE(scopedHandle->customProperties);
            auto customMetaData = scopedHandle->customProperties->Owner()->MetaData();
            EXPECT_EQ(propertyApi->MetaData().size(), propertyApi->PropertyCount());
            const auto* buffer = static_cast<const uint8_t*>(scopedHandle->customProperties->RLock());
            for (const auto& meta : customMetaData) {
                switch (meta.type) {
                    case PropertyType::FLOAT_T: {
                        const auto value = *static_cast<const float*>(static_cast<const void*>(buffer + meta.offset));
                        EXPECT_EQ(meta.name, "f_");
                        EXPECT_EQ(value, 2.f);
                        break;
                    }
                    case PropertyType::UINT32_T: {
                        const auto value =
                            *static_cast<const uint32_t*>(static_cast<const void*>(buffer + meta.offset));
                        EXPECT_EQ(meta.name, "u_");
                        EXPECT_EQ(value, 42U);
                        break;
                    }
                    case PropertyType::VEC4_T: {
                        const auto value =
                            *static_cast<const Math::Vec4*>(static_cast<const void*>(buffer + meta.offset));
                        EXPECT_EQ(meta.name, "v4_");
                        EXPECT_EQ(value, Math::Vec4(0.3f, 0.4f, 0.1f, 0.2f));
                        break;
                    }
                    case PropertyType::UVEC4_T: {
                        const auto value =
                            *static_cast<const Math::UVec4*>(static_cast<const void*>(buffer + meta.offset));
                        EXPECT_EQ(meta.name, "uv4_");
                        EXPECT_EQ(value, Math::UVec4(8U, 9U, 10U, 11U));
                        break;
                    }
                }
            }
            EXPECT_TRUE(propertyApi->MetaData(0u));
            EXPECT_FALSE(propertyApi->MetaData(~0u));
            scopedHandle->customProperties->RUnlock();
        }
    };
    validate2(materialManager, entity);
}
