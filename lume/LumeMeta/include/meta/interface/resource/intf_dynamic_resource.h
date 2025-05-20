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

#ifndef META_INTERFACE_RESOURCE_IDYNAMIC_RESOURCE_H
#define META_INTERFACE_RESOURCE_IDYNAMIC_RESOURCE_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The IDynamicResource interface defines an interface for changeable resources
 */
class IDynamicResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDynamicResource, "38e11d91-0c5a-41fb-bf0c-d62c6d61d9f5")
public:
    /**
     * @brief Resource changed
     */
    META_EVENT(META_NS::IOnChanged, OnResourceChanged)
};

class IReloadableResource : public IDynamicResource {
    META_INTERFACE(IDynamicResource, IReloadableResource, "924efc47-63a8-4a25-ab7d-bacacf27eee0")
public:
    /**
     * @brief Manually refresh the resource. May lead to OnResourceChanged being invoked.
     */
    virtual void Refresh() = 0;
};

META_INTERFACE_TYPE(META_NS::IDynamicResource)

META_END_NAMESPACE()

#endif
