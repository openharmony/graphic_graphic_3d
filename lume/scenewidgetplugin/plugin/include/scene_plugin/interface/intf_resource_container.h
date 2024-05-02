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
#ifndef INTF_RESOURCE_CONTAINER_H
#define INTF_RESOURCE_CONTAINER_H

#include <scene_plugin/namespace.h>

#include <meta/base/types.h>
#include <meta/interface/intf_container.h>

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IResourceContainer, "48d408d7-92aa-44b1-b80b-85902ace3808")
class IResourceContainer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceContainer, InterfaceId::IResourceContainer)
public:
    /**
     * @brief Get resources in this container.
     * @return A container object for all resources in this instance.
     */
    virtual META_NS::IContainer::Ptr GetResources() = 0;
};
SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IResourceContainer::WeakPtr);
META_TYPE(SCENE_NS::IResourceContainer::Ptr);

#endif // INTF_RESOURCE_CONTAINER_H
