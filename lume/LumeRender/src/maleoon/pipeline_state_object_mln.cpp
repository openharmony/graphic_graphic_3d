/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "pipeline_state_object_mln.h"
#include "mln_log.h"
#include "util/log.h"

#include <cstring>
#include <securec.h>

#include <base/containers/vector.h>
#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_program_util.h"
#include "maleoon/device_mln.h"
#include "maleoon/gpu_program_mln.h"
#include "mln_convert.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {

MlnStencilOpState ToMlnStencilOpState(const GraphicsState::StencilOpState& src)
{
    MlnStencilOpState dst{};
    dst.failOp = ToMlnStencilOp(src.failOp);
    dst.passOp = ToMlnStencilOp(src.passOp);
    dst.depthFailOp = ToMlnStencilOp(src.depthFailOp);
    dst.compareOp = ToMlnCompareOp(src.compareOp);
    dst.compareMask = src.compareMask;
    dst.writeMask = src.writeMask;
    dst.reference = src.reference;
    return dst;
}

} // namespace

// ============================================================================
// GraphicsPipelineStateObjectMln
// ============================================================================

GraphicsPipelineStateObjectMln::GraphicsPipelineStateObjectMln(Device& device, const GpuShaderProgram& gpuProgram,
    const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
    const VertexInputDeclarationView& vertexInputDeclaration,
    const ShaderSpecializationConstantDataView& specializationConstants,
    array_view<const DynamicStateEnum> dynamicStates,
    const array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, uint32_t subpassIndex,
    const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData)
    : device_(device)
{
    // NOTE: Startup logging is heavily rate-limited on OHOS (hilog drops 50-100+ lines).
    // Keep PSO creation to MAX 2 CORE_LOG_E calls total (1 summary + 1 result) to avoid drops.
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    const auto& programMln = static_cast<const GpuShaderProgramMln&>(gpuProgram);
    const auto& platProgram = programMln.GetPlatformData();

    // === Shader stages ===
    MlnShaderStageState stages[2u]{};
    uint32_t stageCount = 0;

    // === Specialization constants (BUG-16 FIX) ===
    // Build per-stage specialization constant descriptors from the input data.
    // Without this, all specialization constants default to 0, disabling material
    // flags, lighting modes, and other shader features → black model output.
    vector<MlnCompileTimeConstantMapEntry> vsSpecEntries;
    vector<MlnCompileTimeConstantMapEntry> fsSpecEntries;
    uint32_t vsDataSize = 0;
    uint32_t fsDataSize = 0;

    for (const auto& constant : specializationConstants.constants) {
        const uint32_t constantSize = GpuProgramUtil::SpecializationByteSize(constant.type);
        MlnCompileTimeConstantMapEntry entry{};
        entry.constantID = static_cast<uint32_t>(constant.id);
        entry.offset = constant.offset;
        entry.size = constantSize;

        if (constant.shaderStage & CORE_SHADER_STAGE_VERTEX_BIT) {
            vsSpecEntries.push_back(entry);
            const uint32_t end = constant.offset + constantSize;
            if (end > vsDataSize)
                vsDataSize = end;
        }
        if (constant.shaderStage & CORE_SHADER_STAGE_FRAGMENT_BIT) {
            fsSpecEntries.push_back(entry);
            const uint32_t end = constant.offset + constantSize;
            if (end > fsDataSize)
                fsDataSize = end;
        }
    }

    MlnCompileTimeConstantState vsSpecDesc{};
    vsSpecDesc.mapEntryCount = static_cast<uint32_t>(vsSpecEntries.size());
    vsSpecDesc.mapEntries = vsSpecEntries.empty() ? nullptr : vsSpecEntries.data();
    vsSpecDesc.dataSize = vsDataSize;
    vsSpecDesc.data = specializationConstants.data.empty() ? nullptr : specializationConstants.data.data();

    MlnCompileTimeConstantState fsSpecDesc{};
    fsSpecDesc.mapEntryCount = static_cast<uint32_t>(fsSpecEntries.size());
    fsSpecDesc.mapEntries = fsSpecEntries.empty() ? nullptr : fsSpecEntries.data();
    fsSpecDesc.dataSize = fsDataSize;
    fsSpecDesc.data = specializationConstants.data.empty() ? nullptr : specializationConstants.data.data();

    // Spec const details suppressed to avoid hilog rate limit

    // Build inline MlnShaderDescriptors (new API: no MlnShader handle)
    MlnShaderDescriptor vsShaderDesc{};
    vsShaderDesc.source = platProgram.vertSpvSource;
    vsShaderDesc.sourceSize = platProgram.vertSpvSourceSize;

    MlnShaderDescriptor fsShaderDesc{};
    fsShaderDesc.source = platProgram.fragSpvSource;
    fsShaderDesc.sourceSize = platProgram.fragSpvSourceSize;

    if (platProgram.vertSpvSource) {
        auto& vs = stages[stageCount++];
        vs.extensionCount = 0;
        vs.extensions = nullptr;
        vs.flags = 0;
        vs.stage = MLN_SHADER_STAGE_VERTEX_BIT;
        vs.shaderDescriptor = &vsShaderDesc;
        vs.name = "main";
        vs.compileTimeConstant = vsSpecEntries.empty() ? nullptr : &vsSpecDesc;
    }
    if (platProgram.fragSpvSource) {
        auto& fs = stages[stageCount++];
        fs.extensionCount = 0;
        fs.extensions = nullptr;
        fs.flags = 0;
        fs.stage = MLN_SHADER_STAGE_FRAGMENT_BIT;
        fs.shaderDescriptor = &fsShaderDesc;
        fs.name = "main";
        fs.compileTimeConstant = fsSpecEntries.empty() ? nullptr : &fsSpecDesc;
    }

    // === Vertex input state ===
    vector<MlnVertexInputBindingState> viBindings(vertexInputDeclaration.bindingDescriptions.size());
    for (size_t i = 0; i < viBindings.size(); ++i) {
        const auto& src = vertexInputDeclaration.bindingDescriptions[i];
        viBindings[i].binding = src.binding;
        viBindings[i].stride = src.stride;
        viBindings[i].inputRate = static_cast<MlnVertexInputRate>(src.vertexInputRate);
    }
    // Diagnostic: print vertex input binding strides for PSO stride verification
    for (size_t i = 0; i < viBindings.size(); ++i) {
        MLN_LOG_GRAPH("GfxPSO-VI: binding[%zu] slot=%u stride=%u inputRate=%u", i, viBindings[i].binding,
            viBindings[i].stride, static_cast<uint32_t>(viBindings[i].inputRate));
    }
    if (viBindings.empty()) {
        MLN_LOG_GRAPH("GfxPSO-VI: NO vertex input bindings (fullscreen/procedural draw)");
    }

    vector<MlnVertexInputAttributeState> viAttributes(vertexInputDeclaration.attributeDescriptions.size());
    for (size_t i = 0; i < viAttributes.size(); ++i) {
        const auto& src = vertexInputDeclaration.attributeDescriptions[i];
        viAttributes[i].location = src.location;
        viAttributes[i].binding = src.binding;
        viAttributes[i].format = ToMlnFormat(src.format);
        viAttributes[i].offset = src.offset;
    }

    MlnVertexInputState vertexInputState{};
    vertexInputState.extensionCount = 0;
    vertexInputState.extensions = nullptr;
    vertexInputState.flags = 0;
    vertexInputState.vertexBindingStateCount = static_cast<uint32_t>(viBindings.size());
    vertexInputState.vertexBindingStates = viBindings.data();
    vertexInputState.vertexAttributeStateCount = static_cast<uint32_t>(viAttributes.size());
    vertexInputState.vertexAttributeStates = viAttributes.data();

    // === Input assembly ===
    MlnInputAssemblyState inputAssemblyState{};
    inputAssemblyState.extensionCount = 0;
    inputAssemblyState.extensions = nullptr;
    inputAssemblyState.flags = 0;
    inputAssemblyState.topology = ToMlnPrimitiveTopology(graphicsState.inputAssembly.primitiveTopology);
    inputAssemblyState.primitiveRestartEnable = graphicsState.inputAssembly.enablePrimitiveRestart ? 1u : 0u;

    // === Viewport state (dynamic, just count) ===
    MlnViewportState viewportState{};
    viewportState.extensionCount = 0;
    viewportState.extensions = nullptr;
    viewportState.flags = 0;
    viewportState.viewportCount = 1;
    viewportState.viewports = nullptr; // dynamic
    viewportState.scissorCount = 1;
    viewportState.scissors = nullptr; // dynamic

    // === Rasterization state ===
    const auto& rs = graphicsState.rasterizationState;
    MlnRasterizationState rasterState{};
    rasterState.extensionCount = 0;
    rasterState.extensions = nullptr;
    rasterState.flags = 0;
    rasterState.depthClampEnable = rs.enableDepthClamp ? 1u : 0u;
    rasterState.rasterizerDiscardEnable = rs.enableRasterizerDiscard ? 1u : 0u;
    rasterState.polygonMode = ToMlnPolygonMode(rs.polygonMode);
    rasterState.cullMode = ToMlnCullModeFlags(rs.cullModeFlags);
    rasterState.frontFace = ToMlnFrontFace(rs.frontFace);
    rasterState.depthBiasEnable = rs.enableDepthBias ? 1u : 0u;
    rasterState.depthBiasConstantFactor = rs.depthBiasConstantFactor;
    rasterState.depthBiasClamp = rs.depthBiasClamp;
    rasterState.depthBiasSlopeFactor = rs.depthBiasSlopeFactor;
    rasterState.lineWidth = rs.lineWidth;

    // === Multisample state ===
    // Determine sample count from first color attachment (matching Vulkan PSO behavior)
    const LowLevelRenderPassDataMln* rpDataMln = static_cast<const LowLevelRenderPassDataMln*>(renderPassData);
    MlnSampleCountFlagBits sampleCountFlagBits = MLN_SAMPLE_COUNT_1_BIT;
    if (!renderPassSubpassDescs.empty() && subpassIndex < renderPassSubpassDescs.size()) {
        const auto& sp = renderPassSubpassDescs[subpassIndex];
        if (sp.colorAttachmentCount > 0) {
            const uint32_t attIdx = sp.colorAttachmentIndices[0];
            if (rpDataMln && attIdx < rpDataMln->attachmentCount) {
                const MlnSampleCountFlags flags = rpDataMln->attachmentSampleCounts[attIdx];
                sampleCountFlagBits =
                    (flags == 0) ? MLN_SAMPLE_COUNT_1_BIT : static_cast<MlnSampleCountFlagBits>(flags);
            }
        }
    }

    const bool msaaEnabled =
        (sampleCountFlagBits != MLN_SAMPLE_COUNT_1_BIT) && (sampleCountFlagBits != MLN_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM);
    MlnMultisampleState multisampleState{};
    multisampleState.extensionCount = 0;
    multisampleState.extensions = nullptr;
    multisampleState.flags = 0;
    multisampleState.rasterizationSamples = sampleCountFlagBits;
    // Maleoon driver 暂不支持 sampleRateShading feature 查询。
    // Vulkan 端通过 enabledPhysicalDeviceFeatures.sampleRateShading 控制（当前为 VK_FALSE）。
    // 待 Maleoon driver 支持 feature 查询后，改为从 driver 查询 sampleRateShading 能力，
    // 并使用 deviceFeatureConfig.minSampleShading 替代硬编码值。
    multisampleState.sampleShadingEnable = 0u;
    multisampleState.minSampleShading = 0.0f;
    static const MlnSampleMask defaultSampleMask = 0xFFFFFFFFu;
    multisampleState.sampleMask = &defaultSampleMask;
    multisampleState.alphaToCoverageEnable = 0;
    multisampleState.alphaToOneEnable = 0;

    // === Depth stencil state ===
    const auto& ds = graphicsState.depthStencilState;
    MlnDepthStencilState depthStencilState{};
    depthStencilState.extensionCount = 0;
    depthStencilState.extensions = nullptr;
    depthStencilState.flags = 0;
    depthStencilState.depthTestEnable = ds.enableDepthTest ? 1u : 0u;
    depthStencilState.depthWriteEnable = ds.enableDepthWrite ? 1u : 0u;
    depthStencilState.depthCompareOp = ToMlnCompareOp(ds.depthCompareOp);
    depthStencilState.depthBoundsTestEnable = ds.enableDepthBoundsTest ? 1u : 0u;
    depthStencilState.stencilTestEnable = ds.enableStencilTest ? 1u : 0u;
    depthStencilState.front = ToMlnStencilOpState(ds.frontStencilOpState);
    depthStencilState.back = ToMlnStencilOpState(ds.backStencilOpState);
    depthStencilState.minDepthBounds = ds.minDepthBounds;
    depthStencilState.maxDepthBounds = ds.maxDepthBounds;

    // === Color blend state ===
    const auto& cb = graphicsState.colorBlendState;
    vector<MlnColorBlendAttachmentState> attachments(cb.colorAttachmentCount);
    for (uint32_t i = 0; i < cb.colorAttachmentCount; ++i) {
        const auto& src = cb.colorAttachments[i];
        auto& dst = attachments[i];
        dst.blendEnable = src.enableBlend ? 1u : 0u;
        dst.srcColorBlendFactor = ToMlnBlendFactor(src.srcColorBlendFactor);
        dst.dstColorBlendFactor = ToMlnBlendFactor(src.dstColorBlendFactor);
        dst.colorBlendOp = ToMlnBlendOp(src.colorBlendOp);
        dst.srcAlphaBlendFactor = ToMlnBlendFactor(src.srcAlphaBlendFactor);
        dst.dstAlphaBlendFactor = ToMlnBlendFactor(src.dstAlphaBlendFactor);
        dst.alphaBlendOp = ToMlnBlendOp(src.alphaBlendOp);
        dst.colorWriteMask = ToMlnColorComponentFlags(src.colorWriteMask);
    }

    MlnColorBlendState colorBlendState{};
    colorBlendState.extensionCount = 0;
    colorBlendState.extensions = nullptr;
    colorBlendState.flags = 0;
    colorBlendState.logicOpEnable = cb.enableLogicOp ? 1u : 0u;
    colorBlendState.logicOp = ToMlnLogicOp(cb.logicOp);
    colorBlendState.attachmentCount = cb.colorAttachmentCount;
    colorBlendState.attachments = attachments.data();
    std::copy(cb.colorBlendConstants, cb.colorBlendConstants + 4, colorBlendState.blendConstants);

    // === Rendering state (attachment formats) ===
    // Get color attachment formats from render pass subpass desc (rpDataMln declared above in multisample section)
    vector<MlnFormat> colorFormats;
    MlnFormat depthFormat = MLN_FORMAT_UNDEFINED;
    MlnFormat stencilFormat = MLN_FORMAT_UNDEFINED;

    if (!renderPassSubpassDescs.empty() && subpassIndex < renderPassSubpassDescs.size()) {
        const auto& subpass = renderPassSubpassDescs[subpassIndex];
        for (uint32_t i = 0; i < subpass.colorAttachmentCount; ++i) {
            const uint32_t attIdx = subpass.colorAttachmentIndices[i];
            MlnFormat fmt = MLN_FORMAT_UNDEFINED;
            if (rpDataMln && attIdx < rpDataMln->attachmentCount) {
                fmt = rpDataMln->attachmentFormats[attIdx];
            }
            colorFormats.push_back(fmt);
        }
        if (subpass.depthAttachmentCount > 0) {
            const uint32_t depthIdx = subpass.depthAttachmentIndex;
            if (rpDataMln && depthIdx < rpDataMln->attachmentCount) {
                depthFormat = rpDataMln->attachmentFormats[depthIdx];
                const uint32_t fmt = static_cast<uint32_t>(depthFormat);
                if (fmt == MLN_FORMAT_S8_UINT || fmt == MLN_FORMAT_D16_UNORM_S8_UINT ||
                    fmt == MLN_FORMAT_D24_UNORM_S8_UINT || fmt == MLN_FORMAT_D32_SFLOAT_S8_UINT) {
                    stencilFormat = depthFormat;
                }
            }
        }
    }

    MlnRenderingState renderingState{};
    renderingState.extensionCount = 0;
    renderingState.extensions = nullptr;
    renderingState.viewMask = 0;
    renderingState.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
    renderingState.colorAttachmentFormats = colorFormats.data();
    renderingState.depthAttachmentFormat = depthFormat;
    renderingState.stencilAttachmentFormat = stencilFormat;

    // === Dynamic state ===
    vector<MlnDynamicStateType> mlnDynamicStates;
    mlnDynamicStates.reserve(dynamicStates.size());
    for (const auto ds : dynamicStates) {
        mlnDynamicStates.push_back(ToMlnDynamicStateType(ds));
    }

    MlnDynamicState dynamicState{};
    dynamicState.extensionCount = 0;
    dynamicState.extensions = nullptr;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(mlnDynamicStates.size());
    dynamicState.dynamicStateTypes = mlnDynamicStates.data();

    // Save dynamic state types in platform data for stateSet at draw time (BUG-27 fix)
    plat_.dynamicStateCount =
        (dynamicState.dynamicStateCount < PipelineStateObjectPlatformDataMln::MAX_DYNAMIC_STATE_COUNT)
            ? dynamicState.dynamicStateCount
            : PipelineStateObjectPlatformDataMln::MAX_DYNAMIC_STATE_COUNT;
    for (uint32_t i = 0; i < plat_.dynamicStateCount; ++i) {
        plat_.dynamicStateTypes[i] = mlnDynamicStates[i];
    }

    // === Program interface (from pipeline layout) ===
    // Create MlnBindingLayout for each descriptor set in the pipeline layout.
    // This mirrors the VK backend which creates VkDescriptorSetLayout + VkPipelineLayout
    // from the same PipelineLayout data. Without this, the translated VkPipelineLayout
    // has 0 descriptor set slots and all descriptor set bindings silently fail.
    {
        uint32_t descriptorSetCount = 0;
        for (uint32_t setIdx = 0; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
            const auto& descSetLayout = pipelineLayout.descriptorSetLayouts[setIdx];
            if (descSetLayout.set != PipelineLayoutConstants::INVALID_INDEX) {
                descriptorSetCount = setIdx + 1;
            }
        }

        plat_.bindingLayouts.resize(descriptorSetCount, MLN_NULL_HANDLE);
        for (uint32_t setIdx = 0; setIdx < descriptorSetCount; ++setIdx) {
            const auto& descSetLayout = pipelineLayout.descriptorSetLayouts[setIdx];
            if (descSetLayout.set != PipelineLayoutConstants::INVALID_INDEX && !descSetLayout.bindings.empty()) {
                vector<MlnBindingSlot> slots(descSetLayout.bindings.size());
                for (size_t bi = 0; bi < descSetLayout.bindings.size(); ++bi) {
                    const auto& binding = descSetLayout.bindings[bi];
                    slots[bi].slot = binding.binding;
                    slots[bi].type = static_cast<MlnBindingType>(binding.descriptorType);
                    slots[bi].slotCount = binding.descriptorCount;
                    slots[bi].stageFlags =
                        static_cast<MlnShaderStageFlags>(ToMlnShaderStageFlagBits(binding.shaderStageFlags));
                    slots[bi].immutableSamplers = nullptr;
                }

                MlnBindingLayoutDescriptor layoutDesc{};
                layoutDesc.flags = 0;
                layoutDesc.bindingCount = static_cast<uint32_t>(slots.size());
                layoutDesc.bindings = slots.data();
                layoutDesc.bindingFlags = nullptr;

                plat_.bindingLayouts[setIdx] = MlnCreateBindingLayout(deviceMln.GetMlnDevice(), &layoutDesc);
            } else {
                // Empty set -- create empty binding layout (Vulkan requires contiguous set indices)
                MlnBindingLayoutDescriptor emptyLayoutDesc{};
                emptyLayoutDesc.flags = 0;
                emptyLayoutDesc.bindingCount = 0;
                emptyLayoutDesc.bindings = nullptr;
                emptyLayoutDesc.bindingFlags = nullptr;
                plat_.bindingLayouts[setIdx] = MlnCreateBindingLayout(deviceMln.GetMlnDevice(), &emptyLayoutDesc);
            }
        }

        MlnProgramInterfaceDescriptor ifaceDesc{};
        ifaceDesc.flags = 0;
        ifaceDesc.bindingLayoutCount = descriptorSetCount;
        ifaceDesc.bindingLayouts = plat_.bindingLayouts.empty() ? nullptr : plat_.bindingLayouts.data();

        plat_.programInterface = MlnCreateProgramInterface(deviceMln.GetMlnDevice(), &ifaceDesc);
    }

    // === Assemble MlnGraphicsProgramState ===
    MlnGraphicsProgramState programState{};
    programState.extensionCount = 0;
    programState.extensions = nullptr;
    programState.flags = 0;
    programState.stageCount = stageCount;
    programState.stages = stages;
    programState.vertexInputState = &vertexInputState;
    programState.inputAssemblyState = &inputAssemblyState;
    programState.viewportState = &viewportState;
    programState.rasterizationState = &rasterState;
    programState.multisampleState = &multisampleState;
    programState.depthStencilState = &depthStencilState;
    programState.colorBlendState = &colorBlendState;
    programState.renderState = &renderingState;
    programState.dynamicState = &dynamicState;
    programState.programInterface = plat_.programInterface;

    plat_.program = MlnCreateGraphicsProgram(deviceMln.GetMlnDevice(), deviceMln.GetMlnProgramCache(), &programState);
    // Single-line PSO result (minimized for hilog rate limit)
    MLN_LOG_GRAPH("GfxPSO sub=%u stg=%u col=%zu dep=%u pc=%u iface=%p => %s", subpassIndex, stageCount,
        colorFormats.size(), static_cast<uint32_t>(depthFormat), pipelineLayout.pushConstant.byteSize,
        reinterpret_cast<void*>(plat_.programInterface), plat_.program ? "OK" : "FAILED");
    // DIAGNOSTIC: dump color attachment formats + blend state to debug BUG-27 (all draws invisible)
    {
        char fmtBuf[128] = {};
        int pos = 0;
        for (size_t i = 0; i < colorFormats.size() && i < 4; ++i) {
            const int n = snprintf_s(fmtBuf + pos, sizeof(fmtBuf) - pos, sizeof(fmtBuf) - pos - 1, "%u ",
                static_cast<uint32_t>(colorFormats[i]));
            if (n > 0) {
                pos += n;
            }
        }
        MLN_LOG_GRAPH("GfxPSO-BLEND: colFmts=[%s] attachCount=%u", fmtBuf, colorBlendState.attachmentCount);
        for (uint32_t i = 0; i < colorBlendState.attachmentCount && i < 4; ++i) {
            const auto& a = attachments[i];
            MLN_LOG_GRAPH(
                "GfxPSO-BLEND[%u]: enable=%u srcColor=%u dstColor=%u colorOp=%u "
                "srcAlpha=%u dstAlpha=%u alphaOp=%u writeMask=0x%x",
                i, a.blendEnable, static_cast<uint32_t>(a.srcColorBlendFactor),
                static_cast<uint32_t>(a.dstColorBlendFactor), static_cast<uint32_t>(a.colorBlendOp),
                static_cast<uint32_t>(a.srcAlphaBlendFactor), static_cast<uint32_t>(a.dstAlphaBlendFactor),
                static_cast<uint32_t>(a.alphaBlendOp), static_cast<uint32_t>(a.colorWriteMask));
        }
        // Rasterization state
        MLN_LOG_GRAPH("GfxPSO-RASTER: cullMode=%u frontFace=%u depthBias=%u rastDiscard=%u lineW=%.1f",
            static_cast<uint32_t>(rasterState.cullMode), static_cast<uint32_t>(rasterState.frontFace),
            rasterState.depthBiasEnable, rasterState.rasterizerDiscardEnable, rasterState.lineWidth);
        // Depth/stencil state
        MLN_LOG_GRAPH("GfxPSO-DEPTH: testEn=%u writeEn=%u compareOp=%u boundsEn=%u stencilEn=%u",
            depthStencilState.depthTestEnable, depthStencilState.depthWriteEnable,
            static_cast<uint32_t>(depthStencilState.depthCompareOp), depthStencilState.depthBoundsTestEnable,
            depthStencilState.stencilTestEnable);
        // SPIR-V magic number check
        if (platProgram.vertSpvSource && platProgram.vertSpvSourceSize >= 4) {
            MLN_LOG_GRAPH("GfxPSO-SPV: vertMagic=0x%08x vertSize=%zu fragMagic=0x%08x fragSize=%zu",
                platProgram.vertSpvSource[0], platProgram.vertSpvSourceSize,
                platProgram.fragSpvSource ? platProgram.fragSpvSource[0] : 0, platProgram.fragSpvSourceSize);
        }
    }

    // Cache graphics state for render backend stateSet
    plat_.topology = inputAssemblyState.topology;
    plat_.primitiveRestartEnable = inputAssemblyState.primitiveRestartEnable;
    plat_.rasterizerDiscardEnable = rasterState.rasterizerDiscardEnable;
    plat_.cullMode = rasterState.cullMode;
    plat_.frontFace = rasterState.frontFace;
    plat_.depthBiasEnable = rasterState.depthBiasEnable;
    plat_.lineWidth = rasterState.lineWidth;
    plat_.depthTestEnable = depthStencilState.depthTestEnable;
    plat_.depthWriteEnable = depthStencilState.depthWriteEnable;
    plat_.depthCompareOp = depthStencilState.depthCompareOp;
    plat_.depthBoundsTestEnable = depthStencilState.depthBoundsTestEnable;
    plat_.stencilTestEnable = depthStencilState.stencilTestEnable;
}

GraphicsPipelineStateObjectMln::~GraphicsPipelineStateObjectMln()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    if (plat_.program) {
        MlnDestroyProgram(mlnDevice, plat_.program);
    }
    if (plat_.programInterface) {
        MlnDestroyProgramInterface(mlnDevice, plat_.programInterface);
    }
    for (auto& bl : plat_.bindingLayouts) {
        if (bl) {
            MlnDestroyBindingLayout(mlnDevice, bl);
        }
    }
}

const PipelineStateObjectPlatformDataMln& GraphicsPipelineStateObjectMln::GetPlatformData() const
{
    return plat_;
}

// ============================================================================
// ComputePipelineStateObjectMln
// ============================================================================

ComputePipelineStateObjectMln::ComputePipelineStateObjectMln(Device& device, const GpuComputeProgram& gpuProgram,
    const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
    const LowLevelPipelineLayoutData* pipelineLayoutData)
    : device_(device)
{
    // Compute PSO creation (logging minimized for hilog rate limit)
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    const auto& programMln = static_cast<const GpuComputeProgramMln&>(gpuProgram);
    const auto& platProgram = programMln.GetPlatformData();

    // === Specialization constants for compute (BUG-16 FIX) ===
    vector<MlnCompileTimeConstantMapEntry> csSpecEntries;
    uint32_t csDataSize = 0;

    for (const auto& constant : specializationConstants.constants) {
        const uint32_t constantSize = GpuProgramUtil::SpecializationByteSize(constant.type);
        if (constant.shaderStage & CORE_SHADER_STAGE_COMPUTE_BIT) {
            MlnCompileTimeConstantMapEntry entry{};
            entry.constantID = static_cast<uint32_t>(constant.id);
            entry.offset = constant.offset;
            entry.size = constantSize;
            csSpecEntries.push_back(entry);
            const uint32_t end = constant.offset + constantSize;
            if (end > csDataSize)
                csDataSize = end;
        }
    }

    MlnCompileTimeConstantState csSpecDesc{};
    csSpecDesc.mapEntryCount = static_cast<uint32_t>(csSpecEntries.size());
    csSpecDesc.mapEntries = csSpecEntries.empty() ? nullptr : csSpecEntries.data();
    csSpecDesc.dataSize = csDataSize;
    csSpecDesc.data = specializationConstants.data.empty() ? nullptr : specializationConstants.data.data();

    // Spec const details suppressed for hilog rate limit

    // Build inline MlnShaderDescriptor (new API: no MlnShader handle)
    MlnShaderDescriptor csShaderDesc{};
    csShaderDesc.source = platProgram.compSpvSource;
    csShaderDesc.sourceSize = platProgram.compSpvSourceSize;

    MlnShaderStageState stage{};
    stage.extensionCount = 0;
    stage.extensions = nullptr;
    stage.flags = 0;
    stage.stage = MLN_SHADER_STAGE_COMPUTE_BIT;
    stage.shaderDescriptor = &csShaderDesc;
    stage.name = "main";
    stage.compileTimeConstant = csSpecEntries.empty() ? nullptr : &csSpecDesc;

    // Create binding layouts from pipeline layout (same pattern as graphics PSO)
    {
        uint32_t descriptorSetCount = 0;
        for (uint32_t setIdx = 0; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
            const auto& descSetLayout = pipelineLayout.descriptorSetLayouts[setIdx];
            if (descSetLayout.set != PipelineLayoutConstants::INVALID_INDEX) {
                descriptorSetCount = setIdx + 1;
            }
        }

        plat_.bindingLayouts.resize(descriptorSetCount, MLN_NULL_HANDLE);
        for (uint32_t setIdx = 0; setIdx < descriptorSetCount; ++setIdx) {
            const auto& descSetLayout = pipelineLayout.descriptorSetLayouts[setIdx];
            if (descSetLayout.set != PipelineLayoutConstants::INVALID_INDEX && !descSetLayout.bindings.empty()) {
                vector<MlnBindingSlot> slots(descSetLayout.bindings.size());
                for (size_t bi = 0; bi < descSetLayout.bindings.size(); ++bi) {
                    const auto& binding = descSetLayout.bindings[bi];
                    slots[bi].slot = binding.binding;
                    slots[bi].type = static_cast<MlnBindingType>(binding.descriptorType);
                    slots[bi].slotCount = binding.descriptorCount;
                    slots[bi].stageFlags =
                        static_cast<MlnShaderStageFlags>(ToMlnShaderStageFlagBits(binding.shaderStageFlags));
                    slots[bi].immutableSamplers = nullptr;
                }

                MlnBindingLayoutDescriptor layoutDesc{};
                layoutDesc.flags = 0;
                layoutDesc.bindingCount = static_cast<uint32_t>(slots.size());
                layoutDesc.bindings = slots.data();
                layoutDesc.bindingFlags = nullptr;

                plat_.bindingLayouts[setIdx] = MlnCreateBindingLayout(deviceMln.GetMlnDevice(), &layoutDesc);
            } else {
                MlnBindingLayoutDescriptor emptyLayoutDesc{};
                emptyLayoutDesc.flags = 0;
                emptyLayoutDesc.bindingCount = 0;
                emptyLayoutDesc.bindings = nullptr;
                emptyLayoutDesc.bindingFlags = nullptr;
                plat_.bindingLayouts[setIdx] = MlnCreateBindingLayout(deviceMln.GetMlnDevice(), &emptyLayoutDesc);
            }
        }

        MlnProgramInterfaceDescriptor ifaceDesc{};
        ifaceDesc.flags = 0;
        ifaceDesc.bindingLayoutCount = descriptorSetCount;
        ifaceDesc.bindingLayouts = plat_.bindingLayouts.empty() ? nullptr : plat_.bindingLayouts.data();

        plat_.programInterface = MlnCreateProgramInterface(deviceMln.GetMlnDevice(), &ifaceDesc);
    }

    MlnComputeProgramState computeState{};
    computeState.extensionCount = 0;
    computeState.extensions = nullptr;
    computeState.flags = 0;
    computeState.stage = stage;
    computeState.programInterface = plat_.programInterface;

    plat_.program = MlnCreateComputeProgram(deviceMln.GetMlnDevice(), deviceMln.GetMlnProgramCache(), &computeState);
    MLN_LOG_GRAPH("CompPSO pc=%u iface=%p => %s", pipelineLayout.pushConstant.byteSize,
        reinterpret_cast<void*>(plat_.programInterface), plat_.program ? "OK" : "FAILED");
}

ComputePipelineStateObjectMln::~ComputePipelineStateObjectMln()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    if (plat_.program) {
        MlnDestroyProgram(mlnDevice, plat_.program);
    }
    if (plat_.programInterface) {
        MlnDestroyProgramInterface(mlnDevice, plat_.programInterface);
    }
    for (auto& bl : plat_.bindingLayouts) {
        if (bl) {
            MlnDestroyBindingLayout(mlnDevice, bl);
        }
    }
}

const PipelineStateObjectPlatformDataMln& ComputePipelineStateObjectMln::GetPlatformData() const
{
    return plat_;
}

RENDER_END_NAMESPACE()
