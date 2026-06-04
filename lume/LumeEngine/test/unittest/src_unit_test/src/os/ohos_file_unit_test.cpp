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

#include <memory>
#include <securec.h>

#include <gtest/gtest.h>

#include <base/containers/refcnt_ptr.h>
#include <core/namespace.h>

#include "os/ohos/ohos_file.h"
#include "test_framework.h"

using namespace CORE_NS;
using namespace BASE_NS;

// Test helper to create an OhosFile without a real ResourceManager
// We directly create OhosFileStorage and OhosFile to test their I/O methods

class OhosFileStorageTest : public ::testing::Test {
public:
    void SetUp() override
    {}
    void TearDown() override
    {}
};

/**
 * @tc.name: OhosFileStorage_InitialState001
 * @tc.desc: test OhosFileStorage initial size is 0
 * @tc.type: FUNC
 */
UNIT_TEST(OhosFileStorageTest, InitialState001, testing::ext::TestSize.Level1)
{
    OhosFileStorage storage(nullptr);
    EXPECT_EQ(storage.Size(), 0U);
    EXPECT_EQ(storage.GetStorage(), nullptr);
}

/**
 * @tc.name: OhosFileStorage_SetBuffer001
 * @tc.desc: test OhosFileStorage SetBuffer updates size and data
 * @tc.type: FUNC
 */
UNIT_TEST(OhosFileStorageTest, SetBuffer001, testing::ext::TestSize.Level1)
{
    OhosFileStorage storage(nullptr);

    auto buffer = std::make_unique<uint8_t[]>(16);
    EXPECT_EQ(memset_s(buffer.get(), 16, 0xAB, 16), EOK);

    storage.SetBuffer(std::move(buffer), 16);
    EXPECT_EQ(storage.Size(), 16U);
    ASSERT_NE(storage.GetStorage(), nullptr);
    EXPECT_EQ(storage.GetStorage()[0], 0xAB);
    EXPECT_EQ(storage.GetStorage()[15], 0xAB);
}

/**
 * @tc.name: OhosFileStorage_SetBufferTwice001
 * @tc.desc: test OhosFileStorage SetBuffer replaces previous buffer
 * @tc.type: FUNC
 */
UNIT_TEST(OhosFileStorageTest, SetBufferTwice001, testing::ext::TestSize.Level1)
{
    OhosFileStorage storage(nullptr);

    auto buffer1 = std::make_unique<uint8_t[]>(8);
    EXPECT_EQ(memset_s(buffer1.get(), 8, 0x11, 8), EOK);
    storage.SetBuffer(std::move(buffer1), 8);
    EXPECT_EQ(storage.Size(), 8U);

    auto buffer2 = std::make_unique<uint8_t[]>(32);
    EXPECT_EQ(memset_s(buffer2.get(), 32, 0x22, 32), EOK);
    storage.SetBuffer(std::move(buffer2), 32);
    EXPECT_EQ(storage.Size(), 32U);
    ASSERT_NE(storage.GetStorage(), nullptr);
    EXPECT_EQ(storage.GetStorage()[0], 0x22);
}

/**
 * @tc.name: OhosFileStorage_SetBufferEmpty001
 * @tc.desc: test OhosFileStorage SetBuffer with null buffer and zero size
 * @tc.type: FUNC
 */
UNIT_TEST(OhosFileStorageTest, SetBufferEmpty001, testing::ext::TestSize.Level1)
{
    OhosFileStorage storage(nullptr);

    storage.SetBuffer(nullptr, 0);
    EXPECT_EQ(storage.Size(), 0U);
    EXPECT_EQ(storage.GetStorage(), nullptr);
}

// OhosFile I/O tests using UpdateStorage to inject test data

class OhosFileIOTest : public ::testing::Test {
public:
    void SetUp() override
    {
        // Create a PlatformHapInfo with null resource manager (minimal setup)
        PlatformHapInfo hapInfo("", "", "", nullptr);
        resMgr = new OhosResMgr(hapInfo);
        file = new OhosFile(refcnt_ptr<OhosResMgr>(resMgr));
    }
    void TearDown() override
    {
        // Deleting file destroys its refcnt_ptr<OhosResMgr> member,
        // which calls Unref() on resMgr. When refcount drops to 0,
        // OhosResMgr deletes itself via "delete this".
        delete file;
        file = nullptr;
        resMgr = nullptr;
    }

    OhosResMgr* resMgr = nullptr;
    OhosFile* file = nullptr;
};

/**
 * @tc.name: OhosFile_GetMode001
 * @tc.desc: test OhosFile::GetMode returns READ_ONLY
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, GetMode001, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(file->GetMode(), IFile::Mode::READ_ONLY);
}

/**
 * @tc.name: OhosFile_WriteReturnsZero001
 * @tc.desc: test OhosFile::Write always returns 0 (read-only)
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, WriteReturnsZero001, testing::ext::TestSize.Level1)
{
    uint8_t buf[4] = {1, 2, 3, 4};
    EXPECT_EQ(file->Write(buf, 4), 0U);
}

/**
 * @tc.name: OhosFile_AppendReturnsZero001
 * @tc.desc: test OhosFile::Append always returns 0 (read-only)
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, AppendReturnsZero001, testing::ext::TestSize.Level1)
{
    uint8_t buf[4] = {1, 2, 3, 4};
    EXPECT_EQ(file->Append(buf, 4, 0), 0U);
}

/**
 * @tc.name: OhosFile_GetLengthInitial001
 * @tc.desc: test OhosFile::GetLength with no buffer loaded
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, GetLengthInitial001, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(file->GetLength(), 0U);
}

/**
 * @tc.name: OhosFile_GetPositionInitial001
 * @tc.desc: test OhosFile::GetPosition initially 0
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, GetPositionInitial001, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(file->GetPosition(), 0U);
}

/**
 * @tc.name: OhosFile_SeekNoBuffer001
 * @tc.desc: test OhosFile::Seek fails when no buffer loaded
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, SeekNoBuffer001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(file->Seek(0));
    EXPECT_FALSE(file->Seek(100));
}

/**
 * @tc.name: OhosFile_ReadNoBuffer001
 * @tc.desc: test OhosFile::Read returns 0 when no buffer loaded
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, ReadNoBuffer001, testing::ext::TestSize.Level1)
{
    uint8_t buf[16];
    EXPECT_EQ(file->Read(buf, 16), 0U);
}

/**
 * @tc.name: OhosFile_Close001
 * @tc.desc: test OhosFile::Close is no-op and doesn't crash
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, Close001, testing::ext::TestSize.Level1)
{
    file->Close();
    // No crash expected
}

/**
 * @tc.name: OhosFile_UpdateStorageAndRead001
 * @tc.desc: test OhosFile with injected storage can read data
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, UpdateStorageAndRead001, testing::ext::TestSize.Level1)
{
    // Create a storage with known data
    auto storage = std::make_shared<OhosFileStorage>(nullptr);
    auto buffer = std::make_unique<uint8_t[]>(8);
    buffer[0] = 'H';
    buffer[1] = 'e';
    buffer[2] = 'l';
    buffer[3] = 'l';
    buffer[4] = 'o';
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;
    storage->SetBuffer(std::move(buffer), 8);

    file->UpdateStorage(storage);

    EXPECT_EQ(file->GetLength(), 8U);
    EXPECT_EQ(file->GetPosition(), 0U);

    uint8_t readBuf[5] = {};
    auto bytesRead = file->Read(readBuf, 5);
    EXPECT_EQ(bytesRead, 5U);
    EXPECT_EQ(readBuf[0], 'H');
    EXPECT_EQ(readBuf[4], 'o');
    EXPECT_EQ(file->GetPosition(), 5U);
}

/**
 * @tc.name: OhosFile_SeekAndRead001
 * @tc.desc: test OhosFile Seek then Read from offset
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, SeekAndRead001, testing::ext::TestSize.Level1)
{
    auto storage = std::make_shared<OhosFileStorage>(nullptr);
    auto buffer = std::make_unique<uint8_t[]>(16);
    for (int i = 0; i < 16; i++) {
        buffer[i] = static_cast<uint8_t>(i);
    }
    storage->SetBuffer(std::move(buffer), 16);

    file->UpdateStorage(storage);

    EXPECT_TRUE(file->Seek(10));
    EXPECT_EQ(file->GetPosition(), 10U);

    uint8_t readBuf[4] = {};
    auto bytesRead = file->Read(readBuf, 4);
    EXPECT_EQ(bytesRead, 4U);
    EXPECT_EQ(readBuf[0], 10);
    EXPECT_EQ(readBuf[3], 13);
    EXPECT_EQ(file->GetPosition(), 14U);
}

/**
 * @tc.name: OhosFile_SeekBeyondEnd001
 * @tc.desc: test OhosFile::Seek fails when offset >= size
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, SeekBeyondEnd001, testing::ext::TestSize.Level1)
{
    auto storage = std::make_shared<OhosFileStorage>(nullptr);
    auto buffer = std::make_unique<uint8_t[]>(8);
    storage->SetBuffer(std::move(buffer), 8);

    file->UpdateStorage(storage);

    EXPECT_FALSE(file->Seek(8));         // exactly at end
    EXPECT_FALSE(file->Seek(100));       // beyond end
    EXPECT_EQ(file->GetPosition(), 0U);  // position unchanged
}

/**
 * @tc.name: OhosFile_ReadBeyondEnd001
 * @tc.desc: test OhosFile::Read when count exceeds buffer size
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, ReadBeyondEnd001, testing::ext::TestSize.Level1)
{
    auto storage = std::make_shared<OhosFileStorage>(nullptr);
    auto buffer = std::make_unique<uint8_t[]>(4);
    buffer[0] = 'A';
    buffer[1] = 'B';
    buffer[2] = 'C';
    buffer[3] = 'D';
    storage->SetBuffer(std::move(buffer), 4);

    file->UpdateStorage(storage);

    uint8_t readBuf[16] = {};
    auto bytesRead = file->Read(readBuf, 16);
    EXPECT_EQ(bytesRead, 4U);
    EXPECT_EQ(readBuf[0], 'A');
    EXPECT_EQ(readBuf[3], 'D');
}

/**
 * @tc.name: OhosFile_ReadZeroCount001
 * @tc.desc: test OhosFile::Read with count = 0
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, ReadZeroCount001, testing::ext::TestSize.Level1)
{
    auto storage = std::make_shared<OhosFileStorage>(nullptr);
    auto buffer = std::make_unique<uint8_t[]>(8);
    storage->SetBuffer(std::move(buffer), 8);

    file->UpdateStorage(storage);

    uint8_t readBuf[4] = {};
    auto bytesRead = file->Read(readBuf, 0);
    EXPECT_EQ(bytesRead, 0U);
    EXPECT_EQ(file->GetPosition(), 0U);
}

/**
 * @tc.name: OhosFile_SeekToBeginning001
 * @tc.desc: test OhosFile::Seek(0) succeeds
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFileIOTest, SeekToBeginning001, testing::ext::TestSize.Level1)
{
    auto storage = std::make_shared<OhosFileStorage>(nullptr);
    auto buffer = std::make_unique<uint8_t[]>(8);
    storage->SetBuffer(std::move(buffer), 8);

    file->UpdateStorage(storage);

    EXPECT_TRUE(file->Seek(5));
    EXPECT_EQ(file->GetPosition(), 5U);

    EXPECT_TRUE(file->Seek(0));
    EXPECT_EQ(file->GetPosition(), 0U);
}