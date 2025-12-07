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

#include <3d/ecs/components/post_process_configuration_component.h>
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
#include <render/intf_render_context.h>

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

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessConfigurationComponent, CreateTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto ppConfManager = GetManager<IPostProcessConfigurationComponentManager>(*ecs);
    auto componentManager = static_cast<IComponentManager*>(ppConfManager);
    EXPECT_EQ("PostProcessConfigurationComponent", componentManager->GetName());
    EXPECT_EQ(IPostProcessConfigurationComponentManager::UID, componentManager->GetUid());
    EXPECT_EQ(ecs.get(), &componentManager->GetEcs());

    Entity entity = ecs->GetEntityManager().Create();
    uint32_t generation;
    {
        ppConfManager->Create(entity);
        ASSERT_TRUE(ppConfManager->HasComponent(entity));
        auto id = ppConfManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        EXPECT_EQ(IComponentManager::INVALID_COMPONENT_ID, ppConfManager->GetComponentId(Entity {}));
        ASSERT_EQ(entity, ppConfManager->GetEntity(id));
        generation = ppConfManager->GetComponentGeneration(id);
        EXPECT_LE(generation, ppConfManager->GetGenerationCounter());
        EXPECT_EQ(0u, ppConfManager->GetComponentGeneration(IComponentManager::INVALID_COMPONENT_ID));
        EXPECT_TRUE(ppConfManager->GetModifiedFlags() & CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT);
    }
    // Recreate entity
    {
        ppConfManager->Create(entity);
        ASSERT_TRUE(ppConfManager->HasComponent(entity));
        auto id = ppConfManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        ASSERT_EQ(entity, ppConfManager->GetEntity(id));
        ASSERT_NE(generation, ppConfManager->GetComponentGeneration(id));
    }
}

/**
 * @tc.name: GetSetTest
 * @tc.desc: Tests for Get Set Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessConfigurationComponent, GetSetTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto ppConfManager = GetManager<IPostProcessConfigurationComponentManager>(*ecs);

    {
        Entity entity = ecs->GetEntityManager().Create();
        ppConfManager->Create(entity);
        PostProcessConfigurationComponent ppConf;
        ppConf.postProcesses.resize(5);
        ppConf.postProcesses[0].enabled = true;
        ppConf.postProcesses[0].factor = Math::Vec4 { 1.0f, 0.5f, 0.3f, 1.0f };
        ppConf.postProcesses[0].flags = 15u;
        ppConf.postProcesses[0].name = "Effect0";
        ppConf.customRenderNodeGraphFile = "RngFile";
        ppConfManager->Set(entity, ppConf);
        auto result = ppConfManager->Get(entity);
        ASSERT_EQ(ppConf.postProcesses.size(), result.postProcesses.size());
        ASSERT_EQ(ppConf.postProcesses[0].enabled, result.postProcesses[0].enabled);
        ASSERT_EQ(ppConf.postProcesses[0].factor, result.postProcesses[0].factor);
        ASSERT_EQ(ppConf.postProcesses[0].flags, result.postProcesses[0].flags);
        ASSERT_EQ(ppConf.postProcesses[0].name, result.postProcesses[0].name);
        ASSERT_EQ(ppConf.customRenderNodeGraphFile, result.customRenderNodeGraphFile);
    }
    // Set without Create
    {
        Entity entity = ecs->GetEntityManager().Create();
        PostProcessConfigurationComponent ppConf;
        ppConf.postProcesses.resize(5);
        ppConf.postProcesses[0].enabled = true;
        ppConf.postProcesses[0].factor = Math::Vec4 { 1.0f, 0.5f, 0.3f, 1.0f };
        ppConf.postProcesses[0].flags = 15u;
        ppConf.postProcesses[0].name = "Effect0";
        ppConf.customRenderNodeGraphFile = "RngFile";
        ppConfManager->Set(entity, ppConf);
        auto result = ppConfManager->Get(entity);
        ASSERT_EQ(ppConf.postProcesses.size(), result.postProcesses.size());
        ASSERT_EQ(ppConf.postProcesses[0].enabled, result.postProcesses[0].enabled);
        ASSERT_EQ(ppConf.postProcesses[0].factor, result.postProcesses[0].factor);
        ASSERT_EQ(ppConf.postProcesses[0].flags, result.postProcesses[0].flags);
        ASSERT_EQ(ppConf.postProcesses[0].name, result.postProcesses[0].name);
        ASSERT_EQ(ppConf.customRenderNodeGraphFile, result.customRenderNodeGraphFile);
    }
    // Set with ComponentId
    {
        Entity entity = ecs->GetEntityManager().Create();
        ppConfManager->Create(entity);
        auto componentId = ppConfManager->GetComponentId(entity);
        PostProcessConfigurationComponent ppConf;
        ppConf.postProcesses.resize(5);
        ppConf.postProcesses[0].enabled = true;
        ppConf.postProcesses[0].factor = Math::Vec4 { 1.0f, 0.5f, 0.3f, 1.0f };
        ppConf.postProcesses[0].flags = 15u;
        ppConf.postProcesses[0].name = "Effect0";
        ppConf.customRenderNodeGraphFile = "RngFile";
        ppConfManager->Set(componentId, ppConf);
        auto result = ppConfManager->Get(componentId);
        ASSERT_EQ(ppConf.postProcesses.size(), result.postProcesses.size());
        ASSERT_EQ(ppConf.postProcesses[0].enabled, result.postProcesses[0].enabled);
        ASSERT_EQ(ppConf.postProcesses[0].factor, result.postProcesses[0].factor);
        ASSERT_EQ(ppConf.postProcesses[0].flags, result.postProcesses[0].flags);
        ASSERT_EQ(ppConf.postProcesses[0].name, result.postProcesses[0].name);
        ASSERT_EQ(ppConf.customRenderNodeGraphFile, result.customRenderNodeGraphFile);
    }
    // Get with invalid Entity
    {
        PostProcessConfigurationComponent defaultConf {};
        auto result = ppConfManager->Get(Entity {});
        ASSERT_EQ(defaultConf.postProcesses.size(), result.postProcesses.size());
        ASSERT_EQ(defaultConf.customRenderNodeGraphFile, result.customRenderNodeGraphFile);
    }
    // Get with invalid ComponentId
    {
        PostProcessConfigurationComponent defaultConf {};
        auto componentId = ppConfManager->GetComponentId(Entity {});
        auto result = ppConfManager->Get(componentId);
        ASSERT_EQ(defaultConf.postProcesses.size(), result.postProcesses.size());
        ASSERT_EQ(defaultConf.customRenderNodeGraphFile, result.customRenderNodeGraphFile);
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
} // namespace

/**
 * @tc.name: CustomPropertyMetadataTest
 * @tc.desc: Tests for Custom Property Metadata Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessConfigurationComponent, CustomPropertyMetadataTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto validate = [](IPostProcessConfigurationComponentManager* ppConfManager, Entity entity) {
        if (auto scopedHandle = ppConfManager->Read(entity); scopedHandle) {
            ASSERT_EQ(1, scopedHandle->postProcesses.size());
            ASSERT_TRUE(scopedHandle->postProcesses[0].customProperties);
            auto customMetaData = scopedHandle->postProcesses[0].customProperties->Owner()->MetaData();
            const auto* buffer = static_cast<const uint8_t*>(scopedHandle->postProcesses[0].customProperties->RLock());
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
                        EXPECT_EQ(meta.name, "u_");
                        EXPECT_EQ(value, 7U);
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
            scopedHandle->postProcesses[0].customProperties->RUnlock();
        }
    };
    auto ppConfManager = GetManager<IPostProcessConfigurationComponentManager>(*ecs);

    Entity entity = ecs->GetEntityManager().Create();
    ppConfManager->Create(entity);
    if (auto scopedHandle = ppConfManager->Write(entity); scopedHandle) {
        scopedHandle->postProcesses.resize(1);
        scopedHandle->postProcesses[0].shader = CreateShader(*ecs, *renderContext, "test://shader/test.shader");
    }
    validate(ppConfManager, entity);

    // switch to another shader with values in different order
    if (auto scopedHandle = ppConfManager->Write(entity); scopedHandle) {
        scopedHandle->postProcesses[0].shader = CreateShader(*ecs, *renderContext, "test://shader/test2.shader");
    }
    validate(ppConfManager, entity);

    // modify custom values and change to the first shader
    if (auto scopedHandle = ppConfManager->Write(entity); scopedHandle) {
        auto* customProperties =
            (!scopedHandle->postProcesses.empty()) ? scopedHandle->postProcesses[0].customProperties : nullptr;
        ASSERT_TRUE(customProperties);
        CORE_NS::SetPropertyValue(*customProperties, "f_", 2.f);
        CORE_NS::SetPropertyValue(*customProperties, "u_", 42U);
        CORE_NS::SetPropertyValue(*customProperties, "v4_", Math::Vec4(0.3f, 0.4f, 0.1f, 0.2f));
        CORE_NS::SetPropertyValue(*customProperties, "uv4_", Math::UVec4(8U, 9U, 10U, 11U));
    }

    auto validate2 = [](IPostProcessConfigurationComponentManager* ppConfManager, Entity entity) {
        if (auto scopedHandle = ppConfManager->Read(entity); scopedHandle) {
            ASSERT_EQ(1, scopedHandle->postProcesses.size());
            ASSERT_TRUE(scopedHandle->postProcesses[0].customProperties);
            auto customMetaData = scopedHandle->postProcesses[0].customProperties->Owner()->MetaData();
            const auto* buffer = static_cast<const uint8_t*>(scopedHandle->postProcesses[0].customProperties->RLock());
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
                    } break;
                }
            }

            scopedHandle->postProcesses[0].customProperties->RUnlock();
        }
    };
    validate2(ppConfManager, entity);
}

/**
 * @tc.name: ReadWriteTest
 * @tc.desc: Tests for Read Write Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessConfigurationComponent, ReadWriteTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto ppConfManager = GetManager<IPostProcessConfigurationComponentManager>(*ecs);

    // Write with Entity
    {
        Entity entity = ecs->GetEntityManager().Create();
        ppConfManager->Create(entity);
        if (auto scopedHandle = ppConfManager->Write(entity); scopedHandle) {
            scopedHandle->postProcesses.resize(5);
            scopedHandle->postProcesses[0].enabled = true;
            scopedHandle->postProcesses[0].factor = Math::Vec4 { 1.0f, 0.5f, 0.3f, 1.0f };
            scopedHandle->postProcesses[0].flags = 15u;
            scopedHandle->postProcesses[0].name = "Effect0";
            scopedHandle->customRenderNodeGraphFile = "RngFile";
        }
        if (auto result = ppConfManager->Read(entity); result) {
            ASSERT_EQ(5, result->postProcesses.size());
            ASSERT_EQ(true, result->postProcesses[0].enabled);
            ASSERT_EQ(1.0f, result->postProcesses[0].factor[0]);
            ASSERT_EQ(0.5f, result->postProcesses[0].factor[1]);
            ASSERT_EQ(0.3f, result->postProcesses[0].factor[2]);
            ASSERT_EQ(1.0f, result->postProcesses[0].factor[3]);
            ASSERT_EQ(15u, result->postProcesses[0].flags);
            ASSERT_EQ("Effect0", result->postProcesses[0].name);
            ASSERT_EQ("RngFile", result->customRenderNodeGraphFile);
        }
    }
    // Write with ComponentId
    {
        Entity entity = ecs->GetEntityManager().Create();
        ppConfManager->Create(entity);
        auto componentId = ppConfManager->GetComponentId(entity);
        if (auto scopedHandle = ppConfManager->Write(componentId); scopedHandle) {
            scopedHandle->postProcesses.resize(5);
            scopedHandle->postProcesses[0].enabled = true;
            scopedHandle->postProcesses[0].factor = Math::Vec4 { 1.0f, 0.5f, 0.3f, 1.0f };
            scopedHandle->postProcesses[0].flags = 15u;
            scopedHandle->postProcesses[0].name = "Effect0";
            scopedHandle->customRenderNodeGraphFile = "RngFile";
        }
        if (auto result = ppConfManager->Read(entity); result) {
            ASSERT_EQ(5, result->postProcesses.size());
            ASSERT_EQ(true, result->postProcesses[0].enabled);
            ASSERT_EQ(1.0f, result->postProcesses[0].factor[0]);
            ASSERT_EQ(0.5f, result->postProcesses[0].factor[1]);
            ASSERT_EQ(0.3f, result->postProcesses[0].factor[2]);
            ASSERT_EQ(1.0f, result->postProcesses[0].factor[3]);
            ASSERT_EQ(15u, result->postProcesses[0].flags);
            ASSERT_EQ("Effect0", result->postProcesses[0].name);
            ASSERT_EQ("RngFile", result->customRenderNodeGraphFile);
        }
    }
    // Read with invalid Entity
    {
        auto result = ppConfManager->Read(Entity {});
        ASSERT_FALSE(result);
    }
    // Read with invalid ComponentId
    {
        auto componentId = ppConfManager->GetComponentId(Entity {});
        auto result = ppConfManager->Read(componentId);
        ASSERT_FALSE(result);
    }
}

/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsPostProcessConfigurationComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto ppConfManager = GetManager<IPostProcessConfigurationComponentManager>(*ecs);
    auto& propertyApi = ppConfManager->GetPropertyApi();
    constexpr BASE_NS::string_view ppConfCompName = "PostProcessConfigurationComponent";
    constexpr uint64_t typeHash = BASE_NS::CompileTime::FNV1aHash(ppConfCompName.data(), ppConfCompName.size());
    EXPECT_EQ(typeHash, propertyApi.Type());

    {
        auto propertyHandle = propertyApi.Create();
        ASSERT_NE(nullptr, propertyHandle);
        ASSERT_EQ(sizeof(PostProcessConfigurationComponent), propertyHandle->Size());
        {
            auto propertyApi2 = propertyHandle->Owner();
            ASSERT_EQ(propertyApi.Type(), propertyApi2->Type());
            ASSERT_EQ(propertyApi.PropertyCount(), propertyApi2->PropertyCount());
            for (uint32_t i = 0; i < propertyApi2->PropertyCount(); ++i) {
                ASSERT_EQ(propertyApi.MetaData(i), propertyApi2->MetaData(i));
            }
            ASSERT_EQ(nullptr, propertyApi2->MetaData(propertyApi2->PropertyCount()));
            ASSERT_EQ(propertyApi2->PropertyCount(), propertyApi2->MetaData().size());
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
UNIT_TEST(API_EcsPostProcessConfigurationComponent, GetSetDataTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto ppConfManager = GetManager<IPostProcessConfigurationComponentManager>(*ecs);

    {
        Entity entity1 = ecs->GetEntityManager().Create();
        ppConfManager->Create(entity1);
        PostProcessConfigurationComponent ppConf;
        ppConf.postProcesses.resize(5);
        ppConf.postProcesses[0].enabled = true;
        ppConf.postProcesses[0].factor = Math::Vec4 { 1.0f, 0.5f, 0.3f, 1.0f };
        ppConf.postProcesses[0].flags = 15u;
        ppConf.postProcesses[0].name = "Effect0";
        ppConf.customRenderNodeGraphFile = "RngFile";
        ppConfManager->Set(entity1, ppConf);
        auto propertyHandle = ppConfManager->GetData(entity1);
        ASSERT_NE(nullptr, propertyHandle);

        // SetData with Entity
        {
            Entity entity2 = ecs->GetEntityManager().Create();
            ppConfManager->Create(entity2);
            ppConfManager->SetData(entity2, *ppConfManager->GetData(entity1));
            auto result = ppConfManager->Get(entity2);
            ASSERT_EQ(ppConf.postProcesses.size(), result.postProcesses.size());
            ASSERT_EQ(ppConf.postProcesses[0].enabled, result.postProcesses[0].enabled);
            ASSERT_EQ(ppConf.postProcesses[0].factor, result.postProcesses[0].factor);
            ASSERT_EQ(ppConf.postProcesses[0].flags, result.postProcesses[0].flags);
            ASSERT_EQ(ppConf.postProcesses[0].name, result.postProcesses[0].name);
            ASSERT_EQ(ppConf.customRenderNodeGraphFile, result.customRenderNodeGraphFile);
        }
        // SetData with ComponentId
        {
            Entity entity2 = ecs->GetEntityManager().Create();
            ppConfManager->Create(entity2);
            auto componentId = ppConfManager->GetComponentId(entity2);
            ppConfManager->SetData(componentId, *ppConfManager->GetData(entity1));
            auto result = ppConfManager->Get(entity2);
            ASSERT_EQ(ppConf.postProcesses.size(), result.postProcesses.size());
            ASSERT_EQ(ppConf.postProcesses[0].enabled, result.postProcesses[0].enabled);
            ASSERT_EQ(ppConf.postProcesses[0].factor, result.postProcesses[0].factor);
            ASSERT_EQ(ppConf.postProcesses[0].flags, result.postProcesses[0].flags);
            ASSERT_EQ(ppConf.postProcesses[0].name, result.postProcesses[0].name);
            ASSERT_EQ(ppConf.customRenderNodeGraphFile, result.customRenderNodeGraphFile);
        }
        // GetData with invalid ComponentId
        {
            auto componentId = ppConfManager->GetComponentId(Entity {});
            ASSERT_EQ(nullptr, ppConfManager->GetData(componentId));
        }
    }
}
