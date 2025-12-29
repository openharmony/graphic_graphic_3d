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

#include "spirv_cross_helpers_gles.h"

#include <glcorearb.h>

namespace Gles {
namespace {
constexpr GLenum UINT_TYPES[5][5] = { { 0, 0, 0, 0, 0 }, { 0, GL_UNSIGNED_INT, 0, 0, 0 },
    { 0, GL_UNSIGNED_INT_VEC2, 0, 0, 0 }, { 0, GL_UNSIGNED_INT_VEC3, 0, 0, 0 }, { 0, GL_UNSIGNED_INT_VEC4, 0, 0, 0 } };
constexpr GLenum FLOAT_TYPES[5][5] = {
    { 0, 0, 0, 0, 0 },
    { 0, GL_FLOAT, 0, 0, 0 },
    { 0, GL_FLOAT_VEC2, GL_FLOAT_MAT2, GL_FLOAT_MAT3x2, GL_FLOAT_MAT4x2 },
    { 0, GL_FLOAT_VEC3, GL_FLOAT_MAT2x3, GL_FLOAT_MAT3, GL_FLOAT_MAT4x3 },
    { 0, GL_FLOAT_VEC4, GL_FLOAT_MAT2x4, GL_FLOAT_MAT3x4, GL_FLOAT_MAT4 },
};

template<typename T>
constexpr T Max(T&& lhs, T&& rhs)
{
    return lhs > rhs ? std::forward<T>(lhs) : std::forward<T>(rhs);
}

const spirv_cross::SPIRConstant invalid{};

constexpr int32_t NOT_FOUND = -1;
constexpr int32_t INVALID_MATCH = -2;

int32_t FindConstant(const std::vector<PushConstantReflection>& reflections, const PushConstantReflection& reflection)
{
    for (size_t i = 0; i < reflections.size(); i++) {
        if (reflection.name == reflections[i].name) {
            // Check that it's actually same and not a conflict!.
            if (reflection.type != reflections[i].type) {
                return INVALID_MATCH;
            }
            if (reflection.offset != reflections[i].offset) {
                return INVALID_MATCH;
            }
            if (reflection.size != reflections[i].size) {
                return INVALID_MATCH;
            }
            if (reflection.arraySize != reflections[i].arraySize) {
                return INVALID_MATCH;
            }
            if (reflection.arrayStride != reflections[i].arrayStride) {
                return INVALID_MATCH;
            }
            if (reflection.matrixStride != reflections[i].matrixStride) {
                return INVALID_MATCH;
            }
            return static_cast<int32_t>(i);
        }
    }
    return NOT_FOUND;
}

const spirv_cross::SPIRConstant& ConstByName(const CoreCompiler& compiler, const char* name)
{
    const auto& specInfo = compiler.GetConstants();
    for (auto& c : specInfo) {
        const auto& opName = compiler.get_name(c.self);
        if (opName == name) {
            return compiler.get_constant(c.self);
        }
    }
    // is default invalid?
    return invalid;
}

const spirv_cross::SPIRConstant& SpecConstByName(const spirv_cross::CompilerGLSL& compiler, const char* name)
{
    const auto& specInfo = compiler.get_specialization_constants();
    for (const auto& c : specInfo) {
        const auto& opName = compiler.get_name(c.id);
        if (opName == name) {
            return compiler.get_constant(c.id);
        }
    }
    // is default invalid?
    return invalid;
}
} // namespace

// inherit from CompilerGLSL to have better access
CoreCompiler::CoreCompiler(const uint32_t* ir, size_t wordCount) : CompilerGLSL(ir, wordCount) {}

std::vector<spirv_cross::SPIRConstant> CoreCompiler::GetConstants() const
{
    std::vector<spirv_cross::SPIRConstant> consts;
    ir.for_each_typed_id<spirv_cross::SPIRConstant>(
        [&consts](uint32_t, const spirv_cross::SPIRConstant& c) { consts.push_back(c); });
    return consts;
}

const spirv_cross::ParsedIR& CoreCompiler::GetIr() const
{
    return ir;
}

void ReflectPushConstants(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
    std::vector<PushConstantReflection>& reflections, const ShaderStageFlags stage)
{
    static constexpr std::string_view pcName = "CORE_PC_00";
    static constexpr auto nameBaseSize = pcName.size() - 2U; // length without the 2 digits
    unsigned int id = 0;
    // There can be only one push_constant_buffer, but since spirv-cross has prepared for this to be relaxed, we will
    // too.
    Gles::PushConstantReflection base{};
    base.stage = stage;
    base.name = pcName;
    for (auto& remap : resources.push_constant_buffers) {
        const auto& blockType = compiler.get_type(remap.base_type_id);
        (void)(blockType);

        base.name.resize(nameBaseSize);
        base.name.append(std::to_string(id));
        compiler.set_name(remap.id, base.name);
        assert((blockType.basetype == spirv_cross::SPIRType::Struct) && "Push constant is not a struct!");
        ProcessStruct(compiler, base, remap.base_type_id, reflections);
        ++id;
    }
}

// Converts specialization constant to normal constant, (to reduce unnecessary clutter in glsl)
void ConvertSpecConstToConstant(spirv_cross::CompilerGLSL& compiler, const char* name)
{
    const auto& c = SpecConstByName(compiler, name);
    if (c.self == invalid.self) {
        return;
    }
    compiler.unset_decoration(c.self, spv::Decoration::DecorationSpecId);
}

/// Converts constant declaration to uniform. (actually only works on spec constants)
void ConvertConstantToUniform(const CoreCompiler& compiler, std::string& source, const char* name)
{
    static constexpr std::string_view constBool = "const bool ";
    static constexpr std::string_view constUint = "const uint ";
    static constexpr std::string_view constInt = "const int ";
    static constexpr std::string_view constFloat = "const float ";
    static constexpr std::string_view equals = " =";
    static constexpr auto extraSpace =
        Max(Max(constBool.size(), constUint.size()), Max(constInt.size(), constFloat.size())) + equals.size();
    std::string tmp;
    tmp.reserve(strlen(name) + extraSpace);
    const auto& constant = ConstByName(compiler, name);
    if (constant.self == invalid.self) {
        return;
    }
    const auto& type = compiler.get_type(constant.constant_type);
    if (type.basetype == spirv_cross::SPIRType::Boolean) {
        tmp += constBool;
    } else if (type.basetype == spirv_cross::SPIRType::UInt) {
        tmp += constUint;
    } else if (type.basetype == spirv_cross::SPIRType::Int) {
        tmp += constInt;
    } else if (type.basetype == spirv_cross::SPIRType::Float) {
        tmp += constFloat;
    } else {
        assert(false && "Unhandled specialization constant type");
    }
    // We expect spirv_cross to generate them with certain pattern...
    tmp += name;
    tmp += equals;
    const auto p = source.find(tmp);
    if (p != std::string::npos) {
        // found it, change it. (changes const to uniform)
        auto bi = source.begin() + static_cast<int64_t>(p);
        auto ei = bi + 6;
        source.replace(bi, ei, "uniform ");

        // remove the initializer...
        const auto p2 = source.find('=', p);
        const auto p3 = source.find(';', p);
        if ((p2 != std::string::npos) && (p3 != std::string::npos)) {
            if (p2 < p3) {
                // should be correct (tm)
                bi = source.begin() + static_cast<int64_t>(p2);
                ei = source.begin() + static_cast<int64_t>(p3);
                source.erase(bi, ei);
            }
        }
    }
}

void SetSpecMacro(spirv_cross::CompilerGLSL& compiler, const char* name, const uint32_t value)
{
    const auto& vc = SpecConstByName(compiler, name);
    if (vc.self == invalid.self) {
        return;
    }
    const auto constantId = compiler.get_decoration(vc.self, spv::Decoration::DecorationSpecId);
    compiler.add_header_line(
        "#define SPIRV_CROSS_CONSTANT_ID_" + std::to_string(constantId) + " " + std::to_string(value) + "u");
}

void ProcessStruct(const spirv_cross::Compiler& compiler, const PushConstantReflection& base, // NOLINT(*-no-recursion)
    const uint32_t structTypeId, std::vector<PushConstantReflection>& reflections)
{
    const auto& structType = compiler.get_type(structTypeId);
    reflections.reserve(reflections.size() + structType.member_types.size());
    for (uint32_t bi = 0; bi < structType.member_types.size(); bi++) {
        const uint32_t memberTypeId = structType.member_types[bi];
        const auto& memberType = compiler.get_type(memberTypeId);
        const auto& name = compiler.get_member_name(structTypeId, bi);

        PushConstantReflection t{ base.stage, INVALID_LOCATION, 0u, {},
            base.offset + compiler.type_struct_member_offset(structType, bi),
            compiler.get_declared_struct_member_size(structType, bi), 0u, 0u, 0u };
        t.name.reserve(base.name.size() + 1U + name.size());
        t.name = base.name;
        t.name += '.';
        t.name += name;
        if (!memberType.array.empty()) {
            // Get array stride, e.g. float4 foo[]; Will have array stride of 16 bytes.
            t.arrayStride = compiler.type_struct_member_array_stride(structType, bi);
            t.arraySize = memberType.array[0]; // We don't support arrays of arrays. just use the size of first.
        }

        if (memberType.columns > 1) {
            // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
            t.matrixStride = compiler.type_struct_member_matrix_stride(structType, bi);
        }

        if (memberType.basetype == spirv_cross::SPIRType::UInt) {
            t.type = UINT_TYPES[memberType.vecsize][memberType.columns];
        } else if (memberType.basetype == spirv_cross::SPIRType::Float) {
            t.type = FLOAT_TYPES[memberType.vecsize][memberType.columns];
        } else if (memberType.basetype == spirv_cross::SPIRType::Struct) {
            ProcessStruct(compiler, t, memberTypeId, reflections);
            continue;
        }
        assert((t.type != 0) && "Unhandled Type!");
        const int32_t res = FindConstant(reflections, t);
        assert((res >= NOT_FOUND) && "Push constant conflict.");
        if (res == NOT_FOUND) {
            reflections.push_back(std::move(t));
        }
    }
}
} // namespace Gles
