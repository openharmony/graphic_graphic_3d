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
#include "engine_value.h"

#include <core/property/intf_property_api.h>

META_BEGIN_NAMESPACE()

namespace Internal {

EngineValue::EngineValue(
    BASE_NS::string name, IEngineInternalValueAccess::ConstPtr access, const EnginePropertyParams& p)
    : params_(p), access_(BASE_NS::move(access)), name_(BASE_NS::move(name)), value_(access_->CreateAny())
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
    return access_->SyncFromEngine(params_, *value_);
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
            res = access_->SyncToEngine(*value_, params_) ? AnyReturn::NOTHING_TO_DO : AnyReturn::FAIL;
        }
    }
    return res;
}
const IAny& EngineValue::GetValue() const
{
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
ResetResult EngineValue::ProcessOnReset(const IAny&)
{
    return RESET_CONTINUE;
}
BASE_NS::shared_ptr<IEvent> EngineValue::EventOnChanged() const
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
    return true;
}

} // namespace Internal

META_END_NAMESPACE()
