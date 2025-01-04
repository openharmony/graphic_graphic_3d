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
#ifndef META_API_ENGINE_UTIL_H
#define META_API_ENGINE_UTIL_H

#include <meta/api/util.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

META_BEGIN_NAMESPACE()

inline void AddEngineValuesRecursively(
    const IEngineValueManager::Ptr& m, const IEngineValue::Ptr& value, EngineValueOptions options = {})
{
    BASE_NS::vector<IEngineValue::Ptr> vec;
    if (m->ConstructValues(
            value, EngineValueOptions { options.namePrefix, &vec, options.pushValuesDirectlyToEngine })) {
        if (options.values) {
            options.values->insert(options.values->end(), vec.begin(), vec.end());
        }
        for (auto&& v : vec) {
            AddEngineValuesRecursively(m, v, options);
        }
    }
}

inline void AddEngineValuesRecursively(
    const IEngineValueManager::Ptr& m, EnginePropertyHandle handle, EngineValueOptions options = {})
{
    BASE_NS::vector<IEngineValue::Ptr> vec;
    if (m->ConstructValues(
        handle, EngineValueOptions { options.namePrefix, &vec, options.pushValuesDirectlyToEngine })) {
        if (options.values) {
            options.values->insert(options.values->end(), vec.begin(), vec.end());
        }
        for (auto&& v : vec) {
            AddEngineValuesRecursively(
                m, v, EngineValueOptions { "", options.values, options.pushValuesDirectlyToEngine });
        }
    }
}

inline bool SetEngineValueToProperty(const IProperty::Ptr& p, const IEngineValue::Ptr& value)
{
    if (p && value && value->IsCompatible(p->GetTypeId())) {
        if (auto i = interface_cast<IStackProperty>(p)) {
            PropertyLock lock { p };
            TypeId ids[] = { IEngineValue::UID };
            for (auto&& v : i->GetValues(ids, false)) {
                i->RemoveValue(v);
            }
            i->PushValue(value);
            return true;
        }
    } else {
        CORE_LOG_W("Trying to attach incompatible engine value to property (%s)", p->GetName().c_str());
    }
    return false;
}

inline IEngineValue::Ptr GetEngineValueFromProperty(const IProperty::ConstPtr& p)
{
    return GetFirstValueFromProperty<IEngineValue>(p);
}

inline IProperty::Ptr PropertyFromEngineValue(BASE_NS::string_view name, const IEngineValue::Ptr& value)
{
    IProperty::Ptr ret;
    if (auto internal = interface_cast<IEngineValueInternal>(value)) {
        ret = GetObjectRegistry().GetPropertyRegister().Create(META_NS::ClassId::StackProperty, name);
        if (auto i = interface_cast<IPropertyInternalAny>(ret)) {
            i->SetInternalAny(internal->CreateAny());
        }
        if (auto i = interface_cast<IStackProperty>(ret)) {
            i->PushValue(value);
        }
    }
    return ret;
}

META_END_NAMESPACE()

#endif
