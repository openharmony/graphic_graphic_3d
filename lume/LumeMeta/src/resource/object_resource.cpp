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

#include "object_resource.h"

#include <meta/api/metadata_util.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

#include "resource_placeholder.h"

META_BEGIN_NAMESPACE()

BASE_NS::Uid ObjectResource::GetResourceType() const
{
    return type_.ToUid();
}
void ObjectResource::SetResourceType(const ObjectId& id)
{
    type_ = id;
}
ReturnError ObjectResource::Export(IExportContext& c) const
{
    if (SerialiseAsResourceId(c) && c.UserContext() != GetSelf()) {
        if (!type_.IsValid()) {
            CORE_LOG_W("Invalid resource type");
            return GenericError::FAIL;
        }
        return ExportResourceId(c);
    }
    return Serializer(c) & NamedValue("resourceType", type_) & AutoSerialize();
}
ReturnError ObjectResource::Import(IImportContext& c)
{
    return Serializer(c) & NamedValue("resourceType", type_) & AutoSerialize();
}

BASE_NS::string ObjectResourceType::GetResourceName() const
{
    return "ObjectResource";
}
BASE_NS::Uid ObjectResourceType::GetResourceType() const
{
    return type_.ToUid();
}
CORE_NS::IResource::Ptr ObjectResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;
    if (s.payload) {
        if (auto importer = GetObjectRegistry().Create<IFileImporter>(META_NS::ClassId::JsonImporter)) {
            importer->SetResourceManager(s.self);
            auto obj = interface_pointer_cast<CORE_NS::IResource>(importer->Import(*s.payload));
            if (!obj || obj->GetResourceType() != type_.ToUid()) {
                CORE_LOG_W("Invalid resource");
                return nullptr;
            }
            if (auto i = interface_cast<CORE_NS::ISetResourceId>(obj)) {
                i->SetResourceId(s.id);
            }
            res = interface_pointer_cast<CORE_NS::IResource>(obj);
        }
    }
    return res;
}
bool ObjectResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    bool res = true;
    if (s.payload) {
        auto i = interface_cast<CORE_NS::IResource>(p);
        if (!i || i->GetResourceType() != type_.ToUid()) {
            CORE_LOG_W("Invalid resource");
            return false;
        }
        if (auto exporter = GetObjectRegistry().Create<IFileExporter>(META_NS::ClassId::JsonExporter)) {
            exporter->SetUserContext(
                interface_pointer_cast<IObject>(CORE_NS::IResource::Ptr(p, const_cast<CORE_NS::IResource*>(p.get()))));
            exporter->SetResourceManager(s.self);
            exporter->SetMetadata(META_NS::SerMetadataValues()
                                      .SetVersion({ 1, 0 })
                                      .SetType("ObjectResource")
                                      .Set("sub-type", type_.ToString()));
            res = exporter->Export(*s.payload, interface_pointer_cast<IObject>(p));
        }
    }
    return res;
}
bool ObjectResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr&) const
{
    return false;
}
void ObjectResourceType::SetResourceType(const ObjectId& id)
{
    type_ = id;
}

bool ObjectResourceOptions::Load(
    CORE_NS::IFile& options, const CORE_NS::ResourceManagerPtr& rman, const CORE_NS::ResourceContextPtr& context)
{
    bool res = false;
    if (auto importer = GetObjectRegistry().Create<IFileImporter>(META_NS::ClassId::JsonImporter)) {
        importer->SetResourceManager(rman);
        importer->SetUserContext(interface_pointer_cast<IObject>(context));
        if (auto obj = importer->Import(options)) {
            if (auto oro = interface_cast<IObjectResourceOptions>(obj)) {
                SetBaseResource(oro->GetBaseResource());
                if (auto in = interface_cast<IMetadata>(obj)) {
                    Clone(*in, *this);
                    res = true;
                }
            }
        }
    }
    return res;
}
bool ObjectResourceOptions::Save(CORE_NS::IFile& options, const CORE_NS::ResourceManagerPtr& rman) const
{
    bool res = false;
    if (auto exporter = GetObjectRegistry().Create<IFileExporter>(META_NS::ClassId::JsonExporter)) {
        exporter->SetResourceManager(rman);
        exporter->SetMetadata(META_NS::SerMetadataValues().SetVersion({ 1, 0 }).SetType("ObjectResourceOptions"));
        res = exporter->Export(options, GetSelf());
    }
    return res;
}
IProperty::Ptr ObjectResourceOptions::GetProperty(BASE_NS::string_view name)
{
    auto i = GetSelf<META_NS::IMetadata>();
    return i ? i->GetProperty(name, META_NS::MetadataQuery::EXISTING) : nullptr;
}
void ObjectResourceOptions::SetProperty(const IProperty::Ptr& p)
{
    if (auto i = GetSelf<META_NS::IMetadata>()) {
        if (auto prop = GetProperty(p->GetName())) {
            i->RemoveProperty(prop);
        }
        i->AddProperty(p);
    }
}
META_NS::ReturnError ObjectResourceOptions::Export(META_NS::IExportContext& c) const
{
    return Serializer(c) & AutoSerialize() & NamedValue("baseResource.name", baseResource_.name) &
           NamedValue("baseResource.group", baseResource_.group);
}
META_NS::ReturnError ObjectResourceOptions::Import(META_NS::IImportContext& c)
{
    return Serializer(c) & AutoSerialize() & NamedValue("baseResource.name", baseResource_.name) &
           NamedValue("baseResource.group", baseResource_.group);
}

META_END_NAMESPACE()
