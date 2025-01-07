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

#include "render_node_graph_loader.h"

#include <charconv>

#include <base/containers/fixed_string.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/render_data_structures.h>

#include "json_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
RENDER_JSON_SERIALIZE_ENUM(GpuQueue::QueueType,
    { { GpuQueue::QueueType::UNDEFINED, nullptr }, { GpuQueue::QueueType::GRAPHICS, "graphics" },
        { GpuQueue::QueueType::COMPUTE, "compute" }, { GpuQueue::QueueType::TRANSFER, "transfer" } })

namespace {
constexpr size_t VERSION_SIZE { 5u };
constexpr uint32_t VERSION_MAJOR { 22u };
constexpr size_t MAX_OUTPUT_RESOURCE_COUNT { 32u };
constexpr size_t MAX_RNG_RENDER_NODE_COUNT { 256u };

void ParseQueueWaitSignals(
    const json::value& node, RenderNodeDesc& data, [[maybe_unused]] IRenderNodeGraphLoader::LoadResult& nodeResult)
{
    if (auto const queueSignals = node.find("gpuQueueWaitSignals"); queueSignals) {
        if (auto const typeNames = queueSignals->find("typeNames"); typeNames) {
            if (typeNames->is_array()) {
                FromJson(*typeNames, data.description.gpuQueueWaitForSignals.typeNames);
            }
        }
        if (auto const nodeNames = queueSignals->find("nodeNames"); nodeNames) {
            if (nodeNames->is_array()) {
                FromJson(*nodeNames, data.description.gpuQueueWaitForSignals.nodeNames);
            }
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if (data.description.gpuQueueWaitForSignals.nodeNames.size() >
            PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS) {
            nodeResult.error += "gpuQueueWaitSignal count must be smaller than" +
                                to_string(PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS) + ")\n";
        }
        if (data.description.gpuQueueWaitForSignals.typeNames.size() >
            PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS) {
            nodeResult.error += "gpuQueueWaitSignal count must be smaller than" +
                                to_string(PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS) + ")\n";
        }
#endif
    }
}

IRenderNodeGraphLoader::LoadResult ParseRenderNode(const json::value& node, RenderNodeDesc& data)
{
    IRenderNodeGraphLoader::LoadResult nodeResult;

    SafeGetJsonValue(node, "typeName", nodeResult.error, data.typeName);
    SafeGetJsonValue(node, "nodeName", nodeResult.error, data.nodeName);
    // render node specific json
    data.nodeJson = json::to_string(node);
    SafeGetJsonValue(node, "nodeDataStoreName", nodeResult.error, data.description.nodeDataStoreName);

    if (auto const queue = node.find("queue"); queue) {
        SafeGetJsonEnum(*queue, "type", nodeResult.error, data.description.queue.type);
        SafeGetJsonValue(*queue, "index", nodeResult.error, data.description.queue.index);
    }

    if (auto const cpuDependencies = node.find("cpuDependencies"); cpuDependencies) {
        if (auto const typeNames = cpuDependencies->find("typeNames"); typeNames) {
            if (typeNames->is_array()) {
                FromJson(*typeNames, data.description.cpuDependencies.typeNames);
            }
        }
        if (auto const nodeNames = cpuDependencies->find("nodeNames"); nodeNames) {
            if (nodeNames->is_array()) {
                FromJson(*nodeNames, data.description.cpuDependencies.nodeNames);
            }
        }
    }
    ParseQueueWaitSignals(node, data, nodeResult);

    return nodeResult;
}

IRenderNodeGraphLoader::LoadResult ParseOutputResources(const json::value& node, RenderNodeGraphOutputResource& data)
{
    IRenderNodeGraphLoader::LoadResult nodeResult;

    SafeGetJsonValue(node, "nodeName", nodeResult.error, data.nodeName);
    SafeGetJsonValue(node, "name", nodeResult.error, data.name);

    return nodeResult;
}

void CompatibilityCheck(const json::value& json, RenderNodeGraphLoader::LoadResult& result)
{
    string ver;
    string type;
    uint32_t verMajor { ~0u };
    uint32_t verMinor { ~0u };
    if (const json::value* iter = json.find("compatibility_info"); iter) {
        SafeGetJsonValue(*iter, "version", result.error, ver);
        SafeGetJsonValue(*iter, "type", result.error, type);
        if (ver.size() == VERSION_SIZE) {
            if (const auto delim = ver.find('.'); delim != string::npos) {
                std::from_chars(ver.data(), ver.data() + delim, verMajor);
                std::from_chars(ver.data() + delim + 1, ver.data() + ver.size(), verMinor);
            }
        }
    }
    if ((type != "rendernodegraph") || (verMajor != VERSION_MAJOR)) {
        result.error += "invalid render node graph type (" + type + ") and/or version (" + ver + ").";
        result.success = false;
    }
}

vector<RenderNodeDesc> GetRenderNodeDescs(const json::value& json, string& error)
{
    vector<RenderNodeDesc> nodeDescriptors;
    if (const auto nodes = json.find("nodes"); nodes) {
        if (nodes->is_array()) {
            nodeDescriptors.reserve(nodes->array_.size());
            for (auto const& node : nodes->array_) {
                RenderNodeDesc data;
                RenderNodeGraphLoader::LoadResult nodeResult = ParseRenderNode(node, data);
                if (nodeResult.error.empty()) {
                    nodeDescriptors.push_back(move(data));
                } else {
                    error += nodeResult.error;
                }
            }
        } else {
            error += "\"nodes\" must to be an array.";
        }
    }
    if (nodeDescriptors.size() > MAX_RNG_RENDER_NODE_COUNT) {
        nodeDescriptors.resize(MAX_RNG_RENDER_NODE_COUNT);
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("Render node graph max render node count exceeded");
#endif
    }
    return nodeDescriptors;
}

vector<RenderNodeGraphOutputResource> GetRenderNodeOutputResources(const json::value& json, string& error)
{
    vector<RenderNodeGraphOutputResource> outputResources;
    if (const auto nodes = json.find("renderNodeGraphOutputResources"); nodes) {
        outputResources.reserve(nodes->array_.size());
        for (auto const& node : nodes->array_) {
            RenderNodeGraphOutputResource data;
            RenderNodeGraphLoader::LoadResult nodeResult = ParseOutputResources(node, data);
            if (nodeResult.error.empty()) {
                outputResources.push_back(move(data));
            } else {
                error += nodeResult.error;
            }
        }
    }
    if (outputResources.size() > MAX_OUTPUT_RESOURCE_COUNT) {
        outputResources.resize(MAX_OUTPUT_RESOURCE_COUNT);
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("Render node graph max output resource count exceeded");
#endif
    }
    return outputResources;
}

IRenderNodeGraphLoader::LoadResult LoadFromNullTerminated(const string_view uri, const string_view jsonString)
{
    if (const auto json = json::parse(jsonString.data()); json) {
        IRenderNodeGraphLoader::LoadResult finalResult;
        CompatibilityCheck(json, finalResult);
        if (!finalResult.success) {
            return finalResult; // compatibility check failed
        }

        string renderNodeGraphName;
        string renderNodeGraphDataStoreName;
        SafeGetJsonValue(json, "renderNodeGraphName", finalResult.error, renderNodeGraphName);
        SafeGetJsonValue(json, "renderNodeGraphDataStoreName", finalResult.error, renderNodeGraphDataStoreName);

        vector<RenderNodeDesc> nodeDescriptors = GetRenderNodeDescs(json, finalResult.error);
        vector<RenderNodeGraphOutputResource> outputResources = GetRenderNodeOutputResources(json, finalResult.error);

        finalResult.success = finalResult.error.empty();
        if (finalResult.error.empty()) {
            finalResult.desc.renderNodeGraphName = renderNodeGraphName;
            finalResult.desc.renderNodeGraphDataStoreName = renderNodeGraphDataStoreName;
            finalResult.desc.renderNodeGraphUri = uri;
            finalResult.desc.nodes = move(nodeDescriptors);
            finalResult.desc.outputResources = move(outputResources);
        }

        return finalResult;
    } else {
        return IRenderNodeGraphLoader::LoadResult("Invalid render node graph json file.");
    }
}
} // namespace

RenderNodeGraphLoader::RenderNodeGraphLoader(IFileManager& fileManager) : fileManager_(fileManager) {}

RenderNodeGraphLoader::LoadResult RenderNodeGraphLoader::Load(const string_view uri)
{
    IFile::Ptr file = fileManager_.OpenFile(uri);
    if (!file) {
        PLUGIN_LOG_D("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to open file.");
    }

    const uint64_t byteLength = file->GetLength();

    string raw(static_cast<size_t>(byteLength), string::value_type());
    if (file->Read(raw.data(), byteLength) != byteLength) {
        PLUGIN_LOG_D("Error loading '%s'", string(uri).c_str());
        return LoadResult("Failed to read file.");
    }

    return LoadFromNullTerminated(uri, raw);
}

RenderNodeGraphLoader::LoadResult RenderNodeGraphLoader::LoadString(const string_view jsonString)
{
    // make sure the input is zero terminated before parsing.
    const auto asString = string(jsonString);
    return LoadFromNullTerminated("", asString);
}
RENDER_END_NAMESPACE()
