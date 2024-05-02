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

#ifndef META_SRC_SERIALIZATION_IMPORTER_H
#define META_SRC_SERIALIZATION_IMPORTER_H

#include <functional>

#include <meta/ext/minimal_object.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/serialization/intf_import_context.h>
#include <meta/interface/serialization/intf_importer.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "../base_object.h"
#include "ser_nodes.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class Importer : public Internal::BaseObjectFwd<Importer, META_NS::ClassId::Importer, IImporter, IImportFunctions> {
public:
    Importer() : registry_(GetObjectRegistry()), globalData_(registry_.GetGlobalSerializationData()) {}
    explicit Importer(IObjectRegistry& reg, IGlobalSerializationData& data) : registry_(reg), globalData_(data) {}

    IObject::Ptr Import(const ISerNode::ConstPtr& tree) override;
    ReturnError ImportValue(const ISerNode::ConstPtr& n, IAny& entity);
    IAny::Ptr ImportAny(const ISerNode::ConstPtr& n);
    IObject::Ptr ImportRef(const RefUri& ref);

    ReturnError ImportFromNode(const ISerNode::ConstPtr&, IAny& entity) override;
    ReturnError AutoImportObject(const IMapNode& members, IObject::Ptr object);

private:
    InstanceId ConvertInstanceId(const InstanceId& id) const;
    IObject::Ptr GetReferencedObject(const InstanceId& uid) const;

    IObject::Ptr ImportObject(const ISerNode::ConstPtr& n);
    IObject::Ptr ImportObject(const IObjectNode::ConstPtr& node, IObject::Ptr object);
    ReturnError ImportAny(const IObjectNode::ConstPtr& n, const IAny::Ptr& any);
    ReturnError ImportBuiltinValue(const ISerNode::ConstPtr& n, IAny& entity);
    ReturnError ImportPointer(const ISerNode::ConstPtr& n, IAny& entity);

    ReturnError ImportArray(const ISerNode::ConstPtr& n, IArrayAny& array);
    ReturnError ImportIMetadata(const IMapNode& members, const IObject::Ptr& owner, IMetadata& data);
    ReturnError ImportIAttach(const ISerNode::ConstPtr& node, const IObject::Ptr& owner, IAttach& cont);
    ReturnError ImportIContainer(const ISerNode::ConstPtr& node, IContainer& cont);
    ReturnError ImportIObjectFlags(const ISerNode::ConstPtr& node, IObjectFlags& flags);

    ReturnError AutoImportObject(const ISerNode::ConstPtr& node, IObject::Ptr object);
    IObject::Ptr ResolveRefUri(const RefUri& uri) override;
    ReturnError ImportWeakPtrInAny(const ISerNode::ConstPtr& node, const IAny::Ptr& any);

    bool IsRegisteredObjectType(const ObjectId& oid) const;

private:
    IObjectRegistry& registry_;
    IGlobalSerializationData& globalData_;
    Version importVersion_;
    BASE_NS::unordered_map<InstanceId, InstanceId> mapInstanceIds_;
    BASE_NS::vector<IImportFinalize::Ptr> finalizes_;
    size_t tentative_ {};

    struct DeferredUriResolve {
        IAny::Ptr target;
        RefUri uri;
    };
    BASE_NS::vector<DeferredUriResolve> deferred_;
};

class ImportContext : public IntroduceInterfaces<IImportContext> {
public:
    ImportContext(Importer& imp, IObject::Ptr p, IMapNode::ConstPtr node) : importer_(imp), object_(p), node_(node) {}

    bool HasMember(BASE_NS::string_view name) const override;
    ReturnError Import(BASE_NS::string_view name, IAny& entity) override;
    ReturnError ImportAny(BASE_NS::string_view name, IAny::Ptr& any) override;
    ReturnError ImportWeakPtr(BASE_NS::string_view name, IObject::WeakPtr& ptr) override;
    ReturnError AutoImport() override;

    IObject::Ptr ResolveRefUri(const RefUri& uri) override;

    ReturnError ImportFromNode(const ISerNode::ConstPtr&, IAny& entity) override;

private:
    Importer& importer_;
    IObject::Ptr object_;
    IMapNode::ConstPtr node_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
