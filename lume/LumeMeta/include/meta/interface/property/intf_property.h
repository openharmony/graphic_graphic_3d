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

#ifndef META_INTERFACE_PROPERTY_H
#define META_INTERFACE_PROPERTY_H

#include <meta/base/expected.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_notify_on_change.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_flags.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IProperty, "772086d4-b7eb-437e-b71b-83353caaffaf")

// Notice that IProperty does not guarantee thread safety, one should use one of the wrapper
// types to access the members to guarantee proper locking,

class IProperty : public INotifyOnChange {
    META_INTERFACE(INotifyOnChange, IProperty)
public:
    virtual BASE_NS::string GetName() const = 0;
    virtual IObject::WeakPtr GetOwner() const = 0;

    virtual AnyReturnValue SetValue(const IAny& value) = 0;
    virtual const IAny& GetValue() const = 0;
    virtual bool IsCompatible(const TypeId& id) const = 0;

    virtual TypeId GetTypeId() const = 0;
    // Invoke the OnChanged handlers
    virtual void NotifyChange() const = 0;

    virtual void ResetValue() = 0;
    virtual bool IsDefaultValue() const = 0;

    bool IsValueSet() const
    {
        return !IsDefaultValue();
    }
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IProperty)

#endif
