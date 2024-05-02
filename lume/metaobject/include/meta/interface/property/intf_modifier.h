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

#ifndef META_INTERFACE_PROPERTY_MODIFIER_H
#define META_INTERFACE_PROPERTY_MODIFIER_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

enum EvaluationResult : uint8_t {
    EVAL_CONTINUE = 0,      /// Continue evaluation of the stack
    EVAL_ERROR = 1,         /// Error happened
    EVAL_RETURN = 2,        /// Stop stack evaluation and return value
    EVAL_VALUE_CHANGED = 4, /// This modifier changed the value
};

inline EvaluationResult operator|(EvaluationResult l, EvaluationResult r)
{
    return EvaluationResult(uint8_t(l) | uint8_t(r));
}

META_REGISTER_INTERFACE(IModifier, "4a6991b7-c410-4663-99d2-4ff00eb9a9e0")

class IModifier : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IModifier)
public:
    virtual EvaluationResult ProcessOnGet(IAny& value) = 0;
    virtual EvaluationResult ProcessOnSet(IAny& value, const IAny& current) = 0;
    virtual bool IsCompatible(const TypeId& id) const = 0;
};

META_INTERFACE_TYPE(IModifier);

META_END_NAMESPACE()

#endif
