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
#include "stack_property.h"

#include <meta/api/util.h>
#include <meta/base/interface_utils.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/property/intf_stack_resetable.h>

#include "../any.h"
#include "dependencies.h"

META_BEGIN_NAMESPACE()
namespace Internal {

StackProperty::StackProperty(BASE_NS::string name)
    : Super(BASE_NS::move(name)), onChangedCallback_(MakeCallback<IOnChanged>([this] { InternalOnChanged(); }))
{}

StackProperty::~StackProperty()
{
    CleanUp();
}

void StackProperty::CleanUp()
{
    if (auto i = interface_cast<INotifyOnChange>(defaultValue_)) {
        i->OnChanged()->RemoveHandler(uintptr_t(this));
    }
    for (auto&& m : modifiers_) {
        if (auto i = interface_cast<INotifyOnChange>(m)) {
            i->OnChanged()->RemoveHandler(uintptr_t(this));
        }
    }
    modifiers_.clear();
    for (auto&& v : values_) {
        if (auto i = interface_cast<INotifyOnChange>(v)) {
            i->OnChanged()->RemoveHandler(uintptr_t(this));
        }
    }
    values_.clear();
}

AnyReturnValue StackProperty::SetValueInValueStack(const IAny& value)
{
    AnyReturnValue res = AnyReturn::FAIL;
    // find first value that accepts the new value, all non-accepting values are removed
    for (int i = int(values_.size()) - 1; i >= 0; --i) {
        {
            InterfaceUniqueLock lock { values_[i] };
            res = values_[i]->SetValue(value);
            if (res) {
                break;
            }
        }
        if (auto noti = interface_cast<INotifyOnChange>(values_[i])) {
            noti->OnChanged()->RemoveHandler(uintptr_t(this));
        }
        values_.pop_back();
    }
    // if there was no any to set the new value, create one
    if (!res) {
        if (auto c = interface_pointer_cast<IValue>(value.Clone(true))) {
            if (auto i = interface_cast<INotifyOnChange>(c)) {
                i->OnChanged()->AddHandler(onChangedCallback_, uintptr_t(this));
            }
            values_.push_back(c);
            res = AnyReturn::SUCCESS;
        } else {
            res = AnyReturn::INCOMPATIBLE_TYPE;
        }
    }
    return res;
}

AnyReturnValue StackProperty::SetValueToStack(const IAny::Ptr& internal)
{
    // first go through modifiers and let them alter the value
    for (auto it = modifiers_.rbegin(); it != modifiers_.rend(); ++it) {
        auto m = (*it)->ProcessOnSet(*internal, GetValueFromStack());
        if (m & EVAL_ERROR) {
            return AnyReturn::FAIL;
        }
        if (m & EVAL_RETURN) {
            if (m & EVAL_VALUE_CHANGED) {
                NotifyChange();
            }
            return AnyReturn::SUCCESS;
        }
    }

    auto res = SetValueInValueStack(*internal);
    if (res != AnyReturn::NOTHING_TO_DO) {
        NotifyChange();
    }
    return res;
}
AnyReturnValue StackProperty::SetValue(const IAny& value)
{
    if (evaluating_) {
        CORE_LOG_E("Recursive property evaluation requested [property name=%s]", GetName().c_str());
        return AnyReturn::RECURSIVE_CALL;
    }
    evaluating_ = true;

    AnyReturnValue res = AnyReturn::SUCCESS;

    auto v = GetData();
    if (!v) {
        CORE_LOG_D("Initializing internal any with SetValue");
        res = SetInternalAny(value.Clone(false));
        if (res) {
            v = GetData();
        }
    }
    if (res) {
        CORE_ASSERT(v);
        res = SetInternalValue(value);
        if (res) {
            res = SetValueToStack(v);
        }
    }

    evaluating_ = false;
    return res;
}
const IAny& StackProperty::GetValueFromStack() const
{
    if (requiresEvaluation_) {
        AnyReturnValue res = AnyReturn::FAIL;
        if (values_.empty()) {
            res = currentValue_->CopyFrom(*defaultValue_);
        } else {
            auto& v = values_.back();
            InterfaceSharedLock lock { v };
            res = currentValue_->CopyFrom(v->GetValue());
        }
        if (!res) {
            CORE_LOG_E("Invalid value in stack, could not copy from [property name=%s]", GetName().c_str());
            return INVALID_ANY;
        }
        for (auto&& m : modifiers_) {
            auto mres = m->ProcessOnGet(*currentValue_);
            if (mres & EVAL_RETURN) {
                break;
            }
        }
        requiresEvaluation_ = false;
    }
    return *currentValue_;
}
const IAny& StackProperty::RawGetValue() const
{
    if (evaluating_) {
        CORE_LOG_E("Recursive property evaluation requested [property name=%s]", GetName().c_str());
        return INVALID_ANY;
    }
    if (!currentValue_) {
        CORE_LOG_E("GetValue called for not initialized property [property name=%s]", GetName().c_str());
        return INVALID_ANY;
    }
    evaluating_ = true;
    const IAny& res = GetValueFromStack();
    evaluating_ = false;
    return res;
}

const IAny& StackProperty::GetValue() const
{
    bool isActive = false;
    if constexpr (ENABLE_DEPENDENCY_CHECK) {
        auto& d = GetDeps();
        if (d.IsActive()) {
            if (!d.AddDependency(self_.lock())) {
                return INVALID_ANY;
            }
            // force evaluation since we are checking dependencies
            requiresEvaluation_ = true;
            isActive = true;
            d.Start();
        }
    }
    const IAny& res = RawGetValue();
    if constexpr (ENABLE_DEPENDENCY_CHECK) {
        if (isActive) {
            GetDeps().End();
        }
    }
    return res;
}

ReturnError StackProperty::PushValue(const IValue::Ptr& value)
{
    auto& internal = GetData();
    if (!internal || !value || !IsValueGetCompatible(*internal, *value)) {
        CORE_LOG_W("Incompatible value");
        return GenericError::INCOMPATIBLE_TYPES;
    }
    values_.push_back(value);
    // if it is property, see that there is no circular dependency
    if (interface_cast<IProperty>(value)) {
        requiresEvaluation_ = true;
        if (!RawGetValue().GetTypeId().IsValid()) {
            values_.pop_back();
            return GenericError::RECURSIVE_CALL;
        }
    }
    if (auto i = interface_cast<INotifyOnChange>(value)) {
        i->OnChanged()->AddHandler(onChangedCallback_, uintptr_t(this));
    }
    NotifyChange();
    return GenericError::SUCCESS;
}
ReturnError StackProperty::PopValue()
{
    if (!values_.empty()) {
        if (auto i = interface_cast<INotifyOnChange>(values_.back())) {
            i->OnChanged()->RemoveHandler(uintptr_t(this));
        }
        values_.pop_back();
        NotifyChange();
        return GenericError::SUCCESS;
    }
    return GenericError::NOT_FOUND;
}
IValue::Ptr StackProperty::TopValue() const
{
    return !values_.empty() ? values_.back() : nullptr;
}
ReturnError StackProperty::RemoveValue(const IValue::Ptr& value)
{
    size_t index = 0;
    for (auto m = values_.rbegin(); m != values_.rend(); ++m, ++index) {
        if (*m == value) {
            if (auto i = interface_cast<INotifyOnChange>(*m)) {
                i->OnChanged()->RemoveHandler(uintptr_t(this));
            }
            values_.erase(m.base() - 1);
            // notify if the top-most value was removed (i.e. pop value)
            if (index == 0) {
                NotifyChange();
            }
            return GenericError::SUCCESS;
        }
    }

    return GenericError::NOT_FOUND;
}
BASE_NS::vector<IValue::Ptr> StackProperty::GetValues(const BASE_NS::array_view<const TypeId>& ids, bool strict) const
{
    BASE_NS::vector<IValue::Ptr> ret;
    for (auto&& v : values_) {
        if (CheckInterfaces(interface_pointer_cast<CORE_NS::IInterface>(v), ids, strict)) {
            ret.push_back(v);
        }
    }
    return ret;
}
ReturnError StackProperty::InsertModifier(IndexType pos, const IModifier::Ptr& mod)
{
    auto& internal = GetData();
    if (!internal || !mod || !IsModifierGetCompatible(*internal, *mod)) {
        CORE_LOG_W("Incompatible modifier");
        return GenericError::INCOMPATIBLE_TYPES;
    }
    if (auto i = interface_cast<INotifyOnChange>(mod)) {
        i->OnChanged()->AddHandler(onChangedCallback_, uintptr_t(this));
    }
    IndexType i = pos < modifiers_.size() ? pos : modifiers_.size();
    modifiers_.insert(modifiers_.begin() + i, mod);
    NotifyChange();
    return GenericError::SUCCESS;
}
IModifier::Ptr StackProperty::RemoveModifier(IndexType pos)
{
    IModifier::Ptr p;
    if (pos < modifiers_.size()) {
        p = modifiers_[pos];
        modifiers_.erase(modifiers_.begin() + pos);
        if (auto i = interface_cast<INotifyOnChange>(p)) {
            i->OnChanged()->RemoveHandler(uintptr_t(this));
        }
        NotifyChange();
    }
    return p;
}
ReturnError StackProperty::RemoveModifier(const IModifier::Ptr& mod)
{
    for (auto m = modifiers_.begin(); m != modifiers_.end(); ++m) {
        if (*m == mod) {
            if (auto i = interface_cast<INotifyOnChange>(*m)) {
                i->OnChanged()->RemoveHandler(uintptr_t(this));
            }
            modifiers_.erase(m);
            NotifyChange();
            return GenericError::SUCCESS;
        }
    }

    return GenericError::NOT_FOUND;
}

BASE_NS::vector<IModifier::Ptr> StackProperty::GetModifiers(
    const BASE_NS::array_view<const TypeId>& ids, bool strict) const
{
    BASE_NS::vector<IModifier::Ptr> ret;
    for (auto&& m : modifiers_) {
        if (CheckInterfaces(interface_pointer_cast<CORE_NS::IInterface>(m), ids, strict)) {
            ret.push_back(m);
        }
    }
    return ret;
}
AnyReturnValue StackProperty::SetDefaultValue(const IAny& value)
{
    CORE_ASSERT_MSG(defaultValue_, "SetInternalAny not called");
    AnyReturnValue res = defaultValue_->CopyFrom(value);
    if (res && values_.empty()) {
        NotifyChange();
    }
    return res;
}
const IAny& StackProperty::GetDefaultValue() const
{
    CORE_ASSERT_MSG(defaultValue_, "SetInternalAny not called");
    return *defaultValue_;
}
AnyReturnValue StackProperty::SetInternalAny(IAny::Ptr any)
{
    auto res = Super::SetInternalAny(any);
    if (res) {
        defaultValue_ = any->Clone(false);
        if (auto i = interface_cast<INotifyOnChange>(defaultValue_)) {
            i->OnChanged()->AddHandler(onChangedCallback_, uintptr_t(this));
        }
        currentValue_ = defaultValue_->Clone(true);
        requiresEvaluation_ = true;
        values_.clear();
        modifiers_.clear();
    }
    return res;
}
void StackProperty::NotifyChange() const
{
    requiresEvaluation_ = true;
    CallOnChanged();
}
template<typename Vec>
bool StackProperty::ProcessResetables(Vec& vec)
{
    for (int i = int(vec.size()) - 1; i >= 0; --i) {
        ResetResult res = ResetResult::RESET_REMOVE_ME;
        if (auto rable = interface_cast<IStackResetable>(vec[i])) {
            res = rable->ProcessOnReset(*defaultValue_);
        }
        if (res & RESET_REMOVE_ME) {
            vec.erase(vec.begin() + i);
        }
        if (res & RESET_STOP) {
            return false;
        }
    }
    return true;
}
void StackProperty::ResetValue()
{
    if (ProcessResetables(modifiers_)) {
        ProcessResetables(values_);
    }
    // reset the currentValue_ and internal value so we don't accidentally keep any shared_ptrs alive
    currentValue_ = defaultValue_->Clone(false);
    SetInternalValue(*currentValue_);
    NotifyChange();
}
void StackProperty::RemoveAll()
{
    values_.clear();
    modifiers_.clear();
    // reset the currentValue_ and internal value so we don't accidentally keep any shared_ptrs alive
    currentValue_ = defaultValue_->Clone(false);
    SetInternalValue(*currentValue_);
    NotifyChange();
}
ReturnError StackProperty::Export(IExportContext& c) const
{
    return Serializer(c) & NamedValue("defaultValue", defaultValue_) & NamedValue("values", values_) &
           NamedValue("modifiers", modifiers_);
}
ReturnError StackProperty::Import(IImportContext& c)
{
    CleanUp();
    BASE_NS::vector<SharedPtrIInterface> values;
    BASE_NS::vector<SharedPtrIInterface> modifiers;
    Serializer ser(c);
    ser& NamedValue("defaultValue", defaultValue_) & NamedValue("values", values) & NamedValue("modifiers", modifiers);
    if (ser) {
        if (!defaultValue_) {
            return GenericError::FAIL;
        }
        if (auto i = interface_cast<INotifyOnChange>(defaultValue_)) {
            i->OnChanged()->AddHandler(onChangedCallback_, uintptr_t(this));
        }
        if (auto res = Super::SetInternalAny(defaultValue_->Clone(false))) {
            currentValue_ = defaultValue_->Clone(true);
            requiresEvaluation_ = true;
        }
        for (auto&& i : values) {
            if (auto v = interface_pointer_cast<IValue>(i)) {
                if (auto i = interface_cast<INotifyOnChange>(v)) {
                    i->OnChanged()->AddHandler(onChangedCallback_, uintptr_t(this));
                }
                values_.push_back(BASE_NS::move(v));
            }
        }
        for (auto&& i : modifiers) {
            if (auto v = interface_pointer_cast<IModifier>(i)) {
                if (auto i = interface_cast<INotifyOnChange>(v)) {
                    i->OnChanged()->AddHandler(onChangedCallback_, uintptr_t(this));
                }
                modifiers_.push_back(BASE_NS::move(v));
            }
        }
    }
    return ser;
}

bool StackProperty::IsDefaultValue() const
{
    return values_.empty();
}

} // namespace Internal
META_END_NAMESPACE()
