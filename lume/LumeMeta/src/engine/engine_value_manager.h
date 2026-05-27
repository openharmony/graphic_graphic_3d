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
#ifndef META_SRC_ENGINE_ENGINE_VALUE_MANAGER_H
#define META_SRC_ENGINE_ENGINE_VALUE_MANAGER_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_task_queue.h>

#include "../object.h"
#include "engine_dirty_list.h"
#include "engine_value.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class EngineValueManager : public IntroduceInterfaces<BaseObject, IEngineValueManager> {
    META_OBJECT(EngineValueManager, META_NS::ClassId::EngineValueManager, IntroduceInterfaces)
public:
    META_NO_COPY_MOVE(EngineValueManager)
    EngineValueManager()
    {
        dirtyList_.owner = this;
    }
    ~EngineValueManager() override;

    void SetNotificationQueue(const ITaskQueue::WeakPtr&) override;
    void SetDirtyNotifier(const IOnChanged::InterfaceTypePtr& notifier) override;
    void NotifyDirty() override;
    bool ConstructValues(EnginePropertyHandle handle, EngineValueOptions) override;
    bool ConstructValues(CORE_NS::IPropertyHandle* handle, EngineValueOptions) override;
    bool ConstructValues(IValue::Ptr value, EngineValueOptions) override;
    bool ConstructValue(EnginePropertyParams property, EngineValueOptions) override;
    bool ConstructValue(EnginePropertyHandle handle, BASE_NS::string_view path, EngineValueOptions) override;

    bool RemoveValue(BASE_NS::string_view name) override;
    void RemoveAll() override;

    IProperty::Ptr ConstructProperty(BASE_NS::string_view name) const override;
    IEngineValue::Ptr GetEngineValue(BASE_NS::string_view name) const override;

    BASE_NS::vector<IProperty::Ptr> ConstructAllProperties() const override;
    BASE_NS::vector<IEngineValue::Ptr> GetAllEngineValues() const override;
    bool HasValues() const override;

    bool Sync(EngineSyncDirection dir, const CORE_NS::IComponentManager* componentManager) override;

private:
    struct SyncResult {
        bool allSucceeded{true};
        bool anyChanged{false};
        bool hadDirtyValues{false};
    };
    void NotifySyncs();
    void RemoveDirtyValue(IEngineValue* value);
    BASE_NS::vector<IEngineValue*> StealDirtyValues(const CORE_NS::IComponentManager* componentManager);
    SyncResult SyncDirtyValues(const CORE_NS::IComponentManager* componentManager, EngineSyncDirection dir);
    SyncResult SyncAllValues(
        const CORE_NS::IComponentManager* componentManager, EngineSyncDirection dir, bool skipUnobserved);
    void ScheduleNotifySyncs();
    IEngineValue::Ptr AddValue(EnginePropertyParams p, EngineValueOptions options);
    IEngineValue::Ptr ConstructValueImplAdd(
        EnginePropertyParams params, BASE_NS::string pathTaken, EngineValueOptions options);
    bool ConstructValueImpl(
        EnginePropertyHandle handle, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options);
    bool ConstructValueImpl(
        EnginePropertyParams params, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options);
    bool ConstructValueImplArraySubs(
        EnginePropertyParams params, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options);

    struct ValueEntry {
        BASE_NS::string name;
        IEngineValue::Ptr value;
    };
    using ValueList = BASE_NS::vector<ValueEntry>;
    ValueList::iterator FindValue(BASE_NS::string_view name);
    ValueList::const_iterator FindValue(BASE_NS::string_view name) const;

private:
    mutable std::shared_mutex mutex_;
    ITaskQueue::WeakPtr queue_;
    ValueList values_;
    ITaskQueue::Token task_token_{};
    IOnChanged::InterfaceTypePtr dirtyNotifier_;
    EngineDirtyList dirtyList_;
};

}  // namespace Internal

META_END_NAMESPACE()

#endif
