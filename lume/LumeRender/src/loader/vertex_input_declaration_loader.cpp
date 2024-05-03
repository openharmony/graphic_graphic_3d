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

#include "vertex_input_declaration_loader.h"

#include <algorithm>

#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <render/device/pipeline_state_desc.h>

#include "json_format_serialization.h"
#include "json_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
// clang-format off
CORE_JSON_SERIALIZE_ENUM(VertexInputRate,
    {
        { CORE_VERTEX_INPUT_RATE_VERTEX, "vertex" },
        { CORE_VERTEX_INPUT_RATE_INSTANCE, "instance" },
    })
// clang-format on
void FromJson(const json::value& jsonData, JsonContext<VertexInputDeclaration::VertexInputBindingDescription>& context)
{
    SafeGetJsonValue(jsonData, "binding", context.error, context.data.binding);
    SafeGetJsonValue(jsonData, "stride", context.error, context.data.stride);
    SafeGetJsonEnum(jsonData, "vertexInputRate", context.error, context.data.vertexInputRate);
}

void FromJson(
    const json::value& jsonData, JsonContext<VertexInputDeclaration::VertexInputAttributeDescription>& context)
{
    SafeGetJsonValue(jsonData, "location", context.error, context.data.location);
    SafeGetJsonValue(jsonData, "binding", context.error, context.data.binding);
    SafeGetJsonEnum(jsonData, "format", context.error, context.data.format);
    SafeGetJsonValue(jsonData, "offset", context.error, context.data.offset);
}

namespace {
VertexInputDeclarationLoader::LoadResult LoadState(
    const json::value& jsonData, const string_view uri, VertexInputDeclarationData& vertexInputDeclarationData_)
{
    VertexInputDeclarationLoader::LoadResult result;

    vector<VertexInputDeclaration::VertexInputBindingDescription> bindings;
    vector<VertexInputDeclaration::VertexInputAttributeDescription> attributes;

    ParseArray<decltype(bindings)::value_type>(jsonData, "vertexInputBindingDescriptions", bindings, result);
    ParseArray<decltype(attributes)::value_type>(jsonData, "vertexInputAttributeDescriptions", attributes, result);

    if (result.success) {
        PLUGIN_ASSERT(bindings.size() <= PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
        PLUGIN_ASSERT(attributes.size() <= PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);

        vertexInputDeclarationData_.bindingDescriptionCount =
            std::min((uint32_t)bindings.size(), PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
        vertexInputDeclarationData_.attributeDescriptionCount =
            std::min((uint32_t)attributes.size(), PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);

        for (uint32_t idx = 0; idx < vertexInputDeclarationData_.bindingDescriptionCount; ++idx) {
            vertexInputDeclarationData_.bindingDescriptions[idx] = bindings[idx];
        }
        for (uint32_t idx = 0; idx < vertexInputDeclarationData_.attributeDescriptionCount; ++idx) {
            vertexInputDeclarationData_.attributeDescriptions[idx] = attributes[idx];
            if (vertexInputDeclarationData_.attributeDescriptions[idx].format == Format::BASE_FORMAT_UNDEFINED) {
                result.success = false;
                result.error += "undefined format for vertex input attribute\n";
                PLUGIN_LOG_E("undefined format in vertex input attribute (%u), in %s", idx, uri.data());
            }
        }
    }

    return result;
}

VertexInputDeclarationLoader::LoadResult Load(
    const json::value& jsonData, const string_view uri, VertexInputDeclarationData& vertexInputDeclarationData_)
{
    VertexInputDeclarationLoader::LoadResult result;

#if (RENDER_VALIDATION_ENABLED == 1)
    {
        string name;
        SafeGetJsonValue(jsonData, "name", result.error, name);
        if (!name.empty()) {
            PLUGIN_LOG_W("RENDER_VALIDATION: name not supported in vertex input declaration json");
        }
    }
#endif
    if (const json::value* stateIter = jsonData.find("vertexInputState"); stateIter) {
        result = LoadState(*stateIter, uri, vertexInputDeclarationData_);
    } else {
        result.error += "vertex input state not found\n";
        result.success = false;
    }

    return result;
}
} // namespace

string_view VertexInputDeclarationLoader::GetUri() const
{
    return uri_;
}

VertexInputDeclarationView VertexInputDeclarationLoader::GetVertexInputDeclarationView() const
{
    return {
        array_view<const VertexInputDeclaration::VertexInputBindingDescription>(
            vertexInputDeclarationData_.bindingDescriptions, vertexInputDeclarationData_.bindingDescriptionCount),
        array_view<const VertexInputDeclaration::VertexInputAttributeDescription>(
            vertexInputDeclarationData_.attributeDescriptions, vertexInputDeclarationData_.attributeDescriptionCount),
    };
}

VertexInputDeclarationLoader::LoadResult VertexInputDeclarationLoader::Load(const string_view jsonString)
{
    VertexInputDeclarationLoader::LoadResult result;
    const auto json = json::parse(jsonString.data());
    if (json) {
        result = RENDER_NS::Load(json, uri_, vertexInputDeclarationData_);
    } else {
        result.success = false;
        result.error = "Invalid json file.";
    }

    return result;
}

VertexInputDeclarationLoader::LoadResult VertexInputDeclarationLoader::Load(
    IFileManager& fileManager, const string_view uri)
{
    uri_ = uri;
    LoadResult result;

    IFile::Ptr file = fileManager.OpenFile(uri);
    if (!file) {
        PLUGIN_LOG_E("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to open file.");
    }

    const uint64_t byteLength = file->GetLength();

    string raw;
    raw.resize(static_cast<size_t>(byteLength));

    if (file->Read(raw.data(), byteLength) != byteLength) {
        PLUGIN_LOG_E("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to read file.");
    }

    return Load(string_view(raw));
}

RENDER_END_NAMESPACE()
