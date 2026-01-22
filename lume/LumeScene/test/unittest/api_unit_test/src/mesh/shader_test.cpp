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

#include <scene/interface/intf_shader.h>
#include <scene/interface/resource/types.h>

#include <core/intf_engine.h>

#include <meta/api/event_handler.h>

#include "scene/scene_test.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginShaderTest : public ScenePluginTest {
public:
    void TestCullAndBlend(IShader::Ptr shader)
    {
        shader->CullMode()->SetValue(CullModeFlags::FRONT_BIT);
        UpdateScene();
        EXPECT_EQ(shader->CullMode()->GetValue(), CullModeFlags::FRONT_BIT);
        shader->CullMode()->SetValue(CullModeFlags::FRONT_BIT | CullModeFlags::BACK_BIT);
        UpdateScene();
        EXPECT_EQ(shader->CullMode()->GetValue(), CullModeFlags::FRONT_AND_BACK);

        shader->Blend()->SetValue(true);
        UpdateScene();
        EXPECT_EQ(shader->CullMode()->GetValue(), CullModeFlags::FRONT_AND_BACK);
        shader->CullMode()->SetValue(CullModeFlags::NONE);
        UpdateScene();
        EXPECT_EQ(shader->CullMode()->GetValue(), CullModeFlags::NONE);
        shader->CullMode()->SetValue(CullModeFlags::FRONT_BIT | CullModeFlags::BACK_BIT);
        shader->Blend()->SetValue(true);
        EXPECT_TRUE(shader->Blend()->GetValue());
        shader->Blend()->SetValue(false);
        UpdateScene();
        EXPECT_FALSE(shader->Blend()->GetValue());
        EXPECT_EQ(shader->CullMode()->GetValue(), CullModeFlags::FRONT_AND_BACK);
    }
};

/**
 * @tc.name: LoadShader
 * @tc.desc: Tests for Load Shader. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginShaderTest, LoadShader, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    for (auto&& uri : {
             "test://shaders/test.shader",
         }) {
        EXPECT_TRUE(man->LoadShader(uri).GetResult());
    }
    EXPECT_FALSE(man->LoadShader("test://this/is/a/nonexistent.shader").GetResult());
}

/**
 * @tc.name: InvalidShader
 * @tc.desc: Tests for Invalid Shader. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginShaderTest, InvalidShader, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto shader = scene->CreateObject<IShader>(ClassId::Shader).GetResult();
    ASSERT_TRUE(shader);
    shader->Blend()->SetValue(true);
    shader->CullMode()->SetValue(CullModeFlags::FRONT_BIT);
}

/**
 * @tc.name: CullAndBlend
 * @tc.desc: Tests for Cull And Blend. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginShaderTest, CullAndBlend, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(man);
    auto shader = man->LoadShader("test://shaders/test.shader").GetResult();
    ASSERT_TRUE(shader);
    TestCullAndBlend(shader);
}

/**
 * @tc.name: DefaultShaderCullAndBlend
 * @tc.desc: Tests for Default Shader Cull And Blend. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginShaderTest, DefaultShaderCullAndBlend, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    auto shader = scene->CreateObject<IShader>(ClassId::Shader).GetResult();
    ASSERT_TRUE(shader);
    auto state = interface_cast<IShaderState>(shader);
    ASSERT_TRUE(state);
    EXPECT_TRUE(state->SetShaderState({}, {}));
    TestCullAndBlend(shader);
}

static void CopyShaderFile(CORE_NS::IFileManager& manager, BASE_NS::string_view path, BASE_NS::string_view name)
{
    BASE_NS::string file = BASE_NS::string("file://shader_reload/") + name;
    if (manager.FileExists(file)) {
        ASSERT_TRUE(manager.DeleteFile(file));
    }
    ASSERT_TRUE(CopyFile(manager, BASE_NS::string(path) + name, file));
}

static void CopyShaderFiles(CORE_NS::IFileManager& manager, BASE_NS::string_view path)
{
    auto dir = "test://shader_reload/" + path + "/";
    CopyShaderFile(manager, dir, "custom_core3d_dm_fw.frag.spv");
    CopyShaderFile(manager, dir, "custom_core3d_dm_fw.frag.spv.gl");
    CopyShaderFile(manager, dir, "custom_core3d_dm_fw.frag.spv.gles");
    CopyShaderFile(manager, dir, "custom_core3d_dm_fw.frag.spv.lsb");
    CopyShaderFile(manager, dir, "test.shader");
}

#ifdef _WIN32
/**
 * @tc.name: ShaderResourceReload
 * @tc.desc: Tests for Shader Resource Reload. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginShaderTest, ShaderResourceReload, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: ShaderResourceReload
 * @tc.desc: Tests for Shader Resource Reload. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginShaderTest, ShaderResourceReload, testing::ext::TestSize.Level1)
#endif
{
    CreateEmptyScene();
    auto material = CreateMaterial();
    ASSERT_TRUE(material);

    auto& manager = context->GetRenderer()->GetEngine().GetFileManager();
    CopyShaderFiles(manager, "1");

    EXPECT_TRUE(
        resources->AddResource("shader", ClassId::ShaderResource.Id().ToUid(), "file://shader_reload/test.shader"));
    auto shader = interface_pointer_cast<IShader>(resources->GetResource("shader"));
    ASSERT_TRUE(shader);

    material->MaterialShader()->SetValue(shader);
    UpdateScene();
    WaitForUserQueue();

    {
        auto meta = material->CustomProperties()->GetValue();
        EXPECT_EQ(meta->GetProperties().size(), 1);
        auto p1 = meta->GetProperty<uint32_t>("var1");
        ASSERT_TRUE(p1);
        EXPECT_EQ(p1->GetValue(), 1);

        auto textures = material->Textures()->GetValue();
        ASSERT_EQ(textures.size(), 11);
        EXPECT_EQ(META_NS::GetName(textures[0]), "texture_2");
    }

    std::atomic<uint64_t> count = 0;
    META_NS::EventHandler h;
    h.Subscribe<META_NS::IOnChanged>(material->Textures()->OnChanged(), [&] { ++count; });

    CopyShaderFiles(manager, "2");
    ASSERT_TRUE(resources->ReloadResource(interface_pointer_cast<CORE_NS::IResource>(shader)));

    UpdateScene();
    WaitForUserQueue();

    {
        auto meta = material->CustomProperties()->GetValue();
        EXPECT_EQ(meta->GetProperties().size(), 2);
        auto p1 = meta->GetProperty<uint32_t>("var1");
        ASSERT_TRUE(p1);
        EXPECT_EQ(p1->GetValue(), 1);
        auto p2 = meta->GetProperty<uint32_t>("var2");
        ASSERT_TRUE(p2);
        EXPECT_EQ(p2->GetValue(), 2);

        auto textures = material->Textures()->GetValue();
        ASSERT_EQ(textures.size(), 11);
        EXPECT_EQ(META_NS::GetName(textures[0]), "texture_1");
        EXPECT_EQ(count, 1);
    }
}

} // namespace UTest
SCENE_END_NAMESPACE()
