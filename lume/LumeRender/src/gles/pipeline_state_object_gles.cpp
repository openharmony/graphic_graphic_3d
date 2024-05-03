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

#include "pipeline_state_object_gles.h"

#include <base/containers/fixed_string.h>
#include <base/util/formats.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/gpu_program_gles.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
void FormatToVertexType(const VertexInputDeclaration::VertexInputAttributeDescription& attributeRef, GLuint& count,
    GLenum& type, GLboolean& normalized, bool& isFloat)
{
    if (attributeRef.format == BASE_FORMAT_R16G16_SFLOAT) {
        count = 2;
        type = GL_HALF_FLOAT;
        normalized = false;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R16G16B16_SFLOAT) {
        count = 3;
        type = GL_HALF_FLOAT;
        normalized = false;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R16G16B16A16_SNORM) {
        count = 4;
        type = GL_SHORT;
        normalized = true;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R16G16B16A16_SFLOAT) {
        count = 4;
        type = GL_HALF_FLOAT;
        normalized = false;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R32G32B32_SFLOAT) {
        count = 3;
        type = GL_FLOAT;
        normalized = false;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R32G32B32A32_SFLOAT) {
        count = 4;
        type = GL_FLOAT;
        normalized = false;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R32G32_SFLOAT) {
        count = 2;
        type = GL_FLOAT;
        normalized = false;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R32_SFLOAT) {
        count = 1;
        type = GL_FLOAT;
        normalized = false;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R8G8B8A8_UINT) {
        count = 4;
        type = GL_UNSIGNED_BYTE;
        normalized = false;
        isFloat = false;
    } else if (attributeRef.format == BASE_FORMAT_R8G8B8A8_UNORM) {
        count = 4;
        type = GL_UNSIGNED_BYTE;
        normalized = true;
        isFloat = true;
    } else if (attributeRef.format == BASE_FORMAT_R32G32B32A32_UINT) {
        count = 4;
        type = GL_UNSIGNED_INT;
        normalized = false;
        isFloat = false;
    } else if (attributeRef.format == BASE_FORMAT_R32_UINT) {
        count = 1;
        type = GL_UNSIGNED_INT;
        normalized = false;
        isFloat = false;
    } else {
        PLUGIN_ASSERT_MSG(false, "Unhandled attribute format");
    }
}

DynamicStateFlags GetDynamicStateFlags(const array_view<const DynamicStateEnum> dynamicStates)
{
    DynamicStateFlags flags = 0;
    for (const auto& ref : dynamicStates) {
        switch (ref) {
            case CORE_DYNAMIC_STATE_ENUM_VIEWPORT:
                flags |= CORE_DYNAMIC_STATE_VIEWPORT;
                break;
            case CORE_DYNAMIC_STATE_ENUM_SCISSOR:
                flags |= CORE_DYNAMIC_STATE_SCISSOR;
                break;
            case CORE_DYNAMIC_STATE_ENUM_LINE_WIDTH:
                flags |= CORE_DYNAMIC_STATE_LINE_WIDTH;
                break;
            case CORE_DYNAMIC_STATE_ENUM_DEPTH_BIAS:
                flags |= CORE_DYNAMIC_STATE_DEPTH_BIAS;
                break;
            case CORE_DYNAMIC_STATE_ENUM_BLEND_CONSTANTS:
                flags |= CORE_DYNAMIC_STATE_BLEND_CONSTANTS;
                break;
            case CORE_DYNAMIC_STATE_ENUM_DEPTH_BOUNDS:
                flags |= CORE_DYNAMIC_STATE_DEPTH_BOUNDS;
                break;
            case CORE_DYNAMIC_STATE_ENUM_STENCIL_COMPARE_MASK:
                flags |= CORE_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
                break;
            case CORE_DYNAMIC_STATE_ENUM_STENCIL_WRITE_MASK:
                flags |= CORE_DYNAMIC_STATE_STENCIL_WRITE_MASK;
                break;
            case CORE_DYNAMIC_STATE_ENUM_STENCIL_REFERENCE:
                flags |= CORE_DYNAMIC_STATE_STENCIL_WRITE_MASK;
                break;
            default:
                break;
        }
    }
    return flags;
}

uint32_t HighestBit(uint32_t value)
{
    uint32_t count = 0;
    while (value) {
        ++count;
        value >>= 1U;
    }
    return count;
}
} // namespace

GraphicsPipelineStateObjectGLES::GraphicsPipelineStateObjectGLES(Device& device,
    const GpuShaderProgram& gpuShaderProgram, const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
    const VertexInputDeclarationView& vertexInputDeclaration,
    const ShaderSpecializationConstantDataView& specializationConstants,
    const array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
    const array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex)
    : GraphicsPipelineStateObject(), device_((DeviceGLES&)device)
{
    plat_.graphicsState = graphicsState;
    plat_.pipelineLayout = pipelineLayout;
    for (const auto& t : vertexInputDeclaration.attributeDescriptions) {
        plat_.vertexInputDeclaration.attributeDescriptions[plat_.vertexInputDeclaration.attributeDescriptionCount] = t;
        plat_.vertexInputDeclaration.attributeDescriptionCount++;
    }
    for (const auto& t : vertexInputDeclaration.bindingDescriptions) {
        plat_.vertexInputDeclaration.bindingDescriptions[plat_.vertexInputDeclaration.bindingDescriptionCount] = t;
        plat_.vertexInputDeclaration.bindingDescriptionCount++;
    }
    plat_.renderPassDesc = renderPassDesc;
    plat_.dynamicStateFlags = GetDynamicStateFlags(dynamicStates);

    const auto& source = static_cast<const GpuShaderProgramGLES&>(gpuShaderProgram);

    uint32_t views = 0U;
    for (const auto& subpass : renderPassSubpassDescs) {
        views = Math::max(HighestBit(subpass.viewMask), views);
    }
    plat_.views = views;

    specialized_ = source.Specialize(specializationConstants, views);
    plat_.graphicsShader = specialized_.get();
    if (plat_.graphicsShader) {
        MakeVAO();
    }
}

GpuShaderProgramGLES* GraphicsPipelineStateObjectGLES::GetOESProgram(array_view<const OES_Bind> oesBinds) const
{
    BASE_NS::string key;
    for (auto& bind : oesBinds) {
        key += BASE_NS::to_string(bind.set);
        key += '_';
        key += BASE_NS::to_string(bind.bind);
        key += '_';
    }
    if (auto it = oesPrograms_.find(key); it != oesPrograms_.end()) {
        // return existing.
        return it->second.get();
    }
    // create new one.
    return oesPrograms_.insert({ key, specialized_->OesPatch(oesBinds, plat_.views) }).first->second.get();
}

void GraphicsPipelineStateObjectGLES::MakeVAO() noexcept
{
    PLUGIN_ASSERT(device_.IsActive());
    if (plat_.vao == 0) {
        const auto& shaderdata = (const GpuShaderProgramPlatformDataGL&)plat_.graphicsShader->GetPlatformData();
        PLUGIN_ASSERT(shaderdata.program != 0);
        const uint32_t curVAO = device_.BoundVertexArray();
        plat_.vao = device_.CreateVertexArray();
        device_.BindVertexArray(plat_.vao);
        const VertexInputDeclarationData& vertexInputDecl = plat_.vertexInputDeclaration;
        for (size_t idx = 0; idx < vertexInputDecl.attributeDescriptionCount; ++idx) {
            if (shaderdata.inputs[idx] != Gles::INVALID_LOCATION) {
                const auto& attributeRef = vertexInputDecl.attributeDescriptions[idx];
                GLuint count = 2;
                GLenum type = GL_FLOAT;
                GLboolean normalized = false;
                bool isFloat = false;
                FormatToVertexType(attributeRef, count, type, normalized, isFloat);
                glEnableVertexAttribArray((GLuint)idx);
                glVertexAttribBinding((GLuint)idx, attributeRef.binding);
                if (isFloat) {
                    glVertexAttribFormat((GLuint)idx, (GLint)count, type, normalized, (GLuint)attributeRef.offset);
                } else {
                    glVertexAttribIFormat((GLuint)idx, (GLint)count, type, (GLuint)attributeRef.offset);
                }
            } else {
                glDisableVertexAttribArray((GLuint)idx);
            }
        }
        device_.BindVertexArray(curVAO);
    }
}

GraphicsPipelineStateObjectGLES::~GraphicsPipelineStateObjectGLES()
{
    if (plat_.vao) {
        PLUGIN_ASSERT(device_.IsActive());
        device_.DeleteVertexArray(plat_.vao);
    }
}

const PipelineStateObjectPlatformDataGL& GraphicsPipelineStateObjectGLES::GetPlatformData() const
{
    return plat_;
}

ComputePipelineStateObjectGLES::ComputePipelineStateObjectGLES(Device& device,
    const GpuComputeProgram& gpuComputeProgram, const PipelineLayout& pipelineLayout,
    const ShaderSpecializationConstantDataView& specializationConstants)
    : ComputePipelineStateObject(), device_((DeviceGLES&)device)
{
    PLUGIN_UNUSED(device_);
    plat_.pipelineLayout = pipelineLayout;
    const auto& source = static_cast<const GpuComputeProgramGLES&>(gpuComputeProgram);
    specialized_ = source.Specialize(specializationConstants);
    plat_.computeShader = specialized_.get();
}

ComputePipelineStateObjectGLES::~ComputePipelineStateObjectGLES() = default;

const PipelineStateObjectPlatformDataGL& ComputePipelineStateObjectGLES::GetPlatformData() const
{
    return plat_;
}
RENDER_END_NAMESPACE()
