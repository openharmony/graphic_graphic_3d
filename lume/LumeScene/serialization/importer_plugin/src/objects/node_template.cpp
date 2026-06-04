/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "node_template.h"

#include <scene/interface/resource/intf_exported_node.h>

#include <core/intf_engine.h>

#include "../import_helpers.h"
#include "../loaders/resource_types.h"
#include "../perf/cpu_perf_scope.h"
#include "index.h"
#include "node_template_internal.h"

SCENE_IMP_BEGIN_NAMESPACE()

static CORE_NS::IResourceManager::Ptr CreateResourceManager(const SCENE_NS::IRenderContext::Ptr& context)
{
    if (!context) {
        return nullptr;
    }
    auto man = META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
    if (man) {
        man->SetFileManager(CORE_NS::IFileManager::Ptr(&context->GetRenderer()->GetEngine().GetFileManager()));
    }
    return man;
}

static IDiagnostics::Ptr LoadNodeTemplateResources(
    ImportContext& context, BASE_NS::string_view index, const SCENE_NS::IExportedNode::Ptr& pload)
{
    auto man = CreateResourceManager(context.GetRenderContext());
    if (!man) {
        CORE_LOG_E("Failed to create resource manager for node template (index: '%.*s')",
            static_cast<int>(index.size()),
            index.data());
        return context.CreateDiagnostics(
            BASE_NS::string("Failed to create resource manager for node template (index: '") + index + "')");
    }
    auto params = context.GetImportParameters();
    params.object = interface_pointer_cast<META_NS::IObject>(man);
    params.scene = nullptr;
    auto newContext = context.CreateContext(context.GetJsonValue(), params);
    auto res = LoadFile(newContext, index, "resourceIndex");
    if (!res) {
        return res.error;
    }
    pload->SetResources(man->GetResourceInfos({CORE_NS::MatchingResourceId{}}, nullptr));
    return nullptr;
}

static void PopulatePayloadJson(
    ImportContext& context, const SCENE_NS::IExportedNode::Ptr& pload, CORE_NS::json::value::object node)
{
    auto internal = interface_cast<INodeTemplatePayloadInternal>(pload);
    if (!internal) {
        return;
    }
    {
        // Promote the parsed (readonly) subtree into a standalone_value that owns its
        // strings, so the payload outlives the importer's source buffer.
        SCENE_IMP_CPU_PERF_SCOPE("Import", "NodeTemplateSetJson");
        internal->SetJson(CORE_NS::json::standalone_value(CORE_NS::json::value(BASE_NS::move(node))));
    }
    internal->SetSourceFilename(context.GetImportParameters().filename);
    internal->SetFileDepth(context.GetImportParameters().fileDepth);
    internal->SetImporter(context.GetConfig().staticConfig.importer);
}

ImportResult ImportNodeTemplate::Import(ImportContext& context)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", "NodeTemplate");
    if (auto err = context.RequireString("type", "nodeTemplate")) {
        return ImportResult{err};
    }
    auto index = context.GetOptString("index");
    auto group = context.GetOptString("primaryGroup");

    auto templ = META_NS::GetObjectRegistry().Create<META_NS::IObjectTemplate>(SCENE_NS::ClassId::NodeTemplate);
    auto pload = META_NS::GetObjectRegistry().Create<SCENE_NS::IExportedNode>(ClassId::NodeTemplatePayload);
    if (!pload || !templ) {
        CORE_LOG_E("Failed to create NodeTemplate/NodeTemplatePayload objects");
        return ImportResult{context.CreateDiagnostics("Failed to create NodeTemplate/NodeTemplatePayload objects")};
    }

    pload->SetPrimaryGroup(group);
    if (!index.empty()) {
        if (auto err = LoadNodeTemplateResources(context, index, pload)) {
            return ImportResult{err};
        }
    }

    auto node = context.GetOptObject("node");
    if (node.empty()) {
        CORE_LOG_E("Missing 'node' property in node template");
        return ImportResult{context.CreateDiagnostics("Missing 'node' property in node template")};
    }
    PopulatePayloadJson(context, pload, BASE_NS::move(node));

    templ->Instantiator()->SetValue(ClassId::NodeTemplateInstantiator);
    templ->SetContent(interface_pointer_cast<META_NS::IObject>(pload));

    return ImportResult{interface_pointer_cast<META_NS::IObject>(templ)};
}

SCENE_IMP_END_NAMESPACE()
