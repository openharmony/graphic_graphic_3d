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

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#include <gles/gpu_program_gles.h>
#include <gles/shader_module_gles.h>
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND

#include <device/device.h>
#include <device/shader_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuProgramTestOpenGL
 * @tc.desc: Tests for GLES program creation by creating shader modules from vertex and fragment spir-v shaders.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuProgram, GpuProgramTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.createWindow = true;
    engine.backend = UTest::GetOpenGLBackend();
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);

    string_view vertUri;
    string_view fragUri;
    if (UTest::GetOpenGLBackend() == Render::DeviceBackendType::OPENGL) {
        vertUri = "rendershaders://shader/fullscreen_triangle.vert.spv.gl";
        fragUri = "rendershaders://shader/PipelineDescriptorSetBinderTest2.frag.spv.gl";
    } else {
        vertUri = "rendershaders://shader/fullscreen_triangle.vert.spv.gles";
        fragUri = "rendershaders://shader/PipelineDescriptorSetBinderTest2.frag.spv.gles";
    }

    vector<uint8_t> vertSpvData;
    {
        auto file = engine.engine->GetFileManager().OpenFile(vertUri);
        ASSERT_TRUE(file);
        const uint64_t byteLength = file->GetLength();
        vertSpvData.resize(byteLength);
        ASSERT_EQ(byteLength, file->Read(vertSpvData.data(), byteLength));
    }
    vector<uint8_t> vertRefData;
    {
        auto file = engine.engine->GetFileManager().OpenFile("rendershaders://shader/fullscreen_triangle.vert.spv.lsb");
        ASSERT_TRUE(file);
        const uint64_t byteLength = file->GetLength();
        vertRefData.resize(byteLength);
        ASSERT_EQ(byteLength, file->Read(vertRefData.data(), byteLength));
    }
    vector<uint8_t> fragSpvData;
    {
        auto file = engine.engine->GetFileManager().OpenFile(fragUri);
        ASSERT_TRUE(file);
        const uint64_t byteLength = file->GetLength();
        fragSpvData.resize(byteLength);
        ASSERT_EQ(byteLength, file->Read(fragSpvData.data(), byteLength));
    }
    vector<uint8_t> fragRefData;
    {
        auto file = engine.engine->GetFileManager().OpenFile(
            "rendershaders://shader/PipelineDescriptorSetBinderTest2.frag.spv.lsb");
        ASSERT_TRUE(file);
        const uint64_t byteLength = file->GetLength();
        fragRefData.resize(byteLength);
        ASSERT_EQ(byteLength, file->Read(fragRefData.data(), byteLength));
    }
    device.Activate();
    BASE_NS::unique_ptr<ShaderModuleGLES> vertShader;
    BASE_NS::unique_ptr<ShaderModuleGLES> fragShader;
    {
        ShaderReflectionData refData(vertRefData);
        ShaderModuleCreateInfo createInfo;
        createInfo.reflectionData = refData;
        createInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
        createInfo.spvData = vertSpvData;
        vertShader = BASE_NS::make_unique<ShaderModuleGLES>(device, createInfo);
        ASSERT_EQ(ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT, vertShader->GetShaderStageFlags());
    }
    {
        ShaderReflectionData refData(fragRefData);
        ShaderModuleCreateInfo createInfo;
        createInfo.reflectionData = refData;
        createInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        createInfo.spvData = fragSpvData;
        fragShader = BASE_NS::make_unique<ShaderModuleGLES>(device, createInfo);
        ASSERT_EQ(ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT, fragShader->GetShaderStageFlags());
    }
    {
        GpuShaderProgramCreateData createData;
        createData.vertShaderModule = vertShader.get();
        createData.fragShaderModule = fragShader.get();
        GpuShaderProgramGLES shader { device, createData };
        ASSERT_EQ(vertShader.get(), shader.GetPlatformData().vertShaderModule_);
        ASSERT_EQ(fragShader.get(), shader.GetPlatformData().fragShaderModule_);
        OES_Bind bind;
        bind.bind = 2u;
        bind.set = 0u;
        auto patch = shader.OesPatch({ &bind, 1 }, 1);
        ASSERT_NE(nullptr, patch);
    }
    device.Deactivate();
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
