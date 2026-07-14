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

#ifndef META_INTERFACE_RESOURCE_IRESOURCE_H
#define META_INTERFACE_RESOURCE_IRESOURCE_H

#include <core/resources/intf_resource.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/property/construct_property.h>

META_BEGIN_NAMESPACE()

class IDerivedFromTemplate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDerivedFromTemplate, "ea586292-8c95-482e-af6c-a0611818bbcb")
public:
    virtual bool SetTemplateId(CORE_NS::ResourceId) = 0;
    virtual CORE_NS::ResourceId GetTemplateId() const = 0;

    virtual META_NS::ObjectId GetDefaultAccess() const = 0;
    virtual CORE_NS::IResource::Ptr CreateEmptyTemplate() const = 0;
    virtual CORE_NS::IResource::Ptr CreateTemplate() const = 0;
    virtual bool SetTemplate(const CORE_NS::IResource::ConstPtr& templ) const = 0;
};

class IResourceTemplateAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceTemplateAccess, "d0a253fb-c844-433b-9e55-d8d7256e6c06")
public:
    virtual CORE_NS::IResource::Ptr CreateEmptyTemplate() const = 0;
    virtual CORE_NS::IResource::Ptr CreateTemplateFromResource(const CORE_NS::IResource::ConstPtr&) const = 0;
    virtual bool SetValuesFromTemplate(
        const CORE_NS::IResource::ConstPtr& templ, const CORE_NS::IResource::Ptr& res) const = 0;
};

class IDerivedResourceOptions : public CORE_NS::IResourceOptions {
    META_INTERFACE(CORE_NS::IResourceOptions, IDerivedResourceOptions, "86578c9c-660a-49ed-9ee9-c3adb6284ad3")
public:
    virtual void SetBaseResource(CORE_NS::ResourceId) = 0;
    virtual CORE_NS::ResourceId GetBaseResource() const = 0;
};

class IObjectResourceOptions : public IDerivedResourceOptions {
    META_INTERFACE(CORE_NS::IResourceOptions, IObjectResourceOptions, "3a8559b2-ad80-48f5-8e3b-3340c9048f77")
public:
    virtual bool Load(
        CORE_NS::IFile& options, const CORE_NS::ResourceManagerPtr&, const CORE_NS::ResourceContextPtr&) = 0;
    virtual bool Save(
        CORE_NS::IFile& options, const CORE_NS::ResourceManagerPtr&, const CORE_NS::ResourceContextPtr&) const = 0;

    virtual void SetOptionData(BASE_NS::vector<uint8_t>) = 0;
    virtual BASE_NS::vector<uint8_t> GetOptionData() const = 0;

    virtual IProperty::Ptr GetProperty(BASE_NS::string_view name) = 0;
    template<typename T>
    Property<T> GetProperty(BASE_NS::string_view name)
    {
        return Property<T>(GetProperty(name));
    }

    virtual void SetProperty(const IProperty::Ptr&) = 0;
    template<typename T>
    void SetProperty(BASE_NS::string_view name, const T& value)
    {
        SetProperty(ConstructProperty<T>(name, value));
    }
};

class IResourceContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceContext, "c3ceaf56-f375-455c-ba86-b01550221103")
public:
    virtual CORE_NS::ResourceContextPtr GetContext() const = 0;
};

class ICollectResources : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICollectResources, "d84fd50f-089a-45bf-8a98-911c950b2433")
public:
    virtual void AddResource(CORE_NS::ResourceIdContext) = 0;
};

inline bool IsResourceMatch(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const CORE_NS::ResourceId& id)
{
    for (auto&& s : selection) {
        if ((s.hasWildCardName || id.name == s.name) && (s.hasWildCardGroup || id.group == s.group)) {
            return true;
        }
    }
    return false;
}

struct ResourceIdLess {
    bool operator()(const CORE_NS::ResourceId& l, const CORE_NS::ResourceId& r) const
    {
        return l.group < r.group || (l.group == r.group && l.name < r.name);
    }
};

META_REGISTER_CLASS(
    ObjectResourceOptions, "196213b6-eb63-4a29-89be-1ec10ed68596", META_NS::ObjectCategoryBits::NO_CATEGORY)

META_END_NAMESPACE()

META_TYPE(CORE_NS::ResourceId)
META_TYPE(CORE_NS::ResourceIdContext)

META_INTERFACE_TYPE(CORE_NS::IResource)
META_INTERFACE_TYPE(CORE_NS::IResourceOptions)
META_INTERFACE_TYPE(CORE_NS::ISetResourceId)
META_INTERFACE_TYPE(META_NS::IDerivedFromTemplate)
META_INTERFACE_TYPE(META_NS::IResourceTemplateAccess)
META_INTERFACE_TYPE(META_NS::IObjectResourceOptions)
META_INTERFACE_TYPE(META_NS::ICollectResources)

#endif
