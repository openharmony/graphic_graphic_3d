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

#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/resource/types.h>

#include <core/intf_engine.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/api/animation.h>
#include <meta/interface/animation/intf_animation.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class AssetObjectLifetimeTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        // Mirror ResourceTypeTest: give the resource manager a file manager so glTF
        // image/material resources can be resolved through it.
        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&context->GetRenderer()->GetEngine().GetFileManager()));
    }

    // Counts the scene's registered resources by ClassId type.
    struct ResourceTypeCounts {
        size_t images{};
        size_t materials{};
        size_t animations{};
    };

    ResourceTypeCounts CountSceneResources(const IScene::Ptr& sc)
    {
        ResourceTypeCounts counts;
        auto rman = GetResourceManager(sc);
        EXPECT_TRUE(rman);
        if (!rman) {
            return counts;
        }
        for (auto&& info : rman->GetResourceInfos(interface_pointer_cast<CORE_NS::IInterface>(sc))) {
            if (info.type == ClassId::ImageResource.Id().ToUid()) {
                ++counts.images;
            } else if (info.type == ClassId::MaterialResource.Id().ToUid()) {
                ++counts.materials;
            } else if (info.type == ClassId::AnimationResource.Id().ToUid()) {
                ++counts.animations;
            }
        }
        return counts;
    }
};

/**
 * @tc.name: ImportedAnimationSurvivesImporterRelease
 * @tc.desc: After AssetObject::Load returns (the glTF importer has been destroyed),
 *           the imported animation is still present and can be driven. The animation
 *           entities are kept alive only by AssetObject::importedResources_; without
 *           that anchor they would be reclaimed by ECS entity GC and the scene would
 *           lose its animations.
 * @tc.type: FUNC
 */
UNIT_TEST_F(AssetObjectLifetimeTest, ImportedAnimationSurvivesImporterRelease, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    // Pump several scene updates BEFORE touching the animations. Load() has already
    // destroyed the importer, and UpdateScene advances the ECS (including entity
    // garbage collection). Any imported entity that is no longer anchored would be
    // reclaimed here, while still-anchored ones (held by importedResources_) survive.
    // The first GetAnimations() call is intentionally deferred until after this loop
    // so it cannot itself anchor the animation entity and mask a regression.
    for (int i = 0; i < 5; ++i) {
        UpdateScene(META_NS::TimeSpan::Milliseconds(16));
    }

    auto anims = scene->GetAnimations().GetResult();
    ASSERT_FALSE(anims.empty());
    auto anim = anims[0];
    ASSERT_TRUE(anim);

    // The surviving animation must actually work: progress advances while running.
    interface_cast<META_NS::IStartableAnimation>(anim)->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());
    auto p1 = anim->Progress()->GetValue();
    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    auto p2 = anim->Progress()->GetValue();
    EXPECT_LT(p1, p2);
}

/**
 * @tc.name: ImportedResourcesRegisteredForScene
 * @tc.desc: Covers the createResources path of AssetObject::Load, which after the
 *           change feeds ResourcesCreator from importedResources_ (a GLTFResourceData)
 *           rather than from importer_->GetResult(). The imported glTF must still
 *           register image, material and animation resources against the scene's
 *           resource manager.
 * @tc.type: FUNC
 */
UNIT_TEST_F(AssetObjectLifetimeTest, ImportedResourcesRegisteredForScene, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    // AnimatedCube.gltf carries images, a material and a single animation.
    auto counts = CountSceneResources(scene);
    EXPECT_GT(counts.images, 0u);
    EXPECT_GT(counts.materials, 0u);
    EXPECT_GT(counts.animations, 0u);
}

/**
 * @tc.name: ImportedMaterialResourceResolvesToLiveObject
 * @tc.desc: A material resource registered from the imported glTF resolves through
 *           the resource manager to a live IMaterial after the importer is gone —
 *           i.e. importedResources_ keeps the underlying material entity valid, not
 *           merely its id in the registry.
 * @tc.type: FUNC
 */
UNIT_TEST_F(AssetObjectLifetimeTest, ImportedMaterialResourceResolvesToLiveObject, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    // Find the first registered material resource and resolve it.
    bool resolvedMaterial = false;
    for (auto&& info : rman->GetResourceInfos(interface_pointer_cast<CORE_NS::IInterface>(scene))) {
        if (info.type != ClassId::MaterialResource.Id().ToUid()) {
            continue;
        }
        auto res = rman->GetResource({info.id, interface_pointer_cast<CORE_NS::IInterface>(scene)});
        ASSERT_TRUE(res);
        auto material = interface_pointer_cast<IMaterial>(res);
        ASSERT_TRUE(material);
        resolvedMaterial = true;
        break;
    }
    EXPECT_TRUE(resolvedMaterial);
}

}  // namespace UTest
SCENE_END_NAMESPACE()
