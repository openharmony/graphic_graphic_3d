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

#include "importer.h"

#include <core/intf_engine.h>

#include "diagnostics.h"
#include "loaders/resource_types.h"
#include "objects/animation.h"
#include "objects/builtin.h"
#include "objects/camera_node.h"
#include "objects/environment.h"
#include "objects/extension_node.h"
#include "objects/external_node.h"
#include "objects/geometry_node.h"
#include "objects/image.h"
#include "objects/index.h"
#include "objects/light_node.h"
#include "objects/light_probe_group_node.h"
#include "objects/material.h"
#include "objects/node.h"
#include "objects/node_template.h"
#include "objects/postprocess.h"
#include "objects/scene.h"
#include "perf/cpu_perf_scope.h"

SCENE_IMP_BEGIN_NAMESPACE()

Importer::~Importer()
{}

static void RegisterTopTypes(StaticImporterConfig& config)
{
    config.topTypes["scene"] = BASE_NS::make_unique<ImportScene>();
    config.topTypes["resourceIndex"] = BASE_NS::make_unique<ImportIndex>();
    config.topTypes["nodeTemplate"] = BASE_NS::make_unique<ImportNodeTemplate>();
    config.topTypes["material"] = BASE_NS::make_unique<ImportMaterial>();
    config.topTypes["environment"] = BASE_NS::make_unique<ImportEnvironment>();
    config.topTypes["image"] = BASE_NS::make_unique<ImportImage>();
    config.topTypes["postProcess"] = BASE_NS::make_unique<ImportPostProcess>();
    config.topTypes["trackAnimation"] = BASE_NS::make_unique<ImportAnimation>();
    config.topTypes["keyframeAnimation"] = BASE_NS::make_unique<ImportAnimation>();
    config.topTypes["sequentialAnimation"] = BASE_NS::make_unique<ImportAnimation>();
    config.topTypes["parallelAnimation"] = BASE_NS::make_unique<ImportAnimation>();
}

static void RegisterNodeAndAnimationSubTypes(StaticImporterConfig& config)
{
    config.subTypes["abstract-node"] = BASE_NS::make_unique<AbstractImportNode>();
    config.subTypes["node"] = BASE_NS::make_unique<ImportNode>();
    config.subTypes["camera"] = BASE_NS::make_unique<ImportCameraNode>();
    config.subTypes["light"] = BASE_NS::make_unique<ImportLightNode>();
    config.subTypes["lightProbeGroup"] = BASE_NS::make_unique<ImportLightProbeGroupNode>();
    config.subTypes["geometry"] = BASE_NS::make_unique<ImportGeometryNode>();
    config.subTypes["extensionNode"] = BASE_NS::make_unique<ImportExtensionNode>();
    config.subTypes["external"] = BASE_NS::make_unique<ImportExternalNode>();

    config.subTypes["gltfAnimation"] = BASE_NS::make_unique<ImportAnimation>();
    config.subTypes["trackAnimation"] = BASE_NS::make_unique<ImportAnimation>();
    config.subTypes["keyframeAnimation"] = BASE_NS::make_unique<ImportAnimation>();
    config.subTypes["sequentialAnimation"] = BASE_NS::make_unique<ImportAnimation>();
    config.subTypes["parallelAnimation"] = BASE_NS::make_unique<ImportAnimation>();
}

static void RegisterResourceSubTypes(StaticImporterConfig& config)
{
    config.subTypes["material"] = BASE_NS::make_unique<ImportMaterial>();
    config.subTypes["materialTemplate"] = BASE_NS::make_unique<ImportMaterial>();
    config.subTypes["occlusionMaterial"] = BASE_NS::make_unique<ImportMaterial>();
    config.subTypes["occlusionMaterialTemplate"] = BASE_NS::make_unique<ImportMaterial>();
    config.subTypes["environment"] = BASE_NS::make_unique<ImportEnvironment>();
    config.subTypes["environmentTemplate"] = BASE_NS::make_unique<ImportEnvironment>();
    config.subTypes["image"] = BASE_NS::make_unique<ImportImage>();
    config.subTypes["postProcess"] = BASE_NS::make_unique<ImportPostProcess>();
    config.subTypes["postProcessTemplate"] = BASE_NS::make_unique<ImportPostProcess>();
    config.subTypes["animationTemplate"] = BASE_NS::make_unique<ImportAnimation>();
}

static void RegisterBuiltinSubTypes(StaticImporterConfig& config)
{
    config.subTypes["resourceId"] = BASE_NS::make_unique<ImportResourceId>();
    config.subTypes["vec2"] = BASE_NS::make_unique<ImportVec2>();
    config.subTypes["vec3"] = BASE_NS::make_unique<ImportVec3>();
    config.subTypes["vec4"] = BASE_NS::make_unique<ImportVec4>();
    config.subTypes["color"] = BASE_NS::make_unique<ImportColor>();
    config.subTypes["quat"] = BASE_NS::make_unique<ImportQuat>();
    config.subTypes["uvec2"] = BASE_NS::make_unique<ImportUVec2>();
    config.subTypes["mat4x4"] = BASE_NS::make_unique<ImportMat4x4>();
    config.subTypes["objectRef"] = BASE_NS::make_unique<ImportObjectRef>();
}

bool Importer::Initialize(const SCENE_NS::IRenderContext::Ptr& c, ImportOptions options)
{
    if (!c) {
        return false;
    }
    context_ = c;
    config_.options = BASE_NS::move(options);
    config_.importer = GetSelf<IImporterInternal>();

    RegisterTopTypes(config_);
    RegisterNodeAndAnimationSubTypes(config_);
    RegisterResourceSubTypes(config_);
    RegisterBuiltinSubTypes(config_);

    RegisterResourceTypes(GetSelf<IImporter>());
    return true;
}
ImportResult Importer::Import(CORE_NS::IFile& data, const ImportParameters& params)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", params.filename);
    auto c = context_.lock();
    if (!c) {
        CORE_LOG_E("Importer not initialized (file: %s)", params.filename.c_str());
        return ImportResult{MakeSimpleError("Importer not initialized", params.filename)};
    }
    return ImportTopLevel(c, data, params);
}
ImportResult Importer::Import(BASE_NS::string_view file, const ImportParameters& params)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", file);
    BASE_NS::string fn(file);
    auto c = context_.lock();
    if (!c) {
        CORE_LOG_E("Importer not initialized (file: %s)", fn.c_str());
        return ImportResult{MakeSimpleError("Importer not initialized", fn)};
    }
    auto ren = c->GetRenderer();
    if (!ren) {
        CORE_LOG_E("Failed to get renderer (file: %s)", fn.c_str());
        return ImportResult{MakeSimpleError("Failed to get renderer", fn)};
    }
    auto& rm = ren->GetEngine().GetFileManager();
    auto f = rm.OpenFile(file);
    if (!f) {
        CORE_LOG_E("Failed to open file: %s", fn.c_str());
        return ImportResult{MakeSimpleError("Failed to open file: " + fn, fn)};
    }
    ImportParameters p = params;
    if (p.filename.empty()) {
        p.filename = file;
    }
    return ImportTopLevel(c, *f, p);
}
ImportResult Importer::ImportTopLevel(
    const SCENE_NS::IRenderContext::Ptr& c, CORE_NS::IFile& data, const ImportParameters& params)
{
    ImporterConfig config{config_, c};
    TopLevelImportContext sc(config, data, ImportContextParameters{params});
    return sc.Import();
}

ImporterConfig Importer::GetConfig() const
{
    return ImporterConfig{config_, context_.lock()};
}

SCENE_IMP_END_NAMESPACE()