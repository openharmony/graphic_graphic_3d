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

#include "animation_resource.h"

#include <core/io/intf_filesystem_api.h>

#include <meta/api/metadata_util.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

#include "../serialization/refuri_builder.h"
#include "resource_placeholder.h"

META_BEGIN_NAMESPACE()

BASE_NS::string AnimationResourceType::GetResourceName() const
{
    return "AnimationResource";
}
BASE_NS::Uid AnimationResourceType::GetResourceType() const
{
    return ClassId::AnimationResource.Id().ToUid();
}

static bool SetValuesFromTemplate(const CORE_NS::IResource::ConstPtr& templ, const CORE_NS::IResource::Ptr& res)
{
    auto obj = interface_cast<META_NS::IObject>(res);
    auto ores = interface_cast<IObjectResource>(templ);
    if (!obj || !ores) {
        CORE_LOG_W("Invalid resource type");
        return false;
    }
    auto resm = interface_cast<META_NS::IMetadata>(templ);
    auto m = interface_cast<META_NS::IMetadata>(res);
    if (!(resm && m)) {
        CORE_LOG_D("Unable to access metadata");
        return false;
    }
    for (auto&& p : resm->GetProperties()) {
        auto target = m->GetProperty(p->GetName());
        if (!target) {
            continue;
        }
        if (PropertyLock lock { target }) {
            if (lock->IsDefaultValue()) {
                CopyToDefaultAndReset(p, *target);
            }
        }
        return true;
    }
    return false;
}

static CORE_NS::IResource::Ptr LoadTemplate(const CORE_NS::IResourceType::StorageInfo& s, CORE_NS::IResource::Ptr res)
{
    auto opts = META_NS::GetObjectRegistry()
        .Create<META_NS::IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    if (!opts) {
        return res;
    }
    opts->Load(*s.options, s.self, s.context);
    if (s.path.empty()) {
        auto type = opts->GetProperty<META_NS::IObject::Ptr>("__anim");
        if (!res && type) {
            res = interface_pointer_cast<CORE_NS::IResource>(type->GetValue());
        }
    }
    if (res) {
        if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(res)) {
            auto base = opts->GetBaseResource();
            if (base.IsValid()) {
                auto r = s.self->GetResource(base, s.context);
                if (!r) {
                    CORE_LOG_W("Could not load base resource for %s", base.ToString().c_str());
                }
                if (SetValuesFromTemplate(r, res)) {
                    i->SetTemplateId(base);
                }
            }
        }
    }
    return res;
}

CORE_NS::IResource::Ptr AnimationResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;
    if (s.payload) {
        if (auto importer = GetObjectRegistry().Create<IFileImporter>(META_NS::ClassId::JsonImporter)) {
            importer->SetResourceManager(s.self);
            importer->SetUserContext(interface_pointer_cast<IObject>(s.context));
            res = interface_pointer_cast<CORE_NS::IResource>(importer->Import(*s.payload));
            if (!res) {
                CORE_LOG_W("Invalid resource");
            }
        }
    }
    if (s.options) {
        res = LoadTemplate(s, res);
    }

    return res;
}

static void SaveTemplate(
    const CORE_NS::IResourceType::StorageInfo& s, const CORE_NS::IResource::ConstPtr& res, const ExportOptions& expOpts)
{
    if (auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
            META_NS::ClassId::ObjectResourceOptions)) {
        if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(res)) {
            auto resource = i->GetTemplateId();
            if (resource.IsValid()) {
                opts->SetBaseResource(resource);
            }
        }

        if (s.path.empty()) {
            auto cp = interface_pointer_cast<META_NS::IObject>(res);
            META_NS::IObject::Ptr p(cp, const_cast<META_NS::IObject*>(cp.get()));
            opts->SetProperty("__anim", p);
        }

        // need to set the export options
        if (auto exporter = GetObjectRegistry().Create<IFileExporter>(META_NS::ClassId::JsonExporter)) {
            exporter->SetUserContext(interface_pointer_cast<IObject>(s.context));
            exporter->SetMetadata(META_NS::SerMetadataValues().SetVersion({ 1, 0 }).SetType("ObjectResourceOptions"));
            exporter->Export(*s.options, interface_pointer_cast<IObject>(opts), expOpts);
        }
    }
}

bool AnimationResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    bool res = true;
    Serialization::RefUriBuilder builder;
    for (auto&& v : anchorTypes_) {
        builder.AddAnchorType(v);
    }
    ExportOptions expOpts;
    expOpts.refUriBuilder = &builder;

    if (s.payload) {
        auto i = interface_cast<CORE_NS::IResource>(p);
        if (!i) {
            CORE_LOG_W("Invalid resource");
            return false;
        }
        if (auto exporter = GetObjectRegistry().Create<IFileExporter>(META_NS::ClassId::JsonExporter)) {
            exporter->SetUserContext(interface_pointer_cast<IObject>(s.context));
            exporter->SetMetadata(META_NS::SerMetadataValues().SetVersion({ 1, 0 }).SetType("AnimationResource"));
            res = exporter->Export(*s.payload, interface_pointer_cast<IObject>(p), expOpts);
        }
    }
    if (res && s.options) {
        SaveTemplate(s, p, expOpts);
    }
    return res;
}
bool AnimationResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

META_END_NAMESPACE()
