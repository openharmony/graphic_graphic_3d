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

#include <base/containers/unordered_map.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_task_queue.h>

#include "../meta_object.h"
#include "engine_value.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class EngineValueManager
    : public Internal::BaseObjectFwd<EngineValueManager, META_NS::ClassId::EngineValueManager, IEngineValueManager> {
public:
    META_NO_COPY_MOVE(EngineValueManager)
    EngineValueManager() = default;
    ~EngineValueManager() override;

    void SetNotificationQueue(const ITaskQueue::WeakPtr&) override;
    bool ConstructValues(CORE_NS::IPropertyHandle* handle, EngineValueOptions) override;
    bool ConstructValues(IValue::Ptr value, EngineValueOptions) override;
    bool ConstructValue(EnginePropertyParams property, EngineValueOptions) override;
    bool ConstructValue(CORE_NS::IPropertyHandle* handle, BASE_NS::string_view path, EngineValueOptions) override;

    bool RemoveValue(BASE_NS::string_view name) override;
    void RemoveAll() override;

    IProperty::Ptr ConstructProperty(BASE_NS::string_view name) const override;
    IEngineValue::Ptr GetEngineValue(BASE_NS::string_view name) const override;

    BASE_NS::vector<IProperty::Ptr> ConstructAllProperties() const override;
    BASE_NS::vector<IEngineValue::Ptr> GetAllEngineValues() const override;

    bool Sync(EngineSyncDirection) override;

private:
    void NotifySyncs();
    IProperty::Ptr PropertyFromValue(BASE_NS::string_view name, const IEngineValue::Ptr&) const;
    void AddValue(EnginePropertyParams p, EngineValueOptions options);
    bool ConstructValueImpl(CORE_NS::IPropertyHandle* handle, BASE_NS::string pathTaken, BASE_NS::string_view path,
        EngineValueOptions options);
    bool ConstructValueImpl(
        EnginePropertyParams params, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options);

private:
    mutable std::shared_mutex mutex_;
    ITaskQueue::WeakPtr queue_;
    struct ValueInfo {
        IEngineValue::Ptr value;
        bool notify {};
    };
    BASE_NS::unordered_map<BASE_NS::string, ValueInfo> values_;
    ITaskQueue::Token task_token_ {};
};

} // namespace Internal

META_END_NAMESPACE()

#endif