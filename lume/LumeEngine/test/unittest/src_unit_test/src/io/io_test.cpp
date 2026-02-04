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
 * @tc.name: factoryTests
 * @tc.desc: Tests for Factory Tests. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, factoryTests, testing::ext::TestSize.Level1)
{
    // test the interface creations.
    // no such factory return null
    auto fileMan = CreateInstance<IFileManager>(Uid("ffffffff-ffff-ffff-ffff-ffffffffffff"), UID_FILE_MANAGER);
    EXPECT_FALSE(fileMan.get());

    // valid class but it's not a IFileManager.
    fileMan = CreateInstance<IFileManager>(UID_SYSTEM_GRAPH_LOADER, UID_FILE_MANAGER);
    EXPECT_FALSE(fileMan.get());

    // factory is valid, but no such interface from the class
    fileMan = CreateInstance<IFileManager>(UID_ENGINE_FACTORY, UID_FILE_MANAGER);
    EXPECT_FALSE(fileMan.get());

    // Use a specific FILESYSTEM API FACTORY, that can create other types also..
    fileMan = CreateInstance<IFileManager>(UID_FILESYSTEM_API_FACTORY, UID_FILE_MANAGER);
    EXPECT_TRUE(fileMan.get());

    // Use the global plugin registrys factory class, (via the UID_GLOBAL_FACTORY class/instance id)
    fileMan = CreateInstance<IFileManager>(UID_GLOBAL_FACTORY, UID_FILE_MANAGER);
    EXPECT_TRUE(fileMan.get());

    // Uses GetInterface to get IClassFactory from global registry
    fileMan = CreateInstance<IFileManager>(UID_FILE_MANAGER);
    EXPECT_TRUE(fileMan.get());

    // no such factory return null
    auto fileMon = CreateInstance<IFileMonitor>(Uid("ffffffff-ffff-ffff-ffff-ffffffffffff"), UID_FILE_MONITOR);
    EXPECT_FALSE(fileMon.get());

    // valid class but it's not a IFileMonitor.
    fileMon = CreateInstance<IFileMonitor>(UID_SYSTEM_GRAPH_LOADER, UID_FILE_MONITOR);
    EXPECT_FALSE(fileMon.get());

    // factory is valid, but no such interface from the class
    fileMon = CreateInstance<IFileMonitor>(UID_ENGINE_FACTORY, UID_FILE_MONITOR);
    EXPECT_FALSE(fileMon.get());

    // Use a specific FILESYSTEM API FACTORY, that can create other types also..
    fileMon = CreateInstance<IFileMonitor>(UID_FILESYSTEM_API_FACTORY, UID_FILE_MONITOR);
    EXPECT_TRUE(fileMon.get());

    // Use the global plugin registrys factory class, (via the UID_GLOBAL_FACTORY class/instance id)
    fileMon = CreateInstance<IFileMonitor>(UID_GLOBAL_FACTORY, UID_FILE_MONITOR);
    EXPECT_TRUE(fileMon.get());

    // Uses GetInterface to get IFileMonitor from global registry
    fileMon = CreateInstance<IFileMonitor>(UID_FILE_MONITOR);
    EXPECT_TRUE(fileMon.get());

    // try the factory directly
    auto factory = GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    EXPECT_TRUE(factory);

    auto inst = factory->CreateInstance(UID_FILE_MANAGER);
    EXPECT_TRUE(inst.get());
    auto man = inst->GetInterface(IFileManager::UID);
    EXPECT_TRUE(man);
    // this should return null, filemanager != filemonitor
    auto mon = inst->GetInterface(IFileMonitor::UID);
    EXPECT_FALSE(mon);

    const auto& constInst = *(inst);
    const auto mun = constInst.GetInterface(IFileManager::UID);
    EXPECT_TRUE(mun);

    fileMan = CreateInstance<IFileManager>(*factory, UID_FILE_MANAGER);
    EXPECT_TRUE(fileMan.get());

    fileMon = CreateInstance<IFileMonitor>(*factory, UID_FILE_MONITOR);
    EXPECT_TRUE(fileMon.get());
    const auto& conMon = *fileMon;
    EXPECT_FALSE(conMon.GetInterface(UID_FILE_MANAGER));
    EXPECT_FALSE(fileMon->GetInterface(UID_FILE_MANAGER));
    EXPECT_TRUE(conMon.GetInterface(IFileMonitor::UID));
    EXPECT_TRUE(fileMon->GetInterface(IFileMonitor::UID));

    fileMan = factory->CreateFilemanager();
    EXPECT_TRUE(fileMan.get());
    fileMon = factory->CreateFilemonitor(*fileMan);
    EXPECT_TRUE(fileMon.get());
}

/**
 * @tc.name: customTests
 * @tc.desc: Tests for Custom Tests. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, customTests, testing::ext::TestSize.Level1)
{
    /*
        This test
        1. creates a new filemanager.
        2. registers a custom filesystem
        3. delete/create/deletes a file from it.
    */

    class TempFile : public CORE_NS::IFile {
    public:
        Mode GetMode() const override
        {
            CORE_ASSERT(false);
            return IFile::Mode::READ_WRITE;
        }
        void Close() override
        {
            CORE_ASSERT(false);
        }
        uint64_t Read(void* buffer, uint64_t count) override
        {
            CORE_ASSERT(false);
            return count;
        }
        uint64_t Write(const void* buffer, uint64_t count) override
        {
            CORE_ASSERT(false);
            return count;
        }
        uint64_t Append(const void* buffer, uint64_t count, uint64_t /* chunkSize */) override
        {
            CORE_ASSERT(false);
            return count;
        }
        uint64_t GetLength() const override
        {
            CORE_ASSERT(false);
            return 0;
        }
        bool Seek(uint64_t offset) override
        {
            CORE_ASSERT(false);
            return true;
        }
        uint64_t GetPosition() const override
        {
            CORE_ASSERT(false);
            return 0;
        }
        void Destroy() override
        {
            delete this;
        }
    };
    class TempFS : public CORE_NS::IFilesystem {
        bool hasFile_ { false };

    public:
        IDirectory::Entry GetEntry(const string_view uri) override
        {
            CORE_ASSERT(false);
            return {};
        }
        IFile::Ptr OpenFile(const string_view path, IFile::Mode mode) override
        {
            CORE_ASSERT(path == "test_file01.dat");
            return IFile::Ptr { new TempFile() };
        }
        IFile::Ptr CreateFile(const string_view path) override
        {
            CORE_ASSERT(path == "test_file01.dat");
            hasFile_ = true;
            return IFile::Ptr { new TempFile() };
        }
        bool DeleteFile(const string_view path) override
        {
            CORE_ASSERT(path == "test_file01.dat");
            if (hasFile_) {
                hasFile_ = false;
                return true;
            }
            return false;
        }
        bool FileExists(const string_view path) const override
        {
            CORE_ASSERT(path == "test_file01.dat");
            return hasFile_;
        }
        IDirectory::Ptr OpenDirectory(const string_view path) override
        {
            CORE_ASSERT(false);
            return {};
        }
        IDirectory::Ptr CreateDirectory(const string_view path) override
        {
            CORE_ASSERT(false);
            return {};
        }
        bool DeleteDirectory(const string_view path) override
        {
            CORE_ASSERT(false);
            return {};
        }
        bool DirectoryExists(const string_view path) const override
        {
            CORE_ASSERT(false);
            return {};
        }
        bool Rename(const string_view fromPath, const string_view toPath) override
        {
            CORE_ASSERT(fromPath == "test_file01.dat");
            CORE_ASSERT(toPath == "test_file02.dat");
            return true;
        }
        vector<string> GetUriPaths(const string_view uri) const override
        {
            CORE_ASSERT(false);
            return {};
        };
        void Destroy() override
        {
            delete this;
        }
    };
    auto files =
        CreateInstance<IFileManager>(UID_FILE_MANAGER); // Uses GetInterface to get IClassFactory from global registry
    ASSERT_TRUE(files.get() != nullptr);

    EXPECT_FALSE(files->GetFilesystem("MyTemp"));
    EXPECT_FALSE(files->RegisterFilesystem("MyTemp", nullptr));
    EXPECT_TRUE(files->RegisterFilesystem("MyTemp", IFilesystem::Ptr(new TempFS())));
    EXPECT_FALSE(files->RegisterFilesystem("MyTemp", IFilesystem::Ptr(new TempFS())));
    EXPECT_TRUE(files->GetFilesystem("MyTemp"));

    auto filename = "MyTemp://test_file01.dat";

    // Make sure there is no existing file.
    files->DeleteFile(filename);

    {
        // Try to create a new file.
        auto newFile = files->CreateFile(filename);
        ASSERT_TRUE(newFile != nullptr);
    }

    {
        // Try to open newly created file.
        auto file = files->OpenFile(filename);
        ASSERT_TRUE(file != nullptr);
    }

    // Delete the file.
    bool deletionResult = files->DeleteFile(filename);
    ASSERT_EQ(deletionResult, true);

    files->UnregisterFilesystem("MyTemp");
}

/**
 * @tc.name: memoryFileSysTests
 * @tc.desc: Tests for Memory File Sys Tests. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, memoryFileSysTests, testing::ext::TestSize.Level1)
{
    auto data = std::string("1234567890");
    auto factory = CORE_NS::GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    auto files = factory->CreateFilemanager();
    const auto& memP = factory->CreateMemFileSystem();
    ASSERT_TRUE(memP != nullptr);
    ASSERT_TRUE(memP->CreateFile("io/test_directory/1.txt") != nullptr);
    ASSERT_TRUE(memP->CreateFile("io/test_directory/2.txt") != nullptr);
    ASSERT_TRUE(memP->CreateFile("io/test_directory/2.txt") != nullptr);
    ASSERT_FALSE(memP->OpenFile("io/test_directory/2.txt", CORE_NS::IFile::Mode::READ_ONLY) != nullptr);
    const auto& memFile1 = memP->CreateFile("io/test_directory/memIO.txt");
    ASSERT_TRUE(memFile1 != nullptr);
    ASSERT_EQ(memFile1->GetMode(), IFile::Mode::READ_WRITE); // NOTE: writing even its read only?
    EXPECT_EQ(memFile1->Write(data.c_str(), data.length()), data.length());
    // length
    auto length = memFile1->GetLength();
    EXPECT_EQ(length, data.length());

    // Read
    auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
    auto read = memFile1->Read(buffer.get(), length);
    EXPECT_EQ(read, length);
    auto overRead = memFile1->Read(buffer.get(), length + 1);
    EXPECT_EQ(overRead, 0);

    // Verify
    auto result = std::string((const char*)buffer.get(), static_cast<size_t>(length));
    EXPECT_EQ(result, data);

    // Then test appending
    EXPECT_EQ(memFile1->Append("abcd", 4, 4), 4);
    EXPECT_EQ(memFile1->GetLength(), data.length() + 4);
    auto buffer2 = std::make_unique<uint8_t[]>(static_cast<size_t>(length + 4));
    memFile1->Seek(0);
    memFile1->Read(buffer2.get(), length + 4);

    // And verify append also
    auto result2 = std::string((const char*)buffer2.get(), static_cast<size_t>(length + 4));
    EXPECT_EQ(result2, data + "abcd");

    EXPECT_TRUE(memFile1->Seek(0));
    EXPECT_FALSE(memFile1->Seek(memFile1->GetLength() + 1));
    EXPECT_EQ(memFile1->GetPosition(), 0);
    memFile1->Close();

    IDirectory::Entry emtEnt {};
    EXPECT_NE(memP->GetEntry("io/test_directory/1.txt").name, emtEnt.name);
    EXPECT_TRUE(memP->DeleteFile("io/test_directory/1.txt"));
    EXPECT_TRUE(memP->DeleteFile("io/test_directory/memIO.txt"));
    EXPECT_EQ(memP->GetEntry("").name, emtEnt.name);
    EXPECT_FALSE(memP->Rename("", ""));
    EXPECT_EQ(memP->GetUriPaths("").size(), 0);
    EXPECT_FALSE(memP->CreateDirectory("io/test_directory") != nullptr);
    EXPECT_FALSE(memP->OpenDirectory("io/test_directory") != nullptr);
    EXPECT_FALSE(memP->DeleteDirectory("io/test_directory"));
}

extern "C" const uint64_t SIZEOFDATAFORTEST;
extern "C" const void* const BINARYDATAFORTEST[];

/**
 * @tc.name: rofSysTest
 * @tc.desc: Tests for Rof Sys Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, rofSysTest, testing::ext::TestSize.Level1)
{
    auto data = std::string("1234567890");
    struct test_entry {
        const char fname[256];
        const uint64_t offset;
        const uint64_t size;
    };
    auto factory = CORE_NS::GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    // rof system test
    test_entry testE1 { "/test1", 0, 50 };
    test_entry testE2 { "test2/", 50, 50 };
    test_entry testE3 { "./test3", 0, 50 };
    test_entry tests[3] = { testE1, testE2, testE3 };
    auto rofSys = factory->CreateROFilesystem(tests, countof(tests));
    auto ent1 = rofSys->GetEntry("/test1");
    auto ent2 = rofSys->GetEntry("test2");
    auto ent3 = rofSys->GetEntry("./test3");
    EXPECT_EQ(ent1.name, "");
    EXPECT_EQ(ent1.type, IDirectory::Entry::Type::UNKNOWN);
    EXPECT_EQ(ent2.name, "test2");
    EXPECT_EQ(ent2.type, IDirectory::Entry::Type::DIRECTORY);
    EXPECT_EQ(ent3.name, "./test3");
    EXPECT_EQ(ent3.type, IDirectory::Entry::Type::FILE);

    EXPECT_TRUE(rofSys->DirectoryExists("test2"));
    auto dir1 = rofSys->OpenDirectory("test2");
    ASSERT_TRUE(dir1);
    auto entries = dir1->GetEntries();
    EXPECT_EQ(entries.size(), 1);
    dir1->Close();

    EXPECT_TRUE(rofSys->FileExists("./test3"));
    auto roF1 = rofSys->OpenFile("./test3", IFile::Mode::READ_ONLY);
    ASSERT_TRUE(roF1);
    EXPECT_EQ(roF1->GetMode(), IFile::Mode::READ_ONLY);
    EXPECT_EQ(roF1->GetLength(), 50);
    EXPECT_EQ(roF1->Seek(5), true);
    EXPECT_EQ(roF1->GetPosition(), 5);
    EXPECT_EQ(roF1->Seek(55), false);

    int length = roF1->GetLength();
    auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
    EXPECT_NE(roF1->Read(buffer.get(), length), 0);
    EXPECT_EQ(roF1->Read(buffer.get(), length), 0);
    EXPECT_EQ(roF1->Write(data.c_str(), data.length()), 0);
    EXPECT_EQ(roF1->Append("abcd", 4, 4), 0);
    roF1->Close();

    EXPECT_FALSE(rofSys->OpenFile("./test3", IFile::Mode::READ_WRITE));
    {
        auto& fileManager = CORE_NS::UTest::GetTestEnv()->fileManager;
        auto rofs = fileManager->CreateROFilesystem(BINARYDATAFORTEST, static_cast<size_t>(SIZEOFDATAFORTEST));
        EXPECT_TRUE(rofs->DirectoryExists("test_data"));
        EXPECT_TRUE(rofs->FileExists("test_data/systemGraph.json"));
        fileManager->RegisterFilesystem("testrofs", move(rofs));
        // empty authority
        EXPECT_TRUE(fileManager->OpenDirectory("testrofs:///test_data/"));
        EXPECT_TRUE(fileManager->OpenFile("testrofs:///test_data/systemGraph.json"));
        // no authority
        EXPECT_TRUE(fileManager->OpenDirectory("testrofs:/test_data/"));
        EXPECT_TRUE(fileManager->OpenFile("testrofs:/test_data/systemGraph.json"));
        fileManager->UnregisterFilesystem("testrofs");
    }
}

/**
 * @tc.name: fileCreationAndDeletion
 * @tc.desc: Tests for File Creation And Deletion. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, fileCreationAndDeletion, testing::ext::TestSize.Level1)
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    auto filename1 = "cache://test_file01.dat";
    auto filename2 = "";
    auto filename3 = "unknown://false";
    // UTF-8 string
    auto filename4 = "cache://\xf0\x9d\x95\xb7\xf0\x9d\x96\x9a\xf0\x9d\x96\x92\xf0\x9d\x96\x8a.dat";

    // Make sure there is no existing file.
    EXPECT_FALSE(files->DeleteFile(filename1));
    EXPECT_FALSE(files->DeleteFile(filename2));
    EXPECT_FALSE(files->DeleteFile(filename3));
    EXPECT_FALSE(files->DeleteFile(filename4));

    {
        EXPECT_FALSE(files->FileExists(filename1));
        // Try to create a new file.
        auto newFile = files->CreateFile(filename1);
        EXPECT_TRUE(newFile);
        EXPECT_TRUE(files->FileExists(filename1));
        // EXPECT_FALSE
        auto newFile2 = files->CreateFile(filename2);
        EXPECT_FALSE(newFile2);
        EXPECT_FALSE(files->FileExists(filename2));
        auto newFile3 = files->CreateFile(filename3);
        EXPECT_FALSE(newFile3);
        EXPECT_FALSE(files->FileExists(filename3));
        auto newFile4 = files->CreateFile(filename4);
        EXPECT_TRUE(newFile4);
        EXPECT_TRUE(files->FileExists(filename4));
    }

    {
        // Try to open newly created file.
        auto file1 = files->OpenFile(filename1);
        ASSERT_TRUE(file1);
        auto file4 = files->OpenFile(filename4);
        ASSERT_TRUE(file4);
    }
    {
        // Delete the file.
        bool deletionResult = files->DeleteFile(filename1);
        EXPECT_TRUE(deletionResult);
        EXPECT_FALSE(files->FileExists(filename1));
        bool deletionResult2 = files->DeleteFile(filename2);
        EXPECT_FALSE(deletionResult2);
        bool deletionResult3 = files->DeleteFile(filename3);
        EXPECT_FALSE(deletionResult2);
        bool deletionResult4 = files->DeleteFile(filename4);
        EXPECT_TRUE(deletionResult4);
        EXPECT_FALSE(files->FileExists(filename4));
    }
}

/**
 * @tc.name: fileReadAndWrite
 * @tc.desc: Tests for File Read And Write. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, fileReadAndWrite, testing::ext::TestSize.Level1)
{
    auto data = std::string("1234567890");

    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);
    auto filename = "cache://test_file02.dat";

    {
        // Create file.
        auto file = files->CreateFile(filename);
        ASSERT_TRUE(file != nullptr);
        ASSERT_EQ(file->GetPosition(), 0);

        // Write data.
        auto written = file->Write(data.c_str(), data.length());
        ASSERT_EQ(written, data.length());
        ASSERT_EQ(file->GetPosition(), data.length());

        // Close file, not necessary but do it anyway.
        file->Close();
    }

    {
        // Open file, read all at once.
        auto file = files->OpenFile(filename);
        ASSERT_TRUE(file != nullptr);

        // Ensure length.
        auto length = file->GetLength();
        ASSERT_EQ(length, data.length());

        // Read all.
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
        auto read = file->Read(buffer.get(), length);
        ASSERT_EQ(read, length);

        // Verify result.
        auto result = std::string((const char*)buffer.get(), static_cast<size_t>(length));
        ASSERT_EQ(result, data);
    }

    {
        // Open file, read all in parts.
        auto file = files->OpenFile(filename);
        ASSERT_TRUE(file != nullptr);

        int offset = 0;

        // Ensure length.
        auto length = file->GetLength();
        ASSERT_EQ(length, data.length());

        std::vector<uint8_t> in;
        in.resize(static_cast<size_t>(length));

        // Read in chunks of 2 bytes.
        while (file->Read(&in.data()[offset], 2)) {
            offset += 2;
        }

        // Verify result.
        auto result = std::string_view((const char*)in.data(), in.size());
        ASSERT_EQ(result, data);
        ASSERT_EQ(file->GetPosition(), in.size());
    }

    {
        // file content append test
        // take some file
        auto file = files->OpenFile(filename);
        auto len = file->GetLength();
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(len));
        file->Read(buffer.get(), len);
        file->Close();
        auto filename2 = "cache://newFile.dat";
        auto newFile = files->CreateFile(filename2);
        // new file for testing from the 'filename'
        newFile->Write(buffer.get(), len);
        newFile->Close();
        // the actual testing
        auto openedFile = files->OpenFile(filename2, IFile::Mode::READ_WRITE);
        const char* newBuf = "abcd";
        openedFile->Append(newBuf, 4, 1);
        auto len2 = openedFile->GetLength();
        EXPECT_EQ(len2, len + 4);
        openedFile->Append(newBuf, 4, 3);
        len2 = openedFile->GetLength();
        EXPECT_EQ(len2, len + 8);
        // zero flush size
        openedFile->Append(newBuf, 4, 0);
        len2 = openedFile->GetLength();
        EXPECT_EQ(len2, len + 12);
        openedFile->Close();

        auto appendedFile = files->OpenFile(filename2);
        auto buffer2 = std::make_unique<uint8_t[]>(static_cast<size_t>(len2));

        appendedFile->Read(buffer2.get(), len2);
        auto result = std::string((const char*)buffer2.get(), static_cast<size_t>(len2));
        auto appends = result.substr(10, 4);
        EXPECT_STREQ(appends.c_str(), "abcd");
        auto appends2 = result.substr(14, 4);
        EXPECT_STREQ(appends2.c_str(), "abcd");
        appendedFile->Close();

        ASSERT_TRUE(files->DeleteFile(filename2));
    }

    // Delete the file
    ASSERT_TRUE(files->DeleteFile(filename));
}

/**
 * @tc.name: directoryCreationAndDeletion
 * @tc.desc: Tests for Directory Creation And Deletion. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, directoryCreationAndDeletion, testing::ext::TestSize.Level1)
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    auto directoryName = "app:///test_directory";
    auto directoryUtf8Name =
        "app:///"
        "\xf0\x9d\x96\x99\xf0\x9d\x96\x8a\xf0\x9d\x96\x98\xf0\x9d\x96\x99\x5f\xf0\x9d\x96\x89\xf0\x9d\x96\x8e\xf0\x9d"
        "\x96\x97\xf0\x9d\x96\x8a\xf0\x9d\x96\x88\xf0\x9d\x96\x99\xf0\x9d\x96\x94\xf0\x9d\x96\x97\xf0\x9d\x96\x9e";

    // Remove directory if it exists.
    files->DeleteDirectory(directoryName);
    files->DeleteDirectory(directoryUtf8Name);

    {
        auto ent = files->GetEntry("cache:/../");
        EXPECT_EQ(ent.name, "");
        auto file = files->CreateFile("cache:../");
        EXPECT_FALSE(file);
        auto dir = files->CreateDirectory("cache:/../");
        EXPECT_FALSE(dir);
        EXPECT_FALSE(files->Rename("cache://", "cache://"));
    }

    {
        // See that there is no such directory present.
        ASSERT_FALSE(files->OpenDirectory(directoryName));
        ASSERT_FALSE(files->OpenDirectory(directoryUtf8Name));
    }

    {
        // delibrate attempts on creating/opening dir errors
        auto directory = files->CreateDirectory("void");
        EXPECT_FALSE(directory);
        directory = files->CreateDirectory("nullProc://void");
        EXPECT_FALSE(directory);
        directory = files->OpenDirectory("void");
        EXPECT_FALSE(directory);
        auto entry = files->GetEntry("void");
        EXPECT_EQ(entry.name, "");
        entry = files->GetEntry("nullProc://void");
        EXPECT_EQ(entry.name, "");
    }

    {
        // Create such directory.
        EXPECT_FALSE(files->DirectoryExists(directoryName));
        EXPECT_TRUE(files->CreateDirectory(directoryName));
        EXPECT_TRUE(files->DirectoryExists(directoryName));
        EXPECT_FALSE(files->DirectoryExists(directoryUtf8Name));
        EXPECT_TRUE(files->CreateDirectory(directoryUtf8Name));
        EXPECT_TRUE(files->DirectoryExists(directoryUtf8Name));
    }
    {
        // Delete the file.
        bool deletionResult = files->DeleteDirectory(directoryName);
        EXPECT_TRUE(deletionResult);
        EXPECT_FALSE(files->DirectoryExists(directoryName));

        deletionResult = files->DeleteDirectory(directoryUtf8Name);
        EXPECT_TRUE(deletionResult);
        EXPECT_FALSE(files->DirectoryExists(directoryUtf8Name));

        deletionResult = files->DeleteDirectory("void");
        EXPECT_FALSE(deletionResult);
    }
}

bool recursivelyDeleteDirectory(IFileManager* files, string_view path)
{
    // Remove directory if it exists.
    auto directory = files->OpenDirectory(path);
    if (directory) {
        // Remove all files.
        auto entries = directory->GetEntries();
        for (auto filename : entries) {
            if (filename.type == IDirectory::Entry::FILE) {
                bool deletionResult = files->DeleteFile(path + '/' + filename.name);
                if (!deletionResult) {
                    return false;
                }
            } else if (filename.type == IDirectory::Entry::DIRECTORY) {
                bool deletionResult = recursivelyDeleteDirectory(files, path + '/' + filename.name);
                if (!deletionResult) {
                    return false;
                }
            }
        }

        // Remove directory.
        if (!files->DeleteDirectory(path)) {
            return false;
        }
    }

    return true;
}

/**
 * @tc.name: fileRename
 * @tc.desc: Tests for File Rename. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, fileRename, testing::ext::TestSize.Level1)
{
    auto data = std::string("1234567890");

    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files);
    auto fromFilename = "cache:/test_file03.dat";
    auto toFilename = "cache:/test_file04.dat";
    auto differentProto = "app:/test_file04.dat";
    auto toUtf8Filename = "cache:/"
                          "\xf0\x9d\x96\x99\xf0\x9d\x96\x8a\xf0\x9d\x96\x98\xf0\x9d\x96\x99\x5f\xf0\x9d\x96\x8b\xf0"
                          "\x9d\x96\x8e\xf0\x9d\x96\x91\xf0\x9d\x96\x8a\x30\x35.dat";

    // Make sure none of the files exist e.g. due to an interrupted test run.
    files->DeleteFile(fromFilename);
    files->DeleteFile(toFilename);
    files->DeleteFile(toUtf8Filename);

    {
        // Create file.
        auto file = files->CreateFile(fromFilename);
        ASSERT_TRUE(file);
        EXPECT_EQ(file->GetPosition(), 0);

        // Write data.
        auto written = file->Write(data.c_str(), data.length());
        EXPECT_EQ(written, data.length());
        EXPECT_EQ(file->GetPosition(), data.length());

        // Close file, not necessary but do it anyway.
        file->Close();
    }

    // Rename file to different protocol, should fail
    ASSERT_FALSE(files->Rename(fromFilename, differentProto));

    // Rename file with same protocol, should pass
    ASSERT_TRUE(files->Rename(fromFilename, toFilename));

    {
        // Open file, read all at once.
        auto file = files->OpenFile(toFilename);
        ASSERT_TRUE(file);

        // Ensure length.
        auto length = file->GetLength();
        ASSERT_EQ(length, data.length());

        // Read all.
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
        auto read = file->Read(buffer.get(), length);
        EXPECT_EQ(read, length);

        // Verify result.
        auto result = std::string_view((const char*)buffer.get(), static_cast<size_t>(length));
        EXPECT_EQ(result, data);
    }
    {
        EXPECT_TRUE(files->Rename(toFilename, toUtf8Filename));
        auto file = files->OpenFile(toUtf8Filename);
        ASSERT_TRUE(file);

        // Ensure length.
        auto length = file->GetLength();
        EXPECT_EQ(length, data.length());

        file->Close();

        EXPECT_TRUE(files->Rename(toUtf8Filename, toFilename));
    }

    // Delete the file
    EXPECT_TRUE(files->DeleteFile(toFilename));
}

/**
 * @tc.name: directoryFilelist
 * @tc.desc: Tests for Directory Filelist. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, directoryFilelist, testing::ext::TestSize.Level1)
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    constexpr const string_view directoryUri = "app:///test_directory_filelist";

    ASSERT_EQ(recursivelyDeleteDirectory(files.get(), directoryUri), true);

    {
        // Create test directory and add 10 files.
        auto directory = files->CreateDirectory(directoryUri);
        ASSERT_NE(directory, nullptr);

        for (int i = 0; i < 10; ++i) {
            auto file = files->CreateFile(directoryUri + '/' + "file" + to_string(i));
            ASSERT_TRUE(file);
            file->Write("12345", 5);
        }
    }

    EXPECT_TRUE(files->CreateDirectory(directoryUri + '/' + "dir1") != nullptr);
    EXPECT_TRUE(files->CreateDirectory(directoryUri + '/' + "dir2") != nullptr);
    EXPECT_TRUE(files->CreateDirectory(directoryUri + '/' + "dir3") != nullptr);

    // Ensure that we are able to read back what was created above.
    auto directory = files->OpenDirectory(directoryUri);
    if (directory) {
        // Check twice, to make sure the directory pointer is reset on every call.
        for (int i = 0; i < 2; ++i) {
            // There should be 10 files and 3 dirs.
            auto entries = directory->GetEntries();
            ASSERT_EQ(entries.size(), 13);
        }
    }

    // Delete the test data.
    ASSERT_EQ(recursivelyDeleteDirectory(files.get(), directoryUri), true);
}

#ifdef DISABLED_TESTS_ON
#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: fileMonitorTest
 * @tc.desc: Tests for File Monitor Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, DISABLED_fileMonitorTest, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: fileMonitorTest
 * @tc.desc: Tests for File Monitor Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, fileMonitorTest, testing::ext::TestSize.Level1)
#endif
{
    auto& fileManager = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(fileManager != nullptr);

    // Application root for test.
    constexpr const string_view testRootUri = "cache:///test_directory_monitor";

    // Clean up.
    recursivelyDeleteDirectory(fileManager.get(), testRootUri);

    // Create root directory.
    ASSERT_TRUE(fileManager->CreateDirectory(testRootUri) != nullptr);

    constexpr const string_view paths[] = { "/folder01", "/folder02", "/folder03/", "/folder04/", "/folder05//",
        "/folder06//", "/\xf0\x9d\x93\xad\xf0\x9d\x93\xb2\xf0\x9d\x93\xbb" };
    constexpr const string_view filenames[] = { "file01", "file02", "file03", "file04", "file05", "file06", "file07",
        "file08", "file09", "\xf0\x9d\x96\x8b\xf0\x9d\x96\x8e\xf0\x9d\x96\x91\xf0\x9d\x96\x8a" };

    auto monitorPtr =
        CreateInstance<IFileMonitor>(UID_FILE_MONITOR); // Uses GetInterface to get IClassFactory from global registry
    ASSERT_TRUE(monitorPtr.get() != nullptr);
    auto& monitor = *monitorPtr;

    // Test uninitialized behavior
    EXPECT_FALSE(monitor.AddPath(testRootUri + "/folder01"));
    EXPECT_FALSE(monitor.RemovePath(testRootUri + "/folder01"));

    monitor.Initialize(*fileManager); // bind the manager and monitor.

    ASSERT_TRUE(fileManager->CreateDirectory(testRootUri + "/folder07") != nullptr);
    ASSERT_TRUE(fileManager->CreateDirectory(testRootUri + "/folder08") != nullptr);
    EXPECT_TRUE(fileManager->CreateFile(testRootUri + "/folder07" + "/" + "file00") != nullptr);
    EXPECT_TRUE(fileManager->CreateFile(testRootUri + "/folder07" + "/" + "file11") != nullptr);
    EXPECT_TRUE(monitor.AddPath(testRootUri + "/folder08"));
    EXPECT_TRUE(monitor.AddPath(testRootUri + "/folder07"));

    EXPECT_FALSE(monitor.RemovePath(testRootUri + "/folder09"));
    EXPECT_TRUE(monitor.RemovePath(testRootUri + "/folder08"));
    EXPECT_TRUE(monitor.RemovePath(testRootUri + "/folder07"));
    EXPECT_FALSE(monitor.RemovePath(testRootUri + "\\"));

    // Try to monitor path that does not exist. (this test is invalid, since we might want to know if the path is
    // created later) ASSERT_EQ(monitor.AddPath(testRootUri + "/does_not_exist"), false);

    // Create directories with some files.
    for (const auto& path : paths) {
        // Create directory.
        ASSERT_TRUE(fileManager->CreateDirectory(testRootUri + path) != nullptr);

        // Add files.
        for (const auto& filename : filenames) {
            auto file = fileManager->CreateFile(testRootUri + path + "/" + filename);
            ASSERT_TRUE(file != nullptr);
            ASSERT_EQ(file->Write("123", 3), 3);
        }
    }

    ASSERT_TRUE(fileManager->CreateDirectory(testRootUri + "/folder01/subFolder01") != nullptr);

    // Start monitoring all directories.
    for (auto& path : paths) {
        ASSERT_EQ(monitor.AddPath(testRootUri + path), true);
    }

    {
        // Make sure there are no changed files.
        vector<string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == 0 && removed.size() == 0 && modified.size() == 0);
    }

    // Add new files to all directories.
    for (const auto& path : paths) {
        {
            auto file = fileManager->CreateFile(testRootUri + path + "/new_file");
            ASSERT_TRUE(file != nullptr);
            ASSERT_EQ(file->Write("0", 1), 1);
        }
    }

    {
        // Make sure there is a new file in every directory.
        vector<string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == countof(paths) && removed.size() == 0 && modified.size() == 0);
    }

    // Time resolution is 1 sec, so we need to wait a while in order to modify the files after creation.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Re-write files in all directories.
    for (const auto& path : paths) {
        auto file = fileManager->CreateFile(testRootUri + path + "/new_file");
        ASSERT_TRUE(file != nullptr);
        ASSERT_EQ(file->Write("321", 3), 3);
        file->Close();
    }

    {
        // Make sure the files are changed.
        vector<string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == 0 && removed.size() == 0 && modified.size() == countof(paths));
    }

    // Time resolution is 1 sec, so we need to wait a while in order to modify the files after creation.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Remove files from all directories.
    for (const auto& path : paths) {
        ASSERT_TRUE(fileManager->DeleteFile(testRootUri + path + "/new_file"));
    }

    {
        // Make sure the deleted files are no longer monitored.
        vector<string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == 0 && removed.size() == countof(paths) && modified.size() == 0);
    }

    // Try to monitor subfolder of folder that is already monitored, should fail.
    ASSERT_TRUE(fileManager->CreateDirectory(testRootUri + paths[0] + "/sub") != nullptr);
    ASSERT_EQ(monitor.AddPath(testRootUri + paths[0] + "/sub"), false);

    // Try to monitor root, as children are already monitored, should fail.
    ASSERT_EQ(monitor.AddPath(testRootUri), false);

    // Delete the temp data.
    ASSERT_TRUE(recursivelyDeleteDirectory(fileManager.get(), testRootUri));
}
#endif // DISABLED_TESTS_ON

/**
 * @tc.name: missingProtocol
 * @tc.desc: Tests for Missing Protocol. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, missingProtocol, testing::ext::TestSize.Level1)
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    // Disable logging (in this object's scope) because we don't want to see the errors this test generates.
    ::Test::LogLevelScope level(GetLogger(), ILogger::LogLevel::LOG_NONE);

    {
        // Test opening a file with a protocol that does not exist.
        auto file = files->OpenFile("invalid:///not_existing_test_file");
        ASSERT_TRUE(file == nullptr);
    }

    {
        // Test opening a dir with a protocol that does not exist.
        auto dir = files->OpenDirectory("invalid:///not_existing_test_dir");
        ASSERT_TRUE(dir == nullptr);
    }
}
#ifdef DISABLED_TESTS_ON
#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: missingFilesAndDirs
 * @tc.desc: Tests for Missing Files And Dirs. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, DISABLED_missingFilesAndDirs, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: missingFilesAndDirs
 * @tc.desc: Tests for Missing Files And Dirs. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, missingFilesAndDirs, testing::ext::TestSize.Level1)
#endif
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    {
        // Test opening a file that does not exist
        auto file = files->OpenFile("app:///not_existing_test_file");
        ASSERT_TRUE(file == nullptr);
    }

    {
        // Test opening a dir that does not exist
        auto dir = files->OpenDirectory("app:///not_existing_test_dir");
        ASSERT_TRUE(dir.get() == nullptr);
    }

    {
        // Test opening a file that does not exist
        auto file = files->OpenFile("test:///not_existing_test_file");
        ASSERT_TRUE(file == nullptr);
    }

    {
        // Test opening a dir that does not exist
        auto dir = files->OpenDirectory("test:///not_existing_test_dir");
        ASSERT_TRUE(dir == nullptr);
    }
}
#if defined(__OHOS__)
/**
 * @tc.name: wrongTypes
 * @tc.desc: Tests for Wrong Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, DISABLED_wrongTypes, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: wrongTypes
 * @tc.desc: Tests for Wrong Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, wrongTypes, testing::ext::TestSize.Level1)
#endif
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    {
        // Try to open a dir as a file.
        auto file = files->OpenFile("test:///io");
        ASSERT_TRUE(file == nullptr);
    }

    {
        // Try to open a file as a dir.
        auto dir = files->OpenDirectory("test:///io/assetReadTest.txt");
        ASSERT_TRUE(dir == nullptr);
    }
}
#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: TestDirectoryListing
 * @tc.desc: Tests for Test Directory Listing. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, DISABLED_TestDirectoryListing, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: TestDirectoryListing
 * @tc.desc: Tests for Test Directory Listing. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, TestDirectoryListing, testing::ext::TestSize.Level1)
#endif
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    {
        // Test opening the root directory from a virtual files system.
        auto root = files->OpenDirectory("test:///");
        ASSERT_TRUE(root != nullptr);
        ASSERT_GT(root->GetEntries().size(), 0u);
    }

    {
        // Test opening a directory from hap.
        auto directory = files->OpenDirectory("test:///io");
        ASSERT_TRUE(directory != nullptr);

        // There should be at least one file.
        auto entries = directory->GetEntries();
        ASSERT_TRUE(!entries.empty());
    }

    {
        auto directory = files->OpenDirectory("test:///io/test_directory");
        ASSERT_TRUE(directory != nullptr);

        // Check twice, to make sure the directory pointer is reset on every call.
        for (int i = 0; i < 2; ++i) {
            // There should be 5 files and 1 directory.
            int fileCount = 0;
            int dirCount = 0;
            for (const auto& entry : directory->GetEntries()) {
                if (entry.type == IDirectory::Entry::FILE) {
                    fileCount++;
                } else if (entry.type == IDirectory::Entry::DIRECTORY) {
                    dirCount++;
                }
            }
            ASSERT_EQ(fileCount, 5);
            ASSERT_EQ(dirCount, 1);
        }
    }
}
#endif // DISABLED_TESTS_ON

/**
 * @tc.name: normalizePath
 * @tc.desc: Tests for Normalize Path. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, normalizePath, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(NormalizePath(""), "/");
    EXPECT_EQ(NormalizePath("."), "/");
    EXPECT_EQ(NormalizePath(".."), "");
    EXPECT_EQ(NormalizePath("/foo"), "/foo");
    EXPECT_EQ(NormalizePath("/foo/"), "/foo/");
    EXPECT_EQ(NormalizePath("foo/"), "/foo/");

    EXPECT_EQ(NormalizePath("/foo/bar"), "/foo/bar");
    EXPECT_EQ(NormalizePath("/foo/bar/"), "/foo/bar/");
    EXPECT_EQ(NormalizePath("foo/bar/"), "/foo/bar/");

    EXPECT_EQ(NormalizePath("/foo/bar/."), "/foo/bar/");
    EXPECT_EQ(NormalizePath("/foo/bar/./"), "/foo/bar/");
    EXPECT_EQ(NormalizePath("foo/bar/./"), "/foo/bar/");

    EXPECT_EQ(NormalizePath("/foo/./bar"), "/foo/bar");
    EXPECT_EQ(NormalizePath("/foo/./bar/"), "/foo/bar/");
    EXPECT_EQ(NormalizePath("foo/./bar/"), "/foo/bar/");

    EXPECT_EQ(NormalizePath("/foo/bar/.."), "/foo/");
    EXPECT_EQ(NormalizePath("/foo/bar/../"), "/foo/");
    EXPECT_EQ(NormalizePath("foo/bar/../"), "/foo/");

    EXPECT_EQ(NormalizePath("/foo/../bar"), "/bar");
    EXPECT_EQ(NormalizePath("/foo/../bar/"), "/bar/");
    EXPECT_EQ(NormalizePath("foo/../bar/"), "/bar/");
    EXPECT_EQ(NormalizePath("foo/../bar/.."), "/");
    EXPECT_EQ(NormalizePath("foo/../bar/../"), "/");
    EXPECT_EQ(NormalizePath("foo/../../bar/"), "");
}
#ifdef DISABLED_TESTS_ON

/**
 * @tc.name: fileApiTest
 * @tc.desc: Tests for File Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, DISABLED_fileApiTest, testing::ext::TestSize.Level1)
{
    auto factory = CORE_NS::GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    EXPECT_TRUE(factory != nullptr);
    factory->Ref();
    factory->Unref();
    EXPECT_FALSE(factory->GetInterface(UID_GLOBAL_FACTORY) != nullptr);
    EXPECT_FALSE(factory->CreateInstance(UID_GLOBAL_FACTORY) != nullptr);
    EXPECT_FALSE(factory->CreateStdFileSystem("") != nullptr);
    const IFileSystemApi& consF = *factory;
    EXPECT_TRUE(consF.GetInterface(IFileSystemApi::UID) != nullptr);
}

/**
 * @tc.name: pathToolTest
 * @tc.desc: Tests for Path Tool Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, DISABLED_pathToolTest, testing::ext::TestSize.Level1)
{
#ifdef WIN32
    string_view curDrive, curPath, curFName, curExt;
    SplitPath("./file", curDrive, curPath, curFName, curExt);
    EXPECT_EQ(curFName, "file");
    SplitPath(".", curDrive, curPath, curFName, curExt);
    EXPECT_EQ(curFName, "");
#endif
    EXPECT_TRUE(IsRelative(""));
}
#endif // DISABLED_TESTS_ON

/**
 * @tc.name: fileModeTest
 * @tc.desc: Tests for File Mode Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_IoTest, fileModeTest, testing::ext::TestSize.Level1)
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);

    // create a file via the Filemanager which can be then opened with StdFile.
    constexpr const string_view fileName = "app:///testFile.txt";
    string path;
    {
        auto file = files->CreateFile(fileName);
        ASSERT_TRUE(file);
        const auto entry = files->GetEntry(fileName);
        path = entry.name;
        file->Close();
    }
    {
        EXPECT_TRUE(StdFile::Open(path, IFile::Mode::READ_ONLY));
    }
    {
        EXPECT_TRUE(StdFile::Open(path, IFile::Mode::READ_WRITE));
    }
    files->DeleteFile(fileName);

    EXPECT_FALSE(StdFile::Create("", IFile::Mode::READ_WRITE));
}