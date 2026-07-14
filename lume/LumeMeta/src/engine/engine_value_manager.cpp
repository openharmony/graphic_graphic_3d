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

#include <algorithm>
#include <charconv>
#include <mutex>

#include <core/property/intf_property_api.h>
#include <core/property/scoped_handle.h>

#include <meta/api/engine/util.h>
#include <meta/api/make_callback.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

namespace Internal {

template<typename Type>
static Type ReadValueFromEngine(const EnginePropertyParams& params)
{
    Type res{};
    if (CORE_NS::ScopedHandle<const Type> guard{params.handle.Handle()}) {
        res = *(const Type*)((uintptr_t) & *guard + params.Offset());
    }
    return res;
}

EngineValueManager::~EngineValueManager()
{
    EngineValueManager::RemoveAll();
    ITaskQueue::WeakPtr q;
    ITaskQueue::Token token{};
    {
        std::unique_lock lock{mutex_};
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

void EngineValueManager::SetDirtyNotifier(const IOnChanged::InterfaceTypePtr& notifier)
{
    dirtyNotifier_ = notifier;
}

void EngineValueManager::NotifyDirty()
{
    if (dirtyNotifier_) {
        dirtyNotifier_->Invoke();
    }
}

EngineValueManager::ValueList::iterator EngineValueManager::FindValue(BASE_NS::string_view name)
{
    auto it =
        std::lower_bound(values_.begin(), values_.end(), name, [](const ValueEntry& entry, BASE_NS::string_view n) {
            return entry.name < n;
        });
    return (it != values_.end() && it->name == name) ? it : values_.end();
}

EngineValueManager::ValueList::const_iterator EngineValueManager::FindValue(BASE_NS::string_view name) const
{
    auto it =
        std::lower_bound(values_.cbegin(), values_.cend(), name, [](const ValueEntry& entry, BASE_NS::string_view n) {
            return entry.name < n;
        });
    return (it != values_.cend() && it->name == name) ? it : values_.cend();
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
void EngineValueManager::RemoveDirtyValue(IEngineValue* value)
{
    std::lock_guard dirtyLock{dirtyList_.mutex};
    dirtyList_.values.erase(
        std::remove(dirtyList_.values.begin(), dirtyList_.values.end(), value), dirtyList_.values.end());
    static_cast<EngineValue*>(value)->ClearDirtyList();
}

IEngineValue::Ptr EngineValueManager::AddValue(EnginePropertyParams p, EngineValueOptions options)
{
    BASE_NS::string name{p.property.name};
    if (!options.namePrefix.empty()) {
        if (name.empty()) {
            name = options.namePrefix;
        } else {
            name = options.namePrefix + "." + name;
        }
    }
    if (auto access = META_NS::GetObjectRegistry().GetEngineData().GetInternalValueAccess(p.property.type)) {
        IEngineValue::Ptr v;
        auto pos =
            std::lower_bound(values_.begin(), values_.end(), name, [](const ValueEntry& entry, BASE_NS::string_view n) {
                return entry.name < n;
            });
        if (pos != values_.end() && pos->name == name) {
            if (auto acc = GetCompatibleValueInternal(pos->value, p)) {
                InterfaceUniqueLock valueLock{pos->value};
                acc->SetPropertyParams(p);
                v = pos->value;
            } else {
                RemoveDirtyValue(pos->value.get());
                pos = values_.erase(pos);
            }
        }
        if (!v) {
            auto* ev = new EngineValue(name, access, p);
            ev->SetDirtyList(&dirtyList_);
            v = IEngineValue::Ptr(ev);
            v->Sync(META_NS::EngineSyncDirection::FROM_ENGINE);
            values_.insert(pos, ValueEntry{name, v});
        }
        if (options.values) {
            options.values->push_back(v);
        }
        return v;
    }
    CORE_LOG_W("No engine internal access type registered for '%s' (required by '%s')",
        BASE_NS::string(p.property.type.name).c_str(),
        name.c_str());
    return nullptr;
}

bool EngineValueManager::ConstructValues(EnginePropertyHandle handle, EngineValueOptions options)
{
    if (auto rh = handle.Handle()) {
        if (auto api = rh->Owner()) {
            std::unique_lock lock{mutex_};
            for (auto& prop : api->MetaData()) {
                EnginePropertyParams params(handle, prop, 0);
                params.pushValueToEngineDirectly = options.pushValuesDirectlyToEngine;
                AddValue(params, options);
            }
            return true;
        }
    }
    return false;
}

bool EngineValueManager::ConstructValues(CORE_NS::IPropertyHandle* handle, EngineValueOptions options)
{
    if (handle) {
        EnginePropertyHandle h;
        h.handle = handle;
        return ConstructValues(h, options);
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
            EnginePropertyHandle h{nullptr, {}, value};
            return ConstructValues(h, EngineValueOptions{prefix, options.values, options.pushValuesDirectlyToEngine});
        }
    }
    if (auto i = interface_cast<IEngineValueInternal>(value)) {
        auto params = i->GetPropertyParams();
        for (auto&& p : params.property.metaData.memberProperties) {
            EnginePropertyParams propParams{params, p};
            propParams.pushValueToEngineDirectly = options.pushValuesDirectlyToEngine;
            AddValue(propParams, EngineValueOptions{prefix, options.values});
        }
    }
    return true;
}

bool EngineValueManager::ConstructValue(EnginePropertyParams property, EngineValueOptions options)
{
    std::unique_lock lock{mutex_};
    AddValue(property, options);
    return true;
}

bool EngineValueManager::ConstructValueImplArraySubs(
    EnginePropertyParams params, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options)
{
    if (!params.property.metaData.containerMethods) {
        CORE_LOG_W("Cannot index non-array in property path");
        return false;
    }
    auto endPos = path.find_first_of(']');
    if (endPos == BASE_NS::string_view::npos) {
        CORE_LOG_W("Invalid property path");
        return false;
    }
    size_t index{};
    std::from_chars_result res = std::from_chars(path.data() + 1, path.data() + endPos, index);
    if (res.ec != std::errc{} || res.ptr != path.data() + endPos) {
        CORE_LOG_W("Invalid property path array index");
        return false;
    }
    EnginePropertyParams elementParams;
    auto cmethods = params.property.metaData.containerMethods;
    if (params.property.type.isArray) {
        if (index >= params.property.count) {
            CORE_LOG_W("Invalid property path array index, out of range");
            return false;
        }
        elementParams = EnginePropertyParams(params, cmethods->property);
        elementParams.baseOffset += elementParams.property.size * index;
    } else {
        if (params.containerMethods) {
            CORE_LOG_W("Can only refer to one subscription over container");
            return false;
        }
        auto handle = params.handle.Handle();
        if (!handle) {
            CORE_LOG_W("Invalid property handle");
            return false;
        }
        auto data = (uintptr_t)handle->RLock();
        if (!data) {
            CORE_LOG_W("Invalid property data");
            return false;
        }
        if (index >= cmethods->size(data + params.Offset())) {
            handle->RUnlock();
            CORE_LOG_W("Invalid property path container index, out of range");
            return false;
        }
        handle->RUnlock();
        elementParams = EnginePropertyParams(params, cmethods, index);
    }
    pathTaken += path.substr(0, endPos + 1);
    path.remove_prefix(endPos + 1);
    if (path.empty()) {
        return ConstructValueImplAdd(elementParams, BASE_NS::move(pathTaken), options) != nullptr;
    }
    path.remove_prefix(1);
    return ConstructValueImpl(elementParams, BASE_NS::move(pathTaken), path, options);
}

IEngineValue::Ptr EngineValueManager::ConstructValueImplAdd(
    EnginePropertyParams params, BASE_NS::string pathTaken, EngineValueOptions options)
{
    if (!options.namePrefix.empty() && !pathTaken.empty()) {
        options.namePrefix += ".";
    }
    options.namePrefix += pathTaken;

    std::unique_lock lock{mutex_};
    return AddValue(params, options);
}

bool EngineValueManager::ConstructValueImpl(
    EnginePropertyParams params, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options)
{
    BASE_NS::string previousPath = pathTaken;
    if (path == params.property.name) {
        return ConstructValueImplAdd(params, BASE_NS::move(pathTaken), options) != nullptr;
    }
    if (!params.property.name.empty()) {
        if (!pathTaken.empty()) {
            pathTaken += ".";
        }
        pathTaken += params.property.name;
        path.remove_prefix(params.property.name.size());

        if (path.starts_with('[')) {
            return ConstructValueImplArraySubs(params, BASE_NS::move(pathTaken), path, options);
        }
        path.remove_prefix(1);
    }

    if (params.property.type == PROPERTYTYPE(CORE_NS::IPropertyHandle*)) {
        if (CORE_NS::IPropertyHandle* phandle = ReadValueFromEngine<CORE_NS::IPropertyHandle*>(params)) {
            if (auto pv = ConstructValueImplAdd(params, previousPath, options)) {
                EnginePropertyHandle h{nullptr, {}, pv};
                return ConstructValueImpl(h, BASE_NS::move(pathTaken), path, options);
            }
            CORE_LOG_W("Failed to construct parent engine value");
        }
    } else {
        auto root = path.substr(0, path.find_first_of('.'));
        for (auto&& p : params.property.metaData.memberProperties) {
            if (p.name == root) {
                EnginePropertyParams propParams(params, p);
                return ConstructValueImpl(propParams, BASE_NS::move(pathTaken), path, options);
            }
        }
    }
    return false;
}

bool EngineValueManager::ConstructValueImpl(
    EnginePropertyHandle handle, BASE_NS::string pathTaken, BASE_NS::string_view path, EngineValueOptions options)
{
    if (auto rh = handle.Handle()) {
        if (auto api = rh->Owner()) {
            auto root = path.substr(0, path.find_first_of(".["));
            if (!root.empty()) {
                for (auto& prop : api->MetaData()) {
                    if (prop.name == root) {
                        EnginePropertyParams params(handle, prop, 0);
                        params.pushValueToEngineDirectly = options.pushValuesDirectlyToEngine;
                        return ConstructValueImpl(params, BASE_NS::move(pathTaken), path, options);
                    }
                }
            }
        }
    }
    return false;
}

bool EngineValueManager::ConstructValue(
    EnginePropertyHandle handle, BASE_NS::string_view path, EngineValueOptions options)
{
    return ConstructValueImpl(handle, "", BASE_NS::string(path), options);
}

bool EngineValueManager::RemoveValue(BASE_NS::string_view name)
{
    std::unique_lock lock{mutex_};
    auto it = FindValue(name);
    bool ret = it != values_.end();
    if (ret) {
        RemoveDirtyValue(it->value.get());
        values_.erase(it);
    }
    return ret;
}

void EngineValueManager::RemoveAll()
{
    {
        std::lock_guard dirtyLock{dirtyList_.mutex};
        dirtyList_.values.clear();
    }
    std::unique_lock lock{mutex_};
    for (auto& v : values_) {
        if (auto* ev = static_cast<EngineValue*>(v.value.get())) {
            ev->ClearDirtyList();
        }
    }
    values_.clear();
}

IProperty::Ptr EngineValueManager::ConstructProperty(BASE_NS::string_view name) const
{
    IEngineValue::Ptr value;
    {
        std::shared_lock lock{mutex_};
        auto it = FindValue(name);
        if (it != values_.end()) {
            value = it->value;
        }
    }
    return PropertyFromEngineValue(name, value);
}

BASE_NS::vector<IProperty::Ptr> EngineValueManager::ConstructAllProperties() const
{
    BASE_NS::vector<IProperty::Ptr> ret;
    std::shared_lock lock{mutex_};
    for (auto&& v : values_) {
        ret.push_back(PropertyFromEngineValue(v.name, v.value));
    }
    return ret;
}

IEngineValue::Ptr EngineValueManager::GetEngineValue(BASE_NS::string_view name) const
{
    std::shared_lock lock{mutex_};
    auto it = FindValue(name);
    return it != values_.end() ? it->value : nullptr;
}

BASE_NS::vector<IEngineValue::Ptr> EngineValueManager::GetAllEngineValues() const
{
    BASE_NS::vector<IEngineValue::Ptr> ret;
    std::shared_lock lock{mutex_};
    for (auto&& v : values_) {
        ret.push_back(v.value);
    }
    return ret;
}

bool EngineValueManager::HasValues() const
{
    std::shared_lock lock{mutex_};
    return !values_.empty();
}

void EngineValueManager::NotifySyncs()
{
    BASE_NS::vector<IEngineValue::Ptr> values;
    {
        std::unique_lock lock{mutex_};
        for (auto&& v : values_) {
            if (auto i = interface_cast<IEngineValueInternal>(v.value); i && i->ResetPendingNotify()) {
                values.push_back(v.value);
            }
        }
        task_token_ = {};
    }
    for (auto&& v : values) {
        static_cast<EngineValue*>(v.get())->InvokeChangeCallbacks();
    }
}

static AnyReturnValue SyncValueParentDependency(IEngineValue* value)
{
    if (auto i = interface_cast<IEngineValueInternal>(value)) {
        if (auto pv = interface_pointer_cast<IEngineValue>(i->GetPropertyParams().handle.parentValue)) {
            InterfaceUniqueLock parentLock{pv};
            auto res = SyncValueParentDependency(pv.get());
            if (!res) {
                return res;
            }
            return pv->Sync(EngineSyncDirection::AUTO);
        }
    }
    return AnyReturn::SUCCESS;
}

static AnyReturnValue SyncValue(IEngineValue* value, EngineSyncDirection dir)
{
    InterfaceUniqueLock valueLock{value};
    auto res = SyncValueParentDependency(value);
    return res ? value->Sync(dir) : res;
}

static CORE_NS::IComponentManager* GetComponentManager(IEngineValue* value)
{
    if (auto i = interface_cast<IEngineValueInternal>(value)) {
        return i->GetPropertyParams().handle.manager;
    }
    return nullptr;
}

BASE_NS::vector<IEngineValue*> EngineValueManager::StealDirtyValues(const CORE_NS::IComponentManager* componentManager)
{
    std::lock_guard dirtyLock{dirtyList_.mutex};
    if (!componentManager) {
        // No component manager specified, return all
        return BASE_NS::move(dirtyList_.values);
    }
    BASE_NS::vector<IEngineValue*> result;
    size_t write = 0;
    // Extract matching values into result
    for (size_t i = 0; i < dirtyList_.values.size(); ++i) {
        if (Internal::GetComponentManager(dirtyList_.values[i]) == componentManager) {
            result.push_back(dirtyList_.values[i]);
        } else {
            dirtyList_.values[write++] = dirtyList_.values[i];
        }
    }
    dirtyList_.values.resize(write);
    return result;
}

EngineValueManager::SyncResult EngineValueManager::SyncDirtyValues(
    const CORE_NS::IComponentManager* componentManager, EngineSyncDirection dir)
{
    SyncResult result;
    auto dirtyValues = StealDirtyValues(componentManager);
    result.hadDirtyValues = !dirtyValues.empty();
    for (auto* ev : dirtyValues) {
        auto res = SyncValue(ev, dir);
        if (res && res != AnyReturn::NOTHING_TO_DO) {
            result.anyChanged = true;
        }
        result.allSucceeded = result.allSucceeded && res;
    }
    return result;
}

EngineValueManager::SyncResult EngineValueManager::SyncAllValues(
    const CORE_NS::IComponentManager* componentManager, EngineSyncDirection dir, bool skipUnobserved)
{
    SyncResult result;
    for (auto&& v : values_) {
        bool skip = componentManager && Internal::GetComponentManager(v.value.get()) != componentManager;
        if (!skip && skipUnobserved) {
            skip = static_cast<EngineValue*>(v.value.get())->CanSkipFromEngineSync();
        }
        if (!skip) {
            auto res = SyncValue(v.value.get(), dir);
            if (res && res != AnyReturn::NOTHING_TO_DO) {
                result.anyChanged = true;
            }
            result.allSucceeded = result.allSucceeded && res;
        }
    }
    return result;
}

void EngineValueManager::ScheduleNotifySyncs()
{
    if (queue_.expired()) {
        return;
    }
    if (auto queue = queue_.lock()) {
        task_token_ = queue->AddTask(MakeCallback<ITaskQueueTask>([this] {
            NotifySyncs();
            return false;
        }));
    } else {
        CORE_LOG_E("Invalid task queue id");
    }
}

bool EngineValueManager::Sync(EngineSyncDirection dir, const CORE_NS::IComponentManager* componentManager)
{
    bool ret = true;
    bool notify = false;
    {
        std::unique_lock lock{mutex_};

        if (dir == EngineSyncDirection::TO_ENGINE) {
            auto result = SyncDirtyValues(componentManager, dir);
            ret = result.allSucceeded;
            notify = result.anyChanged;
        } else if (dir == EngineSyncDirection::AUTO) {
            auto dirtyResult = SyncDirtyValues(componentManager, EngineSyncDirection::AUTO);
            auto allResult =
                SyncAllValues(componentManager, EngineSyncDirection::FROM_ENGINE, !dirtyResult.hadDirtyValues);
            ret = dirtyResult.allSucceeded && allResult.allSucceeded;
            notify = dirtyResult.anyChanged || allResult.anyChanged;
        } else {
            auto result = SyncAllValues(componentManager, dir, false);
            ret = result.allSucceeded;
            notify = result.anyChanged;
        }

        notify = notify && !task_token_;
        if (notify) {
            ScheduleNotifySyncs();
            notify = !task_token_;
        }
    }
    if (notify) {
        NotifySyncs();
    }
    return ret;
}
}  // namespace Internal

META_END_NAMESPACE()
