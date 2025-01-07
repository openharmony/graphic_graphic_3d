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

#include "shader_data_loader.h"

#include <charconv>

#include <base/containers/string.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>

#include "json_util.h"
#include "shader_state_loader_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr size_t VERSION_SIZE { 5u };
constexpr uint32_t VERSION_MAJOR { 22u };

void LoadState(const json::value& jsonData, GraphicsState& graphicsState, GraphicsStateFlags& stateFlags,
    ShaderDataLoader::LoadResult& result)
{
    {
        ShaderStateLoaderUtil::ShaderStateResult ssr;
        ShaderStateLoaderUtil::ParseSingleState(jsonData, ssr);
        ShaderStateLoaderUtil::ParseStateFlags(jsonData, stateFlags, ssr);
        if (ssr.res.success && (ssr.states.states.size() == 1u)) {
            graphicsState = move(ssr.states.states[0u]);
        } else {
            result.error += ssr.res.error;
        }
    }
}

void LoadSingleShaderVariant(const json::value& jsonData, const string& materialMetaData,
    IShaderManager::ShaderVariant& data, ShaderDataLoader::LoadResult& result)
{
    SafeGetJsonValue(jsonData, "renderSlotDefaultShader", result.error, data.renderSlotDefaultShader);
    SafeGetJsonValue(jsonData, "variantName", result.error, data.variantName);
    SafeGetJsonValue(jsonData, "displayName", result.error, data.displayName);
    SafeGetJsonValue(jsonData, "baseShader", result.error, data.addBaseShader);

    string vertexShader { "" };
    string fragmentShader { "" };
    string computeShader { "" };

    SafeGetJsonValue(jsonData, "vert", result.error, vertexShader);
    SafeGetJsonValue(jsonData, "frag", result.error, fragmentShader);
    SafeGetJsonValue(jsonData, "compute", result.error, computeShader);
    if (computeShader.empty()) {
        SafeGetJsonValue(jsonData, "comp", result.error, computeShader);
    }

    if (!vertexShader.empty()) {
        data.shaders.push_back({ vertexShader, CORE_SHADER_STAGE_VERTEX_BIT });
    }
    if (!fragmentShader.empty()) {
        data.shaders.push_back({ fragmentShader, CORE_SHADER_STAGE_FRAGMENT_BIT });
    }
    if (!computeShader.empty()) {
        data.shaders.push_back({ computeShader, CORE_SHADER_STAGE_COMPUTE_BIT });
    }

    {
        SafeGetJsonValue(jsonData, "slot", result.error, data.renderSlot);
        SafeGetJsonValue(jsonData, "renderSlot", result.error, data.renderSlot);
    }
    SafeGetJsonValue(jsonData, "vertexInputDeclaration", result.error, data.vertexInputDeclaration);
    SafeGetJsonValue(jsonData, "pipelineLayout", result.error, data.pipelineLayout);

    if (result.success) {
        data.shaderFileStr = json::to_string(jsonData);
        if (const json::value* iter = jsonData.find("state"); iter) {
            LoadState(*iter, data.graphicsState, data.stateFlags, result);
        }
        if (const json::value* iter = jsonData.find("materialMetadata"); iter) {
            data.materialMetadata = json::to_string(*iter);
        } else if (!materialMetaData.empty()) {
            data.materialMetadata = materialMetaData;
        }
    }
}

ShaderDataLoader::LoadResult LoadFunc(const string_view uri, const json::value& jsonData, string& baseCategory,
    vector<IShaderManager::ShaderVariant>& shaderVariants)
{
    ShaderDataLoader::LoadResult result;
    // compatibility check with early out
    {
        string ver;
        string type;
        uint32_t verMajor { ~0u };
        uint32_t verMinor { ~0u };
        if (const json::value* iter = jsonData.find("compatibility_info"); iter) {
            SafeGetJsonValue(*iter, "type", result.error, type);
            SafeGetJsonValue(*iter, "version", result.error, ver);
            if (ver.size() == VERSION_SIZE) {
                if (const auto delim = ver.find('.'); delim != string::npos) {
                    std::from_chars(ver.data(), ver.data() + delim, verMajor);
                    std::from_chars(ver.data() + delim + 1, ver.data() + ver.size(), verMinor);
                }
            }
        }
        if ((type != "shader") || (verMajor != VERSION_MAJOR)) {
            result.error += "invalid shader type (" + type + ") and/or version (" + ver + ").";
            result.success = false;
            return result;
        }
    }

#if (RENDER_VALIDATION_ENABLED == 1)
    {
        string name;
        SafeGetJsonValue(jsonData, "name", result.error, name);
        if (!name.empty()) {
            PLUGIN_LOG_W("RENDER_VALIDATION: name (%s) not supported in shader json", name.c_str());
        }
    }
    // base shader
    {
        string baseShader;
        SafeGetJsonValue(jsonData, "baseShader", result.error, baseShader);
        if (!(baseShader.empty())) {
            PLUGIN_LOG_W("RENDER_VALIDATION: baseShader supported only for variants (%s)", baseShader.c_str());
        }
    }
#endif
    // category
    SafeGetJsonValue(jsonData, "category", result.error, baseCategory);
    // base materialMetaData
    string materialMetaData;
    if (const json::value* iter = jsonData.find("materialMetadata"); iter) {
        materialMetaData = json::to_string(*iter);
    }

    // check all variants or use (older) single variant style
    if (const json::value* iter = jsonData.find("shaders"); iter) {
        if (iter->is_array()) {
            for (const auto& variantRef : iter->array_) {
                IShaderManager::ShaderVariant sv;
                // add own base shader
                sv.ownBaseShader = uri;
                LoadSingleShaderVariant(variantRef, materialMetaData, sv, result);
                if (result.error.empty()) {
                    shaderVariants.push_back(move(sv));
                }
            }
        }
    } else {
        IShaderManager::ShaderVariant sv;
        LoadSingleShaderVariant(jsonData, materialMetaData, sv, result);
        if (result.error.empty()) {
            shaderVariants.push_back(move(sv));
        }
    }

    result.success = result.error.empty();
    return result;
}
} // namespace

string_view ShaderDataLoader::GetUri() const
{
    return uri_;
}

BASE_NS::string_view ShaderDataLoader::GetBaseCategory() const
{
    return baseCategory_;
}

array_view<const IShaderManager::ShaderVariant> ShaderDataLoader::GetShaderVariants() const
{
    return shaderVariants_;
}

ShaderDataLoader::LoadResult ShaderDataLoader::Load(IFileManager& fileManager, const string_view uri)
{
    uri_ = uri;
    IFile::Ptr file = fileManager.OpenFile(uri);
    if (!file) {
        PLUGIN_LOG_D("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to open file.");
    }

    const uint64_t byteLength = file->GetLength();

    string raw;
    raw.resize(static_cast<size_t>(byteLength));

    if (file->Read(raw.data(), byteLength) != byteLength) {
        PLUGIN_LOG_D("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to read file.");
    }

    return Load(uri, move(raw));
}

ShaderDataLoader::LoadResult ShaderDataLoader::Load(const string_view uri, string&& jsonData)
{
    LoadResult result;
    const auto json = json::parse(jsonData.data());
    if (json) {
        result = RENDER_NS::LoadFunc(uri, json, baseCategory_, shaderVariants_);
    } else {
        result.success = false;
        result.error = "Invalid json file.";
    }

    return result;
}
RENDER_END_NAMESPACE()
