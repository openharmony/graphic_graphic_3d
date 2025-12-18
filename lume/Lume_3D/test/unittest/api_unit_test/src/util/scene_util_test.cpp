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

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/implementation_uids.h>
#include <3d/loaders/intf_scene_loader.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/fixed_string.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>
#include <core/property_tools/core_metadata.inl>
#include <render/datastore/intf_render_data_store_manager.h>
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

namespace {
Entity CreateJoint(IEcs& ecs, const string_view name)
{
    Entity joint = ecs.GetEntityManager().Create();
    auto nameManager = GetManager<INameComponentManager>(ecs);
    nameManager->Create(joint);
    if (auto scopedHandle = nameManager->Write(joint); scopedHandle) {
        scopedHandle->name = name;
    }
    return joint;
}
} // namespace

/**
 * @tc.name: ShareSkinTest
 * @tc.desc: Tests for Share Skin Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, ShareSkinTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& sceneUtil = graphicsContext->GetSceneUtil();
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);

    Entity joint0 = CreateJoint(*ecs, "joint0");
    Entity joint1 = CreateJoint(*ecs, "joint1");
    Entity joint2 = CreateJoint(*ecs, "joint2");

    Entity sourceEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(sourceEntity);
    if (auto scopedHandle = skinJointsManager->Write(sourceEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint0;
        scopedHandle->jointEntities[1] = joint1;
        scopedHandle->jointEntities[2] = joint2;
    }

    Entity joint3 = CreateJoint(*ecs, "joint2");
    Entity joint4 = CreateJoint(*ecs, "joint3");
    Entity joint5 = CreateJoint(*ecs, "joint0");

    Entity targetEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(targetEntity);
    if (auto scopedHandle = skinJointsManager->Write(targetEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint3;
        scopedHandle->jointEntities[1] = joint4;
        scopedHandle->jointEntities[2] = joint5;
    }

    // Share to invalid entity should do nothing
    sceneUtil.ShareSkin(*ecs, Entity {}, sourceEntity);
    if (auto scopedHandle = skinJointsManager->Read(sourceEntity); scopedHandle) {
        EXPECT_EQ(3, scopedHandle->count);
        EXPECT_EQ(joint0, scopedHandle->jointEntities[0]);
        EXPECT_EQ(joint1, scopedHandle->jointEntities[1]);
        EXPECT_EQ(joint2, scopedHandle->jointEntities[2]);
    }

    // Share from invalid entity should do nothing
    sceneUtil.ShareSkin(*ecs, targetEntity, Entity {});
    if (auto scopedHandle = skinJointsManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(3, scopedHandle->count);
        EXPECT_EQ(joint3, scopedHandle->jointEntities[0]);
        EXPECT_EQ(joint4, scopedHandle->jointEntities[1]);
        EXPECT_EQ(joint5, scopedHandle->jointEntities[2]);
    }

    // Valid share should match joints by name component
    sceneUtil.ShareSkin(*ecs, targetEntity, sourceEntity);
    if (auto scopedHandle = skinJointsManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(3, scopedHandle->count);
        EXPECT_EQ(joint2, scopedHandle->jointEntities[0]);
        EXPECT_EQ(Entity {}, scopedHandle->jointEntities[1]);
        EXPECT_EQ(joint0, scopedHandle->jointEntities[2]);
    }
}

namespace {
Entity CreateAnimation(IEcs& ecs, Entity entity)
{
    auto animationManager = GetManager<IAnimationComponentManager>(ecs);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(ecs);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(ecs);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(ecs);
    auto animationSystem = GetSystem<IAnimationSystem>(ecs);
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(ecs);
    auto transformManager = GetManager<ITransformComponentManager>(ecs);
    auto nameManager = GetManager<INameComponentManager>(ecs);

    EntityReference keyFrameData = ecs.GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::VEC3_T;
        constexpr Math::Vec3 values[] = { { 0.0f, 0.0f, 0.0f }, { 5.0f, 5.0f, 5.0f } };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs.GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    vector<Entity> targetEntities;
    vector<EntityReference> tracks;
    auto jointData = skinJointsManager->Read(entity);
    for (uint32_t i = 0; i < jointData->count; ++i) {
        Entity targetEntity = jointData->jointEntities[i];
        transformManager->Create(targetEntity);
        targetEntities.push_back(targetEntity);

        EntityReference track = ecs.GetEntityManager().CreateReferenceCounted();
        animationTrackManager->Create(track);
        if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
            scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::STEP;
            scopedHandle->timestamps = timeStamps;
            scopedHandle->data = keyFrameData;
            scopedHandle->component = ITransformComponentManager::UID;
            scopedHandle->property = "position";
        }
        tracks.push_back(track);
    }

    Entity animation = ecs.GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks = tracks;
    }
    nameManager->Create(animation);
    if (auto scopedHandle = nameManager->Write(animation); scopedHandle) {
        scopedHandle->name = "animation";
    }

    IAnimationPlayback* playback =
        animationSystem->CreatePlayback(animation, { targetEntities.data(), targetEntities.size() });
    EXPECT_TRUE(playback);
    ecs.ProcessEvents();
    return animation;
}
} // namespace

/**
 * @tc.name: RetargetSkinAnimationTest
 * @tc.desc: Tests for Retarget Skin Animation Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, RetargetSkinAnimationTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& sceneUtil = graphicsContext->GetSceneUtil();
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);
    auto nameManager = GetManager<INameComponentManager>(*ecs);
    ASSERT_NE(nullptr, nameManager);
    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, renderMeshManager);
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, meshManager);

    Entity joint0 = CreateJoint(*ecs, "joint0");
    Entity joint1 = CreateJoint(*ecs, "joint1");
    Entity joint2 = CreateJoint(*ecs, "joint2");

    Entity sourceEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(sourceEntity);
    if (auto scopedHandle = skinJointsManager->Write(sourceEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint0;
        scopedHandle->jointEntities[1] = joint1;
        scopedHandle->jointEntities[2] = joint2;
    }
    renderMeshManager->Create(sourceEntity);
    if (auto rmHandle = renderMeshManager->Write(sourceEntity); rmHandle) {
        rmHandle->mesh = ecs->GetEntityManager().Create();
        meshManager->Create(rmHandle->mesh);
        if (auto meshHandle = meshManager->Write(rmHandle->mesh); meshHandle) {
            meshHandle->aabbMin.y = 0.0f;
            meshHandle->aabbMax.y = 1.0f;
        }
    }

    Entity animation = CreateAnimation(*ecs, sourceEntity);

    Entity joint3 = CreateJoint(*ecs, "joint2");
    Entity joint4 = CreateJoint(*ecs, "joint0");
    Entity joint5 = CreateJoint(*ecs, "joint1");

    Entity targetEntity = ecs->GetEntityManager().Create();
    skinJointsManager->Create(targetEntity);
    if (auto scopedHandle = skinJointsManager->Write(targetEntity); scopedHandle) {
        scopedHandle->count = 3;
        scopedHandle->jointEntities[0] = joint3;
        scopedHandle->jointEntities[1] = joint4;
        scopedHandle->jointEntities[2] = joint5;
    }
    renderMeshManager->Create(targetEntity);
    if (auto rmHandle = renderMeshManager->Write(targetEntity); rmHandle) {
        rmHandle->mesh = ecs->GetEntityManager().Create();
        meshManager->Create(rmHandle->mesh);
        if (auto meshHandle = meshManager->Write(rmHandle->mesh); meshHandle) {
            meshHandle->aabbMin.y = 0.0f;
            meshHandle->aabbMax.y = 2.0f;
        }
    }

    auto playback = sceneUtil.RetargetSkinAnimation(*ecs, targetEntity, sourceEntity, animation);
    ASSERT_NE(nullptr, playback);
    auto targetAnimation = playback->GetEntity();
    EXPECT_TRUE(nameManager->HasComponent(targetAnimation));
    if (auto scopedHandle = nameManager->Read(targetAnimation); scopedHandle) {
        EXPECT_EQ("animation retargeted", scopedHandle->name);
    }
    EXPECT_TRUE(animationManager->HasComponent(targetAnimation));
    if (auto animationHandle = animationManager->Read(targetAnimation); animationHandle) {
        ASSERT_EQ(3, animationHandle->tracks.size());
        for (const EntityReference& track : animationHandle->tracks) {
            if (auto trackHandle = animationTrackManager->Read(track); trackHandle) {
                EXPECT_EQ("position", trackHandle->property);
                EXPECT_EQ(ITransformComponentManager::UID, trackHandle->component);
                EXPECT_EQ(AnimationTrackComponent::Interpolation::STEP, trackHandle->interpolationMode);
                if (auto outputHandle = animationOutputManager->Read(trackHandle->data); outputHandle) {
                    ASSERT_EQ(6u * sizeof(float), outputHandle->data.size());
                    ASSERT_EQ(PropertyType::VEC3_T, outputHandle->type);
                    const auto* data = reinterpret_cast<const Math::Vec3*>(outputHandle->data.data());
                    EXPECT_EQ(0.0f, data[0].x);
                    EXPECT_EQ(0.0f, data[0].y);
                    EXPECT_EQ(0.0f, data[0].z);
                    EXPECT_EQ(10.0f, data[1].x);
                    EXPECT_EQ(10.0f, data[1].y);
                    EXPECT_EQ(10.0f, data[1].z);
                }
            }
        }
    }
}

/**
 * @tc.name: CameraLookAt
 * @tc.desc: Tests for Camera Look At. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, CameraLookAt, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& sceneUtil = graphicsContext->GetSceneUtil();

    auto* nodeSystem = GetSystem<INodeSystem>(*ecs);
    ISceneNode* nodes[4];
    for (auto& node : nodes) {
        node = nodeSystem->CreateNode();
    }
    std::transform(std::begin(nodes) + 1, std::end(nodes), std::begin(nodes), std::begin(nodes) + 1,
        [](ISceneNode* node, ISceneNode* parent) {
            node->SetParent(*parent);
            node->SetPosition(Math::Vec3(1.f, 0.f, 0.f));
            return node;
        });

    sceneUtil.CameraLookAt(
        *ecs, nodes[3]->GetEntity(), Math::Vec3(3.f, 1.f, 2.f), Math::Vec3(0.f, 1.f, 2.f), Math::Vec3(0.f, 1.f, 0.f));

    ecs->ProcessEvents();
    ecs->Update(0U, 0U);
    ecs->ProcessEvents();

    auto* wmm = GetManager<IWorldMatrixComponentManager>(*ecs);
    if (auto nodeHandle = wmm->Read(nodes[3]->GetEntity())) {
        EXPECT_EQ(nodeHandle->matrix.w, Math::Vec4(3.f, 1.f, 2.f, 1.f));
        Math::Vec3 scale;
        Math::Quat orientation;
        Math::Vec3 translation;
        Math::Vec3 skew;
        Math::Vec4 perspective;
        EXPECT_TRUE(Math::Decompose(nodeHandle->matrix, scale, orientation, translation, skew, perspective));
        EXPECT_EQ(scale, Math::Vec3(1.f, 1.f, 1.f));
        EXPECT_EQ(orientation, Math::AngleAxis(Math::DEG2RAD * 90.f, Math::Vec3(0.f, 1.f, 0.f)));
        EXPECT_EQ(translation, Math::Vec3(3.f, 1.f, 2.f));
    }

    sceneUtil.CameraLookAt(
        *ecs, nodes[3]->GetEntity(), Math::Vec3(1.f, 2.f, -1.f), Math::Vec3(1.f, 2.f, 0.f), Math::Vec3(0.f, 1.f, 0.f));

    ecs->ProcessEvents();
    ecs->Update(1U, 1U);
    ecs->ProcessEvents();

    if (auto nodeHandle = wmm->Read(nodes[3]->GetEntity())) {
        EXPECT_EQ(nodeHandle->matrix.w, Math::Vec4(1.f, 2.f, -1.f, 1.f));
        Math::Vec3 scale;
        Math::Quat orientation;
        Math::Vec3 translation;
        Math::Vec3 skew;
        Math::Vec4 perspective;
        EXPECT_TRUE(Math::Decompose(nodeHandle->matrix, scale, orientation, translation, skew, perspective));
        EXPECT_EQ(scale, Math::Vec3(1.f, 1.f, 1.f));
        EXPECT_EQ(orientation, Math::AngleAxis(Math::DEG2RAD * 180.f, Math::Vec3(0.f, 1.f, 0.f)));
        EXPECT_EQ(translation, Math::Vec3(1.f, 2.f, -1.f));
    }
}

namespace {
size_t GetEntityCount(IEcs& ecs)
{
    size_t entityCount = 0;

    auto& entityManager = ecs.GetEntityManager();
    auto aliveBegin = entityManager.Begin();
    auto aliveEnd = entityManager.End();

    for (auto& it = aliveBegin; !it->Compare(aliveEnd); it->Next()) {
        entityCount++;
    }

    return entityCount;
}
} // namespace

/**
 * @tc.name: CloneEcsSimple
 * @tc.desc: Tests for Clone Ecs Simple. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, CloneEcsSimple, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    auto sceneNode = nodeSystem->CreateNode();
    sceneNode->SetName("other");
    {
        auto ecs2 = UTest::CreateAndInitializeDefaultEcs(*engine);
        auto nodeSystem2 = GetSystem<INodeSystem>(*ecs2);
        auto sceneNode2 = nodeSystem2->CreateNode();
        sceneNode2->SetName("test");
        const auto clone =
            graphicsContext->GetSceneUtil().Clone(*ecs2, sceneNode2->GetEntity(), *ecs, sceneNode->GetEntity());
        EXPECT_TRUE(EntityUtil::IsValid(clone.node));
        auto children = sceneNode2->GetChildren();
        ASSERT_EQ(children.size(), 1U);
        EXPECT_EQ(children[0U]->GetEntity(), clone.node);
    }
}

/**
 * @tc.name: CloneEcs
 * @tc.desc: Tests for Clone Ecs. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, CloneEcs, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    constexpr string_view filename = "test://gltf/SimpleSkin/SimpleSkin.gltf";
    ISceneLoader::Ptr loader = graphicsContext->GetSceneUtil().GetSceneLoader(filename);
    ASSERT_TRUE(loader);

    ISceneLoader::Result result = loader->Load(filename);
    EXPECT_FALSE(result.error);
    ASSERT_TRUE(result.data);

    ISceneImporter::Ptr importer = loader->CreateSceneImporter(*ecs);
    ASSERT_TRUE(importer);

    importer->ImportResources(result.data, SceneImportFlagBits::CORE_IMPORT_COMPONENT_FLAG_BITS_ALL);
    const auto& importResult = importer->GetResult();
    EXPECT_FALSE(importResult.error);

    const Entity sceneEnity = importer->ImportScene(0U);
    {
        auto ecs2 = UTest::CreateAndInitializeDefaultEcs(*engine);
        const auto sceneClone = graphicsContext->GetSceneUtil().Clone(*ecs2, {}, *ecs, sceneEnity);
        auto nodeSystem = GetSystem<INodeSystem>(*ecs);
        vector<ISceneNode*> stack(1U, nodeSystem->GetNode(sceneEnity));
        auto nodeSystem2 = GetSystem<INodeSystem>(*ecs2);
        vector<ISceneNode*> stack2(1U, nodeSystem2->GetNode(sceneClone.node));
        while (!stack.empty() && !stack2.empty()) {
            ISceneNode* sceneNode = stack.back();
            stack.pop_back();
            EXPECT_TRUE(sceneNode);
            if (!sceneNode) {
                continue;
            }
            auto children = sceneNode->GetChildren();
            ISceneNode* scene2Node = stack2.back();
            stack2.pop_back();
            EXPECT_TRUE(scene2Node);
            if (!scene2Node) {
                continue;
            }
            auto children2 = scene2Node->GetChildren();
            for (auto child : children) {
                auto name = child->GetName();
                auto pos = std::find_if(children2.cbegin(), children2.cend(),
                    [&name](const auto& node) { return node->GetName() == name; });
                EXPECT_NE(pos, children2.cend());
                if (pos != children2.cend()) {
                    stack.push_back(child);
                    stack2.push_back(*pos);
                }
            }
        }
    }
    {
        auto ecs2 = UTest::CreateAndInitializeDefaultEcs(*engine);
        vector<Entity> clones = graphicsContext->GetSceneUtil().Clone(*ecs2, *ecs);
        EXPECT_EQ(GetEntityCount(*ecs), clones.size());
    }
    {
        auto ecs2 = UTest::CreateAndInitializeDefaultEcs(*engine);

        const auto sceneClone = graphicsContext->GetSceneUtil().Clone(*ecs2, {}, *ecs, sceneEnity);

        BASE_NS::unordered_map<CORE_NS::Entity, ISceneUtil::MappedEntity> mapping;
        mapping[sceneEnity] = { sceneClone.node };

        const auto partialClone = graphicsContext->GetSceneUtil().Clone(*ecs2, *ecs, mapping);

        EXPECT_EQ(GetEntityCount(*ecs), partialClone.newEntities.size() + 1);
        ASSERT_EQ(partialClone.extMapEntities.size(), 1);
        EXPECT_EQ(partialClone.extMapEntities[0].id, sceneClone.node.id);

        auto nodeSystem = GetSystem<INodeSystem>(*ecs);
        vector<ISceneNode*> stack(1U, nodeSystem->GetNode(sceneEnity));
        auto nodeSystem2 = GetSystem<INodeSystem>(*ecs2);
        vector<ISceneNode*> stack2(1U, nodeSystem2->GetNode(sceneClone.node));
        while (!stack.empty() && !stack2.empty()) {
            ISceneNode* sceneNode = stack.back();
            stack.pop_back();
            EXPECT_TRUE(sceneNode);
            if (!sceneNode) {
                continue;
            }
            auto children = sceneNode->GetChildren();
            ISceneNode* scene2Node = stack2.back();
            stack2.pop_back();
            EXPECT_TRUE(scene2Node);
            if (!scene2Node) {
                continue;
            }
            auto children2 = scene2Node->GetChildren();
            for (auto child : children) {
                auto name = child->GetName();
                auto pos = std::find_if(children2.cbegin(), children2.cend(),
                    [&name](const auto& node) { return node->GetName() == name; });
                EXPECT_NE(pos, children2.cend());
                if (pos != children2.cend()) {
                    stack.push_back(child);
                    stack2.push_back(*pos);
                }
            }
        }
    }
    {
        auto ecs2 = UTest::CreateAndInitializeDefaultEcs(*engine);

        auto& entityManager = ecs2->GetEntityManager();
        auto entity = entityManager.Create();

        BASE_NS::unordered_map<CORE_NS::Entity, ISceneUtil::MappedEntity> mapping;
        mapping[sceneEnity] = { entity, true };

        const auto partialClone = graphicsContext->GetSceneUtil().Clone(*ecs2, *ecs, mapping);

        EXPECT_EQ(GetEntityCount(*ecs), partialClone.newEntities.size());
        ASSERT_EQ(partialClone.extMapEntities.size(), 1);
        EXPECT_EQ(partialClone.extMapEntities[0].id, entity.id);

        auto nodeSystem = GetSystem<INodeSystem>(*ecs);
        vector<ISceneNode*> stack(1U, nodeSystem->GetNode(sceneEnity));
        auto nodeSystem2 = GetSystem<INodeSystem>(*ecs2);
        vector<ISceneNode*> stack2(1U, nodeSystem2->GetNode(entity));
        while (!stack.empty() && !stack2.empty()) {
            ISceneNode* sceneNode = stack.back();
            stack.pop_back();
            EXPECT_TRUE(sceneNode);
            if (!sceneNode) {
                continue;
            }
            auto children = sceneNode->GetChildren();
            ISceneNode* scene2Node = stack2.back();
            stack2.pop_back();
            EXPECT_TRUE(scene2Node);
            if (!scene2Node) {
                continue;
            }
            auto children2 = scene2Node->GetChildren();
            for (auto child : children) {
                auto name = child->GetName();
                auto pos = std::find_if(children2.cbegin(), children2.cend(),
                    [&name](const auto& node) { return node->GetName() == name; });
                EXPECT_NE(pos, children2.cend());
                if (pos != children2.cend()) {
                    stack.push_back(child);
                    stack2.push_back(*pos);
                }
            }
        }
    }
}

/**
 * @tc.name: Ecs CloneNode
 * @tc.desc: Tests for Ecs CloneNode. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, EcsCloneNode, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    constexpr string_view filename = "test://gltf/SimpleSkin/SimpleSkin.gltf";
    ISceneLoader::Ptr loader = graphicsContext->GetSceneUtil().GetSceneLoader(filename);
    ASSERT_TRUE(loader);

    ISceneLoader::Result result = loader->Load(filename);
    EXPECT_FALSE(result.error);
    ASSERT_TRUE(result.data);

    ISceneImporter::Ptr importer = loader->CreateSceneImporter(*ecs);
    ASSERT_TRUE(importer);

    importer->ImportResources(result.data, SceneImportFlagBits::CORE_IMPORT_COMPONENT_FLAG_BITS_ALL);
    const auto& importResult = importer->GetResult();
    EXPECT_FALSE(importResult.error);

    const Entity sceneEnity = importer->ImportScene(0U);
    {
        auto nodeSystem = GetSystem<INodeSystem>(*ecs);
        vector<ISceneNode*> stack(1U, nodeSystem->GetNode(sceneEnity));
        while (!stack.empty()) {
            ISceneNode* sceneNode = stack.back();
            stack.pop_back();
            EXPECT_TRUE(sceneNode);
            if (!sceneNode) {
                continue;
            }
        }
    }
    {
        vector<Entity> clones = graphicsContext->GetSceneUtil().Clone(*ecs, sceneEnity, sceneEnity).entities;
        // Since we are cloning the root node, the size will be double the size of the cloned data
        EXPECT_EQ(GetEntityCount(*ecs), clones.size() * 2);
    }
}

namespace {
BASE_NS::pair<CORE_NS::EntityReference, CORE_NS::EntityReference> CreateShader(
    CORE_NS::IEcs& ecs, RENDER_NS::IRenderContext& renderContext, BASE_NS::string uri)
{
    auto uriManager = GetManager<CORE3D_NS::IUriComponentManager>(ecs);
    auto renderHandleManager = GetManager<CORE3D_NS::IRenderHandleComponentManager>(ecs);
    auto& shaderMgr = renderContext.GetDevice().GetShaderManager();

    shaderMgr.LoadShaderFile(uri);
    auto shaderHandle = shaderMgr.GetShaderHandle(uri);
    auto stateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle);

    const EntityReference shader = ecs.GetEntityManager().CreateReferenceCounted();
    const EntityReference state = ecs.GetEntityManager().CreateReferenceCounted();
    renderHandleManager->Create(shader);
    renderHandleManager->Create(state);
    renderHandleManager->Write(shader)->reference = shaderHandle;
    renderHandleManager->Write(state)->reference = stateHandle;

    return { shader, state };
}

template<typename ComponentManager>
Entity LookupResourceByUri(
    string_view uri, const IUriComponentManager& uriManager, const ComponentManager& componentManager)
{
    const auto& entityManager = uriManager.GetEcs().GetEntityManager();
    const auto uriComponents = uriManager.GetComponentCount();
    for (auto i = 0u; i < uriComponents; ++i) {
        if (Entity entity = uriManager.GetEntity(i); entityManager.IsAlive(entity)) {
            if (const auto uriHandle = uriManager.Read(i); uriHandle) {
                const bool found = uriHandle->uri == uri;
                if (found && componentManager.HasComponent(entity)) {
                    return entity;
                }
            }
        }
    }
    return {};
}
} // namespace

/**
 * @tc.name: CloneMaterialWithCustomShader
 * @tc.desc: Tests for Clone Material With Custom Shader. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, CloneMaterialWithCustomShader, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = UTest::CreateAndInitializeDefaultEcs(*engine);
    testContext->renderContext->GetRenderer().RenderFrame({});

    auto ecs2 = engine->CreateEcs();
    ecs2->Initialize();

    const Entity sceneEntity = ecs2->GetEntityManager().Create();
    // create a material in second ecs
    const Entity materialEntity = ecs2->GetEntityManager().Create();
    {
        auto materialManager = GetManager<IMaterialComponentManager>(*ecs2);
        materialManager->Create(materialEntity);
        if (auto scopedHandle = materialManager->Write(materialEntity)) {
            auto [shader, state] = CreateShader(*ecs2, *testContext->renderContext, "test://shader/test.shader");
            scopedHandle->materialShader.shader = shader;
            scopedHandle->materialShader.graphicsState = state;
        }

        if (auto scopedHandle = materialManager->Write(materialEntity)) {
            scopedHandle->textures[0U].image = ecs2->GetEntityManager().CreateReferenceCounted();
            scopedHandle->textures[2U].image = ecs2->GetEntityManager().CreateReferenceCounted();
            if (auto* cb = scopedHandle->customBindingProperties; 0 && cb) {
                {
                    const EntityReference entRef = ecs2->GetEntityManager().CreateReferenceCounted();
                    const bool boolVal =
                        CORE_NS::SetPropertyValue(*cb, "uUserCustomImage", PropertyType::ENTITY_REFERENCE_T, entRef);
                    EXPECT_EQ(boolVal, true);
                }
                {
                    const EntityReference entRef = ecs2->GetEntityManager().CreateReferenceCounted();
                    const bool boolVal =
                        CORE_NS::SetPropertyValue(*cb, "uUserCustomSampler", PropertyType::ENTITY_REFERENCE_T, entRef);
                    EXPECT_EQ(boolVal, true);
                }
            }
        }
    }
    // create a new ecs where a gltf is imported
    auto importEcs = engine->CreateEcs();
    importEcs->Initialize();

    CORE3D_NS::ISceneImporter::Result importResult;
    {
        constexpr string_view filename = "test://gltf/AnimatedCube/glTF/AnimatedCube.gltf";
        ISceneLoader::Ptr loader = graphicsContext->GetSceneUtil().GetSceneLoader(filename);
        ASSERT_TRUE(loader);

        ISceneLoader::Result result = loader->Load(filename);
        EXPECT_FALSE(result.error);
        ASSERT_TRUE(result.data);

        ISceneImporter::Ptr importer = loader->CreateSceneImporter(*importEcs);
        ASSERT_TRUE(importer);

        importer->ImportResources(result.data, SceneImportFlagBits::CORE_IMPORT_COMPONENT_FLAG_BITS_ALL);
        importResult = importer->GetResult();
        EXPECT_FALSE(importResult.error);

        const Entity sceneEnity = importer->ImportScene(0U);
    }
    // clone from the import ecs to second ecs
    vector<Entity> clones = graphicsContext->GetSceneUtil().Clone(*ecs2, *importEcs);

    // change the material of a imported mesh to one with a custom material
    {
        auto meshManager = GetManager<IMeshComponentManager>(*ecs2);
        meshManager->Write(0U)->submeshes[0U].material = materialEntity;
    }

    // clone the final scene to first ecs
    {
        vector<Entity> clones = graphicsContext->GetSceneUtil().Clone(*ecs, *ecs2);
    }
}

/**
 * @tc.name: SphereInsideCameraFrustum
 * @tc.desc: Tests for Sphere Inside Camera Frustum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SceneUtil, SphereInsideCameraFrustum, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;

    auto& sceneUtil = graphicsContext->GetSceneUtil();

    auto ecs = engine->CreateEcs();
    ecs->Initialize();
    // valid entity but missng managers
    EXPECT_FALSE(
        sceneUtil.IsSphereInsideCameraFrustum(*ecs, ecs->GetEntityManager().Create(), { -5.f, 0.f, 0.f }, 0.5));

    ecs = testContext->ecs;
    // invalid entity
    EXPECT_FALSE(sceneUtil.IsSphereInsideCameraFrustum(*ecs, {}, { -5.f, 0.f, 0.f }, 0.5));

    Entity cameraEntity = sceneUtil.CreateCamera(*ecs, {}, { 0.f, 0.f, 0.f, 1.f }, 0.1f, 20.f, 45.f);
    sceneUtil.CameraLookAt(*ecs, cameraEntity, { 10.f, 10.f, -10.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });
    Math::Vec3 center;

    EXPECT_FALSE(sceneUtil.IsSphereInsideCameraFrustum(*ecs, cameraEntity, { -5.f, 0.f, 0.f }, 0.5));
    EXPECT_FALSE(sceneUtil.IsSphereInsideCameraFrustum(*ecs, cameraEntity, { 10.f, 0.f, 0.f }, 0.5));
    EXPECT_TRUE(sceneUtil.IsSphereInsideCameraFrustum(*ecs, cameraEntity, { 0.f, 0.f, 0.f }, 0.5));
    EXPECT_FALSE(sceneUtil.IsSphereInsideCameraFrustum(*ecs, cameraEntity, { 0.f, 10.f, 0.f }, 0.5));
    EXPECT_FALSE(sceneUtil.IsSphereInsideCameraFrustum(*ecs, cameraEntity, { 0.f, -10.f, 0.f }, 0.5));
}