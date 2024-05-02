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
#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/interface/intf_proxy_object.h>

#include <meta/ext/object.h>

#include "intf_proxy_object_holder.h"

namespace {
class PropertyHolderImpl : public META_NS::ObjectFwd<PropertyHolderImpl, SCENE_NS::ClassId::CustomPropertiesHolder,
                               META_NS::ClassId::Object, SCENE_NS::IProxyObject, SCENE_NS::IProxyObjectHolder> {
public: // IProxyObject
    BASE_NS::vector<PropertyPair> ListBoundProperties() const override
    {
        return holder_.GetBoundProperties();
    }

public: // IProxyObjectHolder
    PropertyHandlerArrayHolder& Holder() override
    {
        return holder_;
    }

    PropertyHandlerArrayHolder holder_;
};
} // namespace
SCENE_BEGIN_NAMESPACE()
void RegisterPropertyHolderImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<PropertyHolderImpl>();
}
void UnregisterPropertyHolderImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<PropertyHolderImpl>();
}

SCENE_END_NAMESPACE()
