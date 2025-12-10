/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef META_INTERFACE_RESOURCE_IRESOURCE_QUERY_H
#define META_INTERFACE_RESOURCE_IRESOURCE_QUERY_H

#include <core/resources/intf_resource.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

class IResourceQuery : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceQuery, "712c2161-0885-47b2-9547-8ff3d0eb0767")
public:
    virtual uint32_t GetAliveCount(const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection) const = 0;
    virtual BASE_NS::vector<CORE_NS::IResource::Ptr> FindAliveResources(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection) const = 0;
};

META_INTERFACE_TYPE(META_NS::IResourceQuery)

META_END_NAMESPACE()

#endif
