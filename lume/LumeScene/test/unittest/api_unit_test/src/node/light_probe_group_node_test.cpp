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

// clang-format off
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
// clang-format on

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_light_probe_group.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>
#include <scene_metadata_importer/interface/intf_scene_exporter.h>
#include <scene_metadata_importer/interface/intf_scene_importer.h>

#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>

#include <meta/interface/intf_object_registry.h>

#include "scene/scene_test.h"

CORE3D_BEGIN_NAMESPACE()
inline bool operator==(const LightProbeGroupComponent::LightProbe& lhs, const LightProbeGroupComponent::LightProbe& rhs)
{
    return lhs.position == rhs.position && lhs.shCoefficients[0] == rhs.shCoefficients[0] &&
           lhs.shCoefficients[1] == rhs.shCoefficients[1] && lhs.shCoefficients[2] == rhs.shCoefficients[2] &&
           lhs.shCoefficients[3] == rhs.shCoefficients[3] && lhs.shCoefficients[4] == rhs.shCoefficients[4] &&
           lhs.shCoefficients[5] == rhs.shCoefficients[5] && lhs.shCoefficients[6] == rhs.shCoefficients[6] &&
           lhs.shCoefficients[7] == rhs.shCoefficients[7] && lhs.shCoefficients[8] == rhs.shCoefficients[8] &&
           lhs.bentNormal == rhs.bentNormal && lhs.ao == rhs.ao;
}
CORE3D_END_NAMESPACE()

SCENE_BEGIN_NAMESPACE()

namespace UTest {

class API_ScenePluginLightProbeGroupNodeTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);
        auto node = scene->CreateNode("//test", ClassId::LightProbeGroupNode).GetResult();
        ASSERT_TRUE(node);
        lbg_ = interface_pointer_cast<ILightProbeGroup>(node);
        ASSERT_TRUE(lbg_);
        UpdateScene();
    }

    void TearDown() override
    {
        lbg_.reset();
    }

    bool ExportScene(const IScene::Ptr& s, BASE_NS::string_view file)
    {
        auto exporter = META_NS::GetObjectRegistry().Create<ISceneExporter>(ClassId::SceneExporter);
        auto f = GetTestEnv()->engine->GetFileManager().CreateFile(file);
        return f && exporter && exporter->ExportScene(*f, s);
    }

    IScene::Ptr ImportScene(BASE_NS::string_view file)
    {
        auto importer = META_NS::GetObjectRegistry().Create<ISceneImporter>(ClassId::SceneImporter, params);
        auto f = GetTestEnv()->engine->GetFileManager().OpenFile(file);
        return (f && importer) ? importer->ImportScene(*f, {}) : nullptr;
    }

protected:
    ILightProbeGroup::Ptr lbg_;
};

/**
 * @tc.name: GetAndSetLightProbeGroup
 * @tc.desc: Tests for ILightProbeGroup::GetLightProbeGroup and ILightProbeGroup::SetLightProbeGroup and
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLightProbeGroupNodeTest, GetAndSetLightProbeGroup, testing::ext::TestSize.Level1)
{
    auto group = lbg_->GetLightProbeGroup();
    ASSERT_TRUE(group.empty());

    LightProbe lp{};
    lp.position = {1.f, 2.f, 3.f};
    lp.shCoefficients[0] = {4.f, 5.f, 6.f};

    group.push_back(lp);
    EXPECT_TRUE(lbg_->SetLightProbeGroup(group));

    auto group2 = lbg_->GetLightProbeGroup();
    ASSERT_EQ(group2.size(), 1);

    EXPECT_EQ(group2[0], lp);
}

/**
 * @tc.name: SerializeLightProbeGroup
 * @tc.desc: Tests serialization round-trip for LightProbeGroupNode
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLightProbeGroupNodeTest, SerializeLightProbeGroup, testing::ext::TestSize.Level1)
{
    static constexpr auto LP_COUNT = 3;
    LightProbeGroup group;
    for (auto i = 0; i < LP_COUNT; i++) {
        auto base = float(i);
        LightProbe lp;
        lp.position = {base, base + .5f, base + 1.f};
        for (size_t i = 0; i < 9; ++i) {
            lp.shCoefficients[i] = {base + i, base + i + .5f, base + i + 1.f};
        }
        lp.bentNormal = {base + 100.f, base + 100.5f, base + 101.f};
        lp.ao = base * 0.25f;
        group.push_back(lp);
    }

    ASSERT_TRUE(lbg_->SetLightProbeGroup(group));

    // Create another LightProbeGroupNode, but don't add any light probes. Should still serialize
    auto emptyLbg = scene->CreateNode("//empty_lbg", ClassId::LightProbeGroupNode).GetResult();
    ASSERT_TRUE(emptyLbg);

    ASSERT_TRUE(ExportScene(scene, "app://lpg_test.json"));

    auto imported = ImportScene("app://lpg_test.json");
    ASSERT_TRUE(imported);

    auto node = imported->FindNode("//test").GetResult();
    ASSERT_TRUE(node);
    auto importedLpg = interface_cast<ILightProbeGroup>(node);
    ASSERT_TRUE(importedLpg);

    auto importedGroup = importedLpg->GetLightProbeGroup();
    ASSERT_EQ(importedGroup.size(), LP_COUNT);
    EXPECT_EQ(importedGroup[0], group[0]);
    EXPECT_EQ(importedGroup[1], group[1]);
    EXPECT_EQ(importedGroup[2], group[2]);

    auto emptyLpgNode = imported->FindNode("//empty_lbg").GetResult();
    ASSERT_TRUE(emptyLpgNode);
    auto importedEmptyLpg = interface_cast<ILightProbeGroup>(emptyLpgNode);
    ASSERT_TRUE(importedEmptyLpg);

    EXPECT_TRUE(importedEmptyLpg->GetLightProbeGroup().empty());
}

/**
 * @tc.name: InvalidNode
 * @tc.desc: Test ILightProbeGroup interface
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLightProbeGroupNodeTest, Functions, testing::ext::TestSize.Level1)
{
    LightProbeGroup group;
    LightProbe lp{};
    float base = 1.f;
    lp.position = {base, base + .5f, base + 1.f};
    for (size_t i = 0; i < 9; ++i) {
        lp.shCoefficients[i] = {base + i, base + i + .5f, base + i + 1.f};
    }
    group.push_back(lp);

    EXPECT_TRUE(lbg_->GetLightProbeGroup().empty());
    EXPECT_TRUE(lbg_->SetLightProbeGroup(group));
    EXPECT_EQ(lbg_->GetLightProbeAt(0), lp);
    EXPECT_EQ(lbg_->GetLightProbeAt(1), LightProbe{});
    EXPECT_TRUE(lbg_->SetLightProbeAt(lp, 0));
    EXPECT_FALSE(lbg_->SetLightProbeAt(lp, 1));
}

/**
 * @tc.name: InvalidNode
 * @tc.desc: Test invalid cases
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLightProbeGroupNodeTest, Invalid, testing::ext::TestSize.Level1)
{
    auto ce = interface_cast<ICreateEntity>(lbg_);
    ASSERT_TRUE(ce);
    EXPECT_FALSE(CORE_NS::EntityUtil::IsValid(ce->CreateEntity(nullptr)));
    auto ecs = interface_cast<IEcsObjectAccess>(lbg_);
    ASSERT_TRUE(ecs);
    ecs->SetEcsObject(nullptr);
}

}  // namespace UTest

SCENE_END_NAMESPACE()
