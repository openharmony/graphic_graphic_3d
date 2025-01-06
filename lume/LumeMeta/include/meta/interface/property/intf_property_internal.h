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

#ifndef META_INTERFACE_PROPERTY_INTERNAL_H
#define META_INTERFACE_PROPERTY_INTERNAL_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IPropertyInternal, "1757f95f-07ab-4d2a-af53-5fd6b2f3048a")
META_REGISTER_INTERFACE(IPropertyInternalAny, "65c7309e-21ee-4087-af33-76b297ab4f93")

class IPropertyInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyInternal)
public:
    virtual void SetOwner(IObject::Ptr owner) = 0;
    virtual void SetSelf(IProperty::Ptr self) = 0;
};

class IPropertyInternalAny : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyInternalAny)
public:
    virtual AnyReturnValue SetInternalAny(IAny::Ptr any) = 0;
    virtual IAny::Ptr GetInternalAny() const = 0;
};

META_END_NAMESPACE()

#endif
