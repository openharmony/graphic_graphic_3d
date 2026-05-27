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

#include <gtest/gtest.h>

#include <base/containers/string_view.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

#include "os/ohos/ohos_filesystem.h"
#include "test_framework.h"

using namespace CORE_NS;
using namespace BASE_NS;

// OhosFilesystem requires a ResourceManager. Since we can't easily create one
// in unit tests, we test the readonly/read-only behaviors that don't need one.

class OhosFilesystemTest : public ::testing::Test {
public:
    void SetUp() override
    {
        // Create filesystem with null resource manager
        fs_ = std::make_unique<OhosFilesystem>("", "", "", nullptr);
    }
    void TearDown() override
    {
        fs_.reset();
    }
    std::unique_ptr<OhosFilesystem> fs_;
};

/**
 * @tc.name: CreateFileReturnsNull001
 * @tc.desc: test OhosFilesystem::CreateFile returns null (read-only fs)
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, CreateFileReturnsNull001, testing::ext::TestSize.Level1)
{
    auto file = fs_->CreateFile("test.txt");
    EXPECT_FALSE(file);
}

/**
 * @tc.name: DeleteFileReturnsFalse001
 * @tc.desc: test OhosFilesystem::DeleteFile returns false (read-only fs)
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, DeleteFileReturnsFalse001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->DeleteFile("test.txt"));
}

/**
 * @tc.name: CreateDirectoryReturnsNull001
 * @tc.desc: test OhosFilesystem::CreateDirectory returns null (read-only fs)
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, CreateDirectoryReturnsNull001, testing::ext::TestSize.Level1)
{
    auto dir = fs_->CreateDirectory("test_dir");
    EXPECT_FALSE(dir);
}

/**
 * @tc.name: DeleteDirectoryReturnsFalse001
 * @tc.desc: test OhosFilesystem::DeleteDirectory returns false (read-only fs)
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, DeleteDirectoryReturnsFalse001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->DeleteDirectory("test_dir"));
}

/**
 * @tc.name: RenameReturnsFalse001
 * @tc.desc: test OhosFilesystem::Rename returns false (read-only fs)
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, RenameReturnsFalse001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->Rename("old.txt", "new.txt"));
}

/**
 * @tc.name: GetUriPathsReturnsEmpty001
 * @tc.desc: test OhosFilesystem::GetUriPaths returns empty vector
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, GetUriPathsReturnsEmpty001, testing::ext::TestSize.Level1)
{
    auto paths = fs_->GetUriPaths("some://uri");
    EXPECT_TRUE(paths.empty());
}

/**
 * @tc.name: OpenFileWriteModeReturnsNull001
 * @tc.desc: test OhosFilesystem::OpenFile with WRITE mode returns null
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, OpenFileWriteModeReturnsNull001, testing::ext::TestSize.Level1)
{
    auto file = fs_->OpenFile("test.txt", IFile::Mode::READ_WRITE);
    EXPECT_FALSE(file);
}

/**
 * @tc.name: GetEntryEmptyPath001
 * @tc.desc: test OhosFilesystem::GetEntry with empty path returns unknown
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, GetEntryEmptyPath001, testing::ext::TestSize.Level1)
{
    auto entry = fs_->GetEntry("");
    EXPECT_EQ(entry.type, IDirectory::Entry::Type::UNKNOWN);
}

/**
 * @tc.name: FileExistsEmptyPath001
 * @tc.desc: test OhosFilesystem::FileExists with empty path returns false
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, FileExistsEmptyPath001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->FileExists(""));
}

/**
 * @tc.name: DirectoryExistsEmptyPath001
 * @tc.desc: test OhosFilesystem::DirectoryExists with empty path returns false
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, DirectoryExistsEmptyPath001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->DirectoryExists(""));
}

/**
 * @tc.name: OpenFileEmptyPath001
 * @tc.desc: test OhosFilesystem::OpenFile with empty path returns null
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, OpenFileEmptyPath001, testing::ext::TestSize.Level1)
{
    auto file = fs_->OpenFile("", IFile::Mode::READ_ONLY);
    EXPECT_FALSE(file);
}

/**
 * @tc.name: OpenDirectoryEmptyPath001
 * @tc.desc: test OhosFilesystem::OpenDirectory with empty path returns null
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, OpenDirectoryEmptyPath001, testing::ext::TestSize.Level1)
{
    auto dir = fs_->OpenDirectory("");
    EXPECT_FALSE(dir);
}

/**
 * @tc.name: GetEntryInvalidPath001
 * @tc.desc: test OhosFilesystem::GetEntry with non-existent path returns unknown
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, GetEntryInvalidPath001, testing::ext::TestSize.Level1)
{
    auto entry = fs_->GetEntry("nonexistent/path/file.txt");
    EXPECT_EQ(entry.type, IDirectory::Entry::Type::UNKNOWN);
}

/**
 * @tc.name: DeleteFileVariousPaths001
 * @tc.desc: test OhosFilesystem::DeleteFile always returns false regardless of path
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, DeleteFileVariousPaths001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->DeleteFile("a.txt"));
    EXPECT_FALSE(fs_->DeleteFile("/path/to/file.dat"));
    EXPECT_FALSE(fs_->DeleteFile(" "));
}

/**
 * @tc.name: RenameVariousPaths001
 * @tc.desc: test OhosFilesystem::Rename always returns false regardless of paths
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, RenameVariousPaths001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->Rename("a.txt", "b.txt"));
    EXPECT_FALSE(fs_->Rename("", ""));
    EXPECT_FALSE(fs_->Rename("/old", "/new"));
}

/**
 * @tc.name: GetUriPathsVarious001
 * @tc.desc: test OhosFilesystem::GetUriPaths always returns empty
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, GetUriPathsVarious001, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(fs_->GetUriPaths("").empty());
    EXPECT_TRUE(fs_->GetUriPaths("file:///test.txt").empty());
    EXPECT_TRUE(fs_->GetUriPaths("hap://bundle/module/res").empty());
}

/**
 * @tc.name: DeleteDirectoryVariousPaths001
 * @tc.desc: test OhosFilesystem::DeleteDirectory always returns false
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, DeleteDirectoryVariousPaths001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->DeleteDirectory("dir"));
    EXPECT_FALSE(fs_->DeleteDirectory("/a/b/c"));
    EXPECT_FALSE(fs_->DeleteDirectory(""));
}

/**
 * @tc.name: CreateFileVariousPaths001
 * @tc.desc: test OhosFilesystem::CreateFile always returns null
 * @tc.type: FUNC
 */
UNIT_TEST_F(OhosFilesystemTest, CreateFileVariousPaths001, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(fs_->CreateFile("new_file.txt"));
    EXPECT_FALSE(fs_->CreateFile(""));
    EXPECT_FALSE(fs_->CreateFile("/path/new.dat"));
}
