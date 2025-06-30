/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "gles/gpu_program_gles.h"

#include <algorithm>

#include <base/containers/fixed_string.h>
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
constexpr uint32_t MAX_FINAL_BIND_MAP_A { 16 };
constexpr uint32_t MAX_FINAL_BIND_MAP_B { 16 };
constexpr uint32_t MAX_FINAL_BINDS { MAX_FINAL_BIND_MAP_A * MAX_FINAL_BIND_MAP_B };

struct BindMaps {
    uint8_t maxImageBinding { 0 };   // next free imagetexture unit
    uint8_t maxUniformBinding { 0 }; // next free uniform block binding
    uint8_t maxStorageBinding { 0 }; // next free storege buffer binding

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
    GLsizei len;
    const GLenum uniformProperties[] = { flag, GL_LOCATION };
    constexpr auto propertyCount = static_cast<GLsizei>(countof(uniformProperties));
    GLint inUse[propertyCount] { 0 };
    for (auto& info : plat.infos) {
        // check for duplicates.. (there can be dupes since vertex and fragment both can use the same constant..)
        if (auto pos = std::find_if(map.pushConstants.begin(), map.pushConstants.end(),
                [&name = info.name](auto& info2) { return info2.name == name; });
            pos != map.pushConstants.end()) {
            pos->stage |= info.stage;
        } else {
            const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, info.name.c_str());
            if (index != GL_INVALID_INDEX) {
                glGetProgramResourceiv(
                    program, GL_UNIFORM, index, propertyCount, uniformProperties, propertyCount, &len, inUse);
                if (inUse[0]) {
                    map.pushConstants.push_back(info);
                    map.pushConstants.back().location = inUse[1];
                }
            }
        }
    }
}

void ProcessStorageBlocks(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, const BindMaps& map)
{
#if defined(RENDER_VALIDATION_ENABLED) && (RENDER_VALIDATION_ENABLED)
    GLsizei len;
    const GLenum blockProperties[] = { flag, GL_BUFFER_BINDING };
    constexpr auto propertyCount = static_cast<GLsizei>(countof(blockProperties));
    GLint inUse[propertyCount] { 0 };
    for (const auto& t : plat.sbSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(
                program, GL_SHADER_STORAGE_BLOCK, index, propertyCount, blockProperties, propertyCount, &len, inUse);
            if (inUse[0]) {
                const uint8_t fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                if (inUse[1] != (fi - 1)) {
                    PLUGIN_LOG_E("Set %d, binding %d mapped to %d, expected %d", t.iSet, t.iBind, inUse[1], (fi - 1));
                }
            }
        }
    }
#endif
}

void ProcessUniformBlocks(GLuint program, const ShaderModulePlatformDataGLES& plat, GLenum flag, BindMaps& map)
{
    GLsizei len;
    const GLenum blockProperties[] = { flag, GL_BUFFER_BINDING };
    constexpr auto propertyCount = static_cast<GLsizei>(countof(blockProperties));
    GLint inUse[propertyCount] { 0 };
    for (const auto& t : plat.ubSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        if (t.arrayElements > 1) {
            // need to handle arrays separately, (since each index is a separate resource..)
            uint8_t& fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
            fi = (uint8_t)(map.maxUniformBinding + 1);
            string tmp;
            for (uint32_t i = 0; i < t.arrayElements; i++) {
                const auto iStr = BASE_NS::to_string(i);
                tmp.reserve(t.name.size() + iStr.size() + 2U);
                tmp = t.name;
                tmp += '[';
                tmp += iStr;
                tmp += ']';
                /*
                 * we could do this optimization, but currently no way to signal backend that array element is not in
                 * use.
                 * GLint inUse2[propertyCount] { 0 };
                 * const GLuint elementIndex = glGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, tmp.c_str());
                 * glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, elementIndex, propertyCount,
                 *   blockProperties, propertyCount, &len, inUse2);                 
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
                glGetProgramResourceiv(
                    program, GL_UNIFORM_BLOCK, index, propertyCount, blockProperties, propertyCount, &len, inUse);
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
    const GLenum uniformProperties[] = { flag, GL_LOCATION };
    constexpr auto propertyCount = static_cast<GLsizei>(countof(uniformProperties));
    GLint inUse[propertyCount] { 0 };
    for (const auto& t : plat.ciSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(
                program, GL_UNIFORM, index, propertyCount, uniformProperties, propertyCount, &len, inUse);
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
    constexpr auto propertyCount = static_cast<GLsizei>(countof(props));
    GLint inUse[propertyCount] { 0 };
    for (const auto& t : plat.combSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        PLUGIN_ASSERT(t.sSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.sBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
        glGetProgramResourceiv(program, GL_UNIFORM, index, propertyCount, props, propertyCount, &len, inUse);
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
    const GLenum uniformProperties[] = { flag, GL_LOCATION };
    constexpr auto propertyCount = static_cast<GLsizei>(countof(uniformProperties));
    GLint inUse[propertyCount] { 0 };
    for (const auto& t : plat.siSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(
                program, GL_UNIFORM, index, propertyCount, uniformProperties, propertyCount, &len, inUse);
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
    const GLenum uniformProperties[] = { flag, GL_LOCATION, GL_ARRAY_SIZE };
    constexpr auto propertyCount = static_cast<GLsizei>(countof(uniformProperties));
    GLint inUse[propertyCount] { 0 };
    for (const auto& t : plat.cbSets) {
        PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
        PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
        const GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, t.name.c_str());
        if (index != GL_INVALID_INDEX) {
            glGetProgramResourceiv(
                program, GL_UNIFORM, index, propertyCount, uniformProperties, propertyCount, &len, inUse);
            if (inUse[0]) {
                uint8_t& fi = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                if (fi == 0) {
                    // Assign texture units to each "sampler".
                    GLint units[Gles::ResourceLimits::MAX_SAMPLERS_IN_STAGE];
                    fi = (uint8_t)(map.maxFinalBinding + 1);
                    for (int i = 0; i < inUse[2]; i++) {
                        // (we have NO WAY of knowing if the index is actually used..)
                        units[i] = fi + i - 1;
                    }
                    map.maxFinalBinding += (uint8_t)inUse[2];
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

    // Process combined samplers (create map from image / sampler binds to single combined texture)..
    ProcessCombinedSamplers(program, plat, flag, map);
    // Process uniform blocks
    ProcessUniformBlocks(program, plat, flag, map);
    // Process sub-pass inputs (allocate bind from combinedSamplers.finalBind)..
    ProcessSubPassInputs(program, plat, flag, map);
}

void SetValue(char* source, uint32_t final)
{
    const auto tmp = final - 1;
    PLUGIN_ASSERT(tmp < 99);
    const auto div = static_cast<char>((tmp / 10u) + '0');
    const auto mod = static_cast<char>((tmp % 10u) + '0');
    source[10u] = (tmp > 10u) ? div : ' ';
    source[11u] = mod;
}

struct binder {
    uint8_t& maxBind;
    uint8_t* map;
    vector<size_t>& bindingIndices;
};

template<typename T, size_t N, typename TypeOfOther>
void FixBindings(T (&types)[N], binder& map, const TypeOfOther& sbSets, string& source)
{
    // SetValue does inplace replacements so the string's data addess is constant and a string_view can be used while
    // going through the idices, types, and names.
    const auto data = source.data();
    auto view = string_view(source);
    // remove patched bindings so they are not considered for other type/name combinations.
    auto& indices = map.bindingIndices;
    indices.erase(std::remove_if(indices.begin(), indices.end(),
                      [&view, &types, &sbSets, &map, data](const size_t bindingI) {
                          for (const auto& type : types) {
                              const auto eol = view.find('\n', bindingI);
                              const auto typeI = view.find(type, bindingI);
                              if ((typeI == string_view::npos) || (typeI > eol)) {
                                  continue;
                              }
                              const auto afterType = view.substr(typeI + type.size());
                              for (const auto& t : sbSets) {
                                  PLUGIN_ASSERT(t.iSet < Gles::ResourceLimits::MAX_SETS);
                                  PLUGIN_ASSERT(t.iBind < Gles::ResourceLimits::MAX_BIND_IN_SET);
                                  if (!afterType.starts_with(t.name)) {
                                      continue;
                                  }
                                  // expect to find end of declaration, end of line, or _number (case: multiple uniforms
                                  // with same binding, not actually checking for the number). end of line could be LF
                                  // or CRLF and someone could have left spaces after the name so isspace is reasonable.
                                  if (const char ch = afterType[t.name.size()];
                                      !std::isspace(static_cast<unsigned char>(ch)) && (ch != ';') && (ch != '_')) {
                                      continue;
                                  }
                                  // okay.. do it then..
                                  uint8_t& final = map.map[BIND_MAP_4_4(t.iSet, t.iBind)];
                                  if (final == 0) {
                                      final = ++map.maxBind;
                                  }
                                  PLUGIN_ASSERT(final < Gles::ResourceLimits::MAX_BIND_IN_SET);
                                  // replace the binding point...
                                  SetValue(data + bindingI, final);
                                  return true;
                              }
                          }
                          return false;
                      }),
        indices.cend());
}

constexpr const string_view SSBO_KEYS[] = { " buffer " };
constexpr const string_view IMAGE_KEYS[] = { " image2D ", " iimage2D ", " uimage2D ", " image2DArray ",
    " iimage2DArray ", " uimage2DArray ", " image3D ", " iimage3D ", " uimage3D ", " imageCube ", " iimageCube ",
    " uimageCube ", " imageCubeArray ", " iimageCubeArray ", " uimageCubeArray ", " imageBuffer ", " iimageBuffer ",
    " uimageBuffer " };
constexpr const string_view SPECIAL_BINDING = "binding = 11";

void PostProcessSource(BindMaps& map, const ShaderModulePlatformDataGLES& modPlat, string& source)
{
    // Special handling for shader storage blocks and images...
    // We expect spirv_cross to generate them with certain pattern..
    // layout(std430, set = 3, binding = 2) buffer MyBuffer
    // ->   (note: we force binding = 11 during spirv-cross compile.
    // layout(std430, binding = 11) buffer MyBuffer
    if (modPlat.sbSets.empty() && modPlat.ciSets.empty()) {
        // no need if there are no SSBOs or image stores.
        return;
    }

    // go through the source and gather all the special bindings. FixBindings needs to then consider only the found
    // locations instead of scanning the whole source for every type/name combination.
    vector<size_t> bindings;
    const auto view = string_view(source);
    for (auto pos = view.find(SPECIAL_BINDING); pos != string_view::npos;
         pos = view.find(SPECIAL_BINDING, pos + SPECIAL_BINDING.size())) {
        bindings.push_back(pos);
    }
    if (!bindings.empty()) {
        if (!modPlat.sbSets.empty()) {
            binder storageBindings { map.maxStorageBinding, map.map, bindings };
            FixBindings(SSBO_KEYS, storageBindings, modPlat.sbSets, source);
        }
        if (!modPlat.ciSets.empty()) {
            binder imageBindings { map.maxImageBinding, map.map, bindings };
            FixBindings(IMAGE_KEYS, imageBindings, modPlat.ciSets, source);
        }
        // after patching the list should be empty
#if (RENDER_VALIDATION_ENABLED == 1)
        if (!bindings.empty()) {
            PLUGIN_LOG_E("RENDER_VALIDATION: GL(ES) program bindings not empty.");
        }
#endif
    }
}

void BuildBindInfos(Resources& resources, const PipelineLayout& pipelineLayout, BindMaps& map,
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
            const uint8_t id = map.map[BIND_MAP_4_4(s.set, b.binding)];
            if (!id) {
                continue;
            }
            Binder tmp;
            tmp.set = uint16_t(s.set);
            tmp.bind = uint16_t(b.binding);
            tmp.type = b.descriptorType;
            tmp.descriptors.index = uint16_t(resources.descriptorIndexIds.size());
            tmp.descriptors.count = uint16_t(b.descriptorCount);
            bool add = false;
            resources.descriptorIndexIds.append(tmp.descriptors.count, {});
            auto descriptorIndex =
                array_view(resources.descriptorIndexIds.data() + tmp.descriptors.index, tmp.descriptors.count);
            switch (b.descriptorType) {
                case CORE_DESCRIPTOR_TYPE_SAMPLER: {
                    for (const auto& cs : combSets) {
                        if ((cs.sSet != s.set) || (cs.sBind != b.binding)) {
                            continue;
                        }
                        const uint32_t iid = map.map[BIND_MAP_4_4(cs.iSet, cs.iBind)];
                        if (!iid) {
                            continue;
                        }
                        const uint8_t final = map.finalMap[BIND_MAP_4_4(id, iid)];
                        if (final) {
                            for (auto& ids : descriptorIndex) {
                                if (!ids.count) {
                                    ids.index = uint16_t(resources.ids.size());
                                }
                                ++ids.count;
                                resources.ids.push_back(final - 1U);
                            }
                            add = true;
                        }
                    }
                    break;
                }
                case CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                    for (const auto& cs : combSets) {
                        if ((cs.iSet != s.set) || (cs.iBind != b.binding)) {
                            continue;
                        }
                        const uint32_t sid = map.map[BIND_MAP_4_4(cs.sSet, cs.sBind)];
                        if (!sid) {
                            continue;
                        }
                        const uint8_t final = map.finalMap[BIND_MAP_4_4(sid, id)];
                        if (final) {
                            for (auto& ids : descriptorIndex) {
                                if (!ids.count) {
                                    ids.index = uint16_t(resources.ids.size());
                                }
                                ++ids.count;
                                resources.ids.push_back(final - 1U);
                            }
                            add = true;
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
                    for (auto& ids : descriptorIndex) {
                        ids.index = uint16_t(resources.ids.size());
                        ids.count = 1U;
                        resources.ids.push_back(id - 1U);
                    }
                    add = true;
                    break;
                }
                case CORE_DESCRIPTOR_TYPE_MAX_ENUM:
                default:
                    PLUGIN_ASSERT_MSG(false, "Unhandled descriptor type");
                    break;
            }

            if (add) {
                if (b.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
                    samplers.push_back(move(tmp));
                } else {
                    others.push_back(move(tmp));
                }
            }
        }
    }
    // add samplers first. helps with oes.
    resources.resourceList.reserve(samplers.size() + others.size());
    for (auto& s : samplers) {
        resources.resourceList.push_back(move(s));
    }
    for (auto& o : others) {
        resources.resourceList.push_back(move(o));
    }
}

void PatchOesBinds(
    const array_view<const OES_Bind>& oesBinds, const ShaderModulePlatformDataGLES& fragPlat, string& fragSource)
{
    if (!oesBinds.empty()) {
        // find names of oes binds (we only patch Sampler2D's)
        auto p = fragSource.find("\n", 0);
        constexpr string_view extension("#extension GL_OES_EGL_image_external_essl3 : enable\n");
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

void PatchMultiview(uint32_t views, string& vertSource)
{
    if (views) {
        static constexpr const string_view numViews("layout(num_views = 1)");
        if (auto pos = vertSource.find(numViews); pos != std::string::npos) {
            const auto value = vertSource.cbegin() + static_cast<ptrdiff_t>(pos + numViews.size() - 2U);
            vertSource.replace(value, value + 1, to_string(views));
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
    // combine vertex and fragment shader data
    if (createData.vertShaderModule && createData.fragShaderModule) {
        plat_.vertShaderModule_ = static_cast<const ShaderModuleGLES*>(createData.vertShaderModule);
        plat_.fragShaderModule_ = static_cast<const ShaderModuleGLES*>(createData.fragShaderModule);
        auto& pipelineLayout = reflection_.pipelineLayout;
        // vert
        pipelineLayout = plat_.vertShaderModule_->GetPipelineLayout();
        const auto& sscv = plat_.vertShaderModule_->GetSpecilization();
        // has sort inside
        GpuProgramUtil::CombineSpecializationConstants(sscv.constants, constants_);
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

void GpuShaderProgramGLES::FilterInputs(GpuShaderProgramGLES& ret)
{
    GLint inputs;
    uint32_t inputLocations[Gles::ResourceLimits::MAX_VERTEXINPUT_ATTRIBUTES];
    enum propertyIndices { LOCATION = 0, VERTEX_REF = 1, FRAGMENT_REF = 2, MAX_INDEX };
    constexpr GLenum inputProps[] = { GL_LOCATION, GL_REFERENCED_BY_VERTEX_SHADER, GL_REFERENCED_BY_FRAGMENT_SHADER };
    constexpr auto propertyCount = static_cast<GLsizei>(countof(inputProps));
    static_assert(propertyCount == MAX_INDEX);
    GLint values[propertyCount];
    const auto program = static_cast<GLuint>(ret.plat_.program);
    glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &inputs);
    uint32_t inputsInUse = 0;
    for (GLint i = 0; i < inputs; i++) {
        GLsizei wrote;
        glGetProgramResourceiv(program, GL_PROGRAM_INPUT, static_cast<GLuint>(i), propertyCount, inputProps,
            propertyCount, &wrote, values);
        if ((values[LOCATION] != Gles::INVALID_LOCATION) &&
            ((values[VERTEX_REF] == GL_TRUE) || (values[FRAGMENT_REF] == GL_TRUE))) {
            inputLocations[inputsInUse] = static_cast<uint32_t>((values[LOCATION]));
            inputsInUse++;
        }
    }

    for (auto& input : ret.plat_.inputs) {
        input = Gles::INVALID_LOCATION;
    }

    const auto& list = ret.reflection_.vertexInputDeclarationView.attributeDescriptions;
    PLUGIN_ASSERT(list.size() < Gles::ResourceLimits::MAX_VERTEXINPUT_ATTRIBUTES);
    for (size_t i = 0; i < list.size(); i++) {
        for (uint32_t j = 0; j < inputsInUse; j++) {
            if (list[i].location == inputLocations[j]) {
                ret.plat_.inputs[i] = static_cast<int32_t>(i);
                break;
            }
        }
    }
}

unique_ptr<GpuShaderProgramGLES> GpuShaderProgramGLES::Specialize(
    const ShaderSpecializationConstantDataView& specData, uint32_t views) const
{
    return Specialize(specData, {}, views);
}

unique_ptr<GpuShaderProgramGLES> GpuShaderProgramGLES::OesPatch(
    const array_view<const OES_Bind>& binds, uint32_t views) const
{
    ShaderSpecializationConstantDataView specData;
    specData.constants = constants_;
    specData.data = specializedWith;
    return Specialize(specData, binds, views);
}

unique_ptr<GpuShaderProgramGLES> GpuShaderProgramGLES::Specialize(const ShaderSpecializationConstantDataView& specData,
    const array_view<const OES_Bind>& oesBinds, uint32_t views) const
{
    PLUGIN_ASSERT(device_.IsActive());
    unique_ptr<GpuShaderProgramGLES> ret { new GpuShaderProgramGLES(device_) };
    ret->specializedWith = { specData.data.begin(), specData.data.end() };
    ret->plat_.vertShaderModule_ = plat_.vertShaderModule_;
    ret->plat_.fragShaderModule_ = plat_.fragShaderModule_;
    ret->reflection_ = reflection_;
    ret->constants_ = constants_;
    ret->reflection_.shaderSpecializationConstantView.constants = ret->constants_;

    // Special handling for shader storage blocks and images...
    BindMaps map {};

    if (!(plat_.vertShaderModule_ && plat_.fragShaderModule_)) {
        PLUGIN_LOG_E("Invalid shader module");
        return nullptr;
    }
    string vertSource = plat_.vertShaderModule_->GetGLSL(specData);
    if (vertSource.empty()) {
        PLUGIN_LOG_W("Trying to specialize a program with no vert source");
        return nullptr;
    }
    const auto& vertPlat = static_cast<const ShaderModulePlatformDataGLES&>(plat_.vertShaderModule_->GetPlatformData());
    PostProcessSource(map, vertPlat, vertSource);

    // Patch OVR_multiview num_views
    PatchMultiview(views, vertSource);

    string fragSource = plat_.fragShaderModule_->GetGLSL(specData);
    if (fragSource.empty()) {
        PLUGIN_LOG_W("Trying to specialize a program with no frag source");
        return nullptr;
    }
    const auto& fragPlat = static_cast<const ShaderModulePlatformDataGLES&>(plat_.fragShaderModule_->GetPlatformData());
    PostProcessSource(map, fragPlat, fragSource);

    // if there are oes binds, patches the string (fragSource)
    PatchOesBinds(oesBinds, fragPlat, fragSource);

    // Compile / Cache binary
    ret->plat_.program = device_.CacheProgram(vertSource, fragSource, string_view());
    if (ret->plat_.program == 0) {
        PLUGIN_LOG_E("Invalid shader shader program");
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
    BuildBindInfos(ret->resources_, pipelineLayout, map, combSets);
    ret->plat_.resourcesView.resourceList = ret->resources_.resourceList;
    ret->plat_.resourcesView.descriptorIndexIds = ret->resources_.descriptorIndexIds;
    ret->plat_.resourcesView.ids = ret->resources_.ids;
    return ret;
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
    return plat_;
}

const ComputeShaderReflection& GpuComputeProgramGLES::GetReflection() const
{
    return reflection_;
}

unique_ptr<GpuComputeProgramGLES> GpuComputeProgramGLES::Specialize(
    const ShaderSpecializationConstantDataView& specData) const
{
    PLUGIN_ASSERT(device_.IsActive());
    unique_ptr<GpuComputeProgramGLES> ret { new GpuComputeProgramGLES(device_) };
    ret->plat_.module_ = plat_.module_;
    ret->reflection_ = reflection_;
    ret->constants_ = constants_;
    ret->reflection_.shaderSpecializationConstantView.constants = ret->constants_;
    // Special handling for shader storage blocks and images...
    BindMaps map {};

    if (!plat_.module_) {
        PLUGIN_LOG_E("Invalid shader module");
        return nullptr;
    }
    string compSource = plat_.module_->GetGLSL(specData);
    if (compSource.empty()) {
        PLUGIN_LOG_W("Trying to specialize a program with no source");
    }
    const auto& plat = static_cast<const ShaderModulePlatformDataGLES&>(plat_.module_->GetPlatformData());
    PostProcessSource(map, plat, compSource);
    // Compile / Cache binary
    ret->plat_.program = device_.CacheProgram(string_view(), string_view(), compSource);
    if (ret->plat_.program == 0) {
        // something went wrong.
        PLUGIN_LOG_E("Invalid shader program");
        return nullptr;
    }
    // build the map tables..
    ProcessProgram(ret->plat_.program, plat, GL_REFERENCED_BY_COMPUTE_SHADER, map);
    ret->pushConstants = map.pushConstants;
    ret->plat_.pushConstants = ret->pushConstants;
    ret->plat_.flipLocation = glGetUniformLocation(ret->plat_.program, "CORE_FLIP_NDC");

    const auto& pipelineLayout = reflection_.pipelineLayout;
    BuildBindInfos(ret->resources_, pipelineLayout, map, plat.combSets);
    ret->plat_.resourcesView.resourceList = ret->resources_.resourceList;
    ret->plat_.resourcesView.descriptorIndexIds = ret->resources_.descriptorIndexIds;
    ret->plat_.resourcesView.ids = ret->resources_.ids;
    return ret;
}
RENDER_END_NAMESPACE()
