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

#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>

#include <core/intf_engine.h>
#include <core/resources/intf_resource_manager.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class ResourceTypeTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&context->GetRenderer()->GetEngine().GetFileManager()));
    }
};

/**
 * @tc.name: OcclusionMaterialResourceLoad
 * @tc.desc: Test loading occlusion material through resource manager.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, OcclusionMaterialResourceLoad, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    CORE_NS::ResourceIdContext rid{"test_occ_mat", interface_pointer_cast<CORE_NS::IInterface>(scene)};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::OcclusionMaterialResource.Id().ToUid(), ""));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);

    auto mat = interface_pointer_cast<IMaterial>(res);
    ASSERT_TRUE(mat);
    EXPECT_EQ(mat->Type()->GetValue(), MaterialType::OCCLUSION);
}

/**
 * @tc.name: OcclusionMaterialResourceLoadNullContext
 * @tc.desc: Test loading occlusion material without scene context fails.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, OcclusionMaterialResourceLoadNullContext, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_occ_mat_null"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::OcclusionMaterialResource.Id().ToUid(), ""));
    EXPECT_FALSE(resources->GetResource(rid));
}

/**
 * @tc.name: OcclusionMaterialResourceSave
 * @tc.desc: Test saving occlusion material resource via export.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, OcclusionMaterialResourceSave, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto sceneCtx = interface_pointer_cast<CORE_NS::IInterface>(scene);
    CORE_NS::ResourceIdContext rid{"test_occ_mat_save", sceneCtx};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::OcclusionMaterialResource.Id().ToUid(), ""));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);
    EXPECT_EQ(resources->Export("app://test_occ_mat_export.res", sceneCtx), CORE_NS::IResourceManager::Result::OK);
}

/**
 * @tc.name: OcclusionMaterialResourceReload
 * @tc.desc: Test reloading occlusion material resource without options.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, OcclusionMaterialResourceReload, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    CORE_NS::ResourceIdContext rid{"test_occ_mat_reload", interface_pointer_cast<CORE_NS::IInterface>(scene)};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::OcclusionMaterialResource.Id().ToUid(), ""));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);
    EXPECT_TRUE(resources->ReloadResource(res));
}

/**
 * @tc.name: EnvironmentResourceLoadNullContext
 * @tc.desc: Test loading environment without scene context fails.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, EnvironmentResourceLoadNullContext, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_env_null"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::EnvironmentResource.Id().ToUid(), ""));
    EXPECT_FALSE(resources->GetResource(rid));
}

/**
 * @tc.name: EnvironmentResourceReload
 * @tc.desc: Test reloading environment resource without options.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, EnvironmentResourceReload, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    CORE_NS::ResourceIdContext rid{"test_env_reload", interface_pointer_cast<CORE_NS::IInterface>(scene)};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::EnvironmentResource.Id().ToUid(), ""));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);
    EXPECT_TRUE(resources->ReloadResource(res));
}

/**
 * @tc.name: PostProcessResourceLoadNullContext
 * @tc.desc: Test loading postprocess without scene context fails.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, PostProcessResourceLoadNullContext, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_pp_null"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::PostProcessResource.Id().ToUid(), ""));
    EXPECT_FALSE(resources->GetResource(rid));
}

/**
 * @tc.name: PostProcessResourceReload
 * @tc.desc: Test reloading postprocess resource without options.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, PostProcessResourceReload, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    CORE_NS::ResourceIdContext rid{"test_pp_reload", interface_pointer_cast<CORE_NS::IInterface>(scene)};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::PostProcessResource.Id().ToUid(), ""));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);
    EXPECT_TRUE(resources->ReloadResource(res));
}

/**
 * @tc.name: MaterialResourceLoadNullContext
 * @tc.desc: Test loading material without scene context fails.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, MaterialResourceLoadNullContext, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_mat_null"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::MaterialResource.Id().ToUid(), ""));
    EXPECT_FALSE(resources->GetResource(rid));
}

/**
 * @tc.name: MaterialResourceReload
 * @tc.desc: Test reloading material resource without options.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, MaterialResourceReload, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    CORE_NS::ResourceIdContext rid{"test_mat_reload", interface_pointer_cast<CORE_NS::IInterface>(scene)};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::MaterialResource.Id().ToUid(), ""));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);
    EXPECT_TRUE(resources->ReloadResource(res));
}

/**
 * @tc.name: AnimationResourceLoadNullContext
 * @tc.desc: Test loading animation without scene context fails.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, AnimationResourceLoadNullContext, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_anim_null"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::AnimationResource.Id().ToUid(), ""));
    EXPECT_FALSE(resources->GetResource(rid));
}

/**
 * @tc.name: AnimationResourceReload
 * @tc.desc: Test reloading animation resource without options.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, AnimationResourceReload, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    auto anims = scene->GetAnimations().GetResult();
    ASSERT_FALSE(anims.empty());

    auto animRes = interface_pointer_cast<CORE_NS::IResource>(anims[0]);
    ASSERT_TRUE(animRes);
    EXPECT_TRUE(resources->ReloadResource(animRes));
}

/**
 * @tc.name: GltfSceneResourceReload
 * @tc.desc: Test that GltfScene reload returns false (not implemented).
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, GltfSceneResourceReload, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    auto sceneRes = interface_pointer_cast<CORE_NS::IResource>(scene);
    ASSERT_TRUE(sceneRes);
    EXPECT_FALSE(resources->ReloadResource(sceneRes));
}

/**
 * @tc.name: SceneResourceReload
 * @tc.desc: Test that scene reload returns false (not implemented).
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, SceneResourceReload, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://scene_resource/test1/test.scene");
    ASSERT_TRUE(scene);

    auto sceneRes = interface_pointer_cast<CORE_NS::IResource>(scene);
    ASSERT_TRUE(sceneRes);
    EXPECT_FALSE(resources->ReloadResource(sceneRes));
}

/**
 * @tc.name: ShaderResourceReloadRegistered
 * @tc.desc: Test shader reload through resource manager.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, ShaderResourceReloadRegistered, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_shader_reload"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::ShaderResource.Id().ToUid(), "test://shaders/test.shader"));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);

    auto shader = interface_pointer_cast<IShader>(res);
    EXPECT_TRUE(shader);
    EXPECT_TRUE(resources->ReloadResource(res));
}

/**
 * @tc.name: ImageResourceLoadViaResourceManager
 * @tc.desc: Test image load and reload through resource manager.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, ImageResourceLoadViaResourceManager, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_image_rm"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::ImageResource.Id().ToUid(), "test://images/logo.png"));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);

    auto image = interface_pointer_cast<IImage>(res);
    ASSERT_TRUE(image);

    auto size = image->Size()->GetValue();
    EXPECT_GT(size.x, 0u);
    EXPECT_GT(size.y, 0u);

    // Reload the image
    EXPECT_TRUE(resources->ReloadResource(res));
}

/**
 * @tc.name: ImageResourceSave
 * @tc.desc: Test image save through resource manager export.
 * @tc.type: FUNC
 */
UNIT_TEST_F(ResourceTypeTest, ImageResourceSave, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();

    CORE_NS::ResourceIdContext rid{"test_image_save"};
    EXPECT_TRUE(resources->AddResource(rid, ClassId::ImageResource.Id().ToUid(), "test://images/logo.png"));
    auto res = resources->GetResource(rid);
    ASSERT_TRUE(res);

    // Export triggers SaveResource
    EXPECT_EQ(resources->Export("app://test_image_export.res", nullptr), CORE_NS::IResourceManager::Result::OK);
}

}  // namespace UTest
SCENE_END_NAMESPACE()
