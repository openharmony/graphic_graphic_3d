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

#include "render_node_post_process_interface_util.h"

#include <base/util/uid_util.h>
#include <core/property/property_handle_util.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

#include "datastore/render_data_store_render_post_processes.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr string_view CAMERA = "camera";
constexpr string_view INPUT = "input";
constexpr string_view OUTPUT = "output";
constexpr string_view INPUT_DEPTH = "depth";
constexpr string_view INPUT_VELOCITY = "velocity_normal";
constexpr string_view INPUT_HISTORY = "history";
constexpr string_view INPUT_HISTORY_NEXT = "history_next";

RenderPassDesc::RenderArea GetImageRenderArea(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle handle)
{
    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(handle);
    return { 0U, 0U, desc.width, desc.height };
}

void FillTmpImageDesc(GpuImageDesc& desc)
{
    desc.imageType = CORE_IMAGE_TYPE_2D;
    desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS |
                               EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
    desc.usageFlags =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    // don't want this multisampled even if the final output would be.
    desc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT;
    desc.layerCount = 1U;
    desc.mipCount = 1U;
}
} // namespace

void RenderNodePostProcessInterfaceUtil::Init(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    valid_ = false;
    allPostProcesses_ = {};

    ParseRenderNodeInputs();
    UpdateImageData();
    RegisterOutputs();
}

void RenderNodePostProcessInterfaceUtil::PreExecuteFrame()
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
        descriptorCounts.reserve(allPostProcesses_.postProcessNodeInstances.size() + 1U);

        for (const auto& ppNodeRef : allPostProcesses_.postProcessNodeInstances) {
            if (ppNodeRef.ppNode) {
                ppNodeRef.ppNode->InitNode(*renderNodeContextMgr_);
                descriptorCounts.push_back(ppNodeRef.ppNode->GetRenderDescriptorCounts());
            }
        }
        // add copy descriptor sets
        descriptorCounts.push_back(renderCopy_.GetRenderDescriptorCounts());

        // reset descriptor sets
        INodeContextDescriptorSetManager& dsMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        dsMgr.ResetAndReserve(descriptorCounts);

        renderCopy_.Init(*renderNodeContextMgr_);
    } else if (copyInitNeeded_) {
        // add copy descriptor sets
        DescriptorCounts descriptorCounts = renderCopy_.GetRenderDescriptorCounts();

        // reset descriptor sets
        INodeContextDescriptorSetManager& dsMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        dsMgr.ResetAndReserve(descriptorCounts);

        renderCopy_.Init(*renderNodeContextMgr_);
    }
    copyInitNeeded_ = false;

    RenderPassDesc::RenderArea renderArea =
        GetImageRenderArea(renderNodeContextMgr_->GetGpuResourceManager(), builtInVariables_.output);

    const RenderHandle output =
        RenderHandleUtil::IsValid(builtInVariables_.output) ? builtInVariables_.output : builtInVariables_.defOutput;
    const RenderHandle input =
        RenderHandleUtil::IsValid(builtInVariables_.input) ? builtInVariables_.input : builtInVariables_.defInput;

    PrepareIntermediateTargets(input, output);

    BindableImage biInput;
    biInput.handle = input;
    BindableImage biOutput;
    biOutput.handle = output;

    // pre-execute in correct order
    const auto ppCount = static_cast<uint32_t>(allPostProcesses_.postProcessNodeInstances.size());
    for (uint32_t ppIndex = 0; ppIndex < ppCount; ++ppIndex) {
        const auto& ppNodeRef = allPostProcesses_.postProcessNodeInstances[ppIndex];
        if (auto* ppNode = ppNodeRef.ppNode.get(); ppNode) {
            IPropertyHandle* inputProperties = ppNode->GetRenderInputProperties();
            CORE_NS::SetPropertyValue(inputProperties, "input", biInput);
            // offer additional available inputs
            if (RenderHandleUtil::IsValid(builtInVariables_.depth)) {
                CORE_NS::SetPropertyValue(inputProperties, "depth", BindableImage { builtInVariables_.depth });
            }
            if (RenderHandleUtil::IsValid(builtInVariables_.velocity)) {
                CORE_NS::SetPropertyValue(inputProperties, "velocity", BindableImage { builtInVariables_.velocity });
            }
            if (RenderHandleUtil::IsValid(builtInVariables_.history)) {
                CORE_NS::SetPropertyValue(inputProperties, "history", BindableImage { builtInVariables_.history });
            }
            if (RenderHandleUtil::IsValid(builtInVariables_.camera)) {
                CORE_NS::SetPropertyValue(inputProperties, "camera", BindableBuffer { builtInVariables_.camera });
            }

            IPropertyHandle* outProperties = ppNode->GetRenderOutputProperties();
            if (ppIndex < (ppCount - 1U)) {
                CORE_NS::SetPropertyValue(outProperties, "output", ti_.GetIntermediateImage(biInput.handle));
            } else {
                // try to force the final target for the final post process
                // the effect might not accept this and then we need an extra blit in the end
                CORE_NS::SetPropertyValue(outProperties, "output", biOutput);
            }

            ppNode->SetRenderAreaRequest({ renderArea });
            ppNode->PreExecuteFrame();

            if ((ppNode->GetExecuteFlags() == 0U)) {
                // take output and route to next input
                biInput = CORE_NS::GetPropertyValue<BindableImage>(outProperties, "output");
            }
        }
    }
}

IRenderNode::ExecuteFlags RenderNodePostProcessInterfaceUtil::GetExecuteFlags() const
{
    if (valid_ && (!allPostProcesses_.pipeline.postProcesses.empty())) {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DEFAULT;
    }
    return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
}

void RenderNodePostProcessInterfaceUtil::ExecuteFrame(IRenderCommandList& cmdList)
{
    BindableImage biInput;
    biInput.handle =
        RenderHandleUtil::IsValid(builtInVariables_.input) ? builtInVariables_.input : builtInVariables_.defInput;
    BindableImage biOutput;
    biOutput.handle =
        RenderHandleUtil::IsValid(builtInVariables_.output) ? builtInVariables_.output : builtInVariables_.defOutput;
    const auto ppCount = static_cast<uint32_t>(allPostProcesses_.postProcessNodeInstances.size());

    // execute in correct order
    IRenderPostProcessNode* lastNode = nullptr;
    for (uint32_t ppIndex = 0; ppIndex < ppCount; ++ppIndex) {
        const auto& ppNodeRef = allPostProcesses_.postProcessNodeInstances[ppIndex];
        if (auto* ppNode = ppNodeRef.ppNode.get(); ppNode && (ppNode->GetExecuteFlags() == 0U)) {
            ppNode->ExecuteFrame(cmdList);
            lastNode = ppNode;
        }
    }
    if (lastNode) {
        biInput = CORE_NS::GetPropertyValue<BindableImage>(lastNode->GetRenderOutputProperties(), "output");
    }
    if (biInput.handle != biOutput.handle) {
        IRenderNodeCopyUtil::CopyInfo copyInfo;
        copyInfo.input = biInput;
        copyInfo.output = biOutput;
        renderCopy_.PreExecute();
        renderCopy_.Execute(cmdList, copyInfo);
    }
}

void RenderNodePostProcessInterfaceUtil::SetInfo(const IRenderNodePostProcessInterfaceUtil::Info& info)
{
    dataStorePrefix_ = info.dataStorePrefix;
    if (RenderHandleUtil::IsValid(info.imageData.input.handle)) {
        builtInVariables_.input = info.imageData.input.handle;
    }
    if (RenderHandleUtil::IsValid(info.imageData.output.handle)) {
        builtInVariables_.output = info.imageData.output.handle;
    }
}

void RenderNodePostProcessInterfaceUtil::RegisterOutputs()
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

bool operator==(const IRenderDataStoreRenderPostProcesses::PostProcessData& lhs,
    const IRenderDataStoreRenderPostProcesses::PostProcessData& rhs) noexcept
{
    return lhs.id == rhs.id && lhs.postProcess == rhs.postProcess;
}

void RenderNodePostProcessInterfaceUtil::ProcessPostProcessConfiguration()
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        dataStoreFullName_ = dataStorePrefix_ + jsonInputs_.renderDataStore.dataStoreName;
        if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(dataStoreFullName_)) {
            if (jsonInputs_.renderDataStore.typeName == RenderDataStoreRenderPostProcesses::TYPE_NAME) {
                auto* const dataStore = static_cast<const IRenderDataStoreRenderPostProcesses*>(ds);
                allPostProcesses_.prevPipeline = allPostProcesses_.pipeline;
                allPostProcesses_.pipeline = dataStore->GetData(jsonInputs_.renderDataStore.configurationName);
                valid_ = true;
            }
        }
    }

    // if previous and current effects are the same no need to do anything.
    if ((allPostProcesses_.pipeline.postProcesses.size() == allPostProcesses_.prevPipeline.postProcesses.size()) &&
        std::equal(allPostProcesses_.pipeline.postProcesses.cbegin(), allPostProcesses_.pipeline.postProcesses.cend(),
            allPostProcesses_.prevPipeline.postProcesses.cbegin())) {
        return;
    }

    // process new instances
    auto* renderClassFactory = renderNodeContextMgr_->GetRenderContext().GetInterface<IClassFactory>();
    if (!valid_ || !renderClassFactory) {
        return;
    }

    auto& pp = allPostProcesses_;
    // if the list is empty clear node instances and mark render node copy to be reinitialized.
    if (pp.pipeline.postProcesses.empty()) {
        pp.postProcessNodeInstances.clear();
        copyInitNeeded_ = true;
        return;
    }

    // copy of the current nodes so instances can be reused if the effect is still in the list, but in a different
    // location.
    auto postProcessNodeInstances = pp.postProcessNodeInstances;
    pp.postProcessNodeInstances.resize(pp.pipeline.postProcesses.size());
    auto nodePos = pp.postProcessNodeInstances.begin();
    for (const auto& ppRef : pp.pipeline.postProcesses) {
        if (!ppRef.postProcess) {
            nodePos->ppNode.reset();
            ++nodePos;
            continue;
        }
        auto pos = std::find(pp.prevPipeline.postProcesses.cbegin(), pp.prevPipeline.postProcesses.cend(), ppRef);
        if (pos != pp.prevPipeline.postProcesses.cend()) {
            const auto index = size_t(pos - pp.prevPipeline.postProcesses.cbegin());
            *nodePos = BASE_NS::move(postProcessNodeInstances[index]);
        } else {
            auto node = IRenderPostProcessNode::Ptr { ppRef.postProcess->GetInterface<IRenderPostProcessNode>() };
            if (!node) {
                node = CreateInstance<IRenderPostProcessNode>(
                    *renderClassFactory, ppRef.postProcess->GetRenderPostProcessNodeUid());
            }
            if (node) {
                *nodePos = { ppRef.id, BASE_NS::move(node) };
                pp.newPostProcesses = true; // flag for init and descriptor set management
            } else {
                nodePos->ppNode.reset();
                PLUGIN_LOG_ONCE_W("pp_node_not_found_" + to_string(ppRef.id), "Post process node not found (uid:%s)",
                    to_string(ppRef.postProcess->GetRenderPostProcessNodeUid()).c_str());
            }
        }
        ++nodePos;
    }
}

void RenderNodePostProcessInterfaceUtil::ParseRenderNodeInputs()
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
                PLUGIN_LOG_W("RenderNodeRenderPostProcessesInterfaceUtil default input/output image not supported (%s)",
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
        } else if (ref.usageName == INPUT_DEPTH) {
            jsonInputs_.depthIndex = idx;
        } else if (ref.usageName == INPUT_VELOCITY) {
            jsonInputs_.velocityIndex = idx;
        } else if (ref.usageName == INPUT_HISTORY) {
            jsonInputs_.historyIndex = idx;
        } else if (ref.usageName == INPUT_HISTORY_NEXT) {
            jsonInputs_.historyNextIndex = idx;
        }
    }
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customOutputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customOutputImages[idx];
        if (ref.usageName == OUTPUT) {
            jsonInputs_.outputIdx = idx;
        }
    }
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customInputBuffers.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customInputBuffers[idx];
        if (ref.usageName == CAMERA) {
            jsonInputs_.cameraIdx = idx;
        }
    }
}

void RenderNodePostProcessInterfaceUtil::UpdateImageData()
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
    // do not override if set with SetInfo()
    auto refreshHandleIfNeeded = [](RenderHandle& handle, uint32_t index, array_view<const RenderNodeResource> images) {
        if (!RenderHandleUtil::IsValid(handle)) {
            if (index < images.size()) {
                handle = images[index].handle;
            }
        }
    };
    refreshHandleIfNeeded(builtInVariables_.input, jsonInputs_.inputIdx, inputResources_.customInputImages);
    refreshHandleIfNeeded(builtInVariables_.depth, jsonInputs_.depthIndex, inputResources_.customInputImages);
    refreshHandleIfNeeded(builtInVariables_.velocity, jsonInputs_.velocityIndex, inputResources_.customInputImages);
    refreshHandleIfNeeded(builtInVariables_.history, jsonInputs_.historyIndex, inputResources_.customInputImages);
    refreshHandleIfNeeded(
        builtInVariables_.historyNext, jsonInputs_.historyNextIndex, inputResources_.customInputImages);
    refreshHandleIfNeeded(builtInVariables_.output, jsonInputs_.outputIdx, inputResources_.customOutputImages);
    refreshHandleIfNeeded(builtInVariables_.camera, jsonInputs_.cameraIdx, inputResources_.customInputBuffers);

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

void RenderNodePostProcessInterfaceUtil::PrepareIntermediateTargets(RenderHandle input, RenderHandle output)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const GpuImageDesc outputDesc = gpuResourceMgr.GetImageDescriptor(output);

    const bool sizeChanged = (outputDesc.width != outputSize_.x) || (outputDesc.height != outputSize_.y);
    if (sizeChanged) {
        outputSize_ = { Math::max(1U, outputDesc.width), Math::max(1U, outputDesc.height) };
    }
    if (sizeChanged || (!ti_.images[0U])) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_I(
            "RENDER_VALIDATION: post process temporary images re-created (size:%ux%u)", outputSize_.x, outputSize_.y);
#endif
        const GpuImageDesc intputDesc = gpuResourceMgr.GetImageDescriptor(input);
        GpuImageDesc tmpDesc = outputDesc;
        FillTmpImageDesc(tmpDesc);
        tmpDesc.format = intputDesc.format;
        ti_.images[0U] = gpuResourceMgr.Create(ti_.images[0U], tmpDesc);
        ti_.images[1U] = gpuResourceMgr.Create(ti_.images[1U], tmpDesc);
        ti_.imageCount = 2U;
    }
}

void RenderNodePostProcessInterfaceUtilImpl::Init(IRenderNodeContextManager& renderNodeContextMgr)
{
    rn_.Init(renderNodeContextMgr);
}

void RenderNodePostProcessInterfaceUtilImpl::PreExecute()
{
    rn_.PreExecuteFrame();
}

void RenderNodePostProcessInterfaceUtilImpl::Execute(IRenderCommandList& cmdList)
{
    rn_.ExecuteFrame(cmdList);
}

void RenderNodePostProcessInterfaceUtilImpl::SetInfo(const Info& info)
{
    rn_.SetInfo(info);
}

const IInterface* RenderNodePostProcessInterfaceUtilImpl::GetInterface(const Uid& uid) const
{
    if ((uid == IRenderNodePostProcessInterfaceUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* RenderNodePostProcessInterfaceUtilImpl::GetInterface(const Uid& uid)
{
    if ((uid == IRenderNodePostProcessInterfaceUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderNodePostProcessInterfaceUtilImpl::Ref()
{
    refCount_++;
}

void RenderNodePostProcessInterfaceUtilImpl::Unref()
{
    if (--refCount_ == 0) {
        delete this;
    }
}
RENDER_END_NAMESPACE()
