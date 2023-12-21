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

#include "spirv_cross_helpers_gles.h"

#include <cmath>
#include <glcorearb.h>

namespace Gles {
namespace {
static const spirv_cross::SPIRConstant invalid {};

int32_t FindConstant(const std::vector<PushConstantReflection>& reflections, const PushConstantReflection& reflection)
{
    for (size_t i = 0; i < reflections.size(); i++) {
        if (reflection.name == reflections[i].name) {
            // Check that it's actually same and not a conflict!.
            if (reflection.type != reflections[i].type) {
                return -2;
            }
            if (reflection.offset != reflections[i].offset) {
                return -2;
            }
            if (reflection.size != reflections[i].size) {
                return -2;
            }
            if (reflection.arraySize != reflections[i].arraySize) {
                return -2;
            }
            if (reflection.arrayStride != reflections[i].arrayStride) {
                return -2;
            }
            if (reflection.matrixStride != reflections[i].matrixStride) {
                return -2;
            }
            return (int32_t)i;
        }
    }
    return -1;
}

const spirv_cross::SPIRConstant& ConstByName(const spirv_cross::CompilerGLSL& compiler, const char* name)
{
    const auto& specInfo = ((CoreCompiler&)compiler).GetConstants();
    for (auto& c : specInfo) {
        const auto& opName = compiler.get_name(c.self);
        if (opName == name) {
            auto& constant = compiler.get_constant(c.self);
            return constant;
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
            auto& constant = compiler.get_constant(c.id);
            return constant;
        }
    }
    // is default invalid?
    return invalid;
}
} // namespace

// inherit from CompilerGLSL to have better access
CoreCompiler::CoreCompiler(const uint32_t* ir, size_t wordCount) : CompilerGLSL(ir, wordCount) {}

const std::vector<spirv_cross::SPIRConstant> CoreCompiler::GetConstants() const
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
    std::vector<PushConstantReflection>& reflections, ShaderStageFlags stage)
{
    char ids[64];
    int id = 0;
    // There can be only one push_constant_buffer, but since spirv-cross has prepared for this to be relaxed, we will
    // too.
    std::string name = "CORE_PC_00";
    for (auto& remap : resources.push_constant_buffers) {
        const auto& blockType = compiler.get_type(remap.base_type_id);
        (void)(blockType);
        sprintf(ids, "%d", id);
        name.resize(8);
        name.append(ids);
        compiler.set_name(remap.id, name);
        assert((blockType.basetype == spirv_cross::SPIRType::Struct) && "Push constant is not a struct!");
        ProcessStruct(std::string_view(name.data(), name.size()), 0, compiler, remap.base_type_id, reflections, stage);
        id++;
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

// Converts constant declaration to uniform. (actually only works on spec constants)
void ConvertConstantToUniform(const spirv_cross::CompilerGLSL& compiler, std::string& source, const char* name)
{
    std::string tmp;
    tmp.reserve(strlen(name) + 16);
    const auto& constant = ConstByName(compiler, name);
    if (constant.self == invalid.self) {
        return;
    }
    const auto& type = compiler.get_type(constant.constant_type);
    if (type.basetype == spirv_cross::SPIRType::Boolean) {
        tmp += "const bool ";
    } else if (type.basetype == spirv_cross::SPIRType::UInt) {
        tmp += "const uint ";
    } else if (type.basetype == spirv_cross::SPIRType::Int) {
        tmp += "const int ";
    } else if (type.basetype == spirv_cross::SPIRType::Float) {
        tmp += "const float ";
    } else {
        assert(false && "Unhandled specialization constant type");
    }
    // We expect spirv_cross to generate them with certain pattern..
    tmp += name;
    tmp += " =";
    const auto p = source.find(tmp);
    if (p != std::string::npos) {
        // found it, change it. (changes const to uniform)
        auto bi = source.begin() + (int64_t)p;
        auto ei = bi + 6;
        source.replace(bi, ei, "uniform ");

        // remove the initializer..
        const auto p2 = source.find('=', p);
        const auto p3 = source.find(';', p);
        if ((p2 != std::string::npos) && (p3 != std::string::npos)) {
            if (p2 < p3) {
                // should be correct (tm)
                bi = source.begin() + (int64_t)p2;
                ei = source.begin() + (int64_t)p3;
                source.erase(bi, ei);
            }
        }
    }
}

void SetSpecMacro(spirv_cross::CompilerGLSL& compiler, const char* name, uint32_t value)
{
    const auto& vc = SpecConstByName(compiler, name);
    if (vc.self != invalid.self) {
        const uint32_t constantId = compiler.get_decoration(vc.self, spv::Decoration::DecorationSpecId);
        char buf[1024];
        sprintf(buf, "#define SPIRV_CROSS_CONSTANT_ID_%u %du", constantId, value);
        compiler.add_header_line(buf);
    }
}

void ProcessStruct(std::string_view baseName, size_t baseOffset, const spirv_cross::Compiler& compiler,
    uint32_t structTypeId, std::vector<PushConstantReflection>& reflections, ShaderStageFlags stage)
{
    const auto& structType = compiler.get_type(structTypeId);
    reflections.reserve(reflections.size() + structType.member_types.size());
    for (uint32_t bi = 0; bi < structType.member_types.size(); bi++) {
        const uint32_t memberTypeId = structType.member_types[bi];
        const auto& memberType = compiler.get_type(memberTypeId);
        const auto& name = compiler.get_member_name(structTypeId, bi);

        PushConstantReflection t;
        t.stage = stage;
        t.name = baseName;
        t.name += '.';
        t.name += name;
        t.arrayStride = 0;
        t.matrixStride = 0;
        // Get member offset within this struct.
        t.offset = baseOffset + compiler.type_struct_member_offset(structType, bi);
        t.size = compiler.get_declared_struct_member_size(structType, bi);
        t.arraySize = 0;
        if (!memberType.array.empty()) {
            // Get array stride, e.g. float4 foo[]; Will have array stride of 16 bytes.
            t.arrayStride = compiler.type_struct_member_array_stride(structType, bi);
            t.arraySize = memberType.array[0]; // We don't support arrays of arrays. just use the size of first.
        }

        if (memberType.columns > 1) {
            // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
            t.matrixStride = compiler.type_struct_member_matrix_stride(structType, bi);
        }

        switch (memberType.basetype) {
            case spirv_cross::SPIRType::Struct:
                ProcessStruct(t.name, t.offset, compiler, memberTypeId, reflections, stage);
                continue;
                break;
            case spirv_cross::SPIRType::UInt: {
                constexpr GLenum type[5][5] = { { 0, 0, 0, 0, 0 }, { 0, GL_UNSIGNED_INT, 0, 0, 0 },
                    { 0, GL_UNSIGNED_INT_VEC2, 0, 0, 0 }, { 0, GL_UNSIGNED_INT_VEC3, 0, 0, 0 },
                    { 0, GL_UNSIGNED_INT_VEC4, 0, 0, 0 } };
                t.type = type[memberType.vecsize][memberType.columns];
                break;
            }
            case spirv_cross::SPIRType::Float: {
                constexpr GLenum type[5][5] = {
                    { 0, 0, 0, 0, 0 },
                    { 0, GL_FLOAT, 0, 0, 0 },
                    { 0, GL_FLOAT_VEC2, GL_FLOAT_MAT2, GL_FLOAT_MAT3x2, GL_FLOAT_MAT4x2 },
                    { 0, GL_FLOAT_VEC3, GL_FLOAT_MAT2x3, GL_FLOAT_MAT3, GL_FLOAT_MAT4x3 },
                    { 0, GL_FLOAT_VEC4, GL_FLOAT_MAT2x4, GL_FLOAT_MAT3x4, GL_FLOAT_MAT4 },
                };
                t.type = type[memberType.vecsize][memberType.columns];
                break;
            }
            case spirv_cross::SPIRType::Unknown:
            case spirv_cross::SPIRType::Void:
            case spirv_cross::SPIRType::Boolean:
            case spirv_cross::SPIRType::Char:
            case spirv_cross::SPIRType::SByte:
            case spirv_cross::SPIRType::UByte:
            case spirv_cross::SPIRType::Short:
            case spirv_cross::SPIRType::UShort:
            case spirv_cross::SPIRType::Int:
            case spirv_cross::SPIRType::Int64:
            case spirv_cross::SPIRType::UInt64:
            case spirv_cross::SPIRType::AtomicCounter:
            case spirv_cross::SPIRType::Half:
            case spirv_cross::SPIRType::Double:
            case spirv_cross::SPIRType::Image:
            case spirv_cross::SPIRType::SampledImage:
            case spirv_cross::SPIRType::Sampler:
            case spirv_cross::SPIRType::AccelerationStructure:
            case spirv_cross::SPIRType::ControlPointArray:
            default:
                t.type = 0;
                break;
        }
        assert((t.type != 0) && "Unhandled Type!");
        const int32_t res = FindConstant(reflections, t);
        assert((res >= -1) && "Push constant conflict.");
        if (res == -1) {
            reflections.push_back(t);
        }
    }
}

#ifdef PLUGIN_UNUSED_SPRIV_CROSS_HELPERS
bool DefineForSpec(
    const std::vector<SpecConstantInfo>& reflectionInfo, uint32_t spcid, uintptr_t offset, std::string& result)
{
    // "#define SPIRV_CROSS_CONSTANT_ID_4294967295 4294967295\n" //worst case for bool
    // "#define SPIRV_CROSS_CONSTANT_ID_4294967295 4294967295\n" //worst case for uint32
    // "#define SPIRV_CROSS_CONSTANT_ID_4294967295 -2147483648\n"//worst case for int32
    // and floats can be REALLY long..
    char buf[1024];
    bool ok = false;
    for (const auto& c : reflectionInfo) {
        if (c.constantId == spcid) {
            if (c.name == "CORE_BACKEND_TYPE") {
                ok = true; // backend type can't change anymore..
                continue;
            }
            const auto& type = c.constantType;
            [[maybe_unused]] const size_t size = c.vectorSize * c.columns * sizeof(uint32_t);
            assert((size == sizeof(uint32_t)) && "Specialization constant size is not 4!");
            //  The constant_id can only be applied to a scalar *int*, a scalar *float* or a scalar *bool*.
            //  https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_gl_spirv.txt
            if ((type == SpecConstantInfo::Types::BOOL) || (type == SpecConstantInfo::Types::UINT32)) {
                const uint32_t value = *reinterpret_cast<uint32_t*>(offset);
                const int len = sprintf(buf, "%u %uu\n", c.constantId, value);
                ok = len > 0;
            } else if (type == SpecConstantInfo::Types::INT32) {
                const int32_t value = *reinterpret_cast<int32_t*>(offset);
                const int len = sprintf(buf, "%u %d\n", c.constantId, value);
                ok = len > 0;
            } else if (type == SpecConstantInfo::Types::FLOAT) {
                const float value = *reinterpret_cast<float_t*>(offset);
                // NOTE: resulting constant might not be the same. due to float -> string -> float conversions.
                const int len = sprintf(buf, "%u %f\n", c.constantId, value);
                ok = len > 0;
            } else {
                assert(false && "Unhandled specialization constant type");
            }
            if (ok) {
                result.append("#define SPIRV_CROSS_CONSTANT_ID_");
                result.append(buf);
            }
            break;
        }
    }
    return ok;
}

std::string InsertDefines(std::string_view shaderIn, std::string_view Defines)
{
    std::string shaderOut;
    // Create defines..
    if (!shaderIn.empty()) {
        const size_t voff = shaderIn.find_first_of('\n');
        shaderOut.reserve(shaderIn.length() + Defines.length());
        shaderOut.append(shaderIn.substr(0, voff + 1));
        shaderOut.append(Defines);
        shaderOut.append(shaderIn.substr(voff + 1));
    } else {
        shaderOut = Defines;
    }
    return shaderOut;
}

std::string Specialize(ShaderStageFlags mask, std::string_view shaderTemplate,
    const std::vector<SpecConstantInfo>& info, const ShaderSpecializationConstantDataView& data)
{
    if (shaderTemplate.empty()) {
        return {};
    }
    bool ok = false;
    for (const auto& spc : data.constants) {
        if (spc.shaderStage & mask) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        // nothing to specialize
        return std::string(shaderTemplate);
    }
    // Create defines..
    const uintptr_t base = (uintptr_t)data.data.data();
    std::string defines;
    defines.reserve(256);
    for (const auto& spc : data.constants) {
        if (spc.shaderStage & mask) {
            const uintptr_t offset = base + spc.offset;
            DefineForSpec(info, spc.id, offset, defines);
        }
    }
    // inject defines to shader source.
    return InsertDefines(shaderTemplate, defines);
}

void CreateSpecInfos(const spirv_cross::Compiler& compiler, std::vector<SpecConstantInfo>& outSpecInfo)
{
    const auto& specInfo = compiler.get_specialization_constants();
    for (const auto& c : specInfo) {
        SpecConstantInfo t;
        t.constantId = c.constant_id;
        const spirv_cross::SPIRConstant& constant = compiler.get_constant(c.id);
        const auto& name = compiler.get_name(c.id);
        const auto type = compiler.get_type(constant.constant_type);
        if (type.basetype == spirv_cross::SPIRType::Boolean) {
            t.constantType = SpecConstantInfo::Types::BOOL;
        } else if (type.basetype == spirv_cross::SPIRType::UInt) {
            t.constantType = SpecConstantInfo::Types::UINT32;
        } else if (type.basetype == spirv_cross::SPIRType::Int) {
            t.constantType = SpecConstantInfo::Types::INT32;
        } else if (type.basetype == spirv_cross::SPIRType::Float) {
            t.constantType = SpecConstantInfo::Types::FLOAT;
        } else {
            assert(false && "Unhandled specialization constant type");
        }
        t.vectorSize = constant.vector_size();
        t.columns = constant.columns();
        t.name.assign(name.data(), name.size());
        outSpecInfo.push_back(std::move(t));
    }
}

uint32_t ConstId(spirv_cross::CompilerGLSL& compiler, const char* name)
{
    const auto& c = ConstByName(compiler, name);
    return c.self;
}

uint32_t SpecConstId(spirv_cross::CompilerGLSL& compiler, const char* name)
{
    const auto& c = SpecConstByName(compiler, name);
    return c.self;
}
#endif

} // namespace Gles
