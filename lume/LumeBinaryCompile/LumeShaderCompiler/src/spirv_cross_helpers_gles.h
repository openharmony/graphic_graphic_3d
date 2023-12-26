/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef GLES_SPIRV_CROSS_HELPERS_H
#define GLES_SPIRV_CROSS_HELPERS_H

#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <spirv_parser.hpp>
#include <string>
#include <string_view>

#include "spirv_cross_helper_structs_gles.h"

namespace Gles {
// inherit from CompilerGLSL to have better access
class CoreCompiler final : public spirv_cross::CompilerGLSL {
public:
    CoreCompiler(const uint32_t* ir, size_t wordCount);
    ~CoreCompiler() = default;
    const std::vector<spirv_cross::SPIRConstant> GetConstants() const;
    const spirv_cross::ParsedIR& GetIr() const;
};

void ReflectPushConstants(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
    std::vector<PushConstantReflection>& reflections, ShaderStageFlags stage);

// Converts specialization constant to normal constant, (to reduce unnecessary clutter in glsl)
void ConvertSpecConstToConstant(spirv_cross::CompilerGLSL& compiler, const char* name);

// Converts constant declaration to uniform. (actually only works on spec constants)
void ConvertConstantToUniform(const spirv_cross::CompilerGLSL& compiler, std::string& source, const char* name);

void SetSpecMacro(spirv_cross::CompilerGLSL& compiler, const char* name, uint32_t value);

void ProcessStruct(std::string_view baseName, size_t baseOffset, const spirv_cross::Compiler& compiler,
    uint32_t structTypeId, std::vector<PushConstantReflection>& reflections, ShaderStageFlags stage);

#ifdef PLUGIN_UNUSED_SPRIV_CROSS_HELPERS
bool DefineForSpec(
    const std::vector<SpecConstantInfo>& reflectionInfo, uint32_t spcid, uintptr_t offset, std::string& result);

std::string InsertDefines(std::string_view shaderIn, std::string_view Defines);

std::string Specialize(ShaderStageFlags mask, std::string_view shaderTemplate,
    const std::vector<SpecConstantInfo>& info, const ShaderSpecializationConstantDataView& data);

void CreateSpecInfos(const spirv_cross::Compiler& compiler, std::vector<SpecConstantInfo>& outSpecInfo);

uint32_t ConstId(spirv_cross::CompilerGLSL& compiler, const char* name);

uint32_t SpecConstId(spirv_cross::CompilerGLSL& compiler, const char* name);
#endif
} // namespace Gles

#endif // GLES_SPIRV_CROSS_HELPERS_H
