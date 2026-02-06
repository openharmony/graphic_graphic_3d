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

// standard library
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

// third party headers
#include <gtest/gtest.h>

// internal
#include "compiler_main.h"
#include "io/dev/FileMonitor.h"
#include "lume/Log.h"
#include "shader_type.h"
#include "spirv_cross_helper_structs_gles.h"

#define CORE_SHADER_TEST

namespace UTest {
class EnvFolderBinder {
public:
    EnvFolderBinder()
    {
        auto bindFolder = [](std::string_view protocal0, std::string_view protocal1) {
            std::string runTimeFolderPrefix;
            auto tryAroot = [&](std::string_view root) {
                for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root)) {
                    std::error_code error;
                    if (!entry.is_directory(error) || error) {
                        continue;
                    }
                    if (entry.path().stem() == protocal0) {
                        auto nextLayerPath = entry.path() / protocal1;
                        if (std::filesystem::exists(nextLayerPath, error) &&
                            std::filesystem::is_directory(nextLayerPath, error)) {
                            runTimeFolderPrefix = std::filesystem::canonical(entry.path(), error).string();
                            return true;
                        }
                    }
                }
                return false;
            };
            bool success = tryAroot(".") || tryAroot("..") || tryAroot("../..") || tryAroot("../../..") ||
                tryAroot("../../../..") || tryAroot("../../../../..");

            EXPECT_TRUE(success)
                << "FAIL to use %s folder to locate root folder for unit test, check where is the running exe?";
            return runTimeFolderPrefix;
        };
        shaderCompilerRoot_ = bindFolder("test", "src_unit_test\\assets\\test_shaders");
        testShadersRoot_ = shaderCompilerRoot_ + "\\src_unit_test\\assets\\test_shaders\\";
    }

    std::string GetTestShaderRoot() const
    {
        return testShadersRoot_;
    }

    std::string GetShaderCompilerRoot() const
    {
        return shaderCompilerRoot_;
    }

    std::string GetShaderAssetsFolder() const
    {
        return shaderCompilerRoot_ + "\\src_unit_test\\assets\\";
    }

private:
    std::string testShadersRoot_;
    std::string shaderCompilerRoot_;
};

std::vector<std::thread> pool; // globalThreadPool

EnvFolderBinder gFoldBinder;

int RunMain(std::vector<std::string>& argStrings)
{
    const int argc = static_cast<int>(argStrings.size());
    char** argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = argStrings[i].data();
    }
    auto ret = CompilerMain(argc, argv);
    delete[] argv;
    return ret;
}

// compile lume 3d core shaders, to verify shader compiler's code is consistant with before.
#ifdef CORE_SHADER_TEST
// test fixture
class ShaderCompilationConsistencyTest : public ::testing::Test {
public:
    void SetUp() override
    {
        lume::GetLogger().SetLogLevel(lume::ILogger::LogLevel::INFO);

        if (std::filesystem::exists(testFolder_)) {
            std::filesystem::remove_all(testFolder_);
        }
        std::filesystem::create_directory(testFolder_);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(testFolder_);
    }
    static constexpr std::string_view testFolder_ = "./testFolder0/";
};

bool CompareWithGoldenReference(std::string_view result, std::string_view golden)
{
    auto resultPath = std::filesystem::u8path(result);
    auto goldenPath = std::filesystem::u8path(golden);
    if (!std::filesystem::exists(resultPath) || !std::filesystem::exists(goldenPath)) {
        return false;
    }
    if (std::filesystem::file_size(resultPath) != std::filesystem::file_size(goldenPath)) {
        return false;
    }

    std::ifstream resultStream(resultPath, std::ios::in | std::ios::binary);
    std::ifstream goldenStream(goldenPath, std::ios::in | std::ios::binary);
    if (!resultStream.is_open() || !goldenStream.is_open()) {
        return false;
    }
    constexpr auto bufferSize = 1024U;
    std::ifstream::char_type resultBuffer[bufferSize];
    std::ifstream::char_type goldenBuffer[bufferSize];
    while (resultStream.good() && goldenStream.good()) {
        resultStream.read(resultBuffer, bufferSize);
        const auto resultSize = resultStream.gcount();
        goldenStream.read(goldenBuffer, bufferSize);
        const auto goldenSize = goldenStream.gcount();
        if (resultSize != goldenSize) {
            return false;
        }
        if (!std::equal(std::begin(resultBuffer), std::begin(resultBuffer) + resultSize, std::begin(goldenBuffer),
                std::begin(goldenBuffer) + goldenSize)) {
            return false;
        }
    }
    return true;
}

TEST_F(ShaderCompilationConsistencyTest, core3d_dm_depth_vert)
{
    // compile several shader files to validate LumeShaderCompiler is working
    // E.g. take core3d_dm_fw.shader
    // Compare the output to golden output text file Compare lsb, spv, spv.gl,spv.gles
    static constexpr auto sourceFile = "core3d_dm_depth.vert";
    auto destination = std::string{ testFolder_ };
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        gFoldBinder.GetShaderAssetsFolder() + "shaders\\shader\\" + sourceFile,
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\Lume3D\\api",
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\LumeRender\\api",
        { "--optimize" },
        { "--destination" },
        destination,

    };
    ASSERT_EQ(RunMain(argv), 0);

    EXPECT_TRUE(CompareWithGoldenReference(destination + sourceFile + ".spv",
        gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
}

TEST_F(ShaderCompilationConsistencyTest, core3d_dm_depth_frag)
{
    // compile several shader files to validate LumeShaderCompiler is working
    // E.g. take core3d_dm_fw.shader
    // Compare the output to golden output text file Compare lsb, spv, spv.gl,spv.gles
    static constexpr auto sourceFile = "core3d_dm_depth.frag";
    auto destination = std::string{ testFolder_ };
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        gFoldBinder.GetShaderAssetsFolder() + "shaders\\shader\\" + sourceFile,
        { "--optimize" },
        { "--destination" },
        destination,
    };
    {
        ASSERT_EQ(RunMain(argv), 0);
        EXPECT_TRUE(CompareWithGoldenReference(std::string{ testFolder_ } + sourceFile + ".spv",
            gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
    }
    argv.emplace_back("--vulkan");
    argv.emplace_back("1.1");
    {
        ASSERT_EQ(RunMain(argv), 0);
        EXPECT_FALSE(CompareWithGoldenReference(std::string{ testFolder_ } + sourceFile + ".spv",
            gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
    }
    argv.back().assign("1.4");
    {
        ASSERT_NE(RunMain(argv), 0);
    }
}

TEST_F(ShaderCompilationConsistencyTest, core3d_dm_depth_vsm_frag)
{
    // compile several shader files to validate LumeShaderCompiler is working
    // E.g. take core3d_dm_fw.shader
    // Compare the output to golden output text file Compare lsb, spv, spv.gl,spv.gles
    static constexpr auto sourceFile = "core3d_dm_depth_vsm.frag";
    auto destination = std::string{ testFolder_ };
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        gFoldBinder.GetShaderAssetsFolder() + "shaders\\shader\\" + sourceFile,
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\Lume3D\\api",
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\LumeRender\\api",
        { "--destination" },
        destination,
    };
    ASSERT_EQ(RunMain(argv), 0);

    EXPECT_TRUE(CompareWithGoldenReference(destination + sourceFile + ".spv",
        gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
}

TEST_F(ShaderCompilationConsistencyTest, core3d_dm_fw_vert)
{
    // compile several shader files to validate LumeShaderCompiler is working
    // E.g. take core3d_dm_fw.shader
    // Compare the output to golden output text file Compare lsb, spv, spv.gl,spv.gles

    static constexpr auto sourceFile = "core3d_dm_fw.vert";
    auto destination = std::string{ testFolder_ };
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        gFoldBinder.GetShaderAssetsFolder() + "shaders\\shader\\" + sourceFile,
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\Lume3D\\api",
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\LumeRender\\api",
        { "--optimize" },
        { "--destination" },
        destination,
    };
    ASSERT_EQ(RunMain(argv), 0);

    EXPECT_TRUE(CompareWithGoldenReference(destination + sourceFile + ".spv",
        gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
}

TEST_F(ShaderCompilationConsistencyTest, core3d_dm_fw_frag)
{
    // compile several shader files to validate LumeShaderCompiler is working
    // E.g. take core3d_dm_fw.shader
    // Compare the output to golden output text file Compare lsb, spv, spv.gl,spv.gles

    static constexpr auto sourceFile = "core3d_dm_fw.frag";
    auto destination = std::string{ testFolder_ };
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        gFoldBinder.GetShaderAssetsFolder() + "shaders\\shader\\" + sourceFile,
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\Lume3D\\api",
        { "--include" },
        { gFoldBinder.GetShaderAssetsFolder() + "shaders" },
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\LumeRender\\api",
        { "--optimize" },
        { "--destination" },
        destination,
    };
    ASSERT_EQ(RunMain(argv), 0);

    EXPECT_TRUE(CompareWithGoldenReference(destination + sourceFile + ".spv",
        gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
}

TEST_F(ShaderCompilationConsistencyTest, core3d_dm_fw_mv_vert)
{
    // compile several shader files to validate LumeShaderCompiler is working
    // E.g. take core3d_dm_fw.shader
    // Compare the output to golden output text file Compare lsb, spv, spv.gl,spv.gles

    static constexpr auto sourceFile = "core3d_dm_fw_mv.vert";
    auto destination = std::string{ testFolder_ };
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        gFoldBinder.GetShaderAssetsFolder() + "shaders\\shader\\" + sourceFile,
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\Lume3D\\api",
        { "--include" },
        gFoldBinder.GetShaderAssetsFolder() + "api\\LumeRender\\api",
        { "--optimize" },
        { "--destination" },
        destination,
    };
    ASSERT_EQ(RunMain(argv), 0);

    EXPECT_TRUE(CompareWithGoldenReference(destination + sourceFile + ".spv",
        gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
}

TEST_F(ShaderCompilationConsistencyTest, core3d_dm_morph_comp)
{
    static constexpr auto sourceFile = "core3d_dm_morph.comp";
    auto destination = std::string{ testFolder_ };
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { gFoldBinder.GetShaderAssetsFolder() + "shaders\\computeshader\\core3d_dm_morph.comp" },
        { "--include" },
        { gFoldBinder.GetShaderAssetsFolder() + "api\\Lume3D\\api" },
        { "--include" },
        { gFoldBinder.GetShaderAssetsFolder() + "shaders" },
        { "--include" },
        { gFoldBinder.GetShaderAssetsFolder() + "api\\LumeRender\\api" },
        { "--destination" },
        destination,
    };
    ASSERT_EQ(RunMain(argv), 0);

    EXPECT_TRUE(CompareWithGoldenReference(destination + sourceFile + ".spv",
        gFoldBinder.GetShaderAssetsFolder() + "goldenReferenceBinary\\" + sourceFile + ".spv"));
}
#endif // CORE_SHADER_TEST

TEST(LumeShaderCompilation, ProcessStruct)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "core3d_dm_depth_modify4Test.frag" },
        { "--destination" },
        { testShaderRoot },

    };
    ASSERT_EQ(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, extension_enable)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "extension_enable.vert" },
        { "--destination" },
        { testShaderRoot },

    };
    ASSERT_EQ(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, ProcessResources)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "images_samplerTest.frag" },
        { "--vulkan" },
        { "1.2" },
    };
    ASSERT_EQ(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, VertexInputDeclaration)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "vertexInputDeclartionTest.frag" },
        { "--vulkan" },
        { "1.2" },
        { "--strip-debug-information" },
    };
    // preprocess error
    ASSERT_EQ(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, SourceFileTest)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--source" },
        { testShaderRoot + "shareBinding//" },
        { "--vulkan" },
        { "1.0" },
    };
    ASSERT_EQ(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, invalidShader4)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "invalidShader4.frag" },
    };
    // preprocess error
    ASSERT_NE(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, unsupportedExtension)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();

    std::filesystem::remove(
        std::filesystem::path{ std::string{ testShaderRoot + "images_samplerTest.frag" } }.replace_extension(".meta"));

    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "images_samplerTest.frag" },
        { "--vulkan" },
        { "1.0" },
    };
    // #extension GL_EXT_ray_tracing : enable will cause compiler report not supported spiv-v verison
    ASSERT_NE(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, invalidShader0)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "invalidShader0.frag" },
    };
    ASSERT_NE(RunMain(argv), 0);
}

TEST(LumeShaderCompilation, invalidShader1)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv2 = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "invalidShader1.frag" },
    };
    // assertion from compiler_main.cpp, line 982
    EXPECT_DEBUG_DEATH(RunMain(argv2), ".*");
}

TEST(LumeShaderCompilation, invalidShader2)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv2 = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "invalidShader2.frag" },
    };
    // assertion from spirv_cross_helpers_gles.cpp, line 276
    EXPECT_DEBUG_DEATH(RunMain(argv2), ".*");
}

TEST(LumeShaderCompilation, argParse)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();

    { // test compiled destination is empty
        std::vector<std::string> argv1 = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { "" },
        };
        EXPECT_EQ(RunMain(argv1), 1);
    }
    {
        std::vector<std::string> argv1 = {
            { "LumeShaderCompiler" },
            { "--source" },
        };
        EXPECT_EQ(RunMain(argv1), 1);
    }
    {
        std::vector<std::string> argv2 = {
            { "LumeShaderCompiler" },
            { "--help" },
        };
        EXPECT_EQ(RunMain(argv2), 1);
    }
    {
        std::vector<std::string> argv3 = {
            { "LumeShaderCompiler" },
            { "--source" },
            { testShaderRoot + "DONTEXST_PATH" },
        };
        EXPECT_EQ(RunMain(argv3), 1);
    }
    {
        std::vector<std::string> argv4 = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { testShaderRoot + "DONTEXST_PATH" },
        };
        EXPECT_EQ(RunMain(argv4), 0);
    }
    {
        std::vector<std::string> argv5 = {
            { "LumeShaderCompiler" },
        };
        EXPECT_EQ(RunMain(argv5), 0);
    }
    {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { testShaderRoot + "testShaderInvalid.frag" },
            { "--vulkan" },
            { "1.2" },
        };
        ASSERT_NE(RunMain(argv), 0);
    }
    {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--source" },
            { testShaderRoot },
            { "--sourceFile" },
            { testShaderRoot + "testShader.shader" },
            { "--vulkan" },
            { "1.1" },
        };
        EXPECT_EQ(RunMain(argv), 0);
    }
    {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { testShaderRoot + "testShader.shader" },
            { "--vulkan" },
            { "1.1" },
        };
        EXPECT_EQ(RunMain(argv), 0);
    }
    {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { testShaderRoot + "testShader.json" },
            { "--vulkan" },
            { "1.2" },
        };
        EXPECT_EQ(RunMain(argv), 0);
    }
    {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { testShaderRoot + "testShader.json" },
            { "--vulkan" },
            { "1.3" },
        };
        EXPECT_EQ(RunMain(argv), 0);
    }
    {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { testShaderRoot + "testShader.json" },
            { "--vulkan" },
            { "1.4" },
        };
        EXPECT_EQ(RunMain(argv), 1);
    }
    {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--wrong parameter" },
        };
        EXPECT_EQ(RunMain(argv), 1);
    }
}

constexpr auto SHADER_SOURCE =
    R"(#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "test_inc.h"
void main(void)
{
    foo();
}
)";

constexpr auto SHADER_INCLUDE =
    R"(#ifndef TEST_INC_H
#define TEST_INC_H
int foo()
{
    return 42;
}
#endif
)";

TEST(LumeShaderCompilation, NonAscii)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::filesystem::path directory = testShaderRoot + "temp/";
    directory.make_preferred();

    if (std::filesystem::exists(directory)) {
        std::filesystem::remove_all(directory);
    }
    std::filesystem::create_directory(directory);
    directory /= std::filesystem::u8path("\xf0\x9d\x95\xb7\xf0\x9d\x96\x9a\xf0\x9d\x96\x92\xf0\x9d\x96\x8a");
    directory.make_preferred();
    std::filesystem::create_directory(directory);

    auto sourceFile = directory / std::filesystem::u8path("\x6b\xc3\xa4\xc3\xa4\x6e\x74\x79\x79\x6b\xc3\xb6.frag");
    {
        std::ofstream outFile(sourceFile);
        if (outFile.is_open()) {
            outFile << SHADER_SOURCE;
        }
    }
    {
        auto includeFile = directory / std::filesystem::u8path("test_inc.h");
        std::ofstream outFile(includeFile);
        if (outFile.is_open()) {
            outFile << SHADER_INCLUDE;
        }
    }
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { sourceFile.u8string() },
        { "--include" },
        { directory.u8string() },
        { "--destination" },
        { directory.u8string() },
    };
    EXPECT_EQ(RunMain(argv), 0);
    directory = directory.parent_path();
    std::filesystem::remove_all(directory);
}

TEST(spirv_cross_helper_structs_gles, ShaderKindToStageFlags)
{
    {
        auto testEnum = ShaderKind::COMPUTE;
        auto ret = ShaderKindToStageFlags(testEnum);
        ASSERT_EQ(ret, ShaderStageFlagBits::COMPUTE_BIT);
    }

    {
        auto testEnum = ShaderKind::VERTEX;
        auto ret = ShaderKindToStageFlags(testEnum);
        ASSERT_EQ(ret, ShaderStageFlagBits::VERTEX_BIT);
        uint32_t ReVertex = static_cast<uint32_t>(~ret);
        uint32_t correctRes = 0xFFFFFFFF - 1;
        ASSERT_EQ(ReVertex, correctRes);
    }
    {
        auto testEnum = ShaderKind::FRAGMENT;
        auto ret = ShaderKindToStageFlags(testEnum);
        ASSERT_EQ(ret, ShaderStageFlagBits::FRAGMENT_BIT);
    }
    {
        auto testEnum = ShaderKind::GEOMETRY;
        auto ret = ShaderKindToStageFlags(testEnum);
        ASSERT_EQ(ret, ShaderStageFlagBits::ALL_GRAPHICS);
    }
}

TEST(spirv_cross_helper_structs_gles, ShaderStageFlags)
{
    ShaderStageFlagBits vertexBit = ShaderStageFlagBits ::VERTEX_BIT;   // VERTEX_BIT = 0x00000001,
    ShaderStageFlagBits fragBit = ShaderStageFlagBits ::FRAGMENT_BIT;   // FRAGMENT_BIT= 0x00000010,
    ShaderStageFlagBits computeBit = ShaderStageFlagBits ::COMPUTE_BIT; // COMPUTE_BIT= 0x00000020,
    ShaderStageFlags vertexFlag = ShaderStageFlags(vertexBit);
    ShaderStageFlags fragFlag = ShaderStageFlags(fragBit);
    ShaderStageFlags graphicsFlag = ShaderStageFlags(ShaderStageFlagBits ::ALL_GRAPHICS);
    bool eq = vertexFlag == ShaderStageFlagBits ::FRAGMENT_BIT;
    ASSERT_FALSE(eq);

    vertexFlag &= ShaderStageFlagBits ::ALL_GRAPHICS;
    fragFlag &= ShaderStageFlagBits ::ALL_GRAPHICS;

    ASSERT_TRUE(vertexFlag == ShaderStageFlagBits::VERTEX_BIT);
    ASSERT_TRUE(fragFlag == ShaderStageFlagBits::FRAGMENT_BIT);

    vertexFlag |= ShaderStageFlagBits ::ALL_GRAPHICS;
    fragFlag |= ShaderStageFlagBits ::ALL_GRAPHICS;
    ASSERT_EQ(vertexFlag, ShaderStageFlagBits ::ALL_GRAPHICS);
    ASSERT_EQ(fragFlag, ShaderStageFlagBits ::ALL_GRAPHICS);
    fragFlag |= graphicsFlag;
    ASSERT_EQ(fragFlag, ShaderStageFlagBits ::ALL_GRAPHICS);

    ShaderStageFlags computeFlag = ShaderStageFlags(computeBit);
    computeFlag &= ShaderStageFlagBits ::ALL_GRAPHICS;
    ASSERT_EQ(computeFlag, static_cast<ShaderStageFlagBits>(0));
    ShaderStageFlags allFlag = ShaderStageFlags(ShaderStageFlagBits ::ALL);
    allFlag = ~allFlag;
    ASSERT_EQ(allFlag, static_cast<ShaderStageFlagBits>(0x80000000));
}

TEST(FileMonitor, RemovePath)
{
    ige::FileMonitor fileMonitor;
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::filesystem::path shaderSourcesPath = testShaderRoot;
    std::filesystem::path shaderSourcesFile = testShaderRoot + "core3d_dm_depth_modify4Test.frag";

    ASSERT_FALSE(fileMonitor.RemovePath(shaderSourcesPath.string()));
    ASSERT_TRUE(fileMonitor.AddPath(shaderSourcesPath.string()));
    ASSERT_TRUE(fileMonitor.RemovePath(shaderSourcesPath.string()));
    ASSERT_TRUE(fileMonitor.AddPath(shaderSourcesFile.string()));
    ASSERT_TRUE(fileMonitor.RemovePath(shaderSourcesFile.string()));
    auto ret = fileMonitor.GetMonitoredFiles();
    ASSERT_EQ(ret.size(), 0);
}

TEST(FileMonitor, AddPath)
{
    ige::FileMonitor fileMonitor;
    const std::filesystem::path relavitveShaderSourcesPath = "./";
    const std::filesystem::path relavitveShaderSourcesPath2 = "../";
    const std::filesystem::path shaderSourcesPath = gFoldBinder.GetTestShaderRoot();
    const std::filesystem::path shaderSourcesFile =
        gFoldBinder.GetTestShaderRoot() + "core3d_dm_depth_modify4Test.frag";
    ASSERT_TRUE(fileMonitor.AddPath(relavitveShaderSourcesPath2.string()));
    ASSERT_FALSE(fileMonitor.AddPath(relavitveShaderSourcesPath.string()));
    ASSERT_FALSE(fileMonitor.AddPath(relavitveShaderSourcesPath2.string()));
    auto ret = fileMonitor.GetMonitoredFiles();

    ige::FileMonitor fileMonitor2;
    ASSERT_TRUE(fileMonitor2.AddPath(relavitveShaderSourcesPath.string()));
    ASSERT_FALSE(fileMonitor2.AddPath(relavitveShaderSourcesPath2.string()));
    ASSERT_TRUE(fileMonitor2.AddPath(shaderSourcesPath.string()));
    ASSERT_FALSE(fileMonitor2.AddPath(shaderSourcesFile.string()));
    ASSERT_FALSE(fileMonitor2.AddPath(""));
    auto ret2 = fileMonitor2.GetMonitoredFiles();
    ASSERT_NE(ret2.size(), 0);
}

TEST(FileMonitor, ScanModifications)
{
    ige::FileMonitor fileMonitor;
    std::filesystem::path shaderSourcesPath = "./temp/";

    if (std::filesystem::exists(shaderSourcesPath)) {
        std::filesystem::remove_all(shaderSourcesPath);
    }

    std::filesystem::create_directory(shaderSourcesPath);

    ASSERT_TRUE(fileMonitor.AddPath(shaderSourcesPath.string()));
    std::vector<std::string> addedFiles, removedFiles, modifiedFiles;
    EXPECT_NO_THROW(fileMonitor.ScanModifications(addedFiles, removedFiles, modifiedFiles));

    shaderSourcesPath /= "testFolder";
    std::filesystem::create_directory(shaderSourcesPath);

    shaderSourcesPath /= "example.frag";
    {
        std::ofstream outFile(shaderSourcesPath);
        if (outFile.is_open()) {
            outFile << "hello...";
        }
    }
    EXPECT_NO_THROW(fileMonitor.ScanModifications(addedFiles, removedFiles, modifiedFiles));
    EXPECT_EQ(addedFiles.size(), 1);

    std::this_thread::sleep_for(std::chrono::seconds(1)); // filesystem timestamps have 1 second precision
    {
        std::ofstream outFile(shaderSourcesPath);
        if (outFile.is_open()) {
            outFile << "#version 460 core\
#extension GL_ARB_separate_shader_objects : enable\
#extension GL_ARB_shading_language_420pack : enable ";
        }
    }
    EXPECT_NO_THROW(fileMonitor.ScanModifications(addedFiles, removedFiles, modifiedFiles));
    EXPECT_EQ(modifiedFiles.size(), 1);

    std::this_thread::sleep_for(std::chrono::seconds(1)); // filesystem timestamps have 1 second precision
    {
        shaderSourcesPath.replace_filename(std::filesystem::u8path(
            "\xe6\x8a\x98\xe5\x8f\xa0\xe5\xb1\x8f\xe6\xa1\x8c\xe9\x9d\xa2\xe6\x97\x8b\xe8\xbd\xac.frag"));
        std::ofstream outFile(shaderSourcesPath);
        if (outFile.is_open()) {
            outFile << "hello...";
        }
    }
    EXPECT_NO_THROW(fileMonitor.ScanModifications(addedFiles, removedFiles, modifiedFiles));
    EXPECT_EQ(addedFiles.size(), 2);

    std::this_thread::sleep_for(std::chrono::seconds(1)); // filesystem timestamps have 1 second precision
    {
        std::ofstream outFile(shaderSourcesPath);
        if (outFile.is_open()) {
            outFile << "#version 460 core\
#extension GL_ARB_separate_shader_objects : enable\
#extension GL_ARB_shading_language_420pack : enable ";
        }
    }
    EXPECT_NO_THROW(fileMonitor.ScanModifications(addedFiles, removedFiles, modifiedFiles));
    EXPECT_EQ(modifiedFiles.size(), 2);

    std::filesystem::remove_all("./temp");
    EXPECT_NO_THROW(fileMonitor.ScanModifications(addedFiles, removedFiles, modifiedFiles));
    EXPECT_EQ(removedFiles.size(), 2);
}

TEST(Recompilation, CheckMetaDataUsed)
{
    std::string testFolder = "./testFolder0";

    if (std::filesystem::exists(testFolder)) {
        std::filesystem::remove_all(testFolder);
    }
    std::filesystem::create_directory(testFolder);

    std::filesystem::path aSourcesFile = testFolder + "/a.glsl";
    std::filesystem::path bSourcesFile = testFolder + "/b.glsl";
    std::filesystem::path shaderSourcesFile = testFolder + "/monitorTest.frag";
    std::filesystem::path metaSourcesFile = shaderSourcesFile;
    metaSourcesFile += ".meta";

    std::filesystem::path shaderSpvFile = testFolder + "/monitorTest.frag.spv";

    std::ofstream shaderFile(shaderSourcesFile);
    if (shaderFile.is_open()) {
        shaderFile << "#version 460 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
#include \"a.glsl\"\n \
            void   main(void){ test(); }";
        shaderFile.close();
    }

    std::ofstream aFile(aSourcesFile);
    if (aFile.is_open()) {
        aFile << "#include \"b.glsl\"";
        aFile.close();
    }

    std::ofstream bFile(bSourcesFile);
    if (bFile.is_open()) {
        bFile << "void test() {}";
        bFile.close();
    }

    std::ofstream metaFile(metaSourcesFile);
    if (metaFile.is_open()) {
        metaFile << ".\\testFolder0\\monitorTest.frag:"
                 << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::filesystem::last_write_time(shaderSourcesFile).time_since_epoch())
                        .count()
                 << std::endl;
        metaFile << ".\\testFolder0\\a.glsl:"
                 << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::filesystem::last_write_time(aSourcesFile).time_since_epoch())
                        .count()
                 << std::endl;
        metaFile << ".\\testFolder0\\b.glsl:"
                 << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::filesystem::last_write_time(bSourcesFile).time_since_epoch())
                        .count()
                 << std::endl;
        metaFile.close();
    }

    std::thread taskThread0([&]() {
        std::vector<std::string> argv = { { "LumeShaderCompiler" }, { "--sourceFile" }, { shaderSourcesFile.string() },
            { "--include" }, { testFolder + "/" }, { "--check-if-changed" } };
        RunMain(argv);
    });
    taskThread0.detach();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    GTEST_ASSERT_EQ(std::filesystem::exists(shaderSpvFile), false);
    pool.push_back(std::move(taskThread0));
}

TEST(Recompilation, CheckMetaDataIgnored)
{
    std::string testFolder = "./testFolder3";

    if (std::filesystem::exists(testFolder)) {
        std::filesystem::remove_all(testFolder);
    }
    std::filesystem::create_directory(testFolder);

    std::filesystem::path aSourcesFile = testFolder + "/a.glsl";
    std::filesystem::path bSourcesFile = testFolder + "/b.glsl";
    std::filesystem::path shaderSourcesFile = testFolder + "/monitorTest.frag";
    std::filesystem::path metaSourcesFile =
        std::filesystem::path{ std::string{ testFolder + "/monitorTest.frag" } }.replace_extension(".meta");

    std::filesystem::path shaderSpvFile = testFolder + "/monitorTest.frag.spv";

    std::ofstream shaderFile(shaderSourcesFile);
    if (shaderFile.is_open()) {
        shaderFile << "#version 460 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
#include \"a.glsl\"\n \
            void   main(void){ test(); }";
        shaderFile.close();
    }

    std::ofstream aFile(aSourcesFile);
    if (aFile.is_open()) {
        aFile << "#include \"b.glsl\"";
        aFile.close();
    }

    std::ofstream bFile(bSourcesFile);
    if (bFile.is_open()) {
        bFile << "void test() {}";
        bFile.close();
    }

    std::ofstream metaFile(metaSourcesFile);
    if (metaFile.is_open()) {
        metaFile << ".\\testFolder0\\monitorTest.frag:"
                 << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::filesystem::last_write_time(shaderSourcesFile).time_since_epoch())
                        .count()
                 << std::endl;
        metaFile << ".\\testFolder0\\a.glsl:"
                 << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::filesystem::last_write_time(aSourcesFile).time_since_epoch())
                        .count()
                 << std::endl;
        metaFile << ".\\testFolder0\\b.glsl:"
                 << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::filesystem::last_write_time(bSourcesFile).time_since_epoch())
                        .count()
                 << std::endl;
        metaFile.close();
    }

    std::thread taskThread0([&]() {
        std::vector<std::string> argv = { { "LumeShaderCompiler" }, { "--sourceFile" }, { shaderSourcesFile.string() },
            { "--include" }, { testFolder + "/" } };
        RunMain(argv);
    });
    taskThread0.detach();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    GTEST_ASSERT_EQ(std::filesystem::exists(shaderSpvFile), true);
    pool.push_back(std::move(taskThread0));
}

TEST(Recompilation, CheckMetaDataRegenerated)
{
    std::string testFolder = "./testFolder2";

    if (std::filesystem::exists(testFolder)) {
        std::filesystem::remove_all(testFolder);
    }
    std::filesystem::create_directory(testFolder);

    std::filesystem::path aSourcesFile = testFolder + "/a.glsl";
    std::filesystem::path bSourcesFile = testFolder + "/b.glsl";
    std::filesystem::path shaderSourcesFile = testFolder + "/monitorTest.frag";
    std::filesystem::path metaSourcesFile =
        std::filesystem::path{ std::string{ testFolder + "/monitorTest.frag" } }.replace_extension(".meta");
    std::filesystem::path shaderSpvFile = testFolder + "/monitorTest.frag.spv";

    std::ofstream shaderFile(shaderSourcesFile);
    if (shaderFile.is_open()) {
        shaderFile << "#version 460 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
#include \"a.glsl\"\n \
            void   main(void){ test(); }";
        shaderFile.close();
    }

    std::ofstream aFile(aSourcesFile);
    if (aFile.is_open()) {
        aFile << "#include \"b.glsl\"";
        aFile.close();
    }

    std::ofstream bFile(bSourcesFile);
    if (bFile.is_open()) {
        bFile << "void test() {}";
        bFile.close();
    }

    std::thread taskThread0([&]() {
        std::vector<std::string> argv = { { "LumeShaderCompiler" }, { "--sourceFile" }, { shaderSourcesFile.string() },
            { "--include" }, { testFolder + "/" },

            { "--monitor" }, { "--check-if-changed" } };
        RunMain(argv);
    });
    taskThread0.detach();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    GTEST_ASSERT_EQ(std::filesystem::exists(shaderSpvFile), true);
    pool.push_back(std::move(taskThread0));
}

TEST(FileMonitor, sourceFile)
{
    std::string testFolder = "./testFolder0";

    if (std::filesystem::exists(testFolder)) {
        std::filesystem::remove_all(testFolder);
    }
    std::filesystem::create_directory(testFolder);

    std::filesystem::path shaderSourcesFile = testFolder + "/monitorTest.frag";
    std::ofstream shaderFile(shaderSourcesFile);
    if (shaderFile.is_open()) {
        shaderFile << "#version 460 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
            void   main(void){}";
        shaderFile.close();
    }

    std::thread taskThread0([&]() {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--sourceFile" },
            { shaderSourcesFile.string() },
            { "--monitor" },
        };
        RunMain(argv);
    });
    taskThread0.detach();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    // add shader
    std::ofstream outFile(testFolder + "/monitorTest2.frag");
    if (outFile.is_open()) {
        outFile << "#version 460 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
            void   main(void){}";
        outFile.close();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    // modify shader
    std::ofstream shaderFile2(shaderSourcesFile);
    if (shaderFile2.is_open()) {
        shaderFile2 << "#version 450 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
            void   main(void){}";
        shaderFile2.close();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // remove shader
    std::filesystem::remove(testFolder + "/monitorTest2.frag");

    pool.push_back(std::move(taskThread0));
}

TEST(FileMonitor, sourceFolder)
{
    std::string testFolder = "./testFolder1";

    if (std::filesystem::exists(testFolder)) {
        std::filesystem::remove_all(testFolder);
    }
    std::filesystem::create_directory(testFolder);

    std::filesystem::path shaderSourcesFile = testFolder + "/monitorTest.frag";
    std::ofstream shaderFile(shaderSourcesFile);
    if (shaderFile.is_open()) {
        shaderFile << "#version 460 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
            void   main(void){}";
        shaderFile.close();
    }

    std::thread taskThread0([&]() {
        std::vector<std::string> argv = {
            { "LumeShaderCompiler" },
            { "--source" },
            { testFolder },
            { "--monitor" },
        };
        RunMain(argv);
    });
    taskThread0.detach();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    // add shader
    std::ofstream outFile(testFolder + "/monitorTest2.frag");
    if (outFile.is_open()) {
        outFile << "#version 460 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
            void   main(void){}";
        outFile.close();
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    // modify shader
    std::ofstream shaderFile2(shaderSourcesFile);
    if (shaderFile2.is_open()) {
        shaderFile2 << "#version 450 core\n \
#extension GL_ARB_separate_shader_objects : enable\n \
#extension GL_ARB_shading_language_420pack : enable\n \
            void   main(void){}";
        shaderFile2.close();
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // remove shader
    std::filesystem::remove(testFolder + "/monitorTest2.frag");

    std::this_thread::sleep_for(std::chrono::seconds(3));
    pool.push_back(std::move(taskThread0));
}

TEST(ContainersArrayView, empty)
{
    array_view<bool> bool_av({});
    ASSERT_TRUE(bool_av.size() == 0);
    ASSERT_TRUE(bool_av.size_bytes() == 0);
    ASSERT_TRUE(bool_av.empty());

    array_view<short> short_av({});
    ASSERT_TRUE(short_av.size() == 0);
    ASSERT_TRUE(short_av.size_bytes() == 0);
    ASSERT_TRUE(short_av.empty());

    array_view<uint32_t> uint_av({});
    ASSERT_TRUE(uint_av.size() == 0);
    ASSERT_TRUE(uint_av.size_bytes() == 0);
    ASSERT_TRUE(uint_av.empty());

    array_view<char> char_av({});
    ASSERT_TRUE(char_av.size() == 0);
    ASSERT_TRUE(char_av.size_bytes() == 0);
    ASSERT_TRUE(char_av.empty());

    array_view<void*> p_av({});
    ASSERT_TRUE(p_av.size() == 0);
    ASSERT_TRUE(p_av.size_bytes() == 0);
    ASSERT_TRUE(p_av.empty());

    array_view<array_view<int>> av_av({});
    ASSERT_TRUE(av_av.size() == 0);
    ASSERT_TRUE(av_av.size_bytes() == 0);
    ASSERT_TRUE(av_av.empty());
}

TEST(ContainersArrayView, dataAccess)
{
    int int_data[] = { 3, 2, 1, 0 };
    array_view<int> int_av(int_data);
    ASSERT_TRUE(int_av.size() == 4);
    ASSERT_TRUE(int_av.size_bytes() == 4 * sizeof(int));
    ASSERT_FALSE(int_av.empty());

    ASSERT_TRUE(int_av.data() == int_data);
    ASSERT_TRUE(*int_av.begin() == int_data[0]);
    ASSERT_TRUE(*int_av.cbegin() == int_data[0]);
    ASSERT_TRUE(int_av[1] == int_data[1]);
    ASSERT_TRUE(int_av.at(2) == int_data[2]);
    int* endPos = int_av.end().operator->();
    const int* cendPos = int_av.cend().operator->();
    ASSERT_TRUE(*(endPos - 1) == int_data[3]);
    ASSERT_TRUE(*(cendPos - 1) == int_data[3]);
    array_view<int> int_av2(int_data, int_data + 3);
    ASSERT_TRUE(int_av2.data() == int_av.data());

    const array_view<int> cint_av(int_data);

    ASSERT_TRUE(cint_av.size() == 4);
    ASSERT_TRUE(cint_av.size_bytes() == 4 * sizeof(int));
    ASSERT_FALSE(cint_av.empty());

    ASSERT_TRUE(cint_av.data() == int_data);
    ASSERT_TRUE(*cint_av.begin() == int_data[0]);
    ASSERT_TRUE(*cint_av.cbegin() == int_data[0]);
    ASSERT_TRUE(cint_av[1] == int_data[1]);
    ASSERT_TRUE(cint_av.at(2) == int_data[2]);
    int* endPos2 = cint_av.end().operator->();
    const int* cendPos2 = cint_av.cend().operator->();
    ASSERT_TRUE(*(endPos2 - 1) == cint_av[3]);
    ASSERT_TRUE(*(cendPos2 - 1) == cint_av[3]);

    bool bool_data[] = { true, false };
    array_view<bool> bool_av(bool_data);
    ASSERT_TRUE(bool_av.size() == 2);
    ASSERT_TRUE(bool_av.size_bytes() == 2 * sizeof(bool));
    ASSERT_FALSE(bool_av.empty());

    ASSERT_TRUE(bool_av[0] == bool_data[0]);
    ASSERT_TRUE(bool_av[1] == bool_data[1]);
}

TEST(LOGGER, log_once)
{
    EXPECT_NO_THROW(LUME_ONCE_RESET());
    EXPECT_NO_THROW(LUME_LOG_ONCE_V("0", "test log once i"));
    EXPECT_NO_THROW(LUME_LOG_ONCE_V("0", "test log once i"));
    EXPECT_NO_THROW(LUME_LOG_ONCE_V("1", "test log once i"));
}

TEST(LOGGER, Get_SetLogLevel)
{
    lume::GetLogger().SetLogLevel(lume::ILogger::LogLevel::WARNING);
    ASSERT_TRUE(lume::GetLogger().GetLogLevel() == lume::ILogger::LogLevel::WARNING);
    LUME_LOG_V("test log info");
    lume::GetLogger().SetLogLevel(lume::ILogger::LogLevel::VERBOSE);
    ASSERT_TRUE(lume::GetLogger().GetLogLevel() == lume::ILogger::LogLevel::VERBOSE);
    LUME_LOG_V("test log info");
    LUME_LOG_D("test log info");
    LUME_LOG_I("test log info");
    LUME_LOG_W("test log info");
    lume::GetLogger().SetLogLevel(lume::ILogger::LogLevel::NONE);
    LUME_LOG_I("test log info");
    ASSERT_TRUE(lume::GetLogger().GetLogLevel() == lume::ILogger::LogLevel::NONE);
}

TEST(LOGGER, AddOutput)
{
    lume::GetLogger().AddOutput(nullptr);
    lume::GetLogger().AddOutput(lume::CreateLoggerFileOutput("./tempLog.txt"));
    // lume::GetLogger().AddOutput(lume::CreateLoggerFileOutput(""));
    lume::GetLogger().SetLogLevel(lume::ILogger::LogLevel::WARNING);
    LUME_LOG_I("test log info");
    lume::GetLogger().SetLogLevel(lume::ILogger::LogLevel::VERBOSE);
    LUME_LOG_I("test log info");
}

TEST(LOGGER, log_assert)
{
    EXPECT_NO_THROW(LUME_ASSERT(true));
    ASSERT_DEBUG_DEATH(
        { LUME_ASSERT(false); }, ".*"); // death in another process, doesn't count code coverage by openCppCoverage

    lume::GetLogger().SetLogLevel(lume::ILogger::LogLevel::NONE);
    ASSERT_DEBUG_DEATH(
        { LUME_ASSERT_MSG(lume::GetLogger().GetLogLevel() != lume::ILogger::LogLevel::NONE, "test logAsset(false)"); },
        ".*");
}
} // namespace UTest
