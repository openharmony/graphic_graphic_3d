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

#include <scene/ext/component_util.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_shader.h>

#include <3d/ecs/components/material_component.h>
#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/namespace.h>

#include "scene/scene_component_test.h"

using CORE3D_NS::MaterialComponent;

META_TYPE(CORE3D_NS::MaterialComponent::Type)
META_TYPE(CORE3D_NS::MaterialComponent::Shader)
META_TYPE(CORE3D_NS::MaterialComponent::TextureInfo)
META_TYPE(CORE3D_NS::MaterialComponent::RenderSort)

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginMaterialComponentTest : public ScenePluginComponentTest<CORE3D_NS::IMaterialComponentManager> {
protected:
    template<class T>
    void TestIMaterialPropertyGetters(T* mat)
    {
        EXPECT_TRUE(mat->Type());
        EXPECT_TRUE(mat->AlphaCutoff());
        EXPECT_TRUE(mat->LightingFlags());
        EXPECT_TRUE(mat->MaterialShader());
        EXPECT_TRUE(mat->DepthShader());
        EXPECT_TRUE(mat->RenderSort());
        EXPECT_TRUE(mat->Textures());
        EXPECT_TRUE(mat->CustomProperties());
    }

    void TestIMaterialFunctions(IMaterial* mat)
    {
        ASSERT_TRUE(mat);
        TestIMaterialPropertyGetters<const IMaterial>(mat);
        TestIMaterialPropertyGetters<IMaterial>(mat);
        TestIMaterialPropertyGetters<const IMaterial>(mat);
        TestIMaterialPropertyGetters<IMaterial>(mat);
        TestInvalidComponentPropertyInit(mat);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::IMaterialComponentManager>(node));
    SetComponent(node, "MaterialComponent");

    TestEngineProperty<MaterialType>("Type", MaterialType::CUSTOM, nativeComponent.type);
    TestEngineProperty<float>("AlphaCutoff", 0.5f, nativeComponent.alphaCutoff);
    TestEngineProperty<uint32_t>("LightingFlags", 2, nativeComponent.materialLightingFlags);
    TestEngineProperty<uint32_t>("UseTexcoordSetBit", 1, nativeComponent.useTexcoordSetBit);
    TestEngineProperty<uint32_t>("CustomRenderSlotId", 2, nativeComponent.customRenderSlotId);

    {
        auto prop = GetProperty<uint32_t>("LightingFlags");
        ASSERT_TRUE(prop);
        prop->SetValue(static_cast<uint32_t>(LightingFlags::SHADOW_RECEIVER_BIT | LightingFlags::SHADOW_CASTER_BIT));
        UpdateComponentMembers();
        EXPECT_EQ(nativeComponent.materialLightingFlags,
            static_cast<uint32_t>(LightingFlags::SHADOW_RECEIVER_BIT | LightingFlags::SHADOW_CASTER_BIT));
    }
    {
        auto prop = GetProperty<CORE3D_NS::MaterialComponent::RenderSort>("RenderSort");
        ASSERT_TRUE(prop);
        prop->SetValue({ 1, 2 });
        UpdateComponentMembers();
        EXPECT_EQ(nativeComponent.renderSort.renderSortLayer, 1);
        EXPECT_EQ(nativeComponent.renderSort.renderSortLayerOrder, 2);
    }

    auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
    auto materialManager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
    auto& em = ecs->GetEntityManager();
    auto entity = em.Create();
    materialManager->Create(entity);
    auto propHandle = materialManager->GetData(entity);
    TestEngineProperty<CORE_NS::IPropertyHandle*>("CustomProperties", propHandle, nativeComponent.customProperties);

    for (const BASE_NS::string_view propName : { "MaterialShader", "DepthShader" }) {
        auto prop = GetProperty<MaterialComponent::Shader>(propName);
        prop->SetValue(CreateMaterialComponentShader());
        UpdateComponentMembers();
        const auto propValue = prop->GetValue();
        EXPECT_EQ(propValue.shader, nativeComponent.materialShader.shader);
    }
    {
        auto prop = GetArrayProperty<MaterialComponent::TextureInfo>("Textures");
        MaterialComponent::TextureInfo textures[MaterialComponent::TextureIndex::TEXTURE_COUNT];
        for (int i = 0; i < MaterialComponent::TextureIndex::TEXTURE_COUNT; ++i) {
            textures[i] = MaterialComponent::TextureInfo { {}, {}, { 1.0f + 0.01f * i, 0.0f, 0.0f, 0.0f }, {} };
        }
        prop->SetValue(textures);
        UpdateComponentMembers();
        const auto values = prop->GetValue();
        ASSERT_EQ(values.size(), MaterialComponent::TextureIndex::TEXTURE_COUNT);
        for (int i = 0; i < MaterialComponent::TextureIndex::TEXTURE_COUNT; ++i) {
            EXPECT_EQ(values[i].factor, nativeComponent.textures[i].factor);
        }
    }
    {
        auto prop = GetArrayProperty<CORE_NS::EntityReference>("CustomResources");
        CORE_NS::EntityReference references[2] = { em.CreateReferenceCounted(), em.CreateReferenceCounted() };
        prop->SetValue(references);
        UpdateComponentMembers();
        const auto values = prop->GetValue();
        ASSERT_EQ(values.size(), 2);
        for (int i = 0; i < 2; ++i) {
            EXPECT_EQ(values[i], nativeComponent.customResources[i]);
            EXPECT_EQ(values[i], references[i]);
        }
    }
}

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMaterialComponentTest, Functions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto material = scene->CreateObject(ClassId::Material).GetResult();

    TestIMaterialFunctions(interface_cast<IMaterial>(material));
}

} // namespace UTest

SCENE_END_NAMESPACE()
