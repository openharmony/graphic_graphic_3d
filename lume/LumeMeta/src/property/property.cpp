
#include "property.h"

#include <meta/interface/intf_owner.h>

META_BEGIN_NAMESPACE()
namespace Internal {

BASE_NS::string PropertyBase::GetName() const
{
    return name_;
}
IOwner::WeakPtr PropertyBase::GetOwner() const
{
    return owner_;
}
void PropertyBase::SetOwner(IOwner::Ptr owner)
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
        CallOnChanged(false);
    }
    return res;
}
const IAny& PropertyBase::GetValue() const
{
    auto v = GetData();
    CORE_ASSERT_MSG(v, "Internal any not set");
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
IEvent::Ptr PropertyBase::EventOnChanged(MetadataQuery q) const
{
    // notice that this requires locking outside unlike the old system
    // if needed, consider separate locking for the construction
    if (!onChanged_ && q == MetadataQuery::CONSTRUCT_ON_REQUEST) {
        onChanged_ = CreateShared<OnChangedEvent>("OnChanged");
        onChangedAtomic_.store(onChanged_.get());
    }
    return onChanged_;
}
void PropertyBase::NotifyChange() const
{
    CallOnChanged(false);
}
void PropertyBase::InvokeOnChanged(
    const BASE_NS::shared_ptr<OnChangedEvent>& onChanged, const IOwner::WeakPtr& owner) const
{
    if (auto ow = interface_pointer_cast<IPropertyOwner>(owner)) {
        ow->OnPropertyChanged(*this);
    }
    if (onChanged) {
        onChanged->Invoke();
    }
}
void PropertyBase::CallOnChanged(bool forcePending) const
{
    bool pending = forcePending || locked_ != 0;
    SetPendingInvoke(pending);
    if (!pending) {
        InvokeOnChanged(onChanged_, owner_);
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
    IOwner::WeakPtr owner {};
    if (--locked_ == 0 && HasPendingInvoke()) {
        invoke = onChanged_;
        SetPendingInvoke(false);
        owner = owner_;
    }
    mutex_.unlock();
    InvokeOnChanged(invoke, owner);
}
void PropertyBase::LockShared() const
{
    Lock();
}
void PropertyBase::UnlockShared() const
{
    Unlock();
}
bool PropertyBase::Attaching(const IAttach::Ptr& target, const IObject::Ptr&)
{
    owner_ = interface_pointer_cast<IOwner>(target);
    return true;
}
bool PropertyBase::Detaching(const IAttach::Ptr&)
{
    owner_ = nullptr;
    return true;
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
