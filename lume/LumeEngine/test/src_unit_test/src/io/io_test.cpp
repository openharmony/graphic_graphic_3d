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

#include <chrono>
#include <filesystem>
#include <thread>

#include <gtest/gtest.h>

#include <base/containers/fixed_string.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_monitor.h>
#include <core/io/intf_filesystem_api.h>
#include <core/log.h>
#include <core/os/intf_platform.h>

#include "io/dev/file_monitor.h"
#include "io/file_manager.h"
#include "io/memory_file.h"
#include "io/path_tools.h"
#include "io/std_directory.h"
#include "io/std_file.h"
#include "io/std_filesystem.h"
#include "TestRunner.h"

using namespace CORE_NS;
using namespace BASE_NS;
using namespace testing;
using namespace testing::ext;

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

class IoTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: FactoryTests
 * @tc.desc: test FactoryTests
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, FactoryTests, TestSize.Level1)
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
 * @tc.name: CustomTests
 * @tc.desc: test CustomTests
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, CustomTests, TestSize.Level1)
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
        BASE_NS::vector<BASE_NS::string> GetUriPaths(const string_view uri) const override
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

    files->RegisterFilesystem("MyTemp", IFilesystem::Ptr(new TempFS()));

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
 * @tc.name: MemoryFileSysTests
 * @tc.desc: test MemoryFileSysTests
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, MemoryFileSysTests, TestSize.Level1)
{
    auto data = std::string("1234567890");
    auto factory = CORE_NS::GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    auto files = factory->CreateFilemanager();
    const auto& memP = factory->CreateMemFileSystem();
    ASSERT_TRUE(memP != nullptr);
    ASSERT_TRUE(memP->CreateFile("io/test_directory/1.txt") != nullptr);
    ASSERT_TRUE(memP->CreateFile("io/test_directory/2.txt") != nullptr);
    ASSERT_TRUE(memP->CreateFile("io/test_directory/2.txt") != nullptr);
    ASSERT_FALSE(memP->OpenFile("io/test_directory/2.txt", IFile::Mode::READ_ONLY) != nullptr);
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

/**
 * @tc.name: FileCreationAndDeletion
 * @tc.desc: test FileCreationAndDeletion
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, FileCreationAndDeletion, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    auto filename1 = "file:///data/local/test_file01.dat";
    auto filename2 = "";
    auto filename3 = "unknown://false";
    // UTF-8 string
    auto filename4 = "file:///data/local/\xf0\x9d\x95\xb7\xf0\x9d\x96\x9a\xf0\x9d\x96\x92\xf0\x9d\x96\x8a.dat";

    // Make sure there is no existing file.
    EXPECT_FALSE(fileManager.DeleteFile(filename1));
    EXPECT_FALSE(fileManager.DeleteFile(filename2));
    EXPECT_FALSE(fileManager.DeleteFile(filename3));
    EXPECT_FALSE(fileManager.DeleteFile(filename4));

    {
        EXPECT_FALSE(fileManager.FileExists(filename1));
        // Try to create a new file.
        auto newFile = fileManager.CreateFile(filename1);
        EXPECT_TRUE(newFile);
        EXPECT_TRUE(fileManager.FileExists(filename1));
        // EXPECT_FALSE
        auto newFile2 = fileManager.CreateFile(filename2);
        EXPECT_FALSE(newFile2);
        EXPECT_FALSE(fileManager.FileExists(filename2));
        auto newFile3 = fileManager.CreateFile(filename3);
        EXPECT_FALSE(newFile3);
        EXPECT_FALSE(fileManager.FileExists(filename3));
        auto newFile4 = fileManager.CreateFile(filename4);
        EXPECT_TRUE(newFile4);
        EXPECT_TRUE(fileManager.FileExists(filename4));
    }

    {
        // Try to open newly created file.
        auto file1 = fileManager.OpenFile(filename1);
        ASSERT_TRUE(file1);
        auto file4 = fileManager.OpenFile(filename4);
        ASSERT_TRUE(file4);
    }
    {
        // Delete the file.
        bool deletionResult = fileManager.DeleteFile(filename1);
        EXPECT_TRUE(deletionResult);
        EXPECT_FALSE(fileManager.FileExists(filename1));
        bool deletionResult2 = fileManager.DeleteFile(filename2);
        EXPECT_FALSE(deletionResult2);
        bool deletionResult3 = fileManager.DeleteFile(filename3);
        EXPECT_FALSE(deletionResult2);
        bool deletionResult4 = fileManager.DeleteFile(filename4);
        EXPECT_TRUE(deletionResult4);
        EXPECT_FALSE(fileManager.FileExists(filename4));
    }
}


namespace {
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
} // namespace

/**
 * @tc.name: FileRename
 * @tc.desc: test FileRename
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, FileRename, TestSize.Level1)
{
    auto data = std::string("1234567890");

    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();
    auto fromFilename = "file:///data/local/test_file03.dat";
    auto toFilename = "file:///data/local/test_file04.dat";
    auto differentProto = "app:/test_file04.dat";
    auto toUtf8Filename = "file:///data/local/"
                        "\xf0\x9d\x96\x99\xf0\x9d\x96\x8a\xf0\x9d\x96\x98\xf0\x9d\x96\x99\x5f\xf0\x9d\x96\x8b\xf0"
                        "\x9d\x96\x8e\xf0\x9d\x96\x91\xf0\x9d\x96\x8a\x30\x35.dat";

    // Make sure none of the files exist e.g. due to an interrupted test run.
    fileManager.DeleteFile(fromFilename);
    fileManager.DeleteFile(toFilename);
    fileManager.DeleteFile(toUtf8Filename);

    {
        // Create file.
        auto file = fileManager.CreateFile(fromFilename);
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
    ASSERT_FALSE(fileManager.Rename(fromFilename, differentProto));

    // Rename file with same protocol, should pass
    ASSERT_TRUE(fileManager.Rename(fromFilename, toFilename));

    {
        // Open file, read all at once.
        auto file = fileManager.OpenFile(toFilename);
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
        EXPECT_TRUE(fileManager.Rename(toFilename, toUtf8Filename));
        auto file = fileManager.OpenFile(toUtf8Filename);
        ASSERT_TRUE(file);

        // Ensure length.
        auto length = file->GetLength();
        EXPECT_EQ(length, data.length());

        file->Close();

        EXPECT_TRUE(fileManager.Rename(toUtf8Filename, toFilename));
    }

    // Delete the file
    EXPECT_TRUE(fileManager.DeleteFile(toFilename));
}

/**
 * @tc.name: DirectoryFilelist
 * @tc.desc: test DirectoryFilelist
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, DirectoryFilelist, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    constexpr const string_view directoryUri = "file:///data/local/test_directory_filelist";

    ASSERT_EQ(recursivelyDeleteDirectory(&fileManager, directoryUri), true);

    {
        // Create test directory and add 10 files.
        auto directory = fileManager.CreateDirectory(directoryUri);
        ASSERT_NE(directory, nullptr);

        for (int i = 0; i < 10; ++i) {
            auto file = fileManager.CreateFile(directoryUri + '/' + "file" + to_string(i));
            ASSERT_TRUE(file);
            file->Write("12345", 5);
        }
    }

    EXPECT_TRUE(fileManager.CreateDirectory(directoryUri + '/' + "dir1") != nullptr);
    EXPECT_TRUE(fileManager.CreateDirectory(directoryUri + '/' + "dir2") != nullptr);
    EXPECT_TRUE(fileManager.CreateDirectory(directoryUri + '/' + "dir3") != nullptr);

    // Ensure that we are able to read back what was created above.
    auto directory = fileManager.OpenDirectory(directoryUri);
    if (directory) {
        // Check twice, to make sure the directory pointer is reset on every call.
        for (int i = 0; i < 2; ++i) {
            // There should be 10 files and 3 dirs.
            auto entries = directory->GetEntries();
            ASSERT_EQ(entries.size(), 13);
        }
    }

    // Delete the test data.
    ASSERT_EQ(recursivelyDeleteDirectory(&fileManager, directoryUri), true);
}

/**
 * @tc.name: FileMonitorTest
 * @tc.desc: test FileMonitorTest
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, FileMonitorTest, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    // Application root for test.
    constexpr const string_view testRootUri = "file:///data/local///test_directory_monitor";

    // Clean up.
    recursivelyDeleteDirectory(&fileManager, testRootUri);

    // Create root directory.
    ASSERT_TRUE(fileManager.CreateDirectory(testRootUri) != nullptr);

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

    monitor.Initialize(fileManager); // bind the manager and monitor.

    ASSERT_TRUE(fileManager.CreateDirectory(testRootUri + "/folder07") != nullptr);
    ASSERT_TRUE(fileManager.CreateDirectory(testRootUri + "/folder08") != nullptr);
    EXPECT_TRUE(fileManager.CreateFile(testRootUri + "/folder07" + "/" + "file00") != nullptr);
    EXPECT_TRUE(fileManager.CreateFile(testRootUri + "/folder07" + "/" + "file11") != nullptr);
    EXPECT_TRUE(monitor.AddPath(testRootUri + "/folder08"));
    EXPECT_TRUE(monitor.AddPath(testRootUri + "/folder07"));

    EXPECT_FALSE(monitor.RemovePath(testRootUri + "/folder09"));
    EXPECT_TRUE(monitor.RemovePath(testRootUri + "/folder08"));
    EXPECT_TRUE(monitor.RemovePath(testRootUri + "/folder07"));
    EXPECT_FALSE(monitor.RemovePath(testRootUri + "\\"));

    // Try to monitor path that does not exist. (this test is invalid, since we might want to know if the path is
    // created later)

    // Create directories with some files.
    for (const auto& path : paths) {
        // Create directory.
        ASSERT_TRUE(fileManager.CreateDirectory(testRootUri + path) != nullptr);

        // Add files.
        for (const auto& filename : filenames) {
            auto file = fileManager.CreateFile(testRootUri + path + "/" + filename);
            ASSERT_TRUE(file != nullptr);
            ASSERT_EQ(file->Write("123", 3), 3);
        }
    }

    ASSERT_TRUE(fileManager.CreateDirectory(testRootUri + "/folder01/subFolder01") != nullptr);

    // Start monitoring all directories.
    for (auto& path : paths) {
        ASSERT_EQ(monitor.AddPath(testRootUri + path), true);
    }

    {
        // Make sure there are no changed files.
        BASE_NS::vector<BASE_NS::string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == 0 && removed.size() == 0 && modified.size() == 0);
    }

    // Add new files to all directories.
    for (const auto& path : paths) {
        {
            auto file = fileManager.CreateFile(testRootUri + path + "/new_file");
            ASSERT_TRUE(file != nullptr);
            ASSERT_EQ(file->Write("0", 1), 1);
        }
    }

    {
        // Make sure there is a new file in every directory.
        BASE_NS::vector<BASE_NS::string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == countof(paths) && removed.size() == 0 && modified.size() == 0);
    }

    // Time resolution is 1 sec, so we need to wait a while in order to modify the files after creation.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Re-write files in all directories.
    for (const auto& path : paths) {
        auto file = fileManager.CreateFile(testRootUri + path + "/new_file");
        ASSERT_TRUE(file != nullptr);
        ASSERT_EQ(file->Write("321", 3), 3);
        file->Close();
    }

    {
        // Make sure the files are changed.
        BASE_NS::vector<BASE_NS::string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == 0 && removed.size() == 0 && modified.size() == countof(paths));
    }

    // Time resolution is 1 sec, so we need to wait a while in order to modify the files after creation.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Remove files from all directories.
    for (const auto& path : paths) {
        ASSERT_TRUE(fileManager.DeleteFile(testRootUri + path + "/new_file"));
    }

    {
        // Make sure the deleted files are no longer monitored.
        BASE_NS::vector<BASE_NS::string> added, removed, modified;
        monitor.ScanModifications(added, removed, modified);
        ASSERT_TRUE(added.size() == 0 && removed.size() == countof(paths) && modified.size() == 0);
    }

    // Try to monitor subfolder of folder that is already monitored, should fail.
    ASSERT_TRUE(fileManager.CreateDirectory(testRootUri + paths[0] + "/sub") != nullptr);
    ASSERT_EQ(monitor.AddPath(testRootUri + paths[0] + "/sub"), false);

    // Try to monitor root, as children are already monitored, should fail.
    ASSERT_EQ(monitor.AddPath(testRootUri), false);

    // Delete the temp data.
    ASSERT_TRUE(recursivelyDeleteDirectory(&fileManager, testRootUri));
}

/**
 * @tc.name: MissingProtocol
 * @tc.desc: test MissingProtocol
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, MissingProtocol, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    {
        // Test opening a file with a protocol that does not exist.
        auto file = fileManager.OpenFile("file:///not_existing_test_file");
        ASSERT_TRUE(file == nullptr);
    }

    {
        // Test opening a dir with a protocol that does not exist.
        auto dir = fileManager.OpenDirectory("file:///not_existing_test_dir");
        ASSERT_TRUE(dir == nullptr);
    }
}

/**
 * @tc.name: MissingFilesAndDirs
 * @tc.desc: test MissingFilesAndDirs
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, MissingFilesAndDirs, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    {
        auto file = fileManager.OpenFile("file:///data/local/not_existing_test_file");
        ASSERT_TRUE(file == nullptr);
    }

    {
        auto dir = fileManager.OpenDirectory("file:///data/local/not_existing_test_dir");
        ASSERT_TRUE(dir.get() == nullptr);
    }

    {
        auto file = fileManager.OpenFile("file:///data/local/not_existing_test_file");
        ASSERT_TRUE(file == nullptr);
    }

    {
        auto dir = fileManager.OpenDirectory("file:///data/local/not_existing_test_dir");
        ASSERT_TRUE(dir == nullptr);
    }
}

/**
 * @tc.name: WrongTypes
 * @tc.desc: test WrongTypes
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, WrongTypes, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    {
        // Try to open a dir as a file.
        auto file = fileManager.OpenFile("file:///data/local/io");
        ASSERT_TRUE(file == nullptr);
    }

    {
        // Try to open a file as a dir.
        auto dir = fileManager.OpenDirectory("file:///data/local/test_data/io/assetReadTest.txt");
        ASSERT_TRUE(dir == nullptr);
    }
}

/**
 * @tc.name: TestDirectoryListing
 * @tc.desc: test TestDirectoryListing
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, TestDirectoryListing, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    {
        // Test opening the root directory from a virtual files system.
        auto root = fileManager.OpenDirectory("file:///data/local/");
        ASSERT_TRUE(root != nullptr);
        ASSERT_GT(root->GetEntries().size(), 0u);
    }

    {
        auto directory = fileManager.OpenDirectory("file:///data/local/test_data/io");
        ASSERT_TRUE(directory != nullptr);

        // There should be at least one file.
        auto entries = directory->GetEntries();
        ASSERT_TRUE(!entries.empty());
    }

    {
        auto directory = fileManager.OpenDirectory("file:///data/local/test_data/io/test_directory");
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

/**
 * @tc.name: NormalizePath
 * @tc.desc: test NormalizePath
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, NormalizePath, TestSize.Level1)
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

/**
 * @tc.name: FileModeTest
 * @tc.desc: test FileModeTest
 * @tc.type: FUNC
 */
HWTEST_F(IoTest, FileModeTest, TestSize.Level1)
{
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    // create a file via the Filemanager which can be then opened with StdFile.
    constexpr const BASE_NS::string_view fileName = "file:///data/local/testFile.txt";
    BASE_NS::string path;
    {
        auto file = fileManager.CreateFile(fileName);
        ASSERT_TRUE(file);
        const auto entry = fileManager.GetEntry(fileName);
        path = entry.name;
        file->Close();
    }
    {
        EXPECT_TRUE(StdFile::Open(path, IFile::Mode::READ_ONLY));
    }
    {
        EXPECT_TRUE(StdFile::Open(path, IFile::Mode::READ_WRITE));
    }
    fileManager.DeleteFile(fileName);

    EXPECT_FALSE(StdFile::Create("", IFile::Mode::READ_WRITE));
}