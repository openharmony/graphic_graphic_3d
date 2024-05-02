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

#ifndef META_INTERFACE_PROPERTY_BIND_H
#define META_INTERFACE_PROPERTY_BIND_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IBind, "29495e67-14a6-40aa-a16f-1923630af506")

class IBind : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBind)
public:
    /// getDeps == true to find dependencies, owner can be the property (or null) where this bind is supposed to end up
    /// for checking circular dependencies
    virtual bool SetTarget(const IProperty::ConstPtr& prop, bool getDeps, const IProperty* owner) = 0;
    virtual bool SetTarget(const IFunction::ConstPtr& func, bool getDeps, const IProperty* owner) = 0;
    virtual IFunction::ConstPtr GetTarget() const = 0;

    virtual bool AddDependency(const INotifyOnChange::ConstPtr& dep) = 0;
    virtual bool RemoveDependency(const INotifyOnChange::ConstPtr& dep) = 0;

    virtual BASE_NS::vector<INotifyOnChange::ConstPtr> GetDependencies() const = 0;
};

META_END_NAMESPACE()

#endif
