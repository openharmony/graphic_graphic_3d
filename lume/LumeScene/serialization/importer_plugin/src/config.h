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

#ifndef SCENE_IMP_SRC_CONFIG_H
#define SCENE_IMP_SRC_CONFIG_H

#include <scene/interface/intf_render_context.h>
#include <scene_importer/interface/intf_importer.h>

#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>

SCENE_IMP_BEGIN_NAMESPACE()

class ImportContext;

class ImportBase {
public:
    virtual ~ImportBase() = default;
    virtual ImportResult Import(ImportContext&) = 0;
};

struct ImporterConfig;

class IImporterInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImporterInternal, "4bcf2f57-3965-4b9d-bcab-c18e1bcb489d")
public:
    virtual ImporterConfig GetConfig() const = 0;
};

struct StaticImporterConfig {
    using Types = BASE_NS::unordered_map<BASE_NS::string, BASE_NS::unique_ptr<ImportBase>>;
    Types topTypes;
    Types subTypes;
    ImportOptions options;
    IImporterInternal::WeakPtr importer;
};

struct ImporterConfig {
    const StaticImporterConfig& staticConfig;
    SCENE_NS::IRenderContext::Ptr context;
};

SCENE_IMP_END_NAMESPACE()

#endif