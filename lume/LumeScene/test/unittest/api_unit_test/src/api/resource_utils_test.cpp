/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <scene/api/resource.h>
#include <scene/api/resource_utils.h>
#include <scene/api/scene.h>
#include <scene/api/template/material_template.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/resource/types.h>

#include <core/intf_engine.h>
#include <core/resources/intf_resource.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/interface/intf_object_registry.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class ResourceUtilsTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&context->GetRenderer()->GetEngine().GetFileManager()));
    }

    CORE_NS::ResourceIdContext SceneRid(BASE_NS::string_view name) const
    {
        return CORE_NS::ResourceIdContext{BASE_NS::string(name), interface_pointer_cast<CORE_NS::IInterface>(scene)};
    }

    static CORE_NS::ResourceIdContext FreeRid(BASE_NS::string_view name)
    {
        return CORE_NS::ResourceIdContext{BASE_NS::string(name)};
    }

    bool AddSceneMaterial(BASE_NS::string_view name) const
    {
        return resources->AddResource(SceneRid(name), ClassId::MaterialResource.Id().ToUid(), "");
    }

    bool AddSceneOcclusionMaterial(BASE_NS::string_view name) const
    {
        return resources->AddResource(SceneRid(name), ClassId::OcclusionMaterialResource.Id().ToUid(), "");
    }

    bool AddFreeShader(BASE_NS::string_view name, BASE_NS::string_view url = "test://shaders/test.shader") const
    {
        return resources->AddResource(FreeRid(name), ClassId::ShaderResource.Id().ToUid(), url);
    }

    CORE_NS::IResource::Ptr CreateMaterialTemplateResource(BASE_NS::string_view rid) const
    {
        auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::MaterialTemplate);
        auto res = interface_pointer_cast<CORE_NS::IResource>(obj);
        if (auto sid = interface_cast<CORE_NS::ISetResourceId>(res)) {
            sid->SetResourceId(CORE_NS::ResourceIdContext{BASE_NS::string(rid)});
        }
        return res;
    }
};

UNIT_TEST_F(ResourceUtilsTest, GetResourcesOfTypeReturnsSceneBoundResources, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(AddSceneMaterial("mat1"));

    auto materials = GetResourcesOfType(scene, ClassId::MaterialResource.Id().ToUid());
    ASSERT_EQ(materials.size(), 1u);
    EXPECT_EQ(materials[0]->GetResourceType(), ClassId::MaterialResource.Id().ToUid());
}

UNIT_TEST_F(ResourceUtilsTest, GetResourcesOfTypeFiltersByType, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(AddSceneMaterial("mat_filter"));
    ASSERT_TRUE(AddSceneOcclusionMaterial("occ_filter"));

    auto onlyMat = GetResourcesOfType(scene, ClassId::MaterialResource.Id().ToUid());
    EXPECT_EQ(onlyMat.size(), 1u);

    auto onlyOcc = GetResourcesOfType(scene, ClassId::OcclusionMaterialResource.Id().ToUid());
    EXPECT_EQ(onlyOcc.size(), 1u);
}

UNIT_TEST_F(ResourceUtilsTest, GetResourcesOfTypeWildcardReturnsAllSceneResources, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(AddSceneMaterial("mat_wild"));
    ASSERT_TRUE(AddSceneOcclusionMaterial("occ_wild"));

    auto all = GetResourcesOfType(scene, BASE_NS::Uid{});
    EXPECT_GE(all.size(), 2u);
}

UNIT_TEST_F(
    ResourceUtilsTest, GetResourcesOfTypeManagerOverloadReturnsContextFreeResources, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(AddFreeShader("shader_free"));

    auto shaders = GetResourcesOfType(resources, ClassId::ShaderResource.Id().ToUid());
    ASSERT_EQ(shaders.size(), 1u);
    EXPECT_EQ(shaders[0]->GetResourceType(), ClassId::ShaderResource.Id().ToUid());
}

UNIT_TEST_F(ResourceUtilsTest, GetResourcesOfTypeBucketsAreSeparate, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(AddSceneMaterial("mat_bucket"));
    ASSERT_TRUE(AddFreeShader("shader_bucket"));

    auto sceneShaders = GetResourcesOfType(scene, ClassId::ShaderResource.Id().ToUid());
    EXPECT_EQ(sceneShaders.size(), 0u);

    auto freeMaterials = GetResourcesOfType(resources, ClassId::MaterialResource.Id().ToUid());
    EXPECT_EQ(freeMaterials.size(), 0u);

    auto sceneMats = GetResourcesOfType(scene, ClassId::MaterialResource.Id().ToUid());
    EXPECT_EQ(sceneMats.size(), 1u);

    auto freeShaders = GetResourcesOfType(resources, ClassId::ShaderResource.Id().ToUid());
    EXPECT_EQ(freeShaders.size(), 1u);
}

UNIT_TEST_F(ResourceUtilsTest, GetSceneMaterialsMergesRegularAndOcclusion, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(AddSceneMaterial("mat_a"));
    ASSERT_TRUE(AddSceneMaterial("mat_b"));
    ASSERT_TRUE(AddSceneOcclusionMaterial("mat_occ"));

    auto materials = GetSceneMaterials(scene);
    EXPECT_EQ(materials.size(), 3u);
    for (const auto& m : materials) {
        EXPECT_TRUE(m);
    }
}

UNIT_TEST_F(ResourceUtilsTest, GetSceneShadersReturnsTypedWrappers, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(AddFreeShader("shader_typed"));

    auto shaders = GetSceneShaders(resources);
    ASSERT_EQ(shaders.size(), 1u);
    EXPECT_TRUE(shaders[0]);
}

UNIT_TEST_F(ResourceUtilsTest, GetSceneEnvironmentsSceneOverloadReturnsTypedWrappers, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    ASSERT_TRUE(resources->AddResource(SceneRid("env1"), ClassId::EnvironmentResource.Id().ToUid(), ""));

    auto envs = GetSceneEnvironments(scene);
    ASSERT_EQ(envs.size(), 1u);
    EXPECT_TRUE(envs[0]);
}

UNIT_TEST_F(ResourceUtilsTest, GetSceneMaterialTemplatesReturnsContextFreeTemplates, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEmptyScene());
    auto templ = CreateMaterialTemplateResource("app://mat_template_free.resource");
    ASSERT_TRUE(templ);
    ASSERT_TRUE(resources->AddResource(templ));

    auto templates = GetSceneMaterialTemplates(resources);
    ASSERT_EQ(templates.size(), 1u);
    EXPECT_TRUE(templates[0]);

    auto sceneTemplates = GetSceneMaterialTemplates(scene);
    EXPECT_EQ(sceneTemplates.size(), 0u);
}

UNIT_TEST_F(ResourceUtilsTest, NullSceneReturnsEmpty, testing::ext::TestSize.Level1)
{
    auto byType = GetResourcesOfType(IScene::Ptr{}, ClassId::MaterialResource.Id().ToUid());
    EXPECT_TRUE(byType.empty());
    EXPECT_TRUE(GetSceneMaterials(IScene::Ptr{}).empty());
    EXPECT_TRUE(GetSceneShaders(IScene::Ptr{}).empty());
    EXPECT_TRUE(GetSceneEnvironments(IScene::Ptr{}).empty());
    EXPECT_TRUE(GetSceneAnimations(IScene::Ptr{}).empty());
    EXPECT_TRUE(GetSceneMaterialTemplates(IScene::Ptr{}).empty());
}

UNIT_TEST_F(ResourceUtilsTest, NullManagerReturnsEmpty, testing::ext::TestSize.Level1)
{
    CORE_NS::IResourceManager::Ptr none;
    auto byType = GetResourcesOfType(none, ClassId::ShaderResource.Id().ToUid());
    EXPECT_TRUE(byType.empty());
    EXPECT_TRUE(GetSceneMaterials(none).empty());
    EXPECT_TRUE(GetSceneShaders(none).empty());
    EXPECT_TRUE(GetSceneEnvironments(none).empty());
    EXPECT_TRUE(GetSceneAnimations(none).empty());
    EXPECT_TRUE(GetSceneMaterialTemplates(none).empty());
}

}  // namespace UTest
SCENE_END_NAMESPACE()
