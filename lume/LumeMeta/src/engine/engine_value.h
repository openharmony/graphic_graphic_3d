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
#ifndef META_SRC_ENGINE_ENGINE_VALUE_H
#define META_SRC_ENGINE_ENGINE_VALUE_H

#include <atomic>
#include <shared_mutex>

#include <meta/base/bit_field.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_lockable.h>
#include <meta/interface/intf_notify_on_change.h>
#include <meta/interface/property/intf_stack_resetable.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

namespace Internal {

struct EngineDirtyList;

/** Thread-safe list of weak change callbacks. Borrows an external mutex for synchronisation. */
class ChangeCallbackList {
public:
    void Add(const INotifyOnChangeCallback::Ptr& cb, std::shared_mutex& mutex);
    void Remove(const INotifyOnChangeCallback::Ptr& cb, std::shared_mutex& mutex);
    BASE_NS::vector<INotifyOnChangeCallback::Ptr> GetCallbacks(std::shared_mutex& mutex) const;
    bool HasCallbacks() const
    {
        return count_.load(std::memory_order_relaxed) > 0;
    }

private:
    BASE_NS::vector<INotifyOnChangeCallback::WeakPtr> callbacks_;
    std::atomic<uint8_t> count_{};
};

META_REGISTER_CLASS(EngineValue, "a8a9c80f-3501-48da-b552-f1f323ee399b", ObjectCategoryBits::NO_CATEGORY)

class EngineValue : public IntroduceInterfaces<MinimalObject, IEngineValue, INotifyOnChangeDirect, ILockable,
                        IStackResetable, IEngineValueInternal, ISerializable> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::EngineValue)
    using Super = IntroduceInterfaces;

public:
    EngineValue(BASE_NS::string name, IEngineInternalValueAccess::ConstPtr access, const EnginePropertyParams& p);
    ~EngineValue() override;

    AnyReturnValue Sync(EngineSyncDirection) override;
    bool ResetPendingNotify() override;

    AnyReturnValue SetValue(const IAny& value) override;
    const IAny& GetValue() const override;
    bool IsCompatible(const TypeId& id) const override;

    BASE_NS::string GetName() const override;

    void Lock() const override;
    void Unlock() const override;
    void LockShared() const override;
    void UnlockShared() const override;

    ResetResult ProcessOnReset(const IAny& defaultValue) override;

    void AddChangeCallback(const INotifyOnChangeCallback::Ptr& callback) override;
    void RemoveChangeCallback(const INotifyOnChangeCallback::Ptr& callback) override;

    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;

private:
    IEngineInternalValueAccess::ConstPtr GetInternalAccess() const override
    {
        return access_;
    }
    EnginePropertyParams GetPropertyParams() const override;
    bool SetPropertyParams(const EnginePropertyParams& p) override;
    IAny::Ptr CreateAny() const override;

public:
    /** Set the dirty list that this value should push itself to when modified. */
    void SetDirtyList(EngineDirtyList* list);
    /** Disconnect from the dirty list (e.g. before destruction or removal). */
    void ClearDirtyList();
    /** Returns the component manager this value belongs to (may be null). */
    CORE_NS::IComponentManager* GetComponentManager() const;
    /** Invoke the change callback if set. Called by EngineValueManager::NotifySyncs. */
    void InvokeChangeCallbacks();
    /** Returns true if a change callback is registered. */
    bool HasChangeCallbacks() const;
    /** Returns true if this value can be skipped during FROM_ENGINE sync (already synced and unobserved). */
    bool CanSkipFromEngineSync() const;

    enum class ValueFlags : uint8_t {
        /** @brief Set (and remains set) after after first sync from engine */
        INITIALIZED = 1 << 0,
        /** @brief Set when user modifies the local value with SetValue.
         *         Cleared when the value is successfully synced TO_ENGINE. */
        VALUE_CHANGED = 1 << 1,
        /** @brief Set when a values is read FROM_ENGINE, resulting in a change in local value.
         *         Cleared when NotifySyncs() is called. */
        PENDING_NOTIFY = 1 << 2,
    };

private:
    void MarkDirty();

    mutable std::shared_mutex mutex_;
    EnginePropertyParams params_;
    IEngineInternalValueAccess::ConstPtr access_;
    EnumBitField<ValueFlags, uint8_t> flags_{};
    const BASE_NS::string name_;
    IAny::Ptr value_;
    ChangeCallbackList changeCallbacks_;
    EngineDirtyList* dirtyList_{};
};

}  // namespace Internal

META_END_NAMESPACE()

#endif
