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

#include "geometry_node.h"

#include <scene/interface/intf_mesh.h>

#include <core/log.h>

#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

SCENE_NS::INode::Ptr ImportGeometryNode::ConstructNode(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, const BASE_NS::string& name)
{
    return scene
        ->CreateNode(interface_pointer_cast<SCENE_NS::INode>(context.GetImportParameters().object),
            name,
            SCENE_NS::ClassId::MeshNode)
        .GetResult();
}

ImportResult ImportGeometryNode::Import(ImportContext& context)
{
    auto result = ImportNode(context, "geometry");
    if (!result) {
        return result;
    }

    auto meshId = GetOptResourceId(context, "mesh");
    if (!meshId.value) {
        // Geometry node without a mesh reference is valid (placeholder node).
        return result;
    }
    // The resource manager builds the live IMesh on demand via
    // MeshTemplate::ApplyOptions; per-submesh materials are already wired by
    // the apply step, so the geometry node only needs to plug the IMesh in.
    auto meshRes = ResolveDeferredResource(context.GetImportParameters(), context.GetRenderContext(), *meshId.value);
    auto mesh = interface_pointer_cast<SCENE_NS::IMesh>(meshRes);
    if (!mesh) {
        CORE_LOG_W("Geometry node mesh '%s' did not resolve to an IMesh", meshId.value->ToString().c_str());
        return result;
    }
    auto node = interface_pointer_cast<SCENE_NS::INode>(result.object);
    if (auto meshNode = interface_cast<SCENE_NS::IMeshAccess>(node)) {
        meshNode->SetMesh(mesh);
    }
    return result;
}

SCENE_IMP_END_NAMESPACE()
