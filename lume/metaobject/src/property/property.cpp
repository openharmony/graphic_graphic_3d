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
#include "property.h"

META_BEGIN_NAMESPACE()
namespace Internal {

BASE_NS::string PropertyBase::GetName() const
{
    return name_;
}
IObject::WeakPtr PropertyBase::GetOwner() const
{
    return owner_;
}
void PropertyBase::SetOwner(IObject::Ptr owner)
{
    owner_ = owner;
}
void PropertyBase::SetSelf(IProperty::Ptr self)
{
    self_ = self;
}
AnyReturnValue PropertyBase::SetValue(const IAny& value)
{
    auto res = SetInternalValue(value);
    if (res == AnyReturn::SUCCESS) {
        CallOnChanged();
    }
    return res;
}
const IAny& PropertyBase::GetValue() const
{
    auto v = GetData();
    CORE_ASSERT(v);
    return *v;
}
bool PropertyBase::IsCompatible(const TypeId& id) const
{
    auto v = GetData();
    return v && META_NS::IsCompatible(*v, id);
}
TypeId PropertyBase::GetTypeId() const
{
    auto p = GetData();
    return p ? p->GetTypeId() : TypeId {};
}
IEvent::Ptr PropertyBase::EventOnChanged() const
{
    // notice that this requires locking outside unlike the old system
    // if needed, consider separate locking for the construction
    if (!onChanged_) {
        onChanged_ = CreateShared<OnChangedEvent>("OnChanged");
        onChangedAtomic_.store(onChanged_.get());
    }
    return onChanged_;
}
void PropertyBase::NotifyChange() const
{
    CallOnChanged();
}
void PropertyBase::CallOnChanged() const
{
    pendingInvoke_ = locked_ != 0;
    if (!locked_ && onChanged_) {
        onChanged_->Invoke();
    }
}
void PropertyBase::Lock() const
{
    mutex_.lock();
    ++locked_;
}
void PropertyBase::Unlock() const
{
    BASE_NS::shared_ptr<OnChangedEvent> invoke;
    if (--locked_ == 0 && pendingInvoke_) {
        invoke = onChanged_;
        pendingInvoke_ = false;
    }
    mutex_.unlock();
    if (invoke) {
        invoke->Invoke();
    }
}
void PropertyBase::LockShared() const
{
    Lock();
}
void PropertyBase::UnlockShared() const
{
    Unlock();
}
AnyReturnValue PropertyBase::SetInternalValue(const IAny& value)
{
    if (auto& d = GetData()) {
        if (d.get() == &value) {
            return AnyReturn::SUCCESS;
        }
        return d->CopyFrom(value);
    }
    return AnyReturn::FAIL;
}

AnyReturnValue GenericProperty::SetInternalAny(IAny::Ptr any)
{
    data_ = BASE_NS::move(any);
    return AnyReturn::SUCCESS;
}
IAny::Ptr GenericProperty::GetInternalAny() const
{
    return data_;
}

} // namespace Internal
META_END_NAMESPACE()
