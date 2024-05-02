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

#include <shared_mutex>

#include <meta/ext/event_impl.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_lockable.h>
#include <meta/interface/intf_notify_on_change.h>
#include <meta/interface/property/intf_stack_resetable.h>

META_BEGIN_NAMESPACE()

namespace Internal {

class IEngineValueInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineValueInternal, "86afd68c-7924-46b8-8bb8-aeace0029945")
public:
    virtual IEngineInternalValueAccess::ConstPtr GetInternalAccess() const = 0;
    virtual EnginePropertyParams GetPropertyParams() const = 0;
    virtual bool SetPropertyParams(const EnginePropertyParams& p) = 0;
};

META_REGISTER_CLASS(EngineValue, "a8a9c80f-3501-48da-b552-f1f323ee399b", ObjectCategoryBits::NO_CATEGORY)

class EngineValue : public MinimalObject<ClassId::EngineValue, IEngineValue, INotifyOnChange, ILockable,
                        IStackResetable, IEngineValueInternal> {
public:
    EngineValue(BASE_NS::string name, IEngineInternalValueAccess::ConstPtr access, const EnginePropertyParams& p);

    AnyReturnValue Sync(EngineSyncDirection) override;

    AnyReturnValue SetValue(const IAny& value) override;
    const IAny& GetValue() const override;
    bool IsCompatible(const TypeId& id) const override;

    BASE_NS::string GetName() const override;

    void Lock() const override;
    void Unlock() const override;
    void LockShared() const override;
    void UnlockShared() const override;

    ResetResult ProcessOnReset(const IAny& defaultValue) override;

    BASE_NS::shared_ptr<IEvent> EventOnChanged() const override;

private:
    IEngineInternalValueAccess::ConstPtr GetInternalAccess() const override
    {
        return access_;
    }
    EnginePropertyParams GetPropertyParams() const override;
    bool SetPropertyParams(const EnginePropertyParams& p) override;

private:
    mutable std::shared_mutex mutex_;
    EnginePropertyParams params_;
    IEngineInternalValueAccess::ConstPtr access_;
    bool valueChanged_ {};
    const BASE_NS::string name_;
    IAny::Ptr value_;
    BASE_NS::shared_ptr<EventImpl<IOnChanged>> event_ { new EventImpl<IOnChanged>("OnChanged") };
};

} // namespace Internal

META_END_NAMESPACE()

#endif