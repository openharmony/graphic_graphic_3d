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

#ifndef META_INTERFACE_CALL_CONTEXT_H
#define META_INTERFACE_CALL_CONTEXT_H

#include <base/containers/string.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

struct ArgumentNameValue {
    BASE_NS::string name;
    IAny::Ptr value;
};

/**
 * @brief Interface that contains parameter names and types, arguments and result type and value.
 *        It is passed through when calling meta functions to provide arguments and other call context.
 *        See meta/api/call_context.h for typed helper functions.
 */
class ICallContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICallContext, "2e9cac45-0e61-4152-8b2a-bc1c65fded3d")
public:
    /**
     * @brief Define parameter name, type and default value. The parameters are added in order.
     * @param name Name of the parameter, fails if parameter already defined for same name.
     * @param value Type and default value for the parameter.
     * @return True on success
     */
    virtual bool DefineParameter(BASE_NS::string_view name, const IAny::Ptr& value) = 0;

    /**
     * @brief Set argument value for parameter 'name'.
     * @param name Name of the parameter, parameter must have been defined with the same name.
     * @param value Value of the argument, the type must match with the define parameter type.
     * @return True on success
     */
    virtual bool Set(BASE_NS::string_view name, const IAny& value) = 0;

    /**
     * @brief Get argument value for parameter 'name'
     * @param name Name of the parameter
     * @return Any containing the argument value or nullptr if no such parameter
     */
    virtual IAny::Ptr Get(BASE_NS::string_view name) const = 0;

    /**
     * @brief List all parameters, these should be in the function parameter order (left to right).
     * @return List of parameters/argument values.
     */
    virtual BASE_NS::array_view<const ArgumentNameValue> GetParameters() const = 0;

    /**
     * @brief Check if the call was successful.
     * @return True if SetResult was called with correct result type.
     */
    virtual bool Succeeded() const = 0;

    /**
     * @brief Define result type, null means void which is also the default.
     * @param value Type of the result value, the value itself is overwritten by SetResult when setting result value.
     * @return True on success.
     */
    virtual bool DefineResult(const IAny::Ptr& value) = 0;

    /**
     * @brief Set result value
     * @param value Return value, must match the defined result type.
     * @return True on success
     */
    virtual bool SetResult(const IAny& value) = 0;

    /**
     * @brief Set result value for void function
     * @return True on success
     */
    virtual bool SetResult() = 0;

    /**
     * @brief Get result value
     * @return Any containing the result value or nullptr if void result value (ie. no result).
     */
    virtual IAny::Ptr GetResult() const = 0;

    /**
     * @brief Reset internal state, so that the same context can be used again for calling.
     * @note  This does not change defined parameters or return type, just resets arguments/call result.
     */
    virtual void Reset() = 0;

    /**
     * @brief Report error in the context of the meta call (default implementation logs to console)
     */
    virtual void ReportError(BASE_NS::string_view error) = 0;
};

META_END_NAMESPACE()

META_TYPE(META_NS::ICallContext::Ptr)
META_TYPE(META_NS::ICallContext::ConstPtr)
META_TYPE(META_NS::ICallContext::WeakPtr)
META_TYPE(META_NS::ICallContext::ConstWeakPtr)

#endif
