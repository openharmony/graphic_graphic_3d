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

#include <datastore/render_data_store_default_acceleration_structure_staging.h>
#include <datastore/render_data_store_default_gpu_resource_data_copy.h>
#include <datastore/render_data_store_manager.h>
#include <datastore/render_data_store_pod.h>
#include <device/shader_manager.h>

#include <base/util/base64_decode.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/datastore/intf_render_data_store_default_acceleration_structure_staging.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_shader_passes.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/resource_handle.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

#if RENDER_HAS_VULKAN_BACKEND
#include <render/vulkan/intf_device_vk.h>
#endif
#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#include <render/gles/intf_device_gles.h>
#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

RENDER_BEGIN_NAMESPACE()
bool operator==(const DescriptorSetLayoutBinding& lhs, const DescriptorSetLayoutBinding& rhs) noexcept
{
    return (lhs.binding == rhs.binding) && (lhs.descriptorType == rhs.descriptorType) &&
           (lhs.descriptorCount == rhs.descriptorCount) && (lhs.shaderStageFlags == rhs.shaderStageFlags) &&
           (lhs.additionalDescriptorTypeFlags == rhs.additionalDescriptorTypeFlags);
}

bool operator==(const DescriptorSetLayout& lhs, const DescriptorSetLayout& rhs) noexcept
{
    return (lhs.set == rhs.set) && (lhs.bindings.size() == rhs.bindings.size()) &&
           std::equal(lhs.bindings.cbegin(), lhs.bindings.cend(), rhs.bindings.cbegin());
}
RENDER_END_NAMESPACE()

namespace {
void Validate(const UTest::EngineResources& er)
{
    IShaderManager& shaderMgr = er.context->GetDevice().GetShaderManager();
    {
        uint32_t slotId = shaderMgr.CreateRenderSlotId("Slot0");
        ASSERT_NE(INVALID_SM_INDEX, slotId);

        uint32_t sameSlotId = shaderMgr.CreateRenderSlotId("Slot0");
        ASSERT_EQ(slotId, sameSlotId);
        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId("Slot0"));
        ASSERT_EQ(INVALID_SM_INDEX, shaderMgr.GetRenderSlotId("NonExistingSlot"));

        string path = "state0";
        GraphicsState graphicsState {};
        IShaderManager::GraphicsStateCreateInfo stateInfo { path, graphicsState };
        IShaderManager::GraphicsStateVariantCreateInfo variantInfo {};
        variantInfo.renderSlot = "Slot0";
        auto stateHandle = shaderMgr.CreateGraphicsState(stateInfo, variantInfo);
        ASSERT_NE(RenderHandle {}, stateHandle.GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(path).GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(stateHandle, slotId).GetHandle());
        ASSERT_EQ(RenderHandle {}, shaderMgr.GetGraphicsStateHandle("NonExistngState").GetHandle());

        IShaderManager::ShaderCreateInfo shaderInfo {};
        shaderInfo.path = "shaders://shader/GfxBackBufferRenderNodeTest.shader";
        IShaderManager::ShaderModulePath vertShader {};
        vertShader.path = "rendershaders://shader/fullscreen_triangle.vert.spv";
        vertShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
        IShaderManager::ShaderModulePath fragShader {};
        fragShader.path = "rendershaders://shader/GfxBackBufferRenderNodeTest.frag.spv";
        fragShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        IShaderManager::ShaderModulePath shaderModules[] = { vertShader, fragShader };
        shaderInfo.shaderPaths = { shaderModules, countof(shaderModules) };
        shaderInfo.graphicsState = stateHandle.GetHandle();
        shaderInfo.renderSlotId = slotId;
        auto shaderHandle = shaderMgr.CreateShader(shaderInfo);
        ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
        ASSERT_FALSE(shaderMgr.IsComputeShader(shaderHandle));
        ASSERT_TRUE(shaderMgr.IsShader(shaderHandle));

        const IShaderManager::PipelineLayoutCreateInfo plInfo {
            "pipelinelayouts://PipelineLayoutLoaderTest.shaderpl",
            {},
            0U,
            true,
        };
        auto plHandle = shaderMgr.CreatePipelineLayout(plInfo);

        const IShaderManager::VertexInputDeclarationCreateInfo vidInfo {
            "shaders://shader/VertexInputDeclarationLoaderTest.shader",
            {},
            0U,
            true,
        };
        auto vidHandle = shaderMgr.CreateVertexInputDeclaration(vidInfo);

        shaderMgr.SetRenderSlotData({ slotId, shaderHandle, stateHandle, plHandle, vidHandle });
        auto slotData = shaderMgr.GetRenderSlotData(slotId);
        ASSERT_EQ(slotData.renderSlotId, slotId);
        ASSERT_EQ(slotData.shader.GetHandle(), shaderHandle.GetHandle());
        ASSERT_EQ(slotData.graphicsState.GetHandle(), stateHandle.GetHandle());

        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId(shaderHandle));
        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId(stateHandle));

        auto idDesc = shaderMgr.GetIdDesc(stateHandle);
        ASSERT_EQ(path, idDesc.path);
        idDesc = shaderMgr.GetIdDesc(shaderHandle);
        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId(idDesc.renderSlot));

        ((ShaderManager&)shaderMgr).AddAdditionalNameForHandle(shaderHandle, "shader0");
        ASSERT_EQ(shaderHandle.GetHandle(), shaderMgr.GetShaderHandle("shader0").GetHandle());
        ASSERT_EQ(shaderHandle.GetHandle(), shaderMgr.GetShaderHandle("shader0", "").GetHandle());
        ASSERT_EQ(
            shaderHandle.GetHandle(), ((ShaderManager&)shaderMgr).GetShaderHandle(shaderHandle, slotId).GetHandle());

        auto shaders = shaderMgr.GetShaders(slotId);
        ASSERT_EQ(1, shaders.size());
        ASSERT_EQ(shaderHandle.GetHandle(), shaders[0].GetHandle());
        auto rawShaders = ((ShaderManager&)shaderMgr).GetShaderRawHandles(slotId);
        ASSERT_EQ(1, rawShaders.size());
        ASSERT_EQ(shaderHandle.GetHandle(), rawShaders[0]);

        graphicsState = shaderMgr.GetGraphicsState(stateHandle);
        auto stateHash = shaderMgr.HashGraphicsState(graphicsState, slotId);
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandleByHash(stateHash).GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle).GetHandle());
        ASSERT_EQ(
            RenderHandle {}, shaderMgr.GetGraphicsStateHandleByShaderHandle(RenderHandleReference {}).GetHandle());
        ASSERT_EQ(graphicsState.colorBlendState.colorAttachmentCount,
            ((ShaderManager&)shaderMgr).GetGraphicsStateRef(stateHandle).colorBlendState.colorAttachmentCount);
        ASSERT_EQ(
            GraphicsState {}.colorBlendState.colorAttachmentCount, ((ShaderManager&)shaderMgr)
                                                                       .GetGraphicsStateRef(RenderHandleReference {})
                                                                       .colorBlendState.colorAttachmentCount);
    }
    {
        uint32_t slotId = shaderMgr.CreateRenderSlotId("Slot1");
        ASSERT_NE(INVALID_SM_INDEX, slotId);

        string path = "state1";
        GraphicsState graphicsState {};
        IShaderManager::GraphicsStateCreateInfo stateInfo { path, graphicsState };
        IShaderManager::GraphicsStateVariantCreateInfo variantInfo {};
        variantInfo.renderSlot = "Slot1";
        auto stateHandle = shaderMgr.CreateGraphicsState(stateInfo, variantInfo);
        ASSERT_NE(RenderHandle {}, stateHandle.GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(path).GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(stateHandle, slotId).GetHandle());

        IShaderManager::ComputeShaderCreateInfo shaderInfo {};
        shaderInfo.path = "shaders://computeshader/GfxGpuBufferRenderNodeTest.shader";
        IShaderManager::ShaderModulePath compShader {};
        compShader.path = "rendershaders://computeshader/GfxGpuBufferRenderNodeTest.comp.spv";
        compShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
        IShaderManager::ShaderModulePath shaderModules[] = { compShader };
        shaderInfo.shaderPaths = { shaderModules, countof(shaderModules) };
        shaderInfo.renderSlotId = slotId;
        auto shaderHandle = shaderMgr.CreateComputeShader(shaderInfo);
        ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
        ASSERT_TRUE(shaderMgr.IsComputeShader(shaderHandle));
        ASSERT_FALSE(shaderMgr.IsShader(shaderHandle));

        shaderMgr.SetRenderSlotData({ slotId, shaderHandle, stateHandle });
        auto slotData = shaderMgr.GetRenderSlotData(slotId);
        ASSERT_EQ(slotData.renderSlotId, slotId);
        ASSERT_EQ(slotData.shader.GetHandle(), shaderHandle.GetHandle());
        ASSERT_EQ(slotData.graphicsState.GetHandle(), stateHandle.GetHandle());

        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId(shaderHandle));
        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId(stateHandle));

        auto invalidSlotData = shaderMgr.GetRenderSlotData(421414u);
        ASSERT_EQ(IShaderManager::RenderSlotData {}.renderSlotId, invalidSlotData.renderSlotId);

        ASSERT_EQ(RenderHandle {}, shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(shaderHandle).GetHandle());
        ASSERT_EQ(RenderHandle {}, shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(stateHandle).GetHandle());

        auto idDesc = shaderMgr.GetIdDesc(shaderHandle);
        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId(idDesc.renderSlot));

        // comparison to self should always be true
        ASSERT_TRUE(IShaderManager::CompatibilityFlagBits::EXACT_BIT &
                    shaderMgr.GetCompatibilityFlags(shaderHandle, shaderHandle));

        shaderMgr.Destroy(shaderHandle);
        ((ShaderManager&)shaderMgr).HandlePendingAllocations();
        ASSERT_EQ(RenderHandle {},
            shaderMgr.GetShaderHandle("shaders://computeshader/GfxGpuBufferRenderNodeTest.shader").GetHandle());

        ASSERT_EQ(0, shaderMgr.GetCompatibilityFlags({}, {}));
    }
    {
        shaderMgr.ReloadShaderFile("shaders://computeshader/GfxGpuBufferRenderNodeTest.shader");

        ASSERT_EQ(nullptr, shaderMgr.GetMaterialMetadata({}));

        string vertexInputPath = "vertexInput0";
        VertexInputDeclarationView vertexInputView;
        vertexInputView.attributeDescriptions = {};
        vertexInputView.bindingDescriptions = {};
        IShaderManager::VertexInputDeclarationCreateInfo inputInfo { vertexInputPath, vertexInputView };
        auto oldInputHandle = shaderMgr.CreateVertexInputDeclaration(inputInfo);
        ASSERT_NE(RenderHandle {}, oldInputHandle.GetHandle());
        auto inputHandle = shaderMgr.CreateVertexInputDeclaration(inputInfo);
        ASSERT_NE(RenderHandle {}, inputHandle.GetHandle());
        ASSERT_EQ(RenderHandleUtil::GetIndexPart(oldInputHandle.GetHandle()),
            RenderHandleUtil::GetIndexPart(inputHandle.GetHandle()));
        ASSERT_NE(RenderHandleUtil::GetGenerationIndexPart(oldInputHandle.GetHandle()),
            RenderHandleUtil::GetGenerationIndexPart(inputHandle.GetHandle()));

        auto declView = shaderMgr.GetVertexInputDeclarationView(inputHandle);
        ASSERT_EQ(vertexInputView.attributeDescriptions.size(), declView.attributeDescriptions.size());
        ASSERT_EQ(vertexInputView.bindingDescriptions.size(), declView.bindingDescriptions.size());
        auto idDesc = shaderMgr.GetIdDesc(inputHandle);
        ASSERT_EQ("vertexInput0", idDesc.path);

        uint32_t slotId = shaderMgr.CreateRenderSlotId("Slot0");
        ASSERT_NE(INVALID_SM_INDEX, slotId);

        string path = "state7";
        GraphicsState graphicsState {};
        IShaderManager::GraphicsStateCreateInfo stateInfo { path, graphicsState };
        IShaderManager::GraphicsStateVariantCreateInfo variantInfo {};
        variantInfo.renderSlot = "Slot0";
        auto stateHandle = shaderMgr.CreateGraphicsState(stateInfo, variantInfo);
        variantInfo.baseShaderState = "state1";
        variantInfo.renderSlotDefault = true;
        stateHandle = shaderMgr.CreateGraphicsState(stateInfo, variantInfo);
        ASSERT_NE(RenderHandle {}, stateHandle.GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(path).GetHandle());

        IShaderManager::ShaderCreateInfo shaderInfo {};
        shaderInfo.path = "shaders://shader/GfxBackBufferRenderNodeTest.shader";
        IShaderManager::ShaderModulePath vertShader {};
        vertShader.path = "rendershaders://shader/fullscreen_triangle.vert.spv";
        vertShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
        IShaderManager::ShaderModulePath fragShader {};
        fragShader.path = "rendershaders://shader/GfxBackBufferRenderNodeTest.frag.spv";
        fragShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        IShaderManager::ShaderModulePath shaderModules[] = { vertShader, fragShader };
        shaderInfo.shaderPaths = { shaderModules, countof(shaderModules) };
        shaderInfo.graphicsState = stateHandle.GetHandle();
        shaderInfo.renderSlotId = slotId;
        shaderInfo.vertexInputDeclaration = inputHandle.GetHandle();
        auto shaderHandle = shaderMgr.CreateShader(shaderInfo);
        ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
        ASSERT_FALSE(shaderMgr.IsComputeShader(shaderHandle));
        ASSERT_TRUE(shaderMgr.IsShader(shaderHandle));

        ASSERT_EQ(
            inputHandle.GetHandle(), shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(shaderHandle).GetHandle());

        {
            auto shaders = shaderMgr.GetShaders(inputHandle, ShaderStageFlagBits::CORE_SHADER_STAGE_ALL);
            ASSERT_EQ(1, shaders.size());
            ASSERT_EQ(shaderHandle.GetHandle(), shaders[0].GetHandle());
        }
        {
            auto shaders = ((ShaderManager&)shaderMgr)
                               .GetShaders(inputHandle.GetHandle(), ShaderStageFlagBits::CORE_SHADER_STAGE_ALL);
            ASSERT_EQ(1, shaders.size());
            ASSERT_EQ(shaderHandle.GetHandle(), shaders[0]);
        }
        {
            auto shaders = shaderMgr.GetShaders(inputHandle, ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS);
            ASSERT_EQ(1, shaders.size());
            ASSERT_EQ(shaderHandle.GetHandle(), shaders[0].GetHandle());
        }
        {
            auto shaders = shaderMgr.GetShaders(inputHandle, ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT);
            ASSERT_EQ(0, shaders.size());
        }
        {
            auto shaders = shaderMgr.GetShaders(inputHandle, 0u);
            ASSERT_EQ(0, shaders.size());
        }
        {
            auto shaders =
                ((ShaderManager&)shaderMgr)
                    .GetShaders(inputHandle.GetHandle(), ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS);
            ASSERT_EQ(1, shaders.size());
            ASSERT_EQ(shaderHandle.GetHandle(), shaders[0]);
        }
        {
            auto shaders = ((ShaderManager&)shaderMgr)
                               .GetShaders(inputHandle.GetHandle(), ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT);
            ASSERT_EQ(0, shaders.size());
        }
        {
            auto shaders = ((ShaderManager&)shaderMgr).GetShaders(inputHandle.GetHandle(), 0u);
            ASSERT_EQ(0, shaders.size());
        }
    }
    {
        uint32_t slotId = shaderMgr.CreateRenderSlotId("Slot0");
        ASSERT_NE(INVALID_SM_INDEX, slotId);
        uint32_t slotId2 = shaderMgr.CreateRenderSlotId("Slot3");
        ASSERT_NE(INVALID_SM_INDEX, slotId);

        uint32_t sameSlotId = shaderMgr.CreateRenderSlotId("Slot0");
        ASSERT_EQ(slotId, sameSlotId);
        ASSERT_EQ(slotId, shaderMgr.GetRenderSlotId("Slot0"));
        ASSERT_EQ(INVALID_SM_INDEX, shaderMgr.GetRenderSlotId("NonExistingSlot"));

        string path = "state0";
        GraphicsState graphicsState {};
        IShaderManager::GraphicsStateCreateInfo stateInfo { path, graphicsState };
        IShaderManager::GraphicsStateVariantCreateInfo variantInfo {};
        variantInfo.renderSlot = "Slot0";
        auto stateHandle = shaderMgr.CreateGraphicsState(stateInfo, variantInfo);
        ASSERT_NE(RenderHandle {}, stateHandle.GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(path).GetHandle());

        IShaderManager::ShaderCreateInfo shaderInfo {};
        shaderInfo.path = "shaders://shader/GfxBackBufferRenderNodeTest.shader";
        IShaderManager::ShaderModulePath vertShader {};
        vertShader.path = "rendershaders://shader/fullscreen_triangle.vert.spv";
        vertShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
        IShaderManager::ShaderModulePath fragShader {};
        fragShader.path = "rendershaders://shader/GfxBackBufferRenderNodeTest.frag.spv";
        fragShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        IShaderManager::ShaderModulePath shaderModules[] = { vertShader, fragShader };
        shaderInfo.shaderPaths = { shaderModules, countof(shaderModules) };
        shaderInfo.graphicsState = stateHandle.GetHandle();
        shaderInfo.renderSlotId = slotId;
        auto shaderHandle = shaderMgr.CreateShader(shaderInfo);
        ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
        ASSERT_FALSE(shaderMgr.IsComputeShader(shaderHandle));
        ASSERT_TRUE(shaderMgr.IsShader(shaderHandle));

        auto shaderInfo2 = shaderInfo;
        shaderInfo2.path = "path";
        shaderInfo2.renderSlotId = slotId2;
        auto shaderHandle2 = shaderMgr.CreateShader(shaderInfo2, shaderInfo.path, "variant1");
        ASSERT_TRUE(shaderMgr.IsShader(shaderHandle2));

        ASSERT_EQ(shaderHandle2.GetHandle(),
            ((ShaderManager&)shaderMgr).GetShaderHandle(shaderHandle2.GetHandle(), slotId2).GetHandle());
        ASSERT_EQ(shaderHandle.GetHandle(),
            ((ShaderManager&)shaderMgr).GetShaderHandle(shaderHandle2.GetHandle(), slotId).GetHandle());
    }
    {
        uint32_t slotId = shaderMgr.CreateRenderSlotId("Slot1");
        ASSERT_NE(INVALID_SM_INDEX, slotId);
        uint32_t slotId2 = shaderMgr.CreateRenderSlotId("Slot2");
        ASSERT_NE(INVALID_SM_INDEX, slotId);

        string path = "state1";
        GraphicsState graphicsState {};
        IShaderManager::GraphicsStateCreateInfo stateInfo { path, graphicsState };
        IShaderManager::GraphicsStateVariantCreateInfo variantInfo {};
        variantInfo.renderSlot = "Slot1";
        auto stateHandle = shaderMgr.CreateGraphicsState(stateInfo, variantInfo);
        ASSERT_NE(RenderHandle {}, stateHandle.GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(path).GetHandle());
        ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(stateHandle, slotId).GetHandle());

        IShaderManager::ComputeShaderCreateInfo shaderInfo {};
        shaderInfo.path = "shaders://computeshader/GfxGpuBufferRenderNodeTest.shader";
        IShaderManager::ShaderModulePath compShader {};
        compShader.path = "rendershaders://computeshader/GfxGpuBufferRenderNodeTest.comp.spv";
        compShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
        IShaderManager::ShaderModulePath shaderModules[] = { compShader };
        shaderInfo.shaderPaths = { shaderModules, countof(shaderModules) };
        shaderInfo.renderSlotId = slotId;
        auto shaderHandle = shaderMgr.CreateComputeShader(shaderInfo);
        ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
        ASSERT_TRUE(shaderMgr.IsComputeShader(shaderHandle));
        ASSERT_FALSE(shaderMgr.IsShader(shaderHandle));

        auto shaderInfo2 = shaderInfo;
        shaderInfo2.path = "CoputePath0";
        shaderInfo2.renderSlotId = slotId2;
        auto shaderHandle2 = shaderMgr.CreateComputeShader(shaderInfo2, shaderInfo.path, "variantC1");
        ASSERT_TRUE(shaderMgr.IsComputeShader(shaderHandle2));

        ASSERT_EQ(shaderHandle2.GetHandle(),
            ((ShaderManager&)shaderMgr).GetShaderHandle(shaderHandle2.GetHandle(), slotId2).GetHandle());
        ASSERT_EQ(RenderHandle {},
            ((ShaderManager&)shaderMgr).GetShaderHandle(shaderHandle2.GetHandle(), slotId).GetHandle());
    }
    {
        string plPath = "PipelineLayout0";
        PipelineLayout pipelineLayout {};
        pipelineLayout.pushConstant.byteSize = 32;
        pipelineLayout.pushConstant.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        IShaderManager::PipelineLayoutCreateInfo pipelineInfo { plPath, pipelineLayout };
        auto plHandle = shaderMgr.CreatePipelineLayout(pipelineInfo);
        ASSERT_NE(RenderHandle {}, plHandle.GetHandle());

        ASSERT_EQ(plHandle.GetHandle(), shaderMgr.GetPipelineLayoutHandle(plPath).GetHandle());
        ASSERT_EQ(pipelineLayout.pushConstant.byteSize, shaderMgr.GetPipelineLayout(plHandle).pushConstant.byteSize);
        ASSERT_EQ(PipelineLayout {}.pushConstant.byteSize,
            shaderMgr.GetPipelineLayout(RenderHandleReference {}).pushConstant.byteSize);

        auto idDesc = shaderMgr.GetIdDesc(plHandle);
        ASSERT_EQ(plPath, idDesc.path);

        auto compFlags = shaderMgr.GetCompatibilityFlags(plHandle, plHandle);
        ASSERT_TRUE(compFlags & IShaderManager::CompatibilityFlagBits::EXACT_BIT);
        ((ShaderManager&)shaderMgr).HandlePendingAllocations();

        {
            uint32_t slotId = shaderMgr.CreateRenderSlotId("Slot1");
            ASSERT_NE(INVALID_SM_INDEX, slotId);

            string path = "state1";
            GraphicsState graphicsState {};
            IShaderManager::GraphicsStateCreateInfo stateInfo { path, graphicsState };
            IShaderManager::GraphicsStateVariantCreateInfo variantInfo {};
            variantInfo.renderSlot = "Slot1";
            auto stateHandle = shaderMgr.CreateGraphicsState(stateInfo, variantInfo);
            ASSERT_NE(RenderHandle {}, stateHandle.GetHandle());
            ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(path).GetHandle());
            ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(stateHandle, slotId).GetHandle());

            IShaderManager::ComputeShaderCreateInfo shaderInfo {};
            shaderInfo.path = "CompShader0";
            IShaderManager::ShaderModulePath compShader {};
            compShader.path = "rendershaders://computeshader/GfxGpuBufferRenderNodeTest.comp.spv";
            compShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
            IShaderManager::ShaderModulePath shaderModules[] = { compShader };
            shaderInfo.shaderPaths = { shaderModules, countof(shaderModules) };
            shaderInfo.renderSlotId = slotId;
            shaderInfo.pipelineLayout = plHandle.GetHandle();
            auto shaderHandle = shaderMgr.CreateComputeShader(shaderInfo);
            ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
            ASSERT_TRUE(shaderMgr.IsComputeShader(shaderHandle));
            ASSERT_FALSE(shaderMgr.IsShader(shaderHandle));

            ASSERT_EQ(plHandle.GetHandle(), shaderMgr.GetPipelineLayoutHandleByShaderHandle(shaderHandle).GetHandle());
            auto reflectionHandle = shaderMgr.GetReflectionPipelineLayoutHandle(shaderHandle);
            auto refPl1 = shaderMgr.GetPipelineLayout(reflectionHandle);
            auto refPl2 = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
            EXPECT_TRUE(std::equal(std::begin(refPl1.descriptorSetLayouts), std::end(refPl1.descriptorSetLayouts),
                std::begin(refPl2.descriptorSetLayouts)));
            ASSERT_EQ(refPl1.pushConstant.byteSize, refPl2.pushConstant.byteSize);
            ASSERT_EQ(refPl1.pushConstant.shaderStageFlags, refPl2.pushConstant.shaderStageFlags);

            auto spec = shaderMgr.GetReflectionSpecialization(shaderHandle);
            ASSERT_EQ(0, spec.constants.size());

            auto tgs = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);
            ASSERT_EQ(1, tgs.x);
            ASSERT_EQ(1, tgs.y);
            ASSERT_EQ(1, tgs.z);
        }
        {
            uint32_t slotId = shaderMgr.CreateRenderSlotId("Slot0");
            ASSERT_NE(INVALID_SM_INDEX, slotId);

            string path = "state10";
            GraphicsState graphicsState {};
            IShaderManager::GraphicsStateCreateInfo stateInfo { path, graphicsState };
            auto stateHandle = shaderMgr.CreateGraphicsState(stateInfo);
            ASSERT_NE(RenderHandle {}, stateHandle.GetHandle());
            ASSERT_EQ(stateHandle.GetHandle(), shaderMgr.GetGraphicsStateHandle(path).GetHandle());

            IShaderManager::ShaderCreateInfo shaderInfo {};
            shaderInfo.path = "GfxShader0";
            IShaderManager::ShaderModulePath vertShader {};
            vertShader.path = "rendershaders://shader/fullscreen_triangle.vert.spv";
            vertShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
            IShaderManager::ShaderModulePath fragShader {};
            fragShader.path = "rendershaders://shader/GfxBackBufferRenderNodeTest.frag.spv";
            fragShader.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
            IShaderManager::ShaderModulePath shaderModules[] = { vertShader, fragShader };
            shaderInfo.shaderPaths = { shaderModules, countof(shaderModules) };
            shaderInfo.graphicsState = stateHandle.GetHandle();
            shaderInfo.renderSlotId = slotId;
            shaderInfo.pipelineLayout = plHandle.GetHandle();
            auto shaderHandle = shaderMgr.CreateShader(shaderInfo);
            ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
            ASSERT_FALSE(shaderMgr.IsComputeShader(shaderHandle));
            ASSERT_TRUE(shaderMgr.IsShader(shaderHandle));

            ASSERT_EQ(plHandle.GetHandle(), shaderMgr.GetPipelineLayoutHandleByShaderHandle(shaderHandle).GetHandle());
            auto inputDec = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle);
            ASSERT_EQ(0, inputDec.attributeDescriptions.size());
            ASSERT_EQ(0, inputDec.bindingDescriptions.size());
        }
        shaderMgr.Destroy(plHandle);
    }
    {
        shaderMgr.UnloadShaderFiles({});
        auto binder = shaderMgr.CreateShaderPipelineBinder({});
        ASSERT_FALSE(binder);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GRAPHICS_STATE, 0);
        RenderHandleReference handleRef { handle, {} };
        {
            auto shaders = shaderMgr.GetShaders(handleRef, ShaderStageFlagBits::CORE_SHADER_STAGE_ALL);
            ASSERT_EQ(0, shaders.size());
        }
        {
            auto shaders = ((ShaderManager&)shaderMgr).GetShaders(handle, ShaderStageFlagBits::CORE_SHADER_STAGE_ALL);
            ASSERT_EQ(0, shaders.size());
        }
    }
    {
        PipelineLayout pl;
        pl.pushConstant.byteSize = 763u;
        IShaderManager::PipelineLayoutCreateInfo createInfo { "newPlName", pl };
        auto handle = shaderMgr.CreatePipelineLayout(createInfo);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
    }
    {
        auto pl = ((ShaderManager&)shaderMgr).GetReflectionPipelineLayoutRef({});
        for (const auto& layout : pl.descriptorSetLayouts) {
            EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
        }
        auto handle = shaderMgr.GetReflectionPipelineLayoutHandle({});
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::SHADER_STATE_OBJECT, 0);
        auto pl = ((ShaderManager&)shaderMgr).GetPipelineLayoutRef(handle);
        for (const auto& layout : pl.descriptorSetLayouts) {
            EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
        }
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, 0);
        auto gs = ((ShaderManager&)shaderMgr).GetGraphicsStateRef(handle);
        ASSERT_EQ(GraphicsState {}.inputAssembly.primitiveTopology, gs.inputAssembly.primitiveTopology);
    }
    {
        auto handle = shaderMgr.GetShaderHandle("", "");
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/GfxComputeGenericRenderNodeTest.shader", "");
        ASSERT_EQ(RenderHandleType::COMPUTE_SHADER_STATE_OBJECT, handle.GetHandleType());
    }
    {
        IShaderManager::ShaderCreateInfo createInfo;
        auto handle = shaderMgr.CreateShader(createInfo, "", "");
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        IShaderManager::ShaderCreateInfo createInfo;
        IShaderManager::ShaderModulePath modules[2];
        modules[0].path = "";
        modules[1].path = "";
        createInfo.shaderPaths = { modules, 2 };
        auto handle = shaderMgr.CreateShader(createInfo, "", "");
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        IShaderManager::ComputeShaderCreateInfo createInfo;
        auto handle = shaderMgr.CreateComputeShader(createInfo, "", "");
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        IShaderManager::ComputeShaderCreateInfo createInfo;
        IShaderManager::ShaderModulePath modules[1];
        modules[0].path = "";
        createInfo.shaderPaths = { modules, 1 };
        auto handle = shaderMgr.CreateComputeShader(createInfo, "", "");
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        string path = "rendershaders://computeshader/GfxComputeGenericRenderNodeTest.shader";
        auto handle = shaderMgr.GetShaderHandle(path);
        ((ShaderManager&)shaderMgr).AddAdditionalNameForHandle(handle, path);
        shaderMgr.SetRenderSlotData({ 0u, handle, {}, {}, {} });
    }
    {
        GraphicsState gs;
        IShaderManager::GraphicsStateCreateInfo gsCreateInfo { "gs0Variant", gs };
        IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo;
        variantCreateInfo.baseShaderState = "gs0";
        variantCreateInfo.baseVariant = "gs0";
        variantCreateInfo.renderSlot = 0u;
        variantCreateInfo.variant = "Variant";
        auto handle = shaderMgr.CreateGraphicsState(gsCreateInfo, variantCreateInfo);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
    }
    {
        GraphicsState gs;
        IShaderManager::GraphicsStateCreateInfo gsCreateInfo { "gs0", gs };
        auto handle = shaderMgr.CreateGraphicsState(gsCreateInfo);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
    }
    {
        GraphicsState gs;
        IShaderManager::GraphicsStateCreateInfo gsCreateInfo { "gs0", gs };
        IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo;
        variantCreateInfo.baseShaderState = "gs0";
        variantCreateInfo.baseVariant = "";
        variantCreateInfo.renderSlot = "";
        variantCreateInfo.variant = "Variant";
        {
            auto handle = shaderMgr.CreateGraphicsState(gsCreateInfo, variantCreateInfo);
            ASSERT_NE(RenderHandle {}, handle.GetHandle());
        }
        {
            auto handle = shaderMgr.CreateGraphicsState(gsCreateInfo, variantCreateInfo);
            ASSERT_NE(RenderHandle {}, handle.GetHandle());
        }
    }
    {
        uint32_t slot0 = shaderMgr.CreateRenderSlotId("renderSlotVariant0");
        uint32_t slot1 = shaderMgr.CreateRenderSlotId("renderSlotVariant1");
        RenderHandleReference handle0;
        RenderHandleReference handle1;
        {
            GraphicsState gs;
            IShaderManager::GraphicsStateCreateInfo gsCreateInfo { "gs0", gs };
            IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo;
            variantCreateInfo.baseShaderState = "gs0";
            variantCreateInfo.baseVariant = "";
            variantCreateInfo.renderSlot = "renderSlotVariant0";
            variantCreateInfo.variant = "Variant0";
            handle0 = shaderMgr.CreateGraphicsState(gsCreateInfo, variantCreateInfo);
            ASSERT_NE(RenderHandle {}, handle0.GetHandle());
        }
        {
            GraphicsState gs;
            IShaderManager::GraphicsStateCreateInfo gsCreateInfo { "gs0", gs };
            IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo;
            variantCreateInfo.baseShaderState = "gs0";
            variantCreateInfo.baseVariant = "";
            variantCreateInfo.renderSlot = "renderSlotVariant1";
            variantCreateInfo.variant = "Variant1";
            handle1 = shaderMgr.CreateGraphicsState(gsCreateInfo, variantCreateInfo);
            ASSERT_NE(RenderHandle {}, handle1.GetHandle());
        }
        {
            auto handle = shaderMgr.GetGraphicsStateHandle(handle1, slot0);
            ASSERT_EQ(handle0.GetHandle(), handle.GetHandle());
        }
        {
            auto handle = shaderMgr.GetGraphicsStateHandle(handle0, slot1);
            ASSERT_EQ(handle1.GetHandle(), handle.GetHandle());
        }
    }
    {
        uint32_t slot0 = shaderMgr.CreateRenderSlotId("renderSlotVariant0");
        uint32_t slot1 = shaderMgr.CreateRenderSlotId("renderSlotVariant1");
        uint32_t slot2 = shaderMgr.CreateRenderSlotId("renderSlotVariant2");

        IShaderManager::ShaderCreateInfo shaderCreateInfo;
        shaderCreateInfo.graphicsState = {};
        shaderCreateInfo.path = "shaderBasePath";
        shaderCreateInfo.pipelineLayout = {};
        shaderCreateInfo.renderSlotId = slot2;
        IShaderManager::ShaderModulePath modules[2];
        modules[0].path = "rendershaders://shader/fullscreen_triangle.vert.spv";
        modules[0].shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
        modules[1].path = "rendershaders://shader/ShaderPipelineBinderTest.frag.spv";
        modules[1].shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        shaderCreateInfo.shaderPaths = { modules, countof(modules) };
        shaderCreateInfo.vertexInputDeclaration = {};
        auto baseHandle = shaderMgr.CreateShader(shaderCreateInfo);
        ASSERT_NE(RenderHandle {}, baseHandle.GetHandle());

        shaderCreateInfo.renderSlotId = slot0;
        shaderCreateInfo.path = "shaderBasePath0";
        auto handle0 = shaderMgr.CreateShader(shaderCreateInfo, "shaderBasePath", "Variant0");
        ASSERT_NE(RenderHandle {}, handle0.GetHandle());
        shaderCreateInfo.renderSlotId = slot1;
        shaderCreateInfo.path = "shaderBasePath1";
        auto handle1 = shaderMgr.CreateShader(shaderCreateInfo, "shaderBasePath", "Variant1");
        ASSERT_NE(RenderHandle {}, handle1.GetHandle());

        auto handle2 = shaderMgr.CreateShader(shaderCreateInfo, "nonExistingBasePath", "Variant2");
        ASSERT_NE(RenderHandle {}, handle2.GetHandle());

        {
            auto handle = shaderMgr.GetShaderHandle(baseHandle, slot1);
            ASSERT_EQ(handle1.GetHandle(), handle.GetHandle());
        }
        {
            auto handle = shaderMgr.GetShaderHandle(baseHandle, slot0);
            ASSERT_EQ(handle0.GetHandle(), handle.GetHandle());
        }
    }
    {
        uint32_t slot0 = shaderMgr.CreateRenderSlotId("renderSlotVariant0");
        uint32_t slot1 = shaderMgr.CreateRenderSlotId("renderSlotVariant1");
        uint32_t slot2 = shaderMgr.CreateRenderSlotId("renderSlotVariant2");

        IShaderManager::ComputeShaderCreateInfo shaderCreateInfo;
        shaderCreateInfo.path = "computeShaderBasePath";
        shaderCreateInfo.pipelineLayout = {};
        shaderCreateInfo.renderSlotId = slot2;
        IShaderManager::ShaderModulePath modules[1];
        modules[0].path = "rendershaders://computeshader/GfxComputeGenericRenderNodeTest.comp.spv";
        modules[0].shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
        shaderCreateInfo.shaderPaths = { modules, countof(modules) };
        auto baseHandle = shaderMgr.CreateComputeShader(shaderCreateInfo);
        ASSERT_NE(RenderHandle {}, baseHandle.GetHandle());

        shaderCreateInfo.renderSlotId = slot0;
        auto handle0 = shaderMgr.CreateComputeShader(shaderCreateInfo, "computeShaderBasePath", "Variant0");
        ASSERT_NE(RenderHandle {}, handle0.GetHandle());
        shaderCreateInfo.renderSlotId = slot1;
        auto handle1 = shaderMgr.CreateComputeShader(shaderCreateInfo, "computeShaderBasePath", "Variant1");
        ASSERT_NE(RenderHandle {}, handle1.GetHandle());

        {
            auto handle = shaderMgr.GetShaderHandle(handle0, slot1);
            ASSERT_EQ(handle1.GetHandle(), handle.GetHandle());
        }
        {
            auto handle = shaderMgr.GetShaderHandle(handle1, slot0);
            ASSERT_EQ(handle0.GetHandle(), handle.GetHandle());
        }

        ((ShaderManager&)shaderMgr).HandlePendingAllocations();
        shaderMgr.Destroy(handle1);
        ((ShaderManager&)shaderMgr).HandlePendingAllocations();
    }
#if NDEBUG
    {
        ShaderModuleCreateInfo moduleCreateInfo;
        moduleCreateInfo.reflectionData = {};
        moduleCreateInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
        moduleCreateInfo.spvData = {};
        ((ShaderManager&)shaderMgr)
            .CreateShaderModule(
                "rendershaders://computeshader/GfxComputeGenericRenderNodeTest.comp.spv", moduleCreateInfo);
        ((ShaderManager&)shaderMgr)
            .CreateShaderModule(
                "rendershaders://computeshader/GfxComputeGenericRenderNodeTest.comp.spv", moduleCreateInfo);
        ((ShaderManager&)shaderMgr).HandlePendingAllocations();
    }
    {
        ShaderModuleCreateInfo moduleCreateInfo;
        moduleCreateInfo.reflectionData = {};
        moduleCreateInfo.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
        moduleCreateInfo.spvData = {};
        ((ShaderManager&)shaderMgr)
            .CreateShaderModule("rendershaders://shader/ShaderPipelineBinderTest.frag.spv", moduleCreateInfo);
        ((ShaderManager&)shaderMgr)
            .CreateShaderModule("rendershaders://shader/ShaderPipelineBinderTest.frag.spv", moduleCreateInfo);
        ((ShaderManager&)shaderMgr).HandlePendingAllocations();
    }
#endif // NDEBUG
    {
        PipelineLayout pl;
        pl.descriptorSetLayouts[0].bindings.resize(1);
        pl.descriptorSetLayouts[0].set = 0u;
        pl.pushConstant.byteSize = 64u;
        auto plHandle = shaderMgr.CreatePipelineLayout({ "newPipelineLayout", pl });
        ASSERT_NE(RenderHandle {}, plHandle.GetHandle());

        IShaderManager::ComputeShaderCreateInfo shaderCreateInfo;
        shaderCreateInfo.path = "computeShader17";
        shaderCreateInfo.pipelineLayout = plHandle.GetHandle();
        IShaderManager::ShaderModulePath modules[1];
        modules[0].path = "rendershaders://computeshader/GfxComputeGenericRenderNodeTest.comp.spv";
        modules[0].shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
        shaderCreateInfo.shaderPaths = { modules, countof(modules) };
        auto shaderHandle = shaderMgr.CreateComputeShader(shaderCreateInfo);
        ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());

        {
            auto handles = shaderMgr.GetShaders(plHandle, ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT);
            ASSERT_EQ(1, handles.size());
            ASSERT_EQ(shaderHandle.GetHandle(), handles[0].GetHandle());
        }
        {
            auto handles = ((ShaderManager&)shaderMgr)
                               .GetShaders(plHandle.GetHandle(), ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT);
            ASSERT_EQ(1, handles.size());
            ASSERT_EQ(shaderHandle.GetHandle(), handles[0]);
        }
    }
    {
        ASSERT_EQ(0, shaderMgr.GetShaderFile({}).size());
        auto handle = shaderMgr.GetShaderHandle("rendershaders://shader/ShaderPipelineBinderTest.shader");
        ASSERT_NE(0, shaderMgr.GetShaderFile(handle).size());
    }
}
} // namespace

/**
 * @tc.name: ShaderReflectionDataTest
 * @tc.desc: Tests ShaderReflectionData class using different valid and invalid test cases and checks if pipeline
 * layouts inferred from the reflection data are correct.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderManager, ShaderReflectionDataTest, testing::ext::TestSize.Level1)
{
    {
        const uint8_t data[16] = { 0u };
        ShaderReflectionData srd { { data, countof(data) } };
        EXPECT_FALSE(srd.IsValid());
        EXPECT_EQ(0, srd.GetStageFlags());
    }
    {
        ShaderReflectionData srd { {} };
        EXPECT_FALSE(srd.IsValid());
    }
    {
        const uint8_t data[16] = {
            'r', 'f', 'l', 1, // tag
        };
        ShaderReflectionData srd { { data, countof(data) } };
        EXPECT_TRUE(srd.IsValid());
        EXPECT_TRUE(srd.GetInputDescriptions().empty());
        EXPECT_TRUE(srd.GetSpecializationConstants().empty());
        EXPECT_EQ(0, srd.GetStageFlags());
    }
    {
        // offsets out of bounds
        const uint8_t data[16] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            16, 0,                             // offsetPushConstants
            16, 0,                             // offsetSpecializationConstants
            16, 0,                             // offsetDescriptorSets
            16, 0,                             // offsetInputs
            16, 0,                             // offsetLocalSize
        };
        ShaderReflectionData srd { { data, countof(data) } };
        EXPECT_TRUE(srd.IsValid());
        EXPECT_FALSE(srd.GetPushConstants());
        EXPECT_TRUE(srd.GetSpecializationConstants().empty());
        for (const auto& layout : srd.GetPipelineLayout().descriptorSetLayouts) {
            EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
        }
        EXPECT_TRUE(srd.GetInputDescriptions().empty());
        EXPECT_EQ(srd.GetLocalSize(), Math::UVec3());
    }
    {
        // "valid" push constant
        const uint8_t data[] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            16, 0,                             // offsetPushConstants
            0, 0,                              // offsetSpecializationConstants
            0, 0,                              // offsetDescriptorSets
            0, 0,                              // offsetInputs
            0, 0,                              // offsetLocalSize
            1,                                 // push constants available
            4, 0                               // size in bytes
        };
        {
            // available but not enough data
            ShaderReflectionData srd { { data, countof(data) - 1U } };
            EXPECT_TRUE(srd.IsValid());
            EXPECT_EQ(CORE_SHADER_STAGE_FRAGMENT_BIT, srd.GetStageFlags());
            EXPECT_FALSE(srd.GetPushConstants());
        }
        {
            ShaderReflectionData srd { { data, countof(data) } };
            EXPECT_TRUE(srd.IsValid());
            EXPECT_EQ(CORE_SHADER_STAGE_FRAGMENT_BIT, srd.GetStageFlags());
            EXPECT_TRUE(srd.GetPushConstants());
            EXPECT_EQ(srd.GetPipelineLayout().pushConstant.byteSize, 4U);
        }
    }
    {
        const uint8_t data[] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            0, 0,                              // offsetPushConstants
            16, 0,                             // offsetSpecializationConstants
            0, 0,                              // offsetDescriptorSets
            0, 0,                              // offsetInputs
            0, 0,                              // offsetLocalSize
            2, 0, 0, 0,                        // number of spec consts
            1, 0, 0, 0,                        // spec const id
            3, 0, 0, 0,                        // spec const type
            2, 0, 0, 0,                        // spec const id
            4, 0, 0, 0                         // spec const type
        };
        {
            // valid SpecializationConstants
            ShaderReflectionData srd { { data, countof(data) } };
            EXPECT_TRUE(srd.IsValid());
            const auto specializationConstants = srd.GetSpecializationConstants();
            ASSERT_EQ(specializationConstants.size(), 2U);
            EXPECT_EQ(specializationConstants[0].id, 1U);
            EXPECT_EQ(specializationConstants[0].type, ShaderSpecialization::Constant::Type::INT32);
            EXPECT_EQ(specializationConstants[1].id, 2U);
            EXPECT_EQ(specializationConstants[1].type, ShaderSpecialization::Constant::Type::FLOAT);
        }
        {
            // invalid SpecializationConstants, not enough data
            ShaderReflectionData srd { { data, countof(data) - 1U } };
            EXPECT_TRUE(srd.IsValid());
            EXPECT_TRUE(srd.GetSpecializationConstants().empty());
        }
    }
    {
        const uint8_t data[] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            0, 0,                              // offsetPushConstants
            0, 0,                              // offsetSpecializationConstants
            16, 0,                             // offsetDescriptorSets
            0, 0,                              // offsetInputs
            0, 0,                              // offsetLocalSize
            0, 0,                              // set count
        };
        {
            // valid, no descriptor sets
            ShaderReflectionData srd { { data, countof(data) } };
            EXPECT_TRUE(srd.IsValid());
            const auto pipelineLayout = srd.GetPipelineLayout();
            for (const auto& layout : pipelineLayout.descriptorSetLayouts) {
                EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
            }
        }
        {
            // invalid, not enough data
            ShaderReflectionData srd { { data, countof(data) - 1U } };
            EXPECT_TRUE(srd.IsValid());
            const auto pipelineLayout = srd.GetPipelineLayout();
            for (const auto& layout : pipelineLayout.descriptorSetLayouts) {
                EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
            }
        }
    }
    {
        const uint8_t data[] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            0, 0,                              // offsetPushConstants
            0, 0,                              // offsetSpecializationConstants
            16, 0,                             // offsetDescriptorSets
            0, 0,                              // offsetInputs
            0, 0,                              // offsetLocalSize
            1, 0,                              // set count
            3, 0,                              // set location
            0, 0                               // set descriptor count
        };
        {
            // valid, one descriptor sets
            ShaderReflectionData srd { { data, countof(data) } };
            EXPECT_TRUE(srd.IsValid());
            const auto pipelineLayout = srd.GetPipelineLayout();
            EXPECT_EQ(pipelineLayout.descriptorSetLayouts[3U].set, 3U);
            EXPECT_EQ(pipelineLayout.descriptorSetLayouts[3U].bindings.size(), 0U);
        }
        {
            // invalid, not enough data
            ShaderReflectionData srd { { data, countof(data) - 1U } };
            EXPECT_TRUE(srd.IsValid());
            const auto pipelineLayout = srd.GetPipelineLayout();
            for (const auto& layout : pipelineLayout.descriptorSetLayouts) {
                EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
            }
        }
    }
    {
        const uint8_t data[] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            0, 0,                              // offsetPushConstants
            0, 0,                              // offsetSpecializationConstants
            16, 0,                             // offsetDescriptorSets
            0, 0,                              // offsetInputs
            0, 0,                              // offsetLocalSize
            1, 0,                              // set count
            2, 0,                              // set location
            1, 0,                              // set descriptor count
            4, 0,                              // descriptor binding
            3, 0,                              // descriptor type
            1, 0,                              // descriptor count
            1, 16                              // image dimensions and flags (2d, sampled)
        };
        {
            // valid
            ShaderReflectionData srd { { data, countof(data) } };
            EXPECT_TRUE(srd.IsValid());
            const auto pipelineLayout = srd.GetPipelineLayout();
            EXPECT_EQ(pipelineLayout.descriptorSetLayouts[2U].bindings.size(), 1U);
            EXPECT_EQ(pipelineLayout.descriptorSetLayouts[2U].bindings[0U].binding, 4U);
            EXPECT_EQ(pipelineLayout.descriptorSetLayouts[2U].bindings[0U].descriptorType,
                DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            EXPECT_EQ(pipelineLayout.descriptorSetLayouts[2U].bindings[0U].descriptorCount, 1U);
            EXPECT_EQ(pipelineLayout.descriptorSetLayouts[2U].bindings[0U].additionalDescriptorTypeFlags, 0x00020010);
        }
        {
            // invalid, not enough data for descriptor set bindings
            ShaderReflectionData srd { { data, countof(data) - 1U } };
            EXPECT_TRUE(srd.IsValid());
            const auto pipelineLayout = srd.GetPipelineLayout();
            for (const auto& layout : pipelineLayout.descriptorSetLayouts) {
                EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
            }
        }
    }
    {
        const uint8_t data[] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            0, 0,                              // offsetPushConstants
            0, 0,                              // offsetSpecializationConstants
            0, 0,                              // offsetDescriptorSets
            16, 0,                             // offsetInputs
            0, 0,                              // offsetLocalSize
            2, 0,                              // input descriptor count
            3, 0,                              // input descriptor location
            103, 0,                            // input descriptor format
            4, 0,                              // input descriptor location
            16, 0                              // input descriptor format
        };
        {
            // valid InputDescriptions
            ShaderReflectionData srd { { data, countof(data) } };
            EXPECT_TRUE(srd.IsValid());
            const auto inputs = srd.GetInputDescriptions();
            ASSERT_EQ(inputs.size(), 2U);
            EXPECT_EQ(inputs[0].location, 3U);
            EXPECT_EQ(inputs[0].format, Format::BASE_FORMAT_R32G32_SFLOAT); // == 103
            EXPECT_EQ(inputs[1].location, 4U);
            EXPECT_EQ(inputs[1].format, Format::BASE_FORMAT_R8G8_UNORM); // == 16
        }
        {
            // invalid InputDescriptions, not enough data
            ShaderReflectionData srd { { data, countof(data) - 1U } };
            EXPECT_TRUE(srd.IsValid());
            const auto inputs = srd.GetInputDescriptions();
            EXPECT_TRUE(inputs.empty());
        }
    }
    {
        const uint8_t data[] = {
            'r', 'f', 'l', 1,                  // tag
            CORE_SHADER_STAGE_FRAGMENT_BIT, 0, // type
            0, 0,                              // offsetPushConstants
            0, 0,                              // offsetSpecializationConstants
            0, 0,                              // offsetDescriptorSets
            0, 0,                              // offsetInputs
            16, 0,                             // offsetLocalSize
            1, 2, 3, 4,                        // x
            2, 2, 0, 0,                        // y
            4, 3, 2, 1                         // z
        };
        {
            // valid LocalSize
            ShaderReflectionData srd { { data, countof(data) } };
            EXPECT_TRUE(srd.IsValid());
            EXPECT_EQ(srd.GetLocalSize(), Math::UVec3(0x04030201, 0x202, 0x01020304));
        }
        {
            // invalid LocalSize, not enough data
            ShaderReflectionData srd { { data, countof(data) - 1U } };
            EXPECT_TRUE(srd.IsValid());
            EXPECT_EQ(srd.GetLocalSize(), Math::UVec3());
        }
    }
}

/**
 * @tc.name: ShaderReflectionDataV0Test
 * @tc.desc: Tests ShaderReflectionData by decoding Base64 string from a shader, expecting correct reflection data.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderManager, ShaderReflectionDataV0Test, testing::ext::TestSize.Level1)
{
    {
        // fullscreen_blur.frag.spv.lsb v0 without image dimensions
        auto reflData = BASE_NS::Base64Decode(
            "cmZsABAAEABXAGMAhQAAAAEgAAJSiwAAAAAQAAAAAAAAABgALnVQYy52aWV3cG9ydFNpemVJbnZTaXplUosAABAAEAAAAAAAAA"
            "ALAC51UGMuZmFjdG9yAQAAAAAAAAACAAAAAgAAAAIAAAAGAAEAAQAGAAEAAQACAAAAAAABAAEAAgABAAEAAABnAA ==");

        ShaderReflectionData srd { reflData };
        EXPECT_TRUE(srd.IsValid());
        ShaderStageFlags shaderFlags = srd.GetStageFlags();
        PipelineLayout pipelineLayout = srd.GetPipelineLayout();
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].set, 0U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].bindings.size(), 2U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].bindings[0U].binding, 0U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].bindings[0U].descriptorType,
            DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].bindings[0U].descriptorCount, 1U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].bindings[1U].binding, 1U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].bindings[1U].descriptorType,
            DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[0U].bindings[1U].descriptorCount, 1U);

        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].set, 1U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].bindings.size(), 2U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].bindings[0U].binding, 0U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].bindings[0U].descriptorType,
            DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].bindings[0U].descriptorCount, 1U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].bindings[1U].binding, 1U);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].bindings[1U].descriptorType,
            DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        EXPECT_EQ(pipelineLayout.descriptorSetLayouts[1U].bindings[1U].descriptorCount, 1U);
        BASE_NS::vector<ShaderSpecialization::Constant> specializationConstants = srd.GetSpecializationConstants();
        ASSERT_EQ(specializationConstants.size(), 1U);
        EXPECT_EQ(specializationConstants[0].id, 0U);
        EXPECT_EQ(specializationConstants[0].type, ShaderSpecialization::Constant::Type::UINT32);
        BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> inputs = srd.GetInputDescriptions();
        ASSERT_EQ(inputs.size(), 1U);
        EXPECT_EQ(inputs[0].location, 0U);
        EXPECT_EQ(inputs[0].format, Format::BASE_FORMAT_R32G32_SFLOAT); // == 103
        BASE_NS::Math::UVec3 localSize = srd.GetLocalSize();
        const uint8_t* push = srd.GetPushConstants();
        EXPECT_EQ(srd.GetLocalSize(), Math::UVec3());
    }
}

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: ShaderManagerTestVulkan
 * @tc.desc: Tests the IShaderManager interface by creating shaders, pipeline layouts, vertex input declarations.
 * Overall tests the interface to behave correctly for Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderManager, ShaderManagerTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    Validate(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: ShaderManagerTestOpenGL
 * @tc.desc: Tests the IShaderManager interface by creating shaders, pipeline layouts, vertex input declarations.
 * Overall tests the interface to behave correctly for OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderManager, DISABLED_ShaderManagerTestOpenGL, testing::ext::TestSize.Level1)
{
    // NOTE: Test fails on windows machines

    UTest::EngineResources engine;
    engine.createWindow = true;
    engine.backend = UTest::GetOpenGLBackend();
    UTest::CreateEngineSetup(engine);
    Validate(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
