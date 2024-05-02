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

#ifndef GLES_SHADER_MODULE_GLES_H
#define GLES_SHADER_MODULE_GLES_H

#include <base/containers/string.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/shader_module.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
struct ShaderModuleCreateInfo;
namespace Gles {
struct PushConstantReflection;
struct SpecConstantInfo;
} // namespace Gles
struct ShaderModulePlatformDataGLES : ShaderModulePlatformData {
    BASE_NS::vector<Gles::PushConstantReflection> infos;
    struct Bind {
        uint8_t iSet, iBind;   // binding set, binding index
        uint8_t arrayElements; // for array binds.
        BASE_NS::string name;  // name opengl uniform/block
    };
    BASE_NS::vector<Bind> ubSets; // uniform blocks
    BASE_NS::vector<Bind> sbSets; // shader storage blocks
    BASE_NS::vector<Bind> siSets; // subpass inputs
    BASE_NS::vector<Bind> ciSets; // image textures
    BASE_NS::vector<Bind> cbSets; // combined textures (sampler2D etc)
    struct DoubleBind {
        uint8_t sSet, sBind;  // sampler binding set, binding index
        uint8_t iSet, iBind;  // image binding set, binding index
        BASE_NS::string name; // name of combined image/sampler
    };
    BASE_NS::vector<DoubleBind> combSets; // combined image / sampler (generated from separated image/sampler)
};

class ShaderModuleGLES final : public ShaderModule {
public:
    ShaderModuleGLES(Device&, const ShaderModuleCreateInfo&);
    ~ShaderModuleGLES();

    ShaderStageFlags GetShaderStageFlags() const override;

    const ShaderModulePlatformData& GetPlatformData() const override;

    const PipelineLayout& GetPipelineLayout() const override;
    ShaderSpecializationConstantView GetSpecilization() const override;

    VertexInputDeclarationView GetVertexInputDeclaration() const override;
    ShaderThreadGroup GetThreadGroupSize() const override;

    BASE_NS::string GetGLSL(const ShaderSpecializationConstantDataView&) const;

private:
    Device& device_;
    ShaderStageFlags shaderStageFlags_ { 0u };
    ShaderModulePlatformDataGLES plat_;
    BASE_NS::vector<VertexInputDeclaration::VertexInputBindingDescription> vertexInputBindingDescriptions_;
    BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> vertexInputAttributeDescriptions_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;

    PipelineLayout pipelineLayout_;
    ShaderSpecializationConstantView sscv_;
    VertexInputDeclarationView vidv_;
    ShaderThreadGroup stg_;

    BASE_NS::string source_;
    BASE_NS::vector<Gles::SpecConstantInfo> specInfo_;
    template<typename ShaderBase>
    friend void ProcessShaderModule(ShaderBase&, const ShaderModuleCreateInfo&);
    template<typename ShaderBase>
    friend BASE_NS::string SpecializeShaderModule(const ShaderBase&, const ShaderSpecializationConstantDataView&);
};
RENDER_END_NAMESPACE()

#endif // GLES_SHADER_MODULE_GLES_H
