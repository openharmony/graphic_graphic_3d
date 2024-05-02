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

#ifndef META_ENGINE_INTERFACE_ENGINE_DATA_H
#define META_ENGINE_INTERFACE_ENGINE_DATA_H

#include <meta/interface/engine/intf_engine_value.h>

META_BEGIN_NAMESPACE()

class IEngineData : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineData, "542b7e13-21bd-4c9c-8d3f-a9527732216c")
public:
    virtual IEngineInternalValueAccess::Ptr GetInternalValueAccess(const CORE_NS::PropertyTypeDecl& type) const = 0;
    virtual void RegisterInternalValueAccess(
        const CORE_NS::PropertyTypeDecl& type, IEngineInternalValueAccess::Ptr) = 0;
    virtual void UnregisterInternalValueAccess(const CORE_NS::PropertyTypeDecl& type) = 0;
};

META_END_NAMESPACE()

#endif
