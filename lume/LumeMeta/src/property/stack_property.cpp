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
#include <meta/interface/intf_notify_on_change.h>
#include <meta/interface/property/intf_stack_resetable.h>

#include "../any.h"
#include "dependencies.h"

META_BEGIN_NAMESPACE()
namespace Internal {

namespace {

template<typename T>
void SubscribeOnChanged(const T& obj, StackProperty& self, const ICallable::Ptr& legacyCb)
{
    if (auto d = interface_cast<INotifyOnChangeDirect>(obj)) {
        d->AddChangeCallback(self.GetSelfCallback());
    } else if (auto n = interface_cast<INotifyOnChange>(obj)) {
        n->OnChanged()->AddHandler(legacyCb, reinterpret_cast<uintptr_t>(&self));
    }
}

template<typename T>
void UnsubscribeOnChanged(const T& obj, StackProperty& self, const ICallable::Ptr& legacyCb, bool reenable = false)
{
    if (auto d = interface_cast<INotifyOnChangeDirect>(obj)) {
        d->RemoveChangeCallback(self.GetSelfCallback());
    } else if (auto n = interface_cast<INotifyOnChange>(obj)) {
        n->OnChanged()->RemoveHandler(reinterpret_cast<uintptr_t>(&self), reenable);
    }
}

}  // namespace

StackProperty::StackProperty(BASE_NS::string name) : Super(BASE_NS::move(name))
{}

void StackProperty::SubscribePendingCallbacks()
{
    if (defaultValue_) {
        SubscribeOnChanged(defaultValue_, *this, onChangedCallback_);
    }
    for (auto&& v : values_) {
        SubscribeOnChanged(v, *this, onChangedCallback_);
    }
    for (auto&& m : modifiers_) {
        SubscribeOnChanged(m, *this, onChangedCallback_);
    }
}

StackProperty::~StackProperty()
{
    CleanUp();
}

void StackProperty::CleanUp()
{
    UnsubscribeOnChanged(defaultValue_, *this, onChangedCallback_);
    for (auto&& m : modifiers_) {
        UnsubscribeOnChanged(m, *this, onChangedCallback_);
    }
    modifiers_.clear();
    for (auto&& v : values_) {
        UnsubscribeOnChanged(v, *this, onChangedCallback_);
    }
    values_.clear();
}

AnyReturnValue StackProperty::TryToSetToValue(const IAny& v, IValue::Ptr& value)
{
    AnyReturnValue res = AnyReturn::FAIL;

    // INotifyOnChangeDirect values have deferred notifications, no need to suppress.
    auto noti = interface_cast<INotifyOnChangeDirect>(value) ? nullptr : interface_cast<INotifyOnChange>(value);
    InterfaceUniqueLock lock{value};
    if (noti) {
        noti->OnChanged()->EnableHandler(uintptr_t(this), false);
    }
    res = value->SetValue(v);
    if (res) {
        if (noti) {
            noti->OnChanged()->EnableHandler(uintptr_t(this), true);
        }
    }
    return res;
}

AnyReturnValue StackProperty::SetValueInValueStack(const IAny& value)
{
    AnyReturnValue res = AnyReturn::FAIL;
    // find first value that accepts the new value, all non-accepting values are removed
    for (int i = int(values_.size()) - 1; i >= 0; --i) {
        res = TryToSetToValue(value, values_[i]);
        if (res) {
            break;
        }
        UnsubscribeOnChanged(values_[i], *this, onChangedCallback_, true);
        values_.pop_back();
    }
    // if there was no any to set the new value, create one
    if (!res) {
        if (auto c = interface_pointer_cast<IValue>(value.Clone(true))) {
            SubscribeOnChanged(c, *this, onChangedCallback_);
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
        // reset cached value so that shared_ptrs et al get destroyed
        currentValue_->ResetValue();
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
        CORE_ASSERT_MSG(v, "Internal any not set");
        res = SetInternalValue(value);
        if (res) {
            res = SetValueToStack(v);
        }
        if (res) {
            SetDefaultValueFlag(false);
        }
    }

    evaluating_ = false;
    if (HasPendingInvoke() && !locked_) {
        SetPendingInvoke(false);
        InvokeOnChanged(onChanged_, owner_);
    }
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
            InterfaceSharedLock lock{v};
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

void StackProperty::EvaluateAndStore()
{
    // evaluate
    const IAny& res = RawGetValue();
    // store
    if (auto v = TopValue()) {
        TryToSetToValue(res, v);
    }
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
    SubscribeOnChanged(value, *this, onChangedCallback_);
    NotifyChange();
    return GenericError::SUCCESS;
}
ReturnError StackProperty::PopValue()
{
    if (!values_.empty()) {
        UnsubscribeOnChanged(values_.back(), *this, onChangedCallback_);
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
            UnsubscribeOnChanged(*m, *this, onChangedCallback_);
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
    SubscribeOnChanged(mod, *this, onChangedCallback_);
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
        UnsubscribeOnChanged(p, *this, onChangedCallback_);
        NotifyChange();
    }
    return p;
}
ReturnError StackProperty::RemoveModifier(const IModifier::Ptr& mod)
{
    for (auto m = modifiers_.begin(); m != modifiers_.end(); ++m) {
        if (*m == mod) {
            UnsubscribeOnChanged(*m, *this, onChangedCallback_);
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
    if (!defaultValue_) {
        CORE_LOG_D("Initializing internal any with SetDefaultValue");
        auto res = SetInternalAny(value.Clone(false));
        if (!res) {
            return res;
        }
    }
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
        UnsubscribeOnChanged(defaultValue_, *this, onChangedCallback_);
        for (auto&& v : values_) {
            UnsubscribeOnChanged(v, *this, onChangedCallback_);
        }
        for (auto&& m : modifiers_) {
            UnsubscribeOnChanged(m, *this, onChangedCallback_);
        }
        defaultValue_ = any->Clone(true);
        SubscribeOnChanged(defaultValue_, *this, onChangedCallback_);
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
    CallOnChanged(evaluating_);
}
template<typename Vec>
bool StackProperty::ProcessResetables(Vec& vec)
{
    for (int i = int(vec.size()) - 1; i >= 0; --i) {
        ResetResult res = ResetResult::RESET_REMOVE_ME;
        if (auto rable = interface_cast<IStackResetable>(vec[i])) {
            InterfaceSharedLock lock{rable};
            res = rable->ProcessOnReset(*defaultValue_);
        }
        if (res & RESET_REMOVE_ME) {
            UnsubscribeOnChanged(vec[i], *this, onChangedCallback_);
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
    ProcessResetables(modifiers_);
    ProcessResetables(values_);

    // reset the currentValue_ and internal value so we don't accidentally keep any shared_ptrs alive
    currentValue_ = defaultValue_->Clone(false);
    SetInternalValue(*currentValue_);
    SetDefaultValueFlag(true);
    NotifyChange();
}
void StackProperty::RemoveAll()
{
    for (auto&& v : values_) {
        UnsubscribeOnChanged(v, *this, onChangedCallback_);
    }
    values_.clear();
    for (auto&& m : modifiers_) {
        UnsubscribeOnChanged(m, *this, onChangedCallback_);
    }
    modifiers_.clear();
    // reset the currentValue_ and internal value so we don't accidentally keep any shared_ptrs alive
    currentValue_ = defaultValue_->Clone(false);
    SetInternalValue(*currentValue_);
    SetDefaultValueFlag(true);
    NotifyChange();
}
ReturnError StackProperty::Export(IExportContext& c) const
{
    if (requiresEvaluation_) {
        // evaluate to make sure we have fresh values to export
        RawGetValue();
    }
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
        bool def = true;
        if (!defaultValue_) {
            return GenericError::FAIL;
        }
        SubscribeOnChanged(defaultValue_, *this, onChangedCallback_);
        if (auto res = Super::SetInternalAny(defaultValue_->Clone(false))) {
            currentValue_ = defaultValue_->Clone(true);
            requiresEvaluation_ = true;
        }
        for (auto&& i : values) {
            if (auto v = interface_pointer_cast<IValue>(i)) {
                SubscribeOnChanged(v, *this, onChangedCallback_);
                values_.push_back(BASE_NS::move(v));
                def = false;
            }
        }
        for (auto&& i : modifiers) {
            if (auto v = interface_pointer_cast<IModifier>(i)) {
                SubscribeOnChanged(v, *this, onChangedCallback_);
                modifiers_.push_back(BASE_NS::move(v));
            }
        }
        SetDefaultValueFlag(def);
    }
    return ser;
}

bool StackProperty::IsDefaultValue() const
{
    return HasDefaultValueFlag();
}

}  // namespace Internal
META_END_NAMESPACE()
