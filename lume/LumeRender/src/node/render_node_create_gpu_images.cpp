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

#include "render_node_create_gpu_images.h"

#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/render_data_structures.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
RenderNodeCreateGpuImages::DependencyList GetDependencyList(
    uint32_t dependencyFlags, const float dependencySizeScale, const GpuImageDesc& desc)
{
    using DependencyFlagBits = RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits;
    RenderNodeCreateGpuImages::DependencyList depList;
    if (dependencyFlags == 0) {
        constexpr GpuImageDesc defDesc {};
        if (desc.format == defDesc.format) {
            depList.format = true;
        }
        if ((desc.width == defDesc.width) && (desc.height == defDesc.height) && (desc.depth == defDesc.depth)) {
            depList.size = true;
        }
        if (desc.mipCount == defDesc.mipCount) {
            depList.mipCount = true;
        }
        if (desc.layerCount == defDesc.layerCount) {
            depList.layerCount = true;
        }
        if (desc.sampleCountFlags == defDesc.sampleCountFlags) {
            depList.sampleCount = true;
        }
    } else {
        if (dependencyFlags & DependencyFlagBits::FORMAT) {
            depList.format = true;
        }
        if (dependencyFlags & DependencyFlagBits::SIZE) {
            depList.size = true;
            depList.sizeScale = dependencySizeScale;
        }
        if (dependencyFlags & DependencyFlagBits::MIP_COUNT) {
            depList.mipCount = true;
        }
        if (dependencyFlags & DependencyFlagBits::LAYER_COUNT) {
            depList.layerCount = true;
        }
        if (dependencyFlags & DependencyFlagBits::SAMPLES) {
            depList.sampleCount = true;
        }
    }
    return depList;
}

bool CheckForDescUpdates(const GpuImageDesc& dependencyDesc,
    const RenderNodeCreateGpuImages::DependencyList& dependencyList, GpuImageDesc& desc)
{
    bool needsUpdate = false;
    if (dependencyList.format && (desc.format != dependencyDesc.format)) {
        needsUpdate = true;
        desc.format = dependencyDesc.format;
    }
    if (dependencyList.size) {
        // compare with dependency size scale
        const uint32_t sWidth =
            Math::max(1u, static_cast<uint32_t>(static_cast<float>(dependencyDesc.width) * dependencyList.sizeScale));
        const uint32_t sHeight =
            Math::max(1u, static_cast<uint32_t>(static_cast<float>(dependencyDesc.height) * dependencyList.sizeScale));
        const uint32_t sDepth =
            Math::max(1u, static_cast<uint32_t>(static_cast<float>(dependencyDesc.depth) * dependencyList.sizeScale));
        if ((desc.width != sWidth) || (desc.height != sHeight) || (desc.depth != sDepth)) {
            needsUpdate = true;
            desc.width = sWidth;
            desc.height = sHeight;
            desc.depth = sDepth;
        }
    }
    if (dependencyList.mipCount && (desc.mipCount != dependencyDesc.mipCount)) {
        needsUpdate = true;
        desc.mipCount = dependencyDesc.mipCount;
    }
    if (dependencyList.layerCount && (desc.layerCount != dependencyDesc.layerCount)) {
        needsUpdate = true;
        desc.layerCount = dependencyDesc.layerCount;
    }
    if (dependencyList.sampleCount && (desc.sampleCountFlags != dependencyDesc.sampleCountFlags)) {
        needsUpdate = true;
        desc.sampleCountFlags = dependencyDesc.sampleCountFlags;
    }
    return needsUpdate;
}

void CheckFormat(const IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view nodeName, GpuImageDesc& desc)
{
    // minimal check with just 2 back-up formats (one for color and one for depth)
    FormatFeatureFlags neededFormatFeatureFlags = 0;
    if (desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        neededFormatFeatureFlags |= CORE_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    } else if (desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_DST_BIT) {
        neededFormatFeatureFlags |= CORE_FORMAT_FEATURE_TRANSFER_DST_BIT;
    } else if (desc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT) {
        neededFormatFeatureFlags |= CORE_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    } else if (desc.usageFlags & CORE_IMAGE_USAGE_STORAGE_BIT) {
        neededFormatFeatureFlags |= CORE_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    } else if (desc.usageFlags & CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        neededFormatFeatureFlags |= CORE_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    } else if (desc.usageFlags & CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        neededFormatFeatureFlags |= CORE_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    const FormatProperties formatProperties = gpuResourceMgr.GetFormatProperties(desc.format);
    const FormatFeatureFlags formatFeatureFlags = (desc.imageTiling == CORE_IMAGE_TILING_LINEAR)
                                                      ? formatProperties.linearTilingFeatures
                                                      : formatProperties.optimalTilingFeatures;
    if ((neededFormatFeatureFlags & formatFeatureFlags) != neededFormatFeatureFlags) {
        const Format backupFormat = (neededFormatFeatureFlags & CORE_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                                        ? Format::BASE_FORMAT_D32_SFLOAT
                                        : Format::BASE_FORMAT_R8G8B8A8_UNORM;
        PLUGIN_LOG_W("Format flags not supported for format: %u, in render node %s, backup format: %u",
            static_cast<uint32_t>(desc.format), nodeName.data(), static_cast<uint32_t>(backupFormat));
        desc.format = backupFormat;
    }
}

inline constexpr Size2D LocalClamp(const Size2D val, const Size2D minVal, const Size2D maxVal)
{
    return Size2D { Math::max(minVal.width, Math::min(val.width, maxVal.width)),
        Math::max(minVal.height, Math::min(val.height, maxVal.height)) };
}
} // namespace

void RenderNodeCreateGpuImages::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    if (jsonInputs_.gpuImageDescs.empty()) {
        PLUGIN_LOG_W("RenderNodeCreateGpuImages: No gpu image descs given");
    }

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    descs_.reserve(jsonInputs_.gpuImageDescs.size());
    dependencyHandles_.reserve(jsonInputs_.gpuImageDescs.size());
    dependencyList_.reserve(jsonInputs_.gpuImageDescs.size());
    shadingRateTexelSizes_.reserve(jsonInputs_.gpuImageDescs.size());
    for (const auto& ref : jsonInputs_.gpuImageDescs) {
        GpuImageDesc desc = ref.desc;
        if (desc.format != Format::BASE_FORMAT_UNDEFINED) {
            CheckFormat(gpuResourceMgr, renderNodeContextMgr_->GetName(), desc);
        }

        RenderHandle dependencyHandle;
        DependencyList dependencyList;
        if (!ref.dependencyImageName.empty()) {
            dependencyHandle = gpuResourceMgr.GetImageHandle(ref.dependencyImageName);
            if (RenderHandleUtil::IsValid(dependencyHandle)) {
                const GpuImageDesc dependencyDesc = gpuResourceMgr.GetImageDescriptor(dependencyHandle);
                dependencyList = GetDependencyList(ref.dependencyFlags, ref.dependencySizeScale, desc);

                // update desc
                CheckForDescUpdates(dependencyDesc, dependencyList, desc);
            } else {
                PLUGIN_LOG_E("GpuImage dependency name not found: %s", ref.dependencyImageName.c_str());
            }
        }
        dependencyHandles_.push_back(dependencyHandle);
        dependencyList_.push_back(dependencyList);
        shadingRateTexelSizes_.push_back(GetClampedShadingRateTexelSize(ref.shadingRateTexelSize));

        names_.push_back({ string(ref.name), string(ref.shareName) });
        descs_.push_back(desc);

        // NOTE: shading rate is not in the desc
        desc.width = static_cast<uint32_t>(Math::ceil(float(desc.width) / float(shadingRateTexelSizes_.back().width)));
        desc.height =
            static_cast<uint32_t>(Math::ceil(float(desc.height) / float(shadingRateTexelSizes_.back().height)));
        resourceHandles_.push_back(gpuResourceMgr.Create(ref.name, desc));
    }

    // broadcast the resources
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        rngShareMgr.RegisterRenderNodeOutput(names_[idx].shareName, resourceHandles_[idx].GetHandle());
    }
}

void RenderNodeCreateGpuImages::PreExecuteFrame()
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        const RenderHandle dependencyHandle = dependencyHandles_[idx];
        if (gpuResourceMgr.IsValid(dependencyHandle)) {
            const GpuImageDesc& dependencyDesc = gpuResourceMgr.GetImageDescriptor(dependencyHandle);
            const DependencyList& dependencyList = dependencyList_[idx];
            GpuImageDesc& descRef = descs_[idx];

            const bool recreateImage = CheckForDescUpdates(dependencyDesc, dependencyList, descRef);
            if (recreateImage) {
                descs_[idx] = descRef;
                // NOTE: shading rate is not in the desc
                const Size2D shadingRateTexelSize = shadingRateTexelSizes_[idx];
                descRef.width =
                    static_cast<uint32_t>(Math::ceil(float(descRef.width) / float(shadingRateTexelSize.width)));
                descRef.height =
                    static_cast<uint32_t>(Math::ceil(float(descRef.height) / float(shadingRateTexelSize.height)));
                // replace the handle
                resourceHandles_[idx] = gpuResourceMgr.Create(resourceHandles_[idx], descRef);
            }
        }
    }

    // broadcast the resources
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        rngShareMgr.RegisterRenderNodeOutput(names_[idx].shareName, resourceHandles_[idx].GetHandle());
    }
}

Size2D RenderNodeCreateGpuImages::GetClampedShadingRateTexelSize(const Size2D& shadingRateTexelSize)
{
    Size2D srts = { 1u, 1u };
    if ((shadingRateTexelSize.width > 1u) || (shadingRateTexelSize.height > 1u)) {
        const IDevice& device = renderNodeContextMgr_->GetRenderContext().GetDevice();
        auto fsrProps = device.GetCommonDeviceProperties().fragmentShadingRateProperties;
        fsrProps.minFragmentShadingRateAttachmentTexelSize.width =
            Math::max(1U, fsrProps.minFragmentShadingRateAttachmentTexelSize.width);
        fsrProps.minFragmentShadingRateAttachmentTexelSize.height =
            Math::max(1U, fsrProps.minFragmentShadingRateAttachmentTexelSize.height);
        fsrProps.maxFragmentShadingRateAttachmentTexelSize.width =
            Math::max(1U, fsrProps.maxFragmentShadingRateAttachmentTexelSize.width);
        fsrProps.maxFragmentShadingRateAttachmentTexelSize.height =
            Math::max(1U, fsrProps.maxFragmentShadingRateAttachmentTexelSize.height);
        srts = LocalClamp(shadingRateTexelSize, fsrProps.minFragmentShadingRateAttachmentTexelSize,
            fsrProps.maxFragmentShadingRateAttachmentTexelSize);
    }
    return srts;
}

void RenderNodeCreateGpuImages::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.gpuImageDescs = parserUtil.GetGpuImageDescs(jsonVal, "gpuImageDescs");
}

// for plugin / factory interface
IRenderNode* RenderNodeCreateGpuImages::Create()
{
    return new RenderNodeCreateGpuImages();
}

void RenderNodeCreateGpuImages::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCreateGpuImages*>(instance);
}
RENDER_END_NAMESPACE()
