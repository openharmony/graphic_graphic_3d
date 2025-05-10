/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Stack Property value/modifier resetable
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
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

/**
 * @brief Interface to control what resetting property value does
 *
 * When ResetValue for property is called, the values and modifiers that has implemented with interface
 * can alter the result. This allows to have sticky values or modifiers which are not removed when resetting.
 */
class IStackResetable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStackResetable)
public:
    /**
     * @brief Function that is evaluated on ResetValue of property
     * @param defaultValue The default value of the property
     * @return Result of the evaluation
     */
    virtual ResetResult ProcessOnReset(const IAny& defaultValue) = 0;
};

META_INTERFACE_TYPE(META_NS::IStackResetable)

META_END_NAMESPACE()

#endif
