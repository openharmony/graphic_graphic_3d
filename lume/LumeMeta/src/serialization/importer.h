/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 * Description: Importer
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-03
 */

#ifndef META_SRC_SERIALIZATION_IMPORTER_H
#define META_SRC_SERIALIZATION_IMPORTER_H

#include <functional>

#include <meta/ext/minimal_object.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/resource/intf_resource_consumer.h>
#include <meta/interface/serialization/intf_import_context.h>
#include <meta/interface/serialization/intf_importer.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "../base_object.h"
#include "ser_nodes.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class Importer : public IntroduceInterfaces<BaseObject, IImporter, IImportFunctions, IResourceConsumer> {
    META_OBJECT(Importer, ClassId::Importer, IntroduceInterfaces)
public:
    Importer() : registry_(GetObjectRegistry()), globalData_(registry_.GetGlobalSerializationData()) {}
    explicit Importer(IObjectRegistry& reg, IGlobalSerializationData& data) : registry_(reg), globalData_(data) {}

    IObject::Ptr Import(const ISerNode::ConstPtr& tree) override;
    BASE_NS::unordered_map<InstanceId, InstanceId> GetInstanceIdMapping() const override;
    void SetResourceManager(CORE_NS::IResourceManager::Ptr p) override;
    void SetUserContext(IObject::Ptr) override;

    ReturnError ImportValue(const ISerNode::ConstPtr& n, IAny& entity);
    IAny::Ptr ImportAny(const ISerNode::ConstPtr& n);
    IObject::Ptr ImportRef(const RefUri& ref);

    ReturnError ImportFromNode(const ISerNode::ConstPtr&, IAny& entity) override;
    ReturnError AutoImportObject(const IMapNode& members, IObject::Ptr object);

    void MapInstance(const InstanceId& iid, const IObject::ConstPtr& object);

    CORE_NS::IResourceManager::Ptr GetResourceManager() const override
    {
        return resources_;
    }
    IObject::Ptr GetUserContext() const
    {
        return userContext_;
    }
    SerMetadata GetMetadata() const override
    {
        return metadata_;
    }

private:
    InstanceId ConvertInstanceId(const InstanceId& id) const;
    IObject::Ptr GetReferencedObject(const InstanceId& uid) const;

    IObject::Ptr ImportObject(const ISerNode::ConstPtr& n);
    IObject::Ptr ImportObject(const IObjectNode::ConstPtr& node, IObject::Ptr object);
    ReturnError ImportAny(const IObjectNode::ConstPtr& n, const IAny::Ptr& any);
    ReturnError ImportBuiltinValue(const ISerNode::ConstPtr& n, IAny& entity);
    ReturnError ImportPointer(const ISerNode::ConstPtr& n, IAny& entity);

    ReturnError ImportArray(const ISerNode::ConstPtr& n, IArrayAny& array);
    ReturnError ImportIAttach(const ISerNode::ConstPtr& node, const IObject::Ptr& owner, IAttach& cont);
    ReturnError ImportIContainer(const ISerNode::ConstPtr& node, IContainer& cont);
    ReturnError ImportIObjectFlags(const ISerNode::ConstPtr& node, IObjectFlags& flags);

    ReturnError AutoImportObject(const ISerNode::ConstPtr& node, IObject::Ptr object);
    IObject::Ptr ResolveRefUri(const RefUri& uri) override;
    ReturnError ImportWeakPtrInAny(const ISerNode::ConstPtr& node, const IAny::Ptr& any);

    bool IsRegisteredObjectType(const ObjectId& oid) const;
    bool IsRegisteredObjectOrPropertyType(const ObjectId& oid) const;

private:
    IObjectRegistry& registry_;
    IGlobalSerializationData& globalData_;
    BASE_NS::unordered_map<InstanceId, InstanceId> mapInstanceIds_;
    BASE_NS::vector<IImportFinalize::Ptr> finalizes_;

    struct DeferredUriResolve {
        IAny::Ptr target;
        RefUri uri;
    };
    BASE_NS::vector<DeferredUriResolve> deferred_;
    CORE_NS::IResourceManager::Ptr resources_;
    IObject::Ptr userContext_;
    SerMetadata metadata_;
};

class ImportContext : public IntroduceInterfaces<IImportContext> {
public:
    ImportContext(
        Importer& imp, const BASE_NS::string& name, IObject::Ptr p, const InstanceId& id, IMapNode::ConstPtr node)
        : importer_(imp), object_(BASE_NS::move(p)), iid_(id), node_(BASE_NS::move(node)), name_(name)
    {}

    bool HasMember(BASE_NS::string_view name) const override;
    ReturnError Import(BASE_NS::string_view name, IAny& entity) override;
    ReturnError ImportAny(BASE_NS::string_view name, IAny::Ptr& any) override;
    ReturnError ImportWeakPtr(BASE_NS::string_view name, IObject::WeakPtr& ptr) override;
    ReturnError AutoImport() override;

    IObject::Ptr ResolveRefUri(const RefUri& uri) override;

    ReturnError ImportFromNode(const ISerNode::ConstPtr&, IAny& entity) override;

    CORE_NS::IInterface* Context() const override;
    IObject::Ptr UserContext() const override;
    ReturnError SubstituteThis(IObject::Ptr) override;

    BASE_NS::string GetName() const override;
    SerMetadata GetMetadata() const override;

    IObject::Ptr GetObject() const
    {
        return object_;
    }

private:
    Importer& importer_;
    IObject::Ptr object_;
    InstanceId iid_;
    IMapNode::ConstPtr node_;
    BASE_NS::string name_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
