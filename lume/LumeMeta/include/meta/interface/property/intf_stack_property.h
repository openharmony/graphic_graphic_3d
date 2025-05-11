/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Stack Property interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
 */

#ifndef META_INTERFACE_STACK_PROPERTY_H
#define META_INTERFACE_STACK_PROPERTY_H

#include <meta/base/bit_field.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_value.h>
#include <meta/interface/property/intf_modifier.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IStackProperty, "aa6ab1db-f971-49fd-a862-05fba1dc5761")

/**
 * @brief Interface for property that has evaluation stack
 *
 * There are two separate occasions when the stack is evaluated. First is when value is
 * requested from the property (GetValue) and the second is when value is set for the property (SetValue).
 *
 * The property has two stacks, one for values and one for modifiers. When evaluated for GetValue, the top-most
 * value is taken and then ran over the modifiers (from bottom to top order).
 *
 * When the SetValue is evaluated, the modifiers are ran over the given value (from top to bottom order) and then
 * the altered value is set for top most value in stack that accepts it. The values in stack that do not accept
 * the new value are removed (e.g. bind). If in the end the stack is empty, the value is cloned and placed into
 * the value stack.
 */
class IStackProperty : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStackProperty)
public:
    // Value functions
    /// Push value to the top of values stack
    virtual ReturnError PushValue(const IValue::Ptr& value) = 0;
    /// Remote value from the top of the values stack
    virtual ReturnError PopValue() = 0;
    /// Get value at the top of the values stack if any
    virtual IValue::Ptr TopValue() const = 0;
    /// Remove specific value object from the values stack
    virtual ReturnError RemoveValue(const IValue::Ptr& value) = 0;
    /// Get the values stack with possible restrictions applied as given
    virtual BASE_NS::vector<IValue::Ptr> GetValues(const BASE_NS::array_view<const TypeId>& ids, bool strict) const = 0;

    // Modifier functions
    using IndexType = size_t;
    /// Insert modifier to given location in modifiers stack
    virtual ReturnError InsertModifier(IndexType pos, const IModifier::Ptr& mod) = 0;
    /// Remove modifier at given location on modifiers stack
    virtual IModifier::Ptr RemoveModifier(IndexType pos) = 0;
    /// Remove specific modifier object from the modifier stack
    virtual ReturnError RemoveModifier(const IModifier::Ptr& mod) = 0;
    /// Get the modifiers stack with possible restrictions applied as given
    virtual BASE_NS::vector<IModifier::Ptr> GetModifiers(
        const BASE_NS::array_view<const TypeId>& ids, bool strict) const = 0;

    /// Add modifier to the top of the modifiers stack
    ReturnError AddModifier(const IModifier::Ptr& mod)
    {
        return InsertModifier(-1, mod);
    }

    /// Remove all values and modifiers from both stacks
    virtual void RemoveAll() = 0;

    /// Set default value of this property
    virtual AnyReturnValue SetDefaultValue(const IAny& value) = 0;
    /// Get default value of this property
    virtual const IAny& GetDefaultValue() const = 0;
    /// Evaluate property value (as if calling GetValue) and store the result to top most value.
    virtual void EvaluateAndStore() = 0;
};

META_REGISTER_CLASS(StackProperty, "14366762-0c46-4225-8bfc-1240aba457c6", ObjectCategoryBits::NO_CATEGORY)

META_END_NAMESPACE()

#endif
