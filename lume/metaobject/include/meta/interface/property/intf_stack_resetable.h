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

#ifndef META_INTERFACE_PROPERTY_STACK_RESETABLE_H
#define META_INTERFACE_PROPERTY_STACK_RESETABLE_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_function.h>

META_BEGIN_NAMESPACE()

enum ResetResult : uint8_t {
    RESET_CONTINUE = 0,  /// Continue evaluation of the stack (implied if no RESET_STOP)
    RESET_STOP = 1,      /// Stop evaluating resets
    RESET_REMOVE_ME = 2, /// Remove
};

inline ResetResult operator|(ResetResult l, ResetResult r)
{
    return ResetResult(uint8_t(l) | uint8_t(r));
}

META_REGISTER_INTERFACE(IStackResetable, "b3f9bdc6-6b6f-43ff-b727-86f21e4e0c8d")

class IStackResetable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStackResetable)
public:
    virtual ResetResult ProcessOnReset(const IAny& defaultValue) = 0;
};

META_INTERFACE_TYPE(IStackResetable);

META_END_NAMESPACE()

#endif
