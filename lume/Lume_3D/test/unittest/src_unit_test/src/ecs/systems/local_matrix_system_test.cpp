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

#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/transform_component.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

#include "ecs/systems/local_matrix_system.h"
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

/**
 * @tc.name: LocalMatrixSystemTest
 * @tc.desc: Tests for Local Matrix System Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsLocalMatrixSystem, LocalMatrixSystemTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto localMatrixManager = GetManager<ILocalMatrixComponentManager>(*ecs);
    ASSERT_NE(nullptr, localMatrixManager);
    auto localMatrixSystem = GetSystem<LocalMatrixSystem>(*ecs);
    ASSERT_NE(nullptr, localMatrixSystem);
    localMatrixSystem->SetActive(true);
    EXPECT_TRUE(localMatrixSystem->IsActive());
    EXPECT_EQ("LocalMatrixSystem", localMatrixSystem->GetName());
    EXPECT_EQ(LocalMatrixSystem::UID, ((ISystem*)localMatrixSystem)->GetUid());
    EXPECT_EQ(nullptr, localMatrixSystem->GetProperties());
    EXPECT_EQ(nullptr, ((const LocalMatrixSystem*)localMatrixSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &localMatrixSystem->GetECS());

    auto transformManager = GetManager<ITransformComponentManager>(*ecs);
    ASSERT_NE(nullptr, transformManager);

    Entity entity = ecs->GetEntityManager().Create();
    transformManager->Create(entity);
    localMatrixManager->Create(entity);
    Math::Vec3 position { 1.0f, 1.0f, 2.0f };
    Math::Quat rotation { Math::FromEulerRad(Math::Vec3 { 0.0f, 1.0f, 3.14f }) };
    Math::Vec3 scale { 1.0f, 2.0f, 0.5f };

    if (auto scopedHandle = transformManager->Write(entity); scopedHandle) {
        scopedHandle->position = position;
        scopedHandle->rotation = rotation;
        scopedHandle->scale = scale;
    }

    ecs->ProcessEvents();
    localMatrixSystem->Update(true, 1u, 1u);

    if (auto scopedHandle = localMatrixManager->Read(entity); scopedHandle) {
        Math::Mat4X4 expected = Math::Trs(position, rotation, scale);

        for (uint32_t i = 0; i < 4; ++i) {
            for (uint32_t j = 0; j < 4; ++j) {
                EXPECT_EQ(expected[i][j], scopedHandle->matrix[i][j]);
            }
        }
    }
}
