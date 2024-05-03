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
#include <base/util/uid_util.h>
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

void LoadSingleShaderVariant(
    const json::value& jsonData, ShaderDataLoader::ShaderVariant& data, ShaderDataLoader::LoadResult& result)
{
    SafeGetJsonValue(jsonData, "renderSlotDefaultShader", result.error, data.renderSlotDefaultShader);
    SafeGetJsonValue(jsonData, "variantName", result.error, data.variantName);
    SafeGetJsonValue(jsonData, "displayName", result.error, data.displayName);
    SafeGetJsonValue(jsonData, "vert", result.error, data.vertexShader);
    SafeGetJsonValue(jsonData, "frag", result.error, data.fragmentShader);
    SafeGetJsonValue(jsonData, "compute", result.error, data.computeShader);
    if (data.computeShader.empty()) {
        SafeGetJsonValue(jsonData, "comp", result.error, data.computeShader);
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
        }
    }
}

ShaderDataLoader::LoadResult LoadFunc(const json::value& jsonData, string& baseShader, string& baseCategory,
    vector<ShaderDataLoader::ShaderVariant>& shaderVariants)
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
#endif
    // base shader
    SafeGetJsonValue(jsonData, "baseShader", result.error, baseShader);
    // category
    SafeGetJsonValue(jsonData, "category", result.error, baseCategory);

    // check all variants or use (older) single variant style
    if (const json::value* iter = jsonData.find("shaders"); iter) {
        if (iter->is_array()) {
            for (const auto& variantRef : iter->array_) {
                ShaderDataLoader::ShaderVariant sv;
                LoadSingleShaderVariant(variantRef, sv, result);
                if (result.error.empty()) {
                    shaderVariants.push_back(move(sv));
                }
            }
        }
    } else {
        ShaderDataLoader::ShaderVariant sv;
        LoadSingleShaderVariant(jsonData, sv, result);
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

string_view ShaderDataLoader::GetBaseShader() const
{
    return baseShader_;
}

BASE_NS::string_view ShaderDataLoader::GetBaseCategory() const
{
    return baseCategory_;
}

array_view<const ShaderDataLoader::ShaderVariant> ShaderDataLoader::GetShaderVariants() const
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

    return Load(move(raw));
}

ShaderDataLoader::LoadResult ShaderDataLoader::Load(string&& jsonData)
{
    LoadResult result;
    const auto json = json::parse(jsonData.data());
    if (json) {
        result = RENDER_NS::LoadFunc(json, baseShader_, baseCategory_, shaderVariants_);
    } else {
        result.success = false;
        result.error = "Invalid json file.";
    }

    return result;
}
RENDER_END_NAMESPACE()
