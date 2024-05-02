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
#include <meta/interface/serialization/intf_export_context.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_global_serialization_data.h>

#include "../base_object.h"
#include "ser_nodes.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

constexpr Version EXPORTER_VERSION { 1, 0 };

class Exporter : public Internal::BaseObjectFwd<Exporter, ClassId::Exporter, IExporter, IExportFunctions> {
public:
    Exporter() : registry_(GetObjectRegistry()), globalData_(registry_.GetGlobalSerializationData()) {}
    explicit Exporter(IObjectRegistry& reg, IGlobalSerializationData& data) : registry_(reg), globalData_(data) {}

    ISerNode::Ptr Export(const IObject::ConstPtr& object) override;

    ReturnError ExportValue(const IAny& entity, ISerNode::Ptr&);
    ReturnError ExportAny(const IAny::ConstPtr& any, ISerNode::Ptr&);
    ReturnError ExportWeakPtr(const IObject::ConstWeakPtr& ptr, ISerNode::Ptr&);

    ReturnError ExportToNode(const IAny& entity, ISerNode::Ptr&) override;
    ReturnError AutoExportObjectMembers(const IObject::ConstPtr& object, BASE_NS::vector<NamedNode>& members);

private:
    bool ShouldSerialize(const IObject::ConstPtr& object) const;
    bool ShouldSerialize(const IAny& any) const;
    InstanceId ConvertInstanceId(const InstanceId& id) const;
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

    BASE_NS::vector<NamedNode> ExportIMetadata(const IMetadata& data);
    ISerNode::Ptr ExportIContainer(const IContainer& cont);

private:
    IObjectRegistry& registry_;
    IGlobalSerializationData& globalData_;
    BASE_NS::unordered_map<InstanceId, IObject::ConstWeakPtr> exported_;
    BASE_NS::unordered_map<InstanceId, InstanceId> mapInstanceIds_;
};

class ExportContext : public IntroduceInterfaces<IExportContext> {
public:
    ExportContext(Exporter& exp, const IObject::ConstPtr& p) : exporter_(exp), object_(p) {}

    BASE_NS::shared_ptr<MapNode> ExtractNode();

    ReturnError Export(BASE_NS::string_view name, const IAny& entity) override;
    ReturnError ExportAny(BASE_NS::string_view name, const IAny::Ptr& any) override;
    ReturnError ExportWeakPtr(BASE_NS::string_view name, const IObject::ConstWeakPtr& ptr) override;
    ReturnError AutoExport() override;

    ReturnError ExportToNode(const IAny& entity, ISerNode::Ptr&) override;

private:
    Exporter& exporter_;
    IObject::ConstPtr object_;
    BASE_NS::vector<NamedNode> elements_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
