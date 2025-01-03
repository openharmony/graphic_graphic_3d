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

#ifndef SCENE_EXT_INTF_ENGINE_PROPERTY_INIT_H
#define SCENE_EXT_INTF_ENGINE_PROPERTY_INIT_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

class IEnginePropertyInit : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEnginePropertyInit, "98e0d977-229c-4c0a-bf10-978f042f557c")
public:
    virtual bool AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) = 0;
    virtual bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IEnginePropertyInit)

#endif