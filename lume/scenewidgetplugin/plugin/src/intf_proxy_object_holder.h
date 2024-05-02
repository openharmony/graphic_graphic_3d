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
#ifndef SCENEPLUGIN_INTF_PROXY_OBJECT_HOLDER_H
#define SCENEPLUGIN_INTF_PROXY_OBJECT_HOLDER_H
#include <scene_plugin/namespace.h>

#include <meta/base/types.h>

#include "PropertyHandlerArrayHolder.h"
SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IProxyObjectHolder, "945005d1-8afc-46eb-adc1-52b8834eeb28")
class IProxyObjectHolder : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IProxyObjectHolder, InterfaceId::IProxyObjectHolder)
public:
    virtual PropertyHandlerArrayHolder& Holder() = 0;
};
SCENE_END_NAMESPACE()
#endif
