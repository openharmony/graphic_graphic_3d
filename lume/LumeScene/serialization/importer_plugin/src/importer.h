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

#ifndef SCENE_IMP_SRC_IMPORTER_H
#define SCENE_IMP_SRC_IMPORTER_H

#include <scene_importer/interface/intf_importer.h>

#include <meta/ext/object.h>
#include <meta/interface/interface_helpers.h>

#include "config.h"

SCENE_IMP_BEGIN_NAMESPACE()

class Importer : public META_NS::IntroduceInterfaces<META_NS::BaseObject, IImporter, IImporterInternal> {
    META_OBJECT(Importer, ClassId::Importer, IntroduceInterfaces)
public:
    Importer() = default;
    ~Importer();

    bool Initialize(const SCENE_NS::IRenderContext::Ptr&, ImportOptions) override;
    ImportResult Import(CORE_NS::IFile& data, const ImportParameters& params) override;
    ImportResult Import(BASE_NS::string_view file, const ImportParameters& params) override;

private:
    ImportResult ImportTopLevel(
        const SCENE_NS::IRenderContext::Ptr&, CORE_NS::IFile& data, const ImportParameters& params);

    ImporterConfig GetConfig() const override;

private:
    SCENE_NS::IRenderContext::WeakPtr context_;
    StaticImporterConfig config_;
};

SCENE_IMP_END_NAMESPACE()

#endif