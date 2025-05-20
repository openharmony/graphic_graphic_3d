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

#ifndef META_SRC_RESOURCE_OBJECT_RESOURCE_H
#define META_SRC_RESOURCE_OBJECT_RESOURCE_H

#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class ObjectResource : public IntroduceInterfaces<MetaObject, Resource, IObjectResource> {
    META_OBJECT(ObjectResource, ClassId::ObjectResource, IntroduceInterfaces)
public:
    BASE_NS::Uid GetResourceType() const override;
    void SetResourceType(const ObjectId& id) override;

    ReturnError Export(IExportContext& c) const override;
    ReturnError Import(IImportContext& c) override;

private:
    ObjectId type_ = ClassId::ObjectResourceType.Id();
};

class ObjectResourceType : public IntroduceInterfaces<MetaObject, CORE_NS::IResourceType, IObjectResource> {
    META_OBJECT(ObjectResourceType, ClassId::ObjectResourceType, IntroduceInterfaces)
public:
    BASE_NS::string GetResourceName() const override;
    BASE_NS::Uid GetResourceType() const override;
    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;

    void SetResourceType(const ObjectId& id) override;

private:
    ObjectId type_ = ClassId::ObjectResourceType.Id();
};

class ObjectResourceOptions
    : public IntroduceInterfaces<MetaObject, META_NS::IObjectResourceOptions, META_NS::ISerializable> {
    META_OBJECT(ObjectResourceOptions, ClassId::ObjectResourceOptions, IntroduceInterfaces)
public:
    bool Load(CORE_NS::IFile& options, const CORE_NS::ResourceManagerPtr&, const CORE_NS::ResourceContextPtr&) override;
    bool Save(CORE_NS::IFile& options, const CORE_NS::ResourceManagerPtr&) const override;

    void SetBaseResource(CORE_NS::ResourceId id) override
    {
        baseResource_ = BASE_NS::move(id);
    }
    CORE_NS::ResourceId GetBaseResource() const override
    {
        return baseResource_;
    }

    IProperty::Ptr GetProperty(BASE_NS::string_view name) override;
    void SetProperty(const IProperty::Ptr&) override;

    META_NS::ReturnError Export(META_NS::IExportContext&) const override;
    META_NS::ReturnError Import(META_NS::IImportContext&) override;

private:
    CORE_NS::ResourceId baseResource_;
};

META_END_NAMESPACE()

#endif
