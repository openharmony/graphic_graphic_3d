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

#include "extension_node.h"

#include "../diagnostics.h"
#include "../import_helpers.h"
#include "property.h"

SCENE_IMP_BEGIN_NAMESPACE()

SCENE_NS::INode::Ptr ImportExtensionNode::ConstructNode(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, const BASE_NS::string& name)
{
    auto id = GetOptObjectId(context, "nodeUid");
    if (!id.value) {
        CORE_LOG_E("Extension node missing or invalid nodeUid");
        return nullptr;
    }
    return scene
        ->CreateNode(interface_pointer_cast<SCENE_NS::INode>(context.GetImportParameters().object), name, *id.value)
        .GetResult();
}

ImportResult ImportExtensionNode::Import(ImportContext& context)
{
    auto res = ImportNode(context, "extensionNode");
    if (res) {
        ErrorHandler h(context);
        if (auto err = ImportProperties(context, *interface_pointer_cast<META_NS::IObject>(res.object));
            h.Handle(err)) {
            return ImportResult{err};
        }
        MergeDiagnostics(res.error, static_cast<IDiagnostics::Ptr>(h));
    }
    return res;
}

SCENE_IMP_END_NAMESPACE()
