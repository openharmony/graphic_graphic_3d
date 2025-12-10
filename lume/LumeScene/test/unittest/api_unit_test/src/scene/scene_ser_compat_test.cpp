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

// clang-format off
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
// clang-format on

#include <scene/api/scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_create_mesh.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_text.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>

#include <3d/ecs/components/mesh_component.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_filesystem_api.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/api/metadata_util.h>
#include <meta/api/object.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

#include "scene/scene_test.h"

META_TYPE(CORE_NS::IResourceType::Ptr)

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_SceneSerCompatTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        // construct scene manager which registers the resources types
        META_NS::GetObjectRegistry().Create(ClassId::SceneManager, params);
        renderman_ =
            META_NS::GetObjectRegistry().Create<IRenderResourceManager>(ClassId::RenderResourceManager, params);

        // Get access to protocols defined in engine file manager
        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));

        ASSERT_TRUE(renderman_);
    }

    void TearDown() override
    {
        ScenePluginTest::TearDown();
    }

    template<typename Interface>
    typename Interface::Ptr Import(BASE_NS::string_view file, META_NS::SharedPtrIInterface userContext = nullptr)
    {
        if (auto f = GetTestEnv()->engine->GetFileManager().OpenFile(file)) {
            auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
            importer->SetResourceManager(resources);
            importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(userContext));
            return interface_pointer_cast<Interface>(importer->Import(*f));
        }
        return nullptr;
    }

private:
    IRenderResourceManager::Ptr renderman_;
};

/**
 * @tc.name: EnvironmentCompatibility
 * @tc.desc: Tests for Environment Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerCompatTest, EnvironmentCompatibility, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(resources->Import("test://compatibility/env/env_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = Import<IScene>("test://compatibility/env/env_test.json");
    ASSERT_TRUE(scene);

    auto env = scene->RenderConfiguration()->GetValue()->Environment()->GetValue();
    ASSERT_TRUE(env);

    EXPECT_EQ(env->Background()->GetValue(), EnvBackgroundType::IMAGE);
    EXPECT_TRUE(env->EnvironmentImage()->GetValue());
    EXPECT_EQ(env->RadianceCubemapMipCount()->GetValue(), 4);
    auto vec = env->IrradianceCoefficients()->GetValue();
    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], BASE_NS::Math::Vec3(1, 1, 1));
    EXPECT_EQ(vec[1], BASE_NS::Math::Vec3(1, 2, 3));
}

/**
 * @tc.name: SceneNodeSer
 * @tc.desc: Tests for Scene Node Ser. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerCompatTest, SceneNodeSer, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(
        resources->Import("test://compatibility/scene_ser/test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource("app://test.scene"));
    ASSERT_TRUE(scene);
}

/**
 * @tc.name: ExternalNodeCompatibility
 * @tc.desc: Tests for External Node Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerCompatTest, ExternalNodeCompatibility, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(resources->Import("test://compatibility/external_node/ext_node.resources"),
        CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource("scene"));
    ASSERT_TRUE(scene);
}

} // namespace UTest
SCENE_END_NAMESPACE()
