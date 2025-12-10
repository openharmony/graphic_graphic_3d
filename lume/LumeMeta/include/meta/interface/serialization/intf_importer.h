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

#include <base/containers/unordered_map.h>
#include <core/io/intf_file.h>
#include <core/plugin/intf_interface.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/serialization/intf_ser_node.h>
#include <meta/interface/serialization/intf_ser_transformation.h>

META_BEGIN_NAMESPACE()

struct ImportOptions {
    bool manualDeferredResolve {};
};

/// Interface to import serialisation hierarchy as object
class IImporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImporter, "b3fd4c55-109d-4a44-895c-8568541238ac")
public:
    /// Import serialisation tree as object
    virtual IObject::Ptr Import(const ISerNode::ConstPtr& tree, ImportOptions) = 0;
    IObject::Ptr Import(const ISerNode::ConstPtr& tree)
    {
        return Import(tree, {});
    }
    /**
     * Get mapping from the actual instance id to serialised instance id
     * (key is actual instance id, value is serialised)
     */
    virtual BASE_NS::unordered_map<InstanceId, InstanceId> GetInstanceIdMapping() const = 0;
    /**
     * Set resource manager for importing
     */
    virtual void SetResourceManager(CORE_NS::IResourceManager::Ptr) = 0;
    /**
     * Set User context object
     */
    virtual void SetUserContext(IObject::Ptr) = 0;
    /**
     * Get metadata after importing
     */
    virtual SerMetadata GetMetadata() const = 0;
    /**
     * Do manual later resolving
     */
    virtual void ResolveDeferred() = 0;
};

/// Interface to import objects from file
class IFileImporter : public IImporter {
    META_INTERFACE(IImporter, IFileImporter, "97d96540-675c-48e0-8895-202bf8b9bf69")
public:
    using IImporter::Import;
    /// Import given input file as object
    virtual IObject::Ptr Import(CORE_NS::IFile& input, ImportOptions opts) = 0;
    IObject::Ptr Import(CORE_NS::IFile& input)
    {
        return Import(input, {});
    }
    /// Import given input file as serialisation tree
    virtual ISerNode::Ptr ImportAsTree(CORE_NS::IFile& input) = 0;
    /// Get transformations that are set for this importer
    virtual BASE_NS::vector<ISerTransformation::Ptr> GetTransformations() const = 0;
    /// Set transformations for this importer
    virtual void SetTransformations(BASE_NS::vector<ISerTransformation::Ptr>) = 0;
};

META_END_NAMESPACE()

#endif
