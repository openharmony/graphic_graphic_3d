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

#include "render_data_store_post_process.h"

#include <cinttypes>
#include <cstdint>

#include <base/containers/fixed_string.h>
#include <base/math/matrix.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>

#include "datastore/render_data_store_manager.h"
#include "datastore/render_data_store_pod.h"
#include "loader/json_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr string_view RENDER_DATA_STORE_POST_PROCESS_TYPE_NAME { RenderDataStorePostProcess::TYPE_NAME };
constexpr string_view RENDER_DATA_STORE_POD_NAME { RenderDataStorePod::TYPE_NAME };

constexpr string_view CUSTOM_PROPERTIES = "customProperties";
constexpr string_view GLOBAL_FACTOR = "globalFactor";
constexpr string_view CUSTOM_PROPERTY_DATA = "data";

constexpr uint32_t SIZE_OF_FLOAT { sizeof(float) };
constexpr uint32_t SIZE_OF_VEC2 { sizeof(Math::Vec2) };
constexpr uint32_t SIZE_OF_VEC3 { sizeof(Math::Vec3) };
constexpr uint32_t SIZE_OF_VEC4 { sizeof(Math::Vec4) };
constexpr uint32_t SIZE_OF_MAT3X3 { sizeof(Math::Mat3X3) };
constexpr uint32_t SIZE_OF_MAT4X4 { sizeof(Math::Mat4X4) };

constexpr uint32_t ALIGNMENT_OF_FLOAT { sizeof(float) };
constexpr uint32_t ALIGNMENT_OF_VEC2 { sizeof(float) * 2U };
constexpr uint32_t ALIGNMENT_OF_VEC3 { sizeof(float) * 4U };
constexpr uint32_t ALIGNMENT_OF_VEC4 { sizeof(float) * 4U };
constexpr uint32_t ALIGNMENT_OF_MAT3X3 { sizeof(float) * 4U * 3U };
constexpr uint32_t ALIGNMENT_OF_MAT4X4 { sizeof(float) * 4U * 4U };

constexpr uint32_t MAX_LOCAL_DATA_BYTE_SIZE { PostProcessConstants::USER_LOCAL_FACTOR_BYTE_SIZE };
constexpr uint32_t MAX_GLOBAL_FACTOR_BYTE_SIZE { sizeof(Math::Vec4) };

constexpr string_view TAA_SHADER_NAME { "rendershaders://shader/fullscreen_taa.shader" };
constexpr string_view FXAA_SHADER_NAME { "rendershaders://shader/fullscreen_fxaa.shader" };
constexpr string_view DOF_SHADER_NAME { "rendershaders://shader/depth_of_field.shader" };
constexpr string_view MB_SHADER_NAME { "rendershaders://shader/fullscreen_motion_blur.shader" };

inline constexpr uint32_t GetAlignment(uint32_t value, uint32_t align)
{
    if (align == 0) {
        return value;
    } else {
        return ((value + align - 1U) / align) * align;
    }
}

void AppendValues(
    const uint32_t maxByteSize, const string_view type, const json::value* value, uint32_t& offset, uint8_t* data)
{
    if (type == "vec4") {
        if ((offset + ALIGNMENT_OF_VEC4) <= maxByteSize) {
            Math::Vec4* val = reinterpret_cast<Math::Vec4*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC4, ALIGNMENT_OF_VEC4);
        }
    } else if (type == "uvec4") {
        if ((offset + ALIGNMENT_OF_VEC4) <= maxByteSize) {
            Math::UVec4* val = reinterpret_cast<Math::UVec4*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC4, ALIGNMENT_OF_VEC4);
        }
    } else if (type == "ivec4") {
        if ((offset + ALIGNMENT_OF_VEC4) <= maxByteSize) {
            Math::UVec4* val = reinterpret_cast<Math::UVec4*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC4, ALIGNMENT_OF_VEC4);
        }
    } else if (type == "vec3") {
        if ((offset + ALIGNMENT_OF_VEC3) <= maxByteSize) {
            Math::Vec3* val = reinterpret_cast<Math::Vec3*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC3, ALIGNMENT_OF_VEC3);
        }
    } else if (type == "uvec3") {
        if ((offset + ALIGNMENT_OF_VEC3) <= maxByteSize) {
            Math::UVec3* val = reinterpret_cast<Math::UVec3*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC3, ALIGNMENT_OF_VEC3);
        }
    } else if (type == "ivec3") {
        if ((offset + ALIGNMENT_OF_VEC3) <= maxByteSize) {
            Math::UVec3* val = reinterpret_cast<Math::UVec3*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC3, ALIGNMENT_OF_VEC3);
        }
    } else if (type == "vec2") {
        if ((offset + ALIGNMENT_OF_VEC2) <= maxByteSize) {
            Math::Vec2* val = reinterpret_cast<Math::Vec2*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC2, ALIGNMENT_OF_VEC2);
        }
    } else if (type == "uvec2") {
        if ((offset + ALIGNMENT_OF_VEC2) <= maxByteSize) {
            Math::UVec2* val = reinterpret_cast<Math::UVec2*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC2, ALIGNMENT_OF_VEC2);
        }
    } else if (type == "ivec2") {
        if ((offset + ALIGNMENT_OF_VEC2) <= maxByteSize) {
            Math::UVec2* val = reinterpret_cast<Math::UVec2*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_VEC2, ALIGNMENT_OF_VEC2);
        }
    } else if (type == "float") {
        if ((offset + ALIGNMENT_OF_FLOAT) <= maxByteSize) {
            float* val = reinterpret_cast<float*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_FLOAT, ALIGNMENT_OF_FLOAT);
        }
    } else if (type == "uint") {
        if ((offset + ALIGNMENT_OF_FLOAT) <= maxByteSize) {
            uint32_t* val = reinterpret_cast<uint32_t*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_FLOAT, ALIGNMENT_OF_FLOAT);
        }
    } else if (type == "int") {
        if ((offset + ALIGNMENT_OF_FLOAT) <= maxByteSize) {
            int32_t* val = reinterpret_cast<int32_t*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_FLOAT, ALIGNMENT_OF_FLOAT);
        }
    } else if (type == "mat3x3") {
        if ((offset + ALIGNMENT_OF_MAT3X3) <= maxByteSize) {
            Math::Mat3X3* val = reinterpret_cast<Math::Mat3X3*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_MAT3X3, ALIGNMENT_OF_MAT3X3);
        }
    } else if (type == "mat4x4") {
        if ((offset + ALIGNMENT_OF_MAT4X4) <= maxByteSize) {
            Math::Mat4X4* val = reinterpret_cast<Math::Mat4X4*>(data + offset);
            FromJson(*value, *val);
            offset = GetAlignment(offset + SIZE_OF_MAT4X4, ALIGNMENT_OF_MAT4X4);
        }
    } else {
        PLUGIN_LOG_W("RENDER_VALIDATION: Invalid property type only int, uint, float, and XvecX variants, and mat3x3 "
                     "and mat4x4 supported");
    }
    // NOTE: does not handle invalid types
}
} // namespace

RenderDataStorePostProcess::RenderDataStorePostProcess(const IRenderContext& renderContext, const string_view name)
    : renderContext_(renderContext), name_(name)
{}

RenderDataStorePostProcess::~RenderDataStorePostProcess() {}

void RenderDataStorePostProcess::Create(const string_view name)
{
    const auto lock = std::lock_guard(mutex_);

    if (!allPostProcesses_.contains(name)) {
        CreateFromPod(name);
    } else {
        PLUGIN_LOG_I("Render data store post process with name (%s) already created.", name.data());
    }
}

void RenderDataStorePostProcess::Create(
    const string_view name, const string_view ppName, const RenderHandleReference& shader)
{
    const auto lock = std::lock_guard(mutex_);

    if (!allPostProcesses_.contains(name)) {
        CreateFromPod(name);
    }

    if (auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.end()) {
        auto& postProcesses = iter->second.postProcesses;
        const uint32_t ppCount = static_cast<uint32_t>(postProcesses.size());
        uint32_t ppIndex = ~0u;
        for (uint32_t idx = 0; idx < ppCount; ++idx) {
            if (postProcesses[idx].name == ppName) {
                ppIndex = idx;
                break;
            }
        }

        if (ppIndex < ppCount) {
            // does not set variables
            GetShaderProperties(shader, postProcesses[ppIndex].variables);
            SetGlobalFactorsImpl(postProcesses[ppIndex].variables, ppIndex, iter->second);
        } else {
            PostProcess pp;
            pp.id = static_cast<uint32_t>(postProcesses.size());
            pp.name = ppName;
            pp.shader = shader;
            pp.variables = {}; // default variables, overwritten with shader default properties
            GetShaderProperties(shader, pp.variables);
            postProcesses.push_back(move(pp));
            SetGlobalFactorsImpl(postProcesses[pp.id].variables, pp.id, iter->second);
        }
    } else {
        PLUGIN_LOG_E("Post process creation error (name: %s, ppName: %s)", name.data(), ppName.data());
    }
}

void RenderDataStorePostProcess::Destroy(const string_view name)
{
    const auto lock = std::lock_guard(mutex_);

    if ((!name.empty()) && allPostProcesses_.contains(name)) {
        allPostProcesses_.erase(name);
        if (IRenderDataStorePod* dataStorePod = static_cast<IRenderDataStorePod*>(
                renderContext_.GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_POD_NAME));
            dataStorePod) {
            dataStorePod->DestroyPod(RENDER_DATA_STORE_POST_PROCESS_TYPE_NAME, name);
        }
    } else {
        PLUGIN_LOG_I("Post process not found: %s", name.data());
    }
}

void RenderDataStorePostProcess::Destroy(const string_view name, const string_view ppName)
{
    const auto lock = std::lock_guard(mutex_);

    if (auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.end()) {
        for (auto ppIter = iter->second.postProcesses.begin(); ppIter != iter->second.postProcesses.end(); ++ppIter) {
            if (ppIter->name == ppName) {
                iter->second.postProcesses.erase(ppIter);
                break;
            }
        }
    }
}

bool RenderDataStorePostProcess::Contains(const string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    return allPostProcesses_.contains(name);
}

bool RenderDataStorePostProcess::Contains(const string_view name, const string_view ppName) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.cend()) {
        for (const auto& ref : iter->second.postProcesses) {
            if (ref.name == ppName) {
                return true;
            }
        }
    }
    return false;
}

void RenderDataStorePostProcess::Set(
    const string_view name, const string_view ppName, const PostProcess::Variables& vars)
{
    const auto lock = std::lock_guard(mutex_);

    if (auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.end()) {
        auto& postProcesses = iter->second.postProcesses;
        const uint32_t ppCount = static_cast<uint32_t>(postProcesses.size());
        for (uint32_t idx = 0; idx < ppCount; ++idx) {
            if (postProcesses[idx].name == ppName) {
                SetImpl(vars, idx, iter->second);
                break;
            }
        }
    }
}

void RenderDataStorePostProcess::Set(const string_view name, const array_view<PostProcess::Variables> vars)
{
    const auto lock = std::lock_guard(mutex_);

    if (auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.end()) {
        const auto& postProcesses = iter->second.postProcesses;
        const uint32_t ppCount =
            Math::min(static_cast<uint32_t>(vars.size()), static_cast<uint32_t>(postProcesses.size()));
        for (uint32_t idx = 0; idx < ppCount; ++idx) {
            SetImpl(vars[idx], idx, iter->second);
        }
    }
}

RenderDataStorePostProcess::GlobalFactors RenderDataStorePostProcess::GetGlobalFactors(const string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.end()) {
        return iter->second.globalFactors;
    } else {
        return {};
    }
}

vector<RenderDataStorePostProcess::PostProcess> RenderDataStorePostProcess::Get(const string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.cend()) {
        return iter->second.postProcesses;
    }
    return {};
}

RenderDataStorePostProcess::PostProcess RenderDataStorePostProcess::Get(
    const string_view name, const string_view ppName) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = allPostProcesses_.find(name); iter != allPostProcesses_.cend()) {
        for (const auto& ref : iter->second.postProcesses) {
            if (ppName == ref.name) {
                return ref;
            }
        }
    }
    return {};
}

void RenderDataStorePostProcess::CreateFromPod(const string_view name)
{
    // create base post process render data store
    if (IRenderDataStorePod* dataStorePod = static_cast<IRenderDataStorePod*>(
            renderContext_.GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_POD_NAME));
        dataStorePod) {
        PLUGIN_STATIC_ASSERT(
            countof(PostProcessConstants::POST_PROCESS_NAMES) == PostProcessConstants::POST_PROCESS_COUNT);
        auto& postProcessRef = allPostProcesses_[name];
        // NOTE: needs to fetch the post process "properties" from shader custom properties
        if (const auto arrView = dataStorePod->Get(name); arrView.size_bytes() == sizeof(PostProcessConfiguration)) {
            // populate with built-in data and copy
            PostProcessConfiguration ppConfig = *((const PostProcessConfiguration*)arrView.data());
            FillDefaultPostProcessData(ppConfig, postProcessRef);
        } else {
            // create new
            PostProcessConfiguration ppConfig;
            dataStorePod->CreatePod(RENDER_DATA_STORE_POST_PROCESS_TYPE_NAME, name, arrayviewU8(ppConfig));
            FillDefaultPostProcessData(ppConfig, postProcessRef);
        }
    }
}

void RenderDataStorePostProcess::SetImpl(
    const PostProcess::Variables& vars, const uint32_t ppIndex, PostProcessStack& ppStack)
{
    auto& postProcesses = ppStack.postProcesses;
    PLUGIN_ASSERT(ppIndex < static_cast<uint32_t>(postProcesses.size()));
    postProcesses[ppIndex].variables = vars;
    SetGlobalFactorsImpl(vars, ppIndex, ppStack);
}

void RenderDataStorePostProcess::SetGlobalFactorsImpl(
    const PostProcess::Variables& vars, const uint32_t ppIndex, PostProcessStack& ppStack)
{
    PLUGIN_ASSERT(ppIndex < static_cast<uint32_t>(ppStack.postProcesses.size()));
    // NOTE: does not copy vars
    auto& globalFactors = ppStack.globalFactors;
    if (ppIndex < PostProcessConstants::GLOBAL_FACTOR_COUNT) {
        globalFactors.factors[ppIndex] = vars.factor;
        globalFactors.enableFlags = (uint32_t(vars.enabled) << ppIndex);
    } else if (vars.userFactorIndex < PostProcessConstants::USER_GLOBAL_FACTOR_COUNT) {
        globalFactors.userFactors[vars.userFactorIndex] = vars.factor;
    }
}

void RenderDataStorePostProcess::FillDefaultPostProcessData(
    const PostProcessConfiguration& ppConfig, PostProcessStack& ppStack)
{
    const IShaderManager& shaderMgr = renderContext_.GetDevice().GetShaderManager();
    auto FillBuiltInData = [&](const uint32_t idx, const uint32_t factorIndex, const uint32_t userFactorIndex,
                               const Math::Vec4& factor, const string_view shaderName) {
        PostProcess pp;
        pp.id = idx;
        pp.name = PostProcessConstants::POST_PROCESS_NAMES[pp.id];
        pp.factorIndex = factorIndex;
        pp.shader = shaderName.empty() ? RenderHandleReference {} : shaderMgr.GetShaderHandle(shaderName);
        auto& vars = pp.variables;
        vars.userFactorIndex = userFactorIndex;
        vars.enabled = false;
        vars.factor = factor;
        ClearToValue(vars.customPropertyData, sizeof(vars.customPropertyData), 0x0, sizeof(vars.customPropertyData));
        return pp;
    };

    constexpr uint32_t defUserIdx { ~0u };
    ppStack.postProcesses.push_back(FillBuiltInData(PostProcessConstants::RENDER_TONEMAP,
        PostProcessConstants::RENDER_TONEMAP, defUserIdx, PostProcessConversionHelper::GetFactorTonemap(ppConfig), {}));
    ppStack.postProcesses.push_back(
        FillBuiltInData(PostProcessConstants::RENDER_VIGNETTE, PostProcessConstants::RENDER_VIGNETTE, defUserIdx,
            PostProcessConversionHelper::GetFactorVignette(ppConfig), {}));
    ppStack.postProcesses.push_back(FillBuiltInData(PostProcessConstants::RENDER_DITHER,
        PostProcessConstants::RENDER_DITHER, defUserIdx, PostProcessConversionHelper::GetFactorDither(ppConfig), {}));
    ppStack.postProcesses.push_back(
        FillBuiltInData(PostProcessConstants::RENDER_COLOR_CONVERSION, PostProcessConstants::RENDER_COLOR_CONVERSION,
            defUserIdx, PostProcessConversionHelper::GetFactorColorConversion(ppConfig), {}));
    ppStack.postProcesses.push_back(
        FillBuiltInData(PostProcessConstants::RENDER_COLOR_FRINGE_BIT, PostProcessConstants::RENDER_COLOR_FRINGE_BIT,
            defUserIdx, PostProcessConversionHelper::GetFactorFringe(ppConfig), {}));

    ppStack.postProcesses.push_back(FillBuiltInData(
        PostProcessConstants::RENDER_EMPTY_5, PostProcessConstants::RENDER_EMPTY_5, defUserIdx, {}, {}));
    ppStack.postProcesses.push_back(FillBuiltInData(
        PostProcessConstants::RENDER_EMPTY_6, PostProcessConstants::RENDER_EMPTY_6, defUserIdx, {}, {}));
    ppStack.postProcesses.push_back(FillBuiltInData(
        PostProcessConstants::RENDER_EMPTY_7, PostProcessConstants::RENDER_EMPTY_7, defUserIdx, {}, {}));

    ppStack.postProcesses.push_back(FillBuiltInData(PostProcessConstants::RENDER_BLUR,
        PostProcessConfiguration::INDEX_BLUR, defUserIdx, PostProcessConversionHelper::GetFactorBlur(ppConfig), {}));
    ppStack.postProcesses.push_back(FillBuiltInData(PostProcessConstants::RENDER_BLOOM,
        PostProcessConfiguration::INDEX_BLOOM, defUserIdx, PostProcessConversionHelper::GetFactorBloom(ppConfig), {}));
    ppStack.postProcesses.push_back(
        FillBuiltInData(PostProcessConstants::RENDER_FXAA, PostProcessConfiguration::INDEX_FXAA, defUserIdx,
            PostProcessConversionHelper::GetFactorFxaa(ppConfig), FXAA_SHADER_NAME));
    ppStack.postProcesses.push_back(
        FillBuiltInData(PostProcessConstants::RENDER_TAA, PostProcessConfiguration::INDEX_TAA, defUserIdx,
            PostProcessConversionHelper::GetFactorTaa(ppConfig), TAA_SHADER_NAME));
    {
        auto& pp = ppStack.postProcesses.emplace_back();
        pp.id = PostProcessConstants::RENDER_DOF;
        pp.name = PostProcessConstants::POST_PROCESS_NAMES[pp.id];
        pp.factorIndex = PostProcessConfiguration::INDEX_DOF;
        pp.shader = shaderMgr.GetShaderHandle(DOF_SHADER_NAME);
        auto& vars = pp.variables;
        vars.userFactorIndex = defUserIdx;
        vars.enabled = false;
        auto factors = PostProcessConversionHelper::GetFactorDof(ppConfig);
        CloneData(vars.customPropertyData, sizeof(vars.customPropertyData), &factors, sizeof(factors));
        factors = PostProcessConversionHelper::GetFactorDof2(ppConfig);
        CloneData(vars.customPropertyData + sizeof(factors), sizeof(vars.customPropertyData) - sizeof(factors),
            &factors, sizeof(factors));
    }

    ppStack.postProcesses.push_back(
        FillBuiltInData(PostProcessConstants::RENDER_MOTION_BLUR, PostProcessConstants::RENDER_MOTION_BLUR, defUserIdx,
            PostProcessConversionHelper::GetFactorMotionBlur(ppConfig), MB_SHADER_NAME));

    // fill global built-in factors
    PLUGIN_ASSERT(PostProcessConstants::POST_PROCESS_COUNT == static_cast<uint32_t>(ppStack.postProcesses.size()));
    for (size_t idx = 0; idx < ppStack.postProcesses.size(); ++idx) {
        ppStack.globalFactors.factors[idx] = ppStack.postProcesses[idx].variables.factor;
    }
}

void RenderDataStorePostProcess::GetShaderProperties(
    const RenderHandleReference& shader, IRenderDataStorePostProcess::PostProcess::Variables& vars)
{
    if (shader) {
        const IShaderManager& shaderMgr = renderContext_.GetDevice().GetShaderManager();
        if (const json::value* metaJson = shaderMgr.GetMaterialMetadata(shader); metaJson && metaJson->is_array()) {
            for (const auto& ref : metaJson->array_) {
                if (const auto globalFactor = ref.find(GLOBAL_FACTOR); globalFactor && globalFactor->is_array()) {
                    // process global factor "array", vec4
                    uint32_t offset = 0;
                    for (const auto& propRef : globalFactor->array_) {
                        if (propRef.is_object()) {
                            string_view type;
                            const json::value* value = nullptr;
                            for (const auto& dataObject : propRef.object_) {
                                if (dataObject.key == "type" && dataObject.value.is_string()) {
                                    type = dataObject.value.string_;
                                } else if (dataObject.key == "value") {
                                    value = &dataObject.value;
                                }
                            }
                            uint8_t* factorData = reinterpret_cast<uint8_t*>(&vars.factor);
                            AppendValues(MAX_GLOBAL_FACTOR_BYTE_SIZE, type, value, offset, factorData);
                        }
                    }
                } else if (const auto customProps = ref.find(CUSTOM_PROPERTIES);
                           customProps && customProps->is_array()) {
                    // process custom properties i.e. local factors
                    for (const auto& propRef : customProps->array_) {
                        uint32_t offset = 0;
                        if (const auto customData = propRef.find(CUSTOM_PROPERTY_DATA);
                            customData && customData->is_array()) {
                            if (offset >= MAX_LOCAL_DATA_BYTE_SIZE) {
                                break;
                            }

                            for (const auto& dataValue : customData->array_) {
                                if (dataValue.is_object()) {
                                    string_view type;
                                    const json::value* value = nullptr;
                                    for (const auto& dataObject : dataValue.object_) {
                                        if (dataObject.key == "type" && dataObject.value.is_string()) {
                                            type = dataObject.value.string_;
                                        } else if (dataObject.key == "value") {
                                            value = &dataObject.value;
                                        }
                                    }
                                    AppendValues(
                                        MAX_LOCAL_DATA_BYTE_SIZE, type, value, offset, vars.customPropertyData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// for plugin / factory interface
IRenderDataStore* RenderDataStorePostProcess::Create(IRenderContext& renderContext, char const* name)
{
    // engine not used
    return new RenderDataStorePostProcess(renderContext, name);
}

void RenderDataStorePostProcess::Destroy(IRenderDataStore* aInstance)
{
    delete static_cast<RenderDataStorePostProcess*>(aInstance);
}
RENDER_END_NAMESPACE()
