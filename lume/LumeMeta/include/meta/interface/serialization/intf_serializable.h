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

#ifndef META_INTERFACE_SERIALIZATION_ISERIALIZABLE_H
#define META_INTERFACE_SERIALIZATION_ISERIALIZABLE_H

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/serialization/intf_export_context.h>
#include <meta/interface/serialization/intf_import_context.h>

META_BEGIN_NAMESPACE()

/// Explicit serialisation interface for objects
class ISerializable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerializable, "b832ef10-9cb2-4222-8429-eef0e1bbed3b")
public:
    /// Called when object is being exported
    virtual ReturnError Export(IExportContext&) const = 0;
    /// Called when object is being imported
    virtual ReturnError Import(IImportContext&) = 0;
};

/// Interface for objects to support finalize call after the whole object hierarchy has been imported
class IImportFinalize : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImportFinalize, "b01e26e5-5b83-45fa-94c4-b461425b2311")
public:
    /**
     * @brief Called after whole object interface has been imported
     * @return Return error to fail the import process, or success to continue
     *
     * This can be used to check the data is valid or use IImportFunctions to resolve ref uri.
     * The Finalize calls are called the opposite order than importing, it is called last for the root node.
     */
    virtual ReturnError Finalize(IImportFunctions&) = 0;
};

META_INTERFACE_TYPE(META_NS::ISerializable)

META_END_NAMESPACE()

#endif
