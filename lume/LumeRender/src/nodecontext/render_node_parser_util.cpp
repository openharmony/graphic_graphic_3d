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

#include "render_node_parser_util.h"

#include <cstdint>

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>

#include "device/gpu_resource_manager.h"
#include "device/shader_manager.h"
#include "loader/json_format_serialization.h"
#include "loader/json_util.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
// clang-format off
CORE_JSON_SERIALIZE_ENUM(RenderSlotSortType,
    {
        { RenderSlotSortType::NONE, "none" },
        { RenderSlotSortType::FRONT_TO_BACK, "front_to_back" },
        { RenderSlotSortType::BACK_TO_FRONT, "back_to_front" },
        { RenderSlotSortType::BY_MATERIAL, "by_material" },
    })

CORE_JSON_SERIALIZE_ENUM(RenderSlotCullType,
    {
        { RenderSlotCullType::NONE, "none" },
        { RenderSlotCullType::VIEW_FRUSTUM_CULL, "view_frustum_cull" },
    })

CORE_JSON_SERIALIZE_ENUM(GpuQueue::QueueType,
    { { GpuQueue::QueueType::UNDEFINED, nullptr }, { GpuQueue::QueueType::GRAPHICS, "graphics" },
        { GpuQueue::QueueType::COMPUTE, "compute" }, { GpuQueue::QueueType::TRANSFER, "transfer" } })

CORE_JSON_SERIALIZE_ENUM(DescriptorType,
    {
        { DescriptorType::CORE_DESCRIPTOR_TYPE_MAX_ENUM, nullptr },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER, "sampler" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "combined_image_sampler" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, "sampled_image" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, "storage_image" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, "uniform_texel_buffer" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, "storage_texel_buffer" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, "uniform_buffer" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, "storage_buffer" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "uniform_buffer_dynamic" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, "storage_buffer_dynamic" },
        { DescriptorType::CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, "input_attachment" },
    })

CORE_JSON_SERIALIZE_ENUM(AttachmentLoadOp,
    {
        { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD, "load" },
        { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR, "clear" },
        { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE, "dont_care" },
    })

CORE_JSON_SERIALIZE_ENUM(AttachmentStoreOp,
    {
        { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE, "store" },
        { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE, "dont_care" },
    })

CORE_JSON_SERIALIZE_ENUM(ImageType,
    {
        { ImageType::CORE_IMAGE_TYPE_2D, nullptr }, // default
        { ImageType::CORE_IMAGE_TYPE_1D, "1d" },
        { ImageType::CORE_IMAGE_TYPE_2D, "2d" },
        { ImageType::CORE_IMAGE_TYPE_3D, "3d" },
    })

CORE_JSON_SERIALIZE_ENUM(ImageViewType,
    {
        { ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, nullptr }, // default
        { ImageViewType::CORE_IMAGE_VIEW_TYPE_1D, "1d" }, { ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, "2d" },
        { ImageViewType::CORE_IMAGE_VIEW_TYPE_3D, "3d" }, { ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE, "cube" },
        { ImageViewType::CORE_IMAGE_VIEW_TYPE_1D_ARRAY, "1d_array" },
        { ImageViewType::CORE_IMAGE_VIEW_TYPE_2D_ARRAY, "2d_array" },
        { ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE_ARRAY, "cube_array" },
    })

CORE_JSON_SERIALIZE_ENUM(ImageTiling,
    {
        { ImageTiling::CORE_IMAGE_TILING_OPTIMAL, nullptr }, // default
        { ImageTiling::CORE_IMAGE_TILING_OPTIMAL, "optimal" },
        { ImageTiling::CORE_IMAGE_TILING_LINEAR, "linear" },
    })

CORE_JSON_SERIALIZE_ENUM(ImageUsageFlagBits,
    {
        { (ImageUsageFlagBits)0, nullptr },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_SRC_BIT, "transfer_src" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT, "transfer_dst" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT, "sampled" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_STORAGE_BIT, "storage" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "color_attachment" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "depth_stencil_attachment" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, "transient_attachment" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, "input_attachment" },
        { ImageUsageFlagBits::CORE_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT, "fragment_shading_rate_attachment" },
    })

CORE_JSON_SERIALIZE_ENUM(ImageCreateFlagBits,
    {
        { (ImageCreateFlagBits)0, nullptr },
        { ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, "cube_compatible" },
        { ImageCreateFlagBits::CORE_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT, "2d_array_compatible" },
    })

CORE_JSON_SERIALIZE_ENUM(EngineImageCreationFlagBits,
    {
        { (EngineImageCreationFlagBits)0, nullptr },
        { EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, "dynamic_barriers" },
        { EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS,
            "reset_state_on_frame_borders" },
        { EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS, "generate_mips" },
    })

CORE_JSON_SERIALIZE_ENUM(SampleCountFlagBits,
    {
        { (SampleCountFlagBits)0, nullptr },
        { SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, "1bit" },
        { SampleCountFlagBits::CORE_SAMPLE_COUNT_2_BIT, "2bit" },
        { SampleCountFlagBits::CORE_SAMPLE_COUNT_4_BIT, "4bit" },
        { SampleCountFlagBits::CORE_SAMPLE_COUNT_8_BIT, "8bit" },
        { SampleCountFlagBits::CORE_SAMPLE_COUNT_16_BIT, "16bit" },
        { SampleCountFlagBits::CORE_SAMPLE_COUNT_32_BIT, "32bit" },
        { SampleCountFlagBits::CORE_SAMPLE_COUNT_64_BIT, "64bit" },
    })

CORE_JSON_SERIALIZE_ENUM(MemoryPropertyFlagBits,
    {
        { (MemoryPropertyFlagBits)0, nullptr },
        { MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "device_local" },
        { MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "host_visible" },
        { MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT, "host_coherent" },
        { MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_CACHED_BIT, "host_cached" },
        { MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, "lazily_allocated" },
        { MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_PROTECTED_BIT, "protected" },
    })

CORE_JSON_SERIALIZE_ENUM(EngineBufferCreationFlagBits,
    {
        { (EngineBufferCreationFlagBits)0, nullptr },
        { EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS, "dynamic_barriers" },
    })

CORE_JSON_SERIALIZE_ENUM(BufferUsageFlagBits,
    {
        { (BufferUsageFlagBits)0, nullptr },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT, "transfer_src" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT, "transfer_dst" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, "uniform_texel" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "uniform" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT, "storage_texel" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT, "storage" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_INDEX_BUFFER_BIT, "index" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT, "vertex" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT, "indirect" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT, "shader_binding_table" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, "shader_device_address" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT, "acceleration_structure_build_input_read_only" },
        { BufferUsageFlagBits::CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT, "acceleration_structure_storage" },
    })

CORE_JSON_SERIALIZE_ENUM(RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits,
    {
        { RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits::MAX_DEPENDENCY_FLAG_ENUM, nullptr },
        { RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits::FORMAT, "format" },
        { RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits::SIZE, "size" },
        { RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits::MIP_COUNT, "mipCount" },
        { RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits::LAYER_COUNT, "layerCount" },
        { RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits::SAMPLES, "samples" },
    })

CORE_JSON_SERIALIZE_ENUM(RenderNodeGraphResourceLocationType,
    {
        { RenderNodeGraphResourceLocationType::DEFAULT, "default" },
        { RenderNodeGraphResourceLocationType::FROM_RENDER_GRAPH_INPUT, "from_render_graph_input" },
        { RenderNodeGraphResourceLocationType::FROM_RENDER_GRAPH_OUTPUT, "from_render_graph_output" },
        { RenderNodeGraphResourceLocationType::FROM_PREVIOUS_RENDER_NODE_OUTPUT, "from_previous_render_node_output" },
        { RenderNodeGraphResourceLocationType::FROM_NAMED_RENDER_NODE_OUTPUT, "from_named_render_node_output" },
        { RenderNodeGraphResourceLocationType::FROM_PREVIOUS_RENDER_NODE_GRAPH_OUTPUT, "from_previous_render_node_graph_output" },
    })

CORE_JSON_SERIALIZE_ENUM(ResolveModeFlagBits,
    {
        { ResolveModeFlagBits::CORE_RESOLVE_MODE_NONE, "none" },
        { ResolveModeFlagBits::CORE_RESOLVE_MODE_SAMPLE_ZERO_BIT, "sample_zero" },
        { ResolveModeFlagBits::CORE_RESOLVE_MODE_AVERAGE_BIT, "average" },
        { ResolveModeFlagBits::CORE_RESOLVE_MODE_MIN_BIT, "min" },
        { ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT, "max" },
    })

CORE_JSON_SERIALIZE_ENUM(SubpassContents,
    {
        { SubpassContents::CORE_SUBPASS_CONTENTS_INLINE, "inline" },
        { SubpassContents::CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS, "secondary_command_lists" },
    })
// clang-format on
inline void FromJson(const json::value& jsonData, JsonContext<RenderNodeGraphInputs::Resource>& context)
{
    SafeGetJsonValue(jsonData, "set", context.error, context.data.set);
    SafeGetJsonValue(jsonData, "binding", context.error, context.data.binding);
    SafeGetJsonValue(jsonData, "name", context.error, context.data.name);
    SafeGetJsonEnum(jsonData, "resourceLocation", context.error, context.data.resourceLocation);
    SafeGetJsonValue(jsonData, "resourceIndex", context.error, context.data.resourceIndex);
    SafeGetJsonValue(jsonData, "nodeName", context.error, context.data.nodeName);
    SafeGetJsonValue(jsonData, "usageName", context.error, context.data.usageName);
    SafeGetJsonValue(jsonData, "mip", context.error, context.data.mip);
    SafeGetJsonValue(jsonData, "layer", context.error, context.data.layer);
}

inline void FromJson(const json::value& jsonData, JsonContext<RenderNodeGraphInputs::Attachment>& context)
{
    SafeGetJsonEnum(jsonData, "loadOp", context.error, context.data.loadOp);
    SafeGetJsonEnum(jsonData, "storeOp", context.error, context.data.storeOp);
    SafeGetJsonEnum(jsonData, "stencilLoadOp", context.error, context.data.stencilLoadOp);
    SafeGetJsonEnum(jsonData, "stencilStoreOp", context.error, context.data.stencilStoreOp);
    if (auto const pos = jsonData.find("clearColor"); pos) {
        if (pos->is_array() && pos->array_.size() == 4) {
            FromJson(*pos, context.data.clearValue.color.float32);
        } else {
            const auto asString = to_string(*pos);
            context.error +=
                "clearColor must be an array of length 4 : (" + string_view(asString.data(), asString.size()) + ")\n";
        }
    }
    if (auto const pos = jsonData.find("clearDepth"); pos) {
        if (pos->is_array() && pos->array_.size() == 2) {
            if (pos->array_[0].is_number()) {
                context.data.clearValue.depthStencil.depth = pos->array_[0].as_number<float>();
            } else {
                const auto asString = to_string(*pos);
                context.error += "depthAttachment.clearValue[0] must be a float: (" +
                                 string_view(asString.data(), asString.size()) + ")\n";
            }

            if (pos->array_[1].is_number()) {
                context.data.clearValue.depthStencil.stencil = pos->array_[1].as_number<uint32_t>();
            } else {
                const auto asString = to_string(*pos);
                context.error += "depthAttachment.clearValue[1] must be an uint: (" +
                                 string_view(asString.data(), asString.size()) + ")\n";
            }
        } else {
            const auto asString = to_string(*pos);
            context.error += "Failed to read depthAttachment.clearValue, invalid datatype: (" +
                             string_view(asString.data(), asString.size()) + ")\n";
        }
    }
    SafeGetJsonValue(jsonData, "name", context.error, context.data.name);
    SafeGetJsonEnum(jsonData, "resourceLocation", context.error, context.data.resourceLocation);
    SafeGetJsonValue(jsonData, "resourceIndex", context.error, context.data.resourceIndex);
    SafeGetJsonValue(jsonData, "nodeName", context.error, context.data.nodeName);

    SafeGetJsonValue(jsonData, "mip", context.error, context.data.mip);
    SafeGetJsonValue(jsonData, "layer", context.error, context.data.layer);
}

inline void FromJson(
    const json::value& jsonData, JsonContext<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc>& context)
{
    SafeGetJsonValue(jsonData, "name", context.error, context.data.name);
    SafeGetJsonValue(jsonData, "shareName", context.error, context.data.shareName);
    SafeGetJsonValue(jsonData, "dependencyImageName", context.error, context.data.dependencyImageName);

    SafeGetJsonEnum(jsonData, "imageType", context.error, context.data.desc.imageType);
    SafeGetJsonEnum(jsonData, "imageViewType", context.error, context.data.desc.imageViewType);
    SafeGetJsonEnum(jsonData, "format", context.error, context.data.desc.format);
    SafeGetJsonEnum(jsonData, "imageTiling", context.error, context.data.desc.imageTiling);
    SafeGetJsonBitfield<ImageUsageFlagBits>(jsonData, "usageFlags", context.error, context.data.desc.usageFlags);
    SafeGetJsonBitfield<MemoryPropertyFlagBits>(
        jsonData, "memoryPropertyFlags", context.error, context.data.desc.memoryPropertyFlags);
    SafeGetJsonBitfield<ImageCreateFlagBits>(jsonData, "createFlags", context.error, context.data.desc.createFlags);
    SafeGetJsonBitfield<EngineImageCreationFlagBits>(
        jsonData, "engineCreationFlags", context.error, context.data.desc.engineCreationFlags);
    SafeGetJsonValue(jsonData, "width", context.error, context.data.desc.width);
    SafeGetJsonValue(jsonData, "height", context.error, context.data.desc.height);
    SafeGetJsonValue(jsonData, "depth", context.error, context.data.desc.depth);
    SafeGetJsonValue(jsonData, "mipCount", context.error, context.data.desc.mipCount);
    SafeGetJsonValue(jsonData, "layerCount", context.error, context.data.desc.layerCount);
    SafeGetJsonBitfield<SampleCountFlagBits>(
        jsonData, "sampleCountFlags", context.error, context.data.desc.sampleCountFlags);

    SafeGetJsonBitfield<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc::DependencyFlagBits>(
        jsonData, "dependencyFlags", context.error, context.data.dependencyFlags);
    SafeGetJsonValue(jsonData, "dependencySizeScale", context.error, context.data.dependencySizeScale);

    if (auto const pos = jsonData.find("shadingRateTexelSize"); pos) {
        if (pos->is_array() && pos->array_.size() == 2) {
            if (pos->array_[0].is_number() && pos->array_[1u].is_number()) {
                context.data.shadingRateTexelSize.width = pos->array_[0].as_number<uint32_t>();
                context.data.shadingRateTexelSize.height = pos->array_[1].as_number<uint32_t>();
            }
        }
    }

    if ((context.data.desc.format == Format::BASE_FORMAT_UNDEFINED) && context.data.dependencyImageName.empty()) {
        context.error += "undefined gpu image desc format\n";
    }
}

inline void FromJson(
    const json::value& jsonData, JsonContext<RenderNodeGraphInputs::RenderNodeGraphGpuBufferDesc>& context)
{
    SafeGetJsonValue(jsonData, "name", context.error, context.data.name);
    SafeGetJsonValue(jsonData, "shareName", context.error, context.data.shareName);
    SafeGetJsonValue(jsonData, "dependencyBufferName", context.error, context.data.dependencyBufferName);

    SafeGetJsonBitfield<BufferUsageFlagBits>(jsonData, "usageFlags", context.error, context.data.desc.usageFlags);
    SafeGetJsonBitfield<EngineBufferCreationFlagBits>(
        jsonData, "engineCreationFlags", context.error, context.data.desc.engineCreationFlags);
    SafeGetJsonBitfield<MemoryPropertyFlagBits>(
        jsonData, "memoryPropertyFlags", context.error, context.data.desc.memoryPropertyFlags);
    SafeGetJsonValue(jsonData, "byteSize", context.error, context.data.desc.byteSize);
}

inline void FromJson(const json::value& jsonData, JsonContext<DescriptorCounts::TypedCount>& context)
{
    SafeGetJsonEnum(jsonData, "type", context.error, context.data.type);
    SafeGetJsonValue(jsonData, "count", context.error, context.data.count);
}

namespace {
struct LoadResult {
    string error;
};

void ParseRenderpass(const string_view name, const json::value& node,
    RenderNodeGraphInputs::InputRenderPass& renderPass, LoadResult& result)
{
    if (auto const rp = node.find(name); rp) {
        ParseArray<decltype(renderPass.attachments)::value_type>(*rp, "attachments", renderPass.attachments, result);
        SafeGetJsonValue(*rp, "subpassIndex", result.error, renderPass.subpassIndex);
        SafeGetJsonValue(*rp, "subpassCount", result.error, renderPass.subpassCount);
        SafeGetJsonEnum(*rp, "subpassContents", result.error, renderPass.subpassContents);
#if (RENDER_VALIDATION_ENABLED == 1)
        if (renderPass.subpassIndex >= renderPass.subpassCount) {
            PLUGIN_LOG_E("RENDER_VALIDATION: render pass subpass index must be smaller than subpass index");
        }
#endif
        if (auto const sp = rp->find("subpass"); sp) {
            SafeGetJsonValue(*sp, "depthAttachmentIndex", result.error, renderPass.depthAttachmentIndex);
            SafeGetJsonValue(*sp, "depthResolveAttachmentIndex", result.error, renderPass.depthResolveAttachmentIndex);
            SafeGetJsonEnum(*sp, "depthResolveModeFlagBit", result.error, renderPass.depthResolveModeFlagBit);
            SafeGetJsonEnum(*sp, "stencilResolveModeFlagBit", result.error, renderPass.stencilResolveModeFlagBit);
            SafeGetJsonEnum(
                *sp, "fragmentShadingRateAttachmentIndex", result.error, renderPass.fragmentShadingRateAttachmentIndex);
#if (RENDER_VALIDATION_ENABLED == 1)
            if ((renderPass.depthResolveAttachmentIndex != ~0u) &&
                ((renderPass.depthResolveModeFlagBit | renderPass.stencilResolveModeFlagBit) == 0)) {
                PLUGIN_LOG_W("RENDER_VALIDATION: depth resolve mode flags not set for depth resolve image");
            }
#endif

            const auto getAttachmentIndices = [](const char* attachmentName, const auto& subpass,
                                                  auto& attachmentVector) {
                if (auto const iIter = subpass.find(attachmentName); iIter) {
                    if (iIter->is_array()) {
                        std::transform(iIter->array_.begin(), iIter->array_.end(), std::back_inserter(attachmentVector),
                            [](const json::value& value) {
                                if (value.is_number()) {
                                    return value.template as_number<uint32_t>();
                                }
                                return 0u;
                            });
                    } else {
                        attachmentVector = { iIter->template as_number<uint32_t>() };
                    }
                }
            };

            getAttachmentIndices("inputAttachmentIndices", *sp, renderPass.inputAttachmentIndices);
            getAttachmentIndices("colorAttachmentIndices", *sp, renderPass.colorAttachmentIndices);
            getAttachmentIndices("resolveAttachmentIndices", *sp, renderPass.resolveAttachmentIndices);

            if (auto const pos = sp->find("shadingRateTexelSize"); pos) {
                if (pos->is_array() && pos->array_.size() == 2) { // 2 :size
                    if (pos->array_[0].is_number() && pos->array_[1u].is_number()) {
                        renderPass.shadingRateTexelSize.width = pos->array_[0].as_number<uint32_t>();
                        renderPass.shadingRateTexelSize.height = pos->array_[1].as_number<uint32_t>();
                    }
                }
            }

            SafeGetJsonValue(*sp, "viewMask", result.error, renderPass.viewMask);
        }
    }
}

void ParseResources(const string_view name, const json::value& node, RenderNodeGraphInputs::InputResources& resources,
    LoadResult& result)
{
    if (auto const res = node.find(name); res) {
        ParseArray<decltype(resources.buffers)::value_type>(*res, "buffers", resources.buffers, result);
        ParseArray<decltype(resources.images)::value_type>(*res, "images", resources.images, result);
        ParseArray<decltype(resources.samplers)::value_type>(*res, "samplers", resources.samplers, result);

        ParseArray<decltype(resources.customInputBuffers)::value_type>(
            *res, "customInputBuffers", resources.customInputBuffers, result);
        ParseArray<decltype(resources.customOutputBuffers)::value_type>(
            *res, "customOutputBuffers", resources.customOutputBuffers, result);

        ParseArray<decltype(resources.customInputImages)::value_type>(
            *res, "customInputImages", resources.customInputImages, result);
        ParseArray<decltype(resources.customOutputImages)::value_type>(
            *res, "customOutputImages", resources.customOutputImages, result);
    }
}
} // namespace

RenderNodeParserUtil::RenderNodeParserUtil(const CreateInfo& createInfo) {}

RenderNodeParserUtil::~RenderNodeParserUtil() = default;

uint64_t RenderNodeParserUtil::GetUintValue(const json::value& jsonValue, const string_view name) const
{
    uint64_t val = std::numeric_limits<uint64_t>::max();
    string error;
    if (!SafeGetJsonValue(jsonValue, name, error, val)) {
        PLUGIN_LOG_W("GetUintValue: %s", error.c_str());
    }
    return val;
}

int64_t RenderNodeParserUtil::GetIntValue(const json::value& jsonValue, const string_view name) const
{
    int64_t val = std::numeric_limits<int64_t>::max();
    string error;
    if (!SafeGetJsonValue(jsonValue, name, error, val)) {
        PLUGIN_LOG_W("GetIntValue: %s", error.c_str());
    }
    return val;
}

float RenderNodeParserUtil::GetFloatValue(const json::value& jsonValue, const string_view name) const
{
    float val = std::numeric_limits<float>::max();
    string error;
    if (!SafeGetJsonValue(jsonValue, name, error, val)) {
        PLUGIN_LOG_W("GetFloatValue: %s", error.c_str());
    }
    return val;
}

string RenderNodeParserUtil::GetStringValue(const json::value& jsonValue, const string_view name) const
{
    string parsedString;
    string error;
    if (!SafeGetJsonValue(jsonValue, name, error, parsedString)) {
        PLUGIN_LOG_W("GetStringValue: %s", error.c_str());
    }
    return parsedString;
}

RenderNodeGraphInputs::InputRenderPass RenderNodeParserUtil::GetInputRenderPass(
    const json::value& jsonValue, const string_view name) const
{
    RenderNodeGraphInputs::InputRenderPass renderPass;
    LoadResult result;
    ParseRenderpass(name, jsonValue, renderPass, result);
    if (!result.error.empty()) {
        PLUGIN_LOG_W("GetInputRenderPass: %s", result.error.c_str());
    }
    return renderPass;
}

RenderNodeGraphInputs::InputResources RenderNodeParserUtil::GetInputResources(
    const json::value& jsonValue, const string_view name) const
{
    RenderNodeGraphInputs::InputResources resources;
    LoadResult result;
    ParseResources(name, jsonValue, resources, result);
    if (!result.error.empty()) {
        PLUGIN_LOG_W("GetInputResources: %s", result.error.c_str());
    }
    return resources;
}

RenderNodeGraphInputs::RenderDataStore RenderNodeParserUtil::GetRenderDataStore(
    const json::value& jsonValue, const string_view name) const
{
    RenderNodeGraphInputs::RenderDataStore renderDataStore;
    LoadResult result;
    if (auto const dataStore = jsonValue.find(name); dataStore) {
        SafeGetJsonValue(*dataStore, "dataStoreName", result.error, renderDataStore.dataStoreName);
        SafeGetJsonValue(*dataStore, "typeName", result.error, renderDataStore.typeName);
        SafeGetJsonValue(*dataStore, "configurationName", result.error, renderDataStore.configurationName);
    }
    if (!result.error.empty()) {
        PLUGIN_LOG_W("GetRenderDataStore: %s", result.error.c_str());
    }
    return renderDataStore;
}

vector<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> RenderNodeParserUtil::GetGpuImageDescs(
    const json::value& jsonValue, const string_view name) const
{
    vector<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> resources;
    LoadResult result;
    if (const auto ref = jsonValue.find(name); ref) {
        if (ref->is_array()) {
            ParseArray<decltype(resources)::value_type>(jsonValue, name.data(), resources, result);
        } else {
            result.error += "expecting array";
        }
    }
    if (!result.error.empty()) {
        PLUGIN_LOG_W("GetGpuImageDescs: %s", result.error.c_str());
    }
    return resources;
}

vector<RenderNodeGraphInputs::RenderNodeGraphGpuBufferDesc> RenderNodeParserUtil::GetGpuBufferDescs(
    const json::value& jsonValue, const string_view name) const
{
    vector<RenderNodeGraphInputs::RenderNodeGraphGpuBufferDesc> resources;
    LoadResult result;
    if (const auto ref = jsonValue.find(name); ref) {
        if (ref->is_array()) {
            ParseArray<decltype(resources)::value_type>(jsonValue, name.data(), resources, result);
        } else {
            result.error += "expecting array";
        }
    }
    if (!result.error.empty()) {
        PLUGIN_LOG_W("GetGpuBufferDescs: %s", result.error.c_str());
    }
    return resources;
}

RenderSlotSortType RenderNodeParserUtil::GetRenderSlotSortType(
    const json::value& jsonValue, const string_view name) const
{
    RenderSlotSortType sortType = RenderSlotSortType::NONE;
    string error;
    if (!SafeGetJsonEnum(jsonValue, name, error, sortType)) {
        PLUGIN_LOG_W("GetRenderSlotSortType: %s", error.c_str());
    }
    return sortType;
}

RenderSlotCullType RenderNodeParserUtil::GetRenderSlotCullType(
    const json::value& jsonValue, const string_view name) const
{
    RenderSlotCullType cullType = RenderSlotCullType::NONE;
    string error;
    if (!SafeGetJsonEnum(jsonValue, name, error, cullType)) {
        PLUGIN_LOG_W("GetRenderSlotCullType: %s", error.c_str());
    }
    return cullType;
}

const CORE_NS::IInterface* RenderNodeParserUtil::GetInterface(const BASE_NS::Uid& uid) const
{
    if ((uid == IRenderNodeParserUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* RenderNodeParserUtil::GetInterface(const BASE_NS::Uid& uid)
{
    if ((uid == IRenderNodeParserUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderNodeParserUtil::Ref() {}

void RenderNodeParserUtil::Unref() {}
RENDER_END_NAMESPACE()
