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
#include <gles/pipeline_state_object_gles.h>
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
 * @tc.name: PipelineStateObjectTestOpenGL
 * @tc.desc: Tests GLES GraphicsPipelineStateObject creation.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PipelineStateObject, PipelineStateObjectTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.createWindow = true;
    engine.backend = UTest::GetOpenGLBackend();
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);

    string_view vertUri;
    string_view fragUri;
    if (UTest::GetOpenGLBackend() == Render::DeviceBackendType::OPENGL) {
        vertUri = "rendershaders://shader/PipelineStateObjectGlesTest.vert.spv.gl";
        fragUri = "rendershaders://shader/PipelineStateObjectGlesTest.frag.spv.gl";
    } else {
        vertUri = "rendershaders://shader/PipelineStateObjectGlesTest.vert.spv.gles";
        fragUri = "rendershaders://shader/PipelineStateObjectGlesTest.frag.spv.gles";
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
        auto file =
            engine.engine->GetFileManager().OpenFile("rendershaders://shader/PipelineStateObjectGlesTest.frag.spv.lsb");
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
        auto file =
            engine.engine->GetFileManager().OpenFile("rendershaders://shader/PipelineStateObjectGlesTest.frag.spv.lsb");
        ASSERT_TRUE(file);
        const uint64_t byteLength = file->GetLength();
        fragRefData.resize(byteLength);
        ASSERT_EQ(byteLength, file->Read(fragRefData.data(), byteLength));
    }
    device.Activate();
    ShaderModuleGLES* vertShader = nullptr;
    ShaderModuleGLES* fragShader = nullptr;
    {
        ShaderReflectionData refData(vertRefData);
        ShaderModuleCreateInfo createInfo;
        createInfo.reflectionData = refData;
        createInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
        createInfo.spvData = vertSpvData;
        vertShader = new ShaderModuleGLES { device, createInfo };
        ASSERT_EQ(ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT, vertShader->GetShaderStageFlags());
    }
    {
        ShaderReflectionData refData(fragRefData);
        ShaderModuleCreateInfo createInfo;
        createInfo.reflectionData = refData;
        createInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        createInfo.spvData = fragSpvData;
        fragShader = new ShaderModuleGLES { device, createInfo };
        ASSERT_EQ(ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT, fragShader->GetShaderStageFlags());
    }
    GpuShaderProgramGLES* shader = nullptr;
    {
        GpuShaderProgramCreateData createData;
        createData.vertShaderModule = vertShader;
        createData.fragShaderModule = fragShader;
        shader = new GpuShaderProgramGLES { device, createData };
        ASSERT_EQ(vertShader, shader->GetPlatformData().vertShaderModule_);
        ASSERT_EQ(fragShader, shader->GetPlatformData().fragShaderModule_);
    }
    {
        GraphicsState graphicsState;
        PipelineLayout pipelineLayout;
        VertexInputDeclarationView vidView;
        VertexInputDeclaration::VertexInputAttributeDescription attributes[8];
        attributes[0].format = BASE_FORMAT_R32G32B32_SFLOAT;
        attributes[1].format = BASE_FORMAT_R32G32B32A32_SFLOAT;
        attributes[2].format = BASE_FORMAT_R32_SFLOAT;
        attributes[3].format = BASE_FORMAT_R8G8B8A8_UINT;
        attributes[4].format = BASE_FORMAT_R8G8B8A8_UNORM;
        attributes[5].format = BASE_FORMAT_R32G32B32A32_UINT;
        attributes[6].format = BASE_FORMAT_R32_UINT;
        attributes[7].format = BASE_FORMAT_R16G16_SFLOAT;
        for (uint32_t i = 0; i < 8; i++) {
            attributes[i].location = i;
            attributes[i].binding = i;
        }
        vidView.attributeDescriptions = { attributes, countof(attributes) };
        ShaderSpecializationConstantDataView specConsts;
        uint32_t specData[100] = { 0 };
        specConsts.data = { specData, countof(specData) };
        DynamicStateEnum dynamicStates[6];
        dynamicStates[0] = DynamicStateEnum::CORE_DYNAMIC_STATE_ENUM_SCISSOR;
        dynamicStates[1] = DynamicStateEnum::CORE_DYNAMIC_STATE_ENUM_DEPTH_BIAS;
        dynamicStates[2] = DynamicStateEnum::CORE_DYNAMIC_STATE_ENUM_LINE_WIDTH;
        dynamicStates[3] = DynamicStateEnum::CORE_DYNAMIC_STATE_ENUM_BLEND_CONSTANTS;
        dynamicStates[4] = DynamicStateEnum::CORE_DYNAMIC_STATE_ENUM_DEPTH_BOUNDS;
        dynamicStates[5] = DynamicStateEnum::CORE_DYNAMIC_STATE_ENUM_STENCIL_COMPARE_MASK;
        RenderPassDesc renderPass;
        RenderPassSubpassDesc subpass;
        GraphicsPipelineStateObjectGLES pso { device, *shader, graphicsState, pipelineLayout, vidView, specConsts,
            dynamicStates, renderPass, { &subpass, 1 } };
        OES_Bind bind;
        bind.set = 0u;
        bind.bind = 0u;
        pso.GetOESProgram({ &bind, 1U });
    }
    delete shader;
    delete vertShader;
    delete fragShader;
    device.Deactivate();
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
