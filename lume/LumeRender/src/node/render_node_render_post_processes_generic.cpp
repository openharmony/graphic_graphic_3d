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

#include "render_node_render_post_processes_generic.h"

#include <base/math/mathf.h>
#include <base/util/uid_util.h>
#include <core/plugin/intf_class_factory.h>
#include <core/property/property_handle_util.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/property/property_types.h>

#include "datastore/render_data_store_render_post_processes.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr string_view INPUT = "input";
constexpr string_view OUTPUT = "output";

RenderPassDesc::RenderArea GetImageRenderArea(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle handle)
{
    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(handle);
    return { 0U, 0U, desc.width, desc.height };
}
} // namespace

void RenderNodeRenderPostProcessesGeneric::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    valid_ = false;
    allPostProcesses_ = {};

    ParseRenderNodeInputs();
    UpdateImageData();
    RegisterOutputs();
}

void RenderNodeRenderPostProcessesGeneric::PreExecuteFrame()
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    UpdateImageData();
    ProcessPostProcessConfiguration();
    RegisterOutputs();

    // possible inits called here
    if (allPostProcesses_.newPostProcesses) {
        allPostProcesses_.newPostProcesses = false;

        vector<DescriptorCounts> descriptorCounts;
        descriptorCounts.reserve(allPostProcesses_.postProcessNodeInstances.size());

        for (const auto& ppNodeRef : allPostProcesses_.postProcessNodeInstances) {
            if (ppNodeRef.ppNode) {
                const IRenderPostProcess::Ptr* rpp = nullptr;
                for (const auto& ppRef : allPostProcesses_.pipeline.postProcesses) {
                    // match the id for client-side properties
                    if (ppNodeRef.id == ppRef.id) {
                        rpp = &ppRef.postProcess;
                        break;
                    }
                }
                if (rpp) {
                    ppNodeRef.ppNode->Init(*rpp, *renderNodeContextMgr_);
                    descriptorCounts.push_back(ppNodeRef.ppNode->GetRenderDescriptorCounts());
                }
            }
        }
        // add copy descriptor sets
        descriptorCounts.push_back(renderCopy_.GetRenderDescriptorCounts());

        // reset descriptor sets
        INodeContextDescriptorSetManager& dsMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        dsMgr.ResetAndReserve(descriptorCounts);

        renderCopy_.Init(*renderNodeContextMgr_);
    } else if (copyInitNeeded_) {
        vector<DescriptorCounts> descriptorCounts;

        // add copy descriptor sets
        descriptorCounts.push_back(renderCopy_.GetRenderDescriptorCounts());

        // reset descriptor sets
        INodeContextDescriptorSetManager& dsMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        dsMgr.ResetAndReserve(descriptorCounts);

        renderCopy_.Init(*renderNodeContextMgr_);
    }
    copyInitNeeded_ = false;

    RenderPassDesc::RenderArea renderArea =
        GetImageRenderArea(renderNodeContextMgr_->GetGpuResourceManager(), builtInVariables_.output);
    allPostProcesses_.postProcessCount = 0U;

    const RenderHandle output =
        RenderHandleUtil::IsValid(builtInVariables_.output) ? builtInVariables_.output : builtInVariables_.defOutput;
    BindableImage biInput;
    biInput.handle =
        RenderHandleUtil::IsValid(builtInVariables_.input) ? builtInVariables_.input : builtInVariables_.defInput;
    BindableImage biOutput;
    biOutput.handle = output;

    // pre-execute in correct order
    const auto ppCount = static_cast<uint32_t>(allPostProcesses_.pipeline.postProcesses.size());
    for (uint32_t ppIndex = 0; ppIndex < ppCount; ++ppIndex) {
        const auto& ppRef = allPostProcesses_.pipeline.postProcesses[ppIndex];
        for (auto& ppNodeRef : allPostProcesses_.postProcessNodeInstances) {
            if (ppNodeRef.ppNode && (ppNodeRef.id == ppRef.id)) {
                auto& ppNode = *ppNodeRef.ppNode;

                CORE_NS::SetPropertyValue(ppNode.GetRenderInputProperties(), "input", biInput);
                // try to force the final target for the final post process
                // the effect might not accept this and then we need an extra blit in the end
                if (ppIndex == (ppCount - 1U)) {
                    CORE_NS::SetPropertyValue(ppNode.GetRenderOutputProperties(), "output", biOutput);
                }

                ppNodeRef.ppNode->SetRenderAreaRequest({ renderArea });
                ppNodeRef.ppNode->PreExecute();

                // take output and route to next input
                biInput = CORE_NS::GetPropertyValue<BindableImage>(ppNode.GetRenderOutputProperties(), "output");

                allPostProcesses_.postProcessCount++;
                break;
            }
        }
    }
}

IRenderNode::ExecuteFlags RenderNodeRenderPostProcessesGeneric::GetExecuteFlags() const
{
    if (valid_ && (!allPostProcesses_.pipeline.postProcesses.empty())) {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DEFAULT;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderNodeRenderPostProcessesGeneric::ExecuteFrame(IRenderCommandList& cmdList)
{
    const RenderHandle output =
        RenderHandleUtil::IsValid(builtInVariables_.output) ? builtInVariables_.output : builtInVariables_.defOutput;
    BindableImage biInput;
    biInput.handle =
        RenderHandleUtil::IsValid(builtInVariables_.input) ? builtInVariables_.input : builtInVariables_.defInput;
    BindableImage biOutput;
    biOutput.handle = output;
    const auto ppCount = static_cast<uint32_t>(allPostProcesses_.pipeline.postProcesses.size());
    // execute in correct order
    for (uint32_t ppIndex = 0; ppIndex < ppCount; ++ppIndex) {
        const auto& ppRef = allPostProcesses_.pipeline.postProcesses[ppIndex];
        for (auto& ppNodeRef : allPostProcesses_.postProcessNodeInstances) {
            if (ppNodeRef.ppNode && (ppNodeRef.id == ppRef.id) && (ppNodeRef.ppNode->GetExecuteFlags() == 0U)) {
                auto& ppNode = *ppNodeRef.ppNode;

                CORE_NS::SetPropertyValue(ppNode.GetRenderInputProperties(), "input", biInput);
                // try to force the final target for the final post process
                // the effect might not accept this and then we need an extra blit in the end
                if (ppIndex == (allPostProcesses_.postProcessCount - 1U)) {
                    CORE_NS::SetPropertyValue(ppNode.GetRenderOutputProperties(), "output", biOutput);
                }

                ppNode.Execute(cmdList);

                // take output and route to next input
                biInput = CORE_NS::GetPropertyValue<BindableImage>(ppNode.GetRenderOutputProperties(), "output");
            }
        }
    }

    if (biInput.handle != biOutput.handle) {
        IRenderNodeCopyUtil::CopyInfo copyInfo;
        copyInfo.input = biInput;
        copyInfo.output = biOutput;
        renderCopy_.PreExecute();
        renderCopy_.Execute(cmdList, copyInfo);
    }
}

void RenderNodeRenderPostProcessesGeneric::RegisterOutputs()
{
    const RenderHandle output = builtInVariables_.output;
    IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
    RenderHandle registerOutput;
    if (valid_) {
        if (RenderHandleUtil::IsValid(output)) {
            registerOutput = output;
        }
    }
    if (!RenderHandleUtil::IsValid(registerOutput)) {
        if (((jsonInputs_.defaultOutputImage == DefaultInOutImage::OUTPUT) ||
                (jsonInputs_.defaultOutputImage == DefaultInOutImage::INPUT_OUTPUT_COPY)) &&
            RenderHandleUtil::IsValid(output)) {
            registerOutput = output;
        } else if ((jsonInputs_.defaultOutputImage == DefaultInOutImage::INPUT) &&
                   RenderHandleUtil::IsValid(builtInVariables_.input)) {
            registerOutput = builtInVariables_.input;
        } else {
            registerOutput = builtInVariables_.defOutput;
        }
    }
    shrMgr.RegisterRenderNodeOutput("output", registerOutput);
}

void RenderNodeRenderPostProcessesGeneric::ProcessPostProcessConfiguration()
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName); ds) {
            if (jsonInputs_.renderDataStore.typeName == RenderDataStoreRenderPostProcesses::TYPE_NAME) {
                auto* const dataStore = static_cast<const IRenderDataStoreRenderPostProcesses*>(ds);
                allPostProcesses_.pipeline = dataStore->GetData(jsonInputs_.renderDataStore.configurationName);
                valid_ = true;
            }
        }
    }
    // process new instances
    auto* renderClassFactory = renderNodeContextMgr_->GetRenderContext().GetInterface<IClassFactory>();
    if (valid_ && renderClassFactory) {
        auto& pp = allPostProcesses_;
        for (const auto& ppRef : pp.pipeline.postProcesses) {
            if (!ppRef.postProcess) {
                continue;
            }
            bool createNew = true;
            for (const auto& iRef : pp.postProcessNodeInstances) {
                if (iRef.id == ppRef.id) {
                    createNew = false;
                    break;
                }
            }
            if (createNew) {
                IRenderPostProcessNode::Ptr rppn = CreateInstance<IRenderPostProcessNode>(
                    *renderClassFactory, ppRef.postProcess->GetRenderPostProcessNodeUid());
                if (rppn) {
                    pp.postProcessNodeInstances.push_back({ ppRef.id, rppn });
                    pp.newPostProcesses = true; // flag for init and descriptor set management
                } else {
                    PLUGIN_LOG_ONCE_W("pp_node_not_found_" + to_string(ppRef.id),
                        "Post process node not found (uid:%s)",
                        to_string(ppRef.postProcess->GetRenderPostProcessNodeUid()).c_str());
                }
            }
        }
    }
}

void RenderNodeRenderPostProcessesGeneric::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

#if (RENDER_VALIDATION_ENABLED == 1)
    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_W("RENDER_VALIDATION: RN %s renderDataStore::dataStoreName missing.",
            renderNodeContextMgr_->GetName().data());
    }
    if (jsonInputs_.renderDataStore.configurationName.empty()) {
        PLUGIN_LOG_W("RENDER_VALIDATION: RN %s postProcess name missing.", renderNodeContextMgr_->GetName().data());
    }
#endif

    auto GetDefaultInOutImage = [](const string& ioStr, const DefaultInOutImage defaultImg) {
        DefaultInOutImage defImg = defaultImg;
        if (!ioStr.empty()) {
            if (ioStr == "output") {
                defImg = DefaultInOutImage::OUTPUT;
            } else if (ioStr == "input_output_copy") {
                defImg = DefaultInOutImage::INPUT_OUTPUT_COPY;
            } else if (ioStr == "input") {
                defImg = DefaultInOutImage::INPUT;
            } else if (ioStr == "black") {
                defImg = DefaultInOutImage::BLACK;
            } else if (ioStr == "white") {
                defImg = DefaultInOutImage::WHITE;
            } else {
                PLUGIN_LOG_W("RenderNodeRenderPostProcessesGeneric default input/output image not supported (%s)",
                    ioStr.c_str());
            }
        }
        return defImg;
    };
    const auto defaultOutput = parserUtil.GetStringValue(jsonVal, "defaultOutputImage");
    jsonInputs_.defaultOutputImage = GetDefaultInOutImage(defaultOutput, jsonInputs_.defaultOutputImage);
    const auto defaultInput = parserUtil.GetStringValue(jsonVal, "defaultInputImage");
    jsonInputs_.defaultInputImage = GetDefaultInOutImage(defaultOutput, jsonInputs_.defaultInputImage);

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);

    // process custom resources
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customInputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customInputImages[idx];
        if (ref.usageName == INPUT) {
            jsonInputs_.inputIdx = idx;
        }
    }
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customOutputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customOutputImages[idx];
        if (ref.usageName == OUTPUT) {
            jsonInputs_.outputIdx = idx;
        }
    }
}

void RenderNodeRenderPostProcessesGeneric::UpdateImageData()
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBlackImage)) {
        builtInVariables_.defBlackImage =
            gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defWhiteImage)) {
        builtInVariables_.defWhiteImage =
            gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE_WHITE);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defSampler)) {
        builtInVariables_.defSampler =
            gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
    }
    if (jsonInputs_.inputIdx < inputResources_.customInputImages.size()) {
        builtInVariables_.input = inputResources_.customInputImages[jsonInputs_.inputIdx].handle;
    }
    if (jsonInputs_.outputIdx < inputResources_.customOutputImages.size()) {
        builtInVariables_.output = inputResources_.customOutputImages[jsonInputs_.outputIdx].handle;
    }

    // default in / out if not correct
    if (!RenderHandleUtil::IsValid(builtInVariables_.defInput)) {
        if (jsonInputs_.defaultInputImage == DefaultInOutImage::WHITE) {
            builtInVariables_.defInput = builtInVariables_.defWhiteImage;
        } else if (jsonInputs_.defaultInputImage == DefaultInOutImage::BLACK) {
            builtInVariables_.defInput = builtInVariables_.defBlackImage;
        }
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defOutput)) {
        if (jsonInputs_.defaultOutputImage == DefaultInOutImage::WHITE) {
            builtInVariables_.defOutput = builtInVariables_.defWhiteImage;
        } else if (jsonInputs_.defaultOutputImage == DefaultInOutImage::BLACK) {
            builtInVariables_.defOutput = builtInVariables_.defBlackImage;
        }
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeRenderPostProcessesGeneric::Create()
{
    return new RenderNodeRenderPostProcessesGeneric();
}

void RenderNodeRenderPostProcessesGeneric::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeRenderPostProcessesGeneric*>(instance);
}
RENDER_END_NAMESPACE()
