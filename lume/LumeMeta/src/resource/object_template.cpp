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

#include "object_template.h"

#include <meta/api/metadata_util.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

#include "resource_placeholder.h"

META_BEGIN_NAMESPACE()

CORE_NS::ResourceType ObjectTemplate::GetResourceType() const
{
    return ClassId::ObjectTemplate.Id().ToUid();
}

IObject::Ptr ObjectTemplate::Instantiate(const SharedPtrIInterface& context) const
{
    IObject::Ptr res;
    auto id = GetValue(Instantiator());
    if (id.IsValid()) {
        if (auto instor = GetObjectRegistry().Create<IObjectInstantiator>(id)) {
            res = instor->Instantiate(*this, context);
        }
    }
    return res;
}

META_NS::IObject::Ptr ObjectTemplate::Update(IObject& inst, const SharedPtrIInterface& context) const
{
    META_NS::IObject::Ptr res;
    auto id = GetValue(Instantiator());
    if (id.IsValid()) {
        if (auto instor = GetObjectRegistry().Create<IObjectInstantiator>(id)) {
            res = instor->Update(inst, *this, context);
        }
    }
    return res;
}

bool ObjectTemplate::SetContent(const IObject::Ptr& content)
{
    SetValue(META_ACCESS_PROPERTY(Content), content);
    return true;
}

template<typename Func>
IterationResult ObjectTemplate::IterateImpl(const Func& f) const
{
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return IterationResult::FAILED;
    }
    if (META_ACCESS_PROPERTY_VALUE(ContentSearchable)) {
        if (auto c = META_ACCESS_PROPERTY_VALUE(Content)) {
            return f->Invoke(c);
        }
    }
    return IterationResult::CONTINUE;
}

IterationResult ObjectTemplate::Iterate(const IterationParameters& params)
{
    return IterateImpl(params.function.GetInterface<IIterableCallable<IObject::Ptr>>());
}

IterationResult ObjectTemplate::Iterate(const IterationParameters& params) const
{
    return IterateImpl(params.function.GetInterface<IIterableConstCallable<IObject::Ptr>>());
}

BASE_NS::string ObjectTemplateType::GetResourceName() const
{
    return "ObjectTemplate";
}
BASE_NS::Uid ObjectTemplateType::GetResourceType() const
{
    return ClassId::ObjectTemplate.Id().ToUid();
}
CORE_NS::IResource::Ptr ObjectTemplateType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;
    if (s.payload) {
        if (auto importer = GetObjectRegistry().Create<IFileImporter>(META_NS::ClassId::JsonImporter)) {
            importer->SetResourceManager(s.self);
            auto obj = interface_pointer_cast<CORE_NS::IResource>(importer->Import(*s.payload));
            if (auto i = interface_cast<CORE_NS::ISetResourceId>(obj)) {
                i->SetResourceId(s.id);
            }
            res = interface_pointer_cast<CORE_NS::IResource>(obj);
        }
    }
    return res;
}
bool ObjectTemplateType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    bool res = true;
    if (s.payload) {
        if (auto exporter = GetObjectRegistry().Create<IFileExporter>(META_NS::ClassId::JsonExporter)) {
            exporter->SetUserContext(
                interface_pointer_cast<IObject>(CORE_NS::IResource::Ptr(p, const_cast<CORE_NS::IResource*>(p.get()))));
            exporter->SetResourceManager(s.self);
            exporter->SetMetadata(META_NS::SerMetadataValues().SetVersion({ 1, 0 }).SetType("ObjectTemplate"));
            res = exporter->Export(*s.payload, interface_pointer_cast<IObject>(p));
        }
    }
    return res;
}
bool ObjectTemplateType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

META_END_NAMESPACE()
