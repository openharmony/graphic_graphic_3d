#include "engine_value.h"

#include <core/property/intf_property_api.h>

#include "../any.h"

META_BEGIN_NAMESPACE()

namespace Internal {

EngineValue::EngineValue(
    BASE_NS::string name, IEngineInternalValueAccess::ConstPtr access, const EnginePropertyParams& p)
    : Super(ObjectFlagBits::INTERNAL | ObjectFlagBits::SERIALIZE), params_(p), access_(BASE_NS::move(access)),
      name_(BASE_NS::move(name)), value_(access_->CreateAny(p.property))
{}

AnyReturnValue EngineValue::Sync(EngineSyncDirection dir)
{
    if (!params_.handle) {
        return AnyReturn::INVALID_ARGUMENT;
    }
    if (dir == EngineSyncDirection::TO_ENGINE || (dir == EngineSyncDirection::AUTO && valueChanged_)) {
        if (valueChanged_) {
            valueChanged_ = false;
            return access_->SyncToEngine(*value_, params_) ? AnyReturn::NOTHING_TO_DO : AnyReturn::FAIL;
        }
        return AnyReturn::NOTHING_TO_DO;
    }
    auto res = access_->SyncFromEngine(params_, *value_);
    pendingNotify_ = pendingNotify_ || (res && res != AnyReturn::NOTHING_TO_DO);
    return res;
}
AnyReturnValue EngineValue::SetValue(const IAny& value)
{
    AnyReturnValue res = value_->CopyFrom(value);
    valueChanged_ |= static_cast<bool>(res);
    if (params_.pushValueToEngineDirectly) {
        if (valueChanged_) {
            if (!params_.handle) {
                return AnyReturn::INVALID_ARGUMENT;
            }
            valueChanged_ = false;
            // this returns intentionally NOTHING_TO_DO for now as the ets scene plugin completely breaks if notify
            // about this.
            res = access_->SyncToEngine(*value_, params_) ? AnyReturn::NOTHING_TO_DO : AnyReturn::FAIL;
        }
    }
    return res;
}
const IAny& EngineValue::GetValue() const
{
    if (params_.pushValueToEngineDirectly) {
        if (!params_.handle || !access_->SyncFromEngine(params_, *value_)) {
            return INVALID_ANY;
        }
    }
    return *value_;
}
bool EngineValue::IsCompatible(const TypeId& id) const
{
    return META_NS::IsCompatible(*value_, id);
}
BASE_NS::string EngineValue::GetName() const
{
    return name_;
}
void EngineValue::Lock() const
{
    mutex_.lock();
}
void EngineValue::Unlock() const
{
    mutex_.unlock();
}
void EngineValue::LockShared() const
{
    mutex_.lock_shared();
}
void EngineValue::UnlockShared() const
{
    mutex_.unlock_shared();
}
ResetResult EngineValue::ProcessOnReset(const IAny& value)
{
    SetValue(value);
    return RESET_CONTINUE;
}
BASE_NS::shared_ptr<IEvent> EngineValue::EventOnChanged(MetadataQuery) const
{
    return event_;
}
EnginePropertyParams EngineValue::GetPropertyParams() const
{
    return params_;
}
bool EngineValue::SetPropertyParams(const EnginePropertyParams& p)
{
    params_ = p;
    // todo: make new any to reflect the metadata
    return true;
}
IAny::Ptr EngineValue::CreateAny() const
{
    return access_ ? access_->CreateSerializableAny() : nullptr;
}
bool EngineValue::ResetPendingNotify()
{
    auto res = pendingNotify_;
    pendingNotify_ = false;
    return res;
}
ReturnError EngineValue::Export(IExportContext& c) const
{
    META_NS::IAny::Ptr any = access_->CreateSerializableAny();
    if (any->CopyFrom(*value_)) {
        if (auto node = c.ExportValueToNode(any)) {
            return c.SubstituteThis(node);
        }
    }
    return GenericError::FAIL;
}
ReturnError EngineValue::Import(IImportContext&)
{
    return GenericError::FAIL;
}

} // namespace Internal

META_END_NAMESPACE()
