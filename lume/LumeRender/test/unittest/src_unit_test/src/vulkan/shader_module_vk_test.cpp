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

#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/shader_module_vk.h>
#endif // RENDER_HAS_VULKAN_BACKEND

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

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: ShaderModuleTestVulkan
 * @tc.desc: Tests Vulkan creation of spir-v shader modules with reflection data from a file.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderModule, ShaderModuleTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);
    vector<uint8_t> spvData;
    {
        auto file =
            engine.engine->GetFileManager().OpenFile("rendershaders://shader/ShaderPipelineBinderTest.frag.spv");
        ASSERT_TRUE(file);
        const uint64_t byteLength = file->GetLength();
        spvData.resize(byteLength);
        ASSERT_EQ(byteLength, file->Read(spvData.data(), byteLength));
    }
    vector<uint8_t> rawRefData;
    {
        auto file =
            engine.engine->GetFileManager().OpenFile("rendershaders://shader/ShaderPipelineBinderTest.frag.spv.lsb");
        ASSERT_TRUE(file);
        const uint64_t byteLength = file->GetLength();
        rawRefData.resize(byteLength);
        ASSERT_EQ(byteLength, file->Read(rawRefData.data(), byteLength));
    }
    device.Activate();
    {
        ShaderReflectionData refData(rawRefData);
        ShaderModuleCreateInfo createInfo;
        createInfo.reflectionData = refData;
        createInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
        createInfo.spvData = spvData;
        auto shader = device.CreateComputeShaderModule(createInfo);
        ASSERT_EQ(ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT, shader->GetShaderStageFlags());
    }
    {
        ShaderReflectionData refData(rawRefData);
        ShaderModuleCreateInfo createInfo;
        createInfo.reflectionData = refData;
        createInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL;
        createInfo.spvData = spvData;
        ShaderModuleVk shader { device, createInfo };
        ASSERT_EQ(ShaderStageFlagBits::CORE_SHADER_STAGE_ALL, shader.GetShaderStageFlags());
    }
    device.Deactivate();
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND
