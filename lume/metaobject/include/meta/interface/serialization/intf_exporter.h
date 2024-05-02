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

#ifndef META_INTERFACE_SERIALIZATION_IEXPORTER_H
#define META_INTERFACE_SERIALIZATION_IEXPORTER_H

#include <core/io/intf_file.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

class IExporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExporter, "409223b1-c8ca-4033-8a18-b28aff6bb50c")
public:
    virtual ISerNode::Ptr Export(const IObject::ConstPtr& object) = 0;
};

class IFileExporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IFileExporter, "e3575e71-ff5b-4a8c-838b-c7886f5689cf")
public:
    virtual ReturnError Export(CORE_NS::IFile& output, const IObject::ConstPtr& object) = 0;
};

META_END_NAMESPACE()

#endif
