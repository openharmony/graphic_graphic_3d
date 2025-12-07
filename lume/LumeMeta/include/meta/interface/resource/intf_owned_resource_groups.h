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

#ifndef META_INTERFACE_RESOURCE_IOWNED_RESOURCE_GROUPS_H
#define META_INTERFACE_RESOURCE_IOWNED_RESOURCE_GROUPS_H

#include <core/resources/intf_resource.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/resource/intf_resource_group_handle.h>

META_BEGIN_NAMESPACE()

class IOwnedResourceGroups : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IOwnedResourceGroups, "b56de526-dc5a-40a6-bd26-d0db842fbbdd")
public:
    virtual IResourceGroupHandle::Ptr GetGroupHandle(BASE_NS::string_view group) = 0;
};

META_INTERFACE_TYPE(META_NS::IOwnedResourceGroups)

META_END_NAMESPACE()

#endif
