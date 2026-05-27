/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef META_INTERFACE_RESOURCE_IRESOURCE_MANAGER_EXTENSION_H
#define META_INTERFACE_RESOURCE_IRESOURCE_MANAGER_EXTENSION_H

#include <core/resources/intf_resource.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/resource/intf_resource.h>

META_BEGIN_NAMESPACE()

struct ResourceData : CORE_NS::ResourceInfo {
    CORE_NS::IResource::Ptr object;
    BASE_NS::string version;
};

class IResourceManagerExtension : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceManagerExtension, "0be25eb9-d89f-46ad-af53-d7c649464463")
public:
    /** Update options data and collect dependency data at the same
     */
    virtual BASE_NS::vector<CORE_NS::ResourceIdContext> UpdateOptionsData(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>&, const CORE_NS::ResourceContextPtr&) = 0;

    virtual BASE_NS::vector<CORE_NS::ResourceIdContext> UpdateOptionsData(
        const BASE_NS::array_view<const CORE_NS::ResourceIdContext>& res) = 0;

    virtual bool ReapplyOptions(const CORE_NS::IResource::Ptr& resource, const CORE_NS::ResourceContextPtr&) = 0;
    /** Get cached resource and info without instantiating anything
     */
    virtual BASE_NS::vector<ResourceData> GetResources(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>&, const CORE_NS::ResourceContextPtr&) const = 0;

    virtual ResourceData GetResource(const CORE_NS::ResourceIdContext&) const = 0;
    /** Add resources with cached resource
     */
    virtual void AddResources(const BASE_NS::array_view<const ResourceData>, const CORE_NS::ResourceContextPtr&) = 0;
    virtual bool AddResource(ResourceData, const CORE_NS::ResourceContextPtr&) = 0;
    virtual bool RemoveGroup(BASE_NS::string_view group, const CORE_NS::ResourceContextWeakPtr&) = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IResourceManagerExtension)

#endif
