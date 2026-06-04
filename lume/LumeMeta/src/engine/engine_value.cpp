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

#include <algorithm>

#include <core/property/intf_property_api.h>

#include <meta/interface/engine/intf_engine_value_manager.h>

#include "../any.h"
#include "engine_dirty_list.h"

META_BEGIN_NAMESPACE()

namespace Internal {

EngineValue::EngineValue(
    BASE_NS::string name, IEngineInternalValueAccess::ConstPtr access, const EnginePropertyParams& p)
    : Super(ObjectFlagBits::INTERNAL | ObjectFlagBits::SERIALIZE),
      params_(p),
      access_(BASE_NS::move(access)),
      name_(BASE_NS::move(name)),
      value_(access_->CreateAny(p.property))
{}

EngineValue::~EngineValue() = default;

bool EngineValue::HasChangeCallbacks() const
{
    return changeCallbacks_.HasCallbacks();
}

bool EngineValue::CanSkipFromEngineSync() const
{
    return flags_.IsSet(ValueFlags::INITIALIZED) && !changeCallbacks_.HasCallbacks();
}

AnyReturnValue EngineValue::Sync(EngineSyncDirection dir)
{
    if (!params_.handle) {
        return AnyReturn::INVALID_ARGUMENT;
    }
    if (dir == EngineSyncDirection::TO_ENGINE ||
        (dir == EngineSyncDirection::AUTO && flags_.IsSet(ValueFlags::VALUE_CHANGED))) {
        if (flags_.IsSet(ValueFlags::VALUE_CHANGED)) {
            auto res = access_->SyncToEngine(*value_, params_);
            if (res) {
                flags_.Clear(ValueFlags::VALUE_CHANGED);
            }
            return res ? AnyReturn::NOTHING_TO_DO : AnyReturn::FAIL;
        }
        return params_.handle.Handle() ? AnyReturn::NOTHING_TO_DO : AnyReturn::FAIL;
    }
    auto res = access_->SyncFromEngine(params_, *value_);
    bool firstSync = !flags_.IsSet(ValueFlags::INITIALIZED);
    flags_.Set(ValueFlags::INITIALIZED);
    if (!res && params_.containerMethods) {
        // Container element subscription may refer to an index that is out of range
        // after the parent container was resized. This is not an error.
        return AnyReturn::NOTHING_TO_DO;
    }
    // Don't notify on the first sync — the value is being initialized, not changed.
    if (!firstSync && res && res != AnyReturn::NOTHING_TO_DO) {
        flags_.Set(ValueFlags::PENDING_NOTIFY);
    }
    return res;
}
AnyReturnValue EngineValue::SetValue(const IAny& value)
{
    AnyReturnValue res = value_->CopyFrom(value);
    bool wasDirty = flags_.IsSet(ValueFlags::VALUE_CHANGED);
    if (res) {
        flags_.Set(ValueFlags::VALUE_CHANGED);
    }
    if (!wasDirty && flags_.IsSet(ValueFlags::VALUE_CHANGED)) {
        MarkDirty();
    }
    if (params_.pushValueToEngineDirectly) {
        if (flags_.IsSet(ValueFlags::VALUE_CHANGED)) {
            if (!params_.handle) {
                return AnyReturn::INVALID_ARGUMENT;
            }
            flags_.Clear(ValueFlags::VALUE_CHANGED);
            // this returns intentionally NOTHING_TO_DO for now as the ets scene plugin completely breaks if notify
            // about this.
            res = access_->SyncToEngine(*value_, params_) ? AnyReturn::NOTHING_TO_DO : AnyReturn::FAIL;
        }
    }
    return res;
}
void EngineValue::SetDirtyList(EngineDirtyList* list)
{
    dirtyList_ = list;
}
void EngineValue::ClearDirtyList()
{
    dirtyList_ = nullptr;
}
CORE_NS::IComponentManager* EngineValue::GetComponentManager() const
{
    return params_.handle.manager;
}
void EngineValue::MarkDirty()
{
    if (auto* list = dirtyList_) {
        {
            std::lock_guard lock{list->mutex};
            list->values.push_back(static_cast<IEngineValue*>(this));
        }
        if (list->owner) {
            list->owner->NotifyDirty();
        }
    }
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
void EngineValue::AddChangeCallback(const INotifyOnChangeCallback::Ptr& callback)
{
    changeCallbacks_.Add(callback, mutex_);
}

void EngineValue::RemoveChangeCallback(const INotifyOnChangeCallback::Ptr& callback)
{
    changeCallbacks_.Remove(callback, mutex_);
}

void EngineValue::InvokeChangeCallbacks()
{
    for (auto& cb : changeCallbacks_.GetCallbacks(mutex_)) {
        cb->NotifyChanged(*this);
    }
}

void ChangeCallbackList::Add(const INotifyOnChangeCallback::Ptr& cb, std::shared_mutex& mutex)
{
    std::unique_lock lock{mutex};
    callbacks_.push_back(cb);
    count_.store(static_cast<uint8_t>(callbacks_.size()), std::memory_order_relaxed);
}

void ChangeCallbackList::Remove(const INotifyOnChangeCallback::Ptr& cb, std::shared_mutex& mutex)
{
    std::unique_lock lock{mutex};
    auto it = std::find_if(callbacks_.begin(), callbacks_.end(), [&cb](const INotifyOnChangeCallback::WeakPtr& w) {
        return w.lock() == cb;
    });
    if (it != callbacks_.end()) {
        callbacks_.erase(it);
    }
    count_.store(static_cast<uint8_t>(callbacks_.size()), std::memory_order_relaxed);
}

BASE_NS::vector<INotifyOnChangeCallback::Ptr> ChangeCallbackList::GetCallbacks(std::shared_mutex& mutex) const
{
    BASE_NS::vector<INotifyOnChangeCallback::Ptr> locked;
    {
        std::unique_lock lock{mutex};
        for (auto& w : callbacks_) {
            if (auto cb = w.lock()) {
                locked.push_back(BASE_NS::move(cb));
            }
        }
    }
    return locked;
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
    auto res = flags_.IsSet(ValueFlags::PENDING_NOTIFY);
    flags_.Clear(ValueFlags::PENDING_NOTIFY);
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

}  // namespace Internal

META_END_NAMESPACE()
