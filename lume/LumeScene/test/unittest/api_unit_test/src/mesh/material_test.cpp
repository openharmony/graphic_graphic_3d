
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

// clang-format off
#include <core/property_tools/core_metadata.inl>
// clang-format on
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_ecs.h>
#include <core/log.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_handle_util.h>

#include <meta/api/event_handler.h>
#include <meta/interface/intf_named.h>

#include "scene/scene_test.h"

using CORE3D_NS::MaterialComponent;

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginMaterialTest : public ScenePluginTest {
public:
    // Create a bunch of textures that each contain a unique magic value.
    BASE_NS::vector<MaterialComponent::TextureInfo> CreateTestTextures() const
    {
        auto ret = BASE_NS::vector<MaterialComponent::TextureInfo>();
        for (int i = 0; i < MaterialComponent::TextureIndex::TEXTURE_COUNT; ++i) {
            auto texture = MaterialComponent::TextureInfo {};
            texture.factor.data[0] = MAGIC_VALUE + i;
            ret.emplace_back(texture);
        }
        return ret;
    }

    bool AssignTextures(CORE_NS::Entity entity, const BASE_NS::vector<MaterialComponent::TextureInfo>& textures) const
    {
        const auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
        if (const auto componentManager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs)) {
            if (const auto targetHandle = componentManager->Write(entity)) {
                for (int i = 0; i < MaterialComponent::TextureIndex::TEXTURE_COUNT; ++i) {
                    targetHandle->textures[i] = textures[i];
                }
                return true;
            }
        }
        return false;
    }

    // Add as a custom property a whole material component that contains a magic value.
    bool AddCustomProperty(CORE_NS::Entity entity) const
    {
        const auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
        if (const auto componentManager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs)) {
            if (const auto targetHandle = componentManager->Write(entity)) {
                const auto propEntity = ecs->GetEntityManager().CreateReferenceCounted();
                componentManager->Create(propEntity);
                if (const auto propHandle = componentManager->Write(propEntity)) {
                    // Use alpha cutoff, since it's a simple float.
                    propHandle->alphaCutoff = MAGIC_VALUE;
                    targetHandle->customProperties = componentManager->GetData(propEntity);
                    return true;
                }
            }
        }
        return false;
    }
    const float MAGIC_VALUE { 1.2345f };
};

/**
 * @tc.name: DynamicProperties
 * @tc.desc: Tests for Dynamic Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, DynamicProperties, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto material = CreateMaterial();
    auto metadata = interface_cast<META_NS::IMetadata>(material);
    ASSERT_TRUE(metadata);
    for (auto&& propName : { "MaterialShader", "DepthShader" }) {
        auto prop = metadata->GetProperty<IShader::Ptr>(propName);
        ASSERT_TRUE(prop);
        auto shader = prop->GetValue();
        EXPECT_TRUE(shader);
        {
            auto renderRes = interface_cast<IRenderResource>(shader);
            // default shader does not have render handle set, only the graphics state
            EXPECT_FALSE(renderRes->GetRenderHandle());
        }
        shader = man->LoadShader("test://shaders/test.shader").GetResult();
        ASSERT_TRUE(shader);
        {
            auto renderRes = interface_cast<IRenderResource>(shader);
            EXPECT_TRUE(renderRes->GetRenderHandle());
        }
        prop->SetValue(shader);
        EXPECT_EQ(prop->GetValue(), shader);
    }
}

/**
 * @tc.name: ShaderConverterToTargetType
 * @tc.desc: Tests for Shader Converter To Target Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, ShaderConverterToTargetType, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto material = CreateMaterial();
    auto shader = man->LoadShader("test://shaders/test.shader").GetResult();
    ASSERT_TRUE(shader);

    material->MaterialShader()->SetValue(shader);
    auto ecsObject = interface_cast<IEcsObjectAccess>(material)->GetEcsObject();
    auto sync = interface_cast<META_NS::IEnginePropertySync>(ecsObject);
    sync->SyncProperties();

    auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
    auto manager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
    auto rmanager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs);
    auto nativeComponent = manager->Read(ecsObject->GetEntity());
    ASSERT_TRUE(nativeComponent);

    auto renderRes = interface_cast<IRenderResource>(shader);
    auto graphicsState = interface_cast<IGraphicsState>(shader);
    ASSERT_TRUE(renderRes);
    ASSERT_TRUE(graphicsState);
    EXPECT_EQ(renderRes->GetRenderHandle().GetHandle(),
        rmanager->GetRenderHandleReference(nativeComponent->materialShader.shader).GetHandle());
    EXPECT_EQ(graphicsState->GetGraphicsState().GetHandle(),
        rmanager->GetRenderHandleReference(nativeComponent->materialShader.graphicsState).GetHandle());
    EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(nativeComponent->materialShader.shader));
    EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(nativeComponent->materialShader.graphicsState));

    material->MaterialShader()->SetValue(nullptr);
    sync->SyncProperties();
    EXPECT_FALSE(CORE_NS::EntityUtil::IsValid(nativeComponent->materialShader.shader));
    EXPECT_FALSE(CORE_NS::EntityUtil::IsValid(nativeComponent->materialShader.graphicsState));
}

/**
 * @tc.name: ShaderConverterToSourceType
 * @tc.desc: Tests for Shader Converter To Source Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, ShaderConverterToSourceType, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    const auto material = CreateMaterial();
    const auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
    const auto manager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
    const auto entity = interface_cast<IEcsObjectAccess>(material)->GetEcsObject()->GetEntity();
    const auto nativeShader = CreateMaterialComponentShader();

    {
        const auto nativeComponent = manager->Write(entity);
        ASSERT_TRUE(nativeComponent);
        nativeComponent->materialShader = nativeShader;
    }

    auto shader = material->MaterialShader()->GetValue();

    auto rmanager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs);

    auto renderRes = interface_cast<IRenderResource>(shader);
    auto graphicsState = interface_cast<IGraphicsState>(shader);
    ASSERT_TRUE(renderRes);
    ASSERT_TRUE(graphicsState);
    EXPECT_EQ(
        renderRes->GetRenderHandle().GetHandle(), rmanager->GetRenderHandleReference(nativeShader.shader).GetHandle());
    EXPECT_TRUE(graphicsState->GetGraphicsState());

    manager->Create(entity); // Replace the existing component with an empty new one.
    scene->GetInternalScene()->Update();
    shader = material->MaterialShader()->GetValue();
    EXPECT_TRUE(shader);
}

/**
 * @tc.name: GetValueShaderChanged
 * @tc.desc: Tests for Get Value Shader Changed. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, DISABLED_GetValueShaderChanged, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    auto material = CreateMaterial();
    ASSERT_TRUE(material);

    META_NS::EventHandler h;

    META_NS::IMetadata::Ptr matc;
    if (auto att = interface_cast<META_NS::IAttach>(material)) {
        matc = interface_pointer_cast<META_NS::IMetadata>(
            att->GetAttachmentContainer(true)->FindByName("MaterialComponent"));
    }
    ASSERT_TRUE(matc);
    META_NS::IProperty::Ptr p = matc->GetProperty("MaterialShader");
    ASSERT_TRUE(p);

    IShader::Ptr value;
    std::atomic<uint64_t> count = 0;

    h.Subscribe<META_NS::IOnChanged>(
        p->OnChanged(),
        [&] {
            ++count;
            value = material->MaterialShader()->GetValue();
            UpdateScene();
        },
        context->GetApplicationQueue());

    value = material->MaterialShader()->GetValue();
    UpdateScene();

    // sleep for little while and see if we are looping or not
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(count, 1);
    h.Unsubscribe();
}

/**
 * @tc.name: ShaderChanged
 * @tc.desc: Tests for Shader Changed. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, ShaderChanged, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    auto material = CreateMaterial();
    ASSERT_TRUE(material);

    auto shader = material->MaterialShader()->GetValue();
    UpdateScene();

    auto ecsObject = interface_cast<IEcsObjectAccess>(material)->GetEcsObject();
    auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
    auto manager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
    auto rmanager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs);
    auto nativeComponent = manager->Read(ecsObject->GetEntity());
    ASSERT_TRUE(nativeComponent);

    auto renderRes = interface_cast<IRenderResource>(shader);
    auto graphicsState = interface_cast<IGraphicsState>(shader);
    ASSERT_TRUE(renderRes);
    ASSERT_TRUE(graphicsState);

    EXPECT_EQ(renderRes->GetRenderHandle().GetHandle(),
        rmanager->GetRenderHandleReference(nativeComponent->materialShader.shader).GetHandle());
    EXPECT_EQ(graphicsState->GetGraphicsState().GetHandle(),
        rmanager->GetRenderHandleReference(nativeComponent->materialShader.graphicsState).GetHandle());

    shader->Blend()->SetValue(!shader->Blend()->GetValue());
    UpdateScene();

    EXPECT_EQ(renderRes->GetRenderHandle().GetHandle(),
        rmanager->GetRenderHandleReference(nativeComponent->materialShader.shader).GetHandle());
    EXPECT_EQ(graphicsState->GetGraphicsState().GetHandle(),
        rmanager->GetRenderHandleReference(nativeComponent->materialShader.graphicsState).GetHandle());

    material->MaterialShader()->SetValue(nullptr);
    UpdateScene();

    EXPECT_FALSE(CORE_NS::EntityUtil::IsValid(nativeComponent->materialShader.shader));
    EXPECT_FALSE(CORE_NS::EntityUtil::IsValid(nativeComponent->materialShader.graphicsState));
}

const char* const TEXTURE_NAMES[] = { "BASE_COLOR", "NORMAL", "MATERIAL", "EMISSIVE", "AO", "CLEARCOAT",
    "CLEARCOAT_ROUGHNESS", "CLEARCOAT_NORMAL", "SHEEN", "TRANSMISSION", "SPECULAR" };

/**
 * @tc.name: ConstructTextures
 * @tc.desc: Tests for Construct Textures. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, ConstructTextures, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto material = CreateMaterial();
    const auto access = interface_cast<IEcsObjectAccess>(material);
    ASSERT_TRUE(access);
    auto entity = access->GetEcsObject()->GetEntity();
    auto nativeTextures = CreateTestTextures();
    EXPECT_TRUE(AssignTextures(entity, nativeTextures));

    auto textures = material->Textures()->GetValue();
    for (int i = 0; i < MaterialComponent::TextureIndex::TEXTURE_COUNT; ++i) {
        auto resultFactor = textures[i]->Factor()->GetValue();
        auto nativeFactor = nativeTextures[i].factor;
        EXPECT_EQ(resultFactor, nativeFactor);

        auto named = interface_cast<META_NS::INamed>(textures[i]);
        EXPECT_EQ(named->Name()->GetValue(), TEXTURE_NAMES[i]);

        EXPECT_TRUE(textures[i]->Sampler());
    }
}

/**
 * @tc.name: GetCustomPropertiesEmpty
 * @tc.desc: Tests for Get Custom Properties Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, GetCustomPropertiesEmpty, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    const auto material = CreateMaterial();

    auto customProps = material->GetCustomProperties();
    EXPECT_TRUE(customProps);
    EXPECT_TRUE(customProps->GetProperties().empty());
}

/**
 * @tc.name: GetCustomPropertiesEmptySimple
 * @tc.desc: Tests for Get Custom Properties Empty Simple. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, GetCustomPropertiesEmptySimple, testing::ext::TestSize.Level1)
{
    const auto materialWithoutComponent = META_NS::GetObjectRegistry().Create<IMaterial>(ClassId::Material);
    auto customProps = materialWithoutComponent->GetCustomProperties();
    EXPECT_FALSE(customProps);
}

/**
 * @tc.name: GetCustomProperties
 * @tc.desc: Tests for Get Custom Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, GetCustomProperties, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    const auto material = CreateMaterial();
    const auto access = interface_cast<IEcsObjectAccess>(material);
    ASSERT_TRUE(access);
    auto entity = access->GetEcsObject()->GetEntity();
    EXPECT_TRUE(AddCustomProperty(entity));

    // On the second run, everything is already synced. Test that values already in the engine work too.
    for (int i = 0; i < 2; ++i) {
        auto customProps = material->GetCustomProperties();
        ASSERT_TRUE(customProps);
        EXPECT_FALSE(customProps->GetProperties().empty());

        const auto alphaCutoffProp = customProps->GetProperty("alphaCutoff");
        ASSERT_TRUE(alphaCutoffProp);
        auto value = 0.0f;
        alphaCutoffProp->GetValue().GetValue(value);
        EXPECT_EQ(value, MAGIC_VALUE);
    }
}

/**
 * @tc.name: GetCustomProperty
 * @tc.desc: Tests for Get Custom Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, GetCustomProperty, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    const auto material = CreateMaterial();
    const auto access = interface_cast<IEcsObjectAccess>(material);
    ASSERT_TRUE(access);
    auto entity = access->GetEcsObject()->GetEntity();
    EXPECT_TRUE(AddCustomProperty(entity));

    // On the second run, everything is already synced. Test that values already in the engine work too.
    for (int i = 0; i < 2; ++i) {
        const auto singleProp = material->GetCustomProperty("alphaCutoff");
        ASSERT_TRUE(singleProp);
        auto singleValue = 0.0f;
        singleProp->GetValue().GetValue(singleValue);
        EXPECT_EQ(singleValue, MAGIC_VALUE);
    }
}

/**
 * @tc.name: GetCustomPropertySimpleFailure
 * @tc.desc: Tests for Get Custom Property Simple Failure. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, GetCustomPropertySimpleFailure, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    const auto materialWithoutComponent = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(materialWithoutComponent);
    EXPECT_FALSE(materialWithoutComponent->GetCustomProperty("nonexistent.property.always.missing"));
}

/**
 * @tc.name: GetCustomPropertyFailure
 * @tc.desc: Tests for Get Custom Property Failure. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, GetCustomPropertyFailure, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    const auto material = CreateMaterial();
    ASSERT_TRUE(material);
    EXPECT_FALSE(material->GetCustomProperty("nonexistent.property.always.missing"));
}

/**
 * @tc.name: MaterialType
 * @tc.desc: Tests for Material Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, MaterialType, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto material = scene->CreateObject<SCENE_NS::IMaterial>(SCENE_NS::ClassId::Material).GetResult();
    ASSERT_TRUE(material);
    material->Type()->SetValue(MaterialType::SPECULAR_GLOSSINESS);
    UpdateScene();
    EXPECT_EQ(material->Type()->GetValue(), MaterialType::SPECULAR_GLOSSINESS);
}

/**
 * @tc.name: RenderSort
 * @tc.desc: Tests for Render Sort. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, RenderSort, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    material->RenderSort()->SetValue({ 1, 2 });
    UpdateScene();
    auto res = material->RenderSort()->GetValue();
    EXPECT_EQ(res.renderSortLayer, 1);
    EXPECT_EQ(res.renderSortLayerOrder, 2);

    auto rs = interface_cast<IRenderSort>(material);
    ASSERT_TRUE(rs);
    EXPECT_EQ(META_NS::GetValue(rs->RenderSortLayer()), 1);
    EXPECT_EQ(META_NS::GetValue(rs->RenderSortLayerOrder()), 2);
}

/**
 * @tc.name: RenderSortOrder
 * @tc.desc: Tests for Render Sort Order. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, RenderSortOrder, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    auto rs = interface_cast<IRenderSort>(material);
    ASSERT_TRUE(rs);
    const uint8_t layer = 22;
    const uint8_t layerOrder = 42;

    META_NS::SetValue(rs->RenderSortLayer(), layer);
    META_NS::SetValue(rs->RenderSortLayerOrder(), layerOrder);
    UpdateScene();
    EXPECT_EQ(META_NS::GetValue(rs->RenderSortLayer()), layer);
    EXPECT_EQ(META_NS::GetValue(rs->RenderSortLayerOrder()), layerOrder);

    auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
    auto entity = interface_cast<IEcsObjectAccess>(material)->GetEcsObject()->GetEntity();
    auto mcm = static_cast<CORE3D_NS::IMaterialComponentManager*>(
        ecs->GetComponentManager(CORE3D_NS::IMaterialComponentManager::UID));
    ASSERT_TRUE(mcm);
    auto renderSort = mcm->Get(entity).renderSort;

    auto res = META_NS::GetValue(material->RenderSort());
    EXPECT_EQ(res.renderSortLayer, layer);
    EXPECT_EQ(res.renderSortLayerOrder, layerOrder);

    EXPECT_EQ(renderSort.renderSortLayer, layer);
    EXPECT_EQ(renderSort.renderSortLayerOrder, layerOrder);
}

/**
 * @tc.name: LightingFlags
 * @tc.desc: Tests for Lighting Flags. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, LightingFlags, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    material->LightingFlags()->SetValue(LightingFlags::SHADOW_RECEIVER_BIT);
    UpdateScene();
    EXPECT_EQ(material->LightingFlags()->GetValue(), LightingFlags::SHADOW_RECEIVER_BIT);
}

/**
 * @tc.name: CustomProperties
 * @tc.desc: Tests for Custom Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, CustomProperties, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    {
        auto meta = material->CustomProperties()->GetValue();
        ASSERT_TRUE(meta);
        EXPECT_EQ(meta->GetProperties().size(), 0);
    }
    material->MaterialShader()->SetValue(man->LoadShader("test_assets://shaders/test.shader").GetResult());
    UpdateScene();
    WaitForUserQueue();
    {
        auto meta = material->CustomProperties()->GetValue();
        ASSERT_TRUE(meta);
        EXPECT_EQ(meta->GetProperties().size(), 4);
        auto p1 = meta->GetProperty<BASE_NS::Math::Vec4>("v4_");
        ASSERT_TRUE(p1);
        EXPECT_EQ(p1->GetValue(), (BASE_NS::Math::Vec4 { 0.1, 0.2, 0.3, 0.4 }));
        auto p2 = meta->GetProperty<BASE_NS::Math::UVec4>("uv4_");
        ASSERT_TRUE(p2);
        EXPECT_EQ(p2->GetValue(), (BASE_NS::Math::UVec4 { 3, 4, 5, 6 }));
        auto p3 = meta->GetProperty<float>("f_");
        ASSERT_TRUE(p3);
        EXPECT_EQ(p3->GetValue(), 1.0);
        auto p4 = meta->GetProperty<uint32_t>("u_");
        ASSERT_TRUE(p4);
        EXPECT_EQ(p4->GetValue(), 7);
    }
    material->MaterialShader()->SetValue(man->LoadShader("test_assets://shaders/Custom.shader").GetResult());
    UpdateScene();
    WaitForUserQueue();
    {
        auto meta = material->CustomProperties()->GetValue();
        EXPECT_EQ(meta->GetProperties().size(), 1);
        auto p1 = meta->GetProperty<BASE_NS::Math::Vec4>("AlbedoFactor");
        ASSERT_TRUE(p1);
        EXPECT_EQ(p1->GetValue(), (BASE_NS::Math::Vec4 { 1, 1, 0, 1 }));

        p1->SetValue(BASE_NS::Math::Vec4 { 2, 2, 2, 2 });
    }
    UpdateScene();
    WaitForUserQueue();
    {
        auto meta = material->CustomProperties()->GetValue();
        EXPECT_EQ(meta->GetProperties().size(), 1);
        auto p1 = meta->GetProperty<BASE_NS::Math::Vec4>("AlbedoFactor");
        ASSERT_TRUE(p1);
        EXPECT_EQ(p1->GetValue(), (BASE_NS::Math::Vec4 { 2, 2, 2, 2 }));
    }
    {
        auto ecsObject = interface_cast<IEcsObjectAccess>(material)->GetEcsObject();
        auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
        auto manager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
        auto nativeComponent = manager->Read(ecsObject->GetEntity());
        ASSERT_TRUE(nativeComponent);
        ASSERT_TRUE(nativeComponent->customProperties);

        EXPECT_EQ(CORE_NS::GetPropertyValue<BASE_NS::Math::Vec4>(*nativeComponent->customProperties, "AlbedoFactor"),
            (BASE_NS::Math::Vec4 { 2, 2, 2, 2 }));
    }
}

/**
 * @tc.name: SetCustomProperties
 * @tc.desc: Tests for Set Custom Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, SetCustomProperties, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    auto meta = material->CustomProperties()->GetValue();
    ASSERT_TRUE(meta);
    EXPECT_EQ(meta->GetProperties().size(), 0);

    auto p1 = meta->AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec4>("v4_", { 1.0, 1.0, 1.0, 1.0 }));
    UpdateScene();
    WaitForUserQueue();

    material->MaterialShader()->SetValue(man->LoadShader("test_assets://shaders/test.shader").GetResult());
    UpdateScene();
    WaitForUserQueue();
    {
        auto meta = material->CustomProperties()->GetValue();
        ASSERT_TRUE(meta);
        auto p1 = meta->GetProperty<BASE_NS::Math::Vec4>("v4_");
        ASSERT_TRUE(p1);
        EXPECT_EQ(p1->GetValue(), (BASE_NS::Math::Vec4 { 1.0, 1.0, 1.0, 1.0 }));
    }
    material->MaterialShader()->SetValue(man->LoadShader("test_assets://shaders/test.shader").GetResult());
    UpdateScene();
    WaitForUserQueue();
    {
        auto meta = material->CustomProperties()->GetValue();
        ASSERT_TRUE(meta);
        auto p1 = meta->GetProperty<BASE_NS::Math::Vec4>("v4_");
        ASSERT_TRUE(p1);
        EXPECT_EQ(p1->GetValue(), (BASE_NS::Math::Vec4 { 1.0, 1.0, 1.0, 1.0 }));
    }
}

/**
 * @tc.name: DeadlockingMaterialGetValue
 * @tc.desc: Tests for Deadlocking Material Get Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, DeadlockingMaterialGetValue, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();
    material->MaterialShader()->SetValue(man->LoadShader("test_assets://shaders/test.shader").GetResult());
    UpdateScene();

    material->CustomProperties()->GetValue();
    auto att = interface_pointer_cast<META_NS::IAttach>(material);
    ASSERT_TRUE(att);
    auto attc = att->GetAttachmentContainer(true);
    auto comp = attc->FindByName("MaterialComponent");
    ASSERT_TRUE(comp);

    auto prop = interface_pointer_cast<META_NS::IMetadata>(comp)->GetProperty("MaterialShader");

    for (int i = 0; i != 1000; ++i) {
        prop->NotifyChange();
        material->MaterialShader()->GetValue();
    }
}

/**
 * @tc.name: DeadlockingMaterialSetValue
 * @tc.desc: Tests for Deadlocking Material Set Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, DeadlockingMaterialSetValue, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();
    material->MaterialShader()->SetValue(man->LoadShader("test_assets://shaders/test.shader").GetResult());
    UpdateScene();

    material->CustomProperties()->GetValue();
    auto att = interface_pointer_cast<META_NS::IAttach>(material);
    ASSERT_TRUE(att);
    auto attc = att->GetAttachmentContainer(true);
    auto comp = attc->FindByName("MaterialComponent");
    ASSERT_TRUE(comp);

    auto prop = interface_pointer_cast<META_NS::IMetadata>(comp)->GetProperty("MaterialShader");

    auto shader = man->LoadShader("test_assets://shaders/test.shader").GetResult();
    ASSERT_TRUE(shader);

    for (size_t i = 0; i != 1000; ++i) {
        scene->GetInternalScene()->RunDirectlyOrInTask([&] {
            META_NS::PropertyLock l { prop };
            prop->NotifyChange();
        });
        if (i % 2) {
            material->MaterialShader()->SetValue(shader);
        } else {
            material->MaterialShader()->SetValue(nullptr);
        }
        UpdateScene();
    }
}

/**
 * @tc.name: OverrideShaderState
 * @tc.desc: Tests for Override Shader State. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, OverrideShaderState, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto material = CreateMaterial();
    UpdateScene();
    META_NS::SetValue(material->MaterialShader(), man->LoadShader("test_assets://shaders/test.shader").GetResult());
    UpdateScene();
    auto shader = META_NS::GetValue(material->MaterialShader());
    auto gstate = interface_cast<IGraphicsState>(shader);
    auto sstate = interface_cast<IShaderState>(shader);
    ASSERT_TRUE(gstate);
    ASSERT_TRUE(sstate);
    auto& sm = scene->GetInternalScene()->GetRenderContext().GetDevice().GetShaderManager();
    auto checkState = [&](bool blend, bool test, bool write) {
        auto gs = sm.GetGraphicsState(gstate->GetGraphicsState());
        auto blendEnabled = std::any_of(gs.colorBlendState.colorAttachments,
            gs.colorBlendState.colorAttachments + gs.colorBlendState.colorAttachmentCount,
            [](const RENDER_NS::GraphicsState::ColorBlendState::Attachment& attachment) {
                return attachment.enableBlend;
            });
        EXPECT_EQ(blendEnabled, blend);
        EXPECT_EQ(gs.depthStencilState.enableDepthTest, test);
        EXPECT_EQ(gs.depthStencilState.enableDepthWrite, write);
    };
    checkState(false, true, true);
    SCENE_NS::IShaderState::DepthStencilOptions dso;
    dso.enableDepthWrite = false;
    dso.enableDepthTest = false;
    sstate->SetShaderStateOverride(nullptr, &dso, nullptr);
    UpdateScene();
    checkState(false, false, false);
    SCENE_NS::IShaderState::ColorBlendOptions cbo;
    cbo.enableBlend = true;
    sstate->SetShaderStateOverride(&cbo, &dso, nullptr);
    UpdateScene();
    checkState(true, false, false);
    sstate->SetShaderStateOverride(nullptr, nullptr, nullptr);
    UpdateScene();
    checkState(false, true, true);
}

/**
 * @tc.name: OcclusionMaterial
 * @tc.desc: Test ClassId::OcclusionMaterial
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, OcclusionMaterial, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto material = CreateMaterial(ClassId::OcclusionMaterial);
    ASSERT_TRUE(material);
    ASSERT_EQ(interface_cast<META_NS::IObject>(material)->GetClassId(), ClassId::OcclusionMaterial);

    UpdateScene();
    auto entity = GetEcsObjectEntity(interface_cast<IEcsObjectAccess>(material));
    ASSERT_TRUE(CORE_NS::EntityUtil::IsValid(entity));

    // Check that the created material has the correct type in ECS
    auto internal = scene->GetInternalScene();
    auto& ecs = *internal->GetEcsContext().GetNativeEcs();
    auto mcm = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(ecs);
    ASSERT_TRUE(mcm);
    auto rh = mcm->Read(entity);
    ASSERT_TRUE(rh);
    EXPECT_EQ(rh->type, CORE3D_NS::MaterialComponent::Type::OCCLUSION);
}

/// Create a new material in ECS with given type and assign it to a node with a given name
void CreateAndSetMaterial(const IScene::Ptr& scene, BASE_NS::string_view name, CORE3D_NS::MaterialComponent::Type type)
{
    auto internal = scene->GetInternalScene();
    auto& ecs = *internal->GetEcsContext().GetNativeEcs();

    // Find target node
    auto ns = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
    ASSERT_TRUE(ns);
    auto& root = ns->GetRootNode();
    auto node = root.LookupNodeByName(name);
    ASSERT_TRUE(node);

    auto materialEntity = ecs.GetEntityManager().Create();

    // Create material with given type
    auto matcm = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(ecs);
    ASSERT_TRUE(matcm);
    matcm->Create(materialEntity);
    auto material = matcm->Write(materialEntity);
    ASSERT_TRUE(material);
    material->type = type;

    const auto rmm = CORE_NS::GetManager<CORE3D_NS::IRenderMeshComponentManager>(ecs);
    ASSERT_TRUE(rmm);
    auto rm = rmm->Read(node->GetEntity());
    ASSERT_TRUE(rm);

    // Assign the material as the material of the first submesh of given node
    auto mshcm = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(ecs);
    ASSERT_TRUE(mshcm);

    auto mesh = mshcm->Write(rm->mesh);
    ASSERT_TRUE(mesh);
    ASSERT_GT(mesh->submeshes.size(), 0);
    mesh->submeshes[0].material = materialEntity;
}

/**
 * @tc.name: MaterialConversion
 * @tc.desc: Test material component to LumeScene material class mapping.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialTest, MaterialConversion, testing::ext::TestSize.Level1)
{
    using META_NS::GetValue;

    auto getCubeMaterial = [](const IScene::Ptr& scene) {
        IMaterial::Ptr material;
        if (auto n = scene->FindNode<IMesh>("///AnimatedCube").GetResult()) {
            if (auto subs = n->SubMeshes()->GetValue(); !subs.empty()) {
                material = GetValue(subs[0]->Material());
            }
        }
        return material;
    };

    {
        auto scene = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
        ASSERT_TRUE(scene);
        CreateAndSetMaterial(scene, "AnimatedCube", CORE3D_NS::MaterialComponent::Type::OCCLUSION);
        UpdateScene();

        auto material = getCubeMaterial(scene);
        ASSERT_TRUE(material);
        EXPECT_EQ(GetValue(material->Type()), MaterialType::OCCLUSION);
        EXPECT_EQ(interface_cast<META_NS::IObject>(material)->GetClassId(), ClassId::OcclusionMaterial);
    }

    {
        auto scene = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
        ASSERT_TRUE(scene);
        UpdateScene();

        // Animation coming from gltf should always be METALLIC_ROUGHNESS
        auto material = getCubeMaterial(scene);
        ASSERT_TRUE(material);
        EXPECT_EQ(GetValue(material->Type()), MaterialType::METALLIC_ROUGHNESS);
        EXPECT_EQ(interface_cast<META_NS::IObject>(material)->GetClassId(), ClassId::Material);

        // Switch to occlusion material (in ECS side)
        CreateAndSetMaterial(scene, "AnimatedCube", CORE3D_NS::MaterialComponent::Type::OCCLUSION);
        UpdateScene();
        // The material change should be reflected in the material property
        material = getCubeMaterial(scene);
        ASSERT_TRUE(material);
        EXPECT_EQ(GetValue(material->Type()), MaterialType::OCCLUSION);
        EXPECT_EQ(interface_cast<META_NS::IObject>(material)->GetClassId(), ClassId::OcclusionMaterial);
    }
}

} // namespace UTest
SCENE_END_NAMESPACE()
