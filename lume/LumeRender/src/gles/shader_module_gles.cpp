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

#include "shader_module_gles.h"

#include <algorithm>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/math/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/gpu_program_util.h"
#include "device/shader_manager.h"
#include "gles/spirv_cross_helpers_gles.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
template<typename SetType>
void Collect(const uint32_t set, const DescriptorSetLayoutBinding& binding, SetType& sets)
{
    const auto name = "s" + to_string(set) + "_b" + to_string(binding.binding);
    sets.push_back({ static_cast<uint8_t>(set), static_cast<uint8_t>(binding.binding),
        static_cast<uint8_t>(binding.descriptorCount), string { name } });
}

void CollectRes(const PipelineLayout& pipeline, ShaderModulePlatformDataGLES& plat_)
{
    struct Bind {
        uint8_t set;
        uint8_t bind;
    };
    vector<Bind> samplers;
    vector<Bind> images;
    for (const auto& set : pipeline.descriptorSetLayouts) {
        if (set.set != PipelineLayoutConstants::INVALID_INDEX) {
            for (const auto& binding : set.bindings) {
                switch (binding.descriptorType) {
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER:
                        samplers.push_back({ static_cast<uint8_t>(set.set), static_cast<uint8_t>(binding.binding) });
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                        Collect(set.set, binding, plat_.cbSets);
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                        images.push_back({ static_cast<uint8_t>(set.set), static_cast<uint8_t>(binding.binding) });
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                        Collect(set.set, binding, plat_.ciSets);
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                        Collect(set.set, binding, plat_.ubSets);
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                        Collect(set.set, binding, plat_.sbSets);
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                        Collect(set.set, binding, plat_.siSets);
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE:
                        break;
                    case DescriptorType::CORE_DESCRIPTOR_TYPE_MAX_ENUM:
                        break;
                }
            }
        }
    }
    for (const auto& sBinding : samplers) {
        for (const auto& iBinding : images) {
            const auto name = "s" + to_string(iBinding.set) + "_b" + to_string(iBinding.bind) + "_s" +
                              to_string(sBinding.set) + "_b" + to_string(sBinding.bind);
            plat_.combSets.push_back({ sBinding.set, sBinding.bind, iBinding.set, iBinding.bind, string { name } });
        }
    }
}

void CreateSpecInfos(
    array_view<const ShaderSpecialization::Constant> constants, vector<Gles::SpecConstantInfo>& outSpecInfo)
{
    static_assert(static_cast<uint32_t>(Gles::SpecConstantInfo::Types::BOOL) ==
                  static_cast<uint32_t>(ShaderSpecialization::Constant::Type::BOOL));
    for (const auto& constant : constants) {
        Gles::SpecConstantInfo info { static_cast<Gles::SpecConstantInfo::Types>(constant.type), constant.id, 1U, 1U,
            {} };
        outSpecInfo.push_back(info);
    }
}

void SortSets(PipelineLayout& pipelineLayout)
{
    pipelineLayout.descriptorSetCount = 0;
    for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
        DescriptorSetLayout& currSet = pipelineLayout.descriptorSetLayouts[idx];
        if (currSet.set != PipelineLayoutConstants::INVALID_INDEX) {
            pipelineLayout.descriptorSetCount++;
            std::sort(currSet.bindings.begin(), currSet.bindings.end(),
                [](auto const& lhs, auto const& rhs) { return (lhs.binding < rhs.binding); });
        }
    }
}
} // namespace
struct Reader {
    const uint8_t* ptr;
    uint8_t GetUint8()
    {
        return *ptr++;
    }

    uint16_t GetUint16()
    {
        const uint16_t value = static_cast<uint16_t>(*ptr | (*(ptr + 1) << 8));
        ptr += sizeof(uint16_t);
        return value;
    }
    uint32_t GetUint32()
    {
        const uint32_t value =
            static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8) | ((*(ptr + 2)) << 16) | ((*(ptr + 3)) << 24));
        ptr += sizeof(uint32_t);
        return value;
    }
    string_view GetStringView()
    {
        string_view value;
        const uint16_t len = GetUint16();
        value = string_view(static_cast<const char*>(static_cast<const void*>(ptr)), len);
        ptr += len;
        return value;
    }
};
template<typename ShaderBase>
void ProcessShaderModule(ShaderBase& me, const ShaderModuleCreateInfo& createInfo)
{
    me.pipelineLayout_ = createInfo.reflectionData.GetPipelineLayout();
    if (me.shaderStageFlags_ & CORE_SHADER_STAGE_VERTEX_BIT) {
        me.vertexInputAttributeDescriptions_ = createInfo.reflectionData.GetInputDescriptions();
        me.vertexInputBindingDescriptions_.reserve(me.vertexInputAttributeDescriptions_.size());
        for (const auto& attrib : me.vertexInputAttributeDescriptions_) {
            VertexInputDeclaration::VertexInputBindingDescription bindingDesc;
            bindingDesc.binding = attrib.binding;
            bindingDesc.stride = GpuProgramUtil::FormatByteSize(attrib.format);
            bindingDesc.vertexInputRate = VertexInputRate::CORE_VERTEX_INPUT_RATE_VERTEX;
            me.vertexInputBindingDescriptions_.push_back(bindingDesc);
        }
        me.vidv_.bindingDescriptions = { me.vertexInputBindingDescriptions_.data(),
            me.vertexInputBindingDescriptions_.size() };
        me.vidv_.attributeDescriptions = { me.vertexInputAttributeDescriptions_.data(),
            me.vertexInputAttributeDescriptions_.size() };
    }

    if (me.shaderStageFlags_ & CORE_SHADER_STAGE_COMPUTE_BIT) {
        const Math::UVec3 tgs = createInfo.reflectionData.GetLocalSize();
        me.stg_.x = tgs.x;
        me.stg_.y = tgs.y;
        me.stg_.z = tgs.z;
    }
    if (auto* ptr = createInfo.reflectionData.GetPushConstants(); ptr) {
        Reader read { ptr };
        const auto constants = read.GetUint8();
        for (uint8_t i = 0U; i < constants; ++i) {
            Gles::PushConstantReflection refl;
            refl.type = read.GetUint32();
            refl.offset = read.GetUint16();
            refl.size = read.GetUint16();
            refl.arraySize = read.GetUint16();
            refl.arrayStride = read.GetUint16();
            refl.matrixStride = read.GetUint16();
            refl.name = "CORE_PC_0";
            refl.name += read.GetStringView();
            refl.stage = me.shaderStageFlags_;
            me.plat_.infos.push_back(move(refl));
        }
    }

    me.constants_ = createInfo.reflectionData.GetSpecializationConstants();
    me.sscv_.constants = { me.constants_.data(), me.constants_.size() };
    CollectRes(me.pipelineLayout_, me.plat_);
    CreateSpecInfos(me.constants_, me.specInfo_);
    // sort bindings inside sets (and count them)
    SortSets(me.pipelineLayout_);

    me.source_.assign(
        static_cast<const char*>(static_cast<const void*>(createInfo.spvData.data())), createInfo.spvData.size());
}

template<typename ShaderBase>
string SpecializeShaderModule(const ShaderBase& base, const ShaderSpecializationConstantDataView& specData)
{
    return Gles::Specialize(base.shaderStageFlags_, base.source_, base.constants_, specData);
}

ShaderModuleGLES::ShaderModuleGLES(Device& device, const ShaderModuleCreateInfo& createInfo)
    : device_(device), shaderStageFlags_(createInfo.shaderStageFlags)
{
    if (createInfo.reflectionData.IsValid() &&
        (shaderStageFlags_ &
            (CORE_SHADER_STAGE_VERTEX_BIT | CORE_SHADER_STAGE_FRAGMENT_BIT | CORE_SHADER_STAGE_COMPUTE_BIT))) {
        ProcessShaderModule(*this, createInfo);
    } else {
        PLUGIN_LOG_E("invalid shader stages or invalid reflection data for shader module, invalid shader module");
    }
}

ShaderModuleGLES::~ShaderModuleGLES() = default;

ShaderStageFlags ShaderModuleGLES::GetShaderStageFlags() const
{
    return shaderStageFlags_;
}

string ShaderModuleGLES::GetGLSL(const ShaderSpecializationConstantDataView& specData) const
{
    return SpecializeShaderModule(*this, specData);
}

const ShaderModulePlatformData& ShaderModuleGLES::GetPlatformData() const
{
    return plat_;
}

const PipelineLayout& ShaderModuleGLES::GetPipelineLayout() const
{
    return pipelineLayout_;
}

ShaderSpecializationConstantView ShaderModuleGLES::GetSpecilization() const
{
    return sscv_;
}

VertexInputDeclarationView ShaderModuleGLES::GetVertexInputDeclaration() const
{
    return vidv_;
}

ShaderThreadGroup ShaderModuleGLES::GetThreadGroupSize() const
{
    return stg_;
}
RENDER_END_NAMESPACE()
