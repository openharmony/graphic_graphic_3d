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

#ifndef SCENE_IMP_INTERFACE_IDIAGNOSTICS_H
#define SCENE_IMP_INTERFACE_IDIAGNOSTICS_H

#include <scene_importer/base/namespace.h>

#include <meta/base/interface_macros.h>

SCENE_IMP_BEGIN_NAMESPACE()

/// Information about a single diagnostic message.
struct ErrorInfo {
    /// Human-readable description of the error.
    BASE_NS::string message;
    /// Source file associated with the error. Line/column info is not currently available.
    BASE_NS::string file;
};

/// A diagnostic error, optionally with a trace of contributing locations.
struct Error : ErrorInfo {
    /// Optional chain of locations that led to this error.
    BASE_NS::vector<ErrorInfo> trace;
};

/**
 * @brief Interface for collecting and querying import diagnostics.
 *
 * An IDiagnostics instance is returned as part of ImportResult when an import
 * fails, and can also be produced when continueOnError is set in ImportOptions.
 * Use GetErrors() to inspect what went wrong.
 */
class IDiagnostics : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDiagnostics, "365e6f16-2a80-49e5-9480-61fb6b0a0fcd")
public:
    /**
     * @brief Returns all errors collected during the import.
     * @return List of errors, empty if the import succeeded without issues.
     */
    virtual BASE_NS::vector<Error> GetErrors() const = 0;
};

SCENE_IMP_END_NAMESPACE()

#endif
