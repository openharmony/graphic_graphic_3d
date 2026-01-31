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

#include <custom/shader_input_buffer.h>
#include <gtest/gtest.h>
#include <memory>
#include <cstring>

namespace OHOS::Render3D {

class ShaderInputBufferUT : public testing::Test {
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
 * @tc.name: ShaderInputBuffer_DefaultConstructor_InitialState
 * @tc.desc: Verify default constructor creates invalid buffer
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, ShaderInputBuffer_DefaultConstructor_InitialState, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    EXPECT_FALSE(buffer.IsValid());
    EXPECT_EQ(buffer.FloatSize(), 0u);
    EXPECT_EQ(buffer.ByteSize(), 0u);
    EXPECT_EQ(buffer.Map(1), nullptr);
}

/**
 * @tc.name: Alloc_ValidSize_Success
 * @tc.desc: Verify allocating buffer with valid size succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Alloc_ValidSize_Success, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    bool result = buffer.Alloc(100);

    EXPECT_TRUE(result);
    EXPECT_TRUE(buffer.IsValid());
    EXPECT_EQ(buffer.FloatSize(), 100u);
    EXPECT_EQ(buffer.ByteSize(), 100u * sizeof(float));
}

/**
 * @tc.name: Alloc_ExceedMaxSize_Failure
 * @tc.desc: Verify allocating buffer exceeding max size fails
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Alloc_ExceedMaxSize_Failure, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    bool result = buffer.Alloc(0x100001);

    EXPECT_FALSE(result);
    EXPECT_FALSE(buffer.IsValid());
    EXPECT_EQ(buffer.FloatSize(), 0u);
}

/**
 * @tc.name: Alloc_MaxSize_Success
 * @tc.desc: Verify allocating buffer with max size succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Alloc_MaxSize_Success, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    bool result = buffer.Alloc(0x100000);

    EXPECT_TRUE(result);
    EXPECT_TRUE(buffer.IsValid());
    EXPECT_EQ(buffer.FloatSize(), 0x100000u);
}

/**
 * @tc.name: Alloc_SameSize_NoReallocation
 * @tc.desc: Verify allocating same size again returns true without reallocation
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Alloc_SameSize_NoReallocation, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    buffer.Alloc(100);
    const float* originalPtr = buffer.Map(100);

    bool result = buffer.Alloc(100);
    const float* newPtr = buffer.Map(100);

    EXPECT_TRUE(result);
    EXPECT_EQ(originalPtr, newPtr);
}

/**
 * @tc.name: Alloc_DifferentSize_Reallocation
 * @tc.desc: Verify allocating different size causes reallocation
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Alloc_DifferentSize_Reallocation, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    buffer.Alloc(100);
    const float* originalPtr = buffer.Map(100);

    buffer.Alloc(200);
    const float* newPtr = buffer.Map(200);

    EXPECT_NE(originalPtr, newPtr);
    EXPECT_EQ(buffer.FloatSize(), 200u);
}

/**
 * @tc.name: Alloc_ZeroSize_Success
 * @tc.desc: Verify allocating zero size buffer succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Alloc_ZeroSize_Success, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    bool result = buffer.Alloc(0);

    EXPECT_TRUE(result);
    EXPECT_FALSE(buffer.IsValid());
}

/**
 * @tc.name: Map_ValidSize_ReturnsPointer
 * @tc.desc: Verify mapping with valid size returns buffer pointer
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Map_ValidSize_ReturnsPointer, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(100);

    const float* ptr = buffer.Map(100);

    EXPECT_NE(ptr, nullptr);
}

/**
 * @tc.name: Map_SizeExceedsBuffer_ReturnsNull
 * @tc.desc: Verify mapping with size exceeding buffer returns null
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Map_SizeExceedsBuffer_ReturnsNull, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(100);

    const float* ptr = buffer.Map(101);

    EXPECT_EQ(ptr, nullptr);
}

/**
 * @tc.name: Map_InvalidBuffer_ReturnsNull
 * @tc.desc: Verify mapping invalid buffer returns null
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Map_InvalidBuffer_ReturnsNull, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    const float* ptr = buffer.Map(10);

    EXPECT_EQ(ptr, nullptr);
}

/**
 * @tc.name: Map_ZeroSize_ReturnsPointer
 * @tc.desc: Verify mapping with zero size returns buffer pointer
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Map_ZeroSize_ReturnsPointer, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(100);

    const float* ptr = buffer.Map(0);

    EXPECT_NE(ptr, nullptr);
}

/**
 * @tc.name: FloatSize_AfterAlloc_ReturnsCorrectSize
 * @tc.desc: Verify FloatSize returns allocated size
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, FloatSize_AfterAlloc_ReturnsCorrectSize, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(256);

    EXPECT_EQ(buffer.FloatSize(), 256u);
}

/**
 * @tc.name: ByteSize_AfterAlloc_ReturnsCorrectSize
 * @tc.desc: Verify ByteSize returns correct byte size
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, ByteSize_AfterAlloc_ReturnsCorrectSize, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(100);

    EXPECT_EQ(buffer.ByteSize(), 100u * sizeof(float));
}

/**
 * @tc.name: Delete_ValidBuffer_ClearsBuffer
 * @tc.desc: Verify Delete clears buffer and resets state
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Delete_ValidBuffer_ClearsBuffer, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(100);

    buffer.Delete();

    EXPECT_FALSE(buffer.IsValid());
    EXPECT_EQ(buffer.FloatSize(), 0u);
    EXPECT_EQ(buffer.ByteSize(), 0u);
    EXPECT_EQ(buffer.Map(1), nullptr);
}

/**
 * @tc.name: Delete_InvalidBuffer_NoCrash
 * @tc.desc: Verify deleting invalid buffer doesn't crash
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Delete_InvalidBuffer_NoCrash, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    buffer.Delete();

    EXPECT_FALSE(buffer.IsValid());
}

/**
 * @tc.name: Delete_DoubleDelete_NoCrash
 * @tc.desc: Verify double delete doesn't crash
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Delete_DoubleDelete_NoCrash, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(100);

    buffer.Delete();
    buffer.Delete();

    EXPECT_FALSE(buffer.IsValid());
}

/**
 * @tc.name: Update_Array_ValidData_Success
 * @tc.desc: Verify updating buffer with array data succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_Array_ValidData_Success, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(5);

    float testData[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    buffer.Update(testData, 5);

    const float* ptr = buffer.Map(5);
    ASSERT_NE(ptr, nullptr);
    for (int i = 0; i < 5; i++) {
        EXPECT_FLOAT_EQ(ptr[i], testData[i]);
    }
}

/**
 * @tc.name: Update_Array_InvalidBuffer_Fails
 * @tc.desc: Verify updating invalid buffer fails safely
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_Array_InvalidBuffer_Fails, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    float testData[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    buffer.Update(testData, 5);

    EXPECT_FALSE(buffer.IsValid());
}

/**
 * @tc.name: Update_Array_ExceedsSize_Fails
 * @tc.desc: Verify updating with size exceeding buffer fails
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_Array_ExceedsSize_Fails, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(5);

    float testData[10] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    buffer.Update(testData, 10);

    const float* ptr = buffer.Map(10);
    EXPECT_EQ(ptr, nullptr);
}

/**
 * @tc.name: Update_SingleValue_ValidIndex_Success
 * @tc.desc: Verify updating single value at valid index succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_SingleValue_ValidIndex_Success, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(5);

    buffer.Update(10.5f, 2);

    const float* ptr = buffer.Map(5);
    ASSERT_NE(ptr, nullptr);
    EXPECT_FLOAT_EQ(ptr[2], 10.5f);
}

/**
 * @tc.name: Update_SingleValue_InvalidIndex_Fails
 * @tc.desc: Verify updating with invalid index fails safely
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_SingleValue_InvalidIndex_Fails, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(5);

    buffer.Update(10.5f, 5);

    const float* ptr = buffer.Map(5);
    ASSERT_NE(ptr, nullptr);
}

/**
 * @tc.name: Update_SingleValue_InvalidBuffer_Fails
 * @tc.desc: Verify updating single value on invalid buffer fails
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_SingleValue_InvalidBuffer_Fails, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    buffer.Update(10.5f, 0);

    EXPECT_FALSE(buffer.IsValid());
}

/**
 * @tc.name: Update_SingleValue_FirstIndex
 * @tc.desc: Verify updating first index works correctly
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_SingleValue_FirstIndex, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(10);

    buffer.Update(99.9f, 0);

    const float* ptr = buffer.Map(10);
    ASSERT_NE(ptr, nullptr);
    EXPECT_FLOAT_EQ(ptr[0], 99.9f);
}

/**
 * @tc.name: Update_SingleValue_LastIndex
 * @tc.desc: Verify updating last valid index works correctly
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Update_SingleValue_LastIndex, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(10);

    buffer.Update(88.8f, 9);

    const float* ptr = buffer.Map(10);
    ASSERT_NE(ptr, nullptr);
    EXPECT_FLOAT_EQ(ptr[9], 88.8f);
}

/**
 * @tc.name: IsValid_WithBuffer_ReturnsTrue
 * @tc.desc: Verify IsValid returns true for valid buffer
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, IsValid_WithBuffer_ReturnsTrue, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(10);

    EXPECT_TRUE(buffer.IsValid());
}

/**
 * @tc.name: IsValid_WithoutBuffer_ReturnsFalse
 * @tc.desc: Verify IsValid returns false without buffer
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, IsValid_WithoutBuffer_ReturnsFalse, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    EXPECT_FALSE(buffer.IsValid());
}

/**
 * @tc.name: IsValid_AfterDelete_ReturnsFalse
 * @tc.desc: Verify IsValid returns false after delete
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, IsValid_AfterDelete_ReturnsFalse, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(10);
    buffer.Delete();

    EXPECT_FALSE(buffer.IsValid());
}

/**
 * @tc.name: Destructor_AutoCleanup_NoMemoryLeak
 * @tc.desc: Verify destructor properly cleans up buffer
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, Destructor_AutoCleanup_NoMemoryLeak, testing::ext::TestSize.Level1)
{
    {
        ShaderInputBuffer buffer;
        buffer.Alloc(1000);
        EXPECT_TRUE(buffer.IsValid());
    }
    // Buffer destroyed here, should not leak
    // Note: Memory leak detection should be run separately with valgrind/sanitizers
    ShaderInputBuffer buffer2;
    buffer2.Alloc(500);
    EXPECT_TRUE(buffer2.IsValid());
}

/**
 * @tc.name: MultipleAlloc_Success
 * @tc.desc: Verify multiple allocations with different sizes work
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, MultipleAlloc_Success, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;

    buffer.Alloc(100);
    EXPECT_EQ(buffer.FloatSize(), 100u);

    buffer.Alloc(200);
    EXPECT_EQ(buffer.FloatSize(), 200u);

    buffer.Alloc(50);
    EXPECT_EQ(buffer.FloatSize(), 50u);
}

/**
 * @tc.name: UpdateAfterRealloc_Success
 * @tc.desc: Verify updating after reallocation works
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, UpdateAfterRealloc_Success, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(5);

    float testData1[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    buffer.Update(testData1, 5);

    buffer.Alloc(10);

    float testData2[10] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    buffer.Update(testData2, 10);

    const float* ptr = buffer.Map(10);
    ASSERT_NE(ptr, nullptr);
    EXPECT_FLOAT_EQ(ptr[9], 10.0f);
}

/**
 * @tc.name: ConstMap_ValidBuffer_ReturnsPointer
 * @tc.desc: Verify const Map works correctly
 * @tc.type: FUNC
 */
HWTEST_F(ShaderInputBufferUT, ConstMap_ValidBuffer_ReturnsPointer, testing::ext::TestSize.Level1)
{
    ShaderInputBuffer buffer;
    buffer.Alloc(100);

    const ShaderInputBuffer& constBuffer = buffer;
    const float* ptr = constBuffer.Map(100);

    EXPECT_NE(ptr, nullptr);
}

} // namespace OHOS::Render3D
