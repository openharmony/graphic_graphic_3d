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

#include <base/containers/unordered_map.h>
#include <core/io/intf_file.h>
#include <core/plugin/intf_interface.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

/// Exporter interface to turn object hierarchy to serialisation node hierarchy
class IExporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExporter, "409223b1-c8ca-4033-8a18-b28aff6bb50c")
public:
    /// Export object hierarchy to serialisation node tree
    virtual ISerNode::Ptr Export(const IObject::ConstPtr& object) = 0;
    /**
     * Set mapping from the actual instance id to serialised instance id
     * (key is actual instance id, value is serialised)
     */
    virtual void SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId>) = 0;
    /**
     * Set resource manager for exporting
     */
    virtual void SetResourceManager(CORE_NS::IResourceManager::Ptr) = 0;
    /**
     * Set User context object
     */
    virtual void SetUserContext(IObject::Ptr) = 0;
    /**
     * Set metadata
     */
    virtual void SetMetadata(SerMetadata) = 0;
};

/// File export interface to export object hierarchy directly to file
class IFileExporter : public IExporter {
    META_INTERFACE(IExporter, IFileExporter, "e3575e71-ff5b-4a8c-838b-c7886f5689cf")
public:
    /// Export object hierarchy to given output file
    virtual ReturnError Export(CORE_NS::IFile& output, const IObject::ConstPtr& object) = 0;
};

META_END_NAMESPACE()

#endif
