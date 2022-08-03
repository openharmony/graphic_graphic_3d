/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gles/gpu_program_gles.h"

#include <algorithm>
#include <cmath>

#include <base/containers/unordered_map.h>

#include "device/gpu_program_util.h"
#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/shader_module_gles.h"
#include "util/log.h"
#include "util/string_util.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
// although max set is 4...
#define BIND_MAP_4_4(a, b) (((a) << 4) | (b))

// 16 samplers and 16 textures = 256 combinations
// this is actually not enugh. since there can be 32 sampled images (16 in vertex and 16 in fragment) + 32 samplers (16
// in vertex and 16 in fragment)
static constexpr uint32_t MAX_FINAL_BIND_MAP_A { 16 };
static constexpr uint32_t MAX_FINAL_BIND_MAP_B { 16 };
static constexpr uint32_t MAX_FINAL_BINDS { MAX_FINAL_BIND_MAP_A * MAX_FINAL_BIND_MAP_B };

struct BindMaps {
    uint8_t maxImageBinding { 0 };    // next free imagetexture unit
    uint8_t maxUniformBinding { 0 };  // next free uniform block binding
    uint8_t maxStorageBinding { 0 };  // next free storege buffer binding
    uint8_t maxCSamplerBinding { 0 }; // next combined sampler binding

    uint8_t maxSamplerBinding { 0 }; // next sampler binding
    uint8_t maxTextureBinding { 0 }; // next texture binding
    uint8_t maxFinalBinding {
        0
    }; // "real" last combined sampler binding (last generated combination + maxCSamplerBinding)

    uint8_t map[Gles::ResourceLimits::MAX_BINDS] { 0 }; // mapping from set/binding -> "unit/binding"
    uint8_t finalMap[MAX_FINAL_BINDS] { 0 }; // mapping from sampler/texture combination -> texture sampler unit
    vector<Gles::PushConstantReflection> pushConstants;
};

void ProcessPushConstants(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, BindMaps& map)
{
#if 1
    GLsizei len;
    const GLenum uniform_props[] = { flag, GL_LOCATION };
    GLint inUse[BASE_NS::countof(uniform_props)] { 0 };
    for (auto& info : plat.infos) {
        // check for duplicates.. (there can be dupes since vertex and fragment both can use the same constant..)
        if (auto pos = std::find_if(map.pushConstants.begin(), map.pushConstants.end(),
                [&name = info.name](auto& info2) { return info2.name == name; });
            pos != map.pushConstants.end()) {
            pos->stage |= info.stage;
        } else {
            const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, info.name.c_str());
            if (index != GL_INVALID_INDEX) {
                glGetProgramResourceiv(program, GL_UNIFORM, index, (GLsizei)countof(uniform_props), uniform_props,
                    (GLsizei)sizeof(inUse), &len, inUse);
                if (inUse[0]) {
                    map.pushConstants.push_back(info);
                    map.pushConstants.back().location = inUse[1];
                }
            }
        }
    }
#else
    GLsizei len;
    const GLenum uniform_props[] = { flag, GL_BLOCK_INDEX, GL_NAME_LENGTH, GL_LOCATION, GL_TYPE, GL_OFFSET,
        GL_ARRAY_SIZE, GL_ARRAY_STRIDE, GL_MATRIX_STRIDE };
    GLint inUse[BASE_NS::countof(uniform_props)] { 0 };

    GLint numUniforms = 0;
    glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);

    GLint maxNameLength = 0;
    glGetProgramInterfaceiv(program, GL_UNIFORM, GL_MAX_NAME_LENGTH, &maxNameLength);
    string nameData;
    nameData.reserve(maxNameLength);

    BASE_NS::vector<Gles::PushConstantReflection> infos;
    for (auto& info : plat.infos) {
        for (GLint unif = 0U; unif < numUniforms; ++unif) {
            glGetProgramResourceiv(program, GL_UNIFORM, unif, BASE_NS::countof(uniform_props), uniform_props,
                BASE_NS::countof(inUse), nullptr, inUse);

            // Skip any uniforms that don't fulfill the given flags.
            if (!inUse[0]) {
                continue;
            }
            // Skip any uniforms that are in a block.
            if (inUse[1] != -1) {
                continue;
            }
            nameData.resize(inUse[2]);
            glGetProgramResourceName(program, GL_UNIFORM, unif, nameData.size(), nullptr, nameData.data());
            if (nameData.substr(0, info.name.size()) == info.name) {
                // check for duplicates.. (there can be dupes since vertex and fragment both can use the same
                // constant..)
                if (auto pos = std::find_if(map.pushConstants.begin(), map.pushConstants.end(),
                        [&name = nameData](const Gles::PushConstantReflection& info2) { return info2.name == name; });
                    pos != map.pushConstants.end()) {
                    pos->stage |= info.stage;
                } else {
                    Gles::PushConstantReflection refl {};
                    refl.stage = info.stage;
                    refl.location = inUse[3];
                    refl.type = inUse[4];
                    refl.name = nameData;
                    refl.offset = inUse[5];
                    refl.size;
                    refl.arraySize = inUse[6];
                    refl.arrayStride = inUse[7];
                    refl.matrixStride = inUse[8];
                    map.pushConstants.push_back(refl);
                }
            }
        }
    }
#endif
}

void ProcessStorageBlocks(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, const BindMaps& map)
{
    GLsizei len;
    const GLenum block_props[] = { flag, GL_BUFFER_BINDING };
    GLint inUse[BASE_NS::countof(block_props)] { 0 };
    for (const auto& t : plat.sbSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, (GLsizei)countof(block_props), block_props,
                (GLsizei)sizeof(inUse), &len, inUse);
            if (inUse[0]) {
                const uint8_t fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                PLUGIN_UNUSED(fi);
                PLUGIN_ASSERT(inUse[1] == (fi - 1));
            }
        }
    }
}

void ProcessUniformBlocks(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, BindMaps& map)
{
    GLsizei len;
    const GLenum block_props[] = { flag, GL_BUFFER_BINDING };
    GLint inUse[BASE_NS::countof(block_props)] { 0 };
    for (const auto& t : plat.ubSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        if (t.arrayElements > 1) {
            GLint inUse2[BASE_NS::countof(block_props)] { 0 };
            // need to handle arrays separately, (since each index is a separate resource..)
            uint8_t& fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
            fi = (uint8_t)(map.maxUniformBinding + 1);
            for (uint32_t i = 0; i < t.arrayElements; i++) {
                string tmp = t.name;
                char buf[16];
                if (sprintf_s(buf, sizeof(buf), "[%u]", i) < 0) {
                    PLUGIN_LOG_E("ProcessUniformBlocks: sprintf_s failed");
                }
                tmp += buf;
                const GLuint elementIndex = glGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, tmp.c_str());
                glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, elementIndex, (GLsizei)countof(block_props),
                    block_props, (GLsizei)sizeof(inUse2), &len, inUse2);
                /*
                 * we could do this optimization, but currently no way to signal backend that array element is not in
                 * use.
                 * if (inUse2[0]) {
                 *     uint8_t& fi = map.uniformBindingSets.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                 *     fi = ++map.uniformBindingSets.maxBind;
                 *     glUniformBlockBinding(program, i, (GLuint)(fi - 1));
                 * } else {
                 *     mark as unused. no need to bind.
                 * }
                 */
                glUniformBlockBinding(program, i, (GLuint)(fi - 1 + i));
                ++map.maxUniformBinding;
            }
        } else {
            const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, t.name.c_str());
            if (index != GL_INVALID_INDEX) {
                glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, index, (GLsizei)countof(block_props), block_props,
                    (GLsizei)sizeof(inUse), &len, inUse);
                if (inUse[0]) {
                    uint8_t& fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                    if (fi == 0) {
                        fi = ++map.maxUniformBinding;
                        glUniformBlockBinding(program, index, (GLuint)(fi - 1));
                    }
                }
            }
        }
    }
}

void ProcessImageTextures(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, const BindMaps& map)
{
    GLsizei len;
    const GLenum uniform_props[] = { flag, GL_LOCATION };
    GLint inUse[BASE_NS::countof(uniform_props)] { 0 };
    for (const auto& t : plat.ciSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(program, GL_UNIFORM, index, (GLsizei)countof(uniform_props), uniform_props,
                (GLsizei)sizeof(inUse), &len, inUse);
            if (inUse[0]) {
                const uint8_t& fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                GLint unit = 0;
                glGetUniformiv(program, inUse[1], &unit);
                PLUGIN_UNUSED(fi);
                PLUGIN_ASSERT(unit == (fi - 1));
            }
        }
    }
}

void ProcessCombinedSamplers(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, BindMaps& map)
{
    GLsizei len;
    const GLenum props[] = { flag, GL_LOCATION };
    GLint inUse[BASE_NS::countof(props)] { 0 };
    for (const auto& t : plat.combSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        PLUGIN_ASSERT(t.sSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.sBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(
                program, GL_UNIFORM, index, (GLsizei)countof(props), props, (GLsizei)sizeof(inUse), &len, inUse);
            if (inUse[0]) {
                uint8_t& ii = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                if (ii == 0) {
                    ii = ++map.maxTextureBinding;
                }
                uint8_t& si = map.map[BIND_MAP_4_4(t.sSet, t.sBind)];
                if (si == 0) {
                    si = ++map.maxSamplerBinding;
                }
                PLUGIN_ASSERT(si < MAX_FINAL_BIND_MAP_A);
                PLUGIN_ASSERT(ii < MAX_FINAL_BIND_MAP_B);
                uint8_t& fi = map.finalMap[BIND_MAP_4_4(si, ii)];
                if (fi == 0) {
                    fi = ++map.maxFinalBinding;
                    // assign texture unit for 'sampler'
                    glProgramUniform1i(program, inUse[1], fi - 1);
                }
            }
        }
    }
}

void ProcessSubPassInputs(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, BindMaps& map)
{
    GLsizei len;
    const GLenum uniform_props[] = { flag, GL_LOCATION };
    GLint inUse[BASE_NS::countof(uniform_props)] { 0 };
    for (const auto& t : plat.siSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(program, GL_UNIFORM, index, (GLsizei)countof(uniform_props), uniform_props,
                (GLsizei)sizeof(inUse), &len, inUse);
            if (inUse[0]) {
                uint8_t& fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                if (fi == 0) {
                    fi = ++map.maxFinalBinding;
                    glProgramUniform1i(program, inUse[1], (fi - 1));
                }
            }
        }
    }
}

void ProcessSamplers(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, BindMaps& map)
{
    GLsizei len;
    const GLenum uniform_props[] = { flag, GL_LOCATION, GL_ARRAY_SIZE };
    GLint inUse[BASE_NS::countof(uniform_props)] { 0 };
    for (const auto& t : plat.cbSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(program, GL_UNIFORM, index, (GLsizei)countof(uniform_props), uniform_props,
                (GLsizei)sizeof(inUse), &len, inUse);
            if (inUse[0]) {
                uint8_t& fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                if (fi == 0) {
                    // Assign texture units to each "sampler".
                    GLint units[Gles::ResourceLimits::MAX_SAMPLERS_IN_STAGE];
                    fi = (uint8_t)(map.maxCSamplerBinding + 1);
                    for (int i = 0; i < inUse[2]; i++) {
                        // (we have NO WAY of knowing if the index is actually used..)
                        units[i] = fi + i - 1;
                    }
                    map.maxCSamplerBinding += (uint8_t)inUse[2];
                    glProgramUniform1iv(program, inUse[1], inUse[2], units);
                }
            }
        }
    }
}

void ProcessProgram(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, BindMaps& map)
{
    // Collect used resources, and create mapping from set/bind(vulkan) to bind(gl)
    // Process push constants..
    ProcessPushConstants(program, plat, flag, map);
    // Process shader storage blocks. (these have been bound in source, so just filter actually unused ones.)
    ProcessStorageBlocks(program, plat, flag, map);
    // Process "imagetexture":s (these have been bound in source, so just filter actually unused ones.)
    ProcessImageTextures(program, plat, flag, map);

    // Process combined samplers (actual sampler2D etc.  allocates binds from combinedSamplers.finalBind)..
    ProcessSamplers(program, plat, flag, map);
    map.maxFinalBinding = map.maxCSamplerBinding;

    // Process combined samplers (create map from image / sampler binds to single combined texture)..
    ProcessCombinedSamplers(program, plat, flag, map);
    // Process uniform blocks
    ProcessUniformBlocks(program, plat, flag, map);
    // Process sub-pass inputs (allocate bind from combinedSamplers.finalBind)..
    ProcessSubPassInputs(program, plat, flag, map);
}

size_t Locate(const string& source, const string& match, size_t& cp)
{
    const size_t dp = source.find(match, cp);
    if (dp == string::npos) {
        return string::npos;
    }
    const char ch = source[dp + match.length()];
    if ((ch != ';') && (ch != '\n')) {
        return string::npos;
    }
    // find end-of-previous-line..
    auto start = source.rfind('\n', dp);
    if (start == string::npos) {
        start = 0;
    }
    start++;
    // okay, find end-of-line..
    const auto end = source.find('\n', start);

    // find the binding declaration..
    const auto p = source.find("binding = 11", start);
    if ((p == string::npos) || (p > end)) {
        return string::npos;
    }
    return p;
}

void SetValue(string& source, size_t p, uint32_t final)
{
    uint8_t tmp = (uint8_t)(final - 1);
    PLUGIN_ASSERT(tmp < 99);
    source[p + 10] = ' ';
    if (tmp > 10) {
        source[p + 10] = (tmp / 10) + '0';
    }
    source[p + 11] = (tmp % 10) + '0';
}

struct binder {
    uint8_t& maxBind;
    uint8_t* map;
};

template<typename T, size_t N, typename TypeOfOther>
void FixBindings(T (&types)[N], binder& map, const TypeOfOther& sbSets, string& source)
{
    if (sbSets.size() > 0) {
        // We expect spirv_cross to generate them with certain pattern..
        // layout(std430, set = 3, binding = 2) buffer MyBuffer
        // ->   (note: we force binding=1 during spirv-cross compile.
        // layout(std430, binding = 1) buffer MyBuffer
        size_t cp = 0;
        for (const auto& type : types) {
            for (const auto& t : sbSets) {
                auto p = Locate(source, type + t.name, cp);
                if (p == string::npos) {
                    continue;
                }
                // okay.. do it then..
                PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
                PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
                uint8_t& final = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                if (final == 0) {
                    final = ++map.maxBind;
                }
                PLUGIN_ASSERT(final < Gles::ResourceLimits::MAX_BIND_IN_SET);
                // replace the binding point...
                SetValue(source, p, final);
            }
        }
    }
}

template<typename ProgramPlatformData>
void PostProcessSource(
    BindMaps& map, ProgramPlatformData& plat, const ShaderModulePlatformDataGLES& modPlat, string& source)
{
    // Special handling for shader storage blocks and images...
    constexpr string_view buf[] = { " buffer " };
    constexpr string_view images[] = { " image2D ", " iimage2D ", " uimage2D ", " image2DArray ", " iimage2DArray ",
        " uimage2DArray ", " image3D ", " iimage3D ", " uimage3D ", " imageCube ", " iimageCube ", " uimageCube ",
        " imageCubeArray ", " iimageCubeArray ", " uimageCubeArray ", " imageBuffer ", " iimageBuffer ",
        " uimageBuffer " };
    binder storageBindings { map.maxStorageBinding, map.map };
    FixBindings(buf, storageBindings, modPlat.sbSets, source);
    binder imageBindings { map.maxImageBinding, map.map };
    FixBindings(images, imageBindings, modPlat.ciSets, source);
}

void BuildBindInfos(vector<Binder>& bindinfos, const PipelineLayout& pipelineLayout, BindMaps& map,
    const vector<ShaderModulePlatformDataGLES::DoubleBind>& combSets)
{
    vector<Binder> samplers;
    vector<Binder> others;
    for (uint32_t set = 0; set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; set++) {
        const auto& s = pipelineLayout.descriptorSetLayouts[set];
        if (s.set == PipelineLayoutConstants::INVALID_INDEX) {
            continue;
        }
        PLUGIN_ASSERT(set == s.set);
        for (const auto& b : s.bindings) {
            Binder tmp;
            tmp.set = s.set;
            tmp.bind = b.binding;
            tmp.type = b.descriptorType;
            tmp.id.resize(b.descriptorCount);
            bool add = false;
            for (size_t index = 0; index < b.descriptorCount; index++) {
                switch (b.descriptorType) {
                    case CORE_DESCRIPTOR_TYPE_SAMPLER: {
                        const uint32_t sid = map.map[BIND_MAP_4_4(s.set, b.binding)];
                        if (sid) {
                            for (auto cs : combSets) {
                                if ((cs.sSet == s.set) && (cs.sBind == b.binding)) {
                                    const uint32_t iid = map.map[BIND_MAP_4_4(cs.iSet, cs.iBind)];
                                    if (iid) {
                                        uint32_t final = map.finalMap[BIND_MAP_4_4(sid, iid)];
                                        if (final) {
                                            tmp.id[index].push_back(final - 1);
                                            add = true;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                        const uint32_t iid = map.map[BIND_MAP_4_4(s.set, b.binding)];
                        if (iid) {
                            for (auto cs : combSets) {
                                if ((cs.iSet == s.set) && (cs.iBind == b.binding)) {
                                    const uint32_t sid = map.map[BIND_MAP_4_4(cs.sSet, cs.sBind)];
                                    if (sid) {
                                        uint32_t final = map.finalMap[BIND_MAP_4_4(sid, iid)];
                                        if (final) {
                                            tmp.id[index].push_back(final - 1);
                                            add = true;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    case CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    case CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                        uint32_t id = map.map[BIND_MAP_4_4(s.set, b.binding)];
                        if (id) {
                            tmp.id[index].push_back(id - 1);
                            add = true;
                        }
                        break;
                    }
                    case CORE_DESCRIPTOR_TYPE_MAX_ENUM:
                    default:
                        PLUGIN_ASSERT_MSG(false, "Unhandled descriptor type");
                        break;
                }
            }
            if (add) {
                if (b.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
                    samplers.emplace_back(move(tmp));
                } else {
                    others.emplace_back(move(tmp));
                }
            }
        }
    }
    // add samplers first. helps with oes.
    bindinfos.reserve(samplers.size() + others.size());
    for (auto& s : samplers) {
        bindinfos.emplace_back(move(s));
    }
    for (auto& o : others) {
        bindinfos.emplace_back(move(o));
    }
}

void PatchOesBinds(
    const array_view<const OES_Bind>& oesBinds, const ShaderModulePlatformDataGLES& fragPlat, string& fragSource)
{
    if (!oesBinds.empty()) {
        // find names of oes binds (we only patch Sampler2D's)
        auto p = fragSource.find("\n", 0);
        const string_view extension("#extension GL_OES_EGL_image_external_essl3 : enable\n");
        fragSource.insert(p + 1, extension.data(), extension.size());
        for (const auto& bnd : oesBinds) {
            for (const auto& t : fragPlat.cbSets) {
                if ((t.iSet != bnd.set) || (t.iBind != bnd.bind)) {
                    continue;
                }
                // We expect to find one matching declaration of the sampler in the source.
                StringUtil::FindAndReplaceOne(
                    fragSource, " sampler2D " + t.name + ";", " samplerExternalOES " + t.name + ";");
            }
            for (const auto& t : fragPlat.combSets) {
                if ((t.iSet != bnd.set) || (t.iBind != bnd.bind)) {
                    continue;
                }
                // We expect to find one matching declaration of the sampler in the source.
                StringUtil::FindAndReplaceOne(
                    fragSource, " sampler2D " + t.name + ";", " samplerExternalOES " + t.name + ";");
            }
        }
    }
}
} // namespace

GpuShaderProgramGLES::GpuShaderProgramGLES(Device& device) : GpuShaderProgram(), device_((DeviceGLES&)device) {}

GpuShaderProgramGLES::~GpuShaderProgramGLES()
{
    if (plat_.program) {
        PLUGIN_ASSERT(device_.IsActive());
        device_.ReleaseProgram(plat_.program);
    }
}

GpuShaderProgramGLES::GpuShaderProgramGLES(Device& device, const GpuShaderProgramCreateData& createData)
    : GpuShaderProgram(), device_((DeviceGLES&)device)
{
    PLUGIN_ASSERT(createData.vertShaderModule);
    // combine vertex and fragment shader data
    if (createData.vertShaderModule && createData.fragShaderModule) {
        plat_.vertShaderModule_ = static_cast<const ShaderModuleGLES*>(createData.vertShaderModule);
        plat_.fragShaderModule_ = static_cast<const ShaderModuleGLES*>(createData.fragShaderModule);
        auto& pipelineLayout = reflection_.pipelineLayout;
        // vert
        pipelineLayout = plat_.vertShaderModule_->GetPipelineLayout();
        const auto& sscv = plat_.vertShaderModule_->GetSpecilization();
        constants_ = vector<ShaderSpecialization::Constant>(sscv.constants.cbegin().ptr(), sscv.constants.cend().ptr());
        // not owned, directly reflected from vertex shader module
        const auto& vidv = plat_.vertShaderModule_->GetVertexInputDeclaration();
        reflection_.vertexInputDeclarationView = vidv;
        // frag
        const auto& reflPl = plat_.fragShaderModule_->GetPipelineLayout();
        // has sort inside
        GpuProgramUtil::CombinePipelineLayouts({ &reflPl, 1u }, pipelineLayout);

        const auto& fsscv = plat_.fragShaderModule_->GetSpecilization();
        // has sort inside
        GpuProgramUtil::CombineSpecializationConstants(fsscv.constants, constants_);
        reflection_.shaderSpecializationConstantView.constants = constants_;
    }
}

void GpuShaderProgramGLES::FilterInputs(GpuShaderProgramGLES& ret) const
{
    GLint inputs;
    uint32_t inputLocations[Gles::ResourceLimits::MAX_VERTEXINPUT_ATTRIBUTES];
    enum propertyIndices { LOCATION = 0, VERTEX_REF = 1, FRAGMENT_REF = 2, MAX_INDEX };
    const GLenum inputProps[] = { GL_LOCATION, GL_REFERENCED_BY_VERTEX_SHADER, GL_REFERENCED_BY_FRAGMENT_SHADER };
    static constexpr GLsizei PropertyCount = static_cast<GLsizei>(countof(inputProps));
    static_assert(PropertyCount == MAX_INDEX);
    GLint values[PropertyCount];
    const GLuint program = static_cast<GLuint>(ret.plat_.program);
    glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &inputs);
    uint32_t inputsInUse = 0;
    for (GLint i = 0; i < inputs; i++) {
        GLsizei wrote;
        glGetProgramResourceiv(program, GL_PROGRAM_INPUT, static_cast<GLuint>(i), PropertyCount, inputProps,
            PropertyCount, &wrote, values);
        if ((values[LOCATION] != Gles::INVALID_LOCATION) &&
            ((values[VERTEX_REF] == GL_TRUE) || (values[FRAGMENT_REF] == GL_TRUE))) {
            inputLocations[inputsInUse] = static_cast<uint32_t>((values[LOCATION]));
            inputsInUse++;
        }
    }
    const auto& list = ret.reflection_.vertexInputDeclarationView.attributeDescriptions;
    PLUGIN_ASSERT(list.size() < Gles::ResourceLimits::MAX_VERTEXINPUT_ATTRIBUTES);
    for (size_t i = list.size(); i < Gles::ResourceLimits::MAX_VERTEXINPUT_ATTRIBUTES; i++) {
        ret.plat_.inputs[i] = Gles::INVALID_LOCATION;
    }
    for (size_t i = 0; i < list.size(); i++) {
        ret.plat_.inputs[i] = Gles::INVALID_LOCATION;
        for (uint32_t j = 0; j < inputsInUse; j++) {
            if (list[i].location == inputLocations[j]) {
                ret.plat_.inputs[i] = static_cast<int32_t>(i);
                break;
            }
        }
    }
}

GpuShaderProgramGLES* GpuShaderProgramGLES::Specialize(const ShaderSpecializationConstantDataView& specData) const
{
    return Specialize(specData, {});
}

GpuShaderProgramGLES* GpuShaderProgramGLES::OesPatch(const array_view<const OES_Bind>& binds) const
{
    ShaderSpecializationConstantDataView specData;
    specData.constants = constants_;
    specData.data = { specializedWith.data(), specializedWith.size() };
    return Specialize(specData, binds);
}

GpuShaderProgramGLES* GpuShaderProgramGLES::Specialize(
    const ShaderSpecializationConstantDataView& specData, const array_view<const OES_Bind>& oesBinds) const
{
    BindMaps map {};
    PLUGIN_ASSERT(device_.IsActive());
    PLUGIN_ASSERT(plat_.vertShaderModule_);
    PLUGIN_ASSERT(plat_.fragShaderModule_);
    string vertSource = plat_.vertShaderModule_->GetGLSL(specData);
    PLUGIN_ASSERT_MSG(!vertSource.empty(), "Trying to specialize a program with no vert source");
    string fragSource = plat_.fragShaderModule_->GetGLSL(specData);
    PLUGIN_ASSERT_MSG(!fragSource.empty(), "Trying to specialize a program with no frag source");
    const auto& vertPlat = static_cast<const ShaderModulePlatformDataGLES&>(plat_.vertShaderModule_->GetPlatformData());
    const auto& fragPlat = static_cast<const ShaderModulePlatformDataGLES&>(plat_.fragShaderModule_->GetPlatformData());
    unique_ptr<GpuShaderProgramGLES> ret { new GpuShaderProgramGLES(device_) };
    ret->specializedWith = { specData.data.begin(), specData.data.end() };
    ret->plat_.vertShaderModule_ = plat_.vertShaderModule_;
    ret->plat_.fragShaderModule_ = plat_.fragShaderModule_;
    ret->reflection_ = reflection_;
    ret->constants_ = constants_;
    ret->reflection_.shaderSpecializationConstantView.constants = ret->constants_;
    // Special handling for shader storage blocks and images...
    PostProcessSource(map, ret->plat_, vertPlat, vertSource);
    PostProcessSource(map, ret->plat_, fragPlat, fragSource);

    // if there are oes binds, patches the string (fragSource)
    PatchOesBinds(oesBinds, fragPlat, fragSource);

    // Compile / Cache binary
    ret->plat_.program = device_.CacheProgram(vertSource, fragSource, string_view());
    PLUGIN_ASSERT_MSG(ret->plat_.program, "Compilation failed");
    if (ret->plat_.program == 0) {
        return nullptr;
    }
    // build the map tables..
    ProcessProgram(ret->plat_.program, vertPlat, GL_REFERENCED_BY_VERTEX_SHADER, map);
    ProcessProgram(ret->plat_.program, fragPlat, GL_REFERENCED_BY_FRAGMENT_SHADER, map);
    ret->pushConstants = map.pushConstants;
    ret->plat_.pushConstants = ret->pushConstants;
    ret->plat_.flipLocation = glGetUniformLocation(ret->plat_.program, "CORE_FLIP_NDC");
    FilterInputs(*ret);

    const auto& pipelineLayout = reflection_.pipelineLayout;
    // create a union of the "new" combined samplers (created from separate sampler/image binds)
    vector<ShaderModulePlatformDataGLES::DoubleBind> combSets = fragPlat.combSets;
    for (const auto& c : vertPlat.combSets) {
        bool newEntry = true;
        for (const auto& b : combSets) {
            if ((c.iSet == b.iSet) && (c.sSet == b.sSet) && (c.iBind == b.iBind) && (c.sBind == b.sBind)) {
                newEntry = false;
                break;
            }
        }
        if (newEntry) {
            combSets.push_back(c);
        }
    }
    BuildBindInfos(ret->resourceList, pipelineLayout, map, combSets);
    ret->plat_.resourceList = ret->resourceList;
    return ret.release();
}

const GpuShaderProgramPlatformDataGL& GpuShaderProgramGLES::GetPlatformData() const
{
    return plat_;
}

const ShaderReflection& GpuShaderProgramGLES::GetReflection() const
{
    return reflection_;
}

GpuComputeProgramGLES::GpuComputeProgramGLES(Device& device) : GpuComputeProgram(), device_((DeviceGLES&)device) {}

GpuComputeProgramGLES::GpuComputeProgramGLES(Device& device, const GpuComputeProgramCreateData& createData)
    : GpuComputeProgram(), device_((DeviceGLES&)device)
{
    PLUGIN_ASSERT(createData.compShaderModule);
    plat_ = GpuComputeProgramPlatformDataGL {};
    reflection_ = {};
    if (createData.compShaderModule) {
        plat_.module_ = static_cast<const ShaderModuleGLES*>(createData.compShaderModule);
        reflection_.pipelineLayout = plat_.module_->GetPipelineLayout();
        const auto& tgs = plat_.module_->GetThreadGroupSize();
        reflection_.threadGroupSizeX = Math::max(1u, tgs.x);
        reflection_.threadGroupSizeY = Math::max(1u, tgs.y);
        reflection_.threadGroupSizeZ = Math::max(1u, tgs.z);
        reflection_.shaderSpecializationConstantView = plat_.module_->GetSpecilization();
        const auto& constants = reflection_.shaderSpecializationConstantView.constants;
        constants_ = vector<ShaderSpecialization::Constant>(constants.cbegin().ptr(), constants.cend().ptr());
        // sorted based on offset due to offset mapping with shader combinations
        // NOTE: id and name indexing
        std::sort(constants_.begin(), constants_.end(),
            [](const auto& lhs, const auto& rhs) { return (lhs.offset < rhs.offset); });
        reflection_.shaderSpecializationConstantView.constants = constants_;
    }
}

GpuComputeProgramGLES::~GpuComputeProgramGLES()
{
    if (plat_.program) {
        PLUGIN_ASSERT(device_.IsActive());
        device_.ReleaseProgram(plat_.program);
    }
}

const GpuComputeProgramPlatformDataGL& GpuComputeProgramGLES::GetPlatformData() const
{
    PLUGIN_ASSERT(plat_.program);
    return plat_;
}

const ComputeShaderReflection& GpuComputeProgramGLES::GetReflection() const
{
    PLUGIN_ASSERT(plat_.module_);
    return reflection_;
}

GpuComputeProgramGLES* GpuComputeProgramGLES::Specialize(const ShaderSpecializationConstantDataView& specData) const
{
    BindMaps map {};
    PLUGIN_ASSERT(device_.IsActive());
    PLUGIN_ASSERT(plat_.module_);
    string compSource = plat_.module_->GetGLSL(specData);
    PLUGIN_ASSERT_MSG(!compSource.empty(), "Trying to specialize a program with no source");
    const auto& plat = static_cast<const ShaderModulePlatformDataGLES&>(plat_.module_->GetPlatformData());
    GpuComputeProgramGLES* ret = new GpuComputeProgramGLES(device_);
    ret->plat_.module_ = plat_.module_;
    ret->reflection_ = reflection_;
    ret->constants_ = constants_;
    ret->reflection_.shaderSpecializationConstantView.constants = ret->constants_;
    // Special handling for shader storage blocks and images...
    PostProcessSource(map, ret->plat_, plat, compSource);
    // Compile / Cache binary
    ret->plat_.program = device_.CacheProgram(string_view(), string_view(), compSource);
    PLUGIN_ASSERT(ret->plat_.program);
    if (ret->plat_.program == 0) {
        // something went wrong.
        delete ret;
        return nullptr;
    }
    // build the map tables..
    ProcessProgram(ret->plat_.program, plat, GL_REFERENCED_BY_COMPUTE_SHADER, map);
    ret->pushConstants = map.pushConstants;
    ret->plat_.pushConstants = ret->pushConstants;
    ret->plat_.flipLocation = glGetUniformLocation(ret->plat_.program, "CORE_FLIP_NDC");

    const auto& pipelineLayout = reflection_.pipelineLayout;
    BuildBindInfos(ret->resourceList, pipelineLayout, map, plat.combSets);
    ret->plat_.resourceList = ret->resourceList;
    return ret;
}
RENDER_END_NAMESPACE()
