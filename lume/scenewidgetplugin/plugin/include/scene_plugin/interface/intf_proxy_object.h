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
#ifndef SCENEPLUGIN_INTF_PROXY_OBJECT_H
#define SCENEPLUGIN_INTF_PROXY_OBJECT_H
#include <scene_plugin/namespace.h>

#include <meta/base/types.h>
#include <meta/interface/property/intf_property.h>
SCENE_BEGIN_NAMESPACE()
REGISTER_INTERFACE(IProxyObject, "9ed03ed6-7742-4512-9876-534038f3eac9")
class IProxyObject : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IProxyObject, InterfaceId::IProxyObject)
public:
    struct PropertyPair {
        META_NS::IProperty::ConstPtr first;
        META_NS::IProperty::ConstPtr second;
    };

    /**
     * @brief List all properties bound to 3D system. The first element points to proxy property, and the second to the
     * one that access 3D components.
     * @return List of all properties proxied by this object.
     */
    virtual BASE_NS::vector<PropertyPair> ListBoundProperties() const = 0;
};
SCENE_END_NAMESPACE()
#endif