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

#include <chrono>
#include <filesystem>
#include <thread>

#include <base/containers/fixed_string.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_monitor.h>
#include <core/io/intf_filesystem_api.h>
#include <core/log.h>
#include <core/os/intf_platform.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "io/dev/file_monitor.h"
#include "io/file_manager.h"
#include "io/memory_file.h"
#include "io/path_tools.h"
#include "io/std_directory.h"
#include "io/std_file.h"
#include "io/std_filesystem.h"
#include "log/logger_output.h"

using namespace CORE_NS;
using namespace BASE_NS;

/**
 * @tc.name: fileUriHandlingTests
 * @tc.desc: Tests for File Uri Handling Tests. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, fileUriHandlingTests, testing::ext::TestSize.Level1)
{
    // TODO: Fix test to run on OHOS ndk - copying the files to a temp dir.
    auto data = std::string("1234567890");

    auto factory = CORE_NS::GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    auto files = factory->CreateFilemanager();
    files->RegisterFilesystem("file", factory->CreateStdFileSystem());

    // Obtain abs path to assets to test file Uri
    std::filesystem::path appAssetsAbsPath =
        (std::filesystem::current_path() / std::filesystem::path(CORE_NS::UTest::GetTestEnv()->appAssetPath.c_str()))
            .lexically_normal();

    const string appAssetsDir(appAssetsAbsPath.generic_string().c_str());

    // no authority
    EXPECT_TRUE(files->OpenFile("file:" + appAssetsDir + "io/test_directory/file1.txt"));
    // empty authority
    EXPECT_TRUE(files->OpenFile("file:///" + appAssetsDir + "io/test_directory/file1.txt"));
    // empty authority but not path-absolute. this is the nonstandard syntax
    EXPECT_TRUE(files->OpenFile("file://" + appAssetsDir + "io/test_directory/file1.txt"));

    // can not test relative file uris, because the working directory is not known here.
    // also for windows:
    //  1. would we possibly like to add tests for different drives?
    //  2. and "current drive" relative paths.. (current drive/path/etc is saved during creation of filemanager, which
    //  currently handles the "file://" uri fixing)

    // Test asset register
    files->RegisterAssetPath("assets:/file1.txt");
    files->UnregisterAssetPath("assets:/file1.txt");

    files->RegisterAssetPath("/");
    files->UnregisterAssetPath("/");

    // Test proxy register
    files->RegisterPath("file", "", false);

    // Test empty Path
    files->RegisterPath("assets", "file:///", false);

    // If there's no D-drive the list of paths will be shorter
    const auto pathsCount = files->OpenDirectory("file:///D:") ? 5U : 3U;
    // Test Win32 case
    files->RegisterPath("assets", "file:///D:", false);
    files->RegisterPath("assets", "file:///", false);
    files->RegisterPath("assets", "file:///D:", false);

    // Test proxy fs..
    files->RegisterPath("assets", "file:///" + appAssetsDir + "io/test_directory/", false);
    ASSERT_TRUE(files->OpenFile("assets:/file1.txt") != nullptr);
    ASSERT_TRUE(files->OpenFile("assets:../assetReadTest.txt") == nullptr);

    // Test absolute path
    // files->RegisterPath("void", "assets://D:", false);
    const auto& constFm = static_cast<CORE_NS::FileManager*>(files.get());
    EXPECT_TRUE(constFm != nullptr);
    EXPECT_EQ(constFm->GetAbsolutePaths("assets:.././").size(), 0);
    EXPECT_EQ(constFm->GetAbsolutePaths("assets:./file1..txt").size(), 0);
    EXPECT_EQ(constFm->GetAbsolutePaths("assets:file1.txt").size(), 0);
    EXPECT_EQ(constFm->GetAbsolutePaths("assets:./").size(), pathsCount);
    // constFm->GetAbsolutePaths("void://file1.txt");

    // Test currently unsupported Rof (creation would always fail)
    const auto& rofPointer = constFm->CreateROFilesystem(nullptr, 0);
    EXPECT_TRUE(rofPointer);
    EXPECT_FALSE(rofPointer->CreateFile("rof1.txt"));
    EXPECT_FALSE(rofPointer->OpenFile("rof1.txt", IFile::Mode::READ_ONLY));
    EXPECT_FALSE(rofPointer->CreateFile("io/test_directory/rofIO.txt"));

    const auto& memFile1 = rofPointer->OpenFile("io/test_directory/rofIO.txt", IFile::Mode::READ_ONLY);
    // memFile1->GetMode();

    // memFile1->Write(data.c_str(), data.length());

    // length
    // auto length = memFile1->GetLength();
    // EXPECT_EQ(length, data.length());

    // Read
    // auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
    // auto read = memFile1->Read(buffer.get(), length);
    // EXPECT_EQ(read, length);

    // Verify
    // auto result = std::string((const char*)buffer.get(), static_cast<size_t>(length));
    // EXPECT_EQ(result, data);

    EXPECT_FALSE(rofPointer->DeleteFile("rof1.txt"));
    EXPECT_FALSE(rofPointer->CreateDirectory("./rof"));
    EXPECT_FALSE(rofPointer->OpenDirectory("./rof"));
    EXPECT_EQ(rofPointer->GetEntry("./").type, 0);
    EXPECT_EQ(rofPointer->GetEntry("//").name, "");
    EXPECT_FALSE(rofPointer->DeleteDirectory("./rof"));
    EXPECT_FALSE(rofPointer->Rename("foo", "bar"));
    EXPECT_EQ(rofPointer->GetUriPaths("foobar").size(), 0);

    // test sandboxed stdfilesystem.
    files->RegisterFilesystem(
        "test_files", factory->CreateStdFileSystem("file:" + appAssetsDir + "io/test_directory/"));

    ASSERT_TRUE(files->OpenFile("test_files:/file1.txt"));
    EXPECT_EQ(files->OpenFile("test_files:/file1.txt")->GetMode(), IFile::Mode::READ_ONLY);
    EXPECT_FALSE(StdDirectory::ResolveAbsolutePath(appAssetsDir + "io/test_directory", true).empty());
    EXPECT_FALSE(files->OpenFile("test_files:../assetReadTest.txt"));
    EXPECT_EQ(CORE_NS::StdDirectory::GetDirName(appAssetsDir + "io/test_directory"), appAssetsDir + "io");
    EXPECT_EQ(CORE_NS::StdDirectory::GetBaseName(appAssetsDir + "io/test_directory"), "test_directory");
    EXPECT_EQ(CORE_NS::StdDirectory::GetDirName("test_directory"), ".");
    EXPECT_EQ(CORE_NS::StdDirectory::GetBaseName("test_directory"), "test_directory");

    EXPECT_TRUE(files->OpenFile("test_files:file1.txt"));
    EXPECT_FALSE(files->OpenFile("test_files:../assetReadTest.txt"));

    const auto& stdSys = factory->CreateStdFileSystem("/");
    EXPECT_TRUE(stdSys);
    EXPECT_FALSE(stdSys->CreateFile("."));
    EXPECT_EQ(stdSys->GetUriPaths("").size(), 0);
#if WIN32
    EXPECT_TRUE(factory->CreateStdFileSystem("\\"));
#endif
    EXPECT_FALSE(factory->CreateStdFileSystem("void://"));

    EXPECT_FALSE(factory->CreateStdFileSystem("file:///D:"));
    EXPECT_TRUE(factory->CreateStdFileSystem("file:./"));
    // EXPECT_TRUE(factory->CreateStdFileSystem("../../../../../../boot.ini"));
}