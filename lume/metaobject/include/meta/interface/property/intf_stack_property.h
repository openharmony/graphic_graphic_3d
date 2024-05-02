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

class IStackProperty : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStackProperty)
public:
    // Value functions
    virtual ReturnError PushValue(const IValue::Ptr& value) = 0;
    virtual ReturnError PopValue() = 0;
    virtual IValue::Ptr TopValue() const = 0;
    virtual ReturnError RemoveValue(const IValue::Ptr& value) = 0;
    virtual BASE_NS::vector<IValue::Ptr> GetValues(const BASE_NS::array_view<const TypeId>& ids, bool strict) const = 0;

    // Modifier functions
    using IndexType = size_t;
    virtual ReturnError InsertModifier(IndexType pos, const IModifier::Ptr& mod) = 0;
    virtual IModifier::Ptr RemoveModifier(IndexType pos) = 0;
    virtual ReturnError RemoveModifier(const IModifier::Ptr& mod) = 0;
    virtual BASE_NS::vector<IModifier::Ptr> GetModifiers(
        const BASE_NS::array_view<const TypeId>& ids, bool strict) const = 0;

    ReturnError AddModifier(const IModifier::Ptr& mod)
    {
        return InsertModifier(-1, mod);
    }

    // Remove all values and modifiers from the stack
    virtual void RemoveAll() = 0;

    // Default values
    virtual AnyReturnValue SetDefaultValue(const IAny& value) = 0;
    virtual const IAny& GetDefaultValue() const = 0;
};

REGISTER_CLASS(StackProperty, "14366762-0c46-4225-8bfc-1240aba457c6", ObjectCategoryBits::NO_CATEGORY)

META_END_NAMESPACE()

#endif
