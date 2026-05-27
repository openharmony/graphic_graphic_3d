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

#ifndef SCENE_IMP_INTERFACE_IIMPORTER_H
#define SCENE_IMP_INTERFACE_IIMPORTER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/scene_options.h>
#include <scene_importer/base/namespace.h>
#include <scene_importer/interface/intf_diagnostics.h>

#include <base/containers/unique_ptr.h>
#include <core/io/intf_file.h>

SCENE_IMP_BEGIN_NAMESPACE()

/// Result of an import operation. Holds either the imported object or diagnostic errors.
struct ImportResult {
    ImportResult() = default;
    explicit ImportResult(META_NS::IObject::Ptr obj) : object(BASE_NS::move(obj))
    {}
    explicit ImportResult(IDiagnostics::Ptr err) : error(BASE_NS::move(err))
    {}

    /// The imported object. Non-null on success.
    META_NS::IObject::Ptr object;
    /// Diagnostics describing what went wrong. Non-null on failure or when continueOnError is set.
    IDiagnostics::Ptr error;

    /// Returns true if the import produced an object.
    explicit operator bool() const
    {
        return object != nullptr;
    }
};

/// Parameters passed to each import call.
struct ImportParameters {
    /// Associated scene to import into, if any.
    SCENE_NS::IScene::Ptr scene;
    /// Optional filename used in diagnostics messages when importing from an IFile.
    BASE_NS::string filename;
};

/// Options that control importer behaviour across all imports.
struct ImportOptions {
    /// Scene creation options forwarded when a new scene is created during import.
    SCENE_NS::SceneOptions scene;
    /// When true, the importer continues past recoverable errors and reports them via ImportResult::error.
    bool continueOnError{false};
};

/**
 * @brief Interface for importing scene data from files or streams.
 *
 * Obtain an instance via the class registry using the Importer class id.
 * Call Initialize() once with a render context before calling Import().
 */
class IImporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImporter, "78f86f45-1652-43e3-b54f-81969f017dae")
public:
    /**
     * @brief Initialize the importer with a render context and options.
     * @param context Render context used for resource creation.
     * @param options Options that apply to all subsequent import calls.
     * @return True on success.
     */
    virtual bool Initialize(const SCENE_NS::IRenderContext::Ptr& context, ImportOptions options) = 0;

    /**
     * @brief Import scene data from an open file stream.
     * @param data    File stream to read from.
     * @param params  Parameters for this import, e.g. associated scene.
     * @return ImportResult with the imported object, or diagnostics on failure.
     */
    virtual ImportResult Import(CORE_NS::IFile& data, const ImportParameters& params) = 0;

    /**
     * @brief Import scene data from a file path.
     * @param file    Path to the file to import.
     * @param params  Parameters for this import, e.g. associated scene.
     * @return ImportResult with the imported object, or diagnostics on failure.
     */
    virtual ImportResult Import(BASE_NS::string_view file, const ImportParameters& params) = 0;
};

META_REGISTER_CLASS(Importer, "9756e836-7794-4bb4-ad39-8b325ad9f7d9", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_IMP_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_IMP_NS::IImporter)

#endif
