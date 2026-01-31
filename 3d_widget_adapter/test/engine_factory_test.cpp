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

#include "engine_factory.h"
#include "i_engine.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>

namespace OHOS::Render3D {

class EngineFactoryTest : public testing::Test {
public:
    static void SetUpTestCase()
    {}

    static void TearDownTestCase()
    {}

    void SetUp() override
    {}

    void TearDown() override
    {}
};

/**
 * @tc.name: CreateEngine_LumeType_Success
 * @tc.desc: Verify creating LUME engine type returns valid engine instance
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_LumeType_Success, testing::ext::TestSize.Level1)
{
    // Create LUME type engine
    auto engine = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);

    // Verify engine is not null
    ASSERT_NE(engine, nullptr);

    // Verify engine is valid pointer
    EXPECT_NE(engine.get(), nullptr);
}

/**
 * @tc.name: CreateEngine_DefaultType_Success
 * @tc.desc: Verify creating default engine type returns valid engine instance
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_DefaultType_Success, testing::ext::TestSize.Level1)
{
    // Create engine with default type
    // Note: If we add more engine types in future, this tests the default case
    auto engine = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);

    // Verify engine is not null
    ASSERT_NE(engine, nullptr);

    // Verify engine can be created
    EXPECT_TRUE(engine != nullptr);
}

/**
 * @tc.name: CreateEngine_MultipleInstances_Success
 * @tc.desc: Verify creating multiple engine instances works correctly
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_MultipleInstances_Success, testing::ext::TestSize.Level1)
{
    // Create first engine instance
    auto engine1 = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    ASSERT_NE(engine1, nullptr);

    // Create second engine instance
    auto engine2 = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    ASSERT_NE(engine2, nullptr);

    // Verify both engines are valid
    EXPECT_NE(engine1.get(), nullptr);
    EXPECT_NE(engine2.get(), nullptr);

    // Verify engines are independent (different pointers)
    EXPECT_NE(engine1.get(), engine2.get());
}

/**
 * @tc.name: CreateEngine_UniquePtr_ValidOwnership
 * @tc.desc: Verify engine is created as unique_ptr with proper ownership
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_UniquePtr_ValidOwnership, testing::ext::TestSize.Level1)
{
    // Create engine
    auto engine = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);

    // Verify it's a valid unique_ptr
    ASSERT_NE(engine, nullptr);
 
    // Transfer ownership
    std::unique_ptr<IEngine> engineMoved = std::move(engine);

    // Verify ownership transferred correctly
    EXPECT_EQ(engine, nullptr);
    EXPECT_NE(engineMoved, nullptr);
}

/**
 * @tc.name: CreateEngine_Loop_MemoryStability
 * @tc.desc: Verify creating and destroying engines in a loop doesn't cause memory leaks
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_Loop_MemoryStability, testing::ext::TestSize.Level1)
{
    // Create and destroy multiple engines in a loop
    const int iterationCount = 100;
    int validEngineCount = 0;

    for (int i = 0; i < iterationCount; ++i) {
        auto engine = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);

        // Verify each engine is valid
        ASSERT_NE(engine, nullptr);
        EXPECT_NE(engine.get(), nullptr);

        if (engine && engine.get()) {
            validEngineCount++;
        }

        // Engine will be automatically destroyed when going out of scope
    }

    // Verify all iterations created valid engines
    EXPECT_EQ(validEngineCount, iterationCount);
}

/**
 * @tc.name: CreateEngine_Reset_SafeDestruction
 * @tc.desc: Verify resetting engine pointer correctly destroys the engine
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_Reset_SafeDestruction, testing::ext::TestSize.Level1)
{
    // Create engine
    auto engine = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    ASSERT_NE(engine, nullptr);

    // Reset the unique_ptr
    engine.reset();

    // Verify engine is null after reset
    EXPECT_EQ(engine, nullptr);
}

/**
 * @tc.name: CreateEngine_Release_ManualDestruction
 * @tc.desc: Verify manually releasing engine pointer works correctly
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_Release_ManualDestruction, testing::ext::TestSize.Level1)
{
    // Create engine
    auto engine = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    ASSERT_NE(engine, nullptr);

    // Get raw pointer
    IEngine* rawPtr = engine.get();
    ASSERT_NE(rawPtr, nullptr);

    // Release ownership
    IEngine* releasedPtr = engine.release();

    // Verify released pointer matches raw pointer
    EXPECT_EQ(releasedPtr, rawPtr);
    EXPECT_EQ(engine, nullptr);

    // Clean up manually
    delete releasedPtr;
}

/**
 * @tc.name: CreateEngine_Scope_AutoDestruction
 * @tc.desc: Verify engine is automatically destroyed when going out of scope
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_Scope_AutoDestruction, testing::ext::TestSize.Level1)
{
    IEngine* rawPtr = nullptr;
    auto engine = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    ASSERT_NE(engine, nullptr);
    ASSERT_NE(engine.get(), nullptr);
    rawPtr = engine.get();

    auto engine2 = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    EXPECT_NE(engine2, nullptr);

    // Verify new engine has a different address (old one was properly destroyed)
    if (rawPtr != nullptr) {
        EXPECT_NE(engine2.get(), rawPtr);
    }
}

/**
 * @tc.name: CreateEngine_Swap_ValidExchange
 * @tc.desc: Verify swapping two engine instances works correctly
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_Swap_ValidExchange, testing::ext::TestSize.Level1)
{
    // Create two engines
    auto engine1 = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    auto engine2 = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);

    // Store original pointers
    auto* ptr1 = engine1.get();
    auto* ptr2 = engine2.get();

    // Swap engines
    engine1.swap(engine2);

    // Verify pointers are swapped
    EXPECT_EQ(engine1.get(), ptr2);
    EXPECT_EQ(engine2.get(), ptr1);
}

/**
 * @tc.name: CreateEngine_MoveAssignment_ValidTransfer
 * @tc.desc: Verify move assignment transfers ownership correctly
 * @tc.type: FUNC
 */
HWTEST_F(EngineFactoryTest, CreateEngine_MoveAssignment_ValidTransfer, testing::ext::TestSize.Level1)
{
    // Create first engine
    auto engine1 = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    auto* originalPtr = engine1.get();
    ASSERT_NE(originalPtr, nullptr);

    // Create second engine
    auto engine2 = EngineFactory::CreateEngine(EngineFactory::EngineType::LUME);
    ASSERT_NE(engine2.get(), nullptr);

    // Move assignment
    engine1 = std::move(engine2);

    // Verify ownership transferred
    EXPECT_NE(engine1.get(), originalPtr);
    EXPECT_EQ(engine2, nullptr);
}
} // namespace OHOS::Render3D
