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

#include <gtest/gtest.h>
#include <app.h>
#include <coff.h>
#include <elf32.h>
#include <elf64.h>
#include <elf_common.h>
#include <dir.h>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace {
void deleteOldFiles()
{
    fs::path currentDir = fs::current_path();
    for (const auto& entry : fs::directory_iterator(currentDir)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            if (extension == ".obj" || extension == ".o") {
                fs::remove(entry.path());
            }
        }
    }
}
bool isFileSizeAbove1KB(const std::string& filename)
{
    std::uintmax_t filesize = fs::file_size(filename);
    return filesize > 1024; // 1024: size
}
std::string getCurrentAbsolutePathExe(const std::string& filename)
{
    fs::path currentDir = fs::current_path();
    return (currentDir / filename).string();
}
std::string getCurrentAbsolutePathAssets(const std::string& filename)
{
    fs::path fullPath = fs::path(TEST_ASSET) / fs::path(filename);
    return fullPath.string();
}
bool comapreFiles(const std::string& file1, const std::string& file2)
{
    std::ifstream f1(file1, std::ios::binary || std::ios::ate);
    std::ifstream f2(file2, std::ios::binary || std::ios::ate);

    // file open failed
    if (f1.fail() || f2.fail()) {
        return false;
    }

    // file sizes are different
    if (f1.tellg() != f2.tellg()) {
        return false;
    }

    f1.seekg(0, std::ios::beg);
    f2.seekg(0, std::ios::beg);

    std::vector<char> buf1(std::istreambuf_iterator<char>(f1), {});
    std::vector<char> buf2(std::istreambuf_iterator<char>(f2), {});

    return buf1 == buf2;
}
} // namespace

TEST(PlatformCompileTest, CompileShaderWindows)
{
    deleteOldFiles();
    const char* objFileName = "rofs_64.obj";

    int argc = 7;
    char* argv[] = { "LumeAssetCompilerTestRunner.exe", "-windows", "-x64", "-extensions", ".shader;", TEST_ASSET,
        "./" };
    app_main(argc, argv);

    ASSERT_TRUE(fs::exists(getCurrentAbsolutePathExe(objFileName)));
    EXPECT_TRUE(isFileSizeAbove1KB(objFileName));
}

TEST(PlatformCompileTest, CompileShaderLinux)
{
    deleteOldFiles();
    const char* objFileName = "rofs_64.o";

    int argc = 7;
    char* argv[] = { "LumeAssetCompilerTestRunner.exe", "-linux", "-arm64-v8a", "-extensions", ".shader;", TEST_ASSET,
        "./" };
    app_main(argc, argv);

    ASSERT_TRUE(fs::exists(getCurrentAbsolutePathExe(objFileName)));
    EXPECT_TRUE(isFileSizeAbove1KB(objFileName));
}

TEST(PlatformCompileTest, CompileShaderAndroid)
{
    deleteOldFiles();
    const char* objFileName = "rofs_32.o";

    int argc = 7;
    char* argv[] = { "LumeAssetCompilerTestRunner.exe", "-android", "-armeabi-v7a", "-extensions", ".shader;",
        TEST_ASSET, "./" };
    app_main(argc, argv);

    ASSERT_TRUE(fs::exists(getCurrentAbsolutePathExe(objFileName)));
    EXPECT_TRUE(isFileSizeAbove1KB(objFileName));
}

TEST(PlatformCompileTest, CompileShaderMac)
{
    deleteOldFiles();
    const char* objFileName = "rofs_64.o";

    int argc = 7;
    char* argv[] = { "LumeAssetCompilerTestRunner.exe", "-mac", "-arm64-v8a", "-extensions", ".shader;", TEST_ASSET,
        "./" };
    app_main(argc, argv);

    ASSERT_TRUE(fs::exists(getCurrentAbsolutePathExe(objFileName)));
    EXPECT_TRUE(isFileSizeAbove1KB(objFileName));
}

TEST(MAIN_TEST, InsufficientArguments)
{
    deleteOldFiles();
    char* argv[] = { "LumeAssetCompiler.exe" };
    int argc = 1;
    int result = app_main(argc, argv);

    EXPECT_EQ(result, -1);
}

TEST(MAIN_TEST, InvalidArgumentCount)
{
    deleteOldFiles();
    int argc = 2;
    char* argv[] = { "LumeAssetCompiler.exe", TEST_ASSET };
    int result = app_main(argc, argv);

    EXPECT_NE(result, 0);
}

TEST(MAIN_TEST, EnoughArguments)
{
    deleteOldFiles();
    const char* objFileName = "rofs_64.obj";

    int argc = 5;
    char* argv[] = {
        "LumeAssetCompilerTestRuner.exe",
        "-windows",
        "-x64",
        TEST_ASSET,
        "./",

    };

    EXPECT_EQ(app_main(argc, argv), 0);
    ASSERT_TRUE(fs::exists(getCurrentAbsolutePathExe(objFileName)));
    EXPECT_TRUE(isFileSizeAbove1KB(objFileName));
}