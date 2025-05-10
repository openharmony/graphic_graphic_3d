#include "engine_value_manager.h"

#include <charconv>
#include <mutex>

#include <core/property/intf_property_api.h>
#include <core/property/scoped_handle.h>

#include <meta/api/engine/util.h>
#include <meta/api/make_callback.h>
#include <meta/ext/event_util.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

namespace Internal {

template<typename Type>
static Type ReadValueFromEngine(const EnginePropertyParams& params)
{
    Type res {};
    if (CORE_NS::ScopedHandle<const Type> guard { params.handle.Handle() }) {
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
IEngineValue::Ptr EngineValueManager::AddValue(EnginePropertyParams p, EngineValueOptions options)
{
    BASE_NS::string name { p.property.name };
    if (!options.namePrefix.empty()) {
        if (name.empty()) {
            name = options.namePrefix;
        } else {
            name = options.namePrefix + "." + name;
        }
    }
    if (auto access = META_NS::GetObjectRegistry().GetEngineData().GetInternalValueAccess(p.property.type)) {
        IEngineValue::Ptr v;
        if (auto it = values_.find(name); it != values_.end()) {
            if (auto acc = GetCompatibleValueInternal(it->second.value, p)) {
                InterfaceUniqueLock valueLock { it->second.value };
                acc->SetPropertyParams(p);
                v = it->second.value;
            } else {
                values_.erase(it);
            }
        }
        if (!v) {
            v = IEngineValue::Ptr(new EngineValue(name, access, p));
            v->Sync(META_NS::EngineSyncDirection::FROM_ENGINE);
            values_[name] = ValueInfo { v };
        }
        if (options.values) {
            options.values->push_back(v);
        }
        return v;
    }
    CORE_LOG_W("No engine internal access type registered for '%s' (required by '%s')",
        BASE_NS::string(p.property.type.name).c_str(), name.c_str());
    return nullptr;
}

bool EngineValueManager::ConstructValues(EnginePropertyHandle handle, EngineValueOptions options)
{
    if (auto rh = handle.Handle()) {
        if (auto api = rh->Owner()) {
            std::unique_lock lock { mutex_ };
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
            EnginePropertyHandle h { nullptr, {}, value };
            return ConstructValues(
                h, EngineValueOptions { prefix, options.values, options.pushValuesDirectlyToEngine });
        }
    }
    if (auto i = interface_cast<IEngineValueInternal>(value)) {
        auto params = i->GetPropertyParams();
        for (auto&& p : params.property.metaData.memberProperties) {
            EnginePropertyParams propParams { params, p };
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
    size_t index {};
    std::from_chars_result res = std::from_chars(path.data() + 1, path.data() + endPos, index);
    if (res.ec != std::errc {} || res.ptr != path.data() + endPos) {
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

    std::unique_lock lock { mutex_ };
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
                EnginePropertyHandle h { nullptr, {}, pv };
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
    return PropertyFromEngineValue(name, value);
}

BASE_NS::vector<IProperty::Ptr> EngineValueManager::ConstructAllProperties() const
{
    BASE_NS::vector<IProperty::Ptr> ret;
    std::shared_lock lock { mutex_ };
    for (auto&& v : values_) {
        ret.push_back(PropertyFromEngineValue(v.first, v.second.value));
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
            if (auto i = interface_cast<IEngineValueInternal>(v.second.value); i && i->ResetPendingNotify()) {
                values.push_back(v.second.value);
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

static AnyReturnValue SyncValue(const IEngineValue::Ptr& value, EngineSyncDirection dir)
{
    InterfaceUniqueLock valueLock { value };
    if (auto i = interface_pointer_cast<IEngineValueInternal>(value)) {
        // if this engine value depends on another one, sync the dependency first
        if (auto pv = interface_pointer_cast<IEngineValue>(i->GetPropertyParams().handle.parentValue)) {
            SyncValue(pv, dir);
        }
    }
    return value->Sync(dir);
}

bool EngineValueManager::Sync(EngineSyncDirection dir)
{
    bool ret = true;
    bool notify = false;
    {
        std::unique_lock lock { mutex_ };
        for (auto&& v : values_) {
            auto res = SyncValue(v.second.value, dir);
            if (res && res != AnyReturn::NOTHING_TO_DO) {
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
