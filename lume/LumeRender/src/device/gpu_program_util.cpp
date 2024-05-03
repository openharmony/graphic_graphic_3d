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

#include "gpu_program_util.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <numeric>

#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace GpuProgramUtil {
namespace {
struct VertexAttributeInfo {
    uint32_t byteSize { 0 };
    VertexInputDeclaration::VertexInputAttributeDescription description;
};
} // namespace

bool AddBindings(const DescriptorSetLayout& inDescriptorSetLayout, DescriptorSetLayout& outDescriptorSetLayout)
{
    const auto& inBindings = inDescriptorSetLayout.bindings;
    auto& outBindings = outDescriptorSetLayout.bindings;
    if (outBindings.size() < inBindings.size()) {
        outBindings.reserve(inBindings.size());
    }
    bool validCombination = true;
    for (size_t idx = 0; idx < inDescriptorSetLayout.bindings.size(); ++idx) {
        bool bindingAlreadyFound = false;
        const auto& inBinding = inDescriptorSetLayout.bindings[idx];
        const uint32_t currBindingIndex = inBinding.binding;
        for (auto& outRef : outBindings) {
            if (currBindingIndex == outRef.binding) {
                bindingAlreadyFound = true;
                outRef.shaderStageFlags |= inBinding.shaderStageFlags;
                if ((inBinding.descriptorType != outRef.descriptorType) ||
                    (inBinding.descriptorCount != outRef.descriptorCount)) {
                    validCombination = false;
                    PLUGIN_LOG_E(
                        "Invalid descriptor set combination with binding %u. Descriptor type %u = %u. Descriptor count "
                        "%u = %u",
                        currBindingIndex, inBinding.descriptorType, outRef.descriptorType, inBinding.descriptorCount,
                        outRef.descriptorCount);
                    // more error log printed in higher level with more info
                }
            }
        }
        if (!bindingAlreadyFound) {
            outBindings.push_back(inBinding);
        }
    }
    return validCombination;
}

void CombinePipelineLayouts(const array_view<const PipelineLayout> inPl, PipelineLayout& outPl)
{
    auto& descriptorSetLayouts = outPl.descriptorSetLayouts;
    for (const auto& plRef : inPl) {
        for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
            if (plRef.descriptorSetLayouts[idx].set != PipelineLayoutConstants::INVALID_INDEX) {
                descriptorSetLayouts[idx].set = plRef.descriptorSetLayouts[idx].set;
                const bool validComb =
                    GpuProgramUtil::AddBindings(plRef.descriptorSetLayouts[idx], descriptorSetLayouts[idx]);
                if (!validComb) {
                    PLUGIN_LOG_E(
                        "Invalid shader module descriptor set combination for shader program. Descriptor set %u.", idx);
                }
            }
        }
        outPl.pushConstant.shaderStageFlags |= plRef.pushConstant.shaderStageFlags;
        outPl.pushConstant.byteSize = Math::max(outPl.pushConstant.byteSize, plRef.pushConstant.byteSize);
    }

    uint32_t descriptorSetCount = 0;
    for (const DescriptorSetLayout& currLayout : outPl.descriptorSetLayouts) {
        if (currLayout.set != PipelineLayoutConstants::INVALID_INDEX) {
            descriptorSetCount++;
        }
    }
    outPl.descriptorSetCount = descriptorSetCount;

    // sort bindings inside sets
    for (DescriptorSetLayout& currSet : outPl.descriptorSetLayouts) {
        if (currSet.set != PipelineLayoutConstants::INVALID_INDEX) {
            std::sort(currSet.bindings.begin(), currSet.bindings.end(),
                [](auto const& lhs, auto const& rhs) { return (lhs.binding < rhs.binding); });
        }
    }
}

uint32_t SpecializationByteSize(ShaderSpecialization::Constant::Type type)
{
    switch (type) {
        case RENDER_NS::ShaderSpecialization::Constant::Type::BOOL:
            [[fallthrough]];
        case RENDER_NS::ShaderSpecialization::Constant::Type::UINT32:
            [[fallthrough]];
        case RENDER_NS::ShaderSpecialization::Constant::Type::INT32:
            [[fallthrough]];
        case RENDER_NS::ShaderSpecialization::Constant::Type::FLOAT:
            return 4;
        default:
            break;
    }
    return 4;
}

void AddSpecializationConstants(const array_view<const ShaderSpecialization::Constant> inSpecializationConstants,
    vector<ShaderSpecialization::Constant>& outSpecializationConstants)
{
    uint32_t offset = 0;
    if (!outSpecializationConstants.empty()) {
        offset =
            outSpecializationConstants.back().offset + SpecializationByteSize(outSpecializationConstants.back().type);
    }
    for (auto const& constant : inSpecializationConstants) {
        outSpecializationConstants.push_back(
            ShaderSpecialization::Constant { constant.shaderStage, constant.id, constant.type, offset });
        offset += SpecializationByteSize(constant.type);
    }
}

void CombineSpecializationConstants(const BASE_NS::array_view<const ShaderSpecialization::Constant> inSc,
    BASE_NS::vector<ShaderSpecialization::Constant>& outSc)
{
    if (!inSc.empty()) {
        GpuProgramUtil::AddSpecializationConstants(inSc, outSc);
    }
    // sorted based on offset due to offset mapping with shader combinations
    // NOTE: id and name indexing
    std::sort(outSc.begin(), outSc.end(), [](const auto& lhs, const auto& rhs) { return (lhs.offset < rhs.offset); });
}

uint32_t FormatByteSize(Format format)
{
    switch (format) {
        case BASE_FORMAT_UNDEFINED:
            return 0;

        case BASE_FORMAT_R4G4_UNORM_PACK8:
            return 1;

        case BASE_FORMAT_R4G4B4A4_UNORM_PACK16:
        case BASE_FORMAT_B4G4R4A4_UNORM_PACK16:
        case BASE_FORMAT_R5G6B5_UNORM_PACK16:
        case BASE_FORMAT_B5G6R5_UNORM_PACK16:
        case BASE_FORMAT_R5G5B5A1_UNORM_PACK16:
        case BASE_FORMAT_B5G5R5A1_UNORM_PACK16:
        case BASE_FORMAT_A1R5G5B5_UNORM_PACK16:
            return 2;

        case BASE_FORMAT_R8_UNORM:
        case BASE_FORMAT_R8_SNORM:
        case BASE_FORMAT_R8_USCALED:
        case BASE_FORMAT_R8_SSCALED:
        case BASE_FORMAT_R8_UINT:
        case BASE_FORMAT_R8_SINT:
        case BASE_FORMAT_R8_SRGB:
            return 1;

        case BASE_FORMAT_R8G8_UNORM:
        case BASE_FORMAT_R8G8_SNORM:
        case BASE_FORMAT_R8G8_USCALED:
        case BASE_FORMAT_R8G8_SSCALED:
        case BASE_FORMAT_R8G8_UINT:
        case BASE_FORMAT_R8G8_SINT:
        case BASE_FORMAT_R8G8_SRGB:
            return 2;

        case BASE_FORMAT_R8G8B8_UNORM:
        case BASE_FORMAT_R8G8B8_SNORM:
        case BASE_FORMAT_R8G8B8_USCALED:
        case BASE_FORMAT_R8G8B8_SSCALED:
        case BASE_FORMAT_R8G8B8_UINT:
        case BASE_FORMAT_R8G8B8_SINT:
        case BASE_FORMAT_R8G8B8_SRGB:
        case BASE_FORMAT_B8G8R8_UNORM:
        case BASE_FORMAT_B8G8R8_SNORM:
        case BASE_FORMAT_B8G8R8_UINT:
        case BASE_FORMAT_B8G8R8_SINT:
        case BASE_FORMAT_B8G8R8_SRGB:
            return 3;

        case BASE_FORMAT_R8G8B8A8_UNORM:
        case BASE_FORMAT_R8G8B8A8_SNORM:
        case BASE_FORMAT_R8G8B8A8_USCALED:
        case BASE_FORMAT_R8G8B8A8_SSCALED:
        case BASE_FORMAT_R8G8B8A8_UINT:
        case BASE_FORMAT_R8G8B8A8_SINT:
        case BASE_FORMAT_R8G8B8A8_SRGB:
        case BASE_FORMAT_B8G8R8A8_UNORM:
        case BASE_FORMAT_B8G8R8A8_SNORM:
        case BASE_FORMAT_B8G8R8A8_UINT:
        case BASE_FORMAT_B8G8R8A8_SINT:
        case BASE_FORMAT_B8G8R8A8_SRGB:
        case BASE_FORMAT_A8B8G8R8_UNORM_PACK32:
        case BASE_FORMAT_A8B8G8R8_SNORM_PACK32:
        case BASE_FORMAT_A8B8G8R8_USCALED_PACK32:
        case BASE_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case BASE_FORMAT_A8B8G8R8_UINT_PACK32:
        case BASE_FORMAT_A8B8G8R8_SINT_PACK32:
        case BASE_FORMAT_A8B8G8R8_SRGB_PACK32:
        case BASE_FORMAT_A2R10G10B10_UNORM_PACK32:
        case BASE_FORMAT_A2R10G10B10_UINT_PACK32:
        case BASE_FORMAT_A2R10G10B10_SINT_PACK32:
        case BASE_FORMAT_A2B10G10R10_UNORM_PACK32:
        case BASE_FORMAT_A2B10G10R10_SNORM_PACK32:
        case BASE_FORMAT_A2B10G10R10_USCALED_PACK32:
        case BASE_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case BASE_FORMAT_A2B10G10R10_UINT_PACK32:
        case BASE_FORMAT_A2B10G10R10_SINT_PACK32:
            return 4;

        case BASE_FORMAT_R16_UNORM:
        case BASE_FORMAT_R16_SNORM:
        case BASE_FORMAT_R16_USCALED:
        case BASE_FORMAT_R16_SSCALED:
        case BASE_FORMAT_R16_UINT:
        case BASE_FORMAT_R16_SINT:
        case BASE_FORMAT_R16_SFLOAT:
            return 2;

        case BASE_FORMAT_R16G16_UNORM:
        case BASE_FORMAT_R16G16_SNORM:
        case BASE_FORMAT_R16G16_USCALED:
        case BASE_FORMAT_R16G16_SSCALED:
        case BASE_FORMAT_R16G16_UINT:
        case BASE_FORMAT_R16G16_SINT:
        case BASE_FORMAT_R16G16_SFLOAT:
            return 4;

        case BASE_FORMAT_R16G16B16_UNORM:
        case BASE_FORMAT_R16G16B16_SNORM:
        case BASE_FORMAT_R16G16B16_USCALED:
        case BASE_FORMAT_R16G16B16_SSCALED:
        case BASE_FORMAT_R16G16B16_UINT:
        case BASE_FORMAT_R16G16B16_SINT:
        case BASE_FORMAT_R16G16B16_SFLOAT:
            return 6;

        case BASE_FORMAT_R16G16B16A16_UNORM:
        case BASE_FORMAT_R16G16B16A16_SNORM:
        case BASE_FORMAT_R16G16B16A16_USCALED:
        case BASE_FORMAT_R16G16B16A16_SSCALED:
        case BASE_FORMAT_R16G16B16A16_UINT:
        case BASE_FORMAT_R16G16B16A16_SINT:
        case BASE_FORMAT_R16G16B16A16_SFLOAT:
            return 8;

        case BASE_FORMAT_R32_UINT:
        case BASE_FORMAT_R32_SINT:
        case BASE_FORMAT_R32_SFLOAT:
            return 4;

        case BASE_FORMAT_R32G32_UINT:
        case BASE_FORMAT_R32G32_SINT:
        case BASE_FORMAT_R32G32_SFLOAT:
            return 8;

        case BASE_FORMAT_R32G32B32_UINT:
        case BASE_FORMAT_R32G32B32_SINT:
        case BASE_FORMAT_R32G32B32_SFLOAT:
            return 24;

        case BASE_FORMAT_R32G32B32A32_UINT:
        case BASE_FORMAT_R32G32B32A32_SINT:
        case BASE_FORMAT_R32G32B32A32_SFLOAT:
        case BASE_FORMAT_B10G11R11_UFLOAT_PACK32:
        case BASE_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 32;

        case BASE_FORMAT_D16_UNORM:
            return 2;

        case BASE_FORMAT_X8_D24_UNORM_PACK32:
        case BASE_FORMAT_D32_SFLOAT:
            return 4;

        case BASE_FORMAT_S8_UINT:
            return 1;

        case BASE_FORMAT_D24_UNORM_S8_UINT:
            return 4;

        default:
            return 0;
    }
}
} // namespace GpuProgramUtil
RENDER_END_NAMESPACE()
