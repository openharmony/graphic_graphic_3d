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

#include "node_template_type.h"

SCENE_IMP_BEGIN_NAMESPACE()

static void LogImportErrors(const BASE_NS::string& name, const CORE_NS::ResourceId& id, const ImportResult& res)
{
    if (!res.error) {
        CORE_LOG_E("Failed to import %s resource '%s': no diagnostics", name.c_str(), id.ToString().c_str());
        return;
    }
    for (auto&& e : res.error->GetErrors()) {
        CORE_LOG_E("Failed to import %s resource '%s': %s", name.c_str(), id.ToString().c_str(), e.message.c_str());
    }
}

CORE_NS::IResource::Ptr NodeTemplateResourceType::LoadResource(const StorageInfo& s) const
{
    auto importer = importer_.lock();
    if (!importer) {
        CORE_LOG_E(
            "Importer not available when loading %s resource: %s", GetResourceName().c_str(), s.id.ToString().c_str());
        return nullptr;
    }
    // Node templates carry no resource options, so the constructed template is returned as-is
    // without an ApplyOptions step. Path resolutions inside the template are anchored at
    // instantiation time, hence params.scene is intentionally left unset.
    ImportParameters params;
    params.filename = s.path;
    ImportResult res;
    if (s.payload) {
        res = importer->Import(*s.payload, params);
    } else if (!s.path.empty()) {
        res = importer->Import(s.path, params);
    } else {
        CORE_LOG_E(
            "No payload or path when loading %s resource: %s", GetResourceName().c_str(), s.id.ToString().c_str());
        return nullptr;
    }
    auto resource = interface_pointer_cast<CORE_NS::IResource>(res.object);
    if (!resource) {
        LogImportErrors(GetResourceName(), s.id, res);
    }
    return resource;
}

bool NodeTemplateResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& res) const
{
    return false;
}

SCENE_IMP_END_NAMESPACE()
