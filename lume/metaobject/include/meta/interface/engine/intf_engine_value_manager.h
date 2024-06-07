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

#ifndef META_ENGINE_INTERFACE_ENGINE_VALUE_MANAGER_H
#define META_ENGINE_INTERFACE_ENGINE_VALUE_MANAGER_H

#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IEngineValueManager, "542b7e13-21bd-4c9c-8d3f-a9527732216c")

struct EngineValueOptions {
    /// prefix that is added before the name (plus '.')
    BASE_NS::string namePrefix;
    /// optional output vector to put all constructed values
    BASE_NS::vector<IEngineValue::Ptr>* values {};
    /// Push value directly to engine when set, if this is enabled, the values can only be set in the engine task queue
    /// thread
    bool pushValuesDirectlyToEngine {};
};

class IEngineValueManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineValueManager)
public:
    virtual void SetNotificationQueue(const ITaskQueue::WeakPtr&) = 0;

    virtual bool Sync(EngineSyncDirection) = 0;

    /// Add engine values from the handle
    virtual bool ConstructValues(CORE_NS::IPropertyHandle* handle, EngineValueOptions) = 0;
    // Add engine values from value (e.g. member properties or follow IPropertyHandle*)
    virtual bool ConstructValues(IValue::Ptr value, EngineValueOptions) = 0;
    // Add single engine value from EnginePropertyParams
    virtual bool ConstructValue(EnginePropertyParams property, EngineValueOptions) = 0;
    virtual bool ConstructValue(CORE_NS::IPropertyHandle* handle, BASE_NS::string_view path, EngineValueOptions) = 0;

    virtual bool RemoveValue(BASE_NS::string_view name) = 0;
    virtual void RemoveAll() = 0;

    virtual IProperty::Ptr ConstructProperty(BASE_NS::string_view name) const = 0;
    virtual IEngineValue::Ptr GetEngineValue(BASE_NS::string_view name) const = 0;
    virtual BASE_NS::vector<IProperty::Ptr> ConstructAllProperties() const = 0;
    virtual BASE_NS::vector<IEngineValue::Ptr> GetAllEngineValues() const = 0;

    template<typename Type>
    Property<Type> ConstructProperty(BASE_NS::string_view name)
    {
        return Property<Type>(ConstructProperty(name));
    }
    template<typename Type>
    ArrayProperty<Type> ConstructArrayProperty(BASE_NS::string_view name)
    {
        return ArrayProperty<Type>(ConstructProperty(name));
    }
    bool ConstructValues(CORE_NS::IPropertyHandle* handle)
    {
        return ConstructValues(handle, {});
    }
};

META_INTERFACE_TYPE(IEngineValueManager);
META_END_NAMESPACE()

#endif
