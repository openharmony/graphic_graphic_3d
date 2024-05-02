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

#include "pipeline_layout_loader.h"

#include <base/math/mathf.h>
#include <core/io/intf_file_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "json_format_serialization.h"
#include "json_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
// clang-format off
CORE_JSON_SERIALIZE_ENUM(ShaderStageFlagBits,
    {
        { (ShaderStageFlagBits)0, nullptr },
        { ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT, "vertex_bit" },
        { ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT, "fragment_bit" },
        { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT, "compute_bit" },
    })

CORE_JSON_SERIALIZE_ENUM(DescriptorType,
    {
        { DescriptorType::CORE_DESCRIPTOR_TYPE_MAX_ENUM, nullptr }, // default
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
        { DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE, "acceleration_structure" },
    })
// clang-format on
void FromJson(const json::value& jsonData, JsonContext<DescriptorSetLayoutBinding>& context)
{
    SafeGetJsonValue(jsonData, "binding", context.error, context.data.binding);
    SafeGetJsonEnum(jsonData, "descriptorType", context.error, context.data.descriptorType);
    SafeGetJsonValue(jsonData, "descriptorCount", context.error, context.data.descriptorCount);
    SafeGetJsonBitfield<ShaderStageFlagBits>(
        jsonData, "shaderStageFlags", context.error, context.data.shaderStageFlags);
}

void FromJson(const json::value& jsonData, JsonContext<DescriptorSetLayout>& context)
{
    SafeGetJsonValue(jsonData, "set", context.error, context.data.set);

    PipelineLayoutLoader::LoadResult loadResult;
    ParseArray<decltype(context.data.bindings)::value_type>(jsonData, "bindings", context.data.bindings, loadResult);
    context.error = loadResult.error;
    // NOTE: does not fetch descriptor set arrays
}

PipelineLayoutLoader::LoadResult Load(const json::value& jsonData, const string_view uri, PipelineLayout& pl)
{
    PipelineLayoutLoader::LoadResult result;
    pl = {}; // reset

    if (const auto pcIter = jsonData.find("pushConstant"); pcIter) {
        SafeGetJsonValue(*pcIter, "size", result.error, pl.pushConstant.byteSize);
        SafeGetJsonValue(*pcIter, "byteSize", result.error, pl.pushConstant.byteSize);
        SafeGetJsonBitfield<ShaderStageFlagBits>(
            *pcIter, "shaderStageFlags", result.error, pl.pushConstant.shaderStageFlags);
#if (RENDER_VALIDATION_ENABLED == 1)
        if (pl.pushConstant.byteSize > PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE) {
            PLUGIN_LOG_W("RENDER_VALIDATION: Invalid push constant size clamped (name:%s). push constant size %u <= %u",
                uri.data(), pl.pushConstant.byteSize, PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE);
        }
#endif
        pl.pushConstant.byteSize =
            Math::min(PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE, pl.pushConstant.byteSize);
    }

    vector<DescriptorSetLayout> descriptorSetLayouts;
    ParseArray<decltype(descriptorSetLayouts)::value_type>(
        jsonData, "descriptorSetLayouts", descriptorSetLayouts, result);
    if (!descriptorSetLayouts.empty()) {
        const uint32_t inputDescriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
#if (RENDER_VALIDATION_ENABLED == 1)
        if (inputDescriptorSetCount > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
            PLUGIN_LOG_W("RENDER_VALIDATION: Invalid pipeline layout sizes clamped (name:%s). Set count %u <= %u",
                uri.data(), inputDescriptorSetCount, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
        }
        for (const auto& descRef : descriptorSetLayouts) {
            if (descRef.bindings.size() > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                PLUGIN_LOG_W(
                    "RENDER_VALIDATION: Binding count exceeds the maximum (name:%s). Binding count count %u <= %u",
                    uri.data(), static_cast<uint32_t>(descRef.bindings.size()),
                    PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);
            }
            for (const auto& bindingRef : descRef.bindings) {
                if (bindingRef.descriptorType == DescriptorType::CORE_DESCRIPTOR_TYPE_MAX_ENUM) {
                    PLUGIN_LOG_W("RENDER_VALIDATION: Unknown descriptor type (name:%s) (set:%u, binding:%u).",
                        uri.data(), descRef.set, bindingRef.binding);
                }
            }
        }
#endif
        // pipeline layout descriptor sets might have gaps and only some sets defined
        for (uint32_t idx = 0; idx < inputDescriptorSetCount; ++idx) {
            const uint32_t setIndex = descriptorSetLayouts[idx].set;
            if (setIndex < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
                pl.descriptorSetLayouts[setIndex] = move(descriptorSetLayouts[idx]);
                pl.descriptorSetCount++;
            }
        }
        // reassure
        pl.descriptorSetCount = Math::min(pl.descriptorSetCount, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
    } else {
        result.error += "invalid descriptor set layout count";
    }

    result.success = result.error.empty();
    if (!result.success) {
        PLUGIN_LOG_E("error loading pipeline layout from json: %s", result.error.c_str());
    }

    return result;
}

string_view PipelineLayoutLoader::GetUri() const
{
    return uri_;
}

const PipelineLayout& PipelineLayoutLoader::GetPipelineLayout() const
{
    return pipelineLayout_;
}

PipelineLayoutLoader::LoadResult PipelineLayoutLoader::Load(const string_view jsonString)
{
    if (json::value jsonData = json::parse(jsonString.data()); jsonData) {
        return RENDER_NS::Load(jsonData, uri_, pipelineLayout_);
    }
    return LoadResult("Invalid json file.");
}

PipelineLayoutLoader::LoadResult PipelineLayoutLoader::Load(IFileManager& fileManager, const string_view uri)
{
    uri_ = uri;

    IFile::Ptr file = fileManager.OpenFile(uri);
    if (!file) {
        PLUGIN_LOG_E("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to open file.");
    }

    const uint64_t byteLength = file->GetLength();

    string raw;
    raw.resize(static_cast<size_t>(byteLength));

    if (file->Read(raw.data(), byteLength) != byteLength) {
        PLUGIN_LOG_E("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to read file.");
    }

    return Load(string_view(raw));
}
RENDER_END_NAMESPACE()
