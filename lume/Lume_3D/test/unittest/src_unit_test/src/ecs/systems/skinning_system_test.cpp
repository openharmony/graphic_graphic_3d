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

#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/shaders/common/3d_dm_structures_common.h>
#include <core/ecs/intf_entity_manager.h>

#include "ecs/components/previous_joint_matrices_component.h"
#include "ecs/systems/skinning_system.h"
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
 * @tc.name: JointMatricesCountClamped
 * @tc.desc: A JointMatricesComponent.count larger than the fixed jointMatrices array capacity must not cause an
 *           out-of-bounds read of current->jointMatrices or write of prev->jointMatrices when SkinningSystem copies
 *           the matrices into the PreviousJointMatricesComponent.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsSkinningSystem, JointMatricesCountClamped, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto skinManager = GetManager<ISkinComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinManager);
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);
    auto jointMatricesManager = GetManager<IJointMatricesComponentManager>(*ecs);
    ASSERT_NE(nullptr, jointMatricesManager);
    auto prevJointMatricesManager = GetManager<IPreviousJointMatricesComponentManager>(*ecs);
    ASSERT_NE(nullptr, prevJointMatricesManager);

    auto skinningSystem = GetSystem<SkinningSystem>(*ecs);
    ASSERT_NE(nullptr, skinningSystem);
    skinningSystem->SetActive(true);

    // Entity must satisfy the SkinningSystem query: Skin (base) + SkinJoints + JointMatrices REQUIRED,
    // PreviousJointMatrices OPTIONAL. With the prev component present, Update() copies jointMatrices into it.
    Entity entity = ecs->GetEntityManager().Create();
    skinManager->Create(entity);
    skinJointsManager->Create(entity);
    jointMatricesManager->Create(entity);
    prevJointMatricesManager->Create(entity);

    // count exceeds the fixed CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT (256) array capacity. The property is public and
    // unbounded, so a corrupt/malicious asset can set this. The unclamped std::copy then walks past both arrays.
    constexpr size_t oversizedCount = CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT + 64U;
    if (auto handle = jointMatricesManager->Write(entity); handle) {
        handle->count = oversizedCount;
    }

    ecs->ProcessEvents();

    // Without the clamp this triggers a heap-buffer-overflow under ASan inside SkinningSystem.
    skinningSystem->Update(true, 1u, 1u);

    // With the clamp, the copied count is bounded by the array capacity.
    if (auto handle = prevJointMatricesManager->Read(entity); handle) {
        EXPECT_LE(handle->count, static_cast<size_t>(CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT));
    }
}
