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
#include "engine_value_manager.h"

#include <core/property/intf_property_api.h>
#include <core/property/scoped_handle.h>

#include <meta/api/make_callback.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

namespace Internal {

template<typename Type>
static Type ReadValueFromEngine(const EnginePropertyParams& params)
{
    Type res {};
    CORE_NS::ScopedHandle<const Type> guard { params.handle };
    if (guard) {
        res = *(const Type*)((uintptr_t) & *guard + params.Offset());
    }
    return res;
}

EngineValueManager::~EngineValueManager()
{
    ITaskQueue::WeakPtr q;
    ITaskQueue::Token token {};
    {
        std::unique_lock lock { mutex_ };
        q = queue_;
        token = task_token_;
    }
    if (token) {
        if (auto queue = q.lock()) {
            queue->CancelTask(token);
        }
    }
}

void EngineValueManager::SetNotificationQueue(const ITaskQueue::WeakPtr& q)
{
    queue_ = q;
}
static IEngineValueInternal::Ptr GetCompatibleValueInternal(IEngineValue::Ptr p, EnginePropertyParams params)
{
    IEngineValueInternal::Ptr ret;
    if (auto i = interface_pointer_cast<IEngineValueInternal>(p)) {
        if (auto acc = i->GetInternalAccess()) {
            if (acc->IsCompatible(params.property.type)) {
                ret = i;
            }
        }
    }
    return ret;
}
void EngineValueManager::AddValue(EnginePropertyParams p, EngineValueOptions options)
{
    BASE_NS::string name { p.property.name };
    if (!options.namePrefix.empty()) {
        name = options.namePrefix + "." + name;
    }
    if (auto access = META_NS::GetObjectRegistry().GetEngineData().GetInternalValueAccess(p.property.type)) {
        auto it = values_.find(name);
        if (it != values_.end()) {
            if (auto acc = GetCompatibleValueInternal(it->second.value, p)) {
                InterfaceUniqueLock valueLock { it->second.value };
                acc->SetPropertyParams(p);
                return;
            }
            values_.erase(it);
        }
        auto v = IEngineValue::Ptr(new EngineValue(name, access, p));
        v->Sync(META_NS::EngineSyncDirection::FROM_ENGINE);
        values_[name] = ValueInfo { v };
        if (options.values) {
            options.values->push_back(v);
        }
    } else {
        CORE_LOG_W("No engine internal access type registered for '%s' (required by '%s')",
            BASE_NS::string(p.property.type.name).c_str(), name.c_str());
    }
}

bool EngineValueManager::ConstructValues(CORE_NS::IPropertyHandle* handle, EngineValueOptions options)
{
    if (handle) {
        if (auto api = handle->Owner()) {
            std::unique_lock lock { mutex_ };
            for (auto& prop : api->MetaData()) {
                EnginePropertyParams params { handle, prop };
                params.pushValueToEngineDirectly = options.pushValuesDirectlyToEngine;
                AddValue(params, options);
            }
            return true;
        }
    }
    return false;
}
bool EngineValueManager::ConstructValues(IValue::Ptr value, EngineValueOptions options)
{
    auto prefix = options.namePrefix;
    if (prefix.empty()) {
        if (auto i = interface_cast<IEngineValue>(value)) {
            prefix = i->GetName();
        }
    }
    if (value->IsCompatible(UidFromType<CORE_NS::IPropertyHandle*>())) {
        if (auto p = GetValue<CORE_NS::IPropertyHandle*>(value->GetValue())) {
            return ConstructValues(
                p, EngineValueOptions { prefix, options.values, options.pushValuesDirectlyToEngine });
        }
    }
    if (auto i = interface_cast<IEngineValueInternal>(value)) {
        auto params = i->GetPropertyParams();
        for (auto&& p : params.property.metaData.memberProperties) {
            EnginePropertyParams propParams { params.handle, p, params.Offset() };
            propParams.pushValueToEngineDirectly = options.pushValuesDirectlyToEngine;
            AddValue(propParams, EngineValueOptions { prefix, options.values });
        }
    }
    return true;
}
bool EngineValueManager::ConstructValue(EnginePropertyParams property, EngineValueOptions options)
{
    std::unique_lock lock { mutex_ };
    AddValue(property, options);
    return true;
}
bool EngineValueManager::ConstructValueImpl(
    EnginePropertyParams params, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options)
{
    if (path == params.property.name) {
        if (!options.namePrefix.empty() && !pathTaken.empty()) {
            options.namePrefix += ".";
        }
        options.namePrefix += pathTaken;
        std::unique_lock lock { mutex_ };
        AddValue(params, options);
        return true;
    }
    path.remove_prefix(params.property.name.size() + 1);
    if (!pathTaken.empty()) {
        pathTaken += ".";
    }
    pathTaken += params.property.name;
    if (params.property.type == PROPERTYTYPE(CORE_NS::IPropertyHandle*)) {
        if (CORE_NS::IPropertyHandle* h = ReadValueFromEngine<CORE_NS::IPropertyHandle*>(params)) {
            return ConstructValueImpl(h, BASE_NS::move(pathTaken), path, options);
        }
    } else {
        auto root = path.substr(0, path.find_first_of('.'));
        for (auto&& p : params.property.metaData.memberProperties) {
            if (p.name == root) {
                EnginePropertyParams propParams { params.handle, p, params.Offset() };
                propParams.pushValueToEngineDirectly = options.pushValuesDirectlyToEngine;
                return ConstructValueImpl(propParams, BASE_NS::move(pathTaken), path, options);
            }
        }
    }
    return false;
}
bool EngineValueManager::ConstructValueImpl(
    CORE_NS::IPropertyHandle* handle, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options)
{
    if (!handle) {
        return false;
    }
    if (auto api = handle->Owner()) {
        auto root = path.substr(0, path.find_first_of('.'));
        if (!root.empty()) {
            for (auto& prop : api->MetaData()) {
                if (prop.name == root) {
                    EnginePropertyParams params { handle, prop };
                    params.pushValueToEngineDirectly = options.pushValuesDirectlyToEngine;
                    return ConstructValueImpl(params, BASE_NS::move(pathTaken), path, options);
                }
            }
        }
    }
    return false;
}
bool EngineValueManager::ConstructValue(
    CORE_NS::IPropertyHandle* handle, BASE_NS::string_view path, EngineValueOptions options)
{
    return ConstructValueImpl(handle, "", BASE_NS::string(path), options);
}
bool EngineValueManager::RemoveValue(BASE_NS::string_view name)
{
    std::unique_lock lock { mutex_ };
    auto it = values_.find(name);
    bool ret = it != values_.end();
    if (ret) {
        values_.erase(it);
    }
    return ret;
}
void EngineValueManager::RemoveAll()
{
    std::unique_lock lock { mutex_ };
    values_.clear();
}

IProperty::Ptr EngineValueManager::PropertyFromValue(BASE_NS::string_view name, const IEngineValue::Ptr& value) const
{
    IProperty::Ptr ret;
    if (auto internal = interface_cast<IEngineValueInternal>(value)) {
        ret = GetObjectRegistry().GetPropertyRegister().Create(META_NS::ClassId::StackProperty, name);
        if (auto i = interface_cast<IPropertyInternalAny>(ret)) {
            i->SetInternalAny(internal->GetInternalAccess()->CreateAny());
        }
        if (auto i = interface_cast<IStackProperty>(ret)) {
            i->PushValue(value);
        }
    }
    return ret;
}

IProperty::Ptr EngineValueManager::ConstructProperty(BASE_NS::string_view name) const
{
    IEngineValue::Ptr value;
    {
        std::shared_lock lock { mutex_ };
        auto it = values_.find(name);
        if (it != values_.end()) {
            value = it->second.value;
        }
    }
    return PropertyFromValue(name, value);
}

BASE_NS::vector<IProperty::Ptr> EngineValueManager::ConstructAllProperties() const
{
    BASE_NS::vector<IProperty::Ptr> ret;
    std::shared_lock lock { mutex_ };
    for (auto&& v : values_) {
        ret.push_back(PropertyFromValue(v.first, v.second.value));
    }
    return ret;
}

IEngineValue::Ptr EngineValueManager::GetEngineValue(BASE_NS::string_view name) const
{
    std::shared_lock lock { mutex_ };
    auto it = values_.find(name);
    return it != values_.end() ? it->second.value : nullptr;
}

BASE_NS::vector<IEngineValue::Ptr> EngineValueManager::GetAllEngineValues() const
{
    BASE_NS::vector<IEngineValue::Ptr> ret;
    std::shared_lock lock { mutex_ };
    for (auto&& v : values_) {
        ret.push_back(v.second.value);
    }
    return ret;
}

void EngineValueManager::NotifySyncs()
{
    BASE_NS::vector<IEngineValue::Ptr> values;
    {
        std::unique_lock lock { mutex_ };
        for (auto&& v : values_) {
            if (v.second.notify) {
                values.push_back(v.second.value);
                v.second.notify = false;
            }
        }
        task_token_ = {};
    }
    for (auto&& v : values) {
        if (auto noti = interface_cast<INotifyOnChange>(v)) {
            Invoke<IOnChanged>(noti->OnChanged());
        }
    }
}

bool EngineValueManager::Sync(EngineSyncDirection dir)
{
    bool ret = true;
    bool notify = false;
    {
        std::unique_lock lock { mutex_ };
        for (auto&& v : values_) {
            InterfaceUniqueLock valueLock { v.second.value };
            auto res = v.second.value->Sync(dir);
            if (res && res != AnyReturn::NOTHING_TO_DO) {
                v.second.notify = true;
                notify = true;
            }
            ret = ret && res;
        }
        notify = notify && !task_token_;
        if (notify && !queue_.expired()) {
            if (auto queue = queue_.lock()) {
                task_token_ = queue->AddTask(MakeCallback<ITaskQueueTask>([this] {
                    NotifySyncs();
                    return false;
                }));
            } else {
                CORE_LOG_E("Invalid task queue id");
            }
            notify = false;
        }
    }
    if (notify) {
        NotifySyncs();
    }
    return ret;
}
} // namespace Internal

META_END_NAMESPACE()
