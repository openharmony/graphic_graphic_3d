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

#include "device/shader_reflection_data.h"

#include <algorithm>
#include <array>

#include <base/util/algorithm.h>

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint8_t REFLECTION_TAG[] = { 'r', 'f', 'l', 1 };
struct ReflectionHeader {
    uint8_t tag[sizeof(REFLECTION_TAG)];
    uint16_t type;
    uint16_t offsetPushConstants;
    uint16_t offsetSpecializationConstants;
    uint16_t offsetDescriptorSets;
    uint16_t offsetInputs;
    uint16_t offsetLocalSize;
};
constexpr size_t INDEX_PUSH_CONSTANTS = 0U;
constexpr size_t INDEX_SPECIALIZATION_CONSTANTS = 1U;
constexpr size_t INDEX_DESCRIPTOR_SETS = 2U;
constexpr size_t INDEX_INPUTS = 3U;
constexpr size_t INDEX_LOCAL_SIZE = 4U;
struct Data {
    uint32_t index;
    uint16_t offset;
    uint16_t size;
};

std::array<Data, 5U> CalculateDataSizes(const ReflectionHeader& header, uint32_t totalSize)
{
    std::array<Data, 5U> data {
        Data { INDEX_PUSH_CONSTANTS, header.offsetPushConstants, 0U },
        Data { INDEX_SPECIALIZATION_CONSTANTS, header.offsetSpecializationConstants, 0U },
        Data { INDEX_DESCRIPTOR_SETS, header.offsetDescriptorSets, 0U },
        Data { INDEX_INPUTS, header.offsetInputs, 0U },
        Data { INDEX_LOCAL_SIZE, header.offsetLocalSize, 0U },
    };
    // first sort by offsets
    BASE_NS::InsertionSort(
        data.begin(), data.end(), [](const auto& lhs, const auto& rhs) { return lhs.offset < rhs.offset; });

    // size of each data segment is what fits between it's offset and the next. if offset + size doesn't fit size is
    // left zero.
    for (auto i = 0LLU, end = data.size() - 1LLU; i < end; ++i) {
        const auto size = data[i + 1LLU].offset - data[i].offset;
        if ((size > 0) && ((data[i].offset + size_t(size)) < totalSize)) {
            data[i].size = uint16_t(size);
        }
    }
    if (data.back().offset < totalSize) {
        data.back().size = uint16_t(totalSize - data.back().offset);
    }
    // sort be index so that the data is easily usable.
    BASE_NS::InsertionSort(
        data.begin(), data.end(), [](const auto& lhs, const auto& rhs) { return lhs.index < rhs.index; });
    return data;
}

inline uint8_t Read8U(const uint8_t*& ptr)
{
    uint8_t ret = *(ptr);
    ptr += sizeof(ret);
    return ret;
}

inline uint16_t Read16U(const uint8_t*& ptr)
{
    auto ret = static_cast<uint16_t>(*(ptr) | (*(ptr + 1) << 8U));
    ptr += sizeof(ret);
    return ret;
}
inline uint32_t Read32U(const uint8_t*& ptr)
{
    auto ret = static_cast<uint32_t>(*ptr | *(ptr + 1) << 8U | *(ptr + 2) << 16U | *(ptr + 3) << 24U);
    ptr += sizeof(ret);
    return ret;
}

constexpr AdditionalDescriptorTypeFlags GetPackedDescriptorTypeFlags(
    const ImageFlags imageFlags, const ImageDimension imageDimension)
{
    AdditionalDescriptorFlags flags = 0U;
    if (imageDimension == ImageDimension::DIMENSION_1D) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_DIMENSION_1D_BIT;
    }
    if (imageDimension == ImageDimension::DIMENSION_2D) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_DIMENSION_2D_BIT;
    }
    if (imageDimension == ImageDimension::DIMENSION_3D) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_DIMENSION_3D_BIT;
    }
    if (imageDimension == ImageDimension::DIMENSION_CUBE) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_DIMENSION_CUBE_BIT;
    }
    if (imageDimension == ImageDimension::DIMENSION_RECT) {
        flags |= 0U;
    }
    if (imageDimension == ImageDimension::DIMENSION_BUFFER) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_DIMENSION_BUFFER_BIT;
    }
    if (imageDimension == ImageDimension::DIMENSION_SUBPASS) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_DIMENSION_SUBPASS_BIT;
    }

    if (imageFlags & ImageFlags::IMAGE_DEPTH) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_DEPTH_BIT;
    }
    if (imageFlags & ImageFlags::IMAGE_ARRAY) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_ARRAY_BIT;
    }
    if (imageFlags & ImageFlags::IMAGE_MULTISAMPLE) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_MULTISAMPLE_BIT;
    }
    if (imageFlags & ImageFlags::IMAGE_SAMPLED) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_SAMPLED_BIT;
    }
    if (imageFlags & ImageFlags::IMAGE_LOAD_STORE) {
        flags |= AdditionalDescriptorTypeImageFlagBits::CORE_DESCRIPTOR_TYPE_IMAGE_LOAD_STORE_BIT;
    }

    return flags;
}

void ReadDescriptorSetsV0(
    PipelineLayout& pipelineLayout, const uint8_t*& ptr, const uint8_t* const end, ShaderStageFlags flags)
{
    for (auto i = 0u; i < pipelineLayout.descriptorSetCount; ++i) {
        // there must be at least 2 uint16 per each descriptor set (set and number of bindings)
        if ((end - ptr) < static_cast<ptrdiff_t>(2U * sizeof(uint16_t))) {
            pipelineLayout.descriptorSetCount = 0U;
            return;
        }
        // write to correct set location
        const auto set = static_cast<uint32_t>(Read16U(ptr));
        if (set >= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
            pipelineLayout.descriptorSetCount = 0U;
            return;
        }
        const auto bindings = static_cast<uint32_t>(Read16U(ptr));
        // each binding needs three 16 bit (binding idx, desc type, desc count).
        static constexpr auto bindingSize = (3U * sizeof(uint16_t));
        if ((end - ptr) < static_cast<ptrdiff_t>(bindings * bindingSize)) {
            pipelineLayout.descriptorSetCount = 0U;
            return;
        }

        auto& layout = pipelineLayout.descriptorSetLayouts[set];
        layout.set = set;
        layout.bindings.reserve(bindings);
        for (auto j = 0u; j < bindings; ++j) {
            DescriptorSetLayoutBinding& binding = layout.bindings.emplace_back();
            binding.binding = static_cast<uint32_t>(Read16U(ptr));
            binding.descriptorType = static_cast<DescriptorType>(Read16U(ptr));
            if ((binding.descriptorType > DescriptorType::CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) &&
                (binding.descriptorType == (DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE & 0xffff))) {
                binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
            }
            binding.descriptorCount = static_cast<uint32_t>(Read16U(ptr));
            binding.shaderStageFlags = flags;
        }
    }
}

void ReadDescriptorSetsV1(
    PipelineLayout& pipelineLayout, const uint8_t*& ptr, const uint8_t* const end, ShaderStageFlags flags)
{
    for (auto i = 0u; i < pipelineLayout.descriptorSetCount; ++i) {
        // there must be at least 2 uint16 per each descriptor set (set and number of bindings)
        if ((end - ptr) < static_cast<ptrdiff_t>(2U * sizeof(uint16_t))) {
            pipelineLayout.descriptorSetCount = 0U;
            return;
        }
        // write to correct set location
        const auto set = static_cast<uint32_t>(Read16U(ptr));
        if (set >= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
            pipelineLayout.descriptorSetCount = 0U;
            return;
        }
        const auto bindings = static_cast<uint32_t>(Read16U(ptr));
        // each binding needs three 16 bit (binding idx, desc type, desc count) and two 8 bit values (image dims and
        // flags).
        static constexpr auto bindingSize = (3U * sizeof(uint16_t) + 2U * sizeof(uint8_t));
        if ((end - ptr) < static_cast<ptrdiff_t>(bindings * bindingSize)) {
            pipelineLayout.descriptorSetCount = 0U;
            return;
        }

        auto& layout = pipelineLayout.descriptorSetLayouts[set];
        layout.set = set;
        layout.bindings.reserve(bindings);
        for (auto j = 0u; j < bindings; ++j) {
            DescriptorSetLayoutBinding& binding = layout.bindings.emplace_back();
            binding.binding = static_cast<uint32_t>(Read16U(ptr));
            binding.descriptorType = static_cast<DescriptorType>(Read16U(ptr));
            if ((binding.descriptorType > DescriptorType::CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) &&
                (binding.descriptorType == (DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE & 0xffff))) {
                binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
            }
            binding.descriptorCount = static_cast<uint32_t>(Read16U(ptr));
            const auto imageDimension = Read8U(ptr);
            if (imageDimension > uint8_t(ImageDimension::DIMENSION_SUBPASS)) {
                pipelineLayout.descriptorSetCount = 0U;
                return;
            }
            const auto imageFlags = Read8U(ptr);
            if (imageFlags >
                uint8_t(ImageFlags::IMAGE_LOAD_STORE | ImageFlags::IMAGE_SAMPLED | ImageFlags::IMAGE_MULTISAMPLE |
                        ImageFlags::IMAGE_ARRAY | ImageFlags::IMAGE_DEPTH)) {
                pipelineLayout.descriptorSetCount = 0U;
                return;
            }
            binding.shaderStageFlags = flags;
            binding.additionalDescriptorTypeFlags =
                GetPackedDescriptorTypeFlags(ImageFlags(imageFlags), ImageDimension(imageDimension));
        }
    }
}
} // namespace

ShaderReflectionData::ShaderReflectionData(BASE_NS::array_view<const uint8_t> reflectionData)
    : reflectionData_(reflectionData)
{
    if (IsValid()) {
        const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
        const auto dataSizes = CalculateDataSizes(header, uint32_t(reflectionData_.size()));
        std::transform(dataSizes.cbegin(), dataSizes.cend(), size_, [](const Data& data) { return data.size; });
    }
}

bool ShaderReflectionData::IsValid() const
{
    if (reflectionData_.size() < sizeof(ReflectionHeader)) {
        return false;
    }
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
    // the begining should match and then check the version next
    auto isSame = memcmp(header.tag, REFLECTION_TAG, sizeof(REFLECTION_TAG) - 1U) == 0;
    return isSame && (header.tag[sizeof(REFLECTION_TAG) - 1U] == 0U || header.tag[sizeof(REFLECTION_TAG) - 1U] == 1U);
}

ShaderStageFlags ShaderReflectionData::GetStageFlags() const
{
    ShaderStageFlags flags;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
    flags = header.type;
    return flags;
}

PipelineLayout ShaderReflectionData::GetPipelineLayout() const
{
    PipelineLayout pipelineLayout;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
    if (header.offsetPushConstants && (size_[INDEX_PUSH_CONSTANTS] >= sizeof(uint8_t)) &&
        (header.offsetPushConstants + size_t(size_[INDEX_PUSH_CONSTANTS])) <= reflectionData_.size()) {
        auto ptr = reflectionData_.data() + header.offsetPushConstants;
        const auto constants = Read8U(ptr);
        if (constants && (size_[INDEX_PUSH_CONSTANTS] >= (sizeof(uint8_t) + sizeof(uint16_t)))) {
            pipelineLayout.pushConstant.shaderStageFlags = header.type;
            pipelineLayout.pushConstant.byteSize = static_cast<uint32_t>(Read16U(ptr));
        }
    }
    if (!header.offsetDescriptorSets || (size_[INDEX_DESCRIPTOR_SETS] < sizeof(uint16_t)) ||
        (header.offsetDescriptorSets + size_t(size_[INDEX_DESCRIPTOR_SETS])) > reflectionData_.size()) {
        return pipelineLayout;
    }
    auto ptr = reflectionData_.data() + header.offsetDescriptorSets;
    auto endSets = ptr + size_[INDEX_DESCRIPTOR_SETS];
    pipelineLayout.descriptorSetCount = static_cast<uint32_t>(Read16U(ptr));
    if (pipelineLayout.descriptorSetCount) {
        switch (header.tag[sizeof(REFLECTION_TAG) - 1U]) {
            case 0U:
                ReadDescriptorSetsV0(pipelineLayout, ptr, endSets, header.type);
                break;

            case 1U:
                ReadDescriptorSetsV1(pipelineLayout, ptr, endSets, header.type);
                break;

            default:
                pipelineLayout.descriptorSetCount = 0U;
                break;
        }
    }
    return pipelineLayout;
}

BASE_NS::vector<ShaderSpecialization::Constant> ShaderReflectionData::GetSpecializationConstants() const
{
    BASE_NS::vector<ShaderSpecialization::Constant> constants;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
    if (!header.offsetSpecializationConstants || (size_[INDEX_SPECIALIZATION_CONSTANTS] < sizeof(uint32_t)) ||
        (header.offsetSpecializationConstants + size_t(size_[INDEX_SPECIALIZATION_CONSTANTS])) >
        reflectionData_.size()) {
        return constants;
    }
    auto ptr = reflectionData_.data() + header.offsetSpecializationConstants;
    const auto size = Read32U(ptr);
    // make sure constant count + id and type for each constant fits in reflection data
    if ((sizeof(uint32_t) + size * (2LLU * sizeof(uint32_t))) > size_t(size_[INDEX_SPECIALIZATION_CONSTANTS])) {
        return constants;
    }
    constants.reserve(size);
    for (auto i = 0U; i < size; ++i) {
        ShaderSpecialization::Constant& constant = constants.emplace_back();
        constant.shaderStage = header.type;
        constant.id = Read32U(ptr);
        constant.type = static_cast<ShaderSpecialization::Constant::Type>(Read32U(ptr));
        constant.offset = 0;
    }

    return constants;
}

BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription>
ShaderReflectionData::GetInputDescriptions() const
{
    BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> inputs;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
    if (!header.offsetInputs || (size_[INDEX_INPUTS] < sizeof(uint16_t)) ||
        (header.offsetInputs + size_t(size_[INDEX_INPUTS])) > reflectionData_.size()) {
        return inputs;
    }
    auto ptr = reflectionData_.data() + header.offsetInputs;
    const auto size = Read16U(ptr);
    // make sure input count + location and format for each input fits in reflection data
    if ((sizeof(uint16_t) + size * (sizeof(uint16_t) * 2LLU)) > size_t(size_[INDEX_INPUTS])) {
        return inputs;
    }
    inputs.reserve(size);
    for (auto i = 0U; i < size; ++i) {
        VertexInputDeclaration::VertexInputAttributeDescription& desc = inputs.emplace_back();
        desc.location = static_cast<uint32_t>(Read16U(ptr));
        desc.binding = desc.location;
        desc.format = static_cast<BASE_NS::Format>(Read16U(ptr));
        desc.offset = 0;
    }

    return inputs;
}

BASE_NS::Math::UVec3 ShaderReflectionData::GetLocalSize() const
{
    BASE_NS::Math::UVec3 sizes;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
    if (header.offsetLocalSize && (size_[INDEX_LOCAL_SIZE] >= sizeof(BASE_NS::Math::UVec3)) &&
        (header.offsetLocalSize + size_t(size_[INDEX_LOCAL_SIZE])) <= reflectionData_.size()) {
        auto ptr = reflectionData_.data() + header.offsetLocalSize;
        sizes.x = Read32U(ptr);
        sizes.y = Read32U(ptr);
        sizes.z = Read32U(ptr);
    }
    return sizes;
}

const uint8_t* ShaderReflectionData::GetPushConstants() const
{
    const uint8_t* ptr = nullptr;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData_.data());
    // constants on/off is uint8 and byte size of constants is uint16
    if (header.offsetPushConstants && (size_[INDEX_PUSH_CONSTANTS] >= sizeof(uint8_t) + sizeof(uint16_t)) &&
        (header.offsetPushConstants + size_t(size_[INDEX_PUSH_CONSTANTS])) <= reflectionData_.size()) {
        const auto constants = *(reflectionData_.data() + header.offsetPushConstants);
        if (constants) {
            ptr = reflectionData_.data() + header.offsetPushConstants + sizeof(uint8_t) + sizeof(uint16_t);
        }
    }
    return ptr;
}
RENDER_END_NAMESPACE()
