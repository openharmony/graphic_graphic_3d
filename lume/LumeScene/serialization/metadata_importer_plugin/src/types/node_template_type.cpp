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

#include "node_template_type.h"

#include <scene/interface/resource/intf_exported_node.h>

#include <core/intf_engine.h>

#include <meta/ext/serialization/serializer.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

#include "../resource_util.h"
#include "resource_types.h"

SCENE_BEGIN_NAMESPACE()

static CORE_NS::IResourceManager::Ptr CreateResourceManager(const IRenderContext::Ptr& context)
{
    if (!context) {
        return nullptr;
    }
    auto renderer = context->GetRenderer();
    if (!renderer) {
        return nullptr;
    }
    auto man = META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
    if (man) {
        man->SetFileManager(CORE_NS::IFileManager::Ptr(&renderer->GetEngine().GetFileManager()));
        RegisterResourceTypes(man, CreateRenderContextArg(context));
    }
    return man;
}

void NodeTemplateType::LoadNodeTemplateResources(const META_NS::IObjectTemplate& ot, const StorageInfo& s) const
{
    if (auto exp = interface_cast<IExportedNode>(META_NS::GetValue(ot.Content()))) {
        if (auto man = CreateResourceManager(context_.lock())) {
            auto dot = s.path.find_last_of('.');
            if (dot == BASE_NS::string::npos) {
                CORE_LOG_W("Invalid node template path (missing extension): %.*s",
                    static_cast<int>(s.path.size()),
                    s.path.data());
                return;
            }
            auto file = s.path.substr(0, dot) + ".index";
            if (man->Import(file) != CORE_NS::IResourceManager::Result::OK) {
                CORE_LOG_W("Failed to import node template index: %s", file.c_str());
            }
            exp->SetResources(man->GetResourceInfos({CORE_NS::MatchingResourceId{}}, nullptr));
        }
    }
}

CORE_NS::IResource::Ptr NodeTemplateType::LoadResource(const StorageInfo& s) const
{
    if (!s.payload) {
        return nullptr;
    }
    CORE_NS::IResource::Ptr res;
    if (auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter)) {
        importer->SetUserContext(interface_pointer_cast<IObject>(s.context));
        importer->SetResourceManager(s.self);
        res = interface_pointer_cast<CORE_NS::IResource>(importer->Import(*s.payload));
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(res)) {
            i->SetResourceId({s.id, s.context});
        }
        if (auto ot = interface_cast<META_NS::IObjectTemplate>(res)) {
            LoadNodeTemplateResources(*ot, s);
        }
    }
    return res;
}

void NodeTemplateType::SaveNodeTemplateResources(const META_NS::IObjectTemplate& ot, const StorageInfo& s) const
{
    if (auto exp = interface_cast<IExportedNode>(META_NS::GetValue(ot.Content()))) {
        if (auto man = CreateResourceManager(context_.lock())) {
            CopyResourceInfos(exp->GetResources(), man, s.context);
            auto dot = s.path.find_last_of('.');
            if (dot == BASE_NS::string::npos) {
                CORE_LOG_W("Invalid node template path (missing extension): %.*s",
                    static_cast<int>(s.path.size()),
                    s.path.data());
                return;
            }
            auto file = s.path.substr(0, dot) + ".index";
            if (man->Export(file, s.context) != CORE_NS::IResourceManager::Result::OK) {
                CORE_LOG_W("Failed to export node template index: %s", file.c_str());
            }
        }
    }
}

bool NodeTemplateType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    if (!s.payload) {
        return true;
    }
    bool res = true;
    if (auto exporter = META_NS::GetObjectRegistry().Create<META_NS::IFileExporter>(META_NS::ClassId::JsonExporter)) {
        exporter->SetUserContext(interface_pointer_cast<IObject>(s.context));
        exporter->SetResourceManager(s.self);
        exporter->SetMetadata(META_NS::SerMetadataValues().SetVersion({1, 0}).SetType("NodeTemplate"));
        res = exporter->Export(*s.payload, interface_pointer_cast<IObject>(p));
        if (res) {
            if (auto ot = interface_cast<META_NS::IObjectTemplate>(p)) {
                SaveNodeTemplateResources(*ot, s);
            }
        }
    }
    return res;
}
bool NodeTemplateType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

SCENE_END_NAMESPACE()
