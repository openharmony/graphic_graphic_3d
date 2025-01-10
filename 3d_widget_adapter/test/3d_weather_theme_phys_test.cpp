/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_3D_WEATHER_THEME_PHYS_TEST_H
#define OHOS_RENDER_3D_3D_WEATHER_THEME_PHYS_TEST_H

#include <gtest/gtest.h>
#include <memory>

#include "weather_theme_phys.h"

namespace OHOS::Render3D {
class WeatherThemePhys3DTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void WeatherThemePhys3DTest::SetUpTestCase()
{}

void WeatherThemePhys3DTest::TearDownTestCase()
{}

void WeatherThemePhys3DTest::SetUp()
{}

void WeatherThemePhys3DTest::TearDown()
{}


namespace {

/**
 * @tc.name: ProcessCollisions1
 * @tc.desc: Process collisions
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ProcessCollisions1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    weatherThemePhys.ProcessCollisions();
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: AddMotphysColliders1
 * @tc.desc: Add motphys colliders
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, AddMotphysColliders1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    CORE_NS::IEcs ecs;
    weatherThemePhys.AddMotphysColliders(ecs);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: DataValid1
 * @tc.desc: Data valid
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, DataValid1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    bool ret = weatherThemePhys.DataValid(nullptr);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: ApplyCollision1
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 10.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = 1000.;
    float height = 1000.;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision2
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision2, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 0.5;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = 1000;
    float height = 1000;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision3
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision3, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = -0.5;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = 1000;
    float height = 1000;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision4
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision4, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = -10.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = 1000;
    float height = 1000;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision5
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision5, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 100000.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = 1000;
    float height = 1000;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision6
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision6, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 10.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 100000;
    float width = 1000;
    float height = 1000;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision7
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision7, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 10.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 0;
    float width = 1000;
    float height = 1000;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision8
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision8, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 10.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = -1.;
    float height = -1.;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision9
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision9, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 10.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = 0.5;
    float height = 0.5;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ApplyCollision10
 * @tc.desc: Apply collision
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ApplyCollision10, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float collisionInfoNum = 10.;
    QuadInWorldInfo quadInfo;
    uint32_t numSeq = 10;
    float width = 100000.;
    float height = 100000.;
    weatherThemePhys.ApplyCollision(collisionInfoNum, quadInfo, numSeq, width, height);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ToID1
 * @tc.desc: To ID
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ToID1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    float num = 10.;
    uint32_t numSeq = 10;
    uint32_t ret = weatherThemePhys.ToID(num, numSeq);
    ASSERT_GE(ret, 0);
}

/**
 * @tc.name: CalcQuadInWorld1
 * @tc.desc: Calc quad in world
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, CalcQuadInWorld1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    BASE_NS::Math::Mat4X4& InverseProjection;
    QuadInWorldInfo ret = weatherThemePhys.CalcQuadInWorld(nullptr, InverseProjection);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: ScreenToWorld1
 * @tc.desc: Screen to world
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ScreenToWorld1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    BASE_NS::Math::Vec2 screen;
    BASE_NS::Math::Mat4X4& InverseProjection;
    BASE_NS::Math::Vec4 ret = weatherThemePhys.ScreenToWorld(screen, InverseProjection);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: AddCollisionInstance1
 * @tc.desc: Screen to world
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, AddCollisionInstance1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    uint32_t id = 1;
    uint32_t assesId = 1;
    BASE_NS::Math::Vec3 model_position;
    BASE_NS::Math::Vec3 scale;
    BASE_NS::Math::Vec3 voxelCount;
    weatherThemePhys.AddCollisionInstance(id, assesId, model_position, scale, voxelCount);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: AddCollisionInstance2
 * @tc.desc: Screen to world
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, AddCollisionInstance2, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys();
    uint32_t id = 0;
    uint32_t assesId = 1;
    BASE_NS::Math::Vec3 model_position;
    BASE_NS::Math::Vec3 scale;
    BASE_NS::Math::Vec3 voxelCount;
    weatherThemePhys.AddCollisionInstance(id, assesId, model_position, scale, voxelCount);
    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: LoadCollisionFiles
 * @tc.desc: Verify WeatherThemePhys WindowChange
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, LoadCollisionFiles, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    weatherThemePyhs.LoadCollisionFiles();

    ASSERT_NE(weatherThemePyhs, nullptr);
}

/**
 * @tc.name: LoadCollisionFiles
 * @tc.desc: Verify WidgetAdapter WindowChange
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, LoadCollisionFiles, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    weatherThemePyhs.LoadCollisionFiles();

    ASSERT_NE(weatherThemePyhs, nullptr);
}

/**
 * @tc.name: VariableSpeed
 * @tc.desc: Verify WeatherThemePhys VariableSpeed
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, VariableSpeed, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    float deltaTime = 0.0f;
    // float deltaTime
    weatherThemePyhs.VariableSpeed(deltaTime);

    ASSERT_EQ(deltaTime, false);
}

/**
 * @tc.name: UpdateThemeAndTime
 * @tc.desc: Verify WeatherThemePhys UpdateThemeAndTime
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, UpdateThemeAndTime, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    float* buffer = nullptr;
    uint32_t size = 0;
    // const float* buffer, uint32_t size
    weatherThemePyhs.UpdateThemeAndTime(buffer, size);

    ASSERT_EQ(buffer, nullptr);
}

/**
 * @tc.name: CreateShader
 * @tc.desc: Verify WeatherThemePhys CreateShader
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, CreateShader, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    CORE_NS::IEcs& ecs;
    const string_view uri;
    auto ret = weatherThemePyhs.CreateShader(ecs, uri);

    ASSERT_NE(weatherThemePyhs, nullptr);
}

/**
 * @tc.name: UpdateShaderInputBuffer
 * @tc.desc: Verify WeatherThemePhys CreateRainDropMaterial
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, CreateRainDropMaterial, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    CORE_NS::IEcs& ecs;
    auto ret = weatherThemePyhs.CreateRainDropMaterial(ecs);

    ASSERT_NE(weatherThemePyhs, nullptr);
}

/**
 * @tc.name: UpdateWeatherConfig
 * @tc.desc: Verify WeatherThemePhys UpdateWeatherConfig
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, UpdateWeatherConfig, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    float rain = 0.0f;
    float snow = 0.0f;
    float dayNight = 0.0f;
    RenderConfigTheme& config;
    weatherThemePyhs.UpdateWeatherConfig(rain, snow, dayNight, config);

    ASSERT_NE(weatherThemePyhs, nullptr);
}

/**
 * @tc.name: CreateMaterialEntity
 * @tc.desc: Verify WeatherThemePhys CreateMaterialEntity
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, CreateMaterialEntity, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    CORE_NS::IEcs& ecs;
    CORE3D_NS::IMaterialComponentManager* materialManager;
    CORE_NS::EntityReference shader;
    CORE_NS::EntityReference mainTexture;
    float brightness = 1;
    auto ret = weatherThemePyhs.CreateMaterialEntity(ecs, materialManager, shader, mainTexture, brightness);

    ASSERT_NE(ret, nullptr);
}

/**
 * @tc.name: CreatTextureEntity
 * @tc.desc: Verify WeatherThemePhys CreatTextureEntity
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, CreatTextureEntity, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    CORE_NS::IEcs& ecs;
    const string_view uri;
    auto ret = weatherThemePyhs.CreatTextureEntity(ecs, uri);

    ASSERT_NE(ret, nullptr);
}

/**
 * @tc.name: ReplaceRenderInstance
 * @tc.desc: Verify WeatherThemePhys ReplaceRenderInstance
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhys3DTest, ReplaceRenderInstance, testing::ext::TestSize.Level1)
{
    Render3D::WeatherThemePhys weatherThemePyhs();

    uint32_t id = 0;
    CORE_NS::Entity target;
    const BASE_NS::Math::Vec3& position = {0};

    weatherThemePyhs.ReplaceRenderInstance(id, target, position);

    ASSERT_EQ(id, 0);
}

/**
 * @tc.name: SetPosition2
 * @tc.desc: Set Position
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetPosition2, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    Entity entity = 200;
    const Math::Vec3& pos = {1, 100, 1000};
    weatherThemePhys.SetPosition(entity, pos);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: GetInverseProjection1
 * @tc.desc: Get Inverse Projection
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, GetInverseProjection1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    const auto& InverseProjection = weatherThemePhys.GetInverseProjection();

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: CreateRainScene1
 * @tc.desc: Create Rain Scene
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, CreateRainScene1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    weatherThemePhys.CreateRainScene(nullptr);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: Create Snow Scene1
 * @tc.desc: Create Snow Scene
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, CreateSnowScene1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    weatherThemePhys.CreateSnowScene(nullptr);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, UpdateParticles1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    weatherThemePhys.UpdateParticles();

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, UpdateParticles1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    weatherThemePhys.UpdateParticles();

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, ClearScene1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    weatherThemePhys.ClearScene();

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupCubeSpawner1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    CORE_NS::IEcs& ecs;
    CORE_NS::Entity entity;
    CORE_NS::EntityReference texture;
    CORE_NS::EntityReference shader;
    uint32_t spawnerIndex = 100;
    weatherThemePhys.SetupCubeSpawner(ecs, entity, texture, shader, spawnerIndex);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupCubeSpawner2, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    CORE_NS::IEcs& ecs;
    CORE_NS::Entity entity;
    CORE_NS::EntityReference texture;
    CORE_NS::EntityReference shader;
    uint32_t spawnerIndex = -1;
    weatherThemePhys.SetupCubeSpawner(ecs, entity, texture, shader, spawnerIndex);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupCubeSpawnerStepContext1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    CORE_NS::IEcs& ecs;
    CORE_NS::Entity entity;
    const SpawnerEffectConfig* spawnerEffectConfigs;
    uint32_t spawnerIndex = -1;
    weatherThemePhys.SetupCubeSpawnerStepContext(ecs, entity, spawnerIndex, spawnerEffectConfigs);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupCubeSpawnerStepContext2, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    CORE_NS::IEcs& ecs;
    CORE_NS::Entity entity;
    const SpawnerEffectConfig* spawnerEffectConfigs;
    uint32_t spawnerIndex = 100;
    weatherThemePhys.SetupCubeSpawnerStepContext(ecs, entity, spawnerIndex, spawnerEffectConfigs);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupSplashSpawner1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    CORE_NS::IEcs& ecs;
    CORE_NS::Entity entity;
    CORE_NS::EntityReference texture;
    CORE_NS::EntityReference shader;
    uint32_t spawnerIndex = 100;
    weatherThemePhys.SetupCubeSpawner(ecs, entity, texture, shader, spawnerIndex);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupSplashSpawner2, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    CORE_NS::IEcs& ecs;
    CORE_NS::Entity entity;
    CORE_NS::EntityReference texture;
    CORE_NS::EntityReference shader;
    uint32_t spawnerIndex = -1;
    weatherThemePhys.SetupCubeSpawner(ecs, entity, texture, shader, spawnerIndex);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupSplashSpawnerSettings1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    uint32_t spawnerIndex,
    CORE_NS::ScopedHandle<MotphysVfx::MotphysVfxEffectComponent>& rain;
    weatherThemePhys.SetupSplashSpawnerSettings(spawnerIndex, rain);

    ASSERT_NE(weatherThemePhys, nullptr);
}

/**
 * @tc.name: SetupSprinkleSpawner1
 * @tc.desc: Set up Sprinkle Spawner
 * @tc.type: FUNC
 * @tc.require: SR20240810872340
 */
HWTEST_F(WeatherThemePhysTest, SetupStickSpawner1, testing::ext::TestSize.Level1)
{
    WeatherThemePhys weatherThemePhys;
    CORE_NS::IEcs& ecs;
    CORE_NS::Entity entity;
    CORE_NS::EntityReference texture;
    CORE_NS::EntityReference shader;
    weatherThemePhys.SetupStickSpawner(ecs, entity, texture, shader);

    ASSERT_NE(weatherThemePhys, nullptr);
}

};
}  // namespace OHOS::Render3D
#endif  // OHOS_RENDER_3D_3D_WEATHER_THEME_PHYS_TEST_H