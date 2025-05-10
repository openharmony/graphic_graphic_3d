/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Property Mod interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
 */

#ifndef META_INTERFACE_PROPERTY_MODIFIER_H
#define META_INTERFACE_PROPERTY_MODIFIER_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

/// Result the modifier evaluation functions return
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

/**
 * @brief Modifier for property to allow to alter the property's behaviour on evaluation
 *
 * Modifiers can be added to property. The ProcessOnGet is called when GetValue is called
 * for the property. The ProcessOnSet is called when SetValue is called for the property.
 * In both cases the stack is evaluated and the modifiers are called in the order they are
 * set for the property (for get from first to last and for set from last to first).
 * The evaluation functions can alter the value being get/set and/or control the further
 * evaluation process by terminating it, short circuiting it or simply continuing.
 */
class IModifier : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IModifier)
public:
    /**
     * @brief Evaluation function for GetValue calls
     * @param value The top most value in property stack (possibly after other modifiers have altered it).
     * @return Evaluation result which decides the action for further modifier evaluation
     */
    virtual EvaluationResult ProcessOnGet(IAny& value) = 0;
    /**
     * @brief Evaluation function for SetValue calls
     * @param value The value being set for the property
     * @return Evaluation result which decides the action for further modifier evaluation and setting the value
     */
    virtual EvaluationResult ProcessOnSet(IAny& value, const IAny& current) = 0;
    /// Check if this modifier is compatible with given type id
    virtual bool IsCompatible(const TypeId& id) const = 0;
};

META_INTERFACE_TYPE(META_NS::IModifier)

META_END_NAMESPACE()

#endif
