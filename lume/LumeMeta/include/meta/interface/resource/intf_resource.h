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

class IDerivedFromResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDerivedFromResource, "ea586292-8c95-482e-af6c-a0611818bbcb")
public:
    virtual bool SetResource(const CORE_NS::IResource::Ptr&) = 0;
    virtual CORE_NS::ResourceId GetResource() const = 0;
    virtual CORE_NS::IResource::Ptr CreateResource() const = 0;
};

class IObjectResourceOptions : public CORE_NS::IResourceOptions {
    META_INTERFACE(CORE_NS::IResourceOptions, IObjectResourceOptions, "3a8559b2-ad80-48f5-8e3b-3340c9048f77")
public:
    virtual void SetBaseResource(CORE_NS::ResourceId) = 0;
    virtual CORE_NS::ResourceId GetBaseResource() const = 0;

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

META_REGISTER_CLASS(
    ObjectResourceOptions, "196213b6-eb63-4a29-89be-1ec10ed68596", META_NS::ObjectCategoryBits::NO_CATEGORY)

META_TYPE(CORE_NS::ResourceId)
META_INTERFACE_TYPE(CORE_NS::IResource)
META_INTERFACE_TYPE(CORE_NS::ISetResourceId)
META_INTERFACE_TYPE(CORE_NS::IResourceOptions)
META_INTERFACE_TYPE(META_NS::IDerivedFromResource)
META_INTERFACE_TYPE(META_NS::IObjectResourceOptions)

META_END_NAMESPACE()

#endif
