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

#ifndef META_INTERFACE_FUNCTION_H
#define META_INTERFACE_FUNCTION_H

#include <core/plugin/intf_interface.h>

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_call_context.h>
#include <meta/interface/intf_callable.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IFunction, "a25fc0e3-4d2c-4005-bd5b-13ed647114ca")

/**
 * @brief Interface for named functions
 */
class IFunction : public ICallable {
    META_INTERFACE(ICallable, IFunction)
public:
    /**
     * @brief Name of this callable entity
     * @return Name of this entity.
     */
    virtual BASE_NS::string GetName() const = 0;

    /**
     * @brief Destination object for which this function is called for.
     * @return Destination object if any (and alive).
     */
    virtual IObject::ConstPtr GetDestination() const = 0;

    /**
     * @brief Invoke the function
     * @param context Needs to contain arguments for the call and the return value will be stored here.
     */
    virtual void Invoke(const ICallContext::Ptr& context) const = 0;

    /**
     * @brief Create default call context for this function, which contains the parameter names and types.
     * @return Call context.
     */
    virtual ICallContext::Ptr CreateCallContext() const = 0;
};

/**
 * @brief IFunction for which the target object and function-name can be set manually.
 */
class ISettableFunction : public IFunction {
    META_INTERFACE(IFunction, ISettableFunction, "4437e8d2-e41f-48a0-8880-b2636c926b21")
public:
    /**
     * @brief Set the target object and function name.
     */
    virtual bool SetTarget(const IObject::Ptr& obj, BASE_NS::string_view name) = 0;
};

/**
 * @brief IFunction which returns the value of given property.
 */
class IPropertyFunction : public IFunction {
    META_INTERFACE(IFunction, IPropertyFunction, "2f5491bf-881e-449e-8aab-78b137dee0d3")
public:
    /**
     * @brief Set the target property.
     */
    virtual bool SetTarget(const IProperty::ConstPtr& prop) = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IFunction)

#endif
