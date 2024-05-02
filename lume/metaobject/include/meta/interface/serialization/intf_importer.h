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

#ifndef META_INTERFACE_SERIALIZATION_IIMPORTER_H
#define META_INTERFACE_SERIALIZATION_IIMPORTER_H

#include <core/io/intf_file.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

class IImporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImporter, "b3fd4c55-109d-4a44-895c-8568541238ac")
public:
    virtual IObject::Ptr Import(const ISerNode::ConstPtr& tree) = 0;
};

class IFileImporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IFileImporter, "97d96540-675c-48e0-8895-202bf8b9bf69")
public:
    virtual IObject::Ptr Import(CORE_NS::IFile& input) = 0;
    virtual ISerNode::Ptr ImportAsTree(CORE_NS::IFile& input) = 0;
};

META_END_NAMESPACE()

#endif
