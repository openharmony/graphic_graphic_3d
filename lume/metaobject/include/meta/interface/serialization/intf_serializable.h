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

class ISerializable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerializable, "b832ef10-9cb2-4222-8429-eef0e1bbed3b")
public:
    virtual ReturnError Export(IExportContext&) const = 0;
    virtual ReturnError Import(IImportContext&) = 0;
};

class IImportFinalize : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImportFinalize, "b01e26e5-5b83-45fa-94c4-b461425b2311")
public:
    virtual ReturnError Finalize(IImportFunctions&) = 0;
};

META_INTERFACE_TYPE(ISerializable);

META_END_NAMESPACE()

#endif
