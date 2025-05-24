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

#ifndef META_SRC_SERIALIZATION_EXPORTER_H
#define META_SRC_SERIALIZATION_EXPORTER_H

#include <base/containers/unordered_map.h>

#include <meta/ext/minimal_object.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource_consumer.h>
#include <meta/interface/serialization/intf_export_context.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_global_serialization_data.h>

#include "base_object.h"
#include "ser_nodes.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

constexpr Version VERSION { 2, 0 };
constexpr Version EXPORTER_VERSION { 1, 0 };

class Exporter : public IntroduceInterfaces<BaseObject, IExporter, IExportFunctions, IResourceConsumer> {
    META_OBJECT(Exporter, ClassId::Exporter, IntroduceInterfaces)
public:
    Exporter() : registry_(GetObjectRegistry()), globalData_(registry_.GetGlobalSerializationData()) {}
    explicit Exporter(IObjectRegistry& reg, IGlobalSerializationData& data) : registry_(reg), globalData_(data) {}

    ISerNode::Ptr Export(const IObject::ConstPtr& object) override;
    void SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId>) override;
    void SetResourceManager(CORE_NS::IResourceManager::Ptr p) override;
    void SetUserContext(IObject::Ptr) override;
    void SetMetadata(SerMetadata m) override;

    ReturnError ExportValue(const IAny& entity, ISerNode::Ptr&);
    ReturnError ExportAny(const IAny::ConstPtr& any, ISerNode::Ptr&);
    ReturnError ExportWeakPtr(const IObject::ConstWeakPtr& ptr, ISerNode::Ptr&);

    ReturnError ExportToNode(const IAny& entity, ISerNode::Ptr&) override;
    ReturnError AutoExportObjectMembers(const IObject::ConstPtr& object, BASE_NS::vector<NamedNode>& members);

    InstanceId ConvertInstanceId(const InstanceId& id) const;

    CORE_NS::IResourceManager::Ptr GetResourceManager() const override
    {
        return resources_;
    }
    META_NS::IObject::Ptr GetUserContext() const
    {
        return userContext_;
    }
    const SerMetadata& GetMetadata() const
    {
        return metadata_;
    }

private:
    struct DeferredWeakPtrResolve {
        IRefUriNode::Ptr node;
        IObject::ConstWeakPtr ptr;
    };

    bool ShouldSerialize(const IObject::ConstPtr& object) const;
    bool ShouldSerialize(const IAny& any) const;
    bool MarkExported(const IObject::ConstPtr& object);
    bool HasBeenExported(const InstanceId& id) const;

    ReturnError ExportObject(const IObject::ConstPtr& object, ISerNode::Ptr&);
    ReturnError ExportPointer(const IAny& entity, ISerNode::Ptr&);
    ISerNode::Ptr ExportBuiltinValue(const IAny& value);
    ISerNode::Ptr ExportArray(const IArrayAny& array);

    ISerNode::Ptr CreateObjectNode(const IObject::ConstPtr& object, BASE_NS::shared_ptr<MapNode> node);
    ISerNode::Ptr CreateObjectRefNode(const RefUri& ref);
    ISerNode::Ptr CreateObjectRefNode(const IObject::ConstPtr& object);
    ISerNode::Ptr AutoExportObject(const IObject::ConstPtr& object);
    IObject::Ptr ResolveUriSegment(const IObject::ConstPtr& ptr, RefUri& uri) const;
    ISerNode::Ptr ExportIContainer(const IContainer& cont);
    ReturnError ResolveDeferredWeakPtr(const DeferredWeakPtrResolve& d);

private:
    IObjectRegistry& registry_;
    IGlobalSerializationData& globalData_;
    BASE_NS::unordered_map<InstanceId, IObject::ConstWeakPtr> exported_;
    BASE_NS::unordered_map<InstanceId, InstanceId> mapInstanceIds_;
    BASE_NS::vector<DeferredWeakPtrResolve> deferred_;
    CORE_NS::IResourceManager::Ptr resources_;
    IObject::Ptr userContext_;
    SerMetadata metadata_;
};

class ExportContext : public IntroduceInterfaces<IExportContext> {
public:
    ExportContext(Exporter& exp, IObject::ConstPtr p) : exporter_(exp), object_(BASE_NS::move(p)) {}

    BASE_NS::shared_ptr<MapNode> ExtractNode();

    ReturnError Export(BASE_NS::string_view name, const IAny& entity) override;
    ReturnError ExportAny(BASE_NS::string_view name, const IAny::Ptr& any) override;
    ReturnError ExportWeakPtr(BASE_NS::string_view name, const IObject::ConstWeakPtr& ptr) override;
    ReturnError AutoExport() override;

    ReturnError ExportToNode(const IAny& entity, ISerNode::Ptr&) override;

    CORE_NS::IInterface* Context() const override;
    META_NS::IObject::Ptr UserContext() const override;
    ReturnError SubstituteThis(ISerNode::Ptr) override;
    SerMetadata GetMetadata() const override;

    ISerNode::Ptr GetSubstitution() const
    {
        return substNode_;
    }

private:
    Exporter& exporter_;
    IObject::ConstPtr object_;
    BASE_NS::vector<NamedNode> elements_;
    ISerNode::Ptr substNode_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
