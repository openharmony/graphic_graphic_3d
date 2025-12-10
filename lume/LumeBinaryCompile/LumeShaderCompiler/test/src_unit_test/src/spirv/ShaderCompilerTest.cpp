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
#if 0
        mCompiler_ = nullptr;
        spvBinary_ = {};
#endif
        std::filesystem::remove_all(testFolder_);
    }
#if 0
    int TestRefactoredMain(const std::vector<std::string>& argStrings)
    {
        const int argc = static_cast<int>(argStrings.size());
        char** argv = new char*[argc];
        for (int i = 0; i < argc; i++) {
            argv[i] = new char[argStrings[i].length() + 1];
            argv[i][argStrings[i].copy(argv[i], argStrings[i].length())] = '\0';
        }
        const std::optional<Inputs> params = Parse(argc, argv);
        for (int i = 0; i < argc; i++) {
            delete[] argv[i];
        }
        delete[] argv;

        if (!params) {
            LUME_LOG_E("Unable to parse input params");
            return -1;
        }

        ige::FileMonitor fileMonitor;

        if (!std::filesystem::exists(params->shaderSourcesPath)) {
            LUME_LOG_E("Source path does not exist: '%s'", params->shaderSourcesPath.string().c_str());
            return -1;
        }

        // Make sure the destination dir exists.
        std::filesystem::create_directories(params->compiledShaderDestinationPath);

        if (!std::filesystem::exists(params->compiledShaderDestinationPath)) {
            LUME_LOG_E("Destination path does not exist: '%s'", params->compiledShaderDestinationPath.string().c_str());
            return -1;
        }

        fileMonitor.AddPath(params->shaderSourcesPath.string());
        std::vector<std::string> fileList = [&]() {
            std::vector<std::string> list;
            if (!params->sourceFile.empty()) {
                list.push_back(params->sourceFile.u8string());
            } else {
                list = fileMonitor.getMonitoredFiles();
            }
            return list;
        }();

        const std::vector<std::string_view> supportedFileTypes = { ".vert", ".frag", ".comp", ".json" };
        fileList = FilterByExtension(fileList, supportedFileTypes);

        LUME_LOG_I("     Source path: '%s'", std::filesystem::absolute(params->shaderSourcesPath).string().c_str());
        for (auto const& path : params->shaderIncludePaths) {
            LUME_LOG_I("    Include path: '%s'", std::filesystem::absolute(path).string().c_str());
        }
        LUME_LOG_I("Destination path: '%s'",
            std::filesystem::absolute(params->compiledShaderDestinationPath).string().c_str());
        LUME_LOG_I("");
        LUME_LOG_I("Processing:");

        int errorCount = 0;
        Scope scope(glslang::InitializeProcess, glslang::FinalizeProcess);

        std::vector<std::filesystem::path> searchPath;
        searchPath.reserve(1U + params->shaderIncludePaths.size());
        searchPath.emplace_back(params->shaderSourcesPath.string());
        std::transform(params->shaderIncludePaths.cbegin(), params->shaderIncludePaths.cend(),
            std::back_inserter(searchPath), [](const std::filesystem::path& path) { return path.string(); });

        auto fileIncluder = FileIncluder(params->shaderSourcesPath, searchPath);
        auto settings = CompilationSettings { params->envVersion, searchPath, {}, params->shaderSourcesPath,
            params->compiledShaderDestinationPath, fileIncluder };

        {
            spv_target_env targetEnv = spv_target_env::SPV_ENV_VULKAN_1_0;
            switch (params->envVersion) {
                case ShaderEnv::version_vulkan_1_0:
                    targetEnv = spv_target_env::SPV_ENV_VULKAN_1_0;
                    break;
                case ShaderEnv::version_vulkan_1_1:
                    targetEnv = spv_target_env::SPV_ENV_VULKAN_1_1;
                    break;
                case ShaderEnv::version_vulkan_1_2:
                    targetEnv = spv_target_env::SPV_ENV_VULKAN_1_2;
                    break;
                case ShaderEnv::version_vulkan_1_3:
                    targetEnv = spv_target_env::SPV_ENV_VULKAN_1_3;
                    break;
                default:
                    break;
            }
            settings.optimizer.emplace(targetEnv);
        }

        // Startup compilation.
        for (auto const& file : fileList) {
            std::string relativeFilename = std::filesystem::relative(file, params->shaderSourcesPath).string();
            LUME_LOG_D("Tracked source file: '%s'", relativeFilename.c_str());
            if (!RunAllCompilationStages(file, settings, params)) {
                errorCount++;
            }
        }
        return errorCount;
    }

    bool CreateGlShader(std::filesystem::path outputFilename, bool multiviewEnabled, DeviceBackendType backendType,
        const ShaderModuleCreateInfo& info)
    {
        try {
            Shader shader;
            shader.shaderStageFlags = info.shaderStageFlags;
            shader.backend = backendType;
            shader.ovrEnabled = multiviewEnabled;
            this->ProcessShaderModule(shader, info);
            const auto* data = static_cast<const uint8_t*>(static_cast<const void*>(shader.source_.data()));
            WriteToFile(array_view(data, shader.source_.size()), outputFilename);
        } catch (std::exception const& e) {
            LUME_LOG_E("Failed to generate GL(ES) shader: %s", e.what());
            return false;
        }
        return true;
    }

    void CreateGlShaders(std::filesystem::path outputFilename, std::string_view shaderSource, ShaderKind shaderKind,
        array_view<const uint32_t> spvBinary, ShaderReflectionData reflectionData)
    {
        auto glFile = outputFilename;
        glFile += ".gl";
        const bool multiviewEnabled = UsesMultiviewExtension(shaderSource);

        const auto info = ShaderModuleCreateInfo {
            ShaderStageFlags(shaderKind),
            array_view(static_cast<const uint8_t*>(static_cast<const void*>(spvBinary.data())), spvBinary.size_bytes()),
            reflectionData,
        };

        this->CreateGlShader(glFile, multiviewEnabled, DeviceBackendType::OPENGL, info);
        glFile += "es";
        this->CreateGlShader(glFile, multiviewEnabled, DeviceBackendType::OPENGLES, info);
    }

    void ProcessShaderModule(Shader& me, const ShaderModuleCreateInfo& createInfo)
    {
        // perform reflection.

        /*    auto compiler = Gles::CoreCompiler(reinterpret_cast<const uint32_t*>(createInfo.spvData.data()),
                static_cast<uint32_t>(createInfo.spvData.size() / sizeof(uint32_t)));*/
        mCompiler_ = std::make_unique<Gles::CoreCompiler>(reinterpret_cast<const uint32_t*>(createInfo.spvData.data()),
            static_cast<uint32_t>(createInfo.spvData.size() / sizeof(uint32_t)));

        auto& compiler = *mCompiler_;

        // Set some options.
        SetupSpirvCross(me.shaderStageFlags, &compiler, me.backend, me.ovrEnabled);

        // first step in converting CORE_FLIP_NDC to regular uniform. (specializationconstant -> constant) this
        // makes the compiled glsl more readable, and simpler to post process later.
        Gles::ConvertSpecConstToConstant(compiler, "CORE_FLIP_NDC");

        auto active = compiler.get_active_interface_variables();
        const auto& res = compiler.get_shader_resources(active);
        compiler.set_enabled_interface_variables(std::move(active));

        Gles::ReflectPushConstants(compiler, res, me.plat.infos, me.shaderStageFlags);
        compiler.build_combined_image_samplers();
        CollectRes(compiler, res, me.plat);

        // set "CORE_BACKEND_TYPE" specialization to 1.
        Gles::SetSpecMacro(compiler, "CORE_BACKEND_TYPE", 1U);

        me.source_ = compiler.compile();
        Gles::ConvertConstantToUniform(compiler, me.source_, "CORE_FLIP_NDC");
    }

    bool RunAllCompilationStages(
        std::string_view inputFilename, CompilationSettings& settings, const std::optional<Inputs>& params)
    {
        try {
            const std::filesystem::path relativeInputFilename =
                std::filesystem::relative(inputFilename, settings.shaderSourcePath);
            const std::string relativeFilename = relativeInputFilename.string();
            const std::string extension = std::filesystem::path(inputFilename).extension().string();
            std::filesystem::path outputFilename = settings.compiledShaderDestinationPath / relativeInputFilename;

            // Make sure the output dir hierarchy exists.
            std::filesystem::create_directories(outputFilename.parent_path());

            ShaderKind shaderKind;

            // Just copying json files to the destination dir.
            if (extension == ".json") {
                if (!std::filesystem::exists(outputFilename) ||
                    !std::filesystem::equivalent(inputFilename, outputFilename)) {
                    LUME_LOG_I("  %s", relativeFilename.c_str());
                    std::filesystem::copy(
                        inputFilename, outputFilename, std::filesystem::copy_options::overwrite_existing);
                }
                return true;
            } else if (extension == ".vert") {
                shaderKind = ShaderKind::VERTEX;
            } else if (extension == ".frag") {
                shaderKind = ShaderKind::FRAGMENT;
            } else if (extension == ".comp") {
                shaderKind = ShaderKind::COMPUTE;
            } else {
                return false;
            }

            outputFilename += ".spv";

            LUME_LOG_I("  %s", relativeFilename.c_str());

            LUME_LOG_V("    input: '%s'", inputFilename.data());
            LUME_LOG_V("      dst: '%s'", settings.compiledShaderDestinationPath.string().c_str());
            LUME_LOG_V(" relative: '%s'", relativeFilename.c_str());
            LUME_LOG_V("   output: '%s'", outputFilename.string().c_str());

            const std::string shaderSource = ReadFileToString(inputFilename);
            if (shaderSource.empty()) {
                return false;
            }

            const std::string preProcessedShader =
                PreProcessShader(shaderSource, shaderKind, relativeFilename, settings);
            if (preProcessedShader.empty()) {
                return false;
            }
            if constexpr (false) {
                auto preprocessedFile = outputFilename;
                preprocessedFile += ".pre";
                if (!WriteToFile(array_view(preProcessedShader.data(), preProcessedShader.size()), preprocessedFile)) {
                    LUME_LOG_E("Failed to save preprocessed %s", preprocessedFile.string().data());
                }
            }

            auto spvBinary = CompileShaderToSpirvBinary(preProcessedShader, shaderKind, relativeFilename, settings);
            if (spvBinary.empty()) {
                return false;
            }

            const auto reflection = ReflectSpvBinary(spvBinary, shaderKind);
            if (reflection.empty()) {
                LUME_LOG_E("Failed to reflect %s", inputFilename.data());
            } else {
                auto reflectionFile = outputFilename;
                reflectionFile += ".lsb";
                if (!WriteToFile(array_view(reflection.data(), reflection.size()), reflectionFile)) {
                    LUME_LOG_E("Failed to save reflection %s", reflectionFile.string().data());
                }
            }

            // spirv-opt resets the passes everytime so then need to be setup
            if (params->optimizeSpirv) {
                settings.optimizer->RegisterPerformancePasses();
            }

            ::RegisterStripPreprocessorDebugInfoPass(settings.optimizer);
            if (!settings.optimizer->Run(spvBinary.data(), spvBinary.size(), &spvBinary)) {
                LUME_LOG_E("Failed to optimize %s", inputFilename.data());
            }

            // generate gl and gles shaders from optimized binary but with file names intact but with just
            // preprocessor extension directives stripped out
            this->CreateGlShaders(
                outputFilename, shaderSource, shaderKind, spvBinary, ShaderReflectionData { reflection });

            // strip out all other debug information like variable names, function names
            if (params->stripDebugInformation == true) {
                bool registerPass = settings.optimizer->RegisterPassFromFlag("--strip-debug");
                if (registerPass == false || !settings.optimizer->Run(spvBinary.data(), spvBinary.size(), &spvBinary)) {
                    LUME_LOG_E("Failed to strip debug information %s", inputFilename.data());
                }
            }

            spvBinary_ = spvBinary;
            // write the spirv-binary to disk
            /* if (!WriteToFile(array_view(spvBinary.data(), spvBinary.size()), outputFilename)) {
                 return false;
             }*/

            LUME_LOG_D("  -> %s", outputFilename.string().c_str());

            return true;
        } catch (std::exception const& e) {
            LUME_LOG_E("Processing file failed '%s': %s", inputFilename.data(), e.what());
        }
        return false;
    }

    std::vector<uint32_t> spvBinary_;
    std::unique_ptr<Gles::CoreCompiler> mCompiler_ = nullptr;
#endif
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
#if 0
TEST_F(ShaderCompilationConsistencyTest, SetSpecMacro)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "core3d_dm_depth_modify4Test.frag" },
        { "--strip-debug-information" },
    };
    ASSERT_EQ(TestRefactoredMain(argv), 0);

    Gles::CoreCompiler& compiler = *mCompiler_;
    // set "CORE_BACKEND_TYPE" specialization to 1.
    Gles::SetSpecMacro(compiler, "CORE_BACKEND_TYPE", 1U);
    Gles::SetSpecMacro(compiler, "CORE_BACKEND_NOT_Exist?", 1U);
    Gles::SetSpecMacro(compiler, "testBool", false);
}

TEST_F(ShaderCompilationConsistencyTest, ConvertSpecConstToConstant)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "core3d_dm_depth_modify4Test.frag" },
        { "--destination" },
        { testShaderRoot },

    };
    ASSERT_EQ(TestRefactoredMain(argv), 0);
    Gles::CoreCompiler& compiler = *mCompiler_;
    const char* name0 = "";
    Gles::ConvertSpecConstToConstant(compiler, name0);
    const char* name1 = "CORE_FLIP_NDC";
    Gles::ConvertSpecConstToConstant(compiler, name1);
    const char* name2 = "CORE_BACKEND_TYPE";
    Gles::ConvertSpecConstToConstant(compiler, name2);
}

TEST_F(ShaderCompilationConsistencyTest, ConvertConstantToUniform)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "core3d_dm_depth_modify4Test.frag" },
    };
    ASSERT_EQ(TestRefactoredMain(argv), 0);
    Gles::CoreCompiler& compiler = *mCompiler_;
    auto source_ = compiler.compile();
    auto ir = compiler.GetIr();
    Gles::ConvertConstantToUniform(compiler, source_, "CORE_FLIP_NDC");
    Gles::ConvertConstantToUniform(compiler, source_, "NUM_SAMPLES");
    Gles::ConvertConstantToUniform(compiler, source_, "ConstantFloat");
    Gles::ConvertConstantToUniform(compiler, source_, "testBool");
}

TEST_F(ShaderCompilationConsistencyTest, ReflectPushConstants)
{
    auto testShaderRoot = gFoldBinder.GetTestShaderRoot();
    std::vector<std::string> argv = {
        { "LumeShaderCompiler" },
        { "--sourceFile" },
        { testShaderRoot + "core3d_dm_depth_modify4Test.frag" },
    };
    ASSERT_EQ(TestRefactoredMain(argv), 0);
    // compile core3d_dm_depth_modify4Test.frag will call   Gles::ReflectPushConstants automatically
    // with  valid resource.push_constant_buffer
    // now call Gles::ReflectPushConstants  with empty parameter.
    Gles::CoreCompiler& compiler = *mCompiler_;
    auto active = compiler.get_active_interface_variables();
    const auto& res = compiler.get_shader_resources(active);
    compiler.set_enabled_interface_variables(std::move(active));

    Shader me;
    me.shaderStageFlags = ShaderStageFlags(ShaderStageFlagBits ::FRAGMENT_BIT);
    Gles::ReflectPushConstants(compiler, res, me.plat.infos, me.shaderStageFlags);
}
#endif
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
