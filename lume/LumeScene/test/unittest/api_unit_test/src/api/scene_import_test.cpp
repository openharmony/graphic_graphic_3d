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

#ifdef WIN_MEMLEAK_CHECK
#include <crtdbg.h>
#endif

#include <scene/api/ecs_scene.h>
#include <scene/api/node.h>
#include <scene/api/resource.h>
#include <scene/api/resource_utils.h>
#include <scene/api/scene.h>
#include <scene/ext/scene_debug_info.h>

#include <gmock/gmock.h>

#include <meta/interface/resource/intf_resource_query.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_SceneApiImportTest : public ScenePluginTest {
    using Super = ScenePluginTest;
    void SetUp() override
    {
        Super::SetUp();
        scene_ = Scene(CreateEmptyScene());
        ASSERT_TRUE(scene_);
    }

    void TearDown() override
    {
        scene_.Release();
        Super::TearDown();
    }

protected:
    Scene& GetScene()
    {
        return scene_;
    }

private:
    Scene scene_ { nullptr };
};

class API_SceneApiImportTestP : public API_SceneApiImportTest,
                                public testing::WithParamInterface<BASE_NS::string_view> {};

using META_NS::Async;
using META_NS::CreateNew;
using META_NS::GetValue;
using META_NS::SetValue;

struct HandleCount {
    uint32_t bufferHandleCount {};
    uint32_t imageHandleCount {};
    uint32_t samplerHandleCount {};
};
bool operator==(const HandleCount& lhs, const HandleCount& rhs)
{
    return lhs.bufferHandleCount == rhs.bufferHandleCount && lhs.imageHandleCount == rhs.imageHandleCount &&
           lhs.samplerHandleCount == rhs.samplerHandleCount;
}
bool operator!=(const HandleCount& lhs, const HandleCount& rhs)
{
    return !(lhs == rhs);
}
bool operator>(const HandleCount& lhs, const HandleCount& rhs)
{
    // Require that one member needs to be larger and no member can be smaller
    bool anyLarger = lhs.bufferHandleCount > rhs.bufferHandleCount || lhs.imageHandleCount > rhs.imageHandleCount ||
                     lhs.samplerHandleCount > rhs.samplerHandleCount;
    bool anySmaller = lhs.bufferHandleCount < rhs.bufferHandleCount || lhs.imageHandleCount < rhs.imageHandleCount ||
                      lhs.samplerHandleCount < rhs.samplerHandleCount;
    return anyLarger && !anySmaller;
}

TEST_P(API_SceneApiImportTestP, ImportSceneHandleCount)
{
    auto uri = GetParam();
    auto scene = GetScene();
    auto sc = scene.GetPtr<IScene>();
    ASSERT_TRUE(sc);
    auto is = sc->GetInternalScene();
    ASSERT_TRUE(is);
    auto rc = is->GetContext()->GetRenderer();
    ASSERT_TRUE(rc);
    auto& man = rc->GetDevice().GetGpuResourceManager();
    auto render = [&]() { UpdateSceneAndRenderFrame(sc); };
    auto getHandlesCount = [&]() {
        HandleCount count;
        count.bufferHandleCount = man.GetBufferHandles().size();
        count.imageHandleCount = man.GetImageHandles().size();
        count.samplerHandleCount = man.GetSamplerHandles().size();
        return count;
    };
    render();
    auto factory = scene.GetResourceFactory();
    auto parent = factory.CreateNode("//parent");
    const auto original = getHandlesCount(); // Store original handle counts

    // Import a scene
    auto imported = GetSceneManager()->CreateScene(uri).GetResult();
    render();
    auto importedCount = getHandlesCount();
    EXPECT_GT(importedCount, original);
    // Delete the scene, handle count should go back to original
    imported.reset();
    render();
    EXPECT_EQ(getHandlesCount(), original);

    // Create another scene
    imported = GetSceneManager()->CreateScene(uri).GetResult();
    // Import the scene into our test scene + delete the imported one
    auto node = scene.ImportScene(imported, parent, "Animated");
    imported.reset();
    render();
    // We should end up with the same amount of gpu handles as we had after just importing the gtlf into
    // a standalone scene
    EXPECT_EQ(getHandlesCount(), importedCount);
    // Remove the imported node from the scene, this should destroy any associated gpu resources
    scene.RemoveNode(node);
    render();
    EXPECT_EQ(getHandlesCount(), original); // Imported things should now all be gone
}

INSTANTIATE_TEST_SUITE_P(API_SceneApiImportTestP, API_SceneApiImportTestP,
    testing::Values("test://AnimatedCube/AnimatedCube.gltf" /*, fails: "test://scene_resource/test1/test.scene"*/));

/**
 * @tc.name: ImportScene
 * @tc.desc: Tests for Import Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportScene, testing::ext::TestSize.Level1)
{
    auto imported = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    ASSERT_TRUE(imported);

    auto scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto parent = factory.CreateNode("//parent");
    EXPECT_TRUE(parent);

    auto importedNode = scene.ImportScene(imported, parent, "Animated");
    EXPECT_TRUE(importedNode);
    auto cube = scene.GetNode("//parent/Animated//AnimatedCube");
    EXPECT_TRUE(cube);

    auto importedNode2 = scene.ImportScene(imported, nullptr, "Animated");
    EXPECT_TRUE(importedNode2);
    auto cube2 = scene.GetNode("//Animated//AnimatedCube");
    EXPECT_TRUE(cube2);
}

/**
 * @tc.name: ImportEditorScene
 * @tc.desc: Tests for Import Editor Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportEditorScene, testing::ext::TestSize.Level1)
{
    auto imported = Scene(LoadScene("test://scene_resource/test1/test.scene"));
    ASSERT_TRUE(imported);

    auto scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto parent = factory.CreateNode("//parent");
    auto parent2 = factory.CreateNode("//parent2");
    ASSERT_TRUE(parent);
    ASSERT_TRUE(parent2);

    auto importedNode = scene.ImportScene(imported, parent, "Animated");
    EXPECT_TRUE(importedNode);

    auto importedNode2 = scene.ImportScene(imported, parent2, "Animated");
    EXPECT_TRUE(importedNode2);
}

/**
 * @tc.name: ImportSceneItself
 * @tc.desc: Tests for Import Scene Itself. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportSceneItself, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    auto node = scene.FindNode("AnimatedCube");
    EXPECT_TRUE(node);
    // Should fail (return null)
    auto imported = scene.ImportScene(scene, nullptr, "Invalid");
    EXPECT_FALSE(imported);
    // Scene should be unaffected
    EXPECT_EQ(node, scene.FindNode("AnimatedCube"));
}

/**
 * @tc.name: ImportSceneRemoveObject
 * @tc.desc: Test IScene::RemoveObject
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportSceneRemoveObject, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test_assets://editor/scene_test_project/assets/default.scene"));
    auto sc = scene.GetPtr<IScene>();

    auto manager = GetResourceManager(sc);
    ASSERT_TRUE(manager);

    ASSERT_TRUE(sc);
    auto is = sc->GetInternalScene();
    ASSERT_TRUE(is);
    auto rc = is->GetContext()->GetRenderer();
    auto render = [&]() {
        UpdateScene();
        rc->GetRenderer().RenderFrame({});
    };
    render();
    auto configuration = META_NS::GetValue(sc->RenderConfiguration());
    auto envProp = configuration->Environment();
    auto e = META_NS::GetValue(envProp);
    auto env = Environment(e);
    ASSERT_TRUE(env);

    auto res = env.GetPtr<CORE_NS::IResource>();
    ASSERT_TRUE(res);
    auto id = res->GetResourceId();

    envProp->ResetValue();

    auto original = GetEntityCount();
    EXPECT_TRUE(scene.RemoveObject(env));
    render();
    auto after = GetEntityCount();
    EXPECT_GT(original, after);

    auto child = scene.GetRootNode().GetChildren()[0];
    scene.RemoveObject(child);
    render();
    EXPECT_GT(after, GetEntityCount());
    EXPECT_FALSE(manager->GetResource(id, scene));
};

/**
 * @tc.name: ImportSceneRemoveObjectKeepIndex
 * @tc.desc: Test IScene::RemoveObject but keep resource in index
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportSceneRemoveObjectKeepIndex, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test_assets://editor/scene_test_project/assets/default.scene"));
    auto sc = scene.GetPtr<IScene>();

    ASSERT_TRUE(sc);
    auto manager = GetResourceManager(sc);
    ASSERT_TRUE(manager);
    auto configuration = META_NS::GetValue(sc->RenderConfiguration());
    auto envProp = configuration->Environment();
    ASSERT_TRUE(envProp);
    auto e = META_NS::GetValue(envProp);
    auto env = Environment(e);
    auto res = env.GetPtr<CORE_NS::IResource>();
    ASSERT_TRUE(res);
    auto id = res->GetResourceId();
    envProp->ResetValue();
    EXPECT_TRUE(scene.RemoveObject(env, { false }));
    // Resource should still be in index
    auto created = manager->GetResource(id, scene);
    EXPECT_TRUE(created);
}

/**
 * @tc.name: ImportSceneUri
 * @tc.desc: Tests for Import Scene Uri. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportSceneUri, testing::ext::TestSize.Level1)
{
    auto scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto parent = factory.CreateNode("//parent");
    EXPECT_TRUE(parent);

    auto importedNode = scene.ImportScene("test://AnimatedCube/AnimatedCube.gltf", nullptr, "Animated");
    EXPECT_TRUE(importedNode);
    auto cube = scene.GetNode("//Animated//AnimatedCube");
    ASSERT_TRUE(cube);
    EXPECT_EQ(cube.GetPath(), "//Animated//AnimatedCube");

    auto importedNode2 = scene.ImportScene("test://AnimatedCube/AnimatedCube.gltf", parent, "Animated");
    EXPECT_TRUE(importedNode2);
    auto cube2 = scene.GetNode("//parent/Animated//AnimatedCube");
    ASSERT_TRUE(cube2);
    EXPECT_EQ(cube2.GetPath(), "//parent/Animated//AnimatedCube");

    auto importedChild = cube2.ImportScene("test://AnimatedCube/AnimatedCube.gltf", "Animated");
    auto importedChildCube = scene.GetNode("//parent/Animated//AnimatedCube/Animated//AnimatedCube");
    ASSERT_TRUE(importedChildCube);
    EXPECT_EQ(importedChildCube.GetPath(), "//parent/Animated//AnimatedCube/Animated//AnimatedCube");
}

/**
 * @tc.name: ImportSceneResources
 * @tc.desc: Tests for Import Scene Resources. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportSceneResources, testing::ext::TestSize.Level1)
{
    auto scene = GetScene();
    auto sc = scene.GetPtr<IScene>();
    auto is = sc->GetInternalScene();
    ASSERT_TRUE(is);
    auto rc = is->GetContext()->GetRenderer();
    ASSERT_TRUE(rc);
    auto render = [&]() {
        UpdateScene();
        rc->GetRenderer().RenderFrame({});
    };
    auto res = is->GetContext()->GetResources();
    auto qi = interface_cast<META_NS::IResourceQuery>(res);
    ASSERT_TRUE(qi);

    auto rman = sc->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
    ASSERT_TRUE(rman);

    BASE_NS::string previous;
    SceneDebugInfo dinfo;
    uint32_t resAlive {};
    size_t groups {};
    size_t resources {};
    size_t gs {};

#ifdef WIN_MEMLEAK_CHECK
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtMemState state;
#endif

    for (uint32_t i = 1; i < 10; i++) {
        IScene::Ptr imported;
        if (auto m = GetSceneManager()) {
            imported = m->CreateScene("test://AnimatedCube/AnimatedCube.gltf").GetResult();
        }
        ASSERT_TRUE(imported);
        auto name = BASE_NS::to_string(i);
        scene.ImportScene(imported, nullptr, name);
        if (!previous.empty()) {
            auto prevNode = scene.FindNode(previous);
            scene.RemoveNode(prevNode);
        }
        previous = name;

        // instantiate all nodes and some textures/custom properties
        META_NS::Internal::ConstIterate(
            interface_cast<META_NS::IIterable>(sc->GetRootNode().GetResult()),
            [&](META_NS::IObject::Ptr nodeObj) {
                if (auto m = interface_cast<IMesh>(nodeObj)) {
                    auto subs = m->SubMeshes()->GetValue();
                    for (auto s : subs) {
                        auto mat = s->Material()->GetValue();
                        mat->Textures()->GetValue();
                        mat->CustomProperties()->GetValue();
                        auto shader = rman->LoadShader("test://shaders/test.shader").GetResult();
                        mat->MaterialShader()->SetValue(shader);
                    }
                }

                return true;
            },
            META_NS::IterateStrategy { META_NS::TraversalType::FULL_HIERARCHY });

        // start all animations
        for (auto anim : sc->GetAnimations().GetResult()) {
            auto a = interface_cast<META_NS::IStartableAnimation>(anim);
            a->Start();
        }

        render();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto di = is->GetDebugInfo();
        if (dinfo.nodeObjectsAlive) {
            EXPECT_EQ(di.nodeObjectsAlive, dinfo.nodeObjectsAlive);
            EXPECT_EQ(di.animationObjectsAlive, dinfo.animationObjectsAlive);
            EXPECT_EQ(resAlive, qi->GetAliveCount({ CORE_NS::MatchingResourceId() }));
            EXPECT_EQ(groups, res->GetResourceGroups().size());
            EXPECT_EQ(resources, res->GetResourceInfos().size());
            EXPECT_EQ(gs, rc->GetDevice().GetShaderManager().GetGraphicsStates().size());

#ifdef WIN_MEMLEAK_CHECK
            _CrtMemState current, diff;
            _CrtMemCheckpoint(&current);
            if (_CrtMemDifference(&diff, &state, &current)) {
                fflush(stdout);
                _CrtMemDumpStatistics(&diff);
            }
#endif
        } else {
            dinfo = di;
            resAlive = qi->GetAliveCount({ CORE_NS::MatchingResourceId() });
            groups = res->GetResourceGroups().size();
            resources = res->GetResourceInfos().size();
            gs = rc->GetDevice().GetShaderManager().GetGraphicsStates().size();
#ifdef WIN_MEMLEAK_CHECK
            _CrtMemCheckpoint(&state);
#endif
        }
    }
}

/**
 * @tc.name: ImportSceneDeadScene
 * @tc.desc: Tests for Import Scene Dead Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiImportTest, ImportSceneDeadScene, testing::ext::TestSize.Level1)
{
    auto rc = context->GetRenderer();
    ASSERT_TRUE(rc);
    auto& man = rc->GetDevice().GetGpuResourceManager();
    auto res = context->GetResources();
    auto render = [&]() {
        UpdateScene();
        rc->GetRenderer().RenderFrame({});
    };
    auto getHandlesCount = [&]() {
        HandleCount count;
        count.bufferHandleCount = man.GetBufferHandles().size();
        count.imageHandleCount = man.GetImageHandles().size();
        count.samplerHandleCount = man.GetSamplerHandles().size();
        return count;
    };
    rc->GetRenderer().RenderFrame({});
    auto original = getHandlesCount();
    auto shaders = rc->GetDevice().GetShaderManager().GetShaders().size();

    BASE_NS::vector<META_NS::IObject::Ptr> objs;
    {
        Scene scene(CreateEmptyScene());
        auto sc = scene.GetPtr<IScene>();
        auto is = sc->GetInternalScene();
        ASSERT_TRUE(is);

        auto res = is->GetContext()->GetResources();

        scene.ImportScene("test://AnimatedCube/AnimatedCube.gltf", nullptr, "test");

        // instantiate all nodes and some textures/custom properties
        META_NS::Internal::ConstIterate(
            interface_cast<META_NS::IIterable>(sc->GetRootNode().GetResult()),
            [&](META_NS::IObject::Ptr nodeObj) {
                if (auto m = interface_pointer_cast<IMesh>(nodeObj)) {
                    auto subs = m->SubMeshes()->GetValue();
                    for (auto s : subs) {
                        auto mat = s->Material()->GetValue();
                        mat->Textures()->GetValue();
                        mat->CustomProperties()->GetValue();

                        auto rman =
                            sc->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
                        auto shader = rman->LoadShader("test://shaders/test.shader").GetResult();
                        mat->MaterialShader()->SetValue(shader);
                        objs.push_back(interface_pointer_cast<META_NS::IObject>(shader));
                    }
                    objs.push_back(interface_pointer_cast<META_NS::IObject>(m));
                }

                return true;
            },
            META_NS::IterateStrategy { META_NS::TraversalType::FULL_HIERARCHY });

        // start all animations
        for (auto anim : sc->GetAnimations().GetResult()) {
            auto a = interface_cast<META_NS::IStartableAnimation>(anim);
            a->Start();
            objs.push_back(interface_pointer_cast<META_NS::IObject>(anim));
        }
        render();

        auto some = rc->GetDevice().GetShaderManager().GetShaders().size();
    }
    scene.reset();
    rc->GetRenderer().RenderFrame({});

    EXPECT_EQ(original, getHandlesCount());
    EXPECT_EQ(0, res->GetResourceInfos().size());

    auto sh = objs[0];
    objs.clear();
    sh.reset();
    rc->GetRenderer().RenderFrame({});

    EXPECT_EQ(shaders, rc->GetDevice().GetShaderManager().GetShaders().size());
}

} // namespace UTest
SCENE_END_NAMESPACE()
